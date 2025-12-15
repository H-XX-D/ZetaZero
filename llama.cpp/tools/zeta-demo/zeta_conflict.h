// Z.E.T.A. Literal Conflict Detection - Guardrail for fact contradictions
// Detects when 14B output contradicts stored ground-truth facts

#ifndef ZETA_CONFLICT_H
#define ZETA_CONFLICT_H

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

// Negation patterns that indicate contradiction
static const char* NEGATION_PATTERNS[] = {
    "don't have", "do not have", "dont have",
    "not a ", "isn't a ", "is not a ", "isnt a ",
    "never ", "no ", "none",
    "wrong", "incorrect", "false",
    "not true", "untrue",
    "doesn't", "does not", "doesnt",
    "can't", "cannot", "cant",
    "wasn't", "was not", "wasnt",
    "weren't", "were not", "werent",
    "didn't", "did not", "didnt",
    NULL
};

// Entity extraction patterns for facts
typedef struct {
    char subject[64];      // "my car", "I", "my dog"
    char predicate[32];    // "is", "was", "have", "live"
    char object[128];      // "Tesla", "Tokyo", "Max"
} zeta_fact_triple_t;

// Conflict result
typedef struct {
    bool has_conflict;
    char fact_subject[64];
    char fact_value[128];
    char output_claim[256];
    float confidence;      // How confident we are this is a real conflict
} zeta_conflict_result_t;

// Convert string to lowercase for comparison
static inline void zeta_to_lower(char* dst, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i]; i++) {
        dst[i] = tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

// Check if text contains negation near a keyword
static inline bool zeta_has_negation_near(const char* text, const char* keyword) {
    char lower_text[1024];
    char lower_key[128];
    zeta_to_lower(lower_text, text, sizeof(lower_text));
    zeta_to_lower(lower_key, keyword, sizeof(lower_key));
    
    // Find keyword in text
    const char* key_pos = strstr(lower_text, lower_key);
    if (!key_pos) return false;
    
    // Check for negation within 50 chars before keyword
    size_t start = (key_pos - lower_text > 50) ? (key_pos - lower_text - 50) : 0;
    char window[100];
    size_t window_len = key_pos - lower_text - start;
    if (window_len >= sizeof(window)) window_len = sizeof(window) - 1;
    strncpy(window, lower_text + start, window_len);
    window[window_len] = '\0';
    
    // Check each negation pattern
    for (int i = 0; NEGATION_PATTERNS[i]; i++) {
        if (strstr(window, NEGATION_PATTERNS[i])) {
            return true;
        }
    }
    
    return false;
}

// Extract key entities from a fact value
static inline void zeta_extract_entities(const char* value, char entities[][64], int* count, int max_entities) {
    *count = 0;
    char lower[512];
    zeta_to_lower(lower, value, sizeof(lower));
    
    // Split by common delimiters and extract significant words
    char* copy = strdup(lower);
    char* token = strtok(copy, " ,.!?;:");
    
    while (token && *count < max_entities) {
        // Skip common words
        if (strlen(token) > 2 &&
            strcmp(token, "the") != 0 &&
            strcmp(token, "a") != 0 &&
            strcmp(token, "an") != 0 &&
            strcmp(token, "is") != 0 &&
            strcmp(token, "are") != 0 &&
            strcmp(token, "was") != 0 &&
            strcmp(token, "were") != 0 &&
            strcmp(token, "my") != 0 &&
            strcmp(token, "in") != 0 &&
            strcmp(token, "at") != 0 &&
            strcmp(token, "to") != 0 &&
            strcmp(token, "and") != 0) {
            strncpy(entities[*count], token, 63);
            entities[*count][63] = '\0';
            (*count)++;
        }
        token = strtok(NULL, " ,.!?;:");
    }
    free(copy);
}

// Main conflict detection function
static inline zeta_conflict_result_t zeta_detect_conflict(
    zeta_dual_ctx_t* ctx,
    const char* output
) {
    zeta_conflict_result_t result = {0};
    
    if (!ctx || !output || strlen(output) < 10) return result;
    
    char lower_output[2048];
    zeta_to_lower(lower_output, output, sizeof(lower_output));
    
    // Check each active fact node for contradiction
    for (int i = 0; i < ctx->num_nodes; i++) {
        zeta_graph_node_t* node = &ctx->nodes[i];
        
        // Only check active, high-salience USER facts
        if (!node->is_active) continue;
        if (node->salience < 0.5f) continue;
        if (node->source != SOURCE_USER) continue;
        
        // Extract key entities from this fact
        char entities[8][64];
        int entity_count = 0;
        zeta_extract_entities(node->value, entities, &entity_count, 8);
        
        // Check if any entity appears with negation in output
        for (int e = 0; e < entity_count; e++) {
            if (zeta_has_negation_near(output, entities[e])) {
                result.has_conflict = true;
                strncpy(result.fact_subject, node->label, sizeof(result.fact_subject) - 1);
                strncpy(result.fact_value, node->value, sizeof(result.fact_value) - 1);
                
                // Extract the conflicting claim from output
                char lower_entity[64];
                zeta_to_lower(lower_entity, entities[e], sizeof(lower_entity));
                const char* pos = strstr(lower_output, lower_entity);
                if (pos) {
                    size_t start = (pos - lower_output > 30) ? (pos - lower_output - 30) : 0;
                    strncpy(result.output_claim, output + start, sizeof(result.output_claim) - 1);
                }
                
                result.confidence = 0.8f;  // High confidence for negation + entity match
                
                fprintf(stderr, "[CONFLICT] Detected contradiction!\n");
                fprintf(stderr, "  Fact: %s = %s\n", node->label, node->value);
                fprintf(stderr, "  Output contradicts with: %.100s...\n", result.output_claim);
                
                return result;
            }
        }
        
        // Also check for direct value contradiction (e.g., "Toyota" when fact says "Tesla")
        // Look for the fact's key value and check if output gives different value
        char lower_value[512];
        zeta_to_lower(lower_value, node->value, sizeof(lower_value));
        
        // Extract the main object from the fact (last significant word often)
        if (entity_count > 0) {
            const char* main_entity = entities[entity_count - 1];
            
            // Check if output mentions a DIFFERENT value for same subject
            // e.g., fact "car is Tesla", output says "car is Toyota"
            if (strstr(node->label, "car") || strstr(lower_value, "car")) {
                const char* car_brands[] = {"tesla", "toyota", "honda", "ford", "bmw", "audi", "mercedes", "chevy", "nissan", NULL};
                for (int b = 0; car_brands[b]; b++) {
                    if (strstr(lower_output, car_brands[b]) && !strstr(lower_value, car_brands[b])) {
                        // Output mentions a different car brand!
                        result.has_conflict = true;
                        strncpy(result.fact_subject, "car", sizeof(result.fact_subject) - 1);
                        strncpy(result.fact_value, node->value, sizeof(result.fact_value) - 1);
                        snprintf(result.output_claim, sizeof(result.output_claim), "mentions %s instead", car_brands[b]);
                        result.confidence = 0.7f;
                        
                        fprintf(stderr, "[CONFLICT] Value substitution detected!\n");
                        fprintf(stderr, "  Fact: %s\n", node->value);
                        fprintf(stderr, "  Output says: %s\n", car_brands[b]);
                        
                        return result;
                    }
                }
            }
        }
    }
    
    return result;
}

// Guardrail wrapper - returns modified output if conflict detected
static inline const char* zeta_apply_conflict_guardrail(
    zeta_dual_ctx_t* ctx,
    const char* output,
    char* safe_output,
    size_t safe_output_size
) {
    zeta_conflict_result_t conflict = zeta_detect_conflict(ctx, output);
    
    if (conflict.has_conflict && conflict.confidence >= 0.7f) {
        // Prepend warning to output
        snprintf(safe_output, safe_output_size,
            "[MEMORY CONFLICT: My records show %s. Please verify.] %s",
            conflict.fact_value, output);
        return safe_output;
    }
    
    return output;  // No conflict, return original
}

#endif // ZETA_CONFLICT_H
