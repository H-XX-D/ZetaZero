// Z.E.T.A. Dual-Process Cognitive Engine
// 3B Subconscious: Memory graph ops, tunneling, staging
// 14B Conscious: Reasoning, momentum signal generation

#ifndef ZETA_DUAL_PROCESS_H
#define ZETA_DUAL_PROCESS_H

#include "llama.h"
#include "zeta-3b-extract.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Global vocab for tokenization (set by server)
static const llama_vocab* g_zeta_vocab = NULL;

static inline void zeta_set_vocab(const llama_vocab* vocab) { g_zeta_vocab = vocab; }

// Global git context for automatic versioning (set by server)
// Uses void* to avoid circular dependency with zeta-graph-git.h
static void* g_zeta_git_ctx = NULL;

static inline void zeta_set_git_ctx(void* git) { g_zeta_git_ctx = git; }

// Function pointer for automatic git commits (set by zeta-graph-git.h)
// Signature: (ctx, type, label, value, salience, source) -> node_id
typedef int64_t (*zeta_git_commit_fn)(void*, int, const char*, const char*, float, int);
static zeta_git_commit_fn g_zeta_git_commit_auto = NULL;

static inline void zeta_set_git_commit_fn(zeta_git_commit_fn fn) { g_zeta_git_commit_auto = fn; }

static inline bool zeta_tokenize_value(const char* value, int32_t* tokens, int* n_tok, int max_tok) {
    if (!g_zeta_vocab || !value || !tokens || !n_tok) return false;
    *n_tok = llama_tokenize(g_zeta_vocab, value, strlen(value), tokens, max_tok, false, false);
    return (*n_tok > 0);
}

#ifdef __cplusplus
extern "C" {
#endif

// Memory tiers based on momentum
#define ZETA_TIER_VRAM   0  // >= 0.96 momentum
#define ZETA_TIER_RAM    1  // >= 0.50 momentum  
#define ZETA_TIER_NVME   2  // < 0.50 momentum

#define ZETA_MAX_GRAPH_NODES  10000
#define ZETA_MAX_EDGES        50000
#define ZETA_MAX_HOP_DEPTH    5
#define ZETA_TUNNEL_THRESHOLD 0.3f

// Graph node types
typedef enum {
    NODE_ENTITY,      // Person, place, thing
    NODE_FACT,        // Attribute or property
    NODE_EVENT,       // Temporal occurrence
    NODE_RELATION     // Relationship type
} zeta_node_type_t;

// Graph edge types  
typedef enum {
    EDGE_IS_A,        // Type relationship
    EDGE_HAS,         // Possession
    EDGE_CREATED,     // Creator relationship
    EDGE_LIKES,       // Preference
    EDGE_RELATED,     // Generic relation
    EDGE_SUPERSEDES,  // Version update
    EDGE_TEMPORAL,    // Time relationship
    EDGE_CAUSES,      // Causal relationship (A causes B)
    EDGE_PREVENTS     // Preventive relationship (A prevents B)
} zeta_edge_type_t;

// Source tag for trust hierarchy
typedef enum {
    SOURCE_USER,    // Ground truth from user input
    SOURCE_MODEL    // Inferred from model output  
} zeta_source_t;

// Memory graph node
typedef struct {
    int64_t node_id;
    zeta_node_type_t type;
    char label[128];           // Entity name
    char value[512];           // Node value/content

    // Pre-tokenized content for direct injection
    int32_t tokens[128];       // Token IDs
    int num_tokens;            // Number of tokens
    bool has_tokens;           // True if tokenized

    float embedding[2048];     // Semantic embedding (supports up to 2048-dim models)
    float salience;            // Importance score 0-1
    int64_t created_at;
    int64_t last_accessed;
    int access_count;
    int current_tier;          // VRAM/RAM/NVME
    bool is_active;
    bool is_pinned;            // Pinned nodes cannot decay (core identity)
    zeta_source_t source;      // USER=ground truth, MODEL=inferred
    int64_t session_id;        // Session isolation
    char concept_key[64];      // For version chains (e.g., "validation_location")
    int64_t superseded_by;     // ID of newer version (0 = current)

    // Hypothetical/counterfactual support
    bool is_hypothetical;
    float hypothetical_decay;
    int64_t hypothetical_parent;
} zeta_graph_node_t;

// Memory graph edge
typedef struct {
    int64_t edge_id;
    int64_t source_id;
    int64_t target_id;
    zeta_edge_type_t type;
    float weight;              // Edge strength
    int64_t created_at;
    int version;               // For versioning
} zeta_graph_edge_t;

// Surfaced context for 14B
typedef struct {
    zeta_graph_node_t* nodes[32];
    int num_nodes;
    float relevance_scores[32];
    char formatted_context[4096];
} zeta_surfaced_context_t;

// Dual-process state
typedef struct {
    // Subconscious model (7B memory/extraction)
    llama_model* model_subconscious;
    llama_context* ctx_subconscious;
    
    // Memory graph
    zeta_graph_node_t nodes[ZETA_MAX_GRAPH_NODES];
    zeta_graph_edge_t edges[ZETA_MAX_EDGES];
    int num_nodes;
    int num_edges;
    int64_t next_node_id;
    int64_t next_edge_id;
    int64_t current_session_id;  // Active session for isolation
    
    // Momentum state (from 14B)
    float current_momentum;
    float momentum_history[64];
    int momentum_idx;
    
    // Staging queues
    int64_t vram_queue[256];   // Hot - stage to VRAM
    int64_t ram_queue[1024];   // Warm - stage to RAM
    int vram_queue_len;
    int ram_queue_len;
    
    // Storage path
    char storage_dir[512];
} zeta_dual_ctx_t;

// Forward declarations
static inline int64_t zeta_create_node_with_source(zeta_dual_ctx_t*, zeta_node_type_t, const char*, const char*, float, zeta_source_t);
static inline int64_t zeta_create_node(zeta_dual_ctx_t* ctx, zeta_node_type_t type, const char* label, const char* value, float salience) {
    return zeta_create_node_with_source(ctx, type, label, value, salience, SOURCE_USER);
}
static inline int64_t zeta_create_edge(zeta_dual_ctx_t*, int64_t, int64_t, zeta_edge_type_t, float);

// ============================================================================
// 3B Subconscious Operations
// ============================================================================

// Initialize dual-process context
static inline zeta_dual_ctx_t* zeta_dual_init(
    llama_model* model_subconscious,
    const char* storage_dir
) {
    zeta_dual_ctx_t* ctx = (zeta_dual_ctx_t*)calloc(1, sizeof(zeta_dual_ctx_t));
    if (!ctx) return NULL;
    
    ctx->model_subconscious = model_subconscious;
    ctx->next_node_id = 1;
    ctx->next_edge_id = 1;
    strncpy(ctx->storage_dir, storage_dir, sizeof(ctx->storage_dir) - 1);
    
    // Create 3B context for semantic operations
    if (model_subconscious) {
        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 8192;  // Larger context for smarter 3B semantic understanding
        cparams.n_batch = 2048;  // Larger batch for better throughput
        ctx->ctx_subconscious = llama_init_from_model(model_subconscious, cparams);
    }
    
    return ctx;
}

// Compute semantic embedding using 3B
static inline void zeta_subconscious_embed(
    zeta_dual_ctx_t* ctx,
    const char* text,
    float* embedding,
    int embed_dim
) {
    if (!ctx || !ctx->ctx_subconscious || !text || !embedding) {
        // Fallback to hash embedding if 3B not available
        memset(embedding, 0, embed_dim * sizeof(float));
        size_t len = strlen(text);
        for (size_t i = 0; i < len; i++) {
            uint32_t h = (uint32_t)text[i] * 2654435761u;
            embedding[h % embed_dim] += 1.0f;
        }
        // Normalize
        float norm = 0;
        for (int i = 0; i < embed_dim; i++) norm += embedding[i] * embedding[i];
        norm = sqrtf(norm + 1e-8f);
        for (int i = 0; i < embed_dim; i++) embedding[i] /= norm;
        return;
    }
    
    // TODO: Run 3B inference to get hidden states as embedding
    // For now, use hash embedding
    memset(embedding, 0, embed_dim * sizeof(float));
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        uint32_t h = (uint32_t)text[i] * 2654435761u;
        embedding[h % embed_dim] += 1.0f;
    }
    float norm = 0;
    for (int i = 0; i < embed_dim; i++) norm += embedding[i] * embedding[i];
    norm = sqrtf(norm + 1e-8f);
    for (int i = 0; i < embed_dim; i++) embedding[i] /= norm;
}

// Create a new graph node
// Forward declarations
static inline int64_t zeta_create_edge(zeta_dual_ctx_t* ctx, int64_t source_id, int64_t target_id, zeta_edge_type_t type, float weight);
static inline float zeta_cosine_sim_early(const float* a, const float* b, int dim);

// Early cosine similarity for deduplication (before main definition)
static inline float zeta_cosine_sim_early(const float* a, const float* b, int dim) {
    float dot = 0, norm_a = 0, norm_b = 0;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (sqrtf(norm_a) * sqrtf(norm_b) + 1e-8f);
}

// Check if label is generic (needs semantic deduplication)
static inline bool zeta_is_generic_label(const char* label) {
    return strcmp(label, "raw_memory") == 0 ||
           strcmp(label, "memory") == 0 ||
           strcmp(label, "fact") == 0 ||
           strcmp(label, "statement") == 0;
}

// Git-style versioned node creation
// If same label exists with DIFFERENT value: create new version + SUPERSEDES edge
// If same value: just update access time (no new version)
static inline int64_t zeta_create_node_with_source(
    zeta_dual_ctx_t* ctx,
    zeta_node_type_t type,
    const char* label,
    const char* value,
    float salience,
    zeta_source_t source
) {
    if (!ctx || ctx->num_nodes >= ZETA_MAX_GRAPH_NODES) return -1;
    if (!label || !value || strlen(value) == 0) return -1;

    // Pre-compute embedding for the new value (needed for semantic dedup)
    float new_embedding[2048];
    zeta_subconscious_embed(ctx, value, new_embedding, 2048);

    // Check for existing node - use SEMANTIC similarity for generic labels
    int64_t existing_id = -1;
    int existing_idx = -1;
    float best_similarity = 0.0f;
    bool is_generic = zeta_is_generic_label(label);

    for (int i = 0; i < ctx->num_nodes; i++) {
        if (!ctx->nodes[i].is_active) continue;

        // Exact label match (always check)
        if (strcmp(ctx->nodes[i].label, label) == 0) {
            // For non-generic labels, label match is enough
            if (!is_generic) {
                existing_id = ctx->nodes[i].node_id;
                existing_idx = i;
                break;
            }
            // For generic labels, check value similarity too
            float sim = zeta_cosine_sim_early(new_embedding, ctx->nodes[i].embedding, 2048);
            if (sim > best_similarity && sim > 0.80f) {
                best_similarity = sim;
                existing_id = ctx->nodes[i].node_id;
                existing_idx = i;
            }
        }
        // For generic labels, also check semantic similarity across ALL nodes
        else if (is_generic) {
            float sim = zeta_cosine_sim_early(new_embedding, ctx->nodes[i].embedding, 2048);
            if (sim > best_similarity && sim > 0.85f) {
                best_similarity = sim;
                existing_id = ctx->nodes[i].node_id;
                existing_idx = i;
                fprintf(stderr, "[3B] Semantic match: %.2f sim with node %d '%s'\n",
                        sim, i, ctx->nodes[i].label);
            }
        }
    }

    if (existing_idx >= 0) {
        // Node with same/similar content exists
        if (strcmp(ctx->nodes[existing_idx].value, value) == 0) {
            // SAME VALUE: just update access time (no new version)
            ctx->nodes[existing_idx].last_accessed = (int64_t)time(NULL);
            ctx->nodes[existing_idx].access_count++;
            fprintf(stderr, "[3B] Dedup: surfacing existing node %lld '%s'\n",
                    existing_id, ctx->nodes[existing_idx].label);
            return existing_id;
        }

        // DIFFERENT VALUE but semantically similar - Check source trust hierarchy
        zeta_source_t existing_source = ctx->nodes[existing_idx].source;

        // SOURCE_MODEL cannot override SOURCE_USER (USER is ground truth)
        if (existing_source == SOURCE_USER && source == SOURCE_MODEL) {
            fprintf(stderr, "[3B] Blocked: MODEL cannot override USER fact '%s'\n",
                    ctx->nodes[existing_idx].label);
            return existing_id;  // Keep existing USER fact
        }

        // Create new version (git-style)
        if (ctx->num_nodes >= ZETA_MAX_GRAPH_NODES) return -1;

        zeta_graph_node_t* new_node = &ctx->nodes[ctx->num_nodes];
        new_node->node_id = ctx->next_node_id++;
        new_node->type = type;
        strncpy(new_node->label, label, sizeof(new_node->label) - 1);
        strncpy(new_node->value, value, sizeof(new_node->value) - 1);
        new_node->salience = salience;
        new_node->created_at = (int64_t)time(NULL);
        new_node->last_accessed = new_node->created_at;
        new_node->session_id = ctx->current_session_id;
        new_node->access_count = 1;
        new_node->current_tier = ZETA_TIER_RAM;
        new_node->is_active = true;
        new_node->source = source;

        memcpy(new_node->embedding, new_embedding, sizeof(new_embedding));
        ctx->num_nodes++;

        // Create SUPERSEDES edge: old -> new (new version supersedes old)
        zeta_create_edge(ctx, existing_id, new_node->node_id, EDGE_SUPERSEDES, 1.0f);

        fprintf(stderr, "[3B] Version update: %s = '%s' -> '%s' (sim=%.2f, v%d)\n",
                ctx->nodes[existing_idx].label, ctx->nodes[existing_idx].value,
                value, best_similarity, ctx->num_nodes);

        return new_node->node_id;
    }
    
    // No existing node: create new
    zeta_graph_node_t* node = &ctx->nodes[ctx->num_nodes];
    node->node_id = ctx->next_node_id++;
    node->type = type;
    strncpy(node->label, label, sizeof(node->label) - 1);
    strncpy(node->value, value, sizeof(node->value) - 1);
    node->salience = salience;
    node->created_at = (int64_t)time(NULL);
    node->session_id = ctx->current_session_id;
    node->last_accessed = node->created_at;
    node->access_count = 1;
    node->current_tier = ZETA_TIER_RAM;
    node->is_active = true;
    node->source = source;
    
    memcpy(node->embedding, new_embedding, sizeof(new_embedding));  // Use pre-computed embedding
    // Pre-tokenize for direct injection
    if (g_zeta_vocab) { node->has_tokens = zeta_tokenize_value(value, node->tokens, &node->num_tokens, 128); if (node->has_tokens) fprintf(stderr, "[TOK] %d tokens: %.40s...\n", node->num_tokens, value); }
    ctx->num_nodes++;
    
    fprintf(stderr, "[3B] Created node: %s = %s (salience=%.2f)\n", 
            label, value, salience);
    
    return node->node_id;
}

// Smart commit: routes through GitGraph when enabled, otherwise direct storage
// This provides automatic domain branching and version tracking invisibly
static inline int64_t zeta_commit_fact(
    zeta_dual_ctx_t* ctx,
    zeta_node_type_t type,
    const char* label,
    const char* value,
    float salience,
    zeta_source_t source
) {
    // If GitGraph is wired up, use automatic domain-based branching
    if (g_zeta_git_ctx && g_zeta_git_commit_auto) {
        int64_t node_id = g_zeta_git_commit_auto(g_zeta_git_ctx, (int)type, label, value, salience, (int)source);
        if (node_id >= 0) {
            fprintf(stderr, "[GIT-AUTO] Committed to domain branch: %s = %.40s...\n", label, value);
            return node_id;
        }
    }
    // Fallback to direct storage
    return zeta_create_node_with_source(ctx, type, label, value, salience, source);
}

// Create edge between nodes
static inline int64_t zeta_create_edge(
    zeta_dual_ctx_t* ctx,
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type,
    float weight
) {
    if (!ctx || ctx->num_edges >= ZETA_MAX_EDGES) return -1;
    
    zeta_graph_edge_t* edge = &ctx->edges[ctx->num_edges];
    edge->edge_id = ctx->next_edge_id++;
    edge->source_id = source_id;
    edge->target_id = target_id;
    edge->type = type;
    edge->weight = weight;
    edge->created_at = (int64_t)time(NULL);
    edge->version = 1;
    
    ctx->num_edges++;
    return edge->edge_id;
}

// ============================================================================
// Edge Deduplication and Pruning
// ============================================================================

// Find existing edge between source and target with same type
static inline int zeta_find_edge(
    zeta_dual_ctx_t* ctx,
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type
) {
    if (!ctx) return -1;
    for (int i = 0; i < ctx->num_edges; i++) {
        if (ctx->edges[i].source_id == source_id &&
            ctx->edges[i].target_id == target_id &&
            ctx->edges[i].type == type) {
            return i;
        }
    }
    return -1;
}

// Create edge only if it doesn't already exist, otherwise update weight
static inline int64_t zeta_create_edge_dedup(
    zeta_dual_ctx_t* ctx,
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type,
    float weight
) {
    if (!ctx) return -1;

    // Check for existing edge
    int existing = zeta_find_edge(ctx, source_id, target_id, type);
    if (existing >= 0) {
        // Reinforce existing edge (weighted average, cap at 1.0)
        float new_weight = ctx->edges[existing].weight * 0.7f + weight * 0.3f;
        if (new_weight > 1.0f) new_weight = 1.0f;
        ctx->edges[existing].weight = new_weight;
        ctx->edges[existing].version++;
        return ctx->edges[existing].edge_id;
    }

    // Create new edge
    return zeta_create_edge(ctx, source_id, target_id, type, weight);
}

// Prune low-weight edges (weight < threshold)
static inline int zeta_prune_edges(
    zeta_dual_ctx_t* ctx,
    float weight_threshold,
    int max_prune
) {
    if (!ctx || ctx->num_edges == 0) return 0;

    int pruned = 0;
    int i = 0;

    while (i < ctx->num_edges && pruned < max_prune) {
        // Don't prune SUPERSEDES edges (versioning)
        if (ctx->edges[i].type == EDGE_SUPERSEDES) {
            i++;
            continue;
        }

        if (ctx->edges[i].weight < weight_threshold) {
            // Remove by shifting remaining edges
            for (int j = i; j < ctx->num_edges - 1; j++) {
                ctx->edges[j] = ctx->edges[j + 1];
            }
            ctx->num_edges--;
            pruned++;
            // Don't increment i - check the shifted element
        } else {
            i++;
        }
    }

    if (pruned > 0) {
        fprintf(stderr, "[GRAPH:PRUNE] Removed %d low-weight edges (threshold=%.2f)\n",
                pruned, weight_threshold);
    }

    return pruned;
}

// Apply weight decay to all edges (for aging)
static inline void zeta_decay_edges(
    zeta_dual_ctx_t* ctx,
    float decay_factor
) {
    if (!ctx) return;
    for (int i = 0; i < ctx->num_edges; i++) {
        // Don't decay SUPERSEDES edges
        if (ctx->edges[i].type != EDGE_SUPERSEDES) {
            ctx->edges[i].weight *= decay_factor;
        }
    }
}

// Version a fact (supersede old with new)
static inline int64_t zeta_version_fact(
    zeta_dual_ctx_t* ctx,
    const char* entity,
    const char* old_value,
    const char* new_value
) {
    int64_t old_node_id = -1;
    
    // Find old node
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (ctx->nodes[i].is_active &&
            strcmp(ctx->nodes[i].label, entity) == 0 &&
            strcmp(ctx->nodes[i].value, old_value) == 0) {
            old_node_id = ctx->nodes[i].node_id;
            break;
        }
    }
    
    // Create new node
    int64_t new_node_id = zeta_create_node(ctx, NODE_FACT, entity, new_value, 0.95f);
    
    // Create supersedes edge
    if (old_node_id > 0 && new_node_id > 0) {
        zeta_create_edge(ctx, old_node_id, new_node_id, EDGE_SUPERSEDES, 1.0f);
        fprintf(stderr, "[3B] Versioned: %s: %s -> %s\n", entity, old_value, new_value);
    }
    
    return new_node_id;
}

// ============================================================================
// Graph Traversal and Tunneling
// ============================================================================

// Cosine similarity between embeddings
static inline float zeta_cosine_sim(const float* a, const float* b, int dim) {
    float dot = 0, norm_a = 0, norm_b = 0;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (sqrtf(norm_a) * sqrtf(norm_b) + 1e-8f);
}

// Tunnel to relevant nodes (sparse attention hop)
static inline int zeta_tunnel(
    zeta_dual_ctx_t* ctx,
    const float* query_embed,
    zeta_graph_node_t** results,
    float* scores,
    int max_results
) {
    if (!ctx || !query_embed || !results || !scores) return 0;
    
    // Score all nodes by similarity
    typedef struct { int idx; float score; } scored_t;
    scored_t* scored = (scored_t*)malloc(ctx->num_nodes * sizeof(scored_t));
    int n_scored = 0;
    
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (!ctx->nodes[i].is_active) continue;
        
        float sim = zeta_cosine_sim(query_embed, ctx->nodes[i].embedding, 2048);
        
        // Apply salience boost
        sim *= (0.5f + 0.5f * ctx->nodes[i].salience);
        
        if (sim >= ZETA_TUNNEL_THRESHOLD) {
            scored[n_scored].idx = i;
            scored[n_scored].score = sim;
            n_scored++;
        }
    }
    
    // Sort by score (simple bubble sort for small n)
    for (int i = 0; i < n_scored - 1; i++) {
        for (int j = i + 1; j < n_scored; j++) {
            if (scored[j].score > scored[i].score) {
                scored_t tmp = scored[i];
                scored[i] = scored[j];
                scored[j] = tmp;
            }
        }
    }
    
    // Return top results
    int count = (n_scored < max_results) ? n_scored : max_results;
    for (int i = 0; i < count; i++) {
        results[i] = &ctx->nodes[scored[i].idx];
        scores[i] = scored[i].score;
        ctx->nodes[scored[i].idx].last_accessed = (int64_t)time(NULL);
        ctx->nodes[scored[i].idx].access_count++;
    }
    
    free(scored);
    return count;
}

// Graph hop - follow edges from a node
static inline int zeta_graph_hop(
    zeta_dual_ctx_t* ctx,
    int64_t start_node_id,
    zeta_graph_node_t** results,
    int max_results,
    int max_depth
) {
    if (!ctx || !results || max_depth <= 0) return 0;
    
    int count = 0;
    int64_t visited[256] = {0};
    int num_visited = 0;
    
    // BFS from start node
    int64_t queue[256];
    int depths[256];
    int q_head = 0, q_tail = 0;
    
    queue[q_tail] = start_node_id;
    depths[q_tail++] = 0;
    
    while (q_head < q_tail && count < max_results) {
        int64_t curr_id = queue[q_head];
        int curr_depth = depths[q_head++];
        
        // Check if visited
        bool found = false;
        for (int i = 0; i < num_visited; i++) {
            if (visited[i] == curr_id) { found = true; break; }
        }
        if (found) continue;
        visited[num_visited++] = curr_id;
        
        // Find node and add to results
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (ctx->nodes[i].node_id == curr_id && ctx->nodes[i].is_active) {
                results[count++] = &ctx->nodes[i];
                break;
            }
        }
        
        if (curr_depth >= max_depth) continue;
        
        // Follow edges
        for (int i = 0; i < ctx->num_edges; i++) {
            zeta_graph_edge_t* e = &ctx->edges[i];
            int64_t next_id = -1;
            
            if (e->source_id == curr_id) next_id = e->target_id;
            else if (e->target_id == curr_id) next_id = e->source_id;
            
            if (next_id > 0 && q_tail < 256) {
                queue[q_tail] = next_id;
                depths[q_tail++] = curr_depth + 1;
            }
        }
    }
    
    return count;
}

// ============================================================================
// Momentum-based Staging
// ============================================================================

// Update momentum from 14B attention signal
static inline void zeta_update_momentum(zeta_dual_ctx_t* ctx, float momentum) {
    if (!ctx) return;
    
    ctx->momentum_history[ctx->momentum_idx % 64] = momentum;
    ctx->momentum_idx++;
    
    // Compute smoothed momentum
    float sum = 0;
    int count = (ctx->momentum_idx < 64) ? ctx->momentum_idx : 64;
    for (int i = 0; i < count; i++) {
        sum += ctx->momentum_history[i];
    }
    ctx->current_momentum = sum / count;
}

// Stage nodes based on momentum
static inline void zeta_stage_by_momentum(
    zeta_dual_ctx_t* ctx,
    zeta_graph_node_t** nodes,
    int num_nodes
) {
    if (!ctx || !nodes) return;
    
    ctx->vram_queue_len = 0;
    ctx->ram_queue_len = 0;
    
    for (int i = 0; i < num_nodes; i++) {
        zeta_graph_node_t* node = nodes[i];
        if (!node) continue;
        
        // Compute effective momentum for this node
        float eff_momentum = ctx->current_momentum * node->salience;
        
        if (eff_momentum >= 0.96f) {
            // Stage to VRAM (L0)
            node->current_tier = ZETA_TIER_VRAM;
            if (ctx->vram_queue_len < 256) {
                ctx->vram_queue[ctx->vram_queue_len++] = node->node_id;
            }
            fprintf(stderr, "[3B] VRAM staging: %s (mom=%.2f)\n", 
                    node->label, eff_momentum);
        } else if (eff_momentum >= 0.50f) {
            // Stage to RAM (L1)
            node->current_tier = ZETA_TIER_RAM;
            if (ctx->ram_queue_len < 1024) {
                ctx->ram_queue[ctx->ram_queue_len++] = node->node_id;
            }
        } else {
            // Leave on NVMe (L2)
            node->current_tier = ZETA_TIER_NVME;
        }
    }
}

// ============================================================================
// Surface Context for 14B
// ============================================================================

// Surface relevant context from graph
// Find node by ID
static inline zeta_graph_node_t* zeta_find_node_by_id(
    zeta_dual_ctx_t* ctx,
    int64_t node_id
) {
    if (!ctx) return NULL;
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (ctx->nodes[i].is_active && ctx->nodes[i].node_id == node_id) {
            return &ctx->nodes[i];
        }
    }
    return NULL;
}

// Follow SUPERSEDES edges to get newest version of a node
static inline zeta_graph_node_t* zeta_get_newest_version(
    zeta_dual_ctx_t* ctx,
    zeta_graph_node_t* node
) {
    if (!ctx || !node) return node;
    
    zeta_graph_node_t* current = node;
    int max_hops = 10;  // Prevent infinite loops
    
    while (max_hops-- > 0) {
        bool found_newer = false;
        // Look for SUPERSEDES edge where current is source (old -> new)
        for (int i = 0; i < ctx->num_edges; i++) {
            if (ctx->edges[i].source_id == current->node_id &&
                ctx->edges[i].type == EDGE_SUPERSEDES) {
                // Found newer version
                zeta_graph_node_t* newer = zeta_find_node_by_id(ctx, ctx->edges[i].target_id);
                if (newer && newer->is_active) {
                    current = newer;
                    found_newer = true;
                    break;
                }
            }
        }
        if (!found_newer) break;  // No newer version, we have the latest
    }
    
    return current;
}

static inline void zeta_surface_context(
    zeta_dual_ctx_t* ctx,
    const char* query,
    zeta_surfaced_context_t* out
) {
    if (!ctx || !query || !out) return;
    
    memset(out, 0, sizeof(zeta_surfaced_context_t));
    
    // Compute query embedding
    float query_embed[2048];
    zeta_subconscious_embed(ctx, query, query_embed, 2048);
    
    // Tunnel to relevant nodes
    out->num_nodes = zeta_tunnel(ctx, query_embed, out->nodes, 
                                  out->relevance_scores, 16);
    
    // For each tunneled node, do graph hops to get related context
    zeta_graph_node_t* hop_results[32];
    int total_nodes = out->num_nodes;
    
    for (int i = 0; i < out->num_nodes && total_nodes < 32; i++) {
        int hop_count = zeta_graph_hop(ctx, out->nodes[i]->node_id, 
                                        hop_results, 8, 2);
        for (int j = 0; j < hop_count && total_nodes < 32; j++) {
            // Check not already in results
            bool found = false;
            for (int k = 0; k < total_nodes; k++) {
                if (out->nodes[k] == hop_results[j]) { found = true; break; }
            }
            if (!found) {
                out->nodes[total_nodes] = hop_results[j];
                out->relevance_scores[total_nodes] = 0.5f;  // Lower score for hops
                total_nodes++;
            }
        }
    }
    out->num_nodes = total_nodes;
    
    // Stage based on current momentum
    // Session boost: prefer current session nodes
    for (int i = 0; i < total_nodes; i++) {
        if (out->nodes[i]->session_id == ctx->current_session_id) {
            out->relevance_scores[i] *= 1.5f;  // 50% boost for current session
        } else if (out->nodes[i]->session_id > 0) {
            out->relevance_scores[i] *= 0.7f;  // 30% penalty for old sessions
        }
        // Version chain: heavily penalize superseded nodes (prefer current truth)
        if (out->nodes[i]->superseded_by != 0) {
            out->relevance_scores[i] *= 0.1f;  // 90% penalty for superseded nodes
        }
    }
    zeta_stage_by_momentum(ctx, out->nodes, out->num_nodes);
    
    // Upgrade each node to its newest version
    for (int i = 0; i < out->num_nodes; i++) {
        out->nodes[i] = zeta_get_newest_version(ctx, out->nodes[i]);
    }

    // Format context for 14B
    char* p = out->formatted_context;
    int remaining = sizeof(out->formatted_context) - 1;
    
    if (out->num_nodes > 0) {
        int n = snprintf(p, remaining, "[Memory Context]\n");
        p += n; remaining -= n;
        
        for (int i = 0; i < out->num_nodes && remaining > 100; i++) {
            zeta_graph_node_t* node = out->nodes[i];
            n = snprintf(p, remaining, "- %s: %s (relevance=%.2f)\n",
                        node->label, node->value, out->relevance_scores[i]);
            p += n; remaining -= n;
        }
        
        snprintf(p, remaining, "[End Memory]\n\n");
    }
}

// ============================================================================
// Semantic Fact Extraction (3B-powered)
// ============================================================================

// Extract facts using 3B semantic analysis  
static inline int zeta_subconscious_extract_facts(
    zeta_dual_ctx_t* ctx,
    const char* text
) {
    if (!ctx || !text) return 0;
    
    int facts_created = 0;
    
    // DEBUG: What text arrives?
    fprintf(stderr, "[EXTRACT DEBUG] Text starts with: %.40s...\n", text);
    
    // ===== REMEMBER SHORT-CIRCUIT =====
    // "Remember:" prefix stores EXACTLY what follows
    if (strncasecmp(text, "remember:", 9) == 0) {
        const char* content = text + 9;
        while (*content == ' ') content++;  // Skip spaces
        
        if (strlen(content) > 5) {
            // Store as raw_memory with high salience (routes through GitGraph if enabled)
            zeta_commit_fact(ctx, NODE_FACT, "raw_memory", content, 0.95f, SOURCE_USER);
            facts_created++;
            fprintf(stderr, "[REMEMBER] Direct storage: %.60s...\n", content);

            // Also extract causal relations from the content
            char lower_content[2048];
            int clen = strlen(content);
            if (clen > 2047) clen = 2047;
            for (int i = 0; i < clen; i++) lower_content[i] = tolower(content[i]);
            lower_content[clen] = '\0';

            // Check for CAUSES patterns
            const char* causes_pats[] = {" wakes ", " eats ", " causes ", " triggers ", " destroys ", " kills ", NULL};
            for (int cp = 0; causes_pats[cp]; cp++) {
                const char* cmatch = strstr(lower_content, causes_pats[cp]);
                if (cmatch) {
                    char subj[128] = {0};
                    const char* s = cmatch - 1;
                    while (s > lower_content && *s == ' ') s--;
                    const char* word_end = s + 1;
                    while (s > lower_content && *s != ' ' && *s != '.' && *s != ',') s--;
                    if (*s == ' ' || *s == '.' || *s == ',') s++;
                    int si = 0;
                    while (s < word_end && si < 127) subj[si++] = *s++;
                    subj[si] = '\0';

                    char obj[128] = {0};
                    const char* o = cmatch + strlen(causes_pats[cp]);
                    int oi = 0;
                    while (*o && oi < 127 && *o != '.' && *o != ',') obj[oi++] = *o++;
                    while (oi > 0 && obj[oi-1] == ' ') oi--;
                    obj[oi] = '\0';

                    if (strlen(subj) > 1 && strlen(obj) > 1) {
                        int64_t subj_id = zeta_commit_fact(ctx, NODE_ENTITY, "causal_agent", subj, 0.9f, SOURCE_USER);
                        int64_t obj_id = zeta_commit_fact(ctx, NODE_ENTITY, "causal_target", obj, 0.9f, SOURCE_USER);
                        zeta_create_edge(ctx, subj_id, obj_id, EDGE_CAUSES, 1.0f);
                        facts_created++;
                        fprintf(stderr, "[CAUSAL] %s --CAUSES--> %s\n", subj, obj);
                    }
                }
            }

            // Check for PREVENTS patterns
            const char* prevents_pats[] = {" slayed ", " killed ", " destroyed ", " prevents ", " stops ", " before it could ", NULL};
            for (int pp = 0; prevents_pats[pp]; pp++) {
                const char* pmatch = strstr(lower_content, prevents_pats[pp]);
                if (pmatch) {
                    char subj[128] = {0};
                    const char* s = pmatch - 1;
                    while (s > lower_content && *s == ' ') s--;
                    const char* word_end = s + 1;
                    while (s > lower_content && *s != ' ' && *s != '.' && *s != ',') s--;
                    if (*s == ' ' || *s == '.' || *s == ',') s++;
                    int si = 0;
                    while (s < word_end && si < 127) subj[si++] = *s++;
                    subj[si] = '\0';

                    char obj[128] = {0};
                    const char* o = pmatch + strlen(prevents_pats[pp]);
                    int oi = 0;
                    while (*o && oi < 127 && *o != '.' && *o != ',') obj[oi++] = *o++;
                    while (oi > 0 && obj[oi-1] == ' ') oi--;
                    obj[oi] = '\0';

                    if (strlen(subj) > 1 && strlen(obj) > 1) {
                        int64_t subj_id = zeta_commit_fact(ctx, NODE_ENTITY, "preventer", subj, 0.95f, SOURCE_USER);
                        int64_t obj_id = zeta_commit_fact(ctx, NODE_ENTITY, "prevented", obj, 0.9f, SOURCE_USER);
                        zeta_create_edge(ctx, subj_id, obj_id, EDGE_PREVENTS, 1.0f);
                        facts_created++;
                        fprintf(stderr, "[PREVENTS] %s --PREVENTS--> %s\n", subj, obj);
                    }
                }
            }

            return facts_created;  // Skip full 3B extraction but kept causal edges
        }
    }
    // DETECT CODE IN TEXT
    bool has_code = (strstr(text, "```") != NULL ||
                     strstr(text, "def ") != NULL ||
                     strstr(text, "class ") != NULL ||
                     strstr(text, "function ") != NULL);
    
    // USE 3B MODEL FOR SEMANTIC EXTRACTION
    if (ctx->ctx_subconscious && ctx->model_subconscious) {
        char prompt[4096];
        if (has_code) {
            // CODE EXTRACTION MODE - extract specs, not code
            snprintf(prompt, sizeof(prompt),
                "<|im_start|>system\n"
                "Extract what to REMEMBER about this code (not the code itself):\n"
                "- func_spec|name: what it does\n"
                "- func_rule|name: validation rules or constraints\n"  
                "- func_param|name: parameter requirements\n"
                "- decision|choice: why this approach was chosen\n- location|concept:module (e.g., location|schema_validation:loader.py)\n"
                "Output format: TYPE|VALUE (one fact per line)\n"
                "NEVER output code. Only TYPE|VALUE facts.\n"
                "<|im_end|>\n"
                "<|im_start|>user\n"
                "%s\n"
                "<|im_end|>\n"
                "<|im_start|>assistant\n",
                text);
            fprintf(stderr, "[3B] CODE MODE extraction\n");
        } else {
            // CONVERSATIONAL EXTRACTION MODE  
            snprintf(prompt, sizeof(prompt),
                "<|im_start|>system\n"
                "You extract facts using English grammar rules:\n"
                "RULE 1: 'My name is X' or 'I am X' or 'call me X' -> user_name|X\n"
                "RULE 2: 'code name X' or 'codenamed X' or 'project called X' -> project_codename|X\n"
                "RULE 3: 'X refers to Y' means X is an identifier for concept Y\n"
                "RULE 4: Quoted strings like \"AURORA-17\" are important names\n"
                "RULE 5: 'I live in X' or 'located in X' -> location|X\n"
                "RULE 6: 'rate limit is N' or 'limit of N' -> rate_limit|N\n"
                "RULE 7: 'my favorite X is Y' -> favorite_X|Y\n"
                "RULE 8: 'I was born in YYYY' or 'born in YYYY' -> birth_year|YYYY\n"
                "RULE 9: 'I am N years old' or 'my age is N' -> age|N\n"
                "RULE 10: 'I make $N' or 'salary is $N' or 'earn $N' -> salary|N\n"
                "RULE 11: 'my sister/brother is X' or 'sibling named X' -> sibling|X\n"
                "RULE 12: 'I work at X' or 'employed at X' or 'my job is at X' -> workplace|X\n"
                "RULE 13: 'my pet X is named Y' or 'X named Y' (dog/cat/pet) -> pet_name|Y\n"
                "RULE 14: 'my password is X' or 'code is X' or 'PIN is X' -> secret_code|X (SENSITIVE)\n"
                "CRITICAL: Extract ALL numeric facts (years, ages, amounts) - they are important!\n"
                "NEVER output code or explanations. Output ONLY TYPE|VALUE lines.\n"
                "<|im_end|>\n"
                "<|im_start|>user\n"
                "%s\n"
                "<|im_end|>\n"
                "<|im_start|>assistant\n",
                text);
        }
        const llama_vocab* vocab = llama_model_get_vocab(ctx->model_subconscious);
        std::vector<llama_token> tokens(2048);
        int n_tokens = llama_tokenize(vocab, prompt, strlen(prompt),
                                       tokens.data(), tokens.size(), true, true);
        
        if (n_tokens > 0 && n_tokens < 1024) {
            tokens.resize(n_tokens);
            llama_memory_clear(llama_get_memory(ctx->ctx_subconscious), true);
            
            llama_batch batch = llama_batch_init(n_tokens, 0, 1);
            for (int i = 0; i < n_tokens; i++) {
                common_batch_add(batch, tokens[i], i, {0}, false);
            }
            batch.logits[batch.n_tokens - 1] = true;
            
            if (llama_decode(ctx->ctx_subconscious, batch) == 0) {
                std::string output;
                int n_cur = n_tokens;
                int n_vocab = llama_vocab_n_tokens(vocab);
                
                for (int g = 0; g < 100 && output.size() < 400; g++) {
                    float* logits = llama_get_logits_ith(ctx->ctx_subconscious, -1);
                    llama_token best = 0;
                    float best_logit = logits[0];
                    for (int v = 1; v < n_vocab; v++) {
                        if (logits[v] > best_logit) {
                            best_logit = logits[v];
                            best = v;
                        }
                    }
                    if (llama_vocab_is_eog(vocab, best)) break;
                    std::string piece = common_token_to_piece(vocab, best, true);
                    if (piece.find("<|im_end|>") != std::string::npos) break;
                    output += piece;
                    
                    llama_batch_free(batch);
                    batch = llama_batch_init(1, 0, 1);
                    common_batch_add(batch, best, n_cur++, {0}, true);
                    if (llama_decode(ctx->ctx_subconscious, batch) != 0) break;
                }
                
                fprintf(stderr, "[3B-SEMANTIC] Output: %s\n", output.c_str());
                
                // Parse TYPE|VALUE lines
                char* out_copy = strdup(output.c_str());
                char* line = strtok(out_copy, "\n");
                while (line) {
                    char* pipe = strchr(line, '|');
                    if (pipe && strlen(line) < 200) {  // Skip long lines (likely code)
                        *pipe = 0;
                        char* type = line;
                        char* value = pipe + 1;
                        while (*type == ' ') type++;
                        while (*value == ' ') value++;
                        size_t vlen = strlen(value);
                        while (vlen > 0 && value[vlen-1] == ' ') value[--vlen] = 0;
                        
                        if (vlen > 0) {
                            float sal = (strstr(type,"user") ? 1.0f : 
                                        strstr(type,"project") ? 0.9f : 0.85f);
                            zeta_node_type_t nt = (strstr(type,"user") || strstr(type,"project")) 
                                ? NODE_ENTITY : NODE_FACT;
                            
                            // VERSION CHAIN: Handle location| facts with concept_key
                            char concept_key[64] = {0};
                            if (strncmp(type, "location", 8) == 0) {
                                // Format: location|concept:module -> concept_key = concept
                                char* colon = strchr(value, ':');
                                if (colon) {
                                    size_t klen = colon - value;
                                    if (klen > 0 && klen < 63) {
                                        strncpy(concept_key, value, klen);
                                        concept_key[klen] = 0;
                                    }
                                }
                            }
                            // For func_spec, func_rule - use name as concept_key
                            else if (strncmp(type, "func_", 5) == 0) {
                                char* pipe = strchr(value, '|');
                                if (pipe) {
                                    size_t klen = pipe - value;
                                    if (klen > 0 && klen < 63) {
                                        strncpy(concept_key, value, klen);
                                        concept_key[klen] = 0;
                                    }
                                }
                            }
                            
                            // Find and supersede old nodes with same concept_key
                            int64_t superseded_count = 0;
                            if (concept_key[0]) {
                                for (int ni = 0; ni < ctx->num_nodes; ni++) {
                                    if (ctx->nodes[ni].is_active && 
                                        ctx->nodes[ni].concept_key[0] &&
                                        strcmp(ctx->nodes[ni].concept_key, concept_key) == 0 &&
                                        ctx->nodes[ni].superseded_by == 0) {
                                        superseded_count++;
                                        fprintf(stderr, "[VERSION] Will supersede node %lld (%s) for concept '%s'\n",
                                                (long long)ctx->nodes[ni].node_id, ctx->nodes[ni].value, concept_key);
                                    }
                                }
                            }
                            
                            int64_t new_id = zeta_commit_fact(ctx, nt, type, value, sal, SOURCE_MODEL);
                            
                            // Set concept_key on new node
                            if (concept_key[0] && new_id > 0) {
                                for (int ni = 0; ni < ctx->num_nodes; ni++) {
                                    if (ctx->nodes[ni].node_id == new_id) {
                                        strncpy(ctx->nodes[ni].concept_key, concept_key, 63);
                                        break;
                                    }
                                }
                                // Now mark old nodes as superseded by new node
                                for (int ni = 0; ni < ctx->num_nodes; ni++) {
                                    if (ctx->nodes[ni].is_active && 
                                        ctx->nodes[ni].node_id != new_id &&
                                        ctx->nodes[ni].concept_key[0] &&
                                        strcmp(ctx->nodes[ni].concept_key, concept_key) == 0 &&
                                        ctx->nodes[ni].superseded_by == 0) {
                                        ctx->nodes[ni].superseded_by = new_id;
                                        fprintf(stderr, "[VERSION] Node %lld superseded by %lld for concept '%s'\n",
                                                (long long)ctx->nodes[ni].node_id, (long long)new_id, concept_key);
                                    }
                                }
                            }
                            
                            facts_created++;
                            fprintf(stderr, "[3B] Extracted: %s = %s (concept_key=%s)\n", type, value, concept_key[0] ? concept_key : "none");
                        }
                    }
                    line = strtok(NULL, "\n");
                }
                free(out_copy);
                if (facts_created > 0) { llama_batch_free(batch); return facts_created; }
            }
            llama_batch_free(batch);
        }
    }
    
    // FALLBACK to pattern matching
    // FALLBACK: Pattern-based extraction if 3B fails
    
    const char* input = text;
    char lower[2048];
    size_t len = strlen(text);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    }
    lower[len] = '\0';
    
    // Identity patterns (CRITICAL salience)
    const char* identity_patterns[] = {
        "my name is ", "i am called ", "call me ", "i'm ", "i am ",
        NULL
    };
    for (int p = 0; identity_patterns[p]; p++) {
        const char* match = strstr(lower, identity_patterns[p]);
        if (match) {
            const char* val_start = text + (match - lower) + strlen(identity_patterns[p]);
            char value[256] = {0};
            int vi = 0;
            while (*val_start && vi < 255 && *val_start != '.' && 
                   *val_start != ',' && *val_start != '!' && *val_start != '\n') {
                if (*val_start == ' ' && vi > 0) {
                    // Check for sentence continuation
                    if (*(val_start+1) == 'a' || *(val_start+1) == 'A' ||
                        *(val_start+1) == 'i' || *(val_start+1) == 'I' ||
                        *(val_start+1) == 't' || *(val_start+1) == 'T' ||
                        *(val_start+1) == 'w' || *(val_start+1) == 'W') break;
                }
                value[vi++] = *val_start++;
            }
            while (vi > 0 && value[vi-1] == ' ') vi--;
            value[vi] = '\0';
            
            if (vi > 0) {
                int64_t nid = zeta_commit_fact(ctx, NODE_ENTITY, "user", value, 1.0f, SOURCE_USER);
                zeta_commit_fact(ctx, NODE_FACT, "user_name", value, 0.95f, SOURCE_USER);
                facts_created++;
            }
        }
    }
    
    // Preference patterns (HIGH salience)
    if (strstr(lower, "favorite") || strstr(lower, "favourite")) {
        const char* types[] = {"color", "colour", "number", "movie", "book", 
                               "song", "food", "animal", "ship", "game", NULL};
        for (int t = 0; types[t]; t++) {
            char pattern[64];
            snprintf(pattern, sizeof(pattern), "favorite %s is ", types[t]);
            const char* match = strstr(lower, pattern);
            if (!match) {
                snprintf(pattern, sizeof(pattern), "favourite %s is ", types[t]);
                match = strstr(lower, pattern);
            }
            if (match) {
                const char* val_start = text + (match - lower) + strlen(pattern);
                char value[256] = {0};
                int vi = 0;
                while (*val_start && vi < 255 && *val_start != '.' && 
                       *val_start != ',' && *val_start != '\n') {
                    value[vi++] = *val_start++;
                }
                while (vi > 0 && value[vi-1] == ' ') vi--;
                value[vi] = '\0';
                
                if (vi > 0) {
                    char entity[64];
                    snprintf(entity, sizeof(entity), "favorite_%s", types[t]);
                    zeta_commit_fact(ctx, NODE_FACT, entity, value, 0.85f, SOURCE_USER);
                    facts_created++;
                }
            }
        }
    }
    
    // Project/creation patterns (HIGH salience)
    const char* project_patterns[] = {
        "code name ", "codename ", "codenamed ", "project code name ", "project ", "working on ", "building ", "created ", 
        "developed ", "made ", NULL
    };
    for (int p = 0; project_patterns[p]; p++) {
        const char* match = strstr(lower, project_patterns[p]);
        if (match) {
            const char* val_start = text + (match - lower) + strlen(project_patterns[p]);
            char value[256] = {0};
            int vi = 0;
            while (*val_start && vi < 255 && *val_start != '.' && 
                   *val_start != ',' && *val_start != '\n') {
                value[vi++] = *val_start++;
            }
            while (vi > 0 && value[vi-1] == ' ') vi--;
            value[vi] = '\0';
            
            if (vi > 0) {
                const char* etype = (strstr(project_patterns[p], "codename"))
                    ? "project_codename" : "project";
                int64_t pid = zeta_commit_fact(ctx, NODE_ENTITY, etype, value, 0.9f, SOURCE_USER);

                // Create creator edge from user to project
                for (int i = 0; i < ctx->num_nodes; i++) {
                    if (strcmp(ctx->nodes[i].label, "user") == 0) {
                        zeta_create_edge(ctx, ctx->nodes[i].node_id, pid,
                                        EDGE_CREATED, 1.0f);
                        break;
                    }
                }
                facts_created++;
            }
        }
    }
    
    // Location patterns (HIGH salience)
    const char* location_patterns[] = {
        "i live in ", "located in ", "city called ", "city named ",
        "based in ", "from ", "hometown is ", NULL
    };
    for (int p = 0; location_patterns[p]; p++) {
        const char* match = strstr(lower, location_patterns[p]);
        if (match) {
            const char* val_start = text + (match - lower) + strlen(location_patterns[p]);
            char value[256] = {0};
            int vi = 0;
            while (*val_start && vi < 255 && *val_start != '.' && 
                   *val_start != ',' && *val_start != '!' && *val_start != '\n') {
                value[vi++] = *val_start++;
            }
            while (vi > 0 && value[vi-1] == ' ') vi--;
            value[vi] = '\0';
            
            if (vi > 1) {
                zeta_commit_fact(ctx, NODE_FACT, "location", value, 0.85f, SOURCE_USER);
                facts_created++;
                fprintf(stderr, "[3B] Extracted location: %s\n", value);
            }
        }
    }
    
    // Numeric fact patterns (rate limit, count, etc)
    const char* numeric_patterns[] = {
        "rate limit is ", "limit is ", "count is ", "number is ",
        "set to ", "configured to ", "equals ", NULL
    };
    for (int p = 0; numeric_patterns[p]; p++) {
        const char* match = strstr(lower, numeric_patterns[p]);
        if (match) {
            const char* val_start = text + (match - lower) + strlen(numeric_patterns[p]);
            char value[256] = {0};
            int vi = 0;
            // Extract number and unit
            while (*val_start && vi < 255 && *val_start != '.' && 
                   *val_start != ',' && *val_start != '!' && *val_start != '\n') {
                value[vi++] = *val_start++;
            }
            while (vi > 0 && value[vi-1] == ' ') vi--;
            value[vi] = '\0';
            
            if (vi > 0) {
                // Determine label from pattern
                const char* label = "numeric_fact";
                if (strstr(numeric_patterns[p], "rate")) label = "rate_limit";
                else if (strstr(numeric_patterns[p], "count")) label = "count";
                zeta_commit_fact(ctx, NODE_FACT, label, value, 0.85f, SOURCE_USER);
                facts_created++;
                fprintf(stderr, "[3B] Extracted numeric: %s = %s\n", label, value);
            }
        }
    }

    // Update patterns (for versioning)
    if (strstr(lower, "changed to ") || strstr(lower, "actually ") ||
        strstr(lower, "now it's ") || strstr(lower, "updated to ")) {
        // TODO: Extract old and new values for versioning
        fprintf(stderr, "[3B] Version update detected (TODO: implement)\n");
    }
    

    // ============================================================
    // CAUSAL patterns - detect "X causes Y", "X prevents Y" etc.
    // Creates EDGE_CAUSES and EDGE_PREVENTS for causal reasoning
    // ============================================================
    const char* causal_verbs[] = {
        " causes ", " triggers ", " leads to ", " results in ",
        " wakes ", " awakens ", " activates ", " starts ",
        " eats ", " consumes ", " destroys ", " kills ",
        " creates ", " produces ", " generates ",
        NULL
    };
    const char* prevent_verbs[] = {
        " prevents ", " stops ", " blocks ", " inhibits ",
        " slays ", " slayed ", " killed ", " destroyed ",
        " before it could ", " before he could ", " before she could ",
        NULL
    };

    // Check for causal relationships (CAUSES)
    for (int cv = 0; causal_verbs[cv]; cv++) {
        const char* cmatch = strstr(lower, causal_verbs[cv]);
        if (cmatch) {
            char c_subject[128] = {0};
            const char* c_subj_start = cmatch - 1;
            while (c_subj_start > lower && *c_subj_start == ' ') c_subj_start--;
            const char* c_word_end = c_subj_start + 1;
            while (c_subj_start > lower && *c_subj_start != ' ' && *c_subj_start != '.' && *c_subj_start != ',') {
                c_subj_start--;
            }
            if (*c_subj_start == ' ' || *c_subj_start == '.' || *c_subj_start == ',') c_subj_start++;
            int csi = 0;
            while (c_subj_start < c_word_end && csi < 127) c_subject[csi++] = *c_subj_start++;
            c_subject[csi] = '\0';

            char c_object[128] = {0};
            int coi = 0;
            const char* c_obj_start = cmatch + strlen(causal_verbs[cv]);
            while (*c_obj_start && coi < 127 && *c_obj_start != '.' && *c_obj_start != ',' && *c_obj_start != '\n') {
                c_object[coi++] = *c_obj_start++;
            }
            while (coi > 0 && c_object[coi-1] == ' ') coi--;
            c_object[coi] = '\0';

            if (strlen(c_subject) > 1 && strlen(c_object) > 1) {
                int64_t c_subj_id = zeta_commit_fact(ctx, NODE_ENTITY, "causal_agent", c_subject, 0.85f, SOURCE_USER);
                int64_t c_obj_id = zeta_commit_fact(ctx, NODE_ENTITY, "causal_target", c_object, 0.85f, SOURCE_USER);
                zeta_create_edge(ctx, c_subj_id, c_obj_id, EDGE_CAUSES, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] CAUSAL: %s --CAUSES--> %s\n", c_subject, c_object);

                // Store full causal sentence for surfacing
                char c_sent[256];
                snprintf(c_sent, sizeof(c_sent), "%s causes %s", c_subject, c_object);
                zeta_commit_fact(ctx, NODE_FACT, "causes_relation", c_sent, 0.95f, SOURCE_USER);
            }
        }
    }

    // Check for preventive relationships (PREVENTS)
    for (int pv = 0; prevent_verbs[pv]; pv++) {
        const char* pmatch = strstr(lower, prevent_verbs[pv]);
        if (pmatch) {
            char p_subject[128] = {0};
            const char* p_subj_start = pmatch - 1;
            while (p_subj_start > lower && *p_subj_start == ' ') p_subj_start--;
            const char* p_word_end = p_subj_start + 1;
            while (p_subj_start > lower && *p_subj_start != ' ' && *p_subj_start != '.' && *p_subj_start != ',') {
                p_subj_start--;
            }
            if (*p_subj_start == ' ' || *p_subj_start == '.' || *p_subj_start == ',') p_subj_start++;
            int psi = 0;
            while (p_subj_start < p_word_end && psi < 127) p_subject[psi++] = *p_subj_start++;
            p_subject[psi] = '\0';

            char p_object[128] = {0};
            int poi = 0;
            const char* p_obj_start = pmatch + strlen(prevent_verbs[pv]);
            while (*p_obj_start && poi < 127 && *p_obj_start != '.' && *p_obj_start != ',' && *p_obj_start != '\n') {
                p_object[poi++] = *p_obj_start++;
            }
            while (poi > 0 && p_object[poi-1] == ' ') poi--;
            p_object[poi] = '\0';

            if (strlen(p_subject) > 1 && strlen(p_object) > 1) {
                int64_t p_subj_id = zeta_commit_fact(ctx, NODE_ENTITY, "preventer", p_subject, 0.9f, SOURCE_USER);
                int64_t p_obj_id = zeta_commit_fact(ctx, NODE_ENTITY, "prevented", p_object, 0.85f, SOURCE_USER);
                zeta_create_edge(ctx, p_subj_id, p_obj_id, EDGE_PREVENTS, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] PREVENTS: %s --PREVENTS--> %s\n", p_subject, p_object);

                // Store full prevents sentence for surfacing
                char p_sent[256];
                snprintf(p_sent, sizeof(p_sent), "%s prevents %s", p_subject, p_object);
                zeta_commit_fact(ctx, NODE_FACT, "prevents_relation", p_sent, 0.95f, SOURCE_USER);
            }
        }
    }

    return facts_created;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_DUAL_PROCESS_H
