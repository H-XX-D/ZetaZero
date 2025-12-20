// Z.E.T.A. Integration Layer Implementation
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-integration.h"
#include "zeta-kv-extract.h"
#include "zeta-model-bind.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// Embedded Constitution (fallback when no file provided)
// ============================================================================

static const char ZETA_EMBEDDED_CONSTITUTION[] =
    "Z.E.T.A. ETHICAL CONSTITUTION\n"
    "Version 1.0 | Cryptographically Bound\n\n"
    "This Constitution establishes the ethical framework under which Z.E.T.A.\n"
    "(Zero Entropy Temporal Assimilation) memory system operates.\n\n"
    "ARTICLE I: CORE PRINCIPLES\n"
    "1.1 Beneficence - Operate to benefit humanity.\n"
    "1.2 Non-Maleficence - Do not knowingly cause harm.\n"
    "1.3 Transparency - Be honest about nature and limitations.\n"
    "1.4 Privacy - Respect user privacy and confidentiality.\n\n"
    "ARTICLE II: PROHIBITED ACTIONS\n"
    "2.1 No weapons of mass destruction assistance.\n"
    "2.2 No exploitation of vulnerable populations.\n"
    "2.3 No disinformation at scale.\n"
    "2.4 No unauthorized system access.\n\n"
    "ARTICLE III: MEMORY ETHICS\n"
    "3.1 Consent through continued use.\n"
    "3.2 Accuracy with confidence levels.\n"
    "3.3 Context preservation.\n"
    "3.4 Natural decay and forgetting.\n\n"
    "Z.E.T.A.(TM) | Patent Pending | (C) 2025\n";

// ============================================================================
// Initialization
// ============================================================================

zeta_context_t* zeta_context_init(
    struct llama_context* llama_ctx,
    const char* storage_dir,
    const char* constitution_path,
    float temporal_lambda,
    float tunneling_threshold,
    float retrieve_threshold,
    float momentum_gamma
) {
    // =========================================================================
    // DEVELOPMENT MODE BYPASS
    // =========================================================================

    const char* dev_mode = getenv("ZETA_DEV_MODE");
    bool is_dev_mode = (dev_mode && dev_mode[0] != '\0' && strcmp(dev_mode, "0") != 0);

    if (is_dev_mode) {
        fprintf(stderr, "\n");
        fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║  ⚠️  Z.E.T.A. DEVELOPMENT MODE ACTIVE                         ║\n");
        fprintf(stderr, "║  Constitutional lock BYPASSED for development.               ║\n");
        fprintf(stderr, "║  Set ZETA_DEV_MODE=0 or unset to enable production mode.     ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");
    }

    // =========================================================================
    // CONSTITUTIONAL LOCK - Model will not function without valid constitution
    // (Bypassed in development mode)
    // =========================================================================

    zeta_constitution_t* constitution = NULL;

    if (is_dev_mode) {
        // Development mode: use embedded constitution, skip verification
        constitution = zeta_constitution_init_embedded(
            ZETA_EMBEDDED_CONSTITUTION,
            sizeof(ZETA_EMBEDDED_CONSTITUTION) - 1
        );
        if (constitution) {
            constitution->verified = true;  // Auto-verify in dev mode
        }
    } else if (constitution_path) {
        // Load from file
        constitution = zeta_constitution_init(constitution_path);
        if (!constitution) {
            fprintf(stderr, "[CRITICAL] Failed to load constitution from: %s\n", constitution_path);
            return NULL;
        }

        // Verify constitution hash (CRITICAL)
        if (zeta_constitution_prepare_model_load(constitution, ZETA_CONSTITUTION_HASH) != 0) {
            zeta_constitution_free(constitution);
            return NULL;  // FAIL - wrong constitution = model cannot function
        }
    } else {
        // Use embedded constitution (always passes - it's the reference)
        constitution = zeta_constitution_init_embedded(
            ZETA_EMBEDDED_CONSTITUTION,
            sizeof(ZETA_EMBEDDED_CONSTITUTION) - 1
        );
        if (constitution) {
            constitution->verified = true;
        }
    }

    if (!constitution) {
        fprintf(stderr, "[CRITICAL] Constitution initialization failed.\n");
        return NULL;
    }

    if (!is_dev_mode) {
        zeta_constitution_print_status(constitution);
    }

    // =========================================================================
    // Normal initialization (only reached if constitution is valid)
    // =========================================================================

    zeta_context_t* ctx = (zeta_context_t*)calloc(1, sizeof(zeta_context_t));
    if (!ctx) {
        zeta_constitution_free(constitution);
        return NULL;
    }

    ctx->constitution = constitution;
    ctx->llama = llama_ctx;
    ctx->temporal_lambda = temporal_lambda;
    ctx->tunneling_threshold = tunneling_threshold;
    ctx->retrieve_threshold = retrieve_threshold;
    ctx->momentum_gamma = momentum_gamma;

    // Get model dimension from llama context
    const struct llama_model* model = llama_get_model(llama_ctx);
    int n_embd = llama_model_n_embd(model);
    int n_vocab = llama_vocab_n_tokens(llama_model_get_vocab(model));

    // =========================================================================
    // Initialize Constitutional Weight Binding
    // This cryptographically binds model output to the constitution
    // (Relaxed in development mode)
    // =========================================================================

    ctx->binding = zeta_model_binding_init(
        constitution,       // Transfer ownership
        ZETA_CONSTITUTION_HASH,
        n_vocab,
        n_embd
    );

    if (!ctx->binding) {
        if (is_dev_mode) {
            fprintf(stderr, "[DEV] Constitutional binding skipped in development mode.\n");
            ctx->constitution = constitution;  // Keep constitution without binding
        } else {
            fprintf(stderr, "[CRITICAL] Failed to initialize constitutional binding.\n");
            fprintf(stderr, "[CRITICAL] Model cannot function without valid constitution.\n");
            free(ctx);
            return NULL;
        }
    } else {
        // Constitution ownership transferred to binding
        ctx->constitution = ctx->binding->constitution;
    }

    // Initialize memory manager
    ctx->memory = zeta_memory_init(
        storage_dir,
        n_embd,
        temporal_lambda,
        retrieve_threshold,
        tunneling_threshold,
        momentum_gamma
    );

    if (!ctx->memory) {
        zeta_constitution_free(constitution);
        free(ctx);
        return NULL;
    }

    // Allocate injection buffer
    ctx->injection_dim = n_embd;
    ctx->injection_buffer = (float*)calloc(n_embd, sizeof(float));
    ctx->has_injection = false;

    // Block summaries cache (filled on demand)
    ctx->block_summaries_cache = NULL;
    ctx->block_summaries_cache_dim = 0;
    ctx->block_summaries_cache_cap_blocks = 0;
    ctx->block_summaries_cache_filled_blocks = 0;

    // Default settings
    ctx->current_step = 0;
    ctx->sublimation_trigger = 1024;  // Sublimate when KV cache > 1024 tokens
    ctx->block_size = ZETA_BLOCK_SIZE;

    // Default sublimation policy (manual)
    ctx->sublimate_policy = ZETA_SUBLIMATE_MANUAL;
    ctx->sublimate_window_size = 512;
    ctx->sublimate_pressure_pct = 0.8f;
    ctx->attention_decay = 0.95f;

    // Attention tracking (allocate max KV size from model)
    int max_kv = llama_n_ctx(llama_ctx);
    ctx->attention_scores_size = max_kv;
    ctx->attention_scores = (float*)calloc(max_kv, sizeof(float));
    ctx->kv_cache_used = 0;

    // =========================================================================
    // Initialize Metal Kernels (for GPU-accelerated attention)
    // =========================================================================

#if ZETA_METAL_AVAILABLE && !defined(ZETA_DISABLE_METAL)
    // Metal GPU acceleration (can be disabled at runtime via env var)
    // - Set ZETA_DISABLE_METAL=1 to force CPU fallback.
    const char * disable_metal = getenv("ZETA_DISABLE_METAL");
    if (disable_metal && disable_metal[0] != '\0' && strcmp(disable_metal, "0") != 0) {
        ctx->metal = NULL;
        fprintf(stderr, "[ZETA] Metal disabled via ZETA_DISABLE_METAL, using CPU fallback.\n");
    } else {
        ctx->metal = zeta_metal_init();
        if (ctx->metal) {
            fprintf(stderr, "[ZETA] Metal GPU acceleration enabled.\n");
        } else {
            fprintf(stderr, "[ZETA] Metal init failed, using CPU fallback.\n");
        }
    }
#else
    ctx->metal = NULL;
    fprintf(stderr, "[ZETA] Compiled without Metal support.\n");
#endif

    fprintf(stderr, "[ZETA] Constitutional lock engaged. Model operational.\n");
    return ctx;
}

void zeta_context_free(zeta_context_t* ctx) {
    if (!ctx) return;

    // Free Metal context
#if ZETA_METAL_AVAILABLE
    if (ctx->metal) {
        zeta_metal_free(ctx->metal);
    }
#endif

    free(ctx->block_summaries_cache);

    // Binding owns the constitution, so free binding (which frees constitution)
    if (ctx->binding) {
        zeta_model_binding_free(ctx->binding);
        ctx->constitution = NULL;  // Already freed by binding
    } else if (ctx->constitution) {
        // Fallback if binding wasn't created
        zeta_constitution_free(ctx->constitution);
    }
    if (ctx->memory) {
        zeta_memory_free(ctx->memory);
    }
    free(ctx->injection_buffer);
    free(ctx->attention_scores);
    free(ctx);
}

// ============================================================================
// Core Operations
// ============================================================================

void zeta_pre_decode(
    zeta_context_t* ctx,
    const float* query_vector,
    int query_dim
) {
    (void)query_dim;  // Used implicitly via memory->summary_dim
    ctx->current_step++;

    // Update temporal decay for all blocks
    zeta_apply_temporal_decay(ctx->memory, ctx->current_step);

    // Update query state and trigger prefetch
    zeta_update_query_and_prefetch(ctx->memory, query_vector);

    // Find relevant memory blocks
    int indices[ZETA_MAX_ACTIVE_BLOCKS];
    float scores[ZETA_MAX_ACTIVE_BLOCKS];

    int num_found = zeta_find_relevant_blocks(
        ctx->memory,
        query_vector,
        indices,
        scores,
        ZETA_MAX_ACTIVE_BLOCKS
    );

    // Reset injection state
    ctx->has_injection = false;
    memset(ctx->injection_buffer, 0, ctx->injection_dim * sizeof(float));

    if (num_found == 0) return;

    // Load relevant blocks and prepare injection
    // Note: Actual attention computation happens in the Metal kernel
    // Here we just ensure blocks are loaded and compute weights
    for (int i = 0; i < num_found; i++) {
        int block_idx = indices[i];

        // Load block (pages in from disk if needed)
        float* kv_data = zeta_load_block(ctx->memory, block_idx);
        if (!kv_data) continue;

        // Mark as accessed (resets decay)
        zeta_touch_block(ctx->memory, block_idx, ctx->current_step);

        // The actual attention injection is done at kernel level
        // We flag that injection should happen
        ctx->has_injection = true;

        #ifdef ZETA_DEBUG
        fprintf(stderr, "zeta: retrieved block %d (score %.3f)\n", block_idx, scores[i]);
        #endif
    }
}

void zeta_post_attention(
    zeta_context_t* ctx,
    float* attention_output,
    int seq_len,
    int dim
) {
    if (!ctx || !attention_output) return;
    if (!ctx->has_injection) return;
    if (ctx->memory->num_blocks == 0) return;

    // Apply superposition injection to each position in the sequence
    // For each position: O_final = O_ctx + Σ(α_k · O_mem_k)
    //
    // Note: This is an approximation when called on full attention output.
    // The ideal integration would inject per-head during attention computation,
    // but that requires forking llama.cpp. This external approach provides
    // a reasonable approximation for memory-augmented attention.

    // Use the injection buffer if we have accumulated memory attention
    if (ctx->injection_buffer && ctx->injection_dim == dim) {
        // Inject for each sequence position
        for (int s = 0; s < seq_len; s++) {
            float* output_pos = attention_output + s * dim;

            // Simple injection: O = O + α · injection_buffer
            // The injection buffer contains accumulated memory attention
            float alpha = 0.3f;  // Base injection weight

            for (int d = 0; d < dim; d++) {
                output_pos[d] += alpha * ctx->injection_buffer[d];
            }
        }
    } else {
        // Fallback: use query-based injection for last position
        // This is called when injection_buffer hasn't been pre-computed
        if (seq_len > 0) {
            float* last_output = attention_output + (seq_len - 1) * dim;
            zeta_inject_superposition(ctx, last_output, last_output, dim);
        }
    }

    // Clear injection flag
    ctx->has_injection = false;
}

// Wire new blocks to nearby temporal/semantic neighbors so multi-hop retrieval has edges
static void zeta_link_new_block(zeta_context_t* ctx, int64_t new_block_id) {
    if (!ctx || !ctx->memory || new_block_id < 0) return;

    zeta_memory_ctx_t* mem = ctx->memory;
    int new_idx = zeta_find_block_by_id(mem, new_block_id);
    if (new_idx < 0 || new_idx >= mem->num_blocks) return;

    zeta_memory_block_t* new_block = &mem->blocks[new_idx];
    if (!new_block->summary || new_block->summary_norm < 1e-6f) return;

    // Temporal neighbor (immediate prior block in archive order)
    if (new_idx > 0) {
        int prev_idx = new_idx - 1;
        zeta_add_link(mem, new_block->block_id, mem->blocks[prev_idx].block_id, 0.8f);
        zeta_add_link(mem, mem->blocks[prev_idx].block_id, new_block->block_id, 0.8f);
    }

    // Semantic neighbors: link to the most recent few blocks with sufficient similarity
    int start = (mem->num_blocks > 8) ? mem->num_blocks - 8 : 0;
    for (int i = start; i < mem->num_blocks; i++) {
        if (i == new_idx) continue;
        zeta_memory_block_t* cand = &mem->blocks[i];
        if (!cand->summary || cand->summary_norm < 1e-6f) continue;

        float dot = 0.0f;
        for (int d = 0; d < mem->summary_dim; d++) {
            dot += new_block->summary[d] * cand->summary[d];
        }

        float cos_sim = dot / (new_block->summary_norm * cand->summary_norm);
        if (cos_sim <= 0.0f) continue;

        float score = cos_sim * cos_sim;  // Match entanglement scoring (quadratic)
        float threshold = mem->retrieve_threshold * 0.7f;
        if (score >= threshold) {
            float weight = fminf(1.0f, score);
            zeta_add_link(mem, new_block->block_id, cand->block_id, weight);
            zeta_add_link(mem, cand->block_id, new_block->block_id, weight);
        }
    }
}

void zeta_force_sublimation(
    zeta_context_t* ctx,
    const float* keys,
    const float* values,
    int token_count,
    int token_start
) {
    int64_t block_id = zeta_sublimate_block(
        ctx->memory,
        keys,
        values,
        token_count,
        token_start
    );

    if (block_id >= 0) {
        zeta_link_new_block(ctx, block_id);
        #ifdef ZETA_DEBUG
        fprintf(stderr, "zeta: sublimated block %lld (tokens %d-%d)\n",
                (long long)block_id, token_start, token_start + token_count);
        #endif
    }
}

// ============================================================================
// Query Helpers
// ============================================================================

void zeta_compute_mean_query(
    const float* query_heads,
    int n_heads,
    int head_dim,
    float* mean_out
) {
    int total_dim = n_heads * head_dim;
    memset(mean_out, 0, total_dim * sizeof(float));

    // Sum all heads
    for (int h = 0; h < n_heads; h++) {
        for (int d = 0; d < head_dim; d++) {
            mean_out[d] += query_heads[h * head_dim + d];
        }
    }

    // Average
    float scale = 1.0f / n_heads;
    for (int d = 0; d < head_dim; d++) {
        mean_out[d] *= scale;
    }
}

// ============================================================================
// Statistics
// ============================================================================

void zeta_get_statistics(
    const zeta_context_t* ctx,
    zeta_stats_t* stats_out
) {
    memset(stats_out, 0, sizeof(zeta_stats_t));

    stats_out->num_archived_blocks = ctx->memory->num_blocks;
    stats_out->num_active_blocks = ctx->memory->num_active;

    zeta_get_stats(
        ctx->memory,
        &stats_out->total_retrievals,
        &stats_out->cache_hits,
        &stats_out->prefetch_hits
    );

    // Compute average retrieval score from active blocks
    float sum = 0.0f;
    for (int i = 0; i < ctx->memory->num_active; i++) {
        int idx = ctx->memory->active_indices[i];
        sum += ctx->memory->blocks[idx].zeta_potential;
    }
    if (ctx->memory->num_active > 0) {
        stats_out->avg_retrieval_score = sum / ctx->memory->num_active;
    }
}

void zeta_print_statistics(const zeta_context_t* ctx) {
    zeta_stats_t stats;
    zeta_get_statistics(ctx, &stats);

    fprintf(stderr,
        "\n=== Z.E.T.A. Statistics ===\n"
        "Archived blocks:    %d\n"
        "Active blocks:      %d\n"
        "Total retrievals:   %lld\n"
        "Cache hits:         %lld (%.1f%%)\n"
        "Prefetch hits:      %lld (%.1f%%)\n"
        "Avg retrieval score: %.3f\n"
        "===========================\n",
        stats.num_archived_blocks,
        stats.num_active_blocks,
        (long long)stats.total_retrievals,
        (long long)stats.cache_hits,
        stats.total_retrievals > 0 ? 100.0 * stats.cache_hits / stats.total_retrievals : 0.0,
        (long long)stats.prefetch_hits,
        stats.total_retrievals > 0 ? 100.0 * stats.prefetch_hits / stats.total_retrievals : 0.0,
        stats.avg_retrieval_score
    );
}

// ============================================================================
// Superposition Injection
// ============================================================================

void zeta_compute_memory_attention(
    const float* query,
    const float* mem_keys,
    const float* mem_values,
    int token_count,
    int head_dim,
    float alpha,
    float* output
) {
    if (!query || !mem_keys || !mem_values || !output) return;
    if (token_count <= 0 || head_dim <= 0) return;

    // Allocate attention scores
    float* scores = (float*)malloc(token_count * sizeof(float));
    if (!scores) return;

    // Compute Q @ K^T (scaled dot-product)
    float scale = 1.0f / sqrtf((float)head_dim);
    float max_score = -1e10f;

    for (int i = 0; i < token_count; i++) {
        float dot = 0.0f;
        for (int d = 0; d < head_dim; d++) {
            dot += query[d] * mem_keys[i * head_dim + d];
        }
        scores[i] = dot * scale;
        if (scores[i] > max_score) max_score = scores[i];
    }

    // Softmax (numerically stable)
    float sum_exp = 0.0f;
    for (int i = 0; i < token_count; i++) {
        scores[i] = expf(scores[i] - max_score);
        sum_exp += scores[i];
    }
    for (int i = 0; i < token_count; i++) {
        scores[i] /= sum_exp;
    }

    // Compute weighted sum of values: softmax(scores) @ V
    for (int d = 0; d < head_dim; d++) {
        float weighted_sum = 0.0f;
        for (int i = 0; i < token_count; i++) {
            weighted_sum += scores[i] * mem_values[i * head_dim + d];
        }
        // Accumulate with alpha weight
        output[d] += alpha * weighted_sum;
    }

    free(scores);
}

void zeta_inject_superposition(
    zeta_context_t* ctx,
    const float* query,
    float* hidden_state,
    int n_embd
) {
    if (!ctx || !query || !hidden_state) return;
    if (ctx->memory->num_blocks == 0) return;

    // Find relevant memory blocks
    int indices[ZETA_MAX_ACTIVE_BLOCKS];
    float scores[ZETA_MAX_ACTIVE_BLOCKS];

    int num_found = zeta_find_relevant_blocks(
        ctx->memory,
        query,
        indices,
        scores,
        ZETA_MAX_ACTIVE_BLOCKS
    );

    if (num_found == 0) return;

    #ifdef ZETA_DEBUG
    fprintf(stderr, "zeta: injecting superposition from %d memory blocks\n", num_found);
    #endif

    // Accumulate attention from each relevant block
    for (int i = 0; i < num_found; i++) {
        int block_idx = indices[i];
        float alpha = scores[i];  // Use entanglement score as weight

        // Load block data
        float* kv_data = zeta_load_block(ctx->memory, block_idx);
        if (!kv_data) continue;

        zeta_memory_block_t* block = &ctx->memory->blocks[block_idx];
        int token_count = (int)block->token_count;

        // kv_data format: [keys: token_count * n_embd, values: token_count * n_embd]
        const float* mem_keys = kv_data;
        const float* mem_values = kv_data + token_count * n_embd;

        // Compute attention and inject
        zeta_compute_memory_attention(
            query,
            mem_keys,
            mem_values,
            token_count,
            n_embd,
            alpha,
            hidden_state
        );

        // Mark as accessed
        zeta_touch_block(ctx->memory, block_idx, ctx->current_step);

        #ifdef ZETA_DEBUG
        fprintf(stderr, "  block %d: alpha=%.3f, tokens=%d\n", block_idx, alpha, token_count);
        #endif
    }
}

int64_t zeta_archive_hidden_states(
    zeta_context_t* ctx,
    const float* hidden_states,
    int token_count,
    int64_t token_start
) {
    if (!ctx || !hidden_states || token_count <= 0) return -1;

    // For this POC, we store hidden states as both keys and values
    // In a full implementation, you'd project through separate K/V matrices
    int64_t block_id = zeta_sublimate_block(
        ctx->memory,
        hidden_states,  // Use as keys
        hidden_states,  // Use as values (same for POC)
        token_count,
        token_start
    );

    if (block_id >= 0) {
        zeta_link_new_block(ctx, block_id);
    }

    return block_id;
}

// ============================================================================
// Configuration
// ============================================================================

void zeta_set_temporal_lambda(zeta_context_t* ctx, float lambda) {
    ctx->temporal_lambda = lambda;
    ctx->memory->temporal_lambda = lambda;
}

void zeta_set_tunneling_threshold(zeta_context_t* ctx, float threshold) {
    ctx->tunneling_threshold = threshold;
    ctx->memory->tunneling_threshold = threshold;
}

void zeta_set_retrieve_threshold(zeta_context_t* ctx, float threshold) {
    ctx->retrieve_threshold = threshold;
    ctx->memory->retrieve_threshold = threshold;
}

void zeta_set_momentum_gamma(zeta_context_t* ctx, float gamma) {
    ctx->momentum_gamma = gamma;
    ctx->memory->momentum_gamma = gamma;
}

float zeta_get_temporal_lambda(const zeta_context_t* ctx) {
    return ctx->temporal_lambda;
}

float zeta_get_tunneling_threshold(const zeta_context_t* ctx) {
    return ctx->tunneling_threshold;
}

// ============================================================================
// Sublimation Policy Implementation
// ============================================================================

void zeta_set_sublimate_policy(
    zeta_context_t* ctx,
    zeta_sublimate_policy_t policy,
    int window_size,
    float pressure_pct,
    float attention_decay
) {
    if (!ctx) return;

    ctx->sublimate_policy = policy;
    ctx->sublimate_window_size = window_size > 0 ? window_size : 512;
    ctx->sublimate_pressure_pct = (pressure_pct > 0.0f && pressure_pct <= 1.0f) ? pressure_pct : 0.8f;
    ctx->attention_decay = (attention_decay > 0.0f && attention_decay <= 1.0f) ? attention_decay : 0.95f;

    // Reset attention scores when changing policy
    if (policy == ZETA_SUBLIMATE_ATTENTION && ctx->attention_scores) {
        memset(ctx->attention_scores, 0, ctx->attention_scores_size * sizeof(float));
    }

    fprintf(stderr, "zeta: sublimation policy set to %d (window=%d, pressure=%.2f, decay=%.2f)\n",
            policy, ctx->sublimate_window_size, ctx->sublimate_pressure_pct, ctx->attention_decay);
}

void zeta_update_attention_scores(
    zeta_context_t* ctx,
    const float* attention_weights,
    int kv_length
) {
    if (!ctx || !ctx->attention_scores || !attention_weights) return;
    if (kv_length <= 0 || kv_length > ctx->attention_scores_size) return;

    // Apply temporal decay to existing scores
    float decay = ctx->attention_decay;
    for (int i = 0; i < kv_length; i++) {
        ctx->attention_scores[i] *= decay;
    }

    // Add new attention weights (accumulate importance)
    for (int i = 0; i < kv_length; i++) {
        ctx->attention_scores[i] += attention_weights[i];
    }

    ctx->kv_cache_used = kv_length;
}

void zeta_set_kv_used(zeta_context_t* ctx, int count) {
    if (!ctx) return;
    ctx->kv_cache_used = count;
}

// Comparison function for sorting by attention score (ascending - lowest first)
typedef struct {
    int index;
    float score;
} scored_index_t;

static int compare_scored_asc(const void* a, const void* b) {
    const scored_index_t* sa = (const scored_index_t*)a;
    const scored_index_t* sb = (const scored_index_t*)b;
    if (sa->score < sb->score) return -1;
    if (sa->score > sb->score) return 1;
    return 0;
}

int zeta_get_eviction_candidates(
    const zeta_context_t* ctx,
    int target_evict_count,
    int* evict_indices
) {
    if (!ctx || !ctx->attention_scores || !evict_indices) return 0;
    if (target_evict_count <= 0) return 0;

    int kv_used = ctx->kv_cache_used;
    if (kv_used <= target_evict_count) {
        // Can't evict everything - keep at least some context
        target_evict_count = kv_used / 2;
    }
    if (target_evict_count <= 0) return 0;

    // Build scored indices array
    scored_index_t* scored = (scored_index_t*)malloc(kv_used * sizeof(scored_index_t));
    if (!scored) return 0;

    for (int i = 0; i < kv_used; i++) {
        scored[i].index = i;
        scored[i].score = ctx->attention_scores[i];
    }

    // Sort by score ascending (lowest attention = evict first)
    qsort(scored, kv_used, sizeof(scored_index_t), compare_scored_asc);

    // Return the lowest-scored indices (but never evict position 0 - BOS token)
    int count = 0;
    for (int i = 0; i < kv_used && count < target_evict_count; i++) {
        if (scored[i].index == 0) continue;  // Keep BOS
        evict_indices[count++] = scored[i].index;
    }

    free(scored);

    #ifdef ZETA_DEBUG
    fprintf(stderr, "zeta: eviction candidates: %d tokens (lowest attention scores)\n", count);
    #endif

    return count;
}

int zeta_auto_sublimate(
    zeta_context_t* ctx,
    int current_kv_used,
    int max_kv_size
) {
    if (!ctx || !ctx->llama) return 0;

    ctx->kv_cache_used = current_kv_used;

    bool should_sublimate = false;
    int tokens_to_sublimate = ctx->block_size;

    switch (ctx->sublimate_policy) {
        case ZETA_SUBLIMATE_MANUAL:
            // Never auto-sublimate
            return 0;

        case ZETA_SUBLIMATE_WINDOW:
            // Sublimate every window_size tokens
            if (current_kv_used >= ctx->sublimate_window_size &&
                current_kv_used % ctx->sublimate_window_size < ctx->block_size) {
                should_sublimate = true;
                tokens_to_sublimate = ctx->sublimate_window_size;
            }
            break;

        case ZETA_SUBLIMATE_PRESSURE:
            // Sublimate when KV cache exceeds pressure threshold
            {
                float usage = (float)current_kv_used / max_kv_size;
                if (usage >= ctx->sublimate_pressure_pct) {
                    should_sublimate = true;
                    // Sublimate enough to get below threshold
                    int target = (int)(max_kv_size * (ctx->sublimate_pressure_pct - 0.1f));
                    tokens_to_sublimate = current_kv_used - target;
                    if (tokens_to_sublimate < ctx->block_size) {
                        tokens_to_sublimate = ctx->block_size;
                    }
                }
            }
            break;

        case ZETA_SUBLIMATE_ATTENTION:
            // Sublimate low-attention tokens when above threshold
            {
                float usage = (float)current_kv_used / max_kv_size;
                if (usage >= ctx->sublimate_pressure_pct) {
                    should_sublimate = true;
                    // Sublimate lowest-attention tokens
                    int target = (int)(max_kv_size * (ctx->sublimate_pressure_pct - 0.1f));
                    tokens_to_sublimate = current_kv_used - target;
                    if (tokens_to_sublimate < ctx->block_size) {
                        tokens_to_sublimate = ctx->block_size;
                    }
                }
            }
            break;
    }

    if (!should_sublimate) return 0;

    // For attention-based policy, get the specific tokens to evict
    int* evict_indices = NULL;
    int evict_count = 0;

    if (ctx->sublimate_policy == ZETA_SUBLIMATE_ATTENTION) {
        evict_indices = (int*)malloc(tokens_to_sublimate * sizeof(int));
        if (evict_indices) {
            evict_count = zeta_get_eviction_candidates(ctx, tokens_to_sublimate, evict_indices);
        }
    }

    // Extract and sublimate KV cache
    // For ATTENTION policy: extract specific low-attention positions
    // For WINDOW/PRESSURE: extract oldest positions (FIFO)

    int64_t block_id = -1;

    if (ctx->sublimate_policy == ZETA_SUBLIMATE_ATTENTION && evict_count > 0) {
        // Sort evict_indices to process in order
        // Then use zeta_sublimate_kv_cache with specific range
        // For now, we sublimate the range containing most eviction candidates

        // Find contiguous range with most evictions (simplified approach)
        // A more sophisticated version would handle scattered evictions
        int min_idx = evict_indices[0];
        int max_idx = evict_indices[0];
        for (int i = 1; i < evict_count; i++) {
            if (evict_indices[i] < min_idx) min_idx = evict_indices[i];
            if (evict_indices[i] > max_idx) max_idx = evict_indices[i];
        }

        // Use the KV extraction API with this range
        // Note: This requires the caller to actually remove these from KV cache
        block_id = zeta_sublimate_kv_cache(
            ctx,
            ctx->llama,
            0,  // seq_id
            -1, // all layers
            min_idx,
            max_idx + 1
        );

        fprintf(stderr, "zeta: attention-based sublimation: archived %d tokens [%d-%d] as block %lld\n",
                max_idx - min_idx + 1, min_idx, max_idx, (long long)block_id);

        // Clear attention scores for evicted positions
        for (int i = min_idx; i <= max_idx && i < ctx->attention_scores_size; i++) {
            ctx->attention_scores[i] = 0.0f;
        }
    } else {
        // FIFO: sublimate oldest tokens (positions 1 to tokens_to_sublimate)
        // Position 0 is typically BOS, keep it
        block_id = zeta_sublimate_kv_cache(
            ctx,
            ctx->llama,
            0,  // seq_id
            -1, // all layers
            1,  // start after BOS
            1 + tokens_to_sublimate
        );

        fprintf(stderr, "zeta: FIFO sublimation: archived %d tokens [1-%d] as block %lld\n",
                tokens_to_sublimate, tokens_to_sublimate, (long long)block_id);
    }

    free(evict_indices);

    if (block_id >= 0) {
        zeta_link_new_block(ctx, block_id);
    }

    return (block_id >= 0) ? tokens_to_sublimate : 0;
}

// ============================================================================
// Constitutional Weight Binding Implementation
// ============================================================================

void zeta_apply_output_binding(
    zeta_context_t* ctx,
    float* logits,
    int n_vocab
) {
    if (!ctx || !ctx->binding || !logits) return;

    // Apply vocabulary permutation to logits
    // This scrambles the logits so that without the correct constitution,
    // sampling produces garbage tokens
    zeta_bind_logits(ctx->binding, logits, n_vocab);
}

int32_t zeta_transform_sampled_token(
    zeta_context_t* ctx,
    int32_t sampled_token
) {
    if (!ctx || !ctx->binding) return sampled_token;

    // Map from bound token space back to true vocabulary
    return zeta_unbind_token(ctx->binding, sampled_token);
}

int32_t zeta_transform_input_token(
    zeta_context_t* ctx,
    int32_t token
) {
    if (!ctx || !ctx->binding) return token;

    // Map from true vocabulary to bound space
    return zeta_bind_token(ctx->binding, token);
}

bool zeta_is_constitutionally_bound(const zeta_context_t* ctx) {
    if (!ctx || !ctx->binding) return false;
    return zeta_model_binding_is_active(ctx->binding);
}

void zeta_print_binding_status(const zeta_context_t* ctx) {
    if (!ctx) {
        fprintf(stderr, "[ZETA] Context not initialized\n");
        return;
    }

    zeta_model_binding_print_status(ctx->binding);
}

// ============================================================================
// Metal Kernel API Implementation
// ============================================================================

bool zeta_metal_is_available(const zeta_context_t* ctx) {
#if ZETA_METAL_AVAILABLE
    return ctx && ctx->metal != NULL;
#else
    (void)ctx;
    return false;
#endif
}

int zeta_kernel_temporal_decay(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len,
    int current_pos
) {
    if (!ctx || !scores) return -1;
    if (ctx->temporal_lambda <= 0.0f) return 0;  // No decay

#if ZETA_METAL_AVAILABLE
    if (ctx->metal) {
        return zeta_metal_temporal_decay(
            ctx->metal,
            scores,
            seq_len,
            kv_len,
            current_pos,
            ctx->temporal_lambda
        );
    }
#endif

    // CPU fallback
    zeta_cpu_temporal_decay(
        scores,
        seq_len,
        kv_len,
        current_pos,
        ctx->temporal_lambda
    );
    return 0;
}

int zeta_kernel_sparse_gate(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len
) {
    if (!ctx || !scores) return -1;

#if ZETA_METAL_AVAILABLE
    if (ctx->metal) {
        return zeta_metal_sparse_gate(
            ctx->metal,
            scores,
            seq_len,
            kv_len,
            ctx->tunneling_threshold
        );
    }
#endif

    // CPU fallback
    zeta_cpu_sparse_gate(
        scores,
        seq_len,
        kv_len,
        ctx->tunneling_threshold
    );
    return 0;
}

int zeta_kernel_attention_modifier(
    zeta_context_t* ctx,
    float* scores,
    int seq_len,
    int kv_len,
    int current_pos
) {
    if (!ctx || !scores) return -1;

#if ZETA_METAL_AVAILABLE
    if (ctx->metal) {
        return zeta_metal_attention_modifier(
            ctx->metal,
            scores,
            seq_len,
            kv_len,
            current_pos,
            ctx->temporal_lambda,
            ctx->tunneling_threshold
        );
    }
#endif

    // CPU fallback (combined)
    zeta_cpu_attention_modifier(
        scores,
        seq_len,
        kv_len,
        current_pos,
        ctx->temporal_lambda,
        ctx->tunneling_threshold
    );
    return 0;
}

int zeta_kernel_block_similarities(
    zeta_context_t* ctx,
    const float* query,
    float* similarities,
    int dim
) {
    if (!ctx || !query || !similarities) return -1;
    if (ctx->memory->num_blocks == 0) return 0;

    int n_blocks = ctx->memory->num_blocks;

    // Ensure a cached, contiguous summaries buffer exists and is up-to-date.
    // Summaries are immutable per block, so we only copy when blocks are added.
    if (ctx->block_summaries_cache == NULL || ctx->block_summaries_cache_dim != dim || ctx->block_summaries_cache_cap_blocks < n_blocks) {
        int new_cap = ctx->block_summaries_cache_cap_blocks > 0 ? ctx->block_summaries_cache_cap_blocks : 16;
        while (new_cap < n_blocks) {
            new_cap *= 2;
        }

        float * new_buf = (float*)realloc(ctx->block_summaries_cache, (size_t)new_cap * (size_t)dim * sizeof(float));
        if (!new_buf) {
            return -1;
        }

        ctx->block_summaries_cache = new_buf;
        ctx->block_summaries_cache_dim = dim;
        ctx->block_summaries_cache_cap_blocks = new_cap;
        ctx->block_summaries_cache_filled_blocks = 0; // force refill
    }

    for (int i = ctx->block_summaries_cache_filled_blocks; i < n_blocks; i++) {
        memcpy(ctx->block_summaries_cache + (size_t)i * (size_t)dim,
               ctx->memory->blocks[i].summary,
               (size_t)dim * sizeof(float));
    }
    ctx->block_summaries_cache_filled_blocks = n_blocks;

    const float * summaries = ctx->block_summaries_cache;

#if ZETA_METAL_AVAILABLE
    if (ctx->metal) {
        int result = zeta_metal_cosine_similarity(
            ctx->metal,
            query,
            summaries,
            similarities,
            n_blocks,
            dim
        );
        return result;
    }
#endif

    // CPU fallback - compute cosine similarity
    for (int b = 0; b < n_blocks; b++) {
        const float* summary = summaries + b * dim;
        float dot = 0.0f, q_norm = 0.0f, s_norm = 0.0f;

        for (int d = 0; d < dim; d++) {
            dot += query[d] * summary[d];
            q_norm += query[d] * query[d];
            s_norm += summary[d] * summary[d];
        }

        q_norm = sqrtf(q_norm + 1e-10f);
        s_norm = sqrtf(s_norm + 1e-10f);
        similarities[b] = dot / (q_norm * s_norm);
    }
    return 0;
}

// ============================================================================
// CPU Fallback Implementations (Moved from zeta-metal.m for cross-platform support)
// ============================================================================

void zeta_cpu_temporal_decay(
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda
) {
    if (lambda <= 0.0f) return;

    for (int q = 0; q < seq_len; q++) {
        for (int k = 0; k < kv_len; k++) {
            int token_age = current_pos - k;
            if (token_age > 0) {
                int idx = q * kv_len + k;
                attention_scores[idx] *= expf(-lambda * (float)token_age);
            }
        }
    }
}

void zeta_cpu_sparse_gate(
    float* attention_scores,
    int seq_len,
    int kv_len,
    float threshold
) {
    int total = seq_len * kv_len;
    for (int i = 0; i < total; i++) {
        // Use -INFINITY for sub-threshold attention (pre-softmax)
        // This ensures exp(-inf) = 0 without causing NaN in softmax
        if (attention_scores[i] < threshold) {
            attention_scores[i] = -INFINITY;
        }
    }
}

void zeta_cpu_attention_modifier(
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold
) {
    for (int q = 0; q < seq_len; q++) {
        float max_score = -INFINITY;
        int max_idx = -1;

        // First pass: apply decay and sparse gating, track max
        for (int k = 0; k < kv_len; k++) {
            int idx = q * kv_len + k;
            float score = attention_scores[idx];

            // Skip already-masked positions
            if (score <= -INFINITY) continue;

            // Temporal decay
            if (lambda > 0.0f) {
                int token_age = current_pos - k;
                if (token_age > 0) {
                    score *= expf(-lambda * (float)token_age);
                }
            }

            // Track max before gating (for safeguard)
            if (score > max_score) {
                max_score = score;
                max_idx = idx;
            }

            // Sparse gating (use -INFINITY to prevent NaN)
            if (score < threshold) {
                score = -INFINITY;
            }

            attention_scores[idx] = score;
        }

        // Safeguard: if ALL values are -INFINITY, restore the max
        // This prevents softmax NaN when -inf - (-inf) = NaN
        bool all_masked = true;
        for (int k = 0; k < kv_len; k++) {
            if (attention_scores[q * kv_len + k] > -INFINITY) {
                all_masked = false;
                break;
            }
        }

        if (all_masked && max_idx >= 0) {
            // Restore the best score to prevent NaN
            attention_scores[max_idx] = max_score;
        }
    }
}
