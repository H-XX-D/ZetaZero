// Z.E.T.A. Embedding Model Integration
// The Hippocampus - Pattern matching for semantic retrieval
//
// This replaces hash-based embeddings with real semantic embeddings
// using a small, fast embedding model (bge-small, nomic-embed, etc.)

#ifndef ZETA_EMBED_INTEGRATION_H
#define ZETA_EMBED_INTEGRATION_H

#include "llama.h"
#include <cstring>
#include <cmath>
#include <vector>

// Embedding model context
struct zeta_embed_ctx {
    llama_model* model;
    llama_context* ctx;
    int embed_dim;        // Output dimension (384 for bge-small, 768 for nomic)
    bool initialized;
};

// Global embedding context (singleton for now)
static zeta_embed_ctx* g_embed_ctx = nullptr;

// Initialize the embedding model
// model_path: path to GGUF embedding model (e.g., bge-small-en-v1.5-q8_0.gguf)
static bool zeta_embed_init(const char* model_path) {
    if (g_embed_ctx && g_embed_ctx->initialized) {
        fprintf(stderr, "[EMBED] Already initialized\n");
        return true;
    }

    g_embed_ctx = new zeta_embed_ctx();
    memset(g_embed_ctx, 0, sizeof(zeta_embed_ctx));

    // Load model
    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0;   // Keep embedding model on CPU to avoid GPU OOM in 7B+7B configs

    fprintf(stderr, "[EMBED] Loading embedding model: %s\n", model_path);
    g_embed_ctx->model = llama_model_load_from_file(model_path, mparams);

    if (!g_embed_ctx->model) {
        fprintf(stderr, "[EMBED] ERROR: Failed to load embedding model\n");
        delete g_embed_ctx;
        g_embed_ctx = nullptr;
        return false;
    }

    // Create context with embeddings ENABLED
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 512;           // Small context for embedding queries
    cparams.n_batch = 512;
    cparams.embeddings = true;     // CRITICAL: Enable embeddings output
    // flash_attn not needed for embedding models

    g_embed_ctx->ctx = llama_init_from_model(g_embed_ctx->model, cparams);

    if (!g_embed_ctx->ctx) {
        fprintf(stderr, "[EMBED] ERROR: Failed to create embedding context\n");
        llama_model_free(g_embed_ctx->model);
        delete g_embed_ctx;
        g_embed_ctx = nullptr;
        return false;
    }

    // Get embedding dimension from model
    g_embed_ctx->embed_dim = llama_model_n_embd(g_embed_ctx->model);
    g_embed_ctx->initialized = true;

    fprintf(stderr, "[EMBED] Initialized: dim=%d\n", g_embed_ctx->embed_dim);
    return true;
}

// Free embedding model resources
static void zeta_embed_free() {
    if (g_embed_ctx) {
        if (g_embed_ctx->ctx) {
            llama_free(g_embed_ctx->ctx);
        }
        if (g_embed_ctx->model) {
            llama_model_free(g_embed_ctx->model);
        }
        delete g_embed_ctx;
        g_embed_ctx = nullptr;
    }
}

// Get embedding for text using the embedding model
// Returns the embedding dimension, or -1 on error
// output must have space for at least embed_dim floats
static int zeta_embed_text(const char* text, float* output, int max_dim) {
    if (!g_embed_ctx || !g_embed_ctx->initialized) {
        fprintf(stderr, "[EMBED] ERROR: Not initialized\n");
        return -1;
    }

    if (!text || !output) {
        return -1;
    }

    // Tokenize the input
    const llama_vocab* vocab = llama_model_get_vocab(g_embed_ctx->model);
    int max_tokens = 256;
    std::vector<llama_token> tokens(max_tokens);

    int n_tokens = llama_tokenize(vocab, text, strlen(text),
                                   tokens.data(), max_tokens,
                                   true,   // add_special (BOS)
                                   true);  // parse_special

    if (n_tokens <= 0) {
        fprintf(stderr, "[EMBED] ERROR: Tokenization failed\n");
        return -1;
    }

    tokens.resize(n_tokens);

    // Clear memory/KV cache for new embedding
    llama_memory_clear(llama_get_memory(g_embed_ctx->ctx), true);

    // Create batch
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);

    for (int i = 0; i < n_tokens; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = 1;  // Mark ALL tokens for embedding output (pooling needs all)
    }
    batch.n_tokens = n_tokens;

    // Run inference
    if (llama_decode(g_embed_ctx->ctx, batch) != 0) {
        fprintf(stderr, "[EMBED] ERROR: Decode failed\n");
        llama_batch_free(batch);
        return -1;
    }

    // Get embeddings - use sequence embeddings if available, else last token
    const float* embeddings = llama_get_embeddings_seq(g_embed_ctx->ctx, 0);
    if (!embeddings) {
        // Fall back to last token embedding
        embeddings = llama_get_embeddings_ith(g_embed_ctx->ctx, n_tokens - 1);
    }

    if (!embeddings) {
        fprintf(stderr, "[EMBED] ERROR: No embeddings returned\n");
        llama_batch_free(batch);
        return -1;
    }

    // Copy to output (truncate or pad to max_dim if needed)
    int copy_dim = (g_embed_ctx->embed_dim < max_dim) ? g_embed_ctx->embed_dim : max_dim;
    memcpy(output, embeddings, copy_dim * sizeof(float));

    // Normalize the embedding (L2 norm)
    float norm = 0.0f;
    for (int i = 0; i < copy_dim; i++) {
        norm += output[i] * output[i];
    }
    norm = sqrtf(norm);
    if (norm > 1e-8f) {
        for (int i = 0; i < copy_dim; i++) {
            output[i] /= norm;
        }
    }

    llama_batch_free(batch);
    return copy_dim;
}

// Compute cosine similarity between two embeddings
static float zeta_embed_similarity(const float* a, const float* b, int dim) {
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;

    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    float denom = sqrtf(norm_a) * sqrtf(norm_b);
    if (denom < 1e-8f) return 0.0f;

    return dot / denom;
}

// Sharpened similarity (as per Z.E.T.A. formalization)
// E(q, e) = cos(q, e)^kappa where kappa=3
static float zeta_embed_similarity_sharp(const float* a, const float* b, int dim, float kappa = 3.0f) {
    float sim = zeta_embed_similarity(a, b, dim);
    // Only sharpen positive similarities
    if (sim > 0) {
        return powf(sim, kappa);
    }
    return sim;
}

// Check if embedding model is ready
static bool zeta_embed_ready() {
    return g_embed_ctx && g_embed_ctx->initialized;
}

// Get embedding dimension
static int zeta_embed_dim() {
    if (!g_embed_ctx) return 0;
    return g_embed_ctx->embed_dim;
}

#endif // ZETA_EMBED_INTEGRATION_H
