// Z.E.T.A. Streaming Memory - Reactive Context Management
// Instead of batch loading, stream nodes on-demand with fast eviction
// Token-budgeted context management for 256-token window constraint

#ifndef ZETA_STREAMING_H
#include "zeta-domains.h"
#include "zeta-embed-integration.h"
#define ZETA_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// Forward declaration - actual struct is in zeta-dual-process.h
// This header must be included AFTER zeta-dual-process.h

// Token budget for 3B context window
extern int g_stream_token_budget;      // default 600
extern int g_stream_max_nodes;         // default 6
#define ZETA_STREAM_CAPACITY 32        // Hard limit for array allocation
#define ZETA_EVICTION_THRESHOLD 0.1f  // Lowered to allow new nodes  // Below this momentum = immediate evict

typedef struct {
    int64_t node_id;
    float priority;         // salience * recency * momentum
    int tokens_consumed;    // Approx tokens this node uses
    bool served;            // Already used by 14B this turn
} zeta_active_node_t;

// Conversation turn for short-term memory
#define ZETA_CONV_HISTORY_SIZE 8     // Track last N turns
#define ZETA_CONV_TURN_LEN 512       // Max chars per turn

typedef struct {
    char user[ZETA_CONV_TURN_LEN];
    char assistant[ZETA_CONV_TURN_LEN];
    time_t timestamp;
} zeta_conv_turn_t;

typedef struct {
    zeta_active_node_t active[ZETA_STREAM_CAPACITY];
    int num_active;
    int total_tokens;
    int64_t last_hop_from;  // For graph hop continuation
    int hop_depth;          // Current hop depth (reset each query)
    float query_embedding[3072]; // Query embedding for semantic matching
    bool has_query_embedding;    // True if query was embedded

    // Conversation history buffer (short-term memory)
    zeta_conv_turn_t history[ZETA_CONV_HISTORY_SIZE];
    int history_head;            // Ring buffer head
    int history_count;           // Number of turns stored
} zeta_stream_state_t;

// Add a turn to conversation history
static inline void zeta_conv_push(zeta_stream_state_t* state, const char* user, const char* assistant) {
    if (!state) return;

    int idx = state->history_head;
    strncpy(state->history[idx].user, user ? user : "", ZETA_CONV_TURN_LEN - 1);
    state->history[idx].user[ZETA_CONV_TURN_LEN - 1] = '\0';
    strncpy(state->history[idx].assistant, assistant ? assistant : "", ZETA_CONV_TURN_LEN - 1);
    state->history[idx].assistant[ZETA_CONV_TURN_LEN - 1] = '\0';
    state->history[idx].timestamp = time(NULL);

    state->history_head = (state->history_head + 1) % ZETA_CONV_HISTORY_SIZE;
    if (state->history_count < ZETA_CONV_HISTORY_SIZE) {
        state->history_count++;
    }
}

// Format conversation history for context
static inline int zeta_conv_format(zeta_stream_state_t* state, char* buf, size_t len) {
    if (!state || !buf || len == 0) return 0;
    buf[0] = '\0';

    if (state->history_count == 0) return 0;

    int off = snprintf(buf, len, "\n=== RECENT CONVERSATION ===\n");

    // Start from oldest turn in ring buffer
    int start = (state->history_count < ZETA_CONV_HISTORY_SIZE)
                ? 0
                : state->history_head;

    for (int i = 0; i < state->history_count && (size_t)off < len - 100; i++) {
        int idx = (start + i) % ZETA_CONV_HISTORY_SIZE;
        zeta_conv_turn_t* turn = &state->history[idx];

        if (turn->user[0]) {
            off += snprintf(buf + off, len - off, "User: %.200s\n", turn->user);
        }
        if (turn->assistant[0]) {
            off += snprintf(buf + off, len - off, "Assistant: %.200s\n", turn->assistant);
        }
    }

    return off;
}

// Compute semantic similarity between query and node
// Uses cached node embedding when available, otherwise computes and caches
static inline float zeta_query_node_similarity(
    zeta_stream_state_t* state,
    zeta_graph_node_t* node
) {
    if (!state || !node || !state->has_query_embedding) return 0.5f;
    if (!g_embed_ctx || !g_embed_ctx->initialized) return 0.5f;

    // Use cached embedding if available, otherwise compute and cache
    float* node_embedding = node->embedding;
    int dim = g_embed_ctx->embed_dim;

    // Safety: check embedding dimension fits in node array (max 3072)
    if (dim > 3072) {
        fprintf(stderr, "[EMBED] WARN: Model dim %d > 3072, truncating\n", dim);
        dim = 3072;
    }

    // Embeddings are pre-computed in zeta_create_node_with_source
    // Just use the node's existing embedding (256-dim from 3B model)

    // Compute cosine similarity using cached embedding
    float similarity = zeta_embed_similarity(state->query_embedding, node_embedding, dim);

    // Convert from [-1,1] to [0,1] range for priority boost
    float boost = (similarity + 1.0f) / 2.0f;

    return boost;
}

// Calculate priority score: recency-weighted salience with momentum boost
static inline float zeta_calc_priority(zeta_graph_node_t* node, float momentum) {
    if (!node || !node->is_active) return 0.0f;

    time_t now = time(NULL);
    float age_hours = (float)(now - node->last_accessed) / 3600.0f;

    // Exponential decay: half-life of 2 hours (not 5 minutes!)
    float decay_rate = 0.35f;
    if (node->is_hypothetical && node->hypothetical_decay > 0.0f) decay_rate *= node->hypothetical_decay;
    float recency = expf(-decay_rate * age_hours);

    // Priority = salience * recency * momentum^2 (momentum has outsized effect)
    // Priority = salience * recency + momentum boost (instead of multiplication)
    // This prevents low momentum from killing high-salience nodes
    return (node->salience * recency * 0.7f) + (momentum * 0.3f);
}

// Estimate tokens for a node (rough: 1 token per 4 chars)
static inline int zeta_estimate_tokens(zeta_graph_node_t* node) {
    if (!node) return 0;
    return (strlen(node->label) + strlen(node->value) + 20) / 4;
}

// Surface TOP node that fits in budget
static inline zeta_graph_node_t* zeta_stream_surface_one(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    const char* query,
    float current_momentum
) {
    if (!ctx || !state) return NULL;

    // Check token budget
    if (state->total_tokens >= g_stream_token_budget) {
        fprintf(stderr, "[STREAM] Token budget exhausted (%d/%d)\n",
                state->total_tokens, g_stream_token_budget);
        return NULL;
    }

    // Embed query for semantic matching (only once per generation)
    if (!state->has_query_embedding && query && strlen(query) > 0 && g_embed_ctx && g_embed_ctx->initialized) {
        int dim = zeta_embed_text(query, state->query_embedding, 3072);
        if (dim > 0) {
            state->has_query_embedding = true;
            fprintf(stderr, "[STREAM] Query embedded: %d dims\n", dim);
        }
    }

    // Classify query domain for filtering
    zeta_semantic_domain_t query_domain = DOMAIN_GENERAL;
    if (query && strlen(query) > 0) {
        query_domain = zeta_classify_domain(query);
        fprintf(stderr, "[STREAM] Query domain: %s\n", zeta_domain_name(query_domain));
    }

    // Find highest priority unserved node
    float best_priority = 0.0f;
    int best_idx = -1;

    for (int i = 0; i < ctx->num_nodes; i++) {
        zeta_graph_node_t* node = &ctx->nodes[i];
        if (!node->is_active) continue;

        // Skip corrupted nodes (invalid salience or empty value)
        if (node->salience <= 0.0f || node->salience > 1.0f) continue;
        if (strlen(node->value) < 3) continue;  // Skip near-empty nodes

        // Domain filtering: skip unrelated domains unless very high salience
        zeta_semantic_domain_t node_domain = zeta_classify_domain(node->value);
        if (!zeta_domains_related(query_domain, node_domain) && node->salience < 0.9f) {
            continue;  // Skip unrelated domain
        }

        // Skip if already in active set
        bool already_active = false;
        for (int j = 0; j < state->num_active; j++) {
            if (state->active[j].node_id == node->node_id) {
                already_active = true;
                break;
            }
        }
        if (already_active) continue;

        float priority = zeta_calc_priority(node, current_momentum);

        // Boost priority by query-node semantic similarity
        if (state->has_query_embedding) {
            float sim = zeta_query_node_similarity(state, node);
            priority *= (0.5f + sim);  // Range [0.5, 1.5] multiplier
        }

        // BOOST: raw_memory nodes contain full context - always boost them
        if (strcmp(node->label, "raw_memory") == 0) {
            priority *= 3.0f;  // 3x boost for raw memories
        }


        // Apply eviction threshold
        if (priority < ZETA_EVICTION_THRESHOLD) continue;

        // Check if fits in remaining budget
        int tokens = zeta_estimate_tokens(node);
        if (state->total_tokens + tokens > g_stream_token_budget) continue;

        if (priority > best_priority) {
            best_priority = priority;
            best_idx = i;
        }
    }

    if (best_idx < 0) return NULL;

    // Safety: ensure we don't exceed array bounds
    if (state->num_active >= g_stream_max_nodes || state->num_active >= ZETA_STREAM_CAPACITY) {
        return NULL; // cannot add more nodes without risking overflow
    }

    // Add to active set
    zeta_graph_node_t* node = &ctx->nodes[best_idx];
    int tokens = zeta_estimate_tokens(node);

    if (state->num_active < g_stream_max_nodes && state->num_active < ZETA_STREAM_CAPACITY) {
        state->active[state->num_active].node_id = node->node_id;
        state->active[state->num_active].priority = best_priority;
        state->active[state->num_active].tokens_consumed = tokens;
        state->active[state->num_active].served = false;
        state->num_active++;
        state->total_tokens += tokens;

        fprintf(stderr, "[STREAM] Surfaced: %s (id=%lld, priority=%.2f, tokens=%d, total=%d/%d)\n",
                node->label, (long long)node->node_id, best_priority, tokens, state->total_tokens, g_stream_token_budget);
    }

    return node;
}

// Mark node as served and apply decay
static inline void zeta_stream_ack_served(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    int64_t node_id
) {
    for (int i = 0; i < state->num_active; i++) {
        if (state->active[i].node_id == node_id) {
            state->active[i].served = true;

            // Find and decay the node
            for (int j = 0; j < ctx->num_nodes; j++) {
                if (ctx->nodes[j].node_id == node_id) {
                    // Served nodes get momentum decay (they did their job)
                    ctx->nodes[j].salience *= 0.8f;  // 20% decay after serving
                    fprintf(stderr, "[STREAM] Ack served: %s (new salience=%.2f)\n",
                            ctx->nodes[j].label, ctx->nodes[j].salience);
                    break;
                }
            }
            break;
        }
    }
}

// Evict served/low-priority nodes to make room
static inline void zeta_stream_evict(
    zeta_stream_state_t* state,
    float current_momentum
) {
    int new_count = 0;
    int freed_tokens = 0;

    for (int i = 0; i < state->num_active; i++) {
        // Evict if: served OR below threshold
        bool evict = state->active[i].served ||
                     (state->active[i].priority < ZETA_EVICTION_THRESHOLD);

        if (!evict) {
            // Keep this one
            if (new_count != i) {
                state->active[new_count] = state->active[i];
            }
            new_count++;
        } else {
            freed_tokens += state->active[i].tokens_consumed;
        }
    }

    if (freed_tokens > 0) {
        fprintf(stderr, "[STREAM] Evicted %d nodes, freed %d tokens\n",
                state->num_active - new_count, freed_tokens);
    }

    state->num_active = new_count;
    state->total_tokens -= freed_tokens;
    state->has_query_embedding = false;  // New query needs fresh embedding
}

// Reset stream state for new query
static inline void zeta_stream_reset(zeta_stream_state_t* state) {
    state->num_active = 0;
    state->total_tokens = 0;
    state->last_hop_from = 0;
    state->hop_depth = 0;
    state->has_query_embedding = false;  // Force re-embed for new query
}

// Format active nodes into compact context
static inline int zeta_stream_format(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    char* buffer,
    int buffer_size
) {
    if (state->num_active == 0) return 0;

    char* p = buffer;
    int remaining = buffer_size - 1;

    int n = snprintf(p, remaining, "[FACTS]\n");
    p += n; remaining -= n;

    for (int i = 0; i < state->num_active && remaining > 50; i++) {
        if (state->active[i].served) {
            fprintf(stderr, "[FORMAT] Skipping served node id=%lld\n", (long long)state->active[i].node_id);
            continue;  // Don't repeat served facts
        }

        // Find the node by ID
        bool found = false;
        for (int j = 0; j < ctx->num_nodes; j++) {
            if (ctx->nodes[j].node_id == state->active[i].node_id) {
                n = snprintf(p, remaining, "%s\n", ctx->nodes[j].value);
                p += n; remaining -= n;
                fprintf(stderr, "[FORMAT] Added: %.50s (id=%lld)\n", ctx->nodes[j].value, (long long)state->active[i].node_id);
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "[FORMAT] Node id=%lld NOT FOUND in %d nodes!\n", (long long)state->active[i].node_id, ctx->num_nodes);
        }
    }

    snprintf(p, remaining, "[/FACTS]\n");
    return buffer_size - remaining;
}

#endif // ZETA_STREAMING_H
