# Z.E.T.A. Integration Guide for llama.cpp

## Target File
`ggml/src/ggml-metal/ggml-metal.metal`

## Key Location: Flash Attention Implementation
**Function**: `kernel_flash_attn_ext_impl` (line 5309)

### Insertion Point 1: Temporal Decay + Sparse Gating
**Location**: Lines 5670-5681 (after score scaling, before softmax)

**Original Code** (lines 5670-5681):
```metal
// scale and apply the logitcap / mask
float2 s2 = ss2[j*SH/2 + tiisg]*args.scale;

if (FC_flash_attn_ext_has_scap) {
    s2 = args.logit_softcap*precise::tanh(s2);
}

// mqk = mqk + slope*mask
if (FC_flash_attn_ext_has_bias) {
    s2 += s2_t(sm2[j*SH + tiisg])*slope;
} else {
    s2 += s2_t(sm2[j*SH + tiisg]);
}
```

**Modified Code with Z.E.T.A.**:
```metal
// scale and apply the logitcap / mask
float2 s2 = ss2[j*SH/2 + tiisg]*args.scale;

if (FC_flash_attn_ext_has_scap) {
    s2 = args.logit_softcap*precise::tanh(s2);
}

// ============================================
// Z.E.T.A. FEATURE 1: Temporal Decay
// Z(t) = Z₀·e^(-λt) implemented as additive bias in log-space
// ============================================
if (args.zeta_temporal_lambda > 0.0f) {
    const int key_pos_0 = ic + tiisg*2;
    const int key_pos_1 = ic + tiisg*2 + 1;
    const int query_pos = iq1 + j;

    s2[0] += -args.zeta_temporal_lambda * float(query_pos - key_pos_0);
    s2[1] += -args.zeta_temporal_lambda * float(query_pos - key_pos_1);
}

// ============================================
// Z.E.T.A. FEATURE 2: Sparse Gating (Tunneling)
// score < τ → -∞
// ============================================
if (args.zeta_tunneling_threshold > -1e9f) {
    if (s2[0] < args.zeta_tunneling_threshold) s2[0] = -INFINITY;
    if (s2[1] < args.zeta_tunneling_threshold) s2[1] = -INFINITY;
}

// mqk = mqk + slope*mask
if (FC_flash_attn_ext_has_bias) {
    s2 += s2_t(sm2[j*SH + tiisg])*slope;
} else {
    s2 += s2_t(sm2[j*SH + tiisg]);
}
```

## Required Struct Modifications

### 1. Add to `ggml_metal_kargs_flash_attn_ext` (ggml-metal-impl.h)
```c
typedef struct {
    // ... existing fields ...

    // Z.E.T.A. parameters
    float zeta_temporal_lambda;      // Decay rate λ (default 0.0 = disabled)
    float zeta_tunneling_threshold;  // Gating threshold τ (default -1e10 = disabled)
} ggml_metal_kargs_flash_attn_ext;
```

### 2. Add to llama.cpp model parameters (llama.h)
```c
struct llama_context_params {
    // ... existing fields ...

    // Z.E.T.A. extended context parameters
    float zeta_temporal_lambda;      // 0.0 to disable, 0.1 typical
    float zeta_tunneling_threshold;  // -1e10 to disable, 0.15 typical
};
```

## Memory Management (External Layer)

The memory compression/retrieval can be implemented as a wrapper in `llama.cpp`:

### New file: `llama-zeta.h`
```c
// Z.E.T.A. Memory Management API

typedef struct {
    float* summary_vector;  // Mean-pooled key vectors
    size_t token_count;
    char* disk_path;        // Path to serialized KV data
    float zeta_potential;   // Decay weight
    int64_t last_access;    // For temporal decay
} zeta_memory_block;

// Compress old KV cache to disk
void zeta_sublimate_block(struct llama_context* ctx, int start_pos, int block_size);

// Compute sharpened cosine similarity
float zeta_entanglement_score(const float* query, const zeta_memory_block* block, int dim);

// Retrieve and inject memory attention
void zeta_superposition_inject(struct llama_context* ctx, zeta_memory_block** blocks, int n_blocks);
```

## Build Instructions

```bash
cd llama.cpp
mkdir build && cd build
cmake .. -DLLAMA_METAL=ON
make -j
```

## Test

```bash
# Standard inference (Z.E.T.A. disabled by default)
./bin/llama-cli -m model.gguf -p "Hello" -n 50

# With Z.E.T.A. temporal decay
./bin/llama-cli -m model.gguf -p "Hello" -n 50 --zeta-lambda 0.1

# With Z.E.T.A. sparse gating
./bin/llama-cli -m model.gguf -p "Hello" -n 50 --zeta-tau 0.15
```

---

## Summary of Changes Required

| File | Change |
|------|--------|
| `ggml-metal.metal` | Add temporal decay + gating to `kernel_flash_attn_ext_impl` |
| `ggml-metal-impl.h` | Add zeta params to kargs struct |
| `ggml-metal.cpp` | Pass zeta params from context to kernel |
| `llama.h` | Add zeta params to context_params |
| `llama.cpp` | Parse CLI args, initialize zeta params |
| `llama-zeta.h/cpp` (new) | Memory compression/retrieval layer |

---

Z.E.T.A.™ | Patent Pending | © 2025 All rights reserved.
