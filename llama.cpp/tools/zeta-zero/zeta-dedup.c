// Z.E.T.A. Dedup Engine Implementation
//
// O(1) deduplication using hash tables, LSH, and bloom filters.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-dedup.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// ============================================================================
// Hash Functions
// ============================================================================

// FNV-1a hash for strings (fast, good distribution)
static uint32_t fnv1a_hash(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

// MurmurHash3 finalizer for integers
static uint32_t murmur3_mix(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// ============================================================================
// Bloom Filter
// ============================================================================

static void bloom_add(zeta_bloom_t* bloom, const char* key) {
    uint32_t h = fnv1a_hash(key);
    for (int i = 0; i < ZETA_DEDUP_BLOOM_HASHES; i++) {
        uint32_t bit = murmur3_mix(h + i) % ZETA_DEDUP_BLOOM_SIZE;
        bloom->bits[bit / 64] |= (1ULL << (bit % 64));
    }
    bloom->num_items++;
}

static bool bloom_check(const zeta_bloom_t* bloom, const char* key) {
    uint32_t h = fnv1a_hash(key);
    for (int i = 0; i < ZETA_DEDUP_BLOOM_HASHES; i++) {
        uint32_t bit = murmur3_mix(h + i) % ZETA_DEDUP_BLOOM_SIZE;
        if (!(bloom->bits[bit / 64] & (1ULL << (bit % 64)))) {
            return false;  // Definitely not present
        }
    }
    return true;  // Might be present
}

// ============================================================================
// LSH (Locality-Sensitive Hashing)
// ============================================================================

// Initialize random hyperplanes for LSH
static void lsh_init_hyperplanes(zeta_lsh_table_t* table, int embd_dim) {
    table->embd_dim = embd_dim;
    int n_hyperplanes = ZETA_DEDUP_LSH_BITS;

    table->hyperplanes = (float*)malloc(n_hyperplanes * embd_dim * sizeof(float));
    if (!table->hyperplanes) return;

    // Random unit vectors (simplified: random gaussian, not normalized)
    srand((unsigned int)time(NULL));
    for (int i = 0; i < n_hyperplanes * embd_dim; i++) {
        // Box-Muller for gaussian
        float u1 = ((float)rand() / RAND_MAX) + 1e-6f;
        float u2 = ((float)rand() / RAND_MAX);
        table->hyperplanes[i] = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * 3.14159f * u2);
    }

    // Initialize buckets
    int n_buckets = 1 << ZETA_DEDUP_LSH_BITS;
    table->buckets = (int64_t**)calloc(n_buckets, sizeof(int64_t*));
    table->bucket_sizes = (int*)calloc(n_buckets, sizeof(int));
    table->bucket_capacities = (int*)calloc(n_buckets, sizeof(int));
}

static void lsh_free(zeta_lsh_table_t* table) {
    free(table->hyperplanes);
    int n_buckets = 1 << ZETA_DEDUP_LSH_BITS;
    for (int i = 0; i < n_buckets; i++) {
        free(table->buckets[i]);
    }
    free(table->buckets);
    free(table->bucket_sizes);
    free(table->bucket_capacities);
}

// Compute LSH hash for an embedding
static uint32_t lsh_hash(const zeta_lsh_table_t* table, const float* embedding) {
    uint32_t hash = 0;
    const float* hp = table->hyperplanes;

    for (int bit = 0; bit < ZETA_DEDUP_LSH_BITS; bit++) {
        float dot = 0.0f;
        for (int j = 0; j < table->embd_dim; j++) {
            dot += embedding[j] * hp[bit * table->embd_dim + j];
        }
        if (dot >= 0) {
            hash |= (1u << bit);
        }
    }

    return hash;
}

// Add node to LSH table
static void lsh_add(zeta_lsh_table_t* table, int64_t node_id, const float* embedding) {
    uint32_t hash = lsh_hash(table, embedding);

    // Grow bucket if needed
    if (table->bucket_sizes[hash] >= table->bucket_capacities[hash]) {
        int new_cap = table->bucket_capacities[hash] == 0 ? 4 : table->bucket_capacities[hash] * 2;
        table->buckets[hash] = (int64_t*)realloc(table->buckets[hash], new_cap * sizeof(int64_t));
        table->bucket_capacities[hash] = new_cap;
    }

    table->buckets[hash][table->bucket_sizes[hash]++] = node_id;
}

// Find candidates in LSH table
static int lsh_find_candidates(
    const zeta_lsh_table_t* table,
    const float* query,
    int64_t* candidates,
    int max_candidates
) {
    uint32_t hash = lsh_hash(table, query);
    int count = 0;

    // Return all nodes in same bucket
    for (int i = 0; i < table->bucket_sizes[hash] && count < max_candidates; i++) {
        candidates[count++] = table->buckets[hash][i];
    }

    return count;
}

// ============================================================================
// Cosine Similarity
// ============================================================================

static float cosine_similarity(const float* a, const float* b, int dim) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    float denom = sqrtf(norm_a) * sqrtf(norm_b);
    return (denom > 1e-8f) ? (dot / denom) : 0.0f;
}

// ============================================================================
// Context Initialization
// ============================================================================

zeta_dedup_ctx_t* zeta_dedup_init(int embd_dim, float similarity_threshold) {
    zeta_dedup_ctx_t* ctx = (zeta_dedup_ctx_t*)calloc(1, sizeof(zeta_dedup_ctx_t));
    if (!ctx) return NULL;

    ctx->embd_dim = embd_dim;
    ctx->similarity_threshold = (similarity_threshold > 0) ? similarity_threshold : 0.85f;

    // Initialize LSH tables with different random seeds
    for (int i = 0; i < ZETA_DEDUP_LSH_TABLES; i++) {
        srand((unsigned int)time(NULL) + i * 12345);
        lsh_init_hyperplanes(&ctx->lsh_tables[i], embd_dim);
    }

    fprintf(stderr, "[DEDUP] Initialized: %d tables, %d bits/hash, threshold=%.2f\n",
            ZETA_DEDUP_LSH_TABLES, ZETA_DEDUP_LSH_BITS, ctx->similarity_threshold);

    return ctx;
}

void zeta_dedup_free(zeta_dedup_ctx_t* ctx) {
    if (!ctx) return;

    // Free hash table entries
    for (int i = 0; i < ZETA_DEDUP_HASH_BUCKETS; i++) {
        zeta_dedup_entry_t* entry = ctx->hash_table[i];
        while (entry) {
            zeta_dedup_entry_t* next = entry->next;
            free(entry);
            entry = next;
        }
    }

    // Free LSH tables
    for (int i = 0; i < ZETA_DEDUP_LSH_TABLES; i++) {
        lsh_free(&ctx->lsh_tables[i]);
    }

    free(ctx);
}

// ============================================================================
// Index Operations
// ============================================================================

bool zeta_dedup_add(
    zeta_dedup_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key,
    const float* embedding
) {
    if (!ctx || !concept_key) return false;

    // Check bloom filter first (fast negative)
    if (bloom_check(&ctx->bloom, concept_key)) {
        // Might exist, check hash table
        int64_t existing = zeta_dedup_find_exact(ctx, concept_key);
        if (existing >= 0) {
            return false;  // Duplicate by concept_key
        }
    }

    // Add to bloom filter
    bloom_add(&ctx->bloom, concept_key);

    // Add to hash table
    uint32_t bucket = fnv1a_hash(concept_key) % ZETA_DEDUP_HASH_BUCKETS;
    zeta_dedup_entry_t* entry = (zeta_dedup_entry_t*)malloc(sizeof(zeta_dedup_entry_t));
    if (!entry) return false;

    strncpy(entry->concept_key, concept_key, 63);
    entry->concept_key[63] = '\0';
    entry->node_id = node_id;
    entry->next = ctx->hash_table[bucket];
    ctx->hash_table[bucket] = entry;

    // Add to LSH tables
    if (embedding) {
        for (int i = 0; i < ZETA_DEDUP_LSH_TABLES; i++) {
            lsh_add(&ctx->lsh_tables[i], node_id, embedding);
        }
    }

    ctx->num_entries++;
    return true;
}

void zeta_dedup_remove(
    zeta_dedup_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key
) {
    if (!ctx || !concept_key) return;

    // Remove from hash table
    uint32_t bucket = fnv1a_hash(concept_key) % ZETA_DEDUP_HASH_BUCKETS;
    zeta_dedup_entry_t** pp = &ctx->hash_table[bucket];

    while (*pp) {
        if (strcmp((*pp)->concept_key, concept_key) == 0) {
            zeta_dedup_entry_t* to_free = *pp;
            *pp = (*pp)->next;
            free(to_free);
            ctx->num_entries--;
            return;
        }
        pp = &(*pp)->next;
    }

    // Note: bloom filter doesn't support removal (would need counting bloom)
    // LSH removal would require scanning buckets (expensive, skip for now)
}

// ============================================================================
// Lookup Operations
// ============================================================================

int64_t zeta_dedup_find_exact(
    zeta_dedup_ctx_t* ctx,
    const char* concept_key
) {
    if (!ctx || !concept_key) return -1;

    ctx->num_lookups++;

    // Quick bloom check
    if (!bloom_check(&ctx->bloom, concept_key)) {
        return -1;  // Definitely not present
    }

    // Check hash table
    uint32_t bucket = fnv1a_hash(concept_key) % ZETA_DEDUP_HASH_BUCKETS;
    zeta_dedup_entry_t* entry = ctx->hash_table[bucket];

    while (entry) {
        if (strcmp(entry->concept_key, concept_key) == 0) {
            ctx->num_hits++;
            return entry->node_id;
        }
        entry = entry->next;
    }

    return -1;
}

int zeta_dedup_find_similar(
    zeta_dedup_ctx_t* ctx,
    const float* query_embedding,
    int64_t* similar_ids,
    float* similarities,
    int max_results
) {
    if (!ctx || !query_embedding || !similar_ids || max_results <= 0) return 0;

    ctx->num_lookups++;

    // Collect candidates from all LSH tables
    int64_t candidates[256];
    int num_candidates = 0;

    for (int t = 0; t < ZETA_DEDUP_LSH_TABLES && num_candidates < 256; t++) {
        int64_t table_candidates[64];
        int n = lsh_find_candidates(&ctx->lsh_tables[t], query_embedding,
                                     table_candidates, 64);

        for (int i = 0; i < n && num_candidates < 256; i++) {
            // Dedup candidates across tables
            bool seen = false;
            for (int j = 0; j < num_candidates; j++) {
                if (candidates[j] == table_candidates[i]) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                candidates[num_candidates++] = table_candidates[i];
            }
        }
    }

    ctx->num_lsh_candidates += num_candidates;

    // Verify candidates with actual cosine similarity
    // Sort by similarity (simple insertion sort for small N)
    int num_results = 0;
    float result_sims[64];

    for (int i = 0; i < num_candidates && num_results < max_results; i++) {
        // Need to get embedding for this node (caller must provide lookup function)
        // For now, we just return candidates without verification
        // In practice, you'd look up the embedding and compute similarity

        similar_ids[num_results] = candidates[i];
        if (similarities) {
            similarities[num_results] = 1.0f;  // Placeholder
        }
        num_results++;
    }

    if (num_results > 0) {
        ctx->num_hits++;
    }

    return num_results;
}

bool zeta_dedup_maybe_exists(
    zeta_dedup_ctx_t* ctx,
    const char* concept_key
) {
    if (!ctx || !concept_key) return false;
    return bloom_check(&ctx->bloom, concept_key);
}

// ============================================================================
// Batch Operations
// ============================================================================

int zeta_dedup_build_index(
    zeta_dedup_ctx_t* ctx,
    const zeta_dedup_node_info_t* nodes,
    int num_nodes
) {
    if (!ctx || !nodes) return 0;

    int added = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (zeta_dedup_add(ctx, nodes[i].node_id, nodes[i].concept_key, nodes[i].embedding)) {
            added++;
        }
    }

    fprintf(stderr, "[DEDUP] Built index: %d/%d nodes added\n", added, num_nodes);
    return added;
}

// ============================================================================
// Statistics
// ============================================================================

void zeta_dedup_get_stats(
    const zeta_dedup_ctx_t* ctx,
    zeta_dedup_stats_t* stats
) {
    if (!ctx || !stats) return;

    memset(stats, 0, sizeof(*stats));
    stats->num_entries = ctx->num_entries;
    stats->num_lookups = ctx->num_lookups;
    stats->num_hits = ctx->num_hits;
    stats->num_lsh_candidates = ctx->num_lsh_candidates;

    if (ctx->num_lookups > 0) {
        stats->hit_rate = (float)ctx->num_hits / ctx->num_lookups;
    }

    // Calculate average bucket depth
    int total_depth = 0;
    int non_empty = 0;
    for (int i = 0; i < ZETA_DEDUP_HASH_BUCKETS; i++) {
        int depth = 0;
        zeta_dedup_entry_t* entry = ctx->hash_table[i];
        while (entry) {
            depth++;
            entry = entry->next;
        }
        if (depth > 0) {
            total_depth += depth;
            non_empty++;
        }
    }

    if (non_empty > 0) {
        stats->avg_bucket_depth = (float)total_depth / non_empty;
    }
}

void zeta_dedup_print_stats(const zeta_dedup_ctx_t* ctx) {
    if (!ctx) return;

    zeta_dedup_stats_t stats;
    zeta_dedup_get_stats(ctx, &stats);

    fprintf(stderr, "[DEDUP] Stats:\n");
    fprintf(stderr, "  Entries: %lld\n", (long long)stats.num_entries);
    fprintf(stderr, "  Lookups: %lld\n", (long long)stats.num_lookups);
    fprintf(stderr, "  Hits: %lld (%.1f%%)\n",
            (long long)stats.num_hits, stats.hit_rate * 100.0f);
    fprintf(stderr, "  LSH candidates: %lld\n", (long long)stats.num_lsh_candidates);
    fprintf(stderr, "  Avg bucket depth: %.2f\n", stats.avg_bucket_depth);
}
