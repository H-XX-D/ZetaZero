# Z.E.T.A. llama.cpp Implementation Status

## Overview

Z.E.T.A. (Zero Entropy Temporal Assimilation) provides extended context through:
- **Temporal Decay**: Exponential attention decay based on token age
- **Sparse Gating**: Thresholding weak attention connections
- **Memory Sublimation**: Archiving KV cache to disk with retrieval
- **Metal GPU Acceleration**: GPU-accelerated similarity search and kernels

## Current Status: WORKING

### What's Implemented

1. **Metal Kernel Infrastructure** - Custom GPU kernels for Z.E.T.A. operations
2. **Memory Management** - Async prefetch + mmap tiered storage
3. **Constitutional Lock** - SHA-256 hash verification
4. **Model Binding** - Vocabulary permutation using constitution hash
5. **GPU-Accelerated Retrieval** - Metal cosine similarity kernel
6. **Superposition Injection** - Memory attention injection into hidden state

### Performance

From testing (100 tokens on M2 Pro):
```
Total generation:     10,593 ms
- Decode time:        10,545 ms (99.5%)  <- llama.cpp inference
- Retrieval time:        26 ms (0.2%)   <- Z.E.T.A. memory (Metal GPU)

GPU retrievals:       100
Avg retrieval:        0.256 ms/token
Metal acceleration:   ACTIVE
```

## Files

### Core Z.E.T.A. (NEW)

| File | Description |
|------|-------------|
| `zeta-memory.h/c` | Memory manager with mmap tiered storage |
| `zeta-integration.h/c` | Integration layer for llama inference |
| `zeta-constitution.h/c` | Constitutional lock (SHA-256) |
| `zeta-model-bind.h/c` | Model weight binding |
| `zeta-kv-extract.h/c` | KV cache extraction |
| `zeta-metal.h/m` | Metal GPU kernel dispatch (Obj-C) |
| `zeta-kernels.metal` | Metal compute shaders |

### Metal Kernels (zeta-kernels.metal)

| Kernel | Function |
|--------|----------|
| `zeta_temporal_decay` | Z(t) = Z0 * e^(-λt) attention decay |
| `zeta_sparse_gate` | Zero sub-threshold attention |
| `zeta_attention_modifier` | Combined decay + gating |
| `zeta_generate_mask` | Pre-compute attention mask |
| `zeta_memory_injection` | O_final = O_ctx + α·O_mem |
| `zeta_sparse_softmax` | Softmax with sparse cleanup |
| `zeta_cosine_similarity` | Block similarity search |

### Demo Tool

| File | Description |
|------|-------------|
| `tools/zeta-demo/zeta-demo.cpp` | Full demo with Metal acceleration |
| `tools/zeta-demo/CMakeLists.txt` | Build config with Metal linking |

## Build Instructions

```bash
cd llama.cpp/build
cmake .. -DGGML_METAL=ON -DCMAKE_BUILD_TYPE=Release
make llama-zeta-demo -j8
```

## Usage

```bash
./bin/llama-zeta-demo \
  -m model.gguf \
  -p "Your prompt" \
  -n 100 \
  --zeta-constitution ../CONSTITUTION.txt \
  --zeta-lambda 0.01 \
  --zeta-tau 0.01
```

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--zeta-lambda` | 0.01 | Temporal decay rate (0 = disabled) |
| `--zeta-tau` | 0.01 | Sparse gating threshold |
| `--zeta-retrieve` | 0.3 | Memory retrieval similarity threshold |
| `--zeta-momentum` | 0.3 | Query momentum for prediction |
| `--zeta-storage` | /tmp/zeta | Storage directory for memory blocks |
| `--zeta-constitution` | (embedded) | Path to constitution file |

## Architecture

### Integration Points

```
llama_decode() ────────────────────> llama.cpp Metal kernels (99.5% time)
       │
       ├──> zeta_pre_decode()        Query update, prefetch trigger
       │
       └──> [after decode]
            │
            ├──> zeta_kernel_block_similarities()  GPU similarity search
            │
            ├──> zeta_inject_superposition()       Memory injection
            │
            └──> logit_bias()                      Confidence adjustment
```

### External vs Internal Integration

**Current: External (Non-invasive)**
- Works alongside llama.cpp without modification
- Memory retrieval and injection on GPU
- ~0.2% overhead

**Future: Internal (Fork Required)**
- Modify `set_input_kq_mask()` for temporal decay
- Inject directly into attention computation
- Could improve memory integration quality

## Known Issues & Fixes

### Sparse Gating NaN Prevention

The sparse gating kernel uses `-INFINITY` instead of `0` for masked positions.
This prevents NaN when all positions are masked (since -inf - (-inf) = NaN).

**Safeguard**: The current position is never masked, ensuring at least one valid attention target.

```metal
// Never mask the most recent token to prevent all-masked rows
if (score < threshold && k_idx != current_pos) {
    score = -INFINITY;
}
```

## Testing

```bash
# Basic test
./bin/llama-zeta-demo -m model.gguf -p "The capital of France is" -n 30 \
  --zeta-constitution ../CONSTITUTION.txt

# Extended generation
./bin/llama-zeta-demo -m model.gguf -p "Once upon a time" -n 500 \
  --zeta-constitution ../CONSTITUTION.txt
```

---

Z.E.T.A.™ | Patent Pending | © 2025 All rights reserved.
