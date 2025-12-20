// Z.E.T.A. Tunneling Momentum Search
//
// Quantum-inspired graph traversal that combines:
// 1. Embedding similarity (local attraction)
// 2. Graph edges (associative pathways)
// 3. Momentum tunneling (escape local optima)
//
// Key insight: High momentum = confident search → tunnel to distant nodes
//              Low momentum = uncertain → stay in local neighborhood
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_TUNNEL_SEARCH_H
#define ZETA_TUNNEL_SEARCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_TUNNEL_MAX_HOPS        6       // Max traversal depth
#define ZETA_TUNNEL_BEAM_WIDTH      8       // Nodes kept at each hop
#define ZETA_TUNNEL_MAX_RESULTS     16      // Max results returned
#define ZETA_TUNNEL_MIN_MOMENTUM    0.1f    // Stop when momentum drops below this

// ============================================================================
// Search Result
// ============================================================================

typedef struct {
    int64_t node_id;
    float relevance;            // Combined score: similarity × path_weight
    float similarity;           // Cosine similarity to query
    float path_weight;          // Product of edge weights along path
    int hop_count;              // Number of hops from seed
    int64_t path[ZETA_TUNNEL_MAX_HOPS];  // Node path taken
} zeta_tunnel_result_t;

// ============================================================================
// Search State
// ============================================================================

typedef struct {
    float momentum;             // Current search momentum [0,1]
    float momentum_decay;       // Decay per hop (default 0.85)
    float tunnel_threshold;     // Momentum needed to tunnel (default 0.7)
    float exploration_factor;   // Random exploration (default 0.1)

    // Beam state
    int64_t beam[ZETA_TUNNEL_BEAM_WIDTH];
    float beam_scores[ZETA_TUNNEL_BEAM_WIDTH];
    int beam_size;

    // Results
    zeta_tunnel_result_t results[ZETA_TUNNEL_MAX_RESULTS];
    int num_results;

    // Visited set (bloom-style for speed)
    uint64_t visited_bits[128];  // 8K bit bloom

    // Stats
    int total_hops;
    int tunnel_jumps;           // Non-local jumps
    int local_steps;            // Edge-following steps
} zeta_tunnel_state_t;

// ============================================================================
// Graph Interface (callbacks)
// ============================================================================

// Get embedding for a node
typedef const float* (*zeta_get_embedding_fn)(int64_t node_id, void* ctx);

// Get edge weight between nodes (0 if no edge)
typedef float (*zeta_get_edge_weight_fn)(int64_t from, int64_t to, void* ctx);

// Get neighbors of a node
typedef int (*zeta_get_neighbors_fn)(int64_t node_id, int64_t* neighbors,
                                      float* weights, int max_neighbors, void* ctx);

// Get random node (for tunneling)
typedef int64_t (*zeta_get_random_node_fn)(void* ctx);

// Check if node is active
typedef bool (*zeta_is_node_active_fn)(int64_t node_id, void* ctx);

typedef struct {
    zeta_get_embedding_fn get_embedding;
    zeta_get_edge_weight_fn get_edge_weight;
    zeta_get_neighbors_fn get_neighbors;
    zeta_get_random_node_fn get_random_node;
    zeta_is_node_active_fn is_active;
    void* ctx;
    int embd_dim;
} zeta_tunnel_graph_t;

// ============================================================================
// Main API
// ============================================================================

// Initialize search state with initial momentum
void zeta_tunnel_init(
    zeta_tunnel_state_t* state,
    float initial_momentum,     // Start momentum (from logits entropy)
    float momentum_decay,       // Per-hop decay (default 0.85)
    float tunnel_threshold      // Momentum to enable tunneling (default 0.7)
);

// Run tunneling search from query embedding
// Returns number of results found
int zeta_tunnel_search(
    zeta_tunnel_state_t* state,
    const zeta_tunnel_graph_t* graph,
    const float* query_embedding,
    int64_t seed_node,          // Starting node (-1 for LSH seed)
    int max_hops
);

// Run search with LSH-seeded start points
int zeta_tunnel_search_lsh(
    zeta_tunnel_state_t* state,
    const zeta_tunnel_graph_t* graph,
    const float* query_embedding,
    const int64_t* lsh_candidates,  // From dedup LSH
    int num_candidates,
    int max_hops
);

// ============================================================================
// Momentum Integration
// ============================================================================

// Update momentum based on search progress
// Call after each generation step to adjust momentum
void zeta_tunnel_update_momentum(
    zeta_tunnel_state_t* state,
    float new_momentum          // From compute_momentum_from_logits()
);

// Get current effective search radius based on momentum
// High momentum = wider search, low = local only
float zeta_tunnel_effective_radius(const zeta_tunnel_state_t* state);

// ============================================================================
// Utility
// ============================================================================

// Check if node was visited
bool zeta_tunnel_was_visited(const zeta_tunnel_state_t* state, int64_t node_id);

// Mark node as visited
void zeta_tunnel_mark_visited(zeta_tunnel_state_t* state, int64_t node_id);

// Reset for new search
void zeta_tunnel_reset(zeta_tunnel_state_t* state);

// Print search stats
void zeta_tunnel_print_stats(const zeta_tunnel_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // ZETA_TUNNEL_SEARCH_H
