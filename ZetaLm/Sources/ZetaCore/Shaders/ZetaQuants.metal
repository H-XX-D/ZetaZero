/**
 * ZetaQuants.metal
 * Quantization kernels ported line-by-line from ggml-quants.c
 * MUST produce identical output to llama.cpp
 */

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// Q4_0 Format: 18 bytes per 32 elements
// Layout: [scale:float16][quants:16 bytes]
// Quants: byte[i] low nibble = element[i], high nibble = element[i+16]
// ============================================================================

/// Dequantize Q4_0 block inline (for use in matmul)
inline void dequant_q4_0_block(
    device const uint8_t* block,
    thread float* out
) {
    // Read scale (float16 at bytes 0-1)
    half scale = *reinterpret_cast<device const half*>(block);

    // Dequantize 32 elements from 16 bytes
    device const uint8_t* qs = block + 2;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = qs[i];

        // GGML Standard layout:
        // Low nibble (bits 0-3) -> element i
        // High nibble (bits 4-7) -> element i + 16
        int8_t q_low = int8_t(byte & 0x0F) - 8;
        int8_t q_high = int8_t((byte >> 4) & 0x0F) - 8;

        out[i] = float(scale) * float(q_low);
        out[i + 16] = float(scale) * float(q_high);
    }
}

/// Q4_0 Matrix-Vector Multiply: y = x @ W^T
/// Input x: [1, in_dim] float
/// Weight W: [out_dim, in_dim] Q4_0 (stored as [out_dim, in_dim/32] blocks)
/// Output y: [1, out_dim] float
kernel void zeta_q4_matmul(
    device const float* x         [[buffer(0)]],
    device const uint8_t* w       [[buffer(1)]],
    device float* y               [[buffer(2)]],
    constant uint& in_dim         [[buffer(3)]],
    constant uint& out_dim        [[buffer(4)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= out_dim) return;

    // Each output element is dot product of x with one row of W
    const uint blocks_per_row = in_dim / 32;
    const uint row_offset = gid * blocks_per_row * 18;  // 18 bytes per block

    float sum = 0.0f;

    for (uint b = 0; b < blocks_per_row; b++) {
        // Dequantize block
        float dequant[32];
        dequant_q4_0_block(w + row_offset + b * 18, dequant);

        // Accumulate dot product
        uint x_offset = b * 32;
        for (int i = 0; i < 32; i++) {
            sum += x[x_offset + i] * dequant[i];
        }
    }

    y[gid] = sum;
}

// ============================================================================
// Q6_K Format: 210 bytes per 256 elements
// Layout: [ql:128][qh:64][scales:16][d:float16]
// ============================================================================

/// Dequantize entire Q6_K block to float array
kernel void zeta_q6k_dequant(
    device const uint8_t* block   [[buffer(0)]],
    device float* out             [[buffer(1)]],
    constant uint& n_blocks       [[buffer(2)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= n_blocks) return;

    const uint block_offset = gid * 210;
    const uint out_offset = gid * 256;

    device const uint8_t* ql = block + block_offset;
    device const uint8_t* qh = block + block_offset + 128;
    device const int8_t* scales = reinterpret_cast<device const int8_t*>(block + block_offset + 192);
    half d = *reinterpret_cast<device const half*>(block + block_offset + 208);

    // Two halves of 128 elements each
    for (int n = 0; n < 2; n++) {
        int ql_off = n * 64;
        int qh_off = n * 32;
        int sc_off = n * 8;
        int y_off = n * 128;

        for (int l = 0; l < 32; l++) {
            int is = l / 16;

            uint8_t ql_0 = ql[ql_off + l];
            uint8_t ql_32 = ql[ql_off + l + 32];
            uint8_t qh_val = qh[qh_off + l];

            // Read 4 scales per llama.cpp pattern
            float sc0 = float(scales[sc_off + is + 0]);
            float sc1 = float(scales[sc_off + is + 2]);
            float sc2 = float(scales[sc_off + is + 4]);
            float sc3 = float(scales[sc_off + is + 6]);

            // Extract 6-bit quants and subtract 32
            int8_t q1 = int8_t((ql_0 & 0xF) | (((qh_val >> 0) & 3) << 4)) - 32;
            int8_t q2 = int8_t((ql_32 & 0xF) | (((qh_val >> 2) & 3) << 4)) - 32;
            int8_t q3 = int8_t(((ql_0 >> 4) & 0xF) | (((qh_val >> 4) & 3) << 4)) - 32;
            int8_t q4 = int8_t(((ql_32 >> 4) & 0xF) | (((qh_val >> 6) & 3) << 4)) - 32;

            float fd = float(d);
            out[out_offset + y_off + l + 0]  = fd * sc0 * float(q1);
            out[out_offset + y_off + l + 32] = fd * sc1 * float(q2);
            out[out_offset + y_off + l + 64] = fd * sc2 * float(q3);
            out[out_offset + y_off + l + 96] = fd * sc3 * float(q4);
        }
    }
}

// ============================================================================
// Q8_0 Format: 34 bytes per 32 elements
// Layout: [scale:float16][quants:32 int8]
// ============================================================================

kernel void zeta_q8_matmul(
    device const float* x         [[buffer(0)]],
    device const uint8_t* w       [[buffer(1)]],
    device float* y               [[buffer(2)]],
    constant uint& in_dim         [[buffer(3)]],
    constant uint& out_dim        [[buffer(4)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= out_dim) return;

    const uint blocks_per_row = in_dim / 32;
    const uint row_offset = gid * blocks_per_row * 34;

    float sum = 0.0f;

    for (uint b = 0; b < blocks_per_row; b++) {
        device const uint8_t* block = w + row_offset + b * 34;
        half scale = *reinterpret_cast<device const half*>(block);
        device const int8_t* qs = reinterpret_cast<device const int8_t*>(block + 2);

        uint x_offset = b * 32;
        for (int i = 0; i < 32; i++) {
            sum += x[x_offset + i] * float(scale) * float(qs[i]);
        }
    }

    y[gid] = sum;
}

// ============================================================================
// F32 Matrix-Vector Multiply (for dequantized weights)
// ============================================================================

kernel void zeta_f32_matmul(
    device const float* x         [[buffer(0)]],
    device const float* w         [[buffer(1)]],
    device float* y               [[buffer(2)]],
    constant uint& in_dim         [[buffer(3)]],
    constant uint& out_dim        [[buffer(4)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= out_dim) return;

    float sum = 0.0f;
    const uint row_offset = gid * in_dim;

    for (uint i = 0; i < in_dim; i++) {
        sum += x[i] * w[row_offset + i];
    }

    y[gid] = sum;
}
