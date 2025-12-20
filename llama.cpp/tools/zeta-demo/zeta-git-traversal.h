// Z.E.T.A. GitGraph Traversal Integration
// Bridges dual-process (tunneling, momentum, decay, surfacing) with GitGraph
//
// Key concepts:
//   - Branches organize knowledge (code, preferences, facts, etc.)
//   - Tunneling CROSSES branches via embedding similarity
//   - Momentum is BRANCH-AWARE (focus boosts current branch nodes)
//   - Decay is UNIVERSAL but HEAD nodes decay slower
//   - Surfacing PREFERS current branch but can tunnel to others
//
// The magic: semantic relationships (tunneling) transcend branch boundaries,
// but organizational structure (branches) guides focus and priority.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_GIT_TRAVERSAL_H
#define ZETA_GIT_TRAVERSAL_H

#include "zeta-graph-git.h"
#include "zeta-dual-process.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// BRANCH-AWARE DECAY
// =============================================================================

// HEAD nodes decay slower (they're the "tip" of knowledge)
#define ZETA_HEAD_DECAY_FACTOR     0.5f    // Heads decay at half rate
#define ZETA_ANCESTOR_DECAY_FACTOR 1.0f    // Older commits decay normally
#define ZETA_ORPHAN_DECAY_FACTOR   1.5f    // Nodes not on any branch decay faster

typedef struct {
    float base_decay_rate;      // Base decay per hour (default 0.01)
    float min_salience;         // Floor for decay (default 0.1)
    bool decay_heads;           // Should HEAD nodes decay at all? (default: slower)
    bool decay_pinned;          // Should pinned nodes decay? (default: no)
} zeta_decay_config_t;

static zeta_decay_config_t g_decay_config = {
    .base_decay_rate = 0.01f,
    .min_salience = 0.1f,
    .decay_heads = true,
    .decay_pinned = false
};

// Check if a node is a HEAD of any branch
static inline bool zeta_is_head_node(zeta_git_ctx_t* git, int64_t node_id) {
    if (!git) return false;
    for (int i = 0; i < git->num_branches; i++) {
        if (git->branches[i].is_active && git->branches[i].head_node_id == node_id) {
            return true;
        }
    }
    return false;
}

// Apply branch-aware decay to all nodes
static inline int zeta_git_decay_all(zeta_git_ctx_t* git, int64_t current_time) {
    if (!git || !git->graph) return 0;

    int decayed = 0;
    for (int i = 0; i < git->graph->num_nodes; i++) {
        zeta_graph_node_t* node = &git->graph->nodes[i];
        if (!node->is_active) continue;
        if (node->is_pinned && !g_decay_config.decay_pinned) continue;

        // Calculate hours since last access
        float hours = (current_time - node->last_accessed) / 3600.0f;
        if (hours <= 0) continue;

        // Determine decay factor based on node status
        float factor = ZETA_ANCESTOR_DECAY_FACTOR;
        if (zeta_is_head_node(git, node->node_id)) {
            factor = ZETA_HEAD_DECAY_FACTOR;
        }

        // Apply decay
        float decay = g_decay_config.base_decay_rate * factor * hours;
        float new_salience = node->salience - decay;

        if (new_salience < g_decay_config.min_salience) {
            new_salience = g_decay_config.min_salience;
        }

        if (new_salience != node->salience) {
            node->salience = new_salience;
            decayed++;
        }
    }

    return decayed;
}

// =============================================================================
// CROSS-BRANCH TUNNELING
// =============================================================================

typedef struct {
    int64_t node_id;
    int branch_idx;             // Which branch this node belongs to
    float similarity;           // Embedding similarity to query
    float salience;             // Current node salience
    float relevance_score;      // Combined: sim * salience * branch_boost
} zeta_tunnel_hit_t;

#define ZETA_MAX_TUNNEL_HITS 64

typedef struct {
    zeta_tunnel_hit_t hits[ZETA_MAX_TUNNEL_HITS];
    int num_hits;
    int current_branch_hits;    // How many are from current branch
    int cross_branch_hits;      // How many crossed branch boundaries
} zeta_tunnel_result_t;

// Tunnel across all branches, preferring current branch
static inline zeta_tunnel_result_t zeta_git_tunnel(
    zeta_git_ctx_t* git,
    const float* query_embedding,
    int embed_dim,
    float min_similarity,
    float current_branch_boost  // Boost for nodes on current branch (e.g., 1.2)
) {
    zeta_tunnel_result_t result = {0};
    if (!git || !git->graph || !query_embedding) return result;

    int current_branch = git->current_branch_idx;

    // Score all active nodes
    typedef struct { int64_t id; float score; int branch; float sim; } scored_t;
    scored_t scores[1024];
    int num_scores = 0;

    for (int i = 0; i < git->graph->num_nodes && num_scores < 1024; i++) {
        zeta_graph_node_t* node = &git->graph->nodes[i];
        if (!node->is_active) continue;

        // Calculate embedding similarity
        float sim = zeta_cosine_sim(query_embedding, node->embedding, embed_dim);
        if (sim < min_similarity) continue;

        // Determine which branch this node belongs to
        // For now, assume main (0) - full implementation would trace edges
        int node_branch = 0;

        // Calculate relevance score
        float branch_factor = (node_branch == current_branch) ? current_branch_boost : 1.0f;
        float score = sim * node->salience * branch_factor;

        scores[num_scores++] = (scored_t){node->node_id, score, node_branch, sim};
    }

    // Sort by score (simple bubble sort for small arrays)
    for (int i = 0; i < num_scores - 1; i++) {
        for (int j = 0; j < num_scores - i - 1; j++) {
            if (scores[j].score < scores[j + 1].score) {
                scored_t tmp = scores[j];
                scores[j] = scores[j + 1];
                scores[j + 1] = tmp;
            }
        }
    }

    // Fill result
    for (int i = 0; i < num_scores && result.num_hits < ZETA_MAX_TUNNEL_HITS; i++) {
        zeta_graph_node_t* node = zeta_find_node_by_id(git->graph, scores[i].id);
        if (!node) continue;

        zeta_tunnel_hit_t* hit = &result.hits[result.num_hits++];
        hit->node_id = scores[i].id;
        hit->branch_idx = scores[i].branch;
        hit->similarity = scores[i].sim;
        hit->salience = node->salience;
        hit->relevance_score = scores[i].score;

        if (scores[i].branch == current_branch) {
            result.current_branch_hits++;
        } else {
            result.cross_branch_hits++;
        }
    }

    fprintf(stderr, "[GIT-TUNNEL] Found %d hits (%d current branch, %d cross-branch)\n",
            result.num_hits, result.current_branch_hits, result.cross_branch_hits);
    return result;
}

// =============================================================================
// BRANCH-AWARE MOMENTUM
// =============================================================================

typedef struct {
    float branch_momentum[ZETA_MAX_BRANCHES];   // Momentum per branch
    float global_momentum;                      // Overall system momentum
    int64_t last_query_time;
    int queries_this_session;
} zeta_branch_momentum_t;

static zeta_branch_momentum_t g_branch_momentum = {0};

// Update momentum based on query and hits
static inline void zeta_git_update_momentum(
    zeta_git_ctx_t* git,
    const zeta_tunnel_result_t* hits,
    float query_boost   // How much to boost (default: 0.1)
) {
    if (!git || !hits) return;

    g_branch_momentum.queries_this_session++;
    g_branch_momentum.last_query_time = (int64_t)time(NULL);

    // Boost branches that had hits
    for (int i = 0; i < hits->num_hits; i++) {
        int branch = hits->hits[i].branch_idx;
        if (branch >= 0 && branch < ZETA_MAX_BRANCHES) {
            g_branch_momentum.branch_momentum[branch] += query_boost * hits->hits[i].relevance_score;
            if (g_branch_momentum.branch_momentum[branch] > 1.0f) {
                g_branch_momentum.branch_momentum[branch] = 1.0f;
            }
        }
    }

    // Current branch always gets a small boost
    int current = git->current_branch_idx;
    if (current >= 0 && current < ZETA_MAX_BRANCHES) {
        g_branch_momentum.branch_momentum[current] += query_boost * 0.5f;
        if (g_branch_momentum.branch_momentum[current] > 1.0f) {
            g_branch_momentum.branch_momentum[current] = 1.0f;
        }
    }

    // Update global momentum
    g_branch_momentum.global_momentum = 0;
    for (int i = 0; i < git->num_branches; i++) {
        if (g_branch_momentum.branch_momentum[i] > g_branch_momentum.global_momentum) {
            g_branch_momentum.global_momentum = g_branch_momentum.branch_momentum[i];
        }
    }
}

// Decay momentum over time
static inline void zeta_git_decay_momentum(float decay_per_minute) {
    for (int i = 0; i < ZETA_MAX_BRANCHES; i++) {
        g_branch_momentum.branch_momentum[i] -= decay_per_minute;
        if (g_branch_momentum.branch_momentum[i] < 0) {
            g_branch_momentum.branch_momentum[i] = 0;
        }
    }
    g_branch_momentum.global_momentum -= decay_per_minute;
    if (g_branch_momentum.global_momentum < 0) {
        g_branch_momentum.global_momentum = 0;
    }
}

// Get momentum-weighted branch preference
static inline float zeta_git_branch_momentum(int branch_idx) {
    if (branch_idx < 0 || branch_idx >= ZETA_MAX_BRANCHES) return 0;
    return g_branch_momentum.branch_momentum[branch_idx];
}

// =============================================================================
// BRANCH-AWARE SURFACING (combines tunnel + hop + momentum)
// =============================================================================

typedef struct {
    zeta_tunnel_hit_t primary_hits[32];     // Direct tunnel hits
    int num_primary;
    int64_t hop_hits[64];                   // Multi-hop related nodes
    int num_hops;
    float context_coherence;                // How coherent is this context?
    int dominant_branch;                    // Which branch dominated?
} zeta_surface_result_t;

static inline zeta_surface_result_t zeta_git_surface(
    zeta_git_ctx_t* git,
    const float* query_embedding,
    int embed_dim,
    int max_primary,
    int max_hops
) {
    zeta_surface_result_t result = {0};
    if (!git) return result;

    // 1. Tunnel to find primary hits
    zeta_tunnel_result_t tunnel = zeta_git_tunnel(
        git, query_embedding, embed_dim,
        0.3f,   // min similarity
        1.2f    // current branch boost
    );

    // 2. Apply momentum weighting
    for (int i = 0; i < tunnel.num_hits && i < max_primary; i++) {
        float momentum = zeta_git_branch_momentum(tunnel.hits[i].branch_idx);
        tunnel.hits[i].relevance_score *= (1.0f + momentum * 0.5f);
    }

    // 3. Re-sort after momentum adjustment
    for (int i = 0; i < tunnel.num_hits - 1; i++) {
        for (int j = 0; j < tunnel.num_hits - i - 1; j++) {
            if (tunnel.hits[j].relevance_score < tunnel.hits[j + 1].relevance_score) {
                zeta_tunnel_hit_t tmp = tunnel.hits[j];
                tunnel.hits[j] = tunnel.hits[j + 1];
                tunnel.hits[j + 1] = tmp;
            }
        }
    }

    // 4. Take top primary hits
    result.num_primary = (tunnel.num_hits < max_primary) ? tunnel.num_hits : max_primary;
    for (int i = 0; i < result.num_primary; i++) {
        result.primary_hits[i] = tunnel.hits[i];
    }

    // 5. Multi-hop from primary hits (find related nodes via edges)
    int hop_count = 0;
    for (int i = 0; i < result.num_primary && hop_count < max_hops; i++) {
        int64_t node_id = result.primary_hits[i].node_id;

        // Find edges from this node
        for (int e = 0; e < git->graph->num_edges && hop_count < max_hops; e++) {
            zeta_graph_edge_t* edge = &git->graph->edges[e];
            if (edge->source_id == node_id || edge->target_id == node_id) {
                int64_t related = (edge->source_id == node_id) ? edge->target_id : edge->source_id;

                // Don't include if already in primary
                bool already = false;
                for (int p = 0; p < result.num_primary; p++) {
                    if (result.primary_hits[p].node_id == related) {
                        already = true;
                        break;
                    }
                }

                if (!already) {
                    result.hop_hits[hop_count++] = related;
                }
            }
        }
    }
    result.num_hops = hop_count;

    // 6. Calculate context coherence (avg similarity between hits)
    if (result.num_primary > 1) {
        float total_sim = 0;
        int pairs = 0;
        for (int i = 0; i < result.num_primary; i++) {
            for (int j = i + 1; j < result.num_primary; j++) {
                zeta_graph_node_t* a = zeta_find_node_by_id(git->graph, result.primary_hits[i].node_id);
                zeta_graph_node_t* b = zeta_find_node_by_id(git->graph, result.primary_hits[j].node_id);
                if (a && b) {
                    total_sim += zeta_cosine_sim(a->embedding, b->embedding, embed_dim);
                    pairs++;
                }
            }
        }
        result.context_coherence = pairs > 0 ? total_sim / pairs : 0;
    }

    // 7. Find dominant branch
    int branch_counts[ZETA_MAX_BRANCHES] = {0};
    for (int i = 0; i < result.num_primary; i++) {
        int b = result.primary_hits[i].branch_idx;
        if (b >= 0 && b < ZETA_MAX_BRANCHES) {
            branch_counts[b]++;
        }
    }
    result.dominant_branch = 0;
    for (int i = 1; i < git->num_branches; i++) {
        if (branch_counts[i] > branch_counts[result.dominant_branch]) {
            result.dominant_branch = i;
        }
    }

    // 8. Update momentum
    zeta_git_update_momentum(git, &tunnel, 0.1f);

    fprintf(stderr, "[GIT-SURFACE] %d primary, %d hops, coherence=%.2f, dominant=%s\n",
            result.num_primary, result.num_hops, result.context_coherence,
            git->branches[result.dominant_branch].name);

    return result;
}

// =============================================================================
// AUTO-ROUTING: Determine which branch a query relates to
// =============================================================================

// Suggest which branch to switch to based on query
static inline int zeta_git_suggest_branch(
    zeta_git_ctx_t* git,
    const float* query_embedding,
    int embed_dim,
    float* confidence
) {
    if (!git || !query_embedding) return -1;

    // Surface and find dominant branch
    zeta_surface_result_t surface = zeta_git_surface(
        git, query_embedding, embed_dim, 16, 32);

    if (confidence) {
        *confidence = surface.context_coherence;
    }

    // If dominant branch is different from current, suggest switch
    if (surface.dominant_branch != git->current_branch_idx &&
        surface.current_branch_hits < surface.num_primary / 2) {
        fprintf(stderr, "[GIT-ROUTE] Suggesting branch switch: %s -> %s\n",
                git->branches[git->current_branch_idx].name,
                git->branches[surface.dominant_branch].name);
    }

    return surface.dominant_branch;
}

// =============================================================================
// HOP THROUGH COMMIT HISTORY (git-log style traversal)
// =============================================================================

typedef void (*zeta_history_walk_fn)(zeta_graph_node_t* node, int depth, void* user_data);

// Walk back through commit history across branches (follows DERIVES_FROM edges)
static inline int zeta_git_walk_history(
    zeta_git_ctx_t* git,
    int64_t start_node,
    int max_depth,
    zeta_history_walk_fn callback,
    void* user_data
) {
    if (!git || !git->graph || start_node < 0) return 0;

    int visited = 0;
    int64_t current = start_node;
    int depth = 0;

    while (current >= 0 && depth < max_depth) {
        zeta_graph_node_t* node = zeta_find_node_by_id(git->graph, current);
        if (!node || !node->is_active) break;

        if (callback) {
            callback(node, depth, user_data);
        }
        visited++;
        depth++;

        // Find parent (DERIVES_FROM edge)
        int64_t parent = -1;
        for (int i = 0; i < git->graph->num_edges; i++) {
            zeta_graph_edge_t* edge = &git->graph->edges[i];
            if (edge->source_id == current && edge->type == EDGE_DERIVES_FROM) {
                parent = edge->target_id;
                break;
            }
        }
        current = parent;
    }

    return visited;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_GIT_TRAVERSAL_H
