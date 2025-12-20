// Z.E.T.A. Dedup Engine - O(1) Deduplication for Large Graphs
//
// Provides fast deduplication using:
// 1. Hash table for concept_key lookup (exact match)
// 2. LSH (Locality-Sensitive Hashing) for embedding similarity
// 3. Bloom filter for fast negative checks
//
// Scales to 10K+ nodes with constant-time lookups.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_DEDUP_H
#define ZETA_DEDUP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_DEDUP_HASH_BUCKETS     1024    // Hash table buckets (power of 2)
#define ZETA_DEDUP_LSH_TABLES       8       // Number of LSH hash tables
#define ZETA_DEDUP_LSH_BITS         12      // Bits per LSH hash
#define ZETA_DEDUP_BLOOM_SIZE       8192    // Bloom filter bits
#define ZETA_DEDUP_BLOOM_HASHES     4       // Number of bloom hash functions
#define ZETA_DEDUP_MAX_BUCKET_DEPTH 16      // Max nodes per hash bucket

// ============================================================================
// Hash Table Entry (for concept_key lookup)
// ============================================================================

typedef struct zeta_dedup_entry {
    char concept_key[64];
    int64_t node_id;
    struct zeta_dedup_entry* next;  // Chaining for collisions
} zeta_dedup_entry_t;

// ============================================================================
// LSH Table (for embedding similarity)
// ============================================================================

typedef struct {
    // Random hyperplanes for this table (dot product sign = hash bit)
    float* hyperplanes;     // [LSH_BITS × embd_dim]
    int embd_dim;

    // Hash buckets: bucket[hash] -> list of node_ids
    int64_t** buckets;      // [2^LSH_BITS][variable]
    int* bucket_sizes;      // [2^LSH_BITS]
    int* bucket_capacities; // [2^LSH_BITS]
} zeta_lsh_table_t;

// ============================================================================
// Bloom Filter (fast negative check)
// ============================================================================

typedef struct {
    uint64_t bits[ZETA_DEDUP_BLOOM_SIZE / 64];
    int num_items;
} zeta_bloom_t;

// ============================================================================
// Dedup Context
// ============================================================================

typedef struct {
    // Hash table for concept_key (exact match)
    zeta_dedup_entry_t* hash_table[ZETA_DEDUP_HASH_BUCKETS];

    // LSH tables for embedding similarity
    zeta_lsh_table_t lsh_tables[ZETA_DEDUP_LSH_TABLES];
    int embd_dim;
    float similarity_threshold;  // Default 0.85

    // Bloom filter for fast negative check
    zeta_bloom_t bloom;

    // Stats
    int64_t num_entries;
    int64_t num_lookups;
    int64_t num_hits;
    int64_t num_lsh_candidates;
} zeta_dedup_ctx_t;

// ============================================================================
// Initialization / Cleanup
// ============================================================================

// Create dedup context
// embd_dim: embedding dimension for LSH (e.g., 256)
// similarity_threshold: cosine similarity threshold for "same" (default 0.85)
zeta_dedup_ctx_t* zeta_dedup_init(int embd_dim, float similarity_threshold);

// Free dedup context
void zeta_dedup_free(zeta_dedup_ctx_t* ctx);

// ============================================================================
// Index Operations
// ============================================================================

// Add a node to the dedup index
// Returns true if added, false if duplicate detected
bool zeta_dedup_add(
    zeta_dedup_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key,
    const float* embedding  // [embd_dim]
);

// Remove a node from the index (e.g., when superseded)
void zeta_dedup_remove(
    zeta_dedup_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key
);

// ============================================================================
// Lookup Operations
// ============================================================================

// Check if concept_key exists (exact match)
// Returns node_id if found, -1 if not found
int64_t zeta_dedup_find_exact(
    zeta_dedup_ctx_t* ctx,
    const char* concept_key
);

// Find similar nodes by embedding (LSH + verify)
// Returns number of similar nodes found
// similar_ids: output array (caller allocates, max_results size)
// similarities: output array of cosine similarities (optional, can be NULL)
int zeta_dedup_find_similar(
    zeta_dedup_ctx_t* ctx,
    const float* query_embedding,
    int64_t* similar_ids,
    float* similarities,
    int max_results
);

// Quick bloom filter check (fast negative)
// Returns true if MIGHT exist, false if DEFINITELY doesn't exist
bool zeta_dedup_maybe_exists(
    zeta_dedup_ctx_t* ctx,
    const char* concept_key
);

// ============================================================================
// Batch Operations
// ============================================================================

// Build index from existing graph nodes
// Call this on startup to index loaded graph
typedef struct {
    int64_t node_id;
    char concept_key[64];
    float* embedding;
} zeta_dedup_node_info_t;

int zeta_dedup_build_index(
    zeta_dedup_ctx_t* ctx,
    const zeta_dedup_node_info_t* nodes,
    int num_nodes
);

// ============================================================================
// Statistics
// ============================================================================

typedef struct {
    int64_t num_entries;
    int64_t num_lookups;
    int64_t num_hits;
    int64_t num_lsh_candidates;
    float hit_rate;
    float avg_bucket_depth;
} zeta_dedup_stats_t;

void zeta_dedup_get_stats(
    const zeta_dedup_ctx_t* ctx,
    zeta_dedup_stats_t* stats
);

void zeta_dedup_print_stats(const zeta_dedup_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // ZETA_DEDUP_H
