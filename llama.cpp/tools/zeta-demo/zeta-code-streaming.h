#ifndef ZETA_CODE_STREAMING_H
#define ZETA_CODE_STREAMING_H

/**
 * Z.E.T.A. Code Mode Streaming Memory
 * 
 * Reactively surfaces relevant code entities based on:
 * - Query relevance (mentions function/class names)
 * - Recency (recently touched files)
 * - Salience (importance of the entity)
 * 
 * Project-scoped: Only surfaces from active project
 */

#include <string.h>
#include <stdio.h>
#include "zeta-code-mode.h"

#define CODE_TOKEN_BUDGET    300   // More tokens for code context
#define CODE_MAX_NODES       5     // Max code entities to surface
#define CODE_RECENCY_BOOST   0.3f  // Boost for recently accessed

typedef struct {
    int64_t node_id;
    float priority;
    int tokens_consumed;
    bool served;
} zeta_code_active_node_t;

static zeta_code_active_node_t g_code_active[CODE_MAX_NODES];
static int g_code_active_count = 0;

// Calculate priority for a code node based on query relevance
static inline float zeta_code_calc_priority(
    zeta_code_node_t* node,
    const char* query,
    int64_t current_time
) {
    float priority = node->salience;
    
    // Recency boost
    int64_t age_sec = current_time - node->created_at;
    if (age_sec < 300) {  // Last 5 minutes
        priority += CODE_RECENCY_BOOST;
    }
    
    // Query relevance boost
    if (query && strlen(query) > 0) {
        char lower_query[512];
        char lower_name[128];
        
        for (size_t i = 0; i < sizeof(lower_query) - 1 && query[i]; i++) {
            lower_query[i] = tolower((unsigned char)query[i]);
        }
        for (size_t i = 0; i < sizeof(lower_name) - 1 && node->name[i]; i++) {
            lower_name[i] = tolower((unsigned char)node->name[i]);
        }
        
        if (strstr(lower_query, lower_name)) {
            priority += 0.5f;  // Strong boost if name mentioned
        }
        
        // Also check file path
        char lower_path[256];
        for (size_t i = 0; i < sizeof(lower_path) - 1 && node->file_path[i]; i++) {
            lower_path[i] = tolower((unsigned char)node->file_path[i]);
        }
        if (strstr(lower_query, lower_path)) {
            priority += 0.3f;
        }
    }
    
    return priority > 1.0f ? 1.0f : priority;
}

// Surface one code node (returns NULL if no more to surface)
static inline zeta_code_node_t* zeta_code_stream_surface(
    zeta_code_ctx_t* ctx,
    const char* query,
    int* tokens_used,
    int token_budget
) {
    if (!ctx || !ctx->active_project) return NULL;
    if (*tokens_used >= token_budget) return NULL;
    
    int64_t now = time(NULL);
    float best_priority = 0.0f;
    zeta_code_node_t* best_node = NULL;
    
    // Find highest priority node not yet served
    for (int i = 0; i < ctx->code_node_count; i++) {
        zeta_code_node_t* node = &ctx->code_nodes[i];
        
        // Project scope
        if (strcmp(node->project_id, ctx->active_project->project_id) != 0) continue;
        
        // Skip low salience
        if (node->salience < 0.3f) continue;
        
        // Check if already served this session
        bool already_served = false;
        for (int j = 0; j < g_code_active_count; j++) {
            if (g_code_active[j].node_id == node->node_id && g_code_active[j].served) {
                already_served = true;
                break;
            }
        }
        if (already_served) continue;
        
        float priority = zeta_code_calc_priority(node, query, now);
        if (priority > best_priority) {
            best_priority = priority;
            best_node = node;
        }
    }
    
    if (!best_node) return NULL;
    
    // Estimate tokens
    int tokens = (strlen(best_node->name) + strlen(best_node->signature) + strlen(best_node->file_path)) / 4;
    tokens = tokens < 20 ? 20 : tokens;
    
    if (*tokens_used + tokens > token_budget) return NULL;
    
    // Add to active list
    if (g_code_active_count < CODE_MAX_NODES) {
        g_code_active[g_code_active_count].node_id = best_node->node_id;
        g_code_active[g_code_active_count].priority = best_priority;
        g_code_active[g_code_active_count].tokens_consumed = tokens;
        g_code_active[g_code_active_count].served = true;
        g_code_active_count++;
    }
    
    *tokens_used += tokens;
    
    fprintf(stderr, "[CODE_STREAM] Surfaced: %s (type=%d, priority=%.2f, tokens=%d)\n",
            best_node->name, best_node->type, best_priority, tokens);
    
    return best_node;
}

// Format code context for 14B prompt
static inline int zeta_code_stream_format(
    zeta_code_ctx_t* ctx,
    const char* query,
    char* buf,
    size_t buf_size
) {
    if (!ctx || !ctx->active_project) return 0;
    
    int tokens_used = 0;
    int offset = 0;
    
    offset += snprintf(buf + offset, buf_size - offset,
                       "[CODE_CONTEXT: %s]\n", ctx->active_project->project_name);
    
    // Surface code nodes
    g_code_active_count = 0;  // Reset for new query
    
    while (tokens_used < CODE_TOKEN_BUDGET) {
        zeta_code_node_t* node = zeta_code_stream_surface(ctx, query, &tokens_used, CODE_TOKEN_BUDGET);
        if (!node) break;
        
        const char* type_str = "entity";
        switch (node->type) {
            case CODE_NODE_FUNCTION: type_str = "function"; break;
            case CODE_NODE_CLASS: type_str = "class"; break;
            case CODE_NODE_FILE: type_str = "file"; break;
            case CODE_NODE_VARIABLE: type_str = "variable"; break;
            case CODE_NODE_IMPORT: type_str = "import"; break;
            case CODE_NODE_TODO: type_str = "todo"; break;
            default: break;
        }
        
        offset += snprintf(buf + offset, buf_size - offset,
                           "- %s %s at %s:%d\n",
                           type_str, node->name, node->file_path, node->line_start);
        
        if (node->signature[0]) {
            offset += snprintf(buf + offset, buf_size - offset,
                               "  signature: %s\n", node->signature);
        }
    }
    
    offset += snprintf(buf + offset, buf_size - offset, "[/CODE_CONTEXT]\n");
    
    fprintf(stderr, "[CODE_STREAM] Formatted %d nodes (%d tokens) for context\n",
            g_code_active_count, tokens_used);
    
    return offset;
}

// Reset streaming state for new session
static inline void zeta_code_stream_reset(void) {
    g_code_active_count = 0;
}

#endif // ZETA_CODE_STREAMING_H
