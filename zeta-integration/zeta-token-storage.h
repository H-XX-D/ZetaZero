// Z.E.T.A. Token-Based Storage
// Store facts as token IDs instead of raw text to eliminate repeated tokenization

#ifndef ZETA_TOKEN_STORAGE_H
#define ZETA_TOKEN_STORAGE_H

#include "llama.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ZETA_MAX_FACT_TOKENS 256  // Max tokens per fact

// Token-based fact storage
typedef struct {
    int64_t fact_id;
    llama_token tokens[ZETA_MAX_FACT_TOKENS];
    int num_tokens;
    float salience;
    time_t last_accessed;
    float embedding[1536];  // Pre-computed embedding for similarity
    bool has_embedding;
} zeta_token_fact_t;

#define ZETA_MAX_TOKEN_FACTS 1024
static zeta_token_fact_t g_token_facts[ZETA_MAX_TOKEN_FACTS] = {0};
static int g_num_token_facts = 0;

// Tokenize and store a fact
static inline int64_t zeta_store_tokenized(
    llama_model* model,
    const char* text,
    float salience
) {
    if (!model || !text || g_num_token_facts >= ZETA_MAX_TOKEN_FACTS) return -1;
    
    zeta_token_fact_t* fact = &g_token_facts[g_num_token_facts];
    fact->fact_id = g_num_token_facts + 1000;  // Offset to distinguish from graph node IDs
    
    // Tokenize once
    fact->num_tokens = llama_tokenize(
        model, text, strlen(text),
        fact->tokens, ZETA_MAX_FACT_TOKENS,
        true,   // add_special
        false   // parse_special
    );
    
    if (fact->num_tokens <= 0) return -1;
    
    fact->salience = salience;
    fact->last_accessed = time(NULL);
    fact->has_embedding = false;
    
    g_num_token_facts++;
    
    fprintf(stderr, "[TOKEN-STORE] Stored fact %ld: %d tokens (vs ~%zu chars)\n",
            fact->fact_id, fact->num_tokens, strlen(text));
    
    return fact->fact_id;
}

// Get tokens directly for context injection (no re-tokenization!)
static inline int zeta_get_fact_tokens(
    int64_t fact_id,
    llama_token* out_tokens,
    int max_tokens
) {
    for (int i = 0; i < g_num_token_facts; i++) {
        if (g_token_facts[i].fact_id == fact_id) {
            int copy = g_token_facts[i].num_tokens;
            if (copy > max_tokens) copy = max_tokens;
            memcpy(out_tokens, g_token_facts[i].tokens, copy * sizeof(llama_token));
            g_token_facts[i].last_accessed = time(NULL);
            return copy;
        }
    }
    return 0;
}

// Detokenize only when needed for display
static inline int zeta_detokenize_fact(
    llama_model* model,
    int64_t fact_id,
    char* out_text,
    int max_len
) {
    for (int i = 0; i < g_num_token_facts; i++) {
        if (g_token_facts[i].fact_id == fact_id) {
            int offset = 0;
            for (int t = 0; t < g_token_facts[i].num_tokens && offset < max_len - 10; t++) {
                char piece[64];
                int piece_len = llama_token_to_piece(
                    model, g_token_facts[i].tokens[t],
                    piece, sizeof(piece), 0, false
                );
                if (piece_len > 0 && offset + piece_len < max_len) {
                    memcpy(out_text + offset, piece, piece_len);
                    offset += piece_len;
                }
            }
            out_text[offset] = '\0';
            return offset;
        }
    }
    return 0;
}

// Get storage stats
static inline void zeta_token_storage_stats(
    int* num_facts,
    int* total_tokens,
    int* avg_tokens_per_fact
) {
    *num_facts = g_num_token_facts;
    *total_tokens = 0;
    for (int i = 0; i < g_num_token_facts; i++) {
        *total_tokens += g_token_facts[i].num_tokens;
    }
    *avg_tokens_per_fact = g_num_token_facts > 0 ? *total_tokens / g_num_token_facts : 0;
}

// Size comparison: tokens vs text
// Tokens: num_tokens * 4 bytes (int32)
// Text: strlen bytes + 1 null
// Example: "The Ancient Dragon awakens the Frost Giant" = 42 chars = 42 bytes
// As tokens: ~10 tokens * 4 bytes = 40 bytes (similar, but no re-tokenization!)
// Real savings come from eliminating repeated tokenize/detokenize cycles

#endif // ZETA_TOKEN_STORAGE_H
