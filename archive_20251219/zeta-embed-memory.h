// Z.E.T.A. Embedding-Based Memory Optimization
// Prevents HoloGit bloat through semantic deduplication and consolidation

#ifndef ZETA_EMBED_MEMORY_H
#define ZETA_EMBED_MEMORY_H

#include "zeta-embed-integration.h"
#include "zeta-dual-process.h"

#define ZETA_DEDUP_THRESHOLD 0.85f    // Skip if >85% similar to existing
#define ZETA_MERGE_THRESHOLD 0.90f    // Merge if >90% similar
#define ZETA_COLD_MOMENTUM   0.30f    // Below this = candidate for embedding-only storage

// Cache for recent embeddings to avoid re-computing
#define ZETA_EMBED_CACHE_SIZE 64
static struct {
    int64_t node_id;
    float embedding[1536];
    bool valid;
} g_embed_cache[ZETA_EMBED_CACHE_SIZE] = {0};
static int g_embed_cache_idx = 0;

// Get or compute embedding for a node
static inline bool zeta_get_node_embedding(
    zeta_dual_ctx_t* ctx,
    int64_t node_id,
    float* out_embedding
) {
    if (!ctx || !out_embedding) return false;
    
    // Check cache first
    for (int i = 0; i < ZETA_EMBED_CACHE_SIZE; i++) {
        if (g_embed_cache[i].valid && g_embed_cache[i].node_id == node_id) {
            memcpy(out_embedding, g_embed_cache[i].embedding, 1536 * sizeof(float));
            return true;
        }
    }
    
    // Find node and compute embedding
    for (int n = 0; n < ctx->num_nodes; n++) {
        if (ctx->nodes[n].node_id == node_id) {
            int dim = zeta_embed_text(ctx->nodes[n].value, out_embedding, 1536);
            if (dim > 0) {
                // Cache it
                g_embed_cache[g_embed_cache_idx].node_id = node_id;
                memcpy(g_embed_cache[g_embed_cache_idx].embedding, out_embedding, 1536 * sizeof(float));
                g_embed_cache[g_embed_cache_idx].valid = true;
                g_embed_cache_idx = (g_embed_cache_idx + 1) % ZETA_EMBED_CACHE_SIZE;
                return true;
            }
            break;
        }
    }
    return false;
}

// Check if a new fact is semantically duplicate of existing facts
// Returns node_id of duplicate if found, -1 otherwise
inline int64_t zeta_check_semantic_duplicate(
    zeta_dual_ctx_t* ctx,
    const char* new_fact,
    float threshold
) {
    if (!ctx || !new_fact || !g_embed_ctx || !g_embed_ctx->initialized) return -1;
    
    // Embed the new fact
    float new_emb[1536];
    int dim = zeta_embed_text(new_fact, new_emb, 1536);
    if (dim <= 0) return -1;
    
    float max_sim = 0.0f;
    int64_t max_id = -1;
    
    // Compare to all existing facts (raw_memory nodes)
    for (int n = 0; n < ctx->num_nodes; n++) {
        if (strcmp(ctx->nodes[n].label, "raw_memory") != 0) continue;
        
        float node_emb[1536];
        if (zeta_get_node_embedding(ctx, ctx->nodes[n].node_id, node_emb)) {
            float sim = zeta_embed_similarity(new_emb, node_emb, 1536);
            if (sim > max_sim) {
                max_sim = sim;
                max_id = ctx->nodes[n].node_id;
            }
        }
    }
    
    if (max_sim > threshold) {
        fprintf(stderr, "[DEDUP] Found %.1f%% similar fact (id=%ld), skipping duplicate\n", 
                max_sim * 100.0f, max_id);
        return max_id;
    }
    
    return -1;
}

// Find all facts similar to a query above threshold
// Returns count of similar facts found
static inline int zeta_find_similar_facts(
    zeta_dual_ctx_t* ctx,
    const char* query,
    float threshold,
    int64_t* out_ids,      // Array to store matching node IDs
    float* out_sims,       // Array to store similarity scores
    int max_results
) {
    if (!ctx || !query || !out_ids || !g_embed_ctx) return 0;
    
    float query_emb[1536];
    int dim = zeta_embed_text(query, query_emb, 1536);
    if (dim <= 0) return 0;
    
    int count = 0;
    
    for (int n = 0; n < ctx->num_nodes && count < max_results; n++) {
        float node_emb[1536];
        if (zeta_get_node_embedding(ctx, ctx->nodes[n].node_id, node_emb)) {
            float sim = zeta_embed_similarity(query_emb, node_emb, 1536);
            if (sim > threshold) {
                out_ids[count] = ctx->nodes[n].node_id;
                if (out_sims) out_sims[count] = sim;
                count++;
            }
        }
    }
    
    return count;
}

// Consolidate similar facts into a single summary node
// Returns new summary node ID, or -1 on failure
static inline int64_t zeta_consolidate_similar(
    zeta_dual_ctx_t* ctx,
    int64_t* node_ids,
    int count
) {
    if (!ctx || !node_ids || count < 2) return -1;
    
    // Build consolidated text from all similar facts
    char consolidated[2048] = {0};
    int offset = 0;
    float max_salience = 0.0f;
    
    for (int i = 0; i < count && offset < 1900; i++) {
        for (int n = 0; n < ctx->num_nodes; n++) {
            if (ctx->nodes[n].node_id == node_ids[i]) {
                if (offset > 0) {
                    offset += snprintf(consolidated + offset, 2048 - offset, " | ");
                }
                offset += snprintf(consolidated + offset, 2048 - offset, "%s", 
                                   ctx->nodes[n].value);
                if (ctx->nodes[n].salience > max_salience) {
                    max_salience = ctx->nodes[n].salience;
                }
                // Mark old node as superseded (low salience)
                ctx->nodes[n].salience = 0.1f;
                break;
            }
        }
    }
    
    // Create summary node
    int64_t summary_id = zeta_create_node(ctx, NODE_FACT, "consolidated", 
                                          consolidated, max_salience);
    
    // Create SUPERSEDES edges from summary to originals
    for (int i = 0; i < count; i++) {
        zeta_create_edge(ctx, summary_id, node_ids[i], EDGE_SUPERSEDES, 1.0f);
    }
    
    fprintf(stderr, "[CONSOLIDATE] Merged %d facts into node %ld\n", count, summary_id);
    return summary_id;
}

// Prune redundant low-momentum facts
// Returns number of facts pruned
static inline int zeta_prune_redundant(
    zeta_dual_ctx_t* ctx,
    float momentum_threshold,
    float similarity_threshold
) {
    if (!ctx || !g_embed_ctx) return 0;
    
    int pruned = 0;
    
    // Find low-momentum facts that are covered by high-momentum ones
    for (int n = 0; n < ctx->num_nodes; n++) {
        zeta_graph_node_t* node = &ctx->nodes[n];
        if (node->salience > momentum_threshold) continue;  // Not low-momentum
        if (strcmp(node->label, "raw_memory") != 0) continue;  // Only raw memories
        
        float node_emb[1536];
        if (!zeta_get_node_embedding(ctx, node->node_id, node_emb)) continue;
        
        // Check if covered by any high-momentum fact
        for (int m = 0; m < ctx->num_nodes; m++) {
            if (m == n) continue;
            zeta_graph_node_t* other = &ctx->nodes[m];
            if (other->salience <= momentum_threshold) continue;  // Also low-momentum
            
            float other_emb[1536];
            if (zeta_get_node_embedding(ctx, other->node_id, other_emb)) {
                float sim = zeta_embed_similarity(node_emb, other_emb, 1536);
                if (sim > similarity_threshold) {
                    // This low-momentum fact is covered by a high-momentum one
                    node->salience = 0.01f;  // Mark as prunable
                    fprintf(stderr, "[PRUNE] Fact %ld (%.2f sim to %ld) marked for pruning\n",
                            node->node_id, sim, other->node_id);
                    pruned++;
                    break;
                }
            }
        }
    }
    
    return pruned;
}

// Get memory stats for monitoring
static inline void zeta_memory_stats(
    zeta_dual_ctx_t* ctx,
    int* total_nodes,
    int* raw_memories,
    int* consolidated,
    int* low_momentum
) {
    if (!ctx) return;
    
    *total_nodes = ctx->num_nodes;
    *raw_memories = 0;
    *consolidated = 0;
    *low_momentum = 0;
    
    for (int n = 0; n < ctx->num_nodes; n++) {
        if (strcmp(ctx->nodes[n].label, "raw_memory") == 0) (*raw_memories)++;
        if (strcmp(ctx->nodes[n].label, "consolidated") == 0) (*consolidated)++;
        if (ctx->nodes[n].salience < ZETA_COLD_MOMENTUM) (*low_momentum)++;
    }
}

#endif // ZETA_EMBED_MEMORY_H
