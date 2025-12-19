# GGUF Q4_K Binary Layout and Dequantization

## Overview

The `Q4_K` format is a "k-quant" type used in GGUF models. It quantizes weights to 4 bits using a super-block structure.

**Important Note:** The standard `llama.cpp` implementation defines the block size as **144 bytes** per 256 weights. The current `UniversalModelLoader.swift` defines it as 132 bytes, which appears to be incorrect (missing the 12-byte scales array).

## Binary Layout (`block_q4_K`)

Each block represents **256 weights** (`QK_K = 256`).

| Field | Type | Size | Description |
|-------|------|------|-------------|
| `d` | `ggml_half` (float16) | 2 bytes | Super-block scale for the quantized scales. |
| `dmin` | `ggml_half` (float16) | 2 bytes | Super-block scale for the quantized minimums. |
| `scales` | `uint8_t[12]` | 12 bytes | Packed 6-bit scales and minimums for the 8 sub-blocks. |
| `qs` | `uint8_t[128]` | 128 bytes | 4-bit quantized weights. Packed 2 per byte. |

**Total Size:** 2 + 2 + 12 + 128 = **144 bytes**.

### `scales` Layout (12 bytes)

The `scales` array contains 16 values (8 scales + 8 mins), each 6 bits.
- **Bytes 0-3:**
  - Lower 6 bits: `d` (scale) for sub-blocks 0-3.
  - Upper 2 bits: High bits of `d` for sub-blocks 4-7.
- **Bytes 4-7:**
  - Lower 6 bits: `m` (min) for sub-blocks 0-3.
  - Upper 2 bits: High bits of `m` for sub-blocks 4-7.
- **Bytes 8-11:**
  - Lower 4 bits: Low bits of `d` for sub-blocks 4-7.
  - Upper 4 bits: Low bits of `m` for sub-blocks 4-7.

### `qs` Layout (128 bytes)

The 256 weights are divided into 8 sub-blocks of 32 weights each.
The `qs` array stores them in interleaved pairs:
- **Bytes 0-31:** Contain weights for Sub-block 0 (low nibble) and Sub-block 1 (high nibble).
- **Bytes 32-63:** Contain weights for Sub-block 2 (low nibble) and Sub-block 3 (high nibble).
- **Bytes 64-95:** Contain weights for Sub-block 4 (low nibble) and Sub-block 5 (high nibble).
- **Bytes 96-127:** Contain weights for Sub-block 6 (low nibble) and Sub-block 7 (high nibble).

## Dequantization Algorithm

### 1. Decode Super-Block Scales
Convert `d` and `dmin` from float16 to float32.

### 2. Iterate Over Sub-Block Pairs
The 8 sub-blocks are processed in 4 iterations (pairs `j` and `j+1`).

For each pair index `is` (0, 2, 4, 6):

#### A. Extract Scales and Mins (`sc`, `m`)
Use the helper logic to unpack 6-bit values from `scales`:

```c
// Helper to extract 6-bit scale (d) and min (m) for sub-block j
void get_scale_min_k4(int j, uint8_t* scales, uint8_t* d, uint8_t* m) {
    if (j < 4) {
        *d = scales[j] & 63;
        *m = scales[j + 4] & 63;
    } else {
        *d = (scales[j + 4] & 0xF) | ((scales[j - 4] >> 6) << 4);
        *m = (scales[j + 4] >> 4) | ((scales[j - 0] >> 6) << 4);
    }
}
```

1. **Sub-block `is`:**
   - `get_scale_min_k4(is, scales, &sc, &m)`
   - `d1 = d * sc`
   - `m1 = dmin * m`

2. **Sub-block `is+1`:**
   - `get_scale_min_k4(is + 1, scales, &sc, &m)`
   - `d2 = d * sc`
   - `m2 = dmin * m`

#### B. Compute Weights
Process 32 bytes from `qs` (corresponding to 64 weights).

For `l` from 0 to 31:
- **Weight `is * 32 + l`:** `y[...] = d1 * (qs[l] & 0xF) - m1`
- **Weight `(is + 1) * 32 + l`:** `y[...] = d2 * (qs[l] >> 4) - m2`

Advance `qs` pointer by 32 bytes.

## Reference Implementation (C)

```c
void dequantize_row_q4_K(const block_q4_K * x, float * y, int64_t k) {
    const int nb = k / 256;

    for (int i = 0; i < nb; i++) {
        const uint8_t * q = x[i].qs;
        const float d   = GGML_FP16_TO_FP32(x[i].d);
        const float min = GGML_FP16_TO_FP32(x[i].dmin);

        int is = 0;
        uint8_t sc, m;
        for (int j = 0; j < 256; j += 64) {
            get_scale_min_k4(is + 0, x[i].scales, &sc, &m);
            const float d1 = d * sc; 
            const float m1 = min * m;
            
            get_scale_min_k4(is + 1, x[i].scales, &sc, &m);
            const float d2 = d * sc; 
            const float m2 = min * m;
            
            for (int l = 0; l < 32; ++l) *y++ = d1 * (q[l] & 0xF) - m1;
            for (int l = 0; l < 32; ++l) *y++ = d2 * (q[l]  >> 4) - m2;
            
            q += 32; 
            is += 2;
        }
    }
}
```
