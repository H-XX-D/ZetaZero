// Z.E.T.A. Proactive Memory - Momentum-Driven Prefetch via Tunneling
// Uses momentum from 14B logits to drive graph traversal:
//   High momentum = tunnel to distant nodes (confident, exploring)
//   Low momentum = stay local (uncertain, need nearby context)
// Output is limited only by available memory, not context window
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_PROACTIVE_MEMORY_H
#define ZETA_PROACTIVE_MEMORY_H

#include "zeta-streaming.h"
#include "zeta-embed-integration.h"
extern "C" {
#include "zeta-tunnel-search.h"
}
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <string>

// ============================================================================
// Proactive Memory Context
// ============================================================================

#define ZETA_PREFETCH_QUEUE_SIZE 16
#define ZETA_PREFETCH_MAX_NODES 8
#define ZETA_PREFETCH_LOOKAHEAD 3  // Pre-fetch for next N likely topics

typedef struct {
    int64_t node_id;
    float relevance;           // Semantic similarity to current output
    char content[2048];        // Pre-loaded content
    int tokens;                // Estimated token count
    bool injected;             // Already injected into context
} zeta_prefetch_node_t;

typedef struct {
    // Pre-fetch queue (nodes ready to inject)
    zeta_prefetch_node_t queue[ZETA_PREFETCH_QUEUE_SIZE];
    int queue_size;
    std::mutex queue_mutex;

    // Currently generating output (7B watches this)
    char output_buffer[8192];
    int output_len;
    std::mutex output_mutex;

    // Momentum tracking (from 14B logits)
    std::atomic<float> current_momentum;
    float momentum_history[16];         // Recent momentum values
    int momentum_idx;
    std::mutex momentum_mutex;

    // Tunneling search state
    zeta_tunnel_state_t tunnel_state;
    float query_embedding[3072];
    bool has_query_embedding;

    // Control flags
    std::atomic<bool> generation_active;
    std::atomic<bool> prefetch_enabled;
    std::atomic<int> total_prefetched;
    std::atomic<int> total_injected;
    std::atomic<int> tunnel_jumps;      // Non-local fetches

    // 7B prefetch thread
    std::thread prefetch_thread;
    std::condition_variable prefetch_cv;

    // References to model context
    zeta_dual_ctx_t* dual_ctx;
    struct llama_context* ctx_7b;
    const struct llama_vocab* vocab_7b;
} zeta_proactive_ctx_t;

static zeta_proactive_ctx_t* g_proactive = nullptr;

// ============================================================================
// Initialization
// ============================================================================

static inline void zeta_proactive_init(
    zeta_dual_ctx_t* dual_ctx,
    struct llama_context* ctx_7b,
    const struct llama_vocab* vocab_7b
) {
    if (g_proactive) return;  // Already initialized

    g_proactive = new zeta_proactive_ctx_t();
    g_proactive->queue_size = 0;
    g_proactive->output_len = 0;
    g_proactive->generation_active = false;
    g_proactive->prefetch_enabled = true;
    g_proactive->total_prefetched = 0;
    g_proactive->total_injected = 0;
    g_proactive->tunnel_jumps = 0;
    g_proactive->dual_ctx = dual_ctx;
    g_proactive->ctx_7b = ctx_7b;
    g_proactive->vocab_7b = vocab_7b;

    // Initialize momentum tracking
    g_proactive->current_momentum = 0.5f;
    g_proactive->momentum_idx = 0;
    g_proactive->has_query_embedding = false;
    memset(g_proactive->momentum_history, 0, sizeof(g_proactive->momentum_history));

    // Initialize tunnel state with default momentum
    zeta_tunnel_init(&g_proactive->tunnel_state, 0.5f, 0.85f, 0.7f);

    fprintf(stderr, "[PROACTIVE] Momentum-driven prefetch initialized (tunnel threshold: 0.7)\n");
}

// Update momentum from 14B logits - drives prefetch tunneling
static inline void zeta_proactive_update_momentum(float momentum) {
    if (!g_proactive) return;

    std::lock_guard<std::mutex> lock(g_proactive->momentum_mutex);

    g_proactive->current_momentum = momentum;
    g_proactive->momentum_history[g_proactive->momentum_idx] = momentum;
    g_proactive->momentum_idx = (g_proactive->momentum_idx + 1) % 16;

    // Update tunnel state with new momentum
    zeta_tunnel_update_momentum(&g_proactive->tunnel_state, momentum);

    // If momentum is high, wake up prefetch thread to tunnel
    if (momentum > 0.7f && g_proactive->generation_active) {
        g_proactive->prefetch_cv.notify_one();
    }
}

// Get average recent momentum
static inline float zeta_proactive_avg_momentum() {
    if (!g_proactive) return 0.5f;

    std::lock_guard<std::mutex> lock(g_proactive->momentum_mutex);
    float sum = 0.0f;
    int count = 0;
    for (int i = 0; i < 16; i++) {
        if (g_proactive->momentum_history[i] > 0.0f) {
            sum += g_proactive->momentum_history[i];
            count++;
        }
    }
    return count > 0 ? sum / count : 0.5f;
}

static inline void zeta_proactive_free() {
    if (!g_proactive) return;

    g_proactive->generation_active = false;
    g_proactive->prefetch_enabled = false;

    if (g_proactive->prefetch_thread.joinable()) {
        g_proactive->prefetch_cv.notify_all();
        g_proactive->prefetch_thread.join();
    }

    delete g_proactive;
    g_proactive = nullptr;
}

// ============================================================================
// Pre-fetch: Load relevant nodes BEFORE 14B starts generating
// ============================================================================

// Graph interface callbacks for tunneling
static const float* proactive_get_embedding(int64_t node_id, void* ctx) {
    zeta_dual_ctx_t* dual = (zeta_dual_ctx_t*)ctx;
    for (int i = 0; i < dual->num_nodes; i++) {
        if (dual->nodes[i].node_id == node_id) {
            return dual->nodes[i].embedding;
        }
    }
    return nullptr;
}

static float proactive_get_edge_weight(int64_t from, int64_t to, void* ctx) {
    zeta_dual_ctx_t* dual = (zeta_dual_ctx_t*)ctx;
    for (int i = 0; i < dual->num_edges; i++) {
        if (dual->edges[i].source_id == from && dual->edges[i].target_id == to) {
            return dual->edges[i].weight;
        }
    }
    return 0.0f;
}

static int proactive_get_neighbors(int64_t node_id, int64_t* neighbors,
                                    float* weights, int max_neighbors, void* ctx) {
    zeta_dual_ctx_t* dual = (zeta_dual_ctx_t*)ctx;
    int count = 0;
    for (int i = 0; i < dual->num_edges && count < max_neighbors; i++) {
        if (dual->edges[i].source_id == node_id) {
            neighbors[count] = dual->edges[i].target_id;
            weights[count] = dual->edges[i].weight;
            count++;
        }
    }
    return count;
}

static int64_t proactive_get_random_node(void* ctx) {
    zeta_dual_ctx_t* dual = (zeta_dual_ctx_t*)ctx;
    if (dual->num_nodes == 0) return -1;
    return dual->nodes[rand() % dual->num_nodes].node_id;
}

static bool proactive_is_active(int64_t node_id, void* ctx) {
    zeta_dual_ctx_t* dual = (zeta_dual_ctx_t*)ctx;
    for (int i = 0; i < dual->num_nodes; i++) {
        if (dual->nodes[i].node_id == node_id) {
            return dual->nodes[i].is_active;
        }
    }
    return false;
}

// Pre-fetch using momentum-driven tunneling
// High momentum = tunnel to distant associative nodes
// Low momentum = stay local, get similar nodes
static inline int zeta_proactive_prefetch(
    const char* query,
    zeta_stream_state_t* stream_state,
    int max_nodes,
    float initial_momentum
) {
    if (!g_proactive || !g_proactive->dual_ctx || !query) return 0;
    if (max_nodes > ZETA_PREFETCH_MAX_NODES) max_nodes = ZETA_PREFETCH_MAX_NODES;

    zeta_dual_ctx_t* ctx = g_proactive->dual_ctx;

    // Embed the query
    int dim = 0;
    if (g_embed_ctx && g_embed_ctx->initialized) {
        dim = zeta_embed_text(query, g_proactive->query_embedding, 3072);
        if (dim > 0) {
            g_proactive->has_query_embedding = true;
            // Copy to stream state for later use
            memcpy(stream_state->query_embedding, g_proactive->query_embedding, sizeof(g_proactive->query_embedding));
            stream_state->has_query_embedding = true;
        }
    }

    // Initialize tunnel search with current momentum
    zeta_tunnel_init(&g_proactive->tunnel_state, initial_momentum, 0.85f, 0.7f);

    // Setup graph interface
    zeta_tunnel_graph_t graph = {
        .get_embedding = proactive_get_embedding,
        .get_edge_weight = proactive_get_edge_weight,
        .get_neighbors = proactive_get_neighbors,
        .get_random_node = proactive_get_random_node,
        .is_active = proactive_is_active,
        .ctx = ctx,
        .embd_dim = dim > 0 ? dim : 256
    };

    // Find seed node (highest salience node that matches query)
    int64_t seed_node = -1;
    float best_match = 0.0f;
    for (int i = 0; i < ctx->num_nodes; i++) {
        zeta_graph_node_t* node = &ctx->nodes[i];
        if (!node->is_active || node->salience < 0.3f) continue;

        float score = node->salience;
        if (g_proactive->has_query_embedding && node->embedding[0] != 0.0f) {
            float sim = zeta_embed_similarity(g_proactive->query_embedding, node->embedding, graph.embd_dim);
            score = score * 0.4f + (sim + 1.0f) * 0.3f;
        }

        if (score > best_match) {
            best_match = score;
            seed_node = node->node_id;
        }
    }

    // Run tunneling search
    int found = 0;
    if (seed_node >= 0 && g_proactive->has_query_embedding) {
        found = zeta_tunnel_search(
            &g_proactive->tunnel_state,
            &graph,
            g_proactive->query_embedding,
            seed_node,
            ZETA_TUNNEL_MAX_HOPS
        );
    }

    // Load results into prefetch queue
    std::lock_guard<std::mutex> lock(g_proactive->queue_mutex);
    g_proactive->queue_size = 0;

    int loaded = 0;
    for (int i = 0; i < found && loaded < max_nodes; i++) {
        zeta_tunnel_result_t* result = &g_proactive->tunnel_state.results[i];

        // Find the node
        for (int j = 0; j < ctx->num_nodes; j++) {
            if (ctx->nodes[j].node_id == result->node_id) {
                zeta_graph_node_t* node = &ctx->nodes[j];

                zeta_prefetch_node_t* pn = &g_proactive->queue[loaded];
                pn->node_id = node->node_id;
                pn->relevance = result->relevance;
                strncpy(pn->content, node->value, sizeof(pn->content) - 1);
                pn->content[sizeof(pn->content) - 1] = '\0';
                pn->tokens = (strlen(pn->content) + 3) / 4;
                pn->injected = false;

                // Track tunnel jumps (non-local fetches)
                if (result->hop_count > 1) {
                    g_proactive->tunnel_jumps++;
                }

                loaded++;
                g_proactive->total_prefetched++;
                break;
            }
        }
    }

    g_proactive->queue_size = loaded;

    fprintf(stderr, "[PROACTIVE] Tunneling prefetch: %d nodes (momentum=%.2f, tunnels=%d, hops=%d)\n",
            loaded, initial_momentum,
            g_proactive->tunnel_state.tunnel_jumps,
            g_proactive->tunnel_state.total_hops);

    return loaded;
}

// ============================================================================
// Parallel Prefetch: 7B watches 14B output and fetches related nodes
// ============================================================================

// Update output buffer (called as 14B generates)
// Uses try_lock to avoid blocking generation if prefetch worker holds mutex
static inline void zeta_proactive_update_output(const char* new_text, int len) {
    if (!g_proactive || !g_proactive->prefetch_enabled) return;

    // Non-blocking lock - skip update if mutex is held by prefetch worker
    std::unique_lock<std::mutex> lock(g_proactive->output_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return;  // Prefetch worker has mutex, skip this update

    // Append to buffer (keep last 4K chars)
    int to_copy = len;
    if (g_proactive->output_len + len > 8000) {
        // Shift buffer
        int shift = g_proactive->output_len + len - 4000;
        memmove(g_proactive->output_buffer,
                g_proactive->output_buffer + shift,
                g_proactive->output_len - shift);
        g_proactive->output_len -= shift;
    }

    memcpy(g_proactive->output_buffer + g_proactive->output_len, new_text, to_copy);
    g_proactive->output_len += to_copy;
    g_proactive->output_buffer[g_proactive->output_len] = '\0';
}

// Check if a topic is mentioned in output (simple keyword matching)
static inline bool zeta_proactive_topic_mentioned(const char* topic) {
    if (!g_proactive || g_proactive->output_len == 0) return false;

    std::lock_guard<std::mutex> lock(g_proactive->output_mutex);
    return strstr(g_proactive->output_buffer, topic) != nullptr;
}

// Prefetch thread worker - uses momentum to drive tunneling during generation
static void zeta_proactive_prefetch_worker() {
    if (!g_proactive) return;

    while (g_proactive->prefetch_enabled) {
        // Wait for generation to be active or momentum spike
        std::unique_lock<std::mutex> lock(g_proactive->output_mutex);
        g_proactive->prefetch_cv.wait_for(lock, std::chrono::milliseconds(100),
            [](){ return !g_proactive->prefetch_enabled || g_proactive->generation_active; });

        if (!g_proactive->prefetch_enabled) break;
        if (!g_proactive->generation_active) continue;

        // Get current momentum
        float momentum = g_proactive->current_momentum.load();
        lock.unlock();

        // Only tunnel-fetch when momentum is high (confident exploration)
        if (momentum < 0.6f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Get output snapshot for context
        std::lock_guard<std::mutex> olock(g_proactive->output_mutex);
        if (g_proactive->output_len < 50) continue;

        // High momentum = tunnel to related distant nodes
        zeta_dual_ctx_t* ctx = g_proactive->dual_ctx;

        // Setup graph for tunneling
        int dim = g_proactive->has_query_embedding ? 256 : 0;
        zeta_tunnel_graph_t graph = {
            .get_embedding = proactive_get_embedding,
            .get_edge_weight = proactive_get_edge_weight,
            .get_neighbors = proactive_get_neighbors,
            .get_random_node = proactive_get_random_node,
            .is_active = proactive_is_active,
            .ctx = ctx,
            .embd_dim = dim > 0 ? dim : 256
        };

        // Embed recent output to find related nodes
        std::string recent(g_proactive->output_buffer + std::max(0, g_proactive->output_len - 500),
                           std::min(500, g_proactive->output_len));

        float output_embedding[3072] = {0};
        int out_dim = 0;
        if (g_embed_ctx && g_embed_ctx->initialized) {
            out_dim = zeta_embed_text(recent.c_str(), output_embedding, 3072);
        }

        if (out_dim <= 0) continue;

        // Initialize tunnel with current high momentum
        zeta_tunnel_state_t tunnel;
        zeta_tunnel_init(&tunnel, momentum, 0.9f, 0.5f);  // More aggressive tunneling

        // Find seed from current topic
        int64_t seed = -1;
        float best = 0.0f;
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (!ctx->nodes[i].is_active) continue;
            float sim = zeta_embed_similarity(output_embedding, ctx->nodes[i].embedding, out_dim);
            if (sim > best) {
                best = sim;
                seed = ctx->nodes[i].node_id;
            }
        }

        if (seed < 0) continue;

        // Run momentum-driven tunnel search
        int found = zeta_tunnel_search(&tunnel, &graph, output_embedding, seed, 4);

        // Add tunnel results to queue
        std::lock_guard<std::mutex> qlock(g_proactive->queue_mutex);

        for (int i = 0; i < found && g_proactive->queue_size < ZETA_PREFETCH_QUEUE_SIZE; i++) {
            zeta_tunnel_result_t* result = &tunnel.results[i];

            // Skip if already in queue
            bool in_queue = false;
            for (int j = 0; j < g_proactive->queue_size; j++) {
                if (g_proactive->queue[j].node_id == result->node_id) {
                    in_queue = true;
                    break;
                }
            }
            if (in_queue) continue;

            // Find and add node
            for (int j = 0; j < ctx->num_nodes; j++) {
                if (ctx->nodes[j].node_id == result->node_id) {
                    zeta_prefetch_node_t* pn = &g_proactive->queue[g_proactive->queue_size];
                    pn->node_id = result->node_id;
                    pn->relevance = result->relevance;
                    strncpy(pn->content, ctx->nodes[j].value, sizeof(pn->content) - 1);
                    pn->tokens = (strlen(pn->content) + 3) / 4;
                    pn->injected = false;

                    g_proactive->queue_size++;
                    g_proactive->total_prefetched++;

                    if (result->hop_count > 1) {
                        g_proactive->tunnel_jumps++;
                        fprintf(stderr, "[PROACTIVE] Tunnel fetch: %s (hops=%d, momentum=%.2f)\n",
                                ctx->nodes[j].label, result->hop_count, momentum);
                    }
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

// Start parallel prefetch thread
static inline void zeta_proactive_start_generation() {
    if (!g_proactive) return;

    g_proactive->generation_active = true;
    g_proactive->output_len = 0;

    // Start prefetch thread if not running
    if (!g_proactive->prefetch_thread.joinable()) {
        g_proactive->prefetch_thread = std::thread(zeta_proactive_prefetch_worker);
    }

    g_proactive->prefetch_cv.notify_one();
}

// Stop parallel prefetch
static inline void zeta_proactive_stop_generation() {
    if (!g_proactive) return;
    g_proactive->generation_active = false;
}

// ============================================================================
// Node Injection: Get pre-fetched content for context
// ============================================================================

// Get next pre-fetched node that hasn't been injected
// Returns content string or empty if none available
static inline std::string zeta_proactive_get_next(int max_tokens) {
    if (!g_proactive) return "";

    std::lock_guard<std::mutex> lock(g_proactive->queue_mutex);

    for (int i = 0; i < g_proactive->queue_size; i++) {
        zeta_prefetch_node_t* pn = &g_proactive->queue[i];
        if (pn->injected) continue;
        if (pn->tokens > max_tokens) continue;

        pn->injected = true;
        g_proactive->total_injected++;

        fprintf(stderr, "[PROACTIVE] Injecting node %lld (%.2f relevance, %d tokens)\n",
                (long long)pn->node_id, pn->relevance, pn->tokens);

        return std::string(pn->content);
    }

    return "";
}

// Get all pre-fetched content formatted for context
static inline std::string zeta_proactive_get_context(int max_tokens) {
    if (!g_proactive) return "";

    std::lock_guard<std::mutex> lock(g_proactive->queue_mutex);

    std::string context = "";
    int tokens_used = 0;

    // Sort by relevance (highest first)
    std::vector<int> order;
    for (int i = 0; i < g_proactive->queue_size; i++) {
        if (!g_proactive->queue[i].injected) order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [](int a, int b) {
        return g_proactive->queue[a].relevance > g_proactive->queue[b].relevance;
    });

    for (int idx : order) {
        zeta_prefetch_node_t* pn = &g_proactive->queue[idx];
        if (tokens_used + pn->tokens > max_tokens) continue;

        context += pn->content;
        context += "\n";
        tokens_used += pn->tokens;
        pn->injected = true;
        g_proactive->total_injected++;
    }

    if (!context.empty()) {
        fprintf(stderr, "[PROACTIVE] Injected %d tokens of pre-fetched context\n", tokens_used);
    }

    return context;
}

// ============================================================================
// Statistics
// ============================================================================

static inline void zeta_proactive_print_stats() {
    if (!g_proactive) {
        fprintf(stderr, "[PROACTIVE] Not initialized\n");
        return;
    }

    fprintf(stderr, "[PROACTIVE] Momentum-Driven Prefetch Stats:\n");
    fprintf(stderr, "  Queue size: %d\n", g_proactive->queue_size);
    fprintf(stderr, "  Total pre-fetched: %d\n", g_proactive->total_prefetched.load());
    fprintf(stderr, "  Total injected: %d\n", g_proactive->total_injected.load());
    fprintf(stderr, "  Tunnel jumps: %d (non-local fetches)\n", g_proactive->tunnel_jumps.load());
    fprintf(stderr, "  Current momentum: %.2f\n", g_proactive->current_momentum.load());
    fprintf(stderr, "  Avg momentum: %.2f\n", zeta_proactive_avg_momentum());
    fprintf(stderr, "  Generation active: %s\n",
            g_proactive->generation_active ? "yes" : "no");
}

#endif // ZETA_PROACTIVE_MEMORY_H
