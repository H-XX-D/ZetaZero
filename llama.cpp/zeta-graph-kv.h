// Z.E.T.A. Graph-KV: Pre-computed KV Cache Storage
//
// Stores pre-computed KV cache states with graph nodes.
// On retrieval: Load cached KV directly into context (skip prefill).
//
// Key insight: Memoize the transformer's internal representation.
// First access computes KV (~50ms), subsequent accesses load (~1ms).
//
// Storage format: Q8_0 quantized (2x compression vs FP16)
// Position encoding: Relative positions, rebased on injection
//
// VRAM savings: 1.5GB fixed -> ~200MB dynamic
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_GRAPH_KV_H
#define ZETA_GRAPH_KV_H

#include "llama.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_GKV_MAX_LAYERS     64      // Max layers to cache
#define ZETA_GKV_MAX_TOKENS     512     // Max tokens per cached segment
#define ZETA_GKV_Q8_BLOCK_SIZE  32      // Q8_0 block size
#define ZETA_GKV_MAGIC          0x5A47  // "ZG" magic number

// ============================================================================
// Q8_0 Quantization Format
// ============================================================================

// Q8_0 block: 2 bytes scale (fp16) + 32 bytes data (int8)
typedef struct {
    uint16_t scale_fp16;                // FP16 scale factor
    int8_t   qs[ZETA_GKV_Q8_BLOCK_SIZE]; // Quantized values
} zeta_gkv_q8_block_t;

// ============================================================================
// Cached KV Segment
// ============================================================================

// Header for serialized KV segment
typedef struct {
    uint16_t magic;             // ZETA_GKV_MAGIC
    uint16_t version;           // Format version (1)
    uint32_t n_tokens;          // Number of tokens in segment
    uint32_t n_layer;           // Number of layers cached
    uint32_t n_embd_k;          // Key embedding dimension (per head)
    uint32_t n_embd_v;          // Value embedding dimension (per head)
    uint32_t n_head_kv;         // Number of KV heads
    int32_t  pos_base;          // Base position for rebasing
    uint32_t k_blocks_per_layer; // Q8 blocks per layer for keys
    uint32_t v_blocks_per_layer; // Q8 blocks per layer for values
    uint32_t data_size;         // Total size of Q8 data following header
    uint32_t checksum;          // Simple checksum for validation
} zeta_gkv_header_t;

// In-memory cached KV segment
typedef struct {
    zeta_gkv_header_t header;

    // Quantized K/V data (Q8_0 blocks)
    // Layout: [n_layer][n_tokens * n_embd / 32] blocks
    zeta_gkv_q8_block_t* k_blocks;  // Keys
    zeta_gkv_q8_block_t* v_blocks;  // Values

    // Relative positions (rebased on injection)
    int32_t* rel_positions;         // [n_tokens]

    // Metadata
    int64_t node_id;                // Graph node this belongs to
    int64_t created_at;             // Timestamp
    int64_t last_used;              // For LRU eviction
    int     use_count;              // Access count

    // Disk path for persistence
    char*   disk_path;              // Path to serialized data (NULL if in-memory only)
    bool    is_dirty;               // True if needs saving
} zeta_gkv_segment_t;

// ============================================================================
// Graph-KV Context
// ============================================================================

typedef struct {
    // Active cached segments (LRU cache)
    zeta_gkv_segment_t** segments;
    int                  num_segments;
    int                  max_segments;

    // Model dimensions (from initialization)
    int n_layer;
    int n_embd_k;
    int n_embd_v;
    int n_head_kv;

    // Storage directory for persistence
    char storage_dir[512];

    // Statistics
    int64_t total_saves;        // Segments saved to disk
    int64_t total_loads;        // Segments loaded from disk
    int64_t total_injections;   // KV injections performed
    int64_t prefill_skipped_ms; // Estimated time saved
} zeta_gkv_ctx_t;

// ============================================================================
// Initialization / Cleanup
// ============================================================================

// Create Graph-KV context
// storage_dir: Directory for persisting cached segments
// max_segments: Maximum in-memory segments (LRU eviction beyond this)
zeta_gkv_ctx_t* zeta_gkv_init(
    const struct llama_model* model,
    const char* storage_dir,
    int max_segments
);

// Free Graph-KV context (saves dirty segments first)
void zeta_gkv_free(zeta_gkv_ctx_t* ctx);

// ============================================================================
// KV Capture (on memory write)
// ============================================================================

// Capture KV cache for a token range and associate with graph node
// Extracts current KV state, quantizes to Q8, stores with node
// Returns segment pointer on success, NULL on failure
zeta_gkv_segment_t* zeta_gkv_capture(
    zeta_gkv_ctx_t* gkv_ctx,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int32_t pos_start,
    int32_t pos_end,
    int64_t node_id
);

// ============================================================================
// KV Injection (on memory retrieval)
// ============================================================================

// Inject cached KV segment into llama context
// Rebases positions relative to injection_pos
// Returns number of tokens injected, 0 on failure
int zeta_gkv_inject(
    zeta_gkv_ctx_t* gkv_ctx,
    struct llama_context* llama_ctx,
    zeta_gkv_segment_t* segment,
    llama_seq_id seq_id,
    int32_t injection_pos
);

// ============================================================================
// Segment Lookup
// ============================================================================

// Find cached segment for a graph node
// Returns NULL if not cached
zeta_gkv_segment_t* zeta_gkv_find(
    zeta_gkv_ctx_t* ctx,
    int64_t node_id
);

// Load segment from disk if not in memory
// Returns segment pointer or NULL if not found
zeta_gkv_segment_t* zeta_gkv_load(
    zeta_gkv_ctx_t* ctx,
    int64_t node_id
);

// ============================================================================
// Persistence
// ============================================================================

// Save segment to disk
int zeta_gkv_save(
    zeta_gkv_ctx_t* ctx,
    zeta_gkv_segment_t* segment
);

// Save all dirty segments
int zeta_gkv_flush(zeta_gkv_ctx_t* ctx);

// Load all segments from storage directory
int zeta_gkv_load_all(zeta_gkv_ctx_t* ctx);

// ============================================================================
// Memory Management
// ============================================================================

// Evict least-recently-used segments to free memory
// Returns number of segments evicted
int zeta_gkv_evict(zeta_gkv_ctx_t* ctx, int count);

// Free a specific segment
void zeta_gkv_segment_free(zeta_gkv_segment_t* segment);

// ============================================================================
// Quantization Helpers
// ============================================================================

// Quantize float array to Q8_0 blocks
// Returns number of blocks written
int zeta_gkv_quantize_q8(
    const float* src,
    zeta_gkv_q8_block_t* dst,
    int n_elements
);

// Dequantize Q8_0 blocks to float array
void zeta_gkv_dequantize_q8(
    const zeta_gkv_q8_block_t* src,
    float* dst,
    int n_blocks
);

// ============================================================================
// Statistics
// ============================================================================

typedef struct {
    int64_t num_segments;
    int64_t total_bytes;        // Memory used by cached segments
    int64_t total_saves;
    int64_t total_loads;
    int64_t total_injections;
    int64_t prefill_skipped_ms; // Estimated time saved by skipping prefill
} zeta_gkv_stats_t;

void zeta_gkv_get_stats(
    const zeta_gkv_ctx_t* ctx,
    zeta_gkv_stats_t* stats
);

// Debug: print segment info
void zeta_gkv_debug_segment(const zeta_gkv_segment_t* segment);

#ifdef __cplusplus
}
#endif

#endif // ZETA_GRAPH_KV_H
