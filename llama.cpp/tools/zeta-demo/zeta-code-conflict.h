#ifndef ZETA_CODE_CONFLICT_H
#define ZETA_CODE_CONFLICT_H

/**
 * Z.E.T.A. Code Mode Conflict Detection
 * 
 * Detects contradictions in code-related claims:
 * - Function signature changes
 * - Type mismatches
 * - Naming conflicts
 * 
 * Does NOT trigger on:
 * - Line numbers (expected to change)
 * - Version numbers
 * - Indices
 */

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "zeta-code-mode.h"

// Code-specific negation patterns
static const char* CODE_NEGATION_PATTERNS[] = {
    "doesn't exist", "does not exist", "not found",
    "removed", "deleted", "deprecated",
    "no longer", "renamed to", "replaced by",
    "was ", "used to ", "previously ",
    NULL
};

// Code entity types that need conflict checking
typedef enum {
    CODE_ENTITY_FUNCTION,
    CODE_ENTITY_CLASS,
    CODE_ENTITY_VARIABLE,
    CODE_ENTITY_FILE,
    CODE_ENTITY_PARAMETER,
    CODE_ENTITY_RETURN_TYPE
} zeta_code_entity_type_t;

typedef struct {
    zeta_code_entity_type_t type;
    char name[128];
    char signature[256];
    char file_path[256];
    int arg_count;
    char return_type[64];
} zeta_code_entity_t;

typedef struct {
    bool has_conflict;
    zeta_code_entity_type_t entity_type;
    char entity_name[128];
    char stored_claim[256];
    char output_claim[256];
    float confidence;
    char conflict_type[64];  // "signature_mismatch", "removed", "renamed"
} zeta_code_conflict_result_t;

// Extract function signature from text
static inline bool zeta_extract_function_sig(
    const char* text,
    char* name,
    int* arg_count,
    char* return_type
) {
    // Look for patterns like "function foo(a, b, c)" or "def foo(x, y):"
    const char* func_markers[] = {"function ", "def ", "fn ", "func ", NULL};
    
    for (int i = 0; func_markers[i]; i++) {
        const char* pos = strstr(text, func_markers[i]);
        if (!pos) continue;
        
        pos += strlen(func_markers[i]);
        
        // Extract name
        int name_len = 0;
        while (pos[name_len] && (isalnum((unsigned char)pos[name_len]) || pos[name_len] == '_')) {
            name[name_len] = pos[name_len];
            name_len++;
            if (name_len >= 127) break;
        }
        name[name_len] = '\0';
        
        if (name_len < 2) continue;
        
        // Count arguments
        const char* paren = strchr(pos, '(');
        if (paren) {
            const char* end_paren = strchr(paren, ')');
            if (end_paren) {
                // Count commas + 1 (unless empty)
                *arg_count = 0;
                const char* p = paren + 1;
                while (*p && p < end_paren && isspace((unsigned char)*p)) p++;
                if (p < end_paren) {
                    *arg_count = 1;
                    while (p < end_paren) {
                        if (*p == ',') (*arg_count)++;
                        p++;
                    }
                }
            }
        }
        
        // Look for return type (after -> or :)
        const char* arrow = strstr(pos, "->");
        if (arrow) {
            arrow += 2;
            while (*arrow && isspace((unsigned char)*arrow)) arrow++;
            int rt_len = 0;
            while (arrow[rt_len] && (isalnum((unsigned char)arrow[rt_len]) || arrow[rt_len] == '_')) {
                return_type[rt_len] = arrow[rt_len];
                rt_len++;
                if (rt_len >= 63) break;
            }
            return_type[rt_len] = '\0';
        }
        
        return true;
    }
    
    return false;
}

// Check if text says a code entity was removed/renamed
static inline bool zeta_code_has_removal(const char* text, const char* entity_name) {
    char lower_text[2048];
    size_t i = 0;
    while (text[i] && i < sizeof(lower_text) - 1) {
        lower_text[i] = tolower((unsigned char)text[i]);
        i++;
    }
    lower_text[i] = '\0';
    
    char lower_entity[128];
    i = 0;
    while (entity_name[i] && i < sizeof(lower_entity) - 1) {
        lower_entity[i] = tolower((unsigned char)entity_name[i]);
        i++;
    }
    lower_entity[i] = '\0';
    
    const char* entity_pos = strstr(lower_text, lower_entity);
    if (!entity_pos) return false;
    
    // Check for removal patterns near entity
    for (int p = 0; CODE_NEGATION_PATTERNS[p]; p++) {
        const char* neg_pos = strstr(lower_text, CODE_NEGATION_PATTERNS[p]);
        if (neg_pos) {
            // Within 50 chars of entity
            if (llabs(neg_pos - entity_pos) < 50) {
                return true;
            }
        }
    }
    
    return false;
}

// Detect code-specific conflicts
static inline zeta_code_conflict_result_t zeta_detect_code_conflict(
    zeta_code_ctx_t* ctx,
    const char* output
) {
    zeta_code_conflict_result_t result = {0};
    
    if (!ctx || !ctx->active_project || !output || strlen(output) < 10) {
        return result;
    }
    
    fprintf(stderr, "[CODE_CONFLICT] Checking output: %.60s...\n", output);
    
    // Check each code node in active project
    for (int i = 0; i < ctx->code_node_count; i++) {
        zeta_code_node_t* node = &ctx->code_nodes[i];
        
        // Only check nodes in active project with high salience
        if (strcmp(node->project_id, ctx->active_project->project_id) != 0) continue;
        if (node->salience < 0.7f) continue;
        
        // Check if output claims this entity was removed
        if (zeta_code_has_removal(output, node->name)) {
            result.has_conflict = true;
            result.entity_type = (zeta_code_entity_type_t)node->type;
            strncpy(result.entity_name, node->name, sizeof(result.entity_name) - 1);
            snprintf(result.stored_claim, sizeof(result.stored_claim),
                     "%s exists at %s:%d", node->name, node->file_path, node->line_start);
            strncpy(result.conflict_type, "claimed_removal", sizeof(result.conflict_type) - 1);
            result.confidence = 0.8f;
            
            fprintf(stderr, "[CODE_CONFLICT] Removal claimed for existing entity: %s\n", node->name);
            return result;
        }
        
        // For functions, check signature changes
        if (node->type == CODE_NODE_FUNCTION) {
            char out_name[128] = {0};
            int out_arg_count = -1;
            char out_return[64] = {0};
            
            if (zeta_extract_function_sig(output, out_name, &out_arg_count, out_return)) {
                // Same function name?
                if (strcasecmp(out_name, node->name) == 0) {
                    // Parse stored signature for arg count
                    int stored_arg_count = -1;
                    char stored_return[64] = {0};
                    char dummy_name[128] = {0}; zeta_extract_function_sig(node->signature, dummy_name, &stored_arg_count, stored_return);
                    
                    // Check arg count mismatch
                    if (out_arg_count >= 0 && stored_arg_count >= 0 && out_arg_count != stored_arg_count) {
                        result.has_conflict = true;
                        result.entity_type = CODE_ENTITY_FUNCTION;
                        strncpy(result.entity_name, node->name, sizeof(result.entity_name) - 1);
                        snprintf(result.stored_claim, sizeof(result.stored_claim),
                                 "%s takes %d arguments", node->name, stored_arg_count);
                        snprintf(result.output_claim, sizeof(result.output_claim),
                                 "Output says %s takes %d arguments", out_name, out_arg_count);
                        strncpy(result.conflict_type, "arg_count_mismatch", sizeof(result.conflict_type) - 1);
                        result.confidence = 0.85f;
                        
                        fprintf(stderr, "[CODE_CONFLICT] Arg count mismatch: %s (%d vs %d)\n",
                                node->name, stored_arg_count, out_arg_count);
                        return result;
                    }
                }
            }
        }
    }
    
    fprintf(stderr, "[CODE_CONFLICT] No conflicts detected\n");
    return result;
}

// Apply code conflict guardrail
static inline const char* zeta_apply_code_conflict_guardrail(
    zeta_code_ctx_t* ctx,
    const char* output,
    char* safe_output,
    size_t safe_output_size
) {
    zeta_code_conflict_result_t conflict = zeta_detect_code_conflict(ctx, output);
    
    if (conflict.has_conflict && conflict.confidence >= 0.7f) {
        snprintf(safe_output, safe_output_size,
            "[CODE CONFLICT: %s - %s. Verify current state.] %s",
            conflict.conflict_type, conflict.stored_claim, output);
        return safe_output;
    }
    
    return output;
}

#endif // ZETA_CODE_CONFLICT_H
