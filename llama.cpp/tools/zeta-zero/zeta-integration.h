// Z.E.T.A. Integration Layer for llama.cpp
// Hooks memory management into llama inference
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_INTEGRATION_H
#define ZETA_INTEGRATION_H

#include "llama.h"
#include "zeta-memory.h"
#include "zeta-constitution.h"
#include "zeta-model-bind.h"
#include "zeta-metal.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constitutional Lock - Expected Hash
// ============================================================================

// SHA-256 of CONSTITUTION.txt - model will not function without correct constitution
// Hash: c5e6454b65e7b9c694af9448174f0c54966b32b5fd55b1d01c0b4a0299653e61
static const uint8_t ZETA_CONSTITUTION_HASH[ZETA_HASH_SIZE] = {
    0xc5, 0xe6, 0x45, 0x4b, 0x65, 0xe7, 0xb9, 0xc6,
    0x94, 0xaf, 0x94, 0x48, 0x17, 0x4f, 0x0c, 0x54,
    0x96, 0x6b, 0x32, 0xb5, 0xfd, 0x55, 0xb1, 0xd0,
    0x1c, 0x0b, 0x4a, 0x02, 0x99, 0x65, 0x3e, 0x61
};

// ============================================================================
// Sublimation Policy
// ============================================================================

typedef enum {
    ZETA_SUBLIMATE_MANUAL = 0,      // Only sublimate on explicit call
    ZETA_SUBLIMATE_WINDOW = 1,      // Sublimate every N tokens (FIFO)
    ZETA_SUBLIMATE_PRESSURE = 2,    // Sublimate when KV cache near capacity
    ZETA_SUBLIMATE_ATTENTION = 3,   // Sublimate low-attention tokens (smartest)
} zeta_sublimate_policy_t;

// ============================================================================
// Z.E.T.A. Context Extension
// ============================================================================

typedef struct zeta_context {
    zeta_memory_ctx_t* memory;     // Memory manager
    struct llama_context* llama;   // Underlying llama context
    zeta_constitution_t* constitution;  // Constitutional lock (required)
    zeta_model_binding_t* binding; // Cryptographic weight binding (required)
    zeta_metal_ctx_t* metal;       // Metal GPU context (NULL if unavailable)

    // Cached contiguous block summaries for fast similarity (avoids per-step malloc/copy)
    float* block_summaries_cache;  // [cache_cap_blocks * summary_dim]
    int block_summaries_cache_dim;
    int block_summaries_cache_cap_blocks;
    int block_summaries_cache_filled_blocks;

    // Configuration
    float temporal_lambda;          // Decay rate for attention (passed to kernel)
    float tunneling_threshold;      // Sparse gating threshold (passed to kernel)
    float retrieve_threshold;       // Similarity threshold for retrieval
    float momentum_gamma;           // Query momentum coefficient

    // Sublimation policy
    zeta_sublimate_policy_t sublimate_policy;
    int sublimate_window_size;      // For WINDOW policy: sublimate every N tokens
    float sublimate_pressure_pct;   // For PRESSURE policy: threshold (e.g., 0.8 = 80%)
    float attention_decay;          // For ATTENTION policy: decay factor per step

    // Attention tracking (for ATTENTION policy)
    float* attention_scores;        // [max_kv_size] cumulative attention per position
    int attention_scores_size;      // Allocated size
    int kv_cache_used;              // Current number of tokens in KV cache

    // State
    int64_t current_step;           // Current inference step
    int sublimation_trigger;        // KV cache usage threshold to trigger sublimation
    int block_size;                 // Tokens per archived block

    // Injection buffers (for superposition)
    float* injection_buffer;        // Accumulated memory attention output
    int injection_dim;
    bool has_injection;
} zeta_context_t;

// ============================================================================
// Initialization
// ============================================================================

// Create Z.E.T.A. context wrapping a llama context
// constitution_path: Path to CONSTITUTION.txt (NULL uses embedded default)
// Returns NULL if constitution verification fails
zeta_context_t* zeta_context_init(
    struct llama_context* llama_ctx,
    const char* storage_dir,
    const char* constitution_path,  // Path to constitution (NULL for embedded)
    float temporal_lambda,       // Default: 0.1
    float tunneling_threshold,   // Default: 0.15
    float retrieve_threshold,    // Default: 0.3
    float momentum_gamma         // Default: 0.3
);

// Free Z.E.T.A. context (does NOT free underlying llama context)
void zeta_context_free(zeta_context_t* ctx);

// ============================================================================
// Core Operations
// ============================================================================

// Call before each decode step
// - Updates temporal decay
// - Checks if sublimation needed
// - Performs retrieval based on current query
void zeta_pre_decode(
    zeta_context_t* ctx,
    const float* query_vector,   // Mean of query heads for current token
    int query_dim
);

// Call after attention computation
// - Injects retrieved memory attention into output
void zeta_post_attention(
    zeta_context_t* ctx,
    float* attention_output,     // [seq_len, dim] - modified in place
    int seq_len,
    int dim
);

// Force sublimation of oldest KV cache block
// Call when KV cache is near capacity
void zeta_force_sublimation(
    zeta_context_t* ctx,
    const float* keys,           // KV cache keys to archive
    const float* values,         // KV cache values to archive
    int token_count,
    int token_start
);

// ============================================================================
// Query Helpers
// ============================================================================

// Compute mean query vector from attention heads
// For use with zeta_pre_decode
void zeta_compute_mean_query(
    const float* query_heads,    // [n_heads, head_dim]
    int n_heads,
    int head_dim,
    float* mean_out              // [head_dim * n_heads] or [model_dim]
);

// ============================================================================
// Statistics
// ============================================================================

typedef struct {
    int num_archived_blocks;
    int num_active_blocks;
    int64_t total_retrievals;
    int64_t cache_hits;
    int64_t prefetch_hits;
    float avg_retrieval_score;
} zeta_stats_t;

void zeta_get_statistics(
    const zeta_context_t* ctx,
    zeta_stats_t* stats_out
);

void zeta_print_statistics(const zeta_context_t* ctx);

// ============================================================================
// Superposition Injection
// ============================================================================

// Compute attention over a single memory block
// Returns weighted output contribution: alpha * softmax(Q @ K^T) @ V
void zeta_compute_memory_attention(
    const float* query,          // [head_dim] - current query
    const float* mem_keys,       // [token_count, head_dim] - memory keys
    const float* mem_values,     // [token_count, head_dim] - memory values
    int token_count,
    int head_dim,
    float alpha,                 // Weight based on entanglement score
    float* output                // [head_dim] - accumulated output
);

// Full superposition injection: O = O_ctx + Σ(α_k · O_mem_k)
// Call after getting context output, before final projection
void zeta_inject_superposition(
    zeta_context_t* ctx,
    const float* query,          // [n_embd] - current query/hidden state
    float* hidden_state,         // [n_embd] - modified in place
    int n_embd
);

// Simulate KV cache sublimation for testing
// Archives the given hidden states as a memory block
int64_t zeta_archive_hidden_states(
    zeta_context_t* ctx,
    const float* hidden_states,  // [token_count, n_embd]
    int token_count,
    int64_t token_start
);

// ============================================================================
// Configuration
// ============================================================================

// Update parameters at runtime
void zeta_set_temporal_lambda(zeta_context_t* ctx, float lambda);
void zeta_set_tunneling_threshold(zeta_context_t* ctx, float threshold);
void zeta_set_retrieve_threshold(zeta_context_t* ctx, float threshold);
void zeta_set_momentum_gamma(zeta_context_t* ctx, float gamma);

// Get current parameters
float zeta_get_temporal_lambda(const zeta_context_t* ctx);
float zeta_get_tunneling_threshold(const zeta_context_t* ctx);

// ============================================================================
// Sublimation Policy API
// ============================================================================

// Set sublimation policy
void zeta_set_sublimate_policy(
    zeta_context_t* ctx,
    zeta_sublimate_policy_t policy,
    int window_size,            // For WINDOW: tokens between sublimations
    float pressure_pct,         // For PRESSURE: KV usage threshold (0.0-1.0)
    float attention_decay       // For ATTENTION: decay factor per step (e.g., 0.95)
);

// Update attention scores after decode
// Call this after each decode with the attention weights from the current token
// attention_weights: [kv_length] - attention scores to all KV positions
void zeta_update_attention_scores(
    zeta_context_t* ctx,
    const float* attention_weights,
    int kv_length
);

// Check and perform automatic sublimation based on policy
// Returns number of tokens sublimated (0 if none)
int zeta_auto_sublimate(
    zeta_context_t* ctx,
    int current_kv_used,        // Current KV cache usage
    int max_kv_size             // Maximum KV cache size
);

// Get indices of tokens to evict based on attention scores
// Returns count of tokens marked for eviction
// evict_indices: output array of KV positions to evict
int zeta_get_eviction_candidates(
    const zeta_context_t* ctx,
    int target_evict_count,     // How many tokens to evict
    int* evict_indices          // Output: indices to evict
);

// Manually update KV cache used count
void zeta_set_kv_used(zeta_context_t* ctx, int count);

// ============================================================================
// Constitutional Weight Binding
// ============================================================================

// Apply constitutional binding to output logits before sampling
// CRITICAL: Call this after model forward pass, before sampling
// Without correct constitution, logits are scrambled → garbage output
void zeta_apply_output_binding(
    zeta_context_t* ctx,
    float* logits,              // [n_vocab] modified in place
    int n_vocab
);

// Transform sampled token back to true vocabulary
// Call this AFTER sampling
int32_t zeta_transform_sampled_token(
    zeta_context_t* ctx,
    int32_t sampled_token
);

// Transform input token to bound space (for prompt encoding)
int32_t zeta_transform_input_token(
    zeta_context_t* ctx,
    int32_t token
);

// Check if constitutional binding is active and verified
bool zeta_is_constitutionally_bound(const zeta_context_t* ctx);

// Print binding status
void zeta_print_binding_status(const zeta_context_t* ctx);

// ============================================================================
// Metal Kernel API (GPU-accelerated attention modifiers)
// ============================================================================

// Check if Metal acceleration is available
bool zeta_metal_is_available(const zeta_context_t* ctx);

// Apply temporal decay to attention scores using Metal (or CPU fallback)
// scores: [seq_len, kv_len] attention scores (post QK^T, pre-softmax)
int zeta_kernel_temporal_decay(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len,
    int current_pos
);

// Apply sparse gating to attention (zeros sub-threshold values)
int zeta_kernel_sparse_gate(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len
);

// Combined temporal decay + sparse gating in one pass
int zeta_kernel_attention_modifier(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len,
    int current_pos
);

// Compute memory similarity scores using Metal
// Returns similarity scores for all memory blocks
int zeta_kernel_block_similarities(
    zeta_context_t* ctx,
    const float* query,
    float* similarities,     // Output: [num_blocks]
    int dim
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_INTEGRATION_H
