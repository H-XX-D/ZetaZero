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
#include <mutex>

// Embedding model context
struct zeta_embed_ctx {
    llama_model* model;
    llama_context* ctx;
    int embed_dim;        // Output dimension (384 for bge-small, 768 for nomic)
    bool initialized;
};

// Global embedding context (singleton for now)
static zeta_embed_ctx* g_embed_ctx = nullptr;

// Mutex for thread-safe embedding access
// The embedding model context is NOT thread-safe - all calls must be serialized
static std::mutex g_embed_mutex;

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
    cparams.n_threads = 20;        // Use 20 of 24 Xeon cores (leave 4 for system/GPU driver)
    cparams.n_threads_batch = 20;  // Parallel batch processing
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

// Internal: Embed a single chunk of text (must already hold mutex)
// Returns embedding dimension or -1 on error
static int zeta_embed_chunk_internal(const char* text, size_t text_len, float* output, int max_dim) {
    const llama_vocab* vocab = llama_model_get_vocab(g_embed_ctx->model);
    if (!vocab) {
        fprintf(stderr, "[EMBED] ERROR: No vocab in embedding model\n");
        return -1;
    }

    int max_tokens = 512;
    std::vector<llama_token> tokens(max_tokens);

    // Try tokenization without special tokens first
    int n_tokens = llama_tokenize(vocab, text, text_len,
                                   tokens.data(), max_tokens,
                                   false, false);

    // If that failed, try with special tokens
    if (n_tokens <= 0) {
        n_tokens = llama_tokenize(vocab, text, text_len,
                                   tokens.data(), max_tokens,
                                   true, true);
    }

    if (n_tokens <= 0) {
        fprintf(stderr, "[EMBED] ERROR: Chunk tokenization failed (len=%zu)\n", text_len);
        return -1;
    }

    tokens.resize(n_tokens);

    // Ensure we don't exceed context size
    int ctx_size = llama_n_ctx(g_embed_ctx->ctx);
    if (n_tokens > ctx_size) {
        n_tokens = ctx_size;
        tokens.resize(n_tokens);
    }

    // Clear memory/KV cache
    llama_memory_clear(llama_get_memory(g_embed_ctx->ctx), true);

    // Create batch
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);

    for (int i = 0; i < n_tokens; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = 1;
    }
    batch.n_tokens = n_tokens;

    // Run inference
    int decode_result = llama_decode(g_embed_ctx->ctx, batch);
    if (decode_result != 0) {
        fprintf(stderr, "[EMBED] ERROR: Chunk decode failed with code %d\n", decode_result);
        llama_batch_free(batch);
        return -1;
    }

    // Get embeddings
    const float* embeddings = llama_get_embeddings_seq(g_embed_ctx->ctx, 0);
    if (!embeddings) {
        embeddings = llama_get_embeddings_ith(g_embed_ctx->ctx, n_tokens - 1);
    }

    if (!embeddings) {
        fprintf(stderr, "[EMBED] ERROR: No embeddings from chunk\n");
        llama_batch_free(batch);
        return -1;
    }

    // Copy to output
    int copy_dim = (g_embed_ctx->embed_dim < max_dim) ? g_embed_ctx->embed_dim : max_dim;
    memcpy(output, embeddings, copy_dim * sizeof(float));

    llama_batch_free(batch);
    return copy_dim;
}

// Get embedding for text using the embedding model
// Returns the embedding dimension, or -1 on error
// output must have space for at least embed_dim floats
// THREAD SAFE: Uses mutex to serialize all embedding operations
// CHUNKING: Long text is split into overlapping chunks and averaged
static int zeta_embed_text(const char* text, float* output, int max_dim) {
    // Lock mutex for thread safety - embedding model is NOT thread-safe
    std::lock_guard<std::mutex> lock(g_embed_mutex);

    if (!g_embed_ctx || !g_embed_ctx->initialized) {
        return -1;
    }

    if (!text || !output) {
        return -1;
    }

    // Skip empty text - return zero embedding instead of crashing
    size_t text_len = strlen(text);
    if (text_len == 0) {
        fprintf(stderr, "[EMBED] WARNING: Empty text, returning zero embedding\n");
        memset(output, 0, g_embed_ctx->embed_dim * sizeof(float));
        return g_embed_ctx->embed_dim;
    }

    // Chunking parameters: ~4 chars per token, 512 token context
    // Use 1500 chars per chunk with 300 char overlap for safety
    const size_t CHUNK_SIZE = 1500;
    const size_t CHUNK_OVERLAP = 300;

    int copy_dim = (g_embed_ctx->embed_dim < max_dim) ? g_embed_ctx->embed_dim : max_dim;

    // Short text: embed directly without chunking
    if (text_len <= CHUNK_SIZE) {
        int result = zeta_embed_chunk_internal(text, text_len, output, max_dim);
        if (result < 0) {
            // Fallback to zero embedding on error
            memset(output, 0, copy_dim * sizeof(float));
            return copy_dim;
        }

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
        return copy_dim;
    }

    // Long text: sliding window chunking with overlap
    fprintf(stderr, "[EMBED] Chunking long text (len=%zu) into overlapping windows\n", text_len);

    // Accumulator for averaging chunk embeddings
    std::vector<float> accum(copy_dim, 0.0f);
    std::vector<float> chunk_embed(copy_dim);
    int num_chunks = 0;

    size_t pos = 0;
    while (pos < text_len) {
        // Calculate chunk boundaries
        size_t chunk_end = pos + CHUNK_SIZE;
        if (chunk_end > text_len) {
            chunk_end = text_len;
        }

        // Try to break at word boundary (look for space near end)
        if (chunk_end < text_len) {
            size_t search_start = chunk_end > 50 ? chunk_end - 50 : pos;
            for (size_t i = chunk_end; i > search_start; i--) {
                if (text[i] == ' ' || text[i] == '\n') {
                    chunk_end = i;
                    break;
                }
            }
        }

        size_t chunk_len = chunk_end - pos;

        // Embed this chunk
        int result = zeta_embed_chunk_internal(text + pos, chunk_len, chunk_embed.data(), max_dim);

        if (result > 0) {
            // Add to accumulator
            for (int i = 0; i < copy_dim; i++) {
                accum[i] += chunk_embed[i];
            }
            num_chunks++;
        }

        // Move to next chunk with overlap
        if (chunk_end >= text_len) {
            break;
        }
        pos = chunk_end > CHUNK_OVERLAP ? chunk_end - CHUNK_OVERLAP : 0;
        if (pos <= (chunk_end - CHUNK_SIZE)) {
            // Prevent infinite loop
            pos = chunk_end;
        }
    }

    if (num_chunks == 0) {
        fprintf(stderr, "[EMBED] ERROR: All chunks failed, returning zero embedding\n");
        memset(output, 0, copy_dim * sizeof(float));
        return copy_dim;
    }

    fprintf(stderr, "[EMBED] Averaged %d chunks for long text\n", num_chunks);

    // Average and normalize
    for (int i = 0; i < copy_dim; i++) {
        output[i] = accum[i] / num_chunks;
    }

    // L2 normalize the final embedding
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

// Wire up 4B embedding model with dual-process layer
static void zeta_embed_wire() {
    if (g_embed_ctx && g_embed_ctx->initialized) {
        zeta_set_embed_fn(zeta_embed_text);
        fprintf(stderr, "[EMBED] Wired to dual-process layer (dim=%d)\n", g_embed_ctx->embed_dim);
    }
}

#endif // ZETA_EMBED_INTEGRATION_H
