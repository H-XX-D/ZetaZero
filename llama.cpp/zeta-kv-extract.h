// Z.E.T.A. KV Cache Extraction
//
// Utility to extract K/V tensors from llama.cpp's serialized state
// for sublimation into Z.E.T.A. memory blocks.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_KV_EXTRACT_H
#define ZETA_KV_EXTRACT_H

#include "llama.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Data Structures
// ============================================================================

typedef struct {
    // Per-layer K/V data (dequantized to float)
    float** keys;       // [n_layer][n_tokens * n_embd_k]
    float** values;     // [n_layer][n_tokens * n_embd_v]

    // Dimensions
    int n_layer;
    int n_tokens;
    int n_embd_k;       // Key embedding dimension
    int n_embd_v;       // Value embedding dimension

    // Token positions (for proper ordering)
    int32_t* positions; // [n_tokens]
} zeta_kv_data_t;

// ============================================================================
// Extraction API
// ============================================================================

// Extract KV cache data for a sequence
// Allocates memory for the output structure (caller must free with zeta_kv_data_free)
// Returns NULL on error
zeta_kv_data_t* zeta_extract_kv_cache(
    struct llama_context* ctx,
    llama_seq_id seq_id
);

// Extract KV cache for a specific token range
// token_start/end are positions within the sequence
zeta_kv_data_t* zeta_extract_kv_range(
    struct llama_context* ctx,
    llama_seq_id seq_id,
    int32_t pos_start,
    int32_t pos_end
);

// Free extracted KV data
void zeta_kv_data_free(zeta_kv_data_t* data);

// ============================================================================
// Sublimation Helper
// ============================================================================

// Convenience function: extract KV cache and sublimate to Z.E.T.A. memory
// Returns block_id on success, -1 on error
// Note: zeta_context_t is defined in zeta-integration.h

int64_t zeta_sublimate_kv_cache(
    void* zeta,  // Actually zeta_context_t*, but using void* to avoid include
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int layer_idx,          // Which layer to sublimate (-1 for mean of all)
    int32_t pos_start,
    int32_t pos_end
);

// Sublimate KV cache with explicit summary vector
// Use this when query vectors are computed differently (e.g., logits-derived)
// to ensure query and summary are in the same representation space
int64_t zeta_sublimate_kv_cache_with_summary(
    void* zeta,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int layer_idx,
    int32_t pos_start,
    int32_t pos_end,
    const float* summary,   // Summary vector (same space as queries)
    int summary_dim
);

// ============================================================================
// Utility
// ============================================================================

// Get model dimensions for KV cache
void zeta_get_kv_dimensions(
    const struct llama_model* model,
    int* n_layer_out,
    int* n_embd_k_out,
    int* n_embd_v_out,
    int* n_head_kv_out
);

// Compute mean K/V across layers (for summary vector)
void zeta_compute_mean_kv(
    const zeta_kv_data_t* data,
    float* mean_k_out,      // [n_tokens * n_embd_k]
    float* mean_v_out       // [n_tokens * n_embd_v]
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_KV_EXTRACT_H
