// Z.E.T.A. Tunneling Momentum Search Implementation
//
// Momentum-driven graph traversal with quantum tunneling inspiration.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-tunnel-search.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// ============================================================================
// Cosine Similarity
// ============================================================================

static float cosine_sim(const float* a, const float* b, int dim) {
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    float denom = sqrtf(na) * sqrtf(nb);
    return (denom > 1e-8f) ? (dot / denom) : 0.0f;
}

// ============================================================================
// Visited Set (simple bloom-style)
// ============================================================================

static uint32_t hash_id(int64_t id) {
    uint64_t h = (uint64_t)id;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return (uint32_t)(h & 0x1FFF);  // 13 bits = 8K range
}

bool zeta_tunnel_was_visited(const zeta_tunnel_state_t* state, int64_t node_id) {
    uint32_t bit = hash_id(node_id);
    return (state->visited_bits[bit / 64] & (1ULL << (bit % 64))) != 0;
}

void zeta_tunnel_mark_visited(zeta_tunnel_state_t* state, int64_t node_id) {
    uint32_t bit = hash_id(node_id);
    state->visited_bits[bit / 64] |= (1ULL << (bit % 64));
}

// ============================================================================
// Initialization
// ============================================================================

void zeta_tunnel_init(
    zeta_tunnel_state_t* state,
    float initial_momentum,
    float momentum_decay,
    float tunnel_threshold
) {
    memset(state, 0, sizeof(*state));
    state->momentum = initial_momentum;
    state->momentum_decay = (momentum_decay > 0) ? momentum_decay : 0.85f;
    state->tunnel_threshold = (tunnel_threshold > 0) ? tunnel_threshold : 0.7f;
    state->exploration_factor = 0.1f;
}

void zeta_tunnel_reset(zeta_tunnel_state_t* state) {
    float m = state->momentum;
    float d = state->momentum_decay;
    float t = state->tunnel_threshold;
    float e = state->exploration_factor;

    memset(state, 0, sizeof(*state));

    state->momentum = m;
    state->momentum_decay = d;
    state->tunnel_threshold = t;
    state->exploration_factor = e;
}

// ============================================================================
// Core Search
// ============================================================================

// Insert result maintaining sorted order by relevance
static void insert_result(
    zeta_tunnel_state_t* state,
    int64_t node_id,
    float relevance,
    float similarity,
    float path_weight,
    int hop_count,
    const int64_t* path
) {
    // Check if already in results
    for (int i = 0; i < state->num_results; i++) {
        if (state->results[i].node_id == node_id) {
            // Update if better relevance
            if (relevance > state->results[i].relevance) {
                state->results[i].relevance = relevance;
                state->results[i].similarity = similarity;
                state->results[i].path_weight = path_weight;
                state->results[i].hop_count = hop_count;
                memcpy(state->results[i].path, path, hop_count * sizeof(int64_t));
            }
            return;
        }
    }

    // Find insertion point
    int pos = state->num_results;
    for (int i = 0; i < state->num_results; i++) {
        if (relevance > state->results[i].relevance) {
            pos = i;
            break;
        }
    }

    // Insert if room or better than worst
    if (state->num_results < ZETA_TUNNEL_MAX_RESULTS) {
        // Shift down
        for (int i = state->num_results; i > pos; i--) {
            state->results[i] = state->results[i-1];
        }
        state->num_results++;
    } else if (pos < ZETA_TUNNEL_MAX_RESULTS) {
        // Shift down, dropping last
        for (int i = ZETA_TUNNEL_MAX_RESULTS - 1; i > pos; i--) {
            state->results[i] = state->results[i-1];
        }
    } else {
        return;  // Not good enough
    }

    // Insert
    state->results[pos].node_id = node_id;
    state->results[pos].relevance = relevance;
    state->results[pos].similarity = similarity;
    state->results[pos].path_weight = path_weight;
    state->results[pos].hop_count = hop_count;
    memcpy(state->results[pos].path, path, hop_count * sizeof(int64_t));
}

// Update beam with new candidates
static void beam_insert(
    int64_t* beam,
    float* scores,
    int* beam_size,
    int64_t node_id,
    float score
) {
    // Find position
    int pos = *beam_size;
    for (int i = 0; i < *beam_size; i++) {
        if (score > scores[i]) {
            pos = i;
            break;
        }
    }

    if (*beam_size < ZETA_TUNNEL_BEAM_WIDTH) {
        // Room to insert
        for (int i = *beam_size; i > pos; i--) {
            beam[i] = beam[i-1];
            scores[i] = scores[i-1];
        }
        beam[pos] = node_id;
        scores[pos] = score;
        (*beam_size)++;
    } else if (pos < ZETA_TUNNEL_BEAM_WIDTH) {
        // Better than worst, shift and insert
        for (int i = ZETA_TUNNEL_BEAM_WIDTH - 1; i > pos; i--) {
            beam[i] = beam[i-1];
            scores[i] = scores[i-1];
        }
        beam[pos] = node_id;
        scores[pos] = score;
    }
}

int zeta_tunnel_search(
    zeta_tunnel_state_t* state,
    const zeta_tunnel_graph_t* graph,
    const float* query_embedding,
    int64_t seed_node,
    int max_hops
) {
    if (!state || !graph || !query_embedding) return 0;

    zeta_tunnel_reset(state);

    // Initialize beam with seed
    if (seed_node >= 0 && graph->is_active(seed_node, graph->ctx)) {
        state->beam[0] = seed_node;
        const float* emb = graph->get_embedding(seed_node, graph->ctx);
        if (emb) {
            state->beam_scores[0] = cosine_sim(query_embedding, emb, graph->embd_dim);
        } else {
            state->beam_scores[0] = 0.5f;
        }
        state->beam_size = 1;
        zeta_tunnel_mark_visited(state, seed_node);
    }

    // Current path tracking
    int64_t current_path[ZETA_TUNNEL_MAX_HOPS];
    int path_len = 0;

    // Main search loop
    for (int hop = 0; hop < max_hops && state->momentum > ZETA_TUNNEL_MIN_MOMENTUM; hop++) {
        if (state->beam_size == 0) break;

        // New beam for this hop
        int64_t new_beam[ZETA_TUNNEL_BEAM_WIDTH];
        float new_scores[ZETA_TUNNEL_BEAM_WIDTH];
        int new_beam_size = 0;

        // Process each node in current beam
        for (int b = 0; b < state->beam_size; b++) {
            int64_t current = state->beam[b];
            float current_score = state->beam_scores[b];

            // Add to results
            const float* emb = graph->get_embedding(current, graph->ctx);
            float sim = emb ? cosine_sim(query_embedding, emb, graph->embd_dim) : 0.0f;
            float relevance = sim * state->momentum;
            insert_result(state, current, relevance, sim, current_score, path_len, current_path);

            // Get neighbors (local step)
            int64_t neighbors[32];
            float weights[32];
            int n_neighbors = graph->get_neighbors(current, neighbors, weights, 32, graph->ctx);

            for (int i = 0; i < n_neighbors; i++) {
                if (zeta_tunnel_was_visited(state, neighbors[i])) continue;
                if (!graph->is_active(neighbors[i], graph->ctx)) continue;

                const float* n_emb = graph->get_embedding(neighbors[i], graph->ctx);
                if (!n_emb) continue;

                float n_sim = cosine_sim(query_embedding, n_emb, graph->embd_dim);
                float edge_score = weights[i] * n_sim * state->momentum;

                beam_insert(new_beam, new_scores, &new_beam_size, neighbors[i], edge_score);
                zeta_tunnel_mark_visited(state, neighbors[i]);
                state->local_steps++;
            }

            // Tunneling: if high momentum, jump to distant node
            if (state->momentum > state->tunnel_threshold) {
                // Probability of tunnel = momentum - threshold
                float tunnel_prob = state->momentum - state->tunnel_threshold;

                // Random check (simplified - should use better RNG)
                if (((float)rand() / RAND_MAX) < tunnel_prob) {
                    int64_t tunnel_target = graph->get_random_node(graph->ctx);

                    if (tunnel_target >= 0 &&
                        !zeta_tunnel_was_visited(state, tunnel_target) &&
                        graph->is_active(tunnel_target, graph->ctx)) {

                        const float* t_emb = graph->get_embedding(tunnel_target, graph->ctx);
                        if (t_emb) {
                            float t_sim = cosine_sim(query_embedding, t_emb, graph->embd_dim);
                            // Tunnel score boosted by momentum
                            float tunnel_score = t_sim * state->momentum * 1.5f;

                            beam_insert(new_beam, new_scores, &new_beam_size, tunnel_target, tunnel_score);
                            zeta_tunnel_mark_visited(state, tunnel_target);
                            state->tunnel_jumps++;
                        }
                    }
                }
            }
        }

        // Update beam
        memcpy(state->beam, new_beam, new_beam_size * sizeof(int64_t));
        memcpy(state->beam_scores, new_scores, new_beam_size * sizeof(float));
        state->beam_size = new_beam_size;

        // Decay momentum
        state->momentum *= state->momentum_decay;
        state->total_hops++;

        // Update path
        if (state->beam_size > 0 && path_len < ZETA_TUNNEL_MAX_HOPS) {
            current_path[path_len++] = state->beam[0];
        }
    }

    return state->num_results;
}

int zeta_tunnel_search_lsh(
    zeta_tunnel_state_t* state,
    const zeta_tunnel_graph_t* graph,
    const float* query_embedding,
    const int64_t* lsh_candidates,
    int num_candidates,
    int max_hops
) {
    if (!state || !graph || !query_embedding) return 0;

    zeta_tunnel_reset(state);

    // Initialize beam with LSH candidates
    for (int i = 0; i < num_candidates && state->beam_size < ZETA_TUNNEL_BEAM_WIDTH; i++) {
        int64_t node = lsh_candidates[i];
        if (!graph->is_active(node, graph->ctx)) continue;

        const float* emb = graph->get_embedding(node, graph->ctx);
        if (!emb) continue;

        float sim = cosine_sim(query_embedding, emb, graph->embd_dim);
        beam_insert(state->beam, state->beam_scores, &state->beam_size, node, sim);
        zeta_tunnel_mark_visited(state, node);
    }

    // Continue with regular search
    if (state->beam_size > 0) {
        // Run from best candidate
        int64_t current_path[ZETA_TUNNEL_MAX_HOPS];
        int path_len = 0;

        // Main search loop (same as zeta_tunnel_search)
        for (int hop = 0; hop < max_hops && state->momentum > ZETA_TUNNEL_MIN_MOMENTUM; hop++) {
            if (state->beam_size == 0) break;

            int64_t new_beam[ZETA_TUNNEL_BEAM_WIDTH];
            float new_scores[ZETA_TUNNEL_BEAM_WIDTH];
            int new_beam_size = 0;

            for (int b = 0; b < state->beam_size; b++) {
                int64_t current = state->beam[b];
                float current_score = state->beam_scores[b];

                const float* emb = graph->get_embedding(current, graph->ctx);
                float sim = emb ? cosine_sim(query_embedding, emb, graph->embd_dim) : 0.0f;
                float relevance = sim * state->momentum;
                insert_result(state, current, relevance, sim, current_score, path_len, current_path);

                int64_t neighbors[32];
                float weights[32];
                int n_neighbors = graph->get_neighbors(current, neighbors, weights, 32, graph->ctx);

                for (int i = 0; i < n_neighbors; i++) {
                    if (zeta_tunnel_was_visited(state, neighbors[i])) continue;
                    if (!graph->is_active(neighbors[i], graph->ctx)) continue;

                    const float* n_emb = graph->get_embedding(neighbors[i], graph->ctx);
                    if (!n_emb) continue;

                    float n_sim = cosine_sim(query_embedding, n_emb, graph->embd_dim);
                    float edge_score = weights[i] * n_sim * state->momentum;

                    beam_insert(new_beam, new_scores, &new_beam_size, neighbors[i], edge_score);
                    zeta_tunnel_mark_visited(state, neighbors[i]);
                    state->local_steps++;
                }

                if (state->momentum > state->tunnel_threshold) {
                    float tunnel_prob = state->momentum - state->tunnel_threshold;
                    if (((float)rand() / RAND_MAX) < tunnel_prob) {
                        int64_t tunnel_target = graph->get_random_node(graph->ctx);
                        if (tunnel_target >= 0 &&
                            !zeta_tunnel_was_visited(state, tunnel_target) &&
                            graph->is_active(tunnel_target, graph->ctx)) {
                            const float* t_emb = graph->get_embedding(tunnel_target, graph->ctx);
                            if (t_emb) {
                                float t_sim = cosine_sim(query_embedding, t_emb, graph->embd_dim);
                                float tunnel_score = t_sim * state->momentum * 1.5f;
                                beam_insert(new_beam, new_scores, &new_beam_size, tunnel_target, tunnel_score);
                                zeta_tunnel_mark_visited(state, tunnel_target);
                                state->tunnel_jumps++;
                            }
                        }
                    }
                }
            }

            memcpy(state->beam, new_beam, new_beam_size * sizeof(int64_t));
            memcpy(state->beam_scores, new_scores, new_beam_size * sizeof(float));
            state->beam_size = new_beam_size;
            state->momentum *= state->momentum_decay;
            state->total_hops++;

            if (state->beam_size > 0 && path_len < ZETA_TUNNEL_MAX_HOPS) {
                current_path[path_len++] = state->beam[0];
            }
        }
    }

    return state->num_results;
}

// ============================================================================
// Momentum Integration
// ============================================================================

void zeta_tunnel_update_momentum(zeta_tunnel_state_t* state, float new_momentum) {
    if (!state) return;
    // Smooth update with EMA
    state->momentum = state->momentum * 0.7f + new_momentum * 0.3f;
}

float zeta_tunnel_effective_radius(const zeta_tunnel_state_t* state) {
    if (!state) return 1.0f;
    // High momentum = wider search
    // radius = base + momentum * scale
    return 1.0f + state->momentum * 4.0f;  // 1-5 hops based on momentum
}

// ============================================================================
// Debug
// ============================================================================

void zeta_tunnel_print_stats(const zeta_tunnel_state_t* state) {
    if (!state) return;

    fprintf(stderr, "[TUNNEL] Search stats:\n");
    fprintf(stderr, "  Results: %d\n", state->num_results);
    fprintf(stderr, "  Total hops: %d\n", state->total_hops);
    fprintf(stderr, "  Local steps: %d\n", state->local_steps);
    fprintf(stderr, "  Tunnel jumps: %d\n", state->tunnel_jumps);
    fprintf(stderr, "  Final momentum: %.3f\n", state->momentum);

    if (state->num_results > 0) {
        fprintf(stderr, "  Top result: node=%lld, relevance=%.3f, sim=%.3f\n",
                (long long)state->results[0].node_id,
                state->results[0].relevance,
                state->results[0].similarity);
    }
}
