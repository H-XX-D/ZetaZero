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
#include <map>
#include <string>

// ============================================================================
// DREAM SUGGESTION (085129): Embedding Cache for 4B Model
// ============================================================================
// Caches embedding results to avoid redundant computation for repeated queries

struct EmbeddingCacheEntry {
    std::vector<float> embedding;
    time_t timestamp;
    int hits;
};

class EmbeddingCache {
public:
    // Configuration
    int max_entries = 500;           // Max cached embeddings
    int ttl_seconds = 600;           // 10 minute TTL
    int min_text_len = 10;           // Don't cache very short texts

private:
    std::map<std::string, EmbeddingCacheEntry> cache_;
    mutable std::mutex cache_mutex_;
    int cache_hits_ = 0;
    int cache_misses_ = 0;

    // Simple hash for cache key (first 100 chars lowercase + length)
    std::string make_key(const char* text) {
        std::string key;
        size_t len = strlen(text);
        size_t copy_len = (len < 100) ? len : 100;
        for (size_t i = 0; i < copy_len; i++) {
            key += tolower(text[i]);
        }
        key += "_" + std::to_string(len);
        return key;
    }

    void prune_expired() {
        time_t now = time(NULL);
        auto it = cache_.begin();
        while (it != cache_.end()) {
            if (now - it->second.timestamp > ttl_seconds) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void prune_lru() {
        if (cache_.size() <= (size_t)max_entries) return;

        // Find entry with lowest hits and oldest timestamp
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.hits < oldest->second.hits ||
                (it->second.hits == oldest->second.hits &&
                 it->second.timestamp < oldest->second.timestamp)) {
                oldest = it;
            }
        }
        cache_.erase(oldest);
    }

public:
    // Check cache for existing embedding
    bool get(const char* text, float* output, int dim) {
        if (!text || strlen(text) < (size_t)min_text_len) return false;

        std::lock_guard<std::mutex> lock(cache_mutex_);

        std::string key = make_key(text);
        auto it = cache_.find(key);

        if (it != cache_.end()) {
            // Check TTL
            time_t now = time(NULL);
            if (now - it->second.timestamp > ttl_seconds) {
                cache_.erase(it);
                cache_misses_++;
                return false;
            }

            // Cache hit
            int copy_dim = (dim < (int)it->second.embedding.size())
                           ? dim : (int)it->second.embedding.size();
            memcpy(output, it->second.embedding.data(), copy_dim * sizeof(float));
            it->second.hits++;
            cache_hits_++;

            fprintf(stderr, "[EMBED-CACHE] HIT: %.30s... (hits=%d)\n",
                    text, it->second.hits);
            return true;
        }

        cache_misses_++;
        return false;
    }

    // Add embedding to cache
    void put(const char* text, const float* embedding, int dim) {
        if (!text || !embedding || strlen(text) < (size_t)min_text_len) return;

        std::lock_guard<std::mutex> lock(cache_mutex_);

        // Prune if needed
        prune_expired();
        prune_lru();

        std::string key = make_key(text);

        EmbeddingCacheEntry entry;
        entry.embedding.resize(dim);
        memcpy(entry.embedding.data(), embedding, dim * sizeof(float));
        entry.timestamp = time(NULL);
        entry.hits = 0;

        cache_[key] = std::move(entry);

        fprintf(stderr, "[EMBED-CACHE] PUT: %.30s... (cache_size=%zu)\n",
                text, cache_.size());
    }

    // Get cache statistics
    std::string get_stats() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        char buf[512];
        float hit_rate = (cache_hits_ + cache_misses_ > 0)
                         ? (float)cache_hits_ / (cache_hits_ + cache_misses_) * 100.0f
                         : 0.0f;

        snprintf(buf, sizeof(buf),
                 "=== Embedding Cache Stats ===\n"
                 "Entries: %zu/%d\n"
                 "Hits: %d\n"
                 "Misses: %d\n"
                 "Hit Rate: %.1f%%\n"
                 "TTL: %d seconds\n",
                 cache_.size(), max_entries,
                 cache_hits_, cache_misses_, hit_rate, ttl_seconds);

        return std::string(buf);
    }

    // Clear cache
    void clear() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_.clear();
        cache_hits_ = 0;
        cache_misses_ = 0;
        fprintf(stderr, "[EMBED-CACHE] Cleared\n");
    }

    // Get hit rate for monitoring
    float get_hit_rate() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (cache_hits_ + cache_misses_ == 0) return 0.0f;
        return (float)cache_hits_ / (cache_hits_ + cache_misses_);
    }
};

// Global embedding cache
static EmbeddingCache g_embed_cache;

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
// DREAM SUGGESTION: Uses embedding cache to avoid redundant computation
static int zeta_embed_text(const char* text, float* output, int max_dim) {
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

    // DREAM SUGGESTION (085129): Check embedding cache first
    if (g_embed_cache.get(text, output, max_dim)) {
        return g_embed_ctx->embed_dim;  // Cache hit - return immediately
    }

    // Lock mutex for thread safety - embedding model is NOT thread-safe
    std::lock_guard<std::mutex> lock(g_embed_mutex);

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

        // DREAM SUGGESTION (085129): Cache the computed embedding
        g_embed_cache.put(text, output, copy_dim);

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

    // DREAM SUGGESTION (085129): Cache the computed embedding
    g_embed_cache.put(text, output, copy_dim);

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
