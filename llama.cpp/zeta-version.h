// Z.E.T.A. Fact Versioning System
//
// Facts are never deleted - only versioned.
// Updated facts create new nodes that link back to predecessors.
//
// Node lifecycle:
//   ACTIVE     → Current version, used in retrieval
//   SUPERSEDED → Replaced by newer version, kept for history
//   RETRACTED  → Explicitly marked as false/outdated
//   MERGED     → Combined into another node
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_VERSION_H
#define ZETA_VERSION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Version Status
// ============================================================================

typedef enum {
    ZETA_VS_ACTIVE      = 0,    // Current version
    ZETA_VS_SUPERSEDED  = 1,    // Replaced by newer version
    ZETA_VS_RETRACTED   = 2,    // Marked as false/outdated
    ZETA_VS_MERGED      = 3,    // Combined into another node
    ZETA_VS_ARCHIVED    = 4     // Moved to cold storage
} zeta_version_status_t;

// ============================================================================
// Version Chain Entry
// ============================================================================

typedef struct {
    int64_t node_id;            // This node
    int64_t prev_version;       // Previous version (-1 if first)
    int64_t next_version;       // Next version (-1 if current)
    int64_t superseded_by;      // Node that replaced this (-1 if active)
    int64_t merged_into;        // Node this was merged into (-1 if not merged)

    zeta_version_status_t status;
    int32_t version_num;        // 1, 2, 3, ... for this concept

    int64_t created_at;         // Timestamp
    int64_t superseded_at;      // When superseded (0 if active)

    char reason[128];           // Why superseded/retracted
    float confidence_delta;     // Change in confidence from prev version
} zeta_version_entry_t;

// ============================================================================
// Version Index (for fast lookup by concept_key)
// ============================================================================

#define ZETA_VERSION_BUCKETS 512

typedef struct zeta_version_chain {
    char concept_key[64];
    int64_t head_node;          // First version (oldest)
    int64_t tail_node;          // Latest version (current)
    int32_t num_versions;
    struct zeta_version_chain* next;  // Hash collision chain
} zeta_version_chain_t;

typedef struct {
    // Hash table: concept_key -> version chain
    zeta_version_chain_t* chains[ZETA_VERSION_BUCKETS];

    // Version entries (indexed by node_id for fast lookup)
    zeta_version_entry_t* entries;
    int num_entries;
    int max_entries;

    // Stats
    int64_t total_versions;
    int64_t total_superseded;
    int64_t total_retracted;
} zeta_version_ctx_t;

// ============================================================================
// Initialization
// ============================================================================

zeta_version_ctx_t* zeta_version_init(int max_entries);
void zeta_version_free(zeta_version_ctx_t* ctx);

// ============================================================================
// Version Operations
// ============================================================================

// Register a new node (first version of a concept)
// Returns version number (1 for new concepts)
int zeta_version_register(
    zeta_version_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key,
    int64_t created_at
);

// Create new version of existing concept
// Returns new version number, or -1 if concept doesn't exist
// The old node is marked SUPERSEDED, new node becomes current
int zeta_version_update(
    zeta_version_ctx_t* ctx,
    int64_t new_node_id,
    const char* concept_key,
    const char* reason,         // Why updated (e.g., "user correction")
    float confidence_delta,     // Change in confidence
    int64_t created_at
);

// Retract a fact (mark as false without replacement)
bool zeta_version_retract(
    zeta_version_ctx_t* ctx,
    int64_t node_id,
    const char* reason
);

// Merge multiple nodes into one
bool zeta_version_merge(
    zeta_version_ctx_t* ctx,
    const int64_t* source_nodes,
    int num_sources,
    int64_t target_node,
    const char* reason
);

// ============================================================================
// Lookup
// ============================================================================

// Get current (latest active) version for a concept
int64_t zeta_version_current(
    zeta_version_ctx_t* ctx,
    const char* concept_key
);

// Get all versions of a concept (oldest to newest)
int zeta_version_history(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    int64_t* node_ids,          // Output array
    int max_nodes
);

// Get version entry for a node
const zeta_version_entry_t* zeta_version_get(
    zeta_version_ctx_t* ctx,
    int64_t node_id
);

// Check if node is current version
bool zeta_version_is_current(
    zeta_version_ctx_t* ctx,
    int64_t node_id
);

// Get previous version of a node
int64_t zeta_version_prev(
    zeta_version_ctx_t* ctx,
    int64_t node_id
);

// ============================================================================
// Conflict Detection
// ============================================================================

// Check if new value conflicts with existing versions
// Returns node_id of conflicting node, or -1 if no conflict
typedef struct {
    int64_t conflicting_node;
    float similarity;           // How similar to existing
    int version_num;            // Which version conflicts
    char conflict_type[32];     // "contradiction", "update", "duplicate"
} zeta_version_conflict_t;

int zeta_version_check_conflict(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    const char* new_value,
    zeta_version_conflict_t* conflicts,
    int max_conflicts
);

// ============================================================================
// Rollback
// ============================================================================

// Rollback to previous version
// Current becomes SUPERSEDED, previous becomes ACTIVE
bool zeta_version_rollback(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    const char* reason
);

// Rollback to specific version number
bool zeta_version_rollback_to(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    int target_version,
    const char* reason
);

// ============================================================================
// Garbage Collection
// ============================================================================

// Archive old superseded versions (move to cold storage)
int zeta_version_archive_old(
    zeta_version_ctx_t* ctx,
    int64_t max_age_seconds,    // Archive versions older than this
    int max_to_archive
);

// Get stats
typedef struct {
    int64_t total_concepts;
    int64_t total_versions;
    int64_t active_versions;
    int64_t superseded_versions;
    int64_t retracted_versions;
    float avg_versions_per_concept;
} zeta_version_stats_t;

void zeta_version_stats(
    const zeta_version_ctx_t* ctx,
    zeta_version_stats_t* stats
);

void zeta_version_print_stats(const zeta_version_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // ZETA_VERSION_H
