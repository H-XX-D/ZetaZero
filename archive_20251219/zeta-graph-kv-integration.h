// Z.E.T.A. Graph-KV Integration
// Bridges Graph-KV system with streaming memory retrieval
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_GRAPH_KV_INTEGRATION_H
#define ZETA_GRAPH_KV_INTEGRATION_H

#include "zeta-dual-process.h"
#include "zeta-streaming.h"

extern "C" {
#include "zeta-graph-kv.h"
}

// ============================================================================
// Global Graph-KV Context
// ============================================================================

static zeta_gkv_ctx_t* g_gkv_ctx = nullptr;

// ============================================================================
// Initialization
// ============================================================================

// Initialize Graph-KV system
// Call after model and dual-process are initialized
static inline bool zeta_gkv_integration_init(
    const struct llama_model* model,
    const char* storage_dir,
    int max_cached_segments = 128
) {
    if (g_gkv_ctx) {
        fprintf(stderr, "[GKV] Already initialized\n");
        return true;
    }

    char gkv_dir[1024];
    snprintf(gkv_dir, sizeof(gkv_dir), "%s/graph_kv", storage_dir);

    g_gkv_ctx = zeta_gkv_init(model, gkv_dir, max_cached_segments);
    if (!g_gkv_ctx) {
        fprintf(stderr, "[GKV] Failed to initialize Graph-KV\n");
        return false;
    }

    fprintf(stderr, "[GKV] Graph-KV initialized: max %d segments, storage: %s\n",
            max_cached_segments, gkv_dir);
    return true;
}

// Cleanup Graph-KV
static inline void zeta_gkv_integration_free() {
    if (g_gkv_ctx) {
        zeta_gkv_free(g_gkv_ctx);
        g_gkv_ctx = nullptr;
        fprintf(stderr, "[GKV] Graph-KV freed\n");
    }
}

// ============================================================================
// KV Capture on Node Creation
// ============================================================================

// Capture KV cache for a newly created node
// Call this after fact extraction when node has high salience
// Returns true if KV was captured, false otherwise
static inline bool zeta_gkv_capture_for_node(
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int64_t node_id,
    int32_t pos_start,
    int32_t pos_end,
    float salience_threshold = 0.8f
) {
    if (!g_gkv_ctx || !llama_ctx) return false;

    // Only capture KV for high-salience nodes
    // This avoids caching ephemeral/low-value content
    if (pos_end <= pos_start) return false;

    // Check if already cached
    zeta_gkv_segment_t* existing = zeta_gkv_find(g_gkv_ctx, node_id);
    if (existing) {
        fprintf(stderr, "[GKV] Node %lld already has cached KV\n", (long long)node_id);
        return true;
    }

    // Capture KV
    zeta_gkv_segment_t* segment = zeta_gkv_capture(
        g_gkv_ctx, llama_ctx, seq_id, pos_start, pos_end, node_id
    );

    if (segment) {
        fprintf(stderr, "[GKV] Captured KV for node %lld: %d tokens\n",
                (long long)node_id, pos_end - pos_start);
        return true;
    }

    return false;
}

// ============================================================================
// KV Injection on Retrieval
// ============================================================================

// Inject cached KV for retrieved nodes into llama context
// Call this during streaming memory retrieval
// Returns total tokens injected
static inline int zeta_gkv_inject_for_stream(
    struct llama_context* llama_ctx,
    zeta_stream_state_t* stream_state,
    llama_seq_id seq_id,
    int32_t base_pos
) {
    if (!g_gkv_ctx || !llama_ctx || !stream_state) return 0;

    int total_injected = 0;
    int32_t current_pos = base_pos;

    // Try to inject cached KV for each active node
    for (int i = 0; i < stream_state->num_active; i++) {
        int64_t node_id = stream_state->active[i].node_id;

        // Look up cached segment
        zeta_gkv_segment_t* segment = zeta_gkv_find(g_gkv_ctx, node_id);
        if (!segment) {
            // Try loading from disk
            segment = zeta_gkv_load(g_gkv_ctx, node_id);
        }

        if (segment) {
            int injected = zeta_gkv_inject(
                g_gkv_ctx, llama_ctx, segment, seq_id, current_pos
            );

            if (injected > 0) {
                // Mark node as having injected KV (skip text prefill)
                stream_state->active[i].served = true;
                total_injected += injected;
                current_pos += injected;

                fprintf(stderr, "[GKV] Injected %d cached tokens for node %lld\n",
                        injected, (long long)node_id);
            }
        }
    }

    return total_injected;
}

// ============================================================================
// Batch Operations
// ============================================================================

// Capture KV for multiple high-salience nodes
// Call after batch fact extraction
static inline int zeta_gkv_capture_batch(
    zeta_dual_ctx_t* dual_ctx,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int32_t context_start,
    float salience_threshold
) {
    if (!g_gkv_ctx || !dual_ctx || !llama_ctx) return 0;

    int captured = 0;

    // Find high-salience nodes without cached KV
    for (int i = 0; i < dual_ctx->num_nodes; i++) {
        zeta_graph_node_t* node = &dual_ctx->nodes[i];
        if (!node->is_active) continue;
        if (node->salience < salience_threshold) continue;

        // Check if already cached
        if (zeta_gkv_find(g_gkv_ctx, node->node_id)) continue;

        // Estimate token range for this node
        // Use pre-tokenized length if available
        int token_count = node->has_tokens ? node->num_tokens : 50;  // Estimate
        int32_t pos_start = context_start + i * token_count;
        int32_t pos_end = pos_start + token_count;

        // Cap at context size
        if (pos_end > context_start + 4096) continue;

        zeta_gkv_segment_t* seg = zeta_gkv_capture(
            g_gkv_ctx, llama_ctx, seq_id, pos_start, pos_end, node->node_id
        );

        if (seg) captured++;
    }

    if (captured > 0) {
        fprintf(stderr, "[GKV] Batch captured KV for %d high-salience nodes\n", captured);
    }

    return captured;
}

// ============================================================================
// Statistics
// ============================================================================

static inline void zeta_gkv_print_stats() {
    if (!g_gkv_ctx) {
        fprintf(stderr, "[GKV] Not initialized\n");
        return;
    }

    zeta_gkv_stats_t stats;
    zeta_gkv_get_stats(g_gkv_ctx, &stats);

    fprintf(stderr, "[GKV] Stats:\n");
    fprintf(stderr, "  Cached segments: %lld\n", (long long)stats.num_segments);
    fprintf(stderr, "  Memory used: %.2f MB\n", stats.total_bytes / (1024.0 * 1024.0));
    fprintf(stderr, "  Total saves: %lld\n", (long long)stats.total_saves);
    fprintf(stderr, "  Total loads: %lld\n", (long long)stats.total_loads);
    fprintf(stderr, "  Total injections: %lld\n", (long long)stats.total_injections);
    fprintf(stderr, "  Prefill time saved: %.2f sec\n", stats.prefill_skipped_ms / 1000.0);
}

// ============================================================================
// Utility
// ============================================================================

// Check if a node has cached KV
static inline bool zeta_gkv_has_cached(int64_t node_id) {
    if (!g_gkv_ctx) return false;
    return zeta_gkv_find(g_gkv_ctx, node_id) != nullptr;
}

// Get cached token count for a node
static inline int zeta_gkv_cached_tokens(int64_t node_id) {
    if (!g_gkv_ctx) return 0;
    zeta_gkv_segment_t* seg = zeta_gkv_find(g_gkv_ctx, node_id);
    if (!seg) return 0;
    return seg->header.n_tokens;
}

// Force save all dirty segments
static inline int zeta_gkv_force_flush() {
    if (!g_gkv_ctx) return 0;
    return zeta_gkv_flush(g_gkv_ctx);
}

#endif // ZETA_GRAPH_KV_INTEGRATION_H
