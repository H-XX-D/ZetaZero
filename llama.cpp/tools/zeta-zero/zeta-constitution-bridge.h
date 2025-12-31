// Z.E.T.A. Constitutional Bridge
//
// Bridges CPU constitution to GPU decryption kernels.
// Call zeta_constitution_bind_gpu() after initializing constitution
// to enable GPU-accelerated weight decryption.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_CONSTITUTION_BRIDGE_H
#define ZETA_CONSTITUTION_BRIDGE_H

#include "zeta-constitution.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPU Binding
// ============================================================================

// Bind constitution to GPU for CUDA weight decryption
// Call after zeta_constitution_init() or zeta_constitution_init_embedded()
// Returns 0 on success, -1 if CUDA not available or initialization failed
int zeta_constitution_bind_gpu(const zeta_constitution_t* ctx);

// Unbind GPU resources
void zeta_constitution_unbind_gpu(void);

// Check if GPU binding is active
bool zeta_constitution_gpu_ready(void);

// ============================================================================
// Unified Weight Decryption API
// ============================================================================

// These functions automatically use GPU if available, CPU fallback otherwise

typedef enum {
    ZETA_DTYPE_F32,
    ZETA_DTYPE_F16,
    ZETA_DTYPE_Q4_0,
    ZETA_DTYPE_Q8_0,
    ZETA_DTYPE_Q4_K,
    ZETA_DTYPE_Q5_K,
    ZETA_DTYPE_Q6_K
} zeta_dtype_t;

// Decrypt weights (auto-selects GPU or CPU)
// For GPU: weights must be device pointer
// For CPU: weights must be host pointer
void zeta_decrypt_weights(
    const zeta_constitution_t* ctx,
    void* weights,          // Device or host pointer
    int n,                  // Number of elements (or blocks for quantized)
    int layer_idx,          // Layer index for unique encryption
    zeta_dtype_t dtype,     // Data type
    bool is_device_ptr      // True if weights is a CUDA device pointer
);

// ============================================================================
// Model Encryption Tool
// ============================================================================

// Encrypt model weights for distribution
// Used during model preparation (not at runtime)
// Encrypted model + correct constitution = working model
// Encrypted model + wrong constitution = garbage output
int zeta_encrypt_model_weights(
    const zeta_constitution_t* ctx,
    const char* input_model_path,
    const char* output_model_path
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_CONSTITUTION_BRIDGE_H
