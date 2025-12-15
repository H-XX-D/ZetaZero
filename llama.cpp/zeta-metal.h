// Z.E.T.A. Metal Kernel Dispatch
//
// C wrapper for Metal kernels implementing temporal decay and sparse gating
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_METAL_H
#define ZETA_METAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Z.E.T.A. Metal Context
// ============================================================================

typedef struct zeta_metal_ctx zeta_metal_ctx_t;

// Initialize Metal context for Z.E.T.A. kernels
// Returns NULL if Metal is not available
zeta_metal_ctx_t* zeta_metal_init(void);

// Free Metal context
void zeta_metal_free(zeta_metal_ctx_t* ctx);

// Check if Metal is available
bool zeta_metal_available(void);

// ============================================================================
// Temporal Decay Kernel
// ============================================================================
// Applies Z(t) = Z0 * e^(-lambda * t) to attention scores
//
// attention_scores: [seq_len, kv_len] in GPU memory
// seq_len: Number of query tokens
// kv_len: Number of key/value tokens
// current_pos: Current generation position (for computing token age)
// lambda: Decay rate (typically 0.01-0.1)

int zeta_metal_temporal_decay(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,  // GPU buffer
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda
);

// ============================================================================
// Sparse Gating Kernel (Tunneling)
// ============================================================================
// Zeros out attention weights below threshold
//
// threshold: Gating threshold (typically 0.01-0.1)

int zeta_metal_sparse_gate(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,  // GPU buffer
    int seq_len,
    int kv_len,
    float threshold
);

// ============================================================================
// Combined Attention Modifier
// ============================================================================
// Applies both temporal decay and sparse gating in one GPU pass

int zeta_metal_attention_modifier(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,  // GPU buffer
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold
);

// ============================================================================
// Generate Z.E.T.A. Attention Mask
// ============================================================================
// Creates a mask with temporal decay baked in (additive bias for pre-softmax)
//
// mask: Output buffer [seq_len, kv_len]
// causal: 1 for causal attention, 0 for bidirectional

int zeta_metal_generate_mask(
    zeta_metal_ctx_t* ctx,
    float* mask,              // GPU buffer (output)
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold,
    int causal
);

// ============================================================================
// Memory Injection (Superposition)
// ============================================================================
// Injects retrieved memory attention: O_final = O_context + alpha * O_memory
//
// output: [seq_len, dim] context output (modified in place)
// memory_output: [seq_len, dim] memory attention output
// alpha: Injection weight (0.0-1.0)

int zeta_metal_memory_injection(
    zeta_metal_ctx_t* ctx,
    float* output,            // GPU buffer (in/out)
    const float* memory_output,  // GPU buffer
    int seq_len,
    int dim,
    float alpha
);

// ============================================================================
// Sparse Softmax
// ============================================================================
// Softmax with sparse cleanup (zeros very small values)
//
// min_attention: Minimum attention value to keep after softmax

int zeta_metal_sparse_softmax(
    zeta_metal_ctx_t* ctx,
    float* scores,            // GPU buffer (in/out)
    int seq_len,
    int kv_len,
    float min_attention
);

// ============================================================================
// Cosine Similarity for Memory Retrieval
// ============================================================================
// Computes similarity between query and memory block summaries
//
// query: [dim] query vector
// summaries: [n_blocks, dim] summary vectors
// similarities: [n_blocks] output similarities

int zeta_metal_cosine_similarity(
    zeta_metal_ctx_t* ctx,
    const float* query,       // GPU buffer
    const float* summaries,   // GPU buffer
    float* similarities,      // GPU buffer (output)
    int n_blocks,
    int dim
);

// ============================================================================
// CPU Fallback Functions (when Metal unavailable)
// ============================================================================

// Apply temporal decay on CPU
void zeta_cpu_temporal_decay(
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda
);

// Apply sparse gating on CPU
void zeta_cpu_sparse_gate(
    float* attention_scores,
    int seq_len,
    int kv_len,
    float threshold
);

// Combined modifier on CPU
void zeta_cpu_attention_modifier(
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_METAL_H
