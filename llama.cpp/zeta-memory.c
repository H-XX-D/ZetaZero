// Z.E.T.A. Memory Manager Implementation
// Async prefetch + mmap tiered storage for extended context
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-memory.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// ============================================================================
// Block File Format (for cross-session persistence)
// ============================================================================

// File header stored at beginning of each .zeta file
typedef struct {
    uint32_t magic;          // 'ZETA' = 0x4154455A
    uint32_t version;        // Format version (1)
    int64_t  block_id;       // Block identifier
    int64_t  token_start;    // Starting token position
    int64_t  token_count;    // Number of tokens
    int32_t  summary_dim;    // Summary vector dimension
    int32_t  reserved;       // Padding for alignment
} zeta_block_header_t;

#define ZETA_MAGIC 0x4154455A  // 'ZETA' in little-endian
#define ZETA_VERSION 1

// Forward declarations
static int zeta_load_block_from_file(zeta_memory_ctx_t* ctx, const char* filepath);
int zeta_load_existing_blocks(zeta_memory_ctx_t* ctx);

// ============================================================================
// Internal Helpers
// ============================================================================

static float dot_product(const float* a, const float* b, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

static float vector_norm(const float* v, int n) {
    return sqrtf(dot_product(v, v, n));
}

static void vector_copy(float* dst, const float* src, int n) {
    memcpy(dst, src, n * sizeof(float));
}

static void vector_sub(float* out, const float* a, const float* b, int n) {
    for (int i = 0; i < n; i++) {
        out[i] = a[i] - b[i];
    }
}

static void vector_add_scaled(float* out, const float* a, const float* b, float scale, int n) {
    for (int i = 0; i < n; i++) {
        out[i] = a[i] + scale * b[i];
    }
}

// ============================================================================
// Initialization / Cleanup
// ============================================================================

zeta_memory_ctx_t* zeta_memory_init(
    const char* storage_dir,
    int summary_dim,
    float temporal_lambda,
    float retrieve_threshold,
    float tunneling_threshold,
    float momentum_gamma
) {
    zeta_memory_ctx_t* ctx = (zeta_memory_ctx_t*)calloc(1, sizeof(zeta_memory_ctx_t));
    if (!ctx) return NULL;

    ctx->retrieval_scored_scratch = NULL;
    ctx->retrieval_scored_capacity = 0;

    // Allocate query buffers
    ctx->query_prev = (float*)calloc(summary_dim, sizeof(float));
    ctx->query_curr = (float*)calloc(summary_dim, sizeof(float));
    if (!ctx->query_prev || !ctx->query_curr) {
        zeta_memory_free(ctx);
        return NULL;
    }

    // Set configuration
    ctx->summary_dim = summary_dim;
    ctx->temporal_lambda = temporal_lambda;
    ctx->retrieve_threshold = retrieve_threshold;
    ctx->tunneling_threshold = tunneling_threshold;
    ctx->momentum_gamma = momentum_gamma;
    ctx->num_blocks = 0;
    ctx->num_active = 0;
    ctx->next_block_id = 0;  // Will be updated by zeta_load_existing_blocks

    // Copy storage directory
    strncpy(ctx->storage_dir, storage_dir, sizeof(ctx->storage_dir) - 1);

    // Create storage directory if needed
    mkdir(storage_dir, 0755);

    // Load any existing blocks from previous sessions (optional)
    // This updates next_block_id to be > max loaded block ID
    const char * no_load_existing = getenv("ZETA_NO_LOAD_EXISTING");
    if (!(no_load_existing && no_load_existing[0] != '\0' && strcmp(no_load_existing, "0") != 0)) {
        zeta_load_existing_blocks(ctx);
    }

    return ctx;
}

void zeta_memory_free(zeta_memory_ctx_t* ctx) {
    if (!ctx) return;

    // Unmap all blocks
    for (int i = 0; i < ctx->num_blocks; i++) {
        zeta_memory_block_t* block = &ctx->blocks[i];
        if (block->mmap_base) {
            munmap(block->mmap_base, block->mmap_total_size);
        }
        free(block->summary);
        free(block->disk_path);
    }

    free(ctx->query_prev);
    free(ctx->query_curr);
    free(ctx->retrieval_scored_scratch);
    free(ctx);
}

// ============================================================================
// Block Management (Sublimation)
// ============================================================================

void zeta_compute_summary(
    const float* keys,
    int token_count,
    int dim,
    float* summary_out
) {
    // Mean pooling: s = (1/L) * sum(k_i)
    memset(summary_out, 0, dim * sizeof(float));

    for (int t = 0; t < token_count; t++) {
        for (int d = 0; d < dim; d++) {
            summary_out[d] += keys[t * dim + d];
        }
    }

    float scale = 1.0f / token_count;
    for (int d = 0; d < dim; d++) {
        summary_out[d] *= scale;
    }
}

int64_t zeta_sublimate_block(
    zeta_memory_ctx_t* ctx,
    const float* keys,
    const float* values,
    int token_count,
    int64_t token_start
) {
    return zeta_sublimate_block_ext(ctx, keys, values, NULL, token_count, token_start);
}

int64_t zeta_sublimate_block_ext(
    zeta_memory_ctx_t* ctx,
    const float* keys,
    const float* values,
    const float* summary_override,
    int token_count,
    int64_t token_start
) {
    if (ctx->num_blocks >= ZETA_MAX_MEMORY_BLOCKS) {
        fprintf(stderr, "zeta: max blocks reached (%d)\n", ZETA_MAX_MEMORY_BLOCKS);
        return -1;
    }

    int idx = ctx->num_blocks;
    zeta_memory_block_t* block = &ctx->blocks[idx];

    // Generate unique block ID (survives across sessions)
    block->block_id = ctx->next_block_id++;
    block->token_start = token_start;
    block->token_count = token_count;
    block->last_access = 0;
    block->zeta_potential = 1.0f;

    // Initialize graph structure
    block->block_type = ZETA_BLOCK_RAW;
    for (int i = 0; i < ZETA_MAX_LINKS; i++) {
        block->links[i] = -1;
        block->link_weights[i] = 0.0f;
    }
    block->temporal_prev = (idx > 0) ? ctx->blocks[idx - 1].block_id : -1;

    // Allocate summary vector
    block->summary = (float*)malloc(ctx->summary_dim * sizeof(float));
    if (!block->summary) return -1;

    // Use provided summary or compute from values
    if (summary_override) {
        memcpy(block->summary, summary_override, ctx->summary_dim * sizeof(float));
    } else {
        // Fallback: compute from values (less accurate than embeddings)
        zeta_compute_summary(values, token_count, ctx->summary_dim, block->summary);
    }

    // Cache summary norm for faster cosine similarity
    block->summary_norm = vector_norm(block->summary, ctx->summary_dim);

    // Create disk path
    block->disk_path = (char*)malloc(600);
    snprintf(block->disk_path, 600, "%s/block_%lld.zeta", ctx->storage_dir, (long long)block->block_id);

    // Write block to disk with header
    // Format: [header] [summary: dim] [keys: token_count * dim] [values: token_count * dim]
    size_t header_size = sizeof(zeta_block_header_t);
    size_t summary_size = ctx->summary_dim * sizeof(float);
    size_t kv_size = 2 * token_count * ctx->summary_dim * sizeof(float);
    size_t total_size = header_size + summary_size + kv_size;

    int fd = open(block->disk_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "zeta: failed to create %s: %s\n", block->disk_path, strerror(errno));
        return -1;
    }

    // Write header
    zeta_block_header_t header = {
        .magic = ZETA_MAGIC,
        .version = ZETA_VERSION,
        .block_id = block->block_id,
        .token_start = token_start,
        .token_count = token_count,
        .summary_dim = ctx->summary_dim,
        .reserved = 0
    };
    if (write(fd, &header, header_size) < 0) {
        close(fd);
        return -1;
    }

    // Write summary vector (for loading without recomputation)
    if (write(fd, block->summary, summary_size) < 0) {
        close(fd);
        return -1;
    }

    // Write keys
    if (write(fd, keys, token_count * ctx->summary_dim * sizeof(float)) < 0) {
        close(fd);
        return -1;
    }

    // Write values
    if (write(fd, values, token_count * ctx->summary_dim * sizeof(float)) < 0) {
        close(fd);
        return -1;
    }

    close(fd);

    // mmap the entire file
    fd = open(block->disk_path, O_RDONLY);
    if (fd >= 0) {
        block->mmap_total_size = total_size;
        block->mmap_kv_size = kv_size;
        block->mmap_base = mmap(NULL, total_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        if (block->mmap_base == MAP_FAILED) {
            block->mmap_base = NULL;
            block->mmap_kv = NULL;
        } else {
            // Set KV pointer to skip header + summary
            block->mmap_kv = (char*)block->mmap_base + header_size + summary_size;
        }
    }

    block->is_warm = false;
    block->is_active = false;

    ctx->num_blocks++;
    return block->block_id;
}

// ============================================================================
// Cross-Session Persistence
// ============================================================================

// Load a single block from disk file
static int zeta_load_block_from_file(
    zeta_memory_ctx_t* ctx,
    const char* filepath
) {
    if (ctx->num_blocks >= ZETA_MAX_MEMORY_BLOCKS) {
        return -1;
    }

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) return -1;

    // Read and validate header
    zeta_block_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }

    if (header.magic != ZETA_MAGIC || header.version != ZETA_VERSION) {
        close(fd);
        fprintf(stderr, "zeta: invalid block file %s (bad magic/version)\n", filepath);
        return -1;
    }

    // Check dimension compatibility
    if (header.summary_dim != ctx->summary_dim) {
        close(fd);
        fprintf(stderr, "zeta: dimension mismatch in %s (got %d, expected %d)\n",
                filepath, header.summary_dim, ctx->summary_dim);
        return -1;
    }

    // Read summary vector
    float* summary = (float*)malloc(ctx->summary_dim * sizeof(float));
    if (!summary || read(fd, summary, ctx->summary_dim * sizeof(float)) != (ssize_t)(ctx->summary_dim * sizeof(float))) {
        free(summary);
        close(fd);
        return -1;
    }

    close(fd);

    // Calculate sizes
    size_t header_size = sizeof(zeta_block_header_t);
    size_t summary_size = ctx->summary_dim * sizeof(float);
    size_t kv_size = 2 * header.token_count * ctx->summary_dim * sizeof(float);
    size_t total_size = header_size + summary_size + kv_size;

    // Create block entry
    int idx = ctx->num_blocks;
    zeta_memory_block_t* block = &ctx->blocks[idx];

    block->block_id = header.block_id;
    block->token_start = header.token_start;
    block->token_count = header.token_count;
    block->last_access = 0;
    block->zeta_potential = 1.0f;
    block->summary = summary;
    block->summary_norm = vector_norm(summary, ctx->summary_dim);

    // Initialize graph structure (not persisted in v1 format)
    block->block_type = ZETA_BLOCK_RAW;
    for (int i = 0; i < ZETA_MAX_LINKS; i++) {
        block->links[i] = -1;
        block->link_weights[i] = 0.0f;
    }
    block->temporal_prev = -1;

    // Copy path
    block->disk_path = strdup(filepath);

    // mmap the file
    fd = open(filepath, O_RDONLY);
    if (fd >= 0) {
        block->mmap_total_size = total_size;
        block->mmap_kv_size = kv_size;
        block->mmap_base = mmap(NULL, total_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        if (block->mmap_base == MAP_FAILED) {
            block->mmap_base = NULL;
            block->mmap_kv = NULL;
        } else {
            block->mmap_kv = (char*)block->mmap_base + header_size + summary_size;
        }
    }

    block->is_warm = false;
    block->is_active = false;

    ctx->num_blocks++;

    #ifdef ZETA_DEBUG
    fprintf(stderr, "zeta: loaded block %lld from %s (%lld tokens)\n",
            (long long)block->block_id, filepath, (long long)block->token_count);
    #endif

    return idx;
}

// Scan storage directory and load all existing .zeta files
int zeta_load_existing_blocks(zeta_memory_ctx_t* ctx) {
    DIR* dir = opendir(ctx->storage_dir);
    if (!dir) {
        // Directory doesn't exist or can't be opened - not an error for first run
        return 0;
    }

    int loaded = 0;
    int64_t max_block_id = -1;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        // Check for .zeta extension
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len < 6 || strcmp(name + len - 5, ".zeta") != 0) {
            continue;
        }

        // Build full path
        char filepath[700];
        snprintf(filepath, sizeof(filepath), "%s/%s", ctx->storage_dir, name);

        // Load block
        int idx = zeta_load_block_from_file(ctx, filepath);
        if (idx >= 0) {
            loaded++;
            if (ctx->blocks[idx].block_id > max_block_id) {
                max_block_id = ctx->blocks[idx].block_id;
            }
        }
    }

    closedir(dir);

    // Set next_block_id to ensure new blocks don't collide with loaded ones
    if (max_block_id >= ctx->next_block_id) {
        ctx->next_block_id = max_block_id + 1;
    }

    if (loaded > 0) {
        fprintf(stderr, "zeta: loaded %d existing memory blocks from %s (next_id=%lld)\n",
                loaded, ctx->storage_dir, (long long)ctx->next_block_id);
    }

    return loaded;
}

// ============================================================================
// Retrieval (Entanglement)
// ============================================================================

float zeta_entanglement_score(
    const float* query,
    const float* summary,
    int dim
) {
    // Sharpened cosine similarity: ReLU(cos(q, s))^3
    float dot = dot_product(query, summary, dim);
    float norm_q = vector_norm(query, dim);

    float norm_s = vector_norm(summary, dim);

    if (norm_q < 1e-8f || norm_s < 1e-8f) return 0.0f;

    float cos_sim = dot / (norm_q * norm_s);

    // ReLU: clamp negatives to zero
    if (cos_sim < 0.0f) cos_sim = 0.0f;

    // Cubic sharpening
    return cos_sim * cos_sim * cos_sim;
}

int zeta_find_relevant_blocks(
    zeta_memory_ctx_t* ctx,
    const float* query,
    int* indices_out,
    float* scores_out,
    int max_results
) {
    // Compute scores for all blocks
    typedef struct { int idx; float score; } scored_block_t;
    if (ctx->num_blocks <= 0) {
        return 0;
    }

    if (!ctx->retrieval_scored_scratch || ctx->retrieval_scored_capacity < ctx->num_blocks) {
        int new_cap = ctx->retrieval_scored_capacity > 0 ? ctx->retrieval_scored_capacity : 16;
        while (new_cap < ctx->num_blocks) {
            new_cap *= 2;
        }

        void * new_buf = realloc(ctx->retrieval_scored_scratch, (size_t)new_cap * sizeof(scored_block_t));
        if (!new_buf) {
            return 0;
        }
        ctx->retrieval_scored_scratch = new_buf;
        ctx->retrieval_scored_capacity = new_cap;
    }

    scored_block_t* scored = (scored_block_t*)ctx->retrieval_scored_scratch;

    // Compute query norm once
    const float q_norm = vector_norm(query, ctx->summary_dim);
    if (q_norm < 1e-8f) {
        return 0;
    }

    int num_above_threshold = 0;
    float max_score_seen = 0.0f;
    for (int i = 0; i < ctx->num_blocks; i++) {
        const float s_norm = ctx->blocks[i].summary_norm;
        if (s_norm < 1e-8f) {
            continue;
        }

        // Cosine similarity
        float dot = dot_product(query, ctx->blocks[i].summary, ctx->summary_dim);
        float cos_sim = dot / (q_norm * s_norm);

        // ReLU clamp
        if (cos_sim < 0.0f) cos_sim = 0.0f;

        // Cubic sharpening
        float raw_score = cos_sim * cos_sim * cos_sim;

        // Apply temporal decay to score
        float score = raw_score * ctx->blocks[i].zeta_potential;

        if (score > max_score_seen) max_score_seen = score;

        if (score >= ctx->retrieve_threshold) {
            scored[num_above_threshold].idx = i;
            scored[num_above_threshold].score = score;
            num_above_threshold++;
        }
    }

    // Debug: print when blocks are found
    if (num_above_threshold > 0) {
#ifdef ZETA_DEBUG
        fprintf(stderr, "zeta: retrieved %d blocks (max_score=%.3f, threshold=%.2f)\n",
                num_above_threshold, max_score_seen, ctx->retrieve_threshold);
#endif
    }

    // Sort by score (simple bubble sort, num_above_threshold is small)
    for (int i = 0; i < num_above_threshold - 1; i++) {
        for (int j = i + 1; j < num_above_threshold; j++) {
            if (scored[j].score > scored[i].score) {
                scored_block_t tmp = scored[i];
                scored[i] = scored[j];
                scored[j] = tmp;
            }
        }
    }

    // Return top results
    int num_results = (num_above_threshold < max_results) ? num_above_threshold : max_results;
    for (int i = 0; i < num_results; i++) {
        indices_out[i] = scored[i].idx;
        scores_out[i] = scored[i].score;
    }

    ctx->total_retrievals++;
    return num_results;
}

// ============================================================================
// Prefetching (Momentum Prediction)
// ============================================================================

void zeta_update_query_and_prefetch(
    zeta_memory_ctx_t* ctx,
    const float* query_current
) {
    // Shift: prev = curr, curr = new
    vector_copy(ctx->query_prev, ctx->query_curr, ctx->summary_dim);
    vector_copy(ctx->query_curr, query_current, ctx->summary_dim);

    // Compute prediction and prefetch
    float* prediction = (float*)malloc(ctx->summary_dim * sizeof(float));
    zeta_compute_prediction_vector(ctx, prediction);
    zeta_prefetch_predicted_blocks(ctx, prediction);
    free(prediction);
}

void zeta_compute_prediction_vector(
    zeta_memory_ctx_t* ctx,
    float* prediction_out
) {
    // p = q_curr + gamma * (q_curr - q_prev)
    float* delta = (float*)malloc(ctx->summary_dim * sizeof(float));
    vector_sub(delta, ctx->query_curr, ctx->query_prev, ctx->summary_dim);
    vector_add_scaled(prediction_out, ctx->query_curr, delta, ctx->momentum_gamma, ctx->summary_dim);
    free(delta);
}

void zeta_prefetch_predicted_blocks(
    zeta_memory_ctx_t* ctx,
    const float* prediction_vector
) {
    // Find blocks that prediction vector is similar to
    for (int i = 0; i < ctx->num_blocks; i++) {
        zeta_memory_block_t* block = &ctx->blocks[i];

        // Skip already warm blocks
        if (block->is_warm) continue;

        float score = zeta_entanglement_score(prediction_vector, block->summary, ctx->summary_dim);
        score *= block->zeta_potential;

        // Prefetch if likely to be needed (lower threshold than retrieval)
        if (score >= ctx->retrieve_threshold * 0.7f) {
            if (block->mmap_base) {
                // Advise kernel to page in this region
                madvise(block->mmap_base, block->mmap_total_size, MADV_WILLNEED);
                block->is_warm = true;

                #ifdef ZETA_DEBUG
                fprintf(stderr, "zeta: prefetching block %lld (score %.3f)\n",
                        (long long)block->block_id, score);
                #endif
            }
        }
    }
}

// ============================================================================
// Block Loading (Superposition Preparation)
// ============================================================================

float* zeta_load_block(
    zeta_memory_ctx_t* ctx,
    int block_index
) {
    if (block_index < 0 || block_index >= ctx->num_blocks) return NULL;

    zeta_memory_block_t* block = &ctx->blocks[block_index];

    // Check if already active
    if (block->is_active) {
        ctx->cache_hits++;
        return (float*)block->mmap_kv;
    }

    // Check if we need to evict an active block
    if (ctx->num_active >= ZETA_MAX_ACTIVE_BLOCKS) {
        // Evict oldest active block (simple LRU)
        int oldest_idx = ctx->active_indices[0];
        zeta_unload_block(ctx, oldest_idx);
    }

    // Ensure mmap'd
    if (!block->mmap_base) {
        int fd = open(block->disk_path, O_RDONLY);
        if (fd < 0) return NULL;

        block->mmap_base = mmap(NULL, block->mmap_total_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        if (block->mmap_base == MAP_FAILED) {
            block->mmap_base = NULL;
            block->mmap_kv = NULL;
            return NULL;
        }

        // Set KV pointer (skip header + summary)
        size_t header_size = sizeof(zeta_block_header_t);
        size_t summary_size = ctx->summary_dim * sizeof(float);
        block->mmap_kv = (char*)block->mmap_base + header_size + summary_size;
    }

    // Page in if not warm
    if (!block->is_warm) {
        madvise(block->mmap_base, block->mmap_total_size, MADV_WILLNEED);
        block->is_warm = true;
    } else {
        ctx->prefetch_hits++;
    }

    // Mark as active
    block->is_active = true;
    ctx->active_indices[ctx->num_active++] = block_index;

    return (float*)block->mmap_kv;
}

void zeta_unload_block(
    zeta_memory_ctx_t* ctx,
    int block_index
) {
    if (block_index < 0 || block_index >= ctx->num_blocks) return;

    zeta_memory_block_t* block = &ctx->blocks[block_index];
    block->is_active = false;

    // Remove from active list
    for (int i = 0; i < ctx->num_active; i++) {
        if (ctx->active_indices[i] == block_index) {
            // Shift remaining
            for (int j = i; j < ctx->num_active - 1; j++) {
                ctx->active_indices[j] = ctx->active_indices[j + 1];
            }
            ctx->num_active--;
            break;
        }
    }

    // Advise kernel we don't need this anymore
    if (block->mmap_base) {
        madvise(block->mmap_base, block->mmap_total_size, MADV_DONTNEED);
        block->is_warm = false;
    }
}

// ============================================================================
// Temporal Decay
// ============================================================================

void zeta_apply_temporal_decay(
    zeta_memory_ctx_t* ctx,
    int64_t current_step
) {
    // w(t) = exp(-lambda * (t - t_access))
    for (int i = 0; i < ctx->num_blocks; i++) {
        zeta_memory_block_t* block = &ctx->blocks[i];
        int64_t age = current_step - block->last_access;
        block->zeta_potential = expf(-ctx->temporal_lambda * (float)age);
    }
}

void zeta_touch_block(
    zeta_memory_ctx_t* ctx,
    int block_index,
    int64_t current_step
) {
    if (block_index < 0 || block_index >= ctx->num_blocks) return;
    ctx->blocks[block_index].last_access = current_step;
    ctx->blocks[block_index].zeta_potential = 1.0f;
}

// ============================================================================
// Utility
// ============================================================================

void zeta_get_stats(
    const zeta_memory_ctx_t* ctx,
    int64_t* total_retrievals,
    int64_t* cache_hits,
    int64_t* prefetch_hits
) {
    if (total_retrievals) *total_retrievals = ctx->total_retrievals;
    if (cache_hits) *cache_hits = ctx->cache_hits;
    if (prefetch_hits) *prefetch_hits = ctx->prefetch_hits;
}

void zeta_debug_print_block(
    const zeta_memory_ctx_t* ctx,
    int block_index
) {
    if (block_index < 0 || block_index >= ctx->num_blocks) return;

    const zeta_memory_block_t* b = &ctx->blocks[block_index];
    fprintf(stderr,
        "Block %lld: tokens[%lld..%lld] potential=%.3f warm=%d active=%d path=%s\n",
        (long long)b->block_id,
        (long long)b->token_start,
        (long long)(b->token_start + b->token_count),
        b->zeta_potential,
        b->is_warm,
        b->is_active,
        b->disk_path
    );
}

// ============================================================================
// Graph Links (Multi-hop Support)
// ============================================================================

int zeta_find_block_by_id(
    const zeta_memory_ctx_t* ctx,
    int64_t block_id
) {
    for (int i = 0; i < ctx->num_blocks; i++) {
        if (ctx->blocks[i].block_id == block_id) {
            return i;
        }
    }
    return -1;
}

int zeta_add_link(
    zeta_memory_ctx_t* ctx,
    int64_t from_block_id,
    int64_t to_block_id,
    float weight
) {
    int from_idx = zeta_find_block_by_id(ctx, from_block_id);
    if (from_idx < 0) return -1;

    zeta_memory_block_t* block = &ctx->blocks[from_idx];

    // Find empty slot or existing link
    for (int i = 0; i < ZETA_MAX_LINKS; i++) {
        if (block->links[i] == to_block_id) {
            // Update existing link weight
            block->link_weights[i] = weight;
            return 0;
        }
        if (block->links[i] < 0) {
            // Use empty slot
            block->links[i] = to_block_id;
            block->link_weights[i] = weight;
            return 0;
        }
    }

    // No empty slots
    return -1;
}

void zeta_remove_link(
    zeta_memory_ctx_t* ctx,
    int64_t from_block_id,
    int64_t to_block_id
) {
    int from_idx = zeta_find_block_by_id(ctx, from_block_id);
    if (from_idx < 0) return;

    zeta_memory_block_t* block = &ctx->blocks[from_idx];

    for (int i = 0; i < ZETA_MAX_LINKS; i++) {
        if (block->links[i] == to_block_id) {
            block->links[i] = -1;
            block->link_weights[i] = 0.0f;
            return;
        }
    }
}

int zeta_retrieve_multihop(
    zeta_memory_ctx_t* ctx,
    const float* query,
    int max_hops,
    int* indices_out,
    float* scores_out,
    int max_results
) {
    // Hop 1: Direct semantic retrieval
    int found = zeta_find_relevant_blocks(ctx, query, indices_out, scores_out, max_results);

    if (max_hops <= 1 || found == 0 || found >= max_results) {
        return found;
    }

    // Track which blocks we've already included (by block_id)
    int64_t* seen_ids = (int64_t*)malloc(max_results * sizeof(int64_t));
    for (int i = 0; i < found; i++) {
        seen_ids[i] = ctx->blocks[indices_out[i]].block_id;
    }
    int num_seen = found;

    // Hop 2+: Follow graph links from retrieved blocks
    for (int hop = 1; hop < max_hops && found < max_results; hop++) {
        int prev_found = found;

        for (int i = 0; i < prev_found && found < max_results; i++) {
            zeta_memory_block_t* block = &ctx->blocks[indices_out[i]];

            // Check all links from this block
            for (int j = 0; j < ZETA_MAX_LINKS && found < max_results; j++) {
                if (block->links[j] < 0) continue;

                // Check if already in results
                bool already_seen = false;
                for (int k = 0; k < num_seen; k++) {
                    if (seen_ids[k] == block->links[j]) {
                        already_seen = true;
                        break;
                    }
                }
                if (already_seen) continue;

                // Find block index for linked block
                int linked_idx = zeta_find_block_by_id(ctx, block->links[j]);
                if (linked_idx < 0) continue;

                // Propagate score through link: parent_score * link_weight * decay_factor
                float hop_score = scores_out[i] * block->link_weights[j] * (1.0f / (hop + 1));

                // Only include if above threshold
                if (hop_score >= ctx->retrieve_threshold * 0.5f) {
                    indices_out[found] = linked_idx;
                    scores_out[found] = hop_score;
                    seen_ids[num_seen++] = block->links[j];
                    found++;
                }
            }

            // Also check temporal_prev link
            if (block->temporal_prev >= 0 && found < max_results) {
                bool already_seen = false;
                for (int k = 0; k < num_seen; k++) {
                    if (seen_ids[k] == block->temporal_prev) {
                        already_seen = true;
                        break;
                    }
                }

                if (!already_seen) {
                    int prev_idx = zeta_find_block_by_id(ctx, block->temporal_prev);
                    if (prev_idx >= 0) {
                        // Temporal links get lower weight
                        float hop_score = scores_out[i] * 0.5f * (1.0f / (hop + 1));
                        if (hop_score >= ctx->retrieve_threshold * 0.5f) {
                            indices_out[found] = prev_idx;
                            scores_out[found] = hop_score;
                            seen_ids[num_seen++] = block->temporal_prev;
                            found++;
                        }
                    }
                }
            }
        }
    }

    free(seen_ids);
    return found;
}
