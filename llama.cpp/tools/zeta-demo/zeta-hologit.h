// Z.E.T.A. HoloGit - Memory Versioning and Correlation Tracking
//
// Concepts:
// - Memory blocks have version history (like git commits)
// - Co-retrieved blocks form "entanglement edges" with weights
// - Weights evolve: frequently co-retrieved blocks strengthen correlation
// - Summary vectors can be "patched" when correlations stabilize
// - Enables semantic merge: related memories reinforce each other
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_HOLOGIT_H
#define ZETA_HOLOGIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_MAX_EDGES_PER_BLOCK  32    // Max correlation edges per block
#define ZETA_MAX_VERSION_HISTORY  16    // Max versions to retain per block
#define ZETA_CORRELATION_DECAY    0.95f // Edge weight decay per step without co-retrieval
#define ZETA_CORRELATION_BOOST    0.1f  // Edge weight increase on co-retrieval

// ============================================================================
// Data Structures
// ============================================================================

// Correlation edge between two memory blocks
typedef struct {
    int64_t block_a;             // First block ID
    int64_t block_b;             // Second block ID
    float   weight;              // Correlation strength [0, 1]
    int64_t co_retrieval_count;  // Times retrieved together
    int64_t last_co_retrieval;   // Step of last co-retrieval
} zeta_edge_t;

// Version snapshot of a block's summary vector
typedef struct {
    int64_t version_id;          // Incrementing version number
    int64_t step_created;        // Inference step when created
    float*  summary_snapshot;    // Copy of summary vector at this version
    char    reason[64];          // Why this version was created
} zeta_version_t;

// Block metadata with versioning
typedef struct {
    int64_t block_id;

    // Version history
    zeta_version_t versions[ZETA_MAX_VERSION_HISTORY];
    int num_versions;
    int current_version;

    // Correlation edges (neighbors in semantic graph)
    int64_t edge_targets[ZETA_MAX_EDGES_PER_BLOCK];
    float   edge_weights[ZETA_MAX_EDGES_PER_BLOCK];
    int     num_edges;

    // Stability metrics
    float   summary_drift;       // How much summary has changed over time
    bool    is_stable;           // True if converged (low drift)
} zeta_block_meta_t;

// HoloGit context
typedef struct {
    zeta_block_meta_t* block_meta;  // Array[num_blocks]
    int max_blocks;
    int num_blocks;

    // Global edge list (for efficient iteration)
    zeta_edge_t* edges;
    int max_edges;
    int num_edges;

    // Configuration
    float correlation_decay;
    float correlation_boost;
    float stability_threshold;   // Drift below this = stable

    // Statistics
    int64_t total_co_retrievals;
    int64_t total_patches;
    int64_t stable_blocks;
} zeta_hologit_t;

// ============================================================================
// Initialization
// ============================================================================

zeta_hologit_t* zeta_hologit_init(int max_blocks, int summary_dim);
void zeta_hologit_free(zeta_hologit_t* hg);

// ============================================================================
// Block Registration
// ============================================================================

// Register a new block with HoloGit (called after sublimation)
// Returns block index in HoloGit, or -1 on error
int zeta_hologit_register_block(
    zeta_hologit_t* hg,
    int64_t block_id,
    const float* initial_summary,
    int summary_dim
);

// ============================================================================
// Co-Retrieval Tracking
// ============================================================================

// Record that blocks were retrieved together
// Strengthens their correlation edge
void zeta_hologit_record_co_retrieval(
    zeta_hologit_t* hg,
    const int64_t* block_ids,
    int num_blocks,
    int64_t current_step
);

// Apply decay to all edges (call periodically, e.g., every 100 steps)
void zeta_hologit_decay_edges(zeta_hologit_t* hg);

// ============================================================================
// Summary Evolution (Patching)
// ============================================================================

// Check if block should be patched based on correlated neighbors
// If strongly correlated blocks have drifted, this block may need updating
bool zeta_hologit_should_patch(
    const zeta_hologit_t* hg,
    int64_t block_id
);

// Compute patched summary incorporating correlated blocks
// weighted_sum = original + sum(weight_i * neighbor_i) / (1 + sum(weight_i))
void zeta_hologit_compute_patch(
    const zeta_hologit_t* hg,
    int64_t block_id,
    const float* original_summary,
    const float** neighbor_summaries,  // Array of pointers to neighbor summaries
    int summary_dim,
    float* patched_summary_out
);

// Apply patch and create new version
void zeta_hologit_apply_patch(
    zeta_hologit_t* hg,
    int64_t block_id,
    const float* new_summary,
    int summary_dim,
    const char* reason
);

// ============================================================================
// Convergence Detection
// ============================================================================

// Check if block has converged (summary stable over versions)
bool zeta_hologit_is_converged(
    const zeta_hologit_t* hg,
    int64_t block_id
);

// Get convergence status for all blocks
void zeta_hologit_check_convergence(zeta_hologit_t* hg, int summary_dim);

// ============================================================================
// Query Enhancement
// ============================================================================

// Enhance query using correlated blocks
// If query matches block A, and A correlates with B,
// we can preemptively include B in retrieval candidates
void zeta_hologit_expand_retrieval_set(
    const zeta_hologit_t* hg,
    const int64_t* initial_blocks,
    int num_initial,
    int64_t* expanded_blocks_out,
    int* num_expanded_out,
    int max_expanded,
    float min_correlation          // Only include edges above this weight
);

// ============================================================================
// Serialization
// ============================================================================

// Save HoloGit state to disk
int zeta_hologit_save(const zeta_hologit_t* hg, const char* path);

// Load HoloGit state from disk
zeta_hologit_t* zeta_hologit_load(const char* path);

// ============================================================================
// Debugging
// ============================================================================

void zeta_hologit_print_stats(const zeta_hologit_t* hg);
void zeta_hologit_print_block(const zeta_hologit_t* hg, int64_t block_id);
void zeta_hologit_print_top_edges(const zeta_hologit_t* hg, int n);

#ifdef __cplusplus
}
#endif

#endif // ZETA_HOLOGIT_H
