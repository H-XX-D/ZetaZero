// Z.E.T.A. 3B Semantic Extraction Engine
// Uses 3B model for robust fact extraction via structured prompts

#ifndef ZETA_3B_EXTRACT_H
#define ZETA_3B_EXTRACT_H

#include "llama.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZETA_EXTRACT_MAX_FACTS 16
#define ZETA_EXTRACT_MAX_LEN 256

// Extracted fact structure
typedef struct {
    char entity[128];      // e.g., "user_name", "favorite_color"
    char value[256];       // e.g., ">X<", "purple"
    float confidence;      // 0.0-1.0
    int importance;        // 1=low, 2=medium, 3=high, 4=critical
} zeta_extracted_fact_t;

// Extraction result
typedef struct {
    zeta_extracted_fact_t facts[ZETA_EXTRACT_MAX_FACTS];
    int num_facts;
} zeta_extraction_result_t;

// 3B extraction context
typedef struct {
    llama_model* model;
    llama_context* ctx;
    const llama_vocab* vocab;
    bool initialized;
} zeta_3b_extractor_t;

// Initialize 3B extractor
static inline zeta_3b_extractor_t* zeta_3b_extractor_init(llama_model* model_3b) {
    zeta_3b_extractor_t* ext = (zeta_3b_extractor_t*)calloc(1, sizeof(zeta_3b_extractor_t));
    if (!ext) return NULL;
    
    if (model_3b) {
        ext->model = model_3b;
        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 1024;
        cparams.n_batch = 128;
        ext->ctx = llama_init_from_model(model_3b, cparams);
        ext->vocab = llama_model_get_vocab(model_3b);
        ext->initialized = (ext->ctx != NULL);
    }
    
    return ext;
}

// Generate extraction prompt for 3B
static inline void zeta_build_extraction_prompt(const char* input, char* prompt, int max_len) {
    snprintf(prompt, max_len,
        "<|im_start|>system\n"
        "Extract facts from user input. Output one fact per line as: ENTITY|VALUE|IMPORTANCE\n"
        "Importance: 4=identity, 3=preference, 2=project, 1=other\n"
        "Examples:\n"
        "Input: My name is John\nOutput: user_name|John|4\n"
        "Input: I love blue\nOutput: favorite_color|blue|3\n"
        "<|im_end|>\n"
        "<|im_start|>user\n"
        "Extract facts from: %s\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n",
        input);
}

// Parse 3B output into facts
static inline int zeta_parse_extraction_output(
    const char* output,
    zeta_extraction_result_t* result
) {
    result->num_facts = 0;
    if (!output || !result) return 0;
    
    char* copy = strdup(output);
    char* line = strtok(copy, "\n");
    
    while (line && result->num_facts < ZETA_EXTRACT_MAX_FACTS) {
        // Skip empty lines
        while (*line == ' ') line++;
        if (*line == '\0') {
            line = strtok(NULL, "\n");
            continue;
        }
        
        // Parse: ENTITY|VALUE|IMPORTANCE
        char* entity = line;
        char* pipe1 = strchr(entity, '|');
        if (!pipe1) {
            line = strtok(NULL, "\n");
            continue;
        }
        *pipe1 = '\0';
        
        char* value = pipe1 + 1;
        char* pipe2 = strchr(value, '|');
        if (!pipe2) {
            line = strtok(NULL, "\n");
            continue;
        }
        *pipe2 = '\0';
        
        char* importance_str = pipe2 + 1;
        int importance = atoi(importance_str);
        if (importance < 1) importance = 1;
        if (importance > 4) importance = 4;
        
        // Store fact
        zeta_extracted_fact_t* fact = &result->facts[result->num_facts];
        strncpy(fact->entity, entity, sizeof(fact->entity) - 1);
        strncpy(fact->value, value, sizeof(fact->value) - 1);
        fact->importance = importance;
        fact->confidence = 0.7f + (importance * 0.075f); // 0.775 - 1.0
        
        result->num_facts++;
        line = strtok(NULL, "\n");
    }
    
    free(copy);
    return result->num_facts;
}

// Run 3B extraction (with model inference)
static inline int zeta_3b_extract_with_model(
    zeta_3b_extractor_t* ext,
    const char* input,
    zeta_extraction_result_t* result
) {
    if (!ext || !ext->initialized || !input || !result) return -1;
    
    result->num_facts = 0;
    
    // Build prompt
    char prompt[2048];
    zeta_build_extraction_prompt(input, prompt, sizeof(prompt));
    
    // Tokenize
    std::vector<llama_token> tokens(1024);
    int n_tokens = llama_tokenize(ext->vocab, prompt, strlen(prompt), 
                                   tokens.data(), tokens.size(), true, true);
    if (n_tokens <= 0) {
        fprintf(stderr, "[3B] Tokenization empty (n=%d)\n", n_tokens);
        return -1;
    }
    tokens.resize(n_tokens);
    
    // Clear context
    llama_memory_clear(llama_get_memory(ext->ctx), true);
    
    // Create batch and decode prompt
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    for (int i = 0; i < n_tokens; i++) {
        common_batch_add(batch, tokens[i], i, {0}, false);
    }
    batch.logits[batch.n_tokens - 1] = true;
    
    if (llama_decode(ext->ctx, batch) != 0) {
        llama_batch_free(batch);
        return -1;
    }
    
    // Generate output (max 256 tokens)
    std::string output;
    int n_cur = n_tokens;
    
    for (int i = 0; i < 256; i++) {
        float* logits = llama_get_logits_ith(ext->ctx, -1);
        
        // Greedy sampling for deterministic extraction
        llama_token best_token = 0;
        float best_logit = logits[0];
        int n_vocab = llama_vocab_n_tokens(ext->vocab);
        for (int j = 1; j < n_vocab; j++) {
            if (logits[j] > best_logit) {
                best_logit = logits[j];
                best_token = j;
            }
        }
        
        // Check for EOS
        if (llama_vocab_is_eog(ext->vocab, best_token)) break;
        
        // Check for end marker
        std::string piece = common_token_to_piece(ext->vocab, best_token, true);
        if (piece.find("<|im_end|>") != std::string::npos) break;
        
        output += piece;
        
        // Decode next
        llama_batch_free(batch);
        batch = llama_batch_init(1, 0, 1);
        common_batch_add(batch, best_token, n_cur++, {0}, true);
        
        if (llama_decode(ext->ctx, batch) != 0) break;
    }
    
    llama_batch_free(batch);
    
    // Parse output
    return zeta_parse_extraction_output(output.c_str(), result);
}

// Robust pattern-based extraction (fallback when no 3B model)
static inline int zeta_extract_robust_patterns(
    const char* input,
    zeta_extraction_result_t* result
) {
    if (!input || !result) return 0;
    result->num_facts = 0;
    
    size_t len = strlen(input);
    char* lower = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        lower[i] = (input[i] >= 'A' && input[i] <= 'Z') ? input[i] + 32 : input[i];
    }
    lower[len] = '\0';
    
    // Helper to extract value after pattern
    #define EXTRACT_VALUE(pattern, entity_name, importance_val) do { \
        const char* match = strstr(lower, pattern); \
        if (match && result->num_facts < ZETA_EXTRACT_MAX_FACTS) { \
            const char* val_start = input + (match - lower) + strlen(pattern); \
            char value[256] = {0}; \
            int vi = 0; \
            /* Skip leading spaces */ \
            while (*val_start == ' ') val_start++; \
            /* Extract until sentence boundary */ \
            while (*val_start && vi < 255) { \
                char c = *val_start; \
                /* Stop at sentence endings */ \
                if (c == '.' || c == '!' || c == '?' || c == '\n') break; \
                /* Stop at comma only if followed by space+conjunction */ \
                if (c == ',') { \
                    if (val_start[1] == ' ' && (val_start[2] == 'a' || val_start[2] == 'b' || \
                        val_start[2] == 's' || val_start[2] == 'w' || val_start[2] == 'i')) break; \
                } \
                /* Stop at " and " or " but " only if we have content */ \
                if (vi > 0 && c == ' ' && val_start[1] == 'a' && val_start[2] == 'n' && val_start[3] == 'd' && val_start[4] == ' ') break; \
                if (vi > 0 && c == ' ' && val_start[1] == 'b' && val_start[2] == 'u' && val_start[3] == 't' && val_start[4] == ' ') break; \
                value[vi++] = *val_start++; \
            } \
            /* Trim trailing spaces */ \
            while (vi > 0 && value[vi-1] == ' ') vi--; \
            value[vi] = '\0'; \
            if (vi > 0) { \
                zeta_extracted_fact_t* f = &result->facts[result->num_facts++]; \
                strncpy(f->entity, entity_name, sizeof(f->entity) - 1); \
                strncpy(f->value, value, sizeof(f->value) - 1); \
                f->importance = importance_val; \
                f->confidence = 0.7f + (importance_val * 0.075f); \
            } \
        } \
    } while(0)
    
    // Identity patterns (CRITICAL - importance 4)
    EXTRACT_VALUE("my name is ", "user_name", 4);
    EXTRACT_VALUE("i am called ", "user_name", 4);
    EXTRACT_VALUE("call me ", "user_name", 4);
    EXTRACT_VALUE("i'm ", "user_identity", 4);
    EXTRACT_VALUE("i am ", "user_identity", 4);
    
    // Preference patterns (HIGH - importance 3)
    EXTRACT_VALUE("favorite color is ", "favorite_color", 3);
    EXTRACT_VALUE("favourite color is ", "favorite_color", 3);
    EXTRACT_VALUE("favorite number is ", "favorite_number", 3);
    EXTRACT_VALUE("favourite number is ", "favorite_number", 3);
    EXTRACT_VALUE("favorite movie is ", "favorite_movie", 3);
    EXTRACT_VALUE("favorite book is ", "favorite_book", 3);
    EXTRACT_VALUE("favorite food is ", "favorite_food", 3);
    EXTRACT_VALUE("favorite song is ", "favorite_song", 3);
    EXTRACT_VALUE("i love ", "preference", 3);
    EXTRACT_VALUE("i like ", "preference", 3);
    EXTRACT_VALUE("i hate ", "dislike", 3);
    
    // Project patterns (MEDIUM - importance 2)
    EXTRACT_VALUE("codenamed ", "project_codename", 2);
    EXTRACT_VALUE("codename is ", "project_codename", 2);
    EXTRACT_VALUE("project is called ", "project_name", 2);
    EXTRACT_VALUE("working on ", "current_project", 2);
    EXTRACT_VALUE("building ", "building", 2);
    EXTRACT_VALUE("created ", "created", 2);
    EXTRACT_VALUE("developed ", "developed", 2);
    
    // Location patterns (LOW - importance 1)
    EXTRACT_VALUE("i live in ", "location", 1);
    EXTRACT_VALUE("i work at ", "workplace", 1);
    EXTRACT_VALUE("i'm from ", "origin", 1);
    
    #undef EXTRACT_VALUE
    
    free(lower);
    return result->num_facts;
}

// Main extraction function - uses 3B if available, falls back to patterns
static inline int zeta_3b_extract(
    zeta_3b_extractor_t* ext,
    const char* input,
    zeta_extraction_result_t* result
) {
    if (!input || !result) return 0;
    
    // Try 3B model extraction first
    if (ext && ext->initialized) {
        int n = zeta_3b_extract_with_model(ext, input, result);
        if (n > 0) {
            fprintf(stderr, "[3B] Model extracted %d facts\n", n);
            return n;
        }
    }
    
    // Fallback to robust pattern matching
    int n = zeta_extract_robust_patterns(input, result);
    if (n > 0) {
        fprintf(stderr, "[3B] Pattern extracted %d facts\n", n);
    }
    return n;
}

// Free extractor
static inline void zeta_3b_extractor_free(zeta_3b_extractor_t* ext) {
    if (ext) {
        if (ext->ctx) llama_free(ext->ctx);
        free(ext);
    }
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_3B_EXTRACT_H
