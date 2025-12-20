// Z.E.T.A. Cyclic Correlation Engine v2
// 3B PARALLEL: Input creates nodes, Output creates correlations ONLY

#ifndef ZETA_CYCLIC_H
#define ZETA_CYCLIC_H

#include "zeta-dual-process.h"
#include <pthread.h>
// Forward declaration for extern function
int zeta_3b_extract_facts(zeta_dual_ctx_t* ctx, const char* text);
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char text[4096];
    bool is_input;
    int64_t timestamp;
    float momentum;
} zeta_cyclic_entry_t;

typedef struct {
    zeta_cyclic_entry_t queue[64];
    int head;
    int tail;
    char last_input[4096];
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool running;
} zeta_cyclic_queue_t;

static zeta_cyclic_queue_t g_cyclic = {0};
static bool g_cyclic_initialized = false;

static inline void zeta_cyclic_init(void) {
    if (g_cyclic_initialized) return;
    pthread_mutex_init(&g_cyclic.lock, NULL);
    pthread_cond_init(&g_cyclic.cond, NULL);
    g_cyclic.running = true;
    g_cyclic_initialized = true;
}

static inline void zeta_cyclic_push(const char* text, bool is_input, float momentum) {
    zeta_cyclic_init();
    pthread_mutex_lock(&g_cyclic.lock);
    
    if (is_input) {
        strncpy(g_cyclic.last_input, text, sizeof(g_cyclic.last_input) - 1);
    }
    
    int next = (g_cyclic.tail + 1) % 64;
    if (next != g_cyclic.head) {
        zeta_cyclic_entry_t* entry = &g_cyclic.queue[g_cyclic.tail];
        strncpy(entry->text, text, sizeof(entry->text) - 1);
        fprintf(stderr, "[CYCLIC:PUSH] Stored: %.60s...\n", entry->text);
        entry->is_input = is_input;
        entry->timestamp = (int64_t)time(NULL);
        entry->momentum = momentum;
        g_cyclic.tail = next;
        pthread_cond_signal(&g_cyclic.cond);
    }
    
    pthread_mutex_unlock(&g_cyclic.lock);
}

// Find references to existing entities
static inline int zeta_find_entity_refs(
    zeta_dual_ctx_t* ctx,
    const char* text,
    int64_t* entity_ids,
    int max_refs
) {
    if (!ctx || !text || !entity_ids) return 0;
    
    int found = 0;
    char lower[4096];
    size_t len = strlen(text);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    }
    lower[len] = '\0';
    
    for (int i = 0; i < ctx->num_nodes && found < max_refs; i++) {
        if (!ctx->nodes[i].is_active) continue;
        if (strlen(ctx->nodes[i].value) < 2) continue;
        
        char val_lower[256];
        size_t vlen = strlen(ctx->nodes[i].value);
        if (vlen >= sizeof(val_lower)) vlen = sizeof(val_lower) - 1;
        for (size_t j = 0; j < vlen; j++) {
            char c = ctx->nodes[i].value[j];
            val_lower[j] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        }
        val_lower[vlen] = '\0';
        
        if (strstr(lower, val_lower)) {
            entity_ids[found++] = ctx->nodes[i].node_id;
        }
    }
    
    return found;
}

// OUTPUT processing: CORRELATIONS ONLY - NO NEW IDENTITY NODES
static inline int zeta_process_output_cyclic(
    zeta_dual_ctx_t* ctx,
    const char* output_text,
    const char* original_query,
    float momentum
) {
    if (!ctx || !output_text) return 0;
    
    int operations = 0;
    
    // Find entity references in output
    int64_t output_refs[32];
    int n_output_refs = zeta_find_entity_refs(ctx, output_text, output_refs, 32);
    
    // Find entity references in query
    int64_t query_refs[32];
    int n_query_refs = original_query ? 
        zeta_find_entity_refs(ctx, original_query, query_refs, 32) : 0;
    
    // Create RELATED edges between query and output entities (with deduplication)
    // Limit to top 5 refs each to prevent O(n*m) explosion
    int max_refs = 5;
    int created_edges = 0;
    for (int i = 0; i < n_query_refs && i < max_refs; i++) {
        for (int j = 0; j < n_output_refs && j < max_refs; j++) {
            if (query_refs[i] != output_refs[j]) {
                float weight = 0.5f + (momentum * 0.5f);
                // Use dedup version - reinforces existing edges instead of duplicating
                int64_t eid = zeta_create_edge_dedup(ctx, query_refs[i], output_refs[j],
                                                     EDGE_RELATED, weight);
                if (eid > 0) {
                    created_edges++;
                    operations++;

                    zeta_graph_node_t* n1 = zeta_find_node_by_id(ctx, query_refs[i]);
                    zeta_graph_node_t* n2 = zeta_find_node_by_id(ctx, output_refs[j]);
                    if (n1 && n2) {
                        fprintf(stderr, "[3B:CYCLIC] Edge: %s <-> %s (w=%.2f)\n",
                                n1->value, n2->value, weight);
                    }
                }
            }
        }
    }

    // Log edge creation rate
    if (created_edges > 0) {
        fprintf(stderr, "[3B:CYCLIC] Created/reinforced %d edges (capped at %dx%d)\n",
                created_edges, max_refs, max_refs);
    }
    
    // Affirmation/negation detection for salience adjustment
    char lower[2048];
    size_t len = strlen(output_text);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (output_text[i] >= 'A' && output_text[i] <= 'Z') 
                    ? output_text[i] + 32 : output_text[i];
    }
    lower[len] = '\0';
    
    // Boost salience on affirmation
    if (strstr(lower, "yes,") || strstr(lower, "correct") || 
        strstr(lower, "exactly") || strstr(lower, "right,")) {
        for (int i = 0; i < n_output_refs; i++) {
            zeta_graph_node_t* node = zeta_find_node_by_id(ctx, output_refs[i]);
            if (node) {
                node->salience = fminf(1.0f, node->salience + 0.1f);
                fprintf(stderr, "[3B:CYCLIC] Affirmed: %s -> %.2f\n", 
                        node->value, node->salience);
            }
        }
    }
    
    return operations;
}

// 3B Parallel Worker
#ifdef __cplusplus
}
#endif
static inline void* zeta_3b_worker(void* arg) {
    zeta_dual_ctx_t* ctx = (zeta_dual_ctx_t*)arg;
    
    fprintf(stderr, "[3B] Parallel worker started\n");
    
    while (g_cyclic.running) {
        pthread_mutex_lock(&g_cyclic.lock);
        
        while (g_cyclic.head == g_cyclic.tail && g_cyclic.running) {
            pthread_cond_wait(&g_cyclic.cond, &g_cyclic.lock);
        }
        
        if (!g_cyclic.running) {
            pthread_mutex_unlock(&g_cyclic.lock);
            break;
        }
        
        zeta_cyclic_entry_t entry = g_cyclic.queue[g_cyclic.head];
        fprintf(stderr, "[CYCLIC:POP] Retrieved: %.60s... is_input=%d\n", entry.text, entry.is_input);
        char last_input[4096];
        strncpy(last_input, g_cyclic.last_input, sizeof(last_input) - 1);
        g_cyclic.head = (g_cyclic.head + 1) % 64;
        
        pthread_mutex_unlock(&g_cyclic.lock);
        
        if (entry.is_input) {
            // INPUT: Extract facts and create nodes (skip questions)
            // Detect questions - don't extract from queries
            bool is_question = false;
            const char* text = entry.text;
            size_t len = strlen(text);
            if (len > 0 && text[len-1] == '?') is_question = true;
            // Check for question words at start
            const char* qwords[] = {"what ", "who ", "where ", "when ", "why ", "how ", 
                                    "is ", "are ", "do ", "does ", "can ", "will ", "would ",
                                    "could ", "should ", "which ", "tell me", NULL};
            char lower[256] = {0};
            for (int i = 0; i < 255 && text[i]; i++) 
                lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
            for (int q = 0; qwords[q] && !is_question; q++) {
                if (strncmp(lower, qwords[q], strlen(qwords[q])) == 0) is_question = true;
            }
            
            int facts = 0;
            if (!is_question) {
                facts = zeta_3b_extract_facts(ctx, entry.text);
            } else {
                fprintf(stderr, "[3B:WORKER] Skipping extraction for question\n");
            }
            fprintf(stderr, "[3B:WORKER] INPUT: %d facts extracted\n", facts);
        } else {
            // OUTPUT: CORRELATIONS ONLY - no new identity nodes
            int ops = zeta_process_output_cyclic(ctx, entry.text, last_input, 
                                                  entry.momentum);
            fprintf(stderr, "[3B:WORKER] OUTPUT: %d correlations\n", ops);
        }
    }
    
    fprintf(stderr, "[3B] Parallel worker stopped\n");
    return NULL;
}

static inline pthread_t zeta_3b_start_worker(zeta_dual_ctx_t* ctx) {
    zeta_cyclic_init();
    pthread_t tid;
    pthread_create(&tid, NULL, zeta_3b_worker, ctx);
    return tid;
}

static inline void zeta_3b_stop_worker(pthread_t tid) {
    pthread_mutex_lock(&g_cyclic.lock);
    g_cyclic.running = false;
    pthread_cond_signal(&g_cyclic.cond);
    pthread_mutex_unlock(&g_cyclic.lock);
    pthread_join(tid, NULL);
}


#endif // ZETA_CYCLIC_H
