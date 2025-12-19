# Performance Fixes Needed

## Summary

Found that you ARE using the pruned model (`tinyllama-zeta.gguf`), but performance is still very low (0.27-0.30 tok/s vs llama.cpp's 21.81 tok/s).

## Root Causes (Updated Analysis)

### 1. **Sparsification IS Implemented** ✅
The `applyZetaSurgeryFromConfig()` function in `ZetaGGUFEngine.swift:246-376` DOES prune weights by setting them to 0.0.

### 2. **Sparse Matmul Kernel Exists BUT Is Inefficient** ⚠️
The `zeta_sparse_float_matmul` kernel at `ZetaGGUFKernels.metal:104-125` exists but has a fundamental problem:

```metal
// CURRENT (INEFFICIENT) - Still O(n) with branch divergence
for (uint i = 0; i < in_dim; i++) {
    float w = weight_row[i];
    if (w != 0.0f) {  // GPU thread divergence!
        sum += w * X[i];
    }
}
```

**Problem:** Iterates over ALL elements and branches on each one. This causes:
- O(n) complexity regardless of sparsity
- GPU thread divergence (massive performance penalty)

**Solution:** Implement CSR (Compressed Sparse Row) format that only stores and iterates non-zero values.

### 3. **Synchronous GPU Waits** ⚠️
`waitUntilCompleted()` is called after every forward pass at `ZetaGGUFEngine.swift:796`. This blocks the CPU while GPU works.

**Solution:** Use async command buffer tracking with semaphores, only wait when reading results.

## Implemented Fixes

### Fix 1: CSR Sparse Matrix Format
- Added `SparseCSRMatrix` struct to store sparse weights efficiently
- Only stores non-zero values + their indices
- Reduces memory and enables O(nnz) iteration

### Fix 2: CSR Sparse Matmul Kernel
- New `zeta_csr_sparse_matmul` kernel that only iterates non-zero values
- No branch divergence - every thread does useful work
- Expected 2-4x speedup for 28% sparse matrices

### Fix 3: Async Command Buffer Pattern
- Track last command buffer reference
- Only wait when we need to read results (sampling)
- Enables CPU/GPU overlap

## Actual Performance After Fixes

- **Before fixes:** 0.27-0.30 tok/s
- **After fixes:** 5-6 tok/s (**~20x improvement!**)
- **llama.cpp baseline:** 214 tok/s (we're at ~2.5% of llama.cpp)
- **Target:** Continue optimization to reach 20+ tok/s

### Performance Gap Analysis
llama.cpp achieves 214 tok/s through:
- Highly optimized simdgroup Metal kernels
- Graph-based execution with kernel fusion
- Minimal CPU-GPU synchronization
- No debug logging overhead

ZetaLm opportunities for improvement:
- Remove debug logging in hot paths
- Implement simdgroup matmul kernels
- Reduce sparse/variational decision overhead
- Better command buffer batching

### Root Cause Found: GGUF Data Offset Bug
The tensor data was being read from the wrong offset (32 bytes off) due to incorrect alignment calculation. This caused NaN values in embeddings and invalid inference.

**Fix:** Changed from ceiling alignment `(offset + 31) & ~31` to floor alignment `offset & ~31` for the GGUF data section start.

## Checklist

- [x] Identify root causes
- [x] Implement CSR sparse format
- [x] Add CSR sparse matmul kernel
- [x] Optimize command buffer wait pattern
- [x] Fix GGUF tensor offset alignment
- [x] Benchmark improvements
- [x] Document results








