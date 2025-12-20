// Z.E.T.A. Fact Versioning Implementation
//
// Immutable fact history with version chains.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-version.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// Hash Function
// ============================================================================

static uint32_t hash_concept(const char* key) {
    uint32_t h = 2166136261u;
    while (*key) {
        h ^= (uint8_t)*key++;
        h *= 16777619u;
    }
    return h % ZETA_VERSION_BUCKETS;
}

// ============================================================================
// Initialization
// ============================================================================

zeta_version_ctx_t* zeta_version_init(int max_entries) {
    zeta_version_ctx_t* ctx = (zeta_version_ctx_t*)calloc(1, sizeof(zeta_version_ctx_t));
    if (!ctx) return NULL;

    ctx->entries = (zeta_version_entry_t*)calloc(max_entries, sizeof(zeta_version_entry_t));
    if (!ctx->entries) {
        free(ctx);
        return NULL;
    }

    ctx->max_entries = max_entries;

    // Initialize entries with -1 for unused
    for (int i = 0; i < max_entries; i++) {
        ctx->entries[i].node_id = -1;
        ctx->entries[i].prev_version = -1;
        ctx->entries[i].next_version = -1;
        ctx->entries[i].superseded_by = -1;
        ctx->entries[i].merged_into = -1;
    }

    fprintf(stderr, "[VERSION] Initialized with max %d entries\n", max_entries);
    return ctx;
}

void zeta_version_free(zeta_version_ctx_t* ctx) {
    if (!ctx) return;

    // Free chain entries
    for (int i = 0; i < ZETA_VERSION_BUCKETS; i++) {
        zeta_version_chain_t* chain = ctx->chains[i];
        while (chain) {
            zeta_version_chain_t* next = chain->next;
            free(chain);
            chain = next;
        }
    }

    free(ctx->entries);
    free(ctx);
}

// ============================================================================
// Internal Helpers
// ============================================================================

// Find or create chain for concept
static zeta_version_chain_t* get_or_create_chain(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    bool create
) {
    uint32_t bucket = hash_concept(concept_key);
    zeta_version_chain_t* chain = ctx->chains[bucket];

    while (chain) {
        if (strcmp(chain->concept_key, concept_key) == 0) {
            return chain;
        }
        chain = chain->next;
    }

    if (!create) return NULL;

    // Create new chain
    chain = (zeta_version_chain_t*)calloc(1, sizeof(zeta_version_chain_t));
    if (!chain) return NULL;

    strncpy(chain->concept_key, concept_key, 63);
    chain->head_node = -1;
    chain->tail_node = -1;
    chain->next = ctx->chains[bucket];
    ctx->chains[bucket] = chain;

    return chain;
}

// Find entry slot for node_id (or allocate new)
static zeta_version_entry_t* get_or_create_entry(
    zeta_version_ctx_t* ctx,
    int64_t node_id
) {
    // First check if entry already exists
    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == node_id) {
            return &ctx->entries[i];
        }
    }

    // Allocate new entry
    if (ctx->num_entries >= ctx->max_entries) {
        fprintf(stderr, "[VERSION] ERROR: Max entries reached\n");
        return NULL;
    }

    zeta_version_entry_t* entry = &ctx->entries[ctx->num_entries++];
    entry->node_id = node_id;
    return entry;
}

// ============================================================================
// Version Operations
// ============================================================================

int zeta_version_register(
    zeta_version_ctx_t* ctx,
    int64_t node_id,
    const char* concept_key,
    int64_t created_at
) {
    if (!ctx || !concept_key) return -1;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, true);
    if (!chain) return -1;

    zeta_version_entry_t* entry = get_or_create_entry(ctx, node_id);
    if (!entry) return -1;

    // Set up entry
    entry->node_id = node_id;
    entry->status = ZETA_VS_ACTIVE;
    entry->created_at = created_at;

    if (chain->num_versions == 0) {
        // First version
        entry->version_num = 1;
        entry->prev_version = -1;
        chain->head_node = node_id;
    } else {
        // This is actually an update - find and link to previous
        entry->version_num = chain->num_versions + 1;
        entry->prev_version = chain->tail_node;

        // Update previous tail's next pointer
        for (int i = 0; i < ctx->num_entries; i++) {
            if (ctx->entries[i].node_id == chain->tail_node) {
                ctx->entries[i].next_version = node_id;
                ctx->entries[i].superseded_by = node_id;
                ctx->entries[i].status = ZETA_VS_SUPERSEDED;
                ctx->entries[i].superseded_at = created_at;
                ctx->total_superseded++;
                break;
            }
        }
    }

    chain->tail_node = node_id;
    chain->num_versions++;
    ctx->total_versions++;

    return entry->version_num;
}

int zeta_version_update(
    zeta_version_ctx_t* ctx,
    int64_t new_node_id,
    const char* concept_key,
    const char* reason,
    float confidence_delta,
    int64_t created_at
) {
    if (!ctx || !concept_key) return -1;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, false);
    if (!chain || chain->num_versions == 0) {
        // No existing versions - register as first
        return zeta_version_register(ctx, new_node_id, concept_key, created_at);
    }

    // Find current tail and supersede it
    int64_t old_tail = chain->tail_node;

    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == old_tail) {
            ctx->entries[i].next_version = new_node_id;
            ctx->entries[i].superseded_by = new_node_id;
            ctx->entries[i].status = ZETA_VS_SUPERSEDED;
            ctx->entries[i].superseded_at = created_at;
            if (reason) {
                strncpy(ctx->entries[i].reason, reason, 127);
            }
            ctx->total_superseded++;
            break;
        }
    }

    // Create new entry
    zeta_version_entry_t* entry = get_or_create_entry(ctx, new_node_id);
    if (!entry) return -1;

    entry->node_id = new_node_id;
    entry->prev_version = old_tail;
    entry->next_version = -1;
    entry->superseded_by = -1;
    entry->status = ZETA_VS_ACTIVE;
    entry->version_num = chain->num_versions + 1;
    entry->created_at = created_at;
    entry->confidence_delta = confidence_delta;

    chain->tail_node = new_node_id;
    chain->num_versions++;
    ctx->total_versions++;

    fprintf(stderr, "[VERSION] Updated '%s' v%d -> v%d (reason: %s)\n",
            concept_key, entry->version_num - 1, entry->version_num,
            reason ? reason : "unspecified");

    return entry->version_num;
}

bool zeta_version_retract(
    zeta_version_ctx_t* ctx,
    int64_t node_id,
    const char* reason
) {
    if (!ctx) return false;

    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == node_id) {
            ctx->entries[i].status = ZETA_VS_RETRACTED;
            ctx->entries[i].superseded_at = (int64_t)time(NULL);
            if (reason) {
                strncpy(ctx->entries[i].reason, reason, 127);
            }
            ctx->total_retracted++;
            fprintf(stderr, "[VERSION] Retracted node %lld: %s\n",
                    (long long)node_id, reason ? reason : "unspecified");
            return true;
        }
    }

    return false;
}

bool zeta_version_merge(
    zeta_version_ctx_t* ctx,
    const int64_t* source_nodes,
    int num_sources,
    int64_t target_node,
    const char* reason
) {
    if (!ctx || !source_nodes || num_sources <= 0) return false;

    int64_t now = (int64_t)time(NULL);

    for (int s = 0; s < num_sources; s++) {
        for (int i = 0; i < ctx->num_entries; i++) {
            if (ctx->entries[i].node_id == source_nodes[s]) {
                ctx->entries[i].status = ZETA_VS_MERGED;
                ctx->entries[i].merged_into = target_node;
                ctx->entries[i].superseded_at = now;
                if (reason) {
                    strncpy(ctx->entries[i].reason, reason, 127);
                }
                break;
            }
        }
    }

    fprintf(stderr, "[VERSION] Merged %d nodes into %lld\n",
            num_sources, (long long)target_node);

    return true;
}

// ============================================================================
// Lookup
// ============================================================================

int64_t zeta_version_current(
    zeta_version_ctx_t* ctx,
    const char* concept_key
) {
    if (!ctx || !concept_key) return -1;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, false);
    if (!chain) return -1;

    return chain->tail_node;
}

int zeta_version_history(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    int64_t* node_ids,
    int max_nodes
) {
    if (!ctx || !concept_key || !node_ids) return 0;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, false);
    if (!chain) return 0;

    int count = 0;
    int64_t current = chain->head_node;

    while (current >= 0 && count < max_nodes) {
        node_ids[count++] = current;

        // Find next version
        int64_t next = -1;
        for (int i = 0; i < ctx->num_entries; i++) {
            if (ctx->entries[i].node_id == current) {
                next = ctx->entries[i].next_version;
                break;
            }
        }
        current = next;
    }

    return count;
}

const zeta_version_entry_t* zeta_version_get(
    zeta_version_ctx_t* ctx,
    int64_t node_id
) {
    if (!ctx) return NULL;

    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == node_id) {
            return &ctx->entries[i];
        }
    }

    return NULL;
}

bool zeta_version_is_current(
    zeta_version_ctx_t* ctx,
    int64_t node_id
) {
    const zeta_version_entry_t* entry = zeta_version_get(ctx, node_id);
    return entry && entry->status == ZETA_VS_ACTIVE;
}

int64_t zeta_version_prev(
    zeta_version_ctx_t* ctx,
    int64_t node_id
) {
    const zeta_version_entry_t* entry = zeta_version_get(ctx, node_id);
    return entry ? entry->prev_version : -1;
}

// ============================================================================
// Rollback
// ============================================================================

bool zeta_version_rollback(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    const char* reason
) {
    if (!ctx || !concept_key) return false;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, false);
    if (!chain || chain->num_versions < 2) return false;

    int64_t current_tail = chain->tail_node;
    int64_t prev_tail = -1;

    // Find current tail and its predecessor
    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == current_tail) {
            prev_tail = ctx->entries[i].prev_version;

            // Mark current as superseded
            ctx->entries[i].status = ZETA_VS_SUPERSEDED;
            ctx->entries[i].superseded_at = (int64_t)time(NULL);
            if (reason) {
                strncpy(ctx->entries[i].reason, reason, 127);
            }
            break;
        }
    }

    if (prev_tail < 0) return false;

    // Reactivate previous version
    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == prev_tail) {
            ctx->entries[i].status = ZETA_VS_ACTIVE;
            ctx->entries[i].superseded_by = -1;
            ctx->entries[i].next_version = -1;
            break;
        }
    }

    chain->tail_node = prev_tail;
    chain->num_versions--;  // Effective version count

    fprintf(stderr, "[VERSION] Rolled back '%s' to v%d\n",
            concept_key, chain->num_versions);

    return true;
}

bool zeta_version_rollback_to(
    zeta_version_ctx_t* ctx,
    const char* concept_key,
    int target_version,
    const char* reason
) {
    if (!ctx || !concept_key || target_version < 1) return false;

    zeta_version_chain_t* chain = get_or_create_chain(ctx, concept_key, false);
    if (!chain) return false;

    // Find target version node
    int64_t target_node = -1;
    int64_t current = chain->head_node;

    while (current >= 0) {
        for (int i = 0; i < ctx->num_entries; i++) {
            if (ctx->entries[i].node_id == current) {
                if (ctx->entries[i].version_num == target_version) {
                    target_node = current;
                }
                current = ctx->entries[i].next_version;
                break;
            }
        }
        if (target_node >= 0) break;
    }

    if (target_node < 0) return false;

    // Mark all versions after target as superseded
    current = target_node;
    bool found_target = false;

    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id == target_node) {
            ctx->entries[i].status = ZETA_VS_ACTIVE;
            ctx->entries[i].superseded_by = -1;

            // Mark everything after as superseded
            int64_t to_mark = ctx->entries[i].next_version;
            while (to_mark >= 0) {
                for (int j = 0; j < ctx->num_entries; j++) {
                    if (ctx->entries[j].node_id == to_mark) {
                        ctx->entries[j].status = ZETA_VS_SUPERSEDED;
                        ctx->entries[j].superseded_at = (int64_t)time(NULL);
                        if (reason) {
                            strncpy(ctx->entries[j].reason, reason, 127);
                        }
                        to_mark = ctx->entries[j].next_version;
                        break;
                    }
                }
            }

            ctx->entries[i].next_version = -1;
            found_target = true;
            break;
        }
    }

    if (!found_target) return false;

    chain->tail_node = target_node;

    fprintf(stderr, "[VERSION] Rolled back '%s' to v%d\n",
            concept_key, target_version);

    return true;
}

// ============================================================================
// Statistics
// ============================================================================

void zeta_version_stats(
    const zeta_version_ctx_t* ctx,
    zeta_version_stats_t* stats
) {
    if (!ctx || !stats) return;

    memset(stats, 0, sizeof(*stats));

    // Count chains (concepts)
    for (int i = 0; i < ZETA_VERSION_BUCKETS; i++) {
        zeta_version_chain_t* chain = ctx->chains[i];
        while (chain) {
            stats->total_concepts++;
            chain = chain->next;
        }
    }

    // Count versions by status
    for (int i = 0; i < ctx->num_entries; i++) {
        if (ctx->entries[i].node_id < 0) continue;

        stats->total_versions++;
        switch (ctx->entries[i].status) {
            case ZETA_VS_ACTIVE:
                stats->active_versions++;
                break;
            case ZETA_VS_SUPERSEDED:
                stats->superseded_versions++;
                break;
            case ZETA_VS_RETRACTED:
                stats->retracted_versions++;
                break;
            default:
                break;
        }
    }

    if (stats->total_concepts > 0) {
        stats->avg_versions_per_concept =
            (float)stats->total_versions / stats->total_concepts;
    }
}

void zeta_version_print_stats(const zeta_version_ctx_t* ctx) {
    if (!ctx) return;

    zeta_version_stats_t stats;
    zeta_version_stats(ctx, &stats);

    fprintf(stderr, "[VERSION] Stats:\n");
    fprintf(stderr, "  Concepts: %lld\n", (long long)stats.total_concepts);
    fprintf(stderr, "  Total versions: %lld\n", (long long)stats.total_versions);
    fprintf(stderr, "  Active: %lld\n", (long long)stats.active_versions);
    fprintf(stderr, "  Superseded: %lld\n", (long long)stats.superseded_versions);
    fprintf(stderr, "  Retracted: %lld\n", (long long)stats.retracted_versions);
    fprintf(stderr, "  Avg versions/concept: %.2f\n", stats.avg_versions_per_concept);
}
