// Z.E.T.A. Memory Manager
// Async prefetch + mmap tiered storage for extended context
//
// Architecture:
//   GPU VRAM (Active)  - Currently computing attention
//   Unified RAM (Warm) - Predicted + recent blocks (mmap'd, paged in)
//   NVMe (Cold)        - Everything else (mmap'd, paged out)
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_MEMORY_H
#define ZETA_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_MAX_MEMORY_BLOCKS    256   // Max archived blocks
#define ZETA_MAX_ACTIVE_BLOCKS    8     // Max blocks in GPU memory
#define ZETA_BLOCK_SIZE           512   // Tokens per block
#define ZETA_SUMMARY_DIM          4096  // Summary vector dimension (model dependent)
#define ZETA_MAX_LINKS            4     // Max graph links per block

// ============================================================================
// Data Structures
// ============================================================================

// Block types for graph structure
typedef enum {
    ZETA_BLOCK_RAW = 0,          // Standard KV cache block
    ZETA_BLOCK_INSIGHT = 1,      // Synthesized from multiple blocks
    ZETA_BLOCK_CLUSTER = 2       // Cluster head (references members)
} zeta_block_type_t;

typedef struct {
    // Summary vector for fast similarity lookup (always in RAM)
    float* summary;              // [ZETA_SUMMARY_DIM] - mean-pooled keys
    float  summary_norm;         // L2 norm of summary (cached)

    // Metadata
    int64_t block_id;            // Unique identifier
    int64_t token_start;         // First token position in original sequence
    int64_t token_count;         // Number of tokens in block
    int64_t last_access;         // Inference step of last retrieval
    float   zeta_potential;      // Current decay weight w(t)

    // Graph structure (for multi-hop retrieval)
    zeta_block_type_t block_type;           // Type of block
    int64_t links[ZETA_MAX_LINKS];          // Connected block IDs (-1 = none)
    float   link_weights[ZETA_MAX_LINKS];   // Entanglement strength of each link
    int64_t temporal_prev;                  // Previous block in time sequence (-1 = none)

    // Storage
    char*   disk_path;           // Path to serialized KV data
    void*   mmap_base;           // Original mmap pointer for munmap
    void*   mmap_kv;             // Pointer to KV data within mmap (after header+summary)
    size_t  mmap_total_size;     // Total size of mmap'd file
    size_t  mmap_kv_size;        // Size of just KV data
    bool    is_warm;             // True if paged into RAM (madvise'd)
    bool    is_active;           // True if loaded into GPU buffer
} zeta_memory_block_t;

typedef struct {
    // Block storage
    zeta_memory_block_t blocks[ZETA_MAX_MEMORY_BLOCKS];
    int num_blocks;
    int64_t next_block_id;       // Next ID to assign (ensures uniqueness across sessions)

    // Active block indices (loaded in GPU)
    int active_indices[ZETA_MAX_ACTIVE_BLOCKS];
    int num_active;

    // Momentum state for prefetching
    float* query_prev;           // Previous query vector
    float* query_curr;           // Current query vector
    float  momentum_gamma;       // Momentum coefficient (default 0.3)

    // Configuration
    float  temporal_lambda;      // Decay rate (default 0.1)
    float  retrieve_threshold;   // Similarity threshold (default 0.3)
    float  tunneling_threshold;  // Sparse gating threshold (default 0.15)
    int    summary_dim;          // Dimension of summary vectors

    // Storage paths
    char   storage_dir[512];     // Directory for archived blocks

    // Statistics
    int64_t total_retrievals;
    int64_t cache_hits;          // Block was already warm
    int64_t prefetch_hits;       // Block was prefetched

    // Scratch buffers (avoid per-call malloc in hot paths)
    void*  retrieval_scored_scratch;   // internal array sized to num_blocks
    int    retrieval_scored_capacity;  // elements allocated in scratch
} zeta_memory_ctx_t;

// ============================================================================
// Initialization / Cleanup
// ============================================================================

// Create memory context
zeta_memory_ctx_t* zeta_memory_init(
    const char* storage_dir,
    int summary_dim,
    float temporal_lambda,
    float retrieve_threshold,
    float tunneling_threshold,
    float momentum_gamma
);

// Free memory context
void zeta_memory_free(zeta_memory_ctx_t* ctx);

// ============================================================================
// Block Management (Sublimation)
// ============================================================================

// Compress and archive a KV cache block
// Returns block_id, or -1 on error
// summary_override: if not NULL, use this as the summary vector instead of computing from values
int64_t zeta_sublimate_block(
    zeta_memory_ctx_t* ctx,
    const float* keys,           // [token_count, summary_dim]
    const float* values,         // [token_count, summary_dim]
    int token_count,
    int64_t token_start
);

// Extended version with optional summary override
int64_t zeta_sublimate_block_ext(
    zeta_memory_ctx_t* ctx,
    const float* keys,
    const float* values,
    const float* summary_override,  // [summary_dim] or NULL
    int token_count,
    int64_t token_start
);

// Compute summary vector (mean pooling)
void zeta_compute_summary(
    const float* keys,
    int token_count,
    int dim,
    float* summary_out          // [dim]
);

// ============================================================================
// Cross-Session Persistence
// ============================================================================

// Load existing blocks from storage directory (called automatically on init)
// Returns number of blocks loaded
int zeta_load_existing_blocks(zeta_memory_ctx_t* ctx);

// ============================================================================
// Retrieval (Entanglement)
// ============================================================================

// Sharpened cosine similarity: ReLU(cos)^3
float zeta_entanglement_score(
    const float* query,
    const float* summary,
    int dim
);

// Find blocks above retrieval threshold
// Returns number of matches, fills indices array
int zeta_find_relevant_blocks(
    zeta_memory_ctx_t* ctx,
    const float* query,          // Current query vector
    int* indices_out,            // Output: block indices
    float* scores_out,           // Output: entanglement scores
    int max_results
);

// ============================================================================
// Prefetching (Momentum Prediction)
// ============================================================================

// Update query state and trigger prefetch
// Call this every token with the current mean query vector
void zeta_update_query_and_prefetch(
    zeta_memory_ctx_t* ctx,
    const float* query_current
);

// Compute momentum-predicted query: p = q + gamma*(q - q_prev)
void zeta_compute_prediction_vector(
    zeta_memory_ctx_t* ctx,
    float* prediction_out        // [summary_dim]
);

// Async prefetch blocks likely to be needed
// Uses madvise(MADV_WILLNEED) to page in mmap'd data
void zeta_prefetch_predicted_blocks(
    zeta_memory_ctx_t* ctx,
    const float* prediction_vector
);

// ============================================================================
// Block Loading (Superposition Preparation)
// ============================================================================

// Load block into active set (GPU-accessible memory)
// Returns pointer to KV data, or NULL on error
// Format: [2, token_count, dim] - keys then values
float* zeta_load_block(
    zeta_memory_ctx_t* ctx,
    int block_index
);

// Unload block from active set (free GPU memory)
void zeta_unload_block(
    zeta_memory_ctx_t* ctx,
    int block_index
);

// ============================================================================
// Temporal Decay
// ============================================================================

// Update zeta_potential for all blocks based on current step
// w(t) = w0 * exp(-lambda * (t - t_access))
void zeta_apply_temporal_decay(
    zeta_memory_ctx_t* ctx,
    int64_t current_step
);

// Mark block as accessed (resets decay)
void zeta_touch_block(
    zeta_memory_ctx_t* ctx,
    int block_index,
    int64_t current_step
);

// ============================================================================
// Graph Links (Multi-hop Support)
// ============================================================================

// Add a semantic link between two blocks
// Returns 0 on success, -1 if link slots full
int zeta_add_link(
    zeta_memory_ctx_t* ctx,
    int64_t from_block_id,
    int64_t to_block_id,
    float weight
);

// Remove a link between blocks
void zeta_remove_link(
    zeta_memory_ctx_t* ctx,
    int64_t from_block_id,
    int64_t to_block_id
);

// Multi-hop retrieval: follow graph links from initial matches
// Returns total blocks found (direct + linked)
int zeta_retrieve_multihop(
    zeta_memory_ctx_t* ctx,
    const float* query,
    int max_hops,                // Max graph traversal depth (1 = direct only)
    int* indices_out,
    float* scores_out,
    int max_results
);

// Find block index by ID
int zeta_find_block_by_id(
    const zeta_memory_ctx_t* ctx,
    int64_t block_id
);

// ============================================================================
// Utility
// ============================================================================

// Get statistics
void zeta_get_stats(
    const zeta_memory_ctx_t* ctx,
    int64_t* total_retrievals,
    int64_t* cache_hits,
    int64_t* prefetch_hits
);

// Debug: print block info
void zeta_debug_print_block(
    const zeta_memory_ctx_t* ctx,
    int block_index
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_MEMORY_H
