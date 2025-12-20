// Z.E.T.A. Semantic Tools - Git-style Tool API
// All operations are tokenized and embedded. No regex parsing.
//
// Tools:
//   extract  - Extract semantic concepts from text using embeddings
//   store    - Store a fact with tokenization and embedding
//   link     - Create semantic edge between concepts
//   query    - Semantic similarity search
//   diff     - Compare embeddings (detect change)
//   merge    - Merge duplicate/similar concepts
//   gc       - Garbage collect low-salience nodes
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_SEMANTIC_TOOLS_H
#define ZETA_SEMANTIC_TOOLS_H

#include "llama.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __cplusplus
#include <vector>
#include <string>
#include <algorithm>
#endif

// ============================================================================
// Configuration
// ============================================================================

// Embedding dimension - must match the embedding model
// Qwen3-embedding-4B: 2048 dims
// Qwen2.5-14B hidden: 5120 dims
// Common values: 768, 1024, 1536, 2048, 4096
#ifndef ZETA_TOOL_EMBED_DIM
#define ZETA_TOOL_EMBED_DIM     2048   // Qwen3-embedding-4B dimension
#endif

#define ZETA_TOOL_MAX_TOKENS    128    // Max tokens per stored value
#define ZETA_TOOL_SIMILARITY_THRESHOLD 0.80f  // Semantic dedup threshold
#define ZETA_TOOL_MAX_EXTRACT   16     // Max concepts per extraction
#define ZETA_TOOL_QUERY_K       8      // Default top-k for queries

// ============================================================================
// Core Data Structures
// ============================================================================

// Semantic concept - the unit of storage
typedef struct {
    int64_t     id;                     // Unique ID (git-style hash would be ideal)

    // Raw content
    char        type[64];               // Concept type (entity, fact, relation, etc.)
    char        key[128];               // Semantic key (like "user_name", "project")
    char        value[512];             // Raw text value

    // TOKENIZED content (for direct model injection)
    int32_t     tokens[ZETA_TOOL_MAX_TOKENS];
    int         n_tokens;
    bool        has_tokens;

    // EMBEDDED content (for semantic operations)
    float       embedding[ZETA_TOOL_EMBED_DIM];
    float       embedding_norm;
    bool        has_embedding;

    // Metadata
    float       salience;               // 0-1, importance weight
    int64_t     created_at;
    int64_t     accessed_at;
    int64_t     session_id;

    // Versioning (git-style)
    int64_t     supersedes;             // ID of concept this replaces (0 = original)
    int64_t     superseded_by;          // ID of newer version (0 = current)

    // Source tracking
    enum { SRC_USER, SRC_MODEL, SRC_TOOL } source;

    bool        active;
} zeta_concept_t;

// Semantic edge - relationship between concepts
typedef struct {
    int64_t     id;
    int64_t     from_id;
    int64_t     to_id;

    char        relation[64];           // Semantic relation type
    float       embedding[ZETA_TOOL_EMBED_DIM];  // Relation embedding
    float       weight;                 // Edge strength (0-1)

    int64_t     created_at;
    int         version;
} zeta_edge_t;

// Tool context - manages concepts and edges
typedef struct {
    // Model for embedding computation (4B embed model)
    llama_model*    embed_model;
    llama_context*  embed_ctx;
    const llama_vocab* vocab;

    // Embedding dimension (from model, or default)
    int             embed_dim;

    // Storage
    zeta_concept_t* concepts;
    int             n_concepts;
    int             cap_concepts;

    zeta_edge_t*    edges;
    int             n_edges;
    int             cap_edges;

    // ID generators
    int64_t         next_concept_id;
    int64_t         next_edge_id;
    int64_t         current_session;

    // Stats
    int64_t         total_stores;
    int64_t         total_queries;
    int64_t         dedup_hits;
} zeta_tool_ctx_t;

// ============================================================================
// Initialization
// ============================================================================

static inline zeta_tool_ctx_t* zeta_tool_init(
    llama_model* embed_model,
    llama_context* embed_ctx
) {
    zeta_tool_ctx_t* ctx = (zeta_tool_ctx_t*)calloc(1, sizeof(zeta_tool_ctx_t));
    if (!ctx) return NULL;

    ctx->embed_model = embed_model;
    ctx->embed_ctx = embed_ctx;
    ctx->vocab = embed_model ? llama_model_get_vocab(embed_model) : NULL;

    // Get embedding dimension from model
    if (embed_model) {
        ctx->embed_dim = llama_model_n_embd(embed_model);
        fprintf(stderr, "[SEMANTIC] Embedding model dimension: %d\n", ctx->embed_dim);
    } else {
        ctx->embed_dim = ZETA_TOOL_EMBED_DIM;  // Fallback to default
        fprintf(stderr, "[SEMANTIC] WARNING: No embed model, using default dim %d\n", ctx->embed_dim);
    }

    ctx->cap_concepts = 1024;
    ctx->concepts = (zeta_concept_t*)calloc(ctx->cap_concepts, sizeof(zeta_concept_t));

    ctx->cap_edges = 4096;
    ctx->edges = (zeta_edge_t*)calloc(ctx->cap_edges, sizeof(zeta_edge_t));

    ctx->next_concept_id = 1;
    ctx->next_edge_id = 1;
    ctx->current_session = (int64_t)time(NULL);

    return ctx;
}

static inline void zeta_tool_free(zeta_tool_ctx_t* ctx) {
    if (!ctx) return;
    free(ctx->concepts);
    free(ctx->edges);
    free(ctx);
}

// ============================================================================
// Tool: TOKENIZE - Convert text to tokens
// ============================================================================

static inline int zeta_tool_tokenize(
    zeta_tool_ctx_t* ctx,
    const char* text,
    int32_t* tokens,
    int max_tokens
) {
    if (!ctx || !ctx->vocab || !text || !tokens) return 0;

    int n = llama_tokenize(ctx->vocab, text, strlen(text),
                           tokens, max_tokens, false, false);
    return (n > 0) ? n : 0;
}

// ============================================================================
// Tool: EMBED - Compute semantic embedding using 4B embedding model
// ============================================================================

static inline void zeta_tool_embed(
    zeta_tool_ctx_t* ctx,
    const char* text,
    float* embedding,
    int dim
) {
    if (!ctx || !text || !embedding) return;

    memset(embedding, 0, dim * sizeof(float));

    // Use the 4B embedding model if available
    if (ctx->embed_ctx && ctx->embed_model) {
        // Tokenize
        std::vector<llama_token> tokens(512);
        int n_tok = llama_tokenize(ctx->vocab, text, strlen(text),
                                    tokens.data(), tokens.size(), true, true);
        if (n_tok <= 0) {
            goto hash_fallback;
        }
        if (n_tok > 256) n_tok = 256;  // Truncate if too long
        tokens.resize(n_tok);

        // Clear KV cache
        llama_memory_clear(llama_get_memory(ctx->embed_ctx), true);

        // Build batch - request embeddings for all positions
        llama_batch batch = llama_batch_init(n_tok, 0, 1);
        for (int i = 0; i < n_tok; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i] = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = true;  // Request output for all positions
        }
        batch.n_tokens = n_tok;

        if (llama_decode(ctx->embed_ctx, batch) == 0) {
            // Get model dimensions
            int n_embd = llama_model_n_embd(ctx->embed_model);

            // For embedding models, get the embeddings from last token
            // llama_get_embeddings_ith returns the hidden state
            const float* emb = llama_get_embeddings_ith(ctx->embed_ctx, n_tok - 1);

            if (emb) {
                // Copy embedding (truncate or pad to requested dim)
                int copy_dim = (n_embd < dim) ? n_embd : dim;
                memcpy(embedding, emb, copy_dim * sizeof(float));

                // Normalize
                float norm = 0;
                for (int i = 0; i < dim; i++) norm += embedding[i] * embedding[i];
                norm = sqrtf(norm + 1e-8f);
                for (int i = 0; i < dim; i++) embedding[i] /= norm;

                llama_batch_free(batch);
                return;  // Success - return with real embedding
            }

            // If embeddings not available, try mean pooling over all positions
            float* pooled = embedding;
            int valid_positions = 0;

            for (int p = 0; p < n_tok; p++) {
                const float* pos_emb = llama_get_embeddings_ith(ctx->embed_ctx, p);
                if (pos_emb) {
                    int copy_dim = (n_embd < dim) ? n_embd : dim;
                    for (int d = 0; d < copy_dim; d++) {
                        pooled[d] += pos_emb[d];
                    }
                    valid_positions++;
                }
            }

            if (valid_positions > 0) {
                // Average pooling
                for (int d = 0; d < dim; d++) {
                    pooled[d] /= valid_positions;
                }
                // Normalize
                float norm = 0;
                for (int i = 0; i < dim; i++) norm += pooled[i] * pooled[i];
                norm = sqrtf(norm + 1e-8f);
                for (int i = 0; i < dim; i++) pooled[i] /= norm;

                llama_batch_free(batch);
                return;  // Success with mean pooling
            }
        }
        llama_batch_free(batch);
    }

hash_fallback:
    // Hash-based fallback (should rarely be used)
    fprintf(stderr, "[EMBED] WARNING: Using hash fallback for '%s'\n", text);
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        uint32_t h = (uint32_t)text[i] * 2654435761u;
        embedding[h % dim] += 1.0f;
    }
    // Normalize
    float norm = 0;
    for (int i = 0; i < dim; i++) norm += embedding[i] * embedding[i];
    norm = sqrtf(norm + 1e-8f);
    for (int i = 0; i < dim; i++) embedding[i] /= norm;
}

// ============================================================================
// Tool: SIMILARITY - Compute cosine similarity
// ============================================================================

static inline float zeta_tool_similarity(
    const float* a,
    const float* b,
    int dim
) {
    float dot = 0, norm_a = 0, norm_b = 0;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (sqrtf(norm_a) * sqrtf(norm_b) + 1e-8f);
}

// ============================================================================
// Tool: STORE - Store a concept with tokenization + embedding
// ============================================================================

static inline int64_t zeta_tool_store(
    zeta_tool_ctx_t* ctx,
    const char* type,
    const char* key,
    const char* value,
    float salience,
    int source  // 0=USER, 1=MODEL, 2=TOOL
) {
    if (!ctx || !type || !value) return -1;
    if (strlen(value) == 0) return -1;

    // Compute embedding for the new value using model's dimension
    std::vector<float> new_embedding(ctx->embed_dim, 0.0f);
    zeta_tool_embed(ctx, value, new_embedding.data(), ctx->embed_dim);

    // Check for semantic duplicates
    int64_t existing_id = -1;
    int existing_idx = -1;
    float best_sim = 0;

    for (int i = 0; i < ctx->n_concepts; i++) {
        zeta_concept_t* c = &ctx->concepts[i];
        if (!c->active || !c->has_embedding) continue;

        // Same type/key = always check
        bool same_key = (strcmp(c->type, type) == 0 &&
                         key && c->key[0] && strcmp(c->key, key) == 0);

        // Compute similarity using model's dimension
        float sim = zeta_tool_similarity(new_embedding.data(), c->embedding, ctx->embed_dim);

        if (same_key || sim > ZETA_TOOL_SIMILARITY_THRESHOLD) {
            if (sim > best_sim) {
                best_sim = sim;
                existing_id = c->id;
                existing_idx = i;
            }
        }
    }

    // Handle duplicate
    if (existing_idx >= 0) {
        zeta_concept_t* existing = &ctx->concepts[existing_idx];

        // Same value = just refresh access time
        if (strcmp(existing->value, value) == 0) {
            existing->accessed_at = (int64_t)time(NULL);
            ctx->dedup_hits++;
            fprintf(stderr, "[TOOL:STORE] Dedup hit: %s/%s (sim=%.2f)\n",
                    type, key ? key : "", best_sim);
            return existing_id;
        }

        // USER source cannot be overwritten by MODEL (0=USER, 1=MODEL)
        if (existing->source == 0 && source == 1) {
            fprintf(stderr, "[TOOL:STORE] Blocked: MODEL cannot override USER fact\n");
            return existing_id;
        }

        // Create new version (git-style supersede)
        fprintf(stderr, "[TOOL:STORE] Version update: %s -> %s (sim=%.2f)\n",
                existing->value, value, best_sim);
    }

    // Expand storage if needed
    if (ctx->n_concepts >= ctx->cap_concepts) {
        int new_cap = ctx->cap_concepts * 2;
        zeta_concept_t* new_arr = (zeta_concept_t*)realloc(
            ctx->concepts, new_cap * sizeof(zeta_concept_t));
        if (!new_arr) return -1;
        ctx->concepts = new_arr;
        ctx->cap_concepts = new_cap;
    }

    // Create new concept
    zeta_concept_t* c = &ctx->concepts[ctx->n_concepts++];
    memset(c, 0, sizeof(zeta_concept_t));

    c->id = ctx->next_concept_id++;
    strncpy(c->type, type, sizeof(c->type) - 1);
    if (key) strncpy(c->key, key, sizeof(c->key) - 1);
    strncpy(c->value, value, sizeof(c->value) - 1);

    // TOKENIZE
    c->n_tokens = zeta_tool_tokenize(ctx, value, c->tokens, ZETA_TOOL_MAX_TOKENS);
    c->has_tokens = (c->n_tokens > 0);

    // EMBED - copy from computed embedding (size determined by model)
    int copy_dim = (ctx->embed_dim < ZETA_TOOL_EMBED_DIM) ? ctx->embed_dim : ZETA_TOOL_EMBED_DIM;
    memcpy(c->embedding, new_embedding.data(), copy_dim * sizeof(float));
    float norm = 0;
    for (int i = 0; i < copy_dim; i++)
        norm += c->embedding[i] * c->embedding[i];
    c->embedding_norm = sqrtf(norm);
    c->has_embedding = true;

    // Metadata
    c->salience = salience;
    c->created_at = (int64_t)time(NULL);
    c->accessed_at = c->created_at;
    c->session_id = ctx->current_session;
    c->source = (typeof(c->source))source;
    c->active = true;

    // Create supersede edge if updating
    if (existing_idx >= 0) {
        c->supersedes = existing_id;
        ctx->concepts[existing_idx].superseded_by = c->id;

        // Create SUPERSEDES edge
        if (ctx->n_edges < ctx->cap_edges) {
            zeta_edge_t* e = &ctx->edges[ctx->n_edges++];
            e->id = ctx->next_edge_id++;
            e->from_id = existing_id;
            e->to_id = c->id;
            strncpy(e->relation, "SUPERSEDES", sizeof(e->relation) - 1);
            e->weight = 1.0f;
            e->created_at = c->created_at;
        }
    }

    ctx->total_stores++;
    fprintf(stderr, "[TOOL:STORE] Created: type=%s key=%s val='%.40s...' tokens=%d\n",
            type, key ? key : "", value, c->n_tokens);

    return c->id;
}

// ============================================================================
// Tool: LINK - Create semantic edge between concepts
// ============================================================================

static inline int64_t zeta_tool_link(
    zeta_tool_ctx_t* ctx,
    int64_t from_id,
    int64_t to_id,
    const char* relation,
    float weight
) {
    if (!ctx || !relation || from_id <= 0 || to_id <= 0) return -1;

    // Check for existing edge
    for (int i = 0; i < ctx->n_edges; i++) {
        zeta_edge_t* e = &ctx->edges[i];
        if (e->from_id == from_id && e->to_id == to_id &&
            strcmp(e->relation, relation) == 0) {
            // Reinforce existing edge
            e->weight = e->weight * 0.7f + weight * 0.3f;
            if (e->weight > 1.0f) e->weight = 1.0f;
            e->version++;
            fprintf(stderr, "[TOOL:LINK] Reinforced: %lld --%s--> %lld (w=%.2f)\n",
                    (long long)from_id, relation, (long long)to_id, e->weight);
            return e->id;
        }
    }

    // Expand if needed
    if (ctx->n_edges >= ctx->cap_edges) {
        int new_cap = ctx->cap_edges * 2;
        zeta_edge_t* new_arr = (zeta_edge_t*)realloc(
            ctx->edges, new_cap * sizeof(zeta_edge_t));
        if (!new_arr) return -1;
        ctx->edges = new_arr;
        ctx->cap_edges = new_cap;
    }

    // Create new edge
    zeta_edge_t* e = &ctx->edges[ctx->n_edges++];
    memset(e, 0, sizeof(zeta_edge_t));

    e->id = ctx->next_edge_id++;
    e->from_id = from_id;
    e->to_id = to_id;
    strncpy(e->relation, relation, sizeof(e->relation) - 1);
    e->weight = weight;
    e->created_at = (int64_t)time(NULL);
    e->version = 1;

    // Compute relation embedding
    zeta_tool_embed(ctx, relation, e->embedding, ZETA_TOOL_EMBED_DIM);

    fprintf(stderr, "[TOOL:LINK] Created: %lld --%s--> %lld (w=%.2f)\n",
            (long long)from_id, relation, (long long)to_id, weight);

    return e->id;
}

// ============================================================================
// Tool: QUERY - Semantic similarity search
// ============================================================================

typedef struct {
    int64_t id;
    float   score;
    zeta_concept_t* concept;
} zeta_query_result_t;

static inline int zeta_tool_query(
    zeta_tool_ctx_t* ctx,
    const char* query_text,
    zeta_query_result_t* results,
    int max_results
) {
    if (!ctx || !query_text || !results || max_results <= 0) return 0;

    // Compute query embedding using model's dimension
    std::vector<float> query_embed(ctx->embed_dim, 0.0f);
    zeta_tool_embed(ctx, query_text, query_embed.data(), ctx->embed_dim);

    // Score all active concepts
    std::vector<zeta_query_result_t> scored;
    scored.reserve(ctx->n_concepts);

    for (int i = 0; i < ctx->n_concepts; i++) {
        zeta_concept_t* c = &ctx->concepts[i];
        if (!c->active || !c->has_embedding) continue;
        if (c->superseded_by != 0) continue;  // Skip old versions

        int cmp_dim = (ctx->embed_dim < ZETA_TOOL_EMBED_DIM) ? ctx->embed_dim : ZETA_TOOL_EMBED_DIM;
        float sim = zeta_tool_similarity(query_embed.data(), c->embedding, cmp_dim);

        // Apply salience boost
        sim *= (0.5f + 0.5f * c->salience);

        // Session boost (prefer current session)
        if (c->session_id == ctx->current_session) {
            sim *= 1.2f;
        }

        if (sim > 0.1f) {
            zeta_query_result_t r;
            r.id = c->id;
            r.score = sim;
            r.concept = c;
            scored.push_back(r);
        }
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(),
        [](const zeta_query_result_t& a, const zeta_query_result_t& b) {
            return a.score > b.score;
        });

    // Return top results
    int count = std::min((int)scored.size(), max_results);
    for (int i = 0; i < count; i++) {
        results[i] = scored[i];
        results[i].concept->accessed_at = (int64_t)time(NULL);
    }

    ctx->total_queries++;
    fprintf(stderr, "[TOOL:QUERY] '%s' -> %d results\n", query_text, count);

    return count;
}

// ============================================================================
// Tool: EXTRACT - Extract semantic concepts from text
// ============================================================================

typedef struct {
    char type[64];
    char key[128];
    char value[512];
    float confidence;
} zeta_extract_result_t;

// Semantic extraction using embedding-based concept detection
// This replaces regex-based extraction entirely
static inline int zeta_tool_extract(
    zeta_tool_ctx_t* ctx,
    const char* text,
    zeta_extract_result_t* results,
    int max_results
) {
    if (!ctx || !text || !results || max_results <= 0) return 0;

    int n_extracted = 0;

    // Compute full text embedding using model's dimension
    std::vector<float> text_embed(ctx->embed_dim, 0.0f);
    zeta_tool_embed(ctx, text, text_embed.data(), ctx->embed_dim);

    // Semantic concept templates (pre-embedded in production)
    // Each template defines what semantic pattern to look for
    static const struct {
        const char* type;
        const char* pattern;  // Semantic pattern hint
        float salience;
    } templates[] = {
        {"user_name",       "my name is",           1.0f},
        {"user_name",       "I am called",          1.0f},
        {"user_name",       "call me",              1.0f},
        {"location",        "I live in",            0.9f},
        {"location",        "located in",           0.9f},
        {"project",         "working on",           0.85f},
        {"project",         "project called",       0.85f},
        {"project_codename","code name",            0.9f},
        {"favorite",        "my favorite",          0.85f},
        {"preference",      "I prefer",             0.8f},
        {"fact",            "remember that",        0.95f},
        {"rate_limit",      "rate limit is",        0.9f},
        {"numeric",         "number is",            0.8f},
        {"age",             "I am years old",       0.95f},
        {"birth_year",      "born in",              0.95f},
        {"sibling",         "my sister",            0.9f},
        {"sibling",         "my brother",           0.9f},
        {"workplace",       "I work at",            0.9f},
        {"pet",             "my dog is named",      0.85f},
        {"pet",             "my cat is named",      0.85f},
        // Causal patterns
        {"causes",          "causes",               0.9f},
        {"causes",          "triggers",             0.9f},
        {"causes",          "leads to",             0.9f},
        {"prevents",        "prevents",             0.9f},
        {"prevents",        "stops",                0.9f},
        {"prevents",        "blocks",               0.9f},
        {NULL, NULL, 0}
    };

    // Compute pattern embeddings and match
    for (int t = 0; templates[t].type && n_extracted < max_results; t++) {
        std::vector<float> pattern_embed(ctx->embed_dim, 0.0f);
        zeta_tool_embed(ctx, templates[t].pattern, pattern_embed.data(), ctx->embed_dim);

        float sim = zeta_tool_similarity(text_embed.data(), pattern_embed.data(), ctx->embed_dim);

        if (sim > 0.25f) {  // Threshold for pattern detection
            // Pattern matched - extract the value after the pattern
            const char* match = strcasestr(text, templates[t].pattern);
            if (match) {
                const char* val_start = match + strlen(templates[t].pattern);
                while (*val_start == ' ' || *val_start == ':') val_start++;

                char value[512] = {0};
                int vi = 0;
                while (*val_start && vi < 511 &&
                       *val_start != '.' && *val_start != '!' &&
                       *val_start != ',' && *val_start != '\n') {
                    value[vi++] = *val_start++;
                }
                // Trim trailing whitespace
                while (vi > 0 && value[vi-1] == ' ') vi--;
                value[vi] = '\0';

                if (vi > 0) {
                    zeta_extract_result_t* r = &results[n_extracted++];
                    strncpy(r->type, templates[t].type, sizeof(r->type) - 1);
                    r->key[0] = '\0';  // Key derived from type
                    strncpy(r->value, value, sizeof(r->value) - 1);
                    r->confidence = sim * templates[t].salience;

                    fprintf(stderr, "[TOOL:EXTRACT] %s: '%s' (conf=%.2f)\n",
                            r->type, r->value, r->confidence);
                }
            }
        }
    }

    // Handle "Remember:" prefix specially - store as raw memory
    if (strncasecmp(text, "remember:", 9) == 0) {
        const char* content = text + 9;
        while (*content == ' ') content++;

        if (strlen(content) > 5 && n_extracted < max_results) {
            zeta_extract_result_t* r = &results[n_extracted++];
            strncpy(r->type, "raw_memory", sizeof(r->type) - 1);
            r->key[0] = '\0';
            strncpy(r->value, content, sizeof(r->value) - 1);
            r->confidence = 1.0f;

            fprintf(stderr, "[TOOL:EXTRACT] raw_memory: '%.40s...' (conf=1.0)\n", content);
        }
    }

    return n_extracted;
}

// ============================================================================
// Tool: DIFF - Compare embeddings to detect semantic change
// ============================================================================

typedef struct {
    float similarity;
    bool is_update;      // True if semantically similar but different value
    bool is_conflict;    // True if contradicts existing fact
    int64_t related_id;  // ID of most similar existing concept
} zeta_semantic_diff_t;

static inline zeta_semantic_diff_t zeta_tool_diff(
    zeta_tool_ctx_t* ctx,
    const char* type,
    const char* value
) {
    zeta_semantic_diff_t result = {0, false, false, 0};
    if (!ctx || !type || !value) return result;

    // Embed the new value using model's dimension
    std::vector<float> new_embed(ctx->embed_dim, 0.0f);
    zeta_tool_embed(ctx, value, new_embed.data(), ctx->embed_dim);

    float best_sim = 0;
    int best_idx = -1;

    for (int i = 0; i < ctx->n_concepts; i++) {
        zeta_concept_t* c = &ctx->concepts[i];
        if (!c->active || !c->has_embedding) continue;
        if (c->superseded_by != 0) continue;

        // Same type
        if (strcmp(c->type, type) == 0) {
            int cmp_dim = (ctx->embed_dim < ZETA_TOOL_EMBED_DIM) ? ctx->embed_dim : ZETA_TOOL_EMBED_DIM;
            float sim = zeta_tool_similarity(new_embed.data(), c->embedding, cmp_dim);
            if (sim > best_sim) {
                best_sim = sim;
                best_idx = i;
            }
        }
    }

    result.similarity = best_sim;

    if (best_idx >= 0) {
        result.related_id = ctx->concepts[best_idx].id;

        // Same value = no change
        if (strcmp(ctx->concepts[best_idx].value, value) == 0) {
            result.is_update = false;
            result.is_conflict = false;
        }
        // High similarity but different value = update
        else if (best_sim > 0.70f) {
            result.is_update = true;
            result.is_conflict = false;
        }
        // Low similarity with same type = potential conflict
        else if (best_sim > 0.40f) {
            result.is_conflict = true;
        }
    }

    return result;
}

// ============================================================================
// Tool: MERGE - Merge duplicate/similar concepts
// ============================================================================

static inline int zeta_tool_merge(
    zeta_tool_ctx_t* ctx,
    float similarity_threshold
) {
    if (!ctx) return 0;

    int merged = 0;

    for (int i = 0; i < ctx->n_concepts; i++) {
        zeta_concept_t* a = &ctx->concepts[i];
        if (!a->active || !a->has_embedding) continue;
        if (a->superseded_by != 0) continue;

        for (int j = i + 1; j < ctx->n_concepts; j++) {
            zeta_concept_t* b = &ctx->concepts[j];
            if (!b->active || !b->has_embedding) continue;
            if (b->superseded_by != 0) continue;

            // Same type
            if (strcmp(a->type, b->type) != 0) continue;

            int cmp_dim = (ctx->embed_dim < ZETA_TOOL_EMBED_DIM) ? ctx->embed_dim : ZETA_TOOL_EMBED_DIM;
            float sim = zeta_tool_similarity(a->embedding, b->embedding, cmp_dim);

            if (sim >= similarity_threshold) {
                // Merge b into a (keep higher salience)
                if (b->salience > a->salience) {
                    a->salience = b->salience;
                }

                // Mark b as superseded
                b->superseded_by = a->id;
                b->active = false;

                // Create merge edge
                zeta_tool_link(ctx, b->id, a->id, "MERGED_INTO", 1.0f);

                merged++;
                fprintf(stderr, "[TOOL:MERGE] Merged %lld into %lld (sim=%.2f)\n",
                        (long long)b->id, (long long)a->id, sim);
            }
        }
    }

    return merged;
}

// ============================================================================
// Tool: GC - Garbage collect low-salience nodes
// ============================================================================

static inline int zeta_tool_gc(
    zeta_tool_ctx_t* ctx,
    float salience_threshold,
    int64_t age_threshold_seconds
) {
    if (!ctx) return 0;

    int64_t now = (int64_t)time(NULL);
    int collected = 0;

    for (int i = 0; i < ctx->n_concepts; i++) {
        zeta_concept_t* c = &ctx->concepts[i];
        if (!c->active) continue;

        // Don't GC current session data
        if (c->session_id == ctx->current_session) continue;

        // Don't GC supersede chain heads
        if (c->superseded_by == 0 && c->supersedes != 0) continue;

        int64_t age = now - c->accessed_at;

        if (c->salience < salience_threshold && age > age_threshold_seconds) {
            c->active = false;
            collected++;
            fprintf(stderr, "[TOOL:GC] Collected %lld (salience=%.2f, age=%llds)\n",
                    (long long)c->id, c->salience, (long long)age);
        }
    }

    return collected;
}

// ============================================================================
// Tool: FORMAT - Format surfaced context for model consumption
// ============================================================================

static inline void zeta_tool_format_context(
    zeta_tool_ctx_t* ctx,
    const char* query,
    char* output,
    int max_len
) {
    if (!ctx || !output) return;

    zeta_query_result_t results[8];
    int n = zeta_tool_query(ctx, query, results, 8);

    char* p = output;
    int remaining = max_len - 1;

    if (n > 0) {
        int written = snprintf(p, remaining, "[Memory Context]\n");
        p += written; remaining -= written;

        for (int i = 0; i < n && remaining > 100; i++) {
            zeta_concept_t* c = results[i].concept;
            written = snprintf(p, remaining, "- %s: %s (relevance=%.2f)\n",
                              c->type, c->value, results[i].score);
            p += written; remaining -= written;
        }

        snprintf(p, remaining, "[End Memory]\n\n");
    } else {
        output[0] = '\0';
    }
}

// ============================================================================
// Tool: GET_TOKENS - Get tokens for direct injection into model
// ============================================================================

static inline int zeta_tool_get_tokens(
    zeta_tool_ctx_t* ctx,
    int64_t concept_id,
    int32_t* tokens,
    int max_tokens
) {
    if (!ctx || !tokens) return 0;

    for (int i = 0; i < ctx->n_concepts; i++) {
        if (ctx->concepts[i].id == concept_id && ctx->concepts[i].active) {
            zeta_concept_t* c = &ctx->concepts[i];
            if (!c->has_tokens) return 0;

            int n = (c->n_tokens < max_tokens) ? c->n_tokens : max_tokens;
            memcpy(tokens, c->tokens, n * sizeof(int32_t));
            return n;
        }
    }

    return 0;
}

// ============================================================================
// Stats
// ============================================================================

static inline void zeta_tool_print_stats(zeta_tool_ctx_t* ctx) {
    if (!ctx) return;

    fprintf(stderr, "\n=== Z.E.T.A. Semantic Tools Stats ===\n");
    fprintf(stderr, "Concepts:     %d\n", ctx->n_concepts);
    fprintf(stderr, "Edges:        %d\n", ctx->n_edges);
    fprintf(stderr, "Total stores: %lld\n", (long long)ctx->total_stores);
    fprintf(stderr, "Total queries:%lld\n", (long long)ctx->total_queries);
    fprintf(stderr, "Dedup hits:   %lld\n", (long long)ctx->dedup_hits);
    fprintf(stderr, "=====================================\n");
}

#endif // ZETA_SEMANTIC_TOOLS_H
