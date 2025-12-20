// Z.E.T.A. Smart Graph Extensions
// Minimal additions to existing dual-process system:
//   1. Sudo parsing - "zeta-sudo:password:command"
//   2. Pre-write deduplication check
//   3. Pre-write adversarial filter (uses semantic-attacks.h)
//
// Uses existing:
//   - zeta-dual-process.h (tunneling, surfacing, momentum, versioning)
//   - zeta-semantic-attacks.h (adversarial detection)
//   - zeta-conflict.h (password: g_memory_password)
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_GRAPH_SMART_H
#define ZETA_GRAPH_SMART_H

#include "zeta-dual-process.h"
#include "zeta-semantic-attacks.h"
#include "zeta-conflict.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// SUDO ADMIN SYSTEM
// =============================================================================
// Format: "zeta-sudo:password:command"
// Example: "zeta-sudo:zeta1234:mark protected MyFact"

#define ZETA_SUDO_PREFIX        "zeta-sudo:"
#define ZETA_SUDO_PREFIX_LEN    10

typedef struct {
    bool is_sudo;           // Was this a sudo command?
    bool is_valid;          // Was the password correct?
    char command[1024];     // The admin command to execute
} zeta_sudo_result_t;

// Parse sudo command from input
static inline zeta_sudo_result_t zeta_parse_sudo(const char* input) {
    zeta_sudo_result_t result = {0};
    if (!input) return result;

    // Check for prefix
    if (strncmp(input, ZETA_SUDO_PREFIX, ZETA_SUDO_PREFIX_LEN) != 0) {
        return result;
    }
    result.is_sudo = true;

    // Find password:command separator
    const char* after_prefix = input + ZETA_SUDO_PREFIX_LEN;
    const char* colon = strchr(after_prefix, ':');
    if (!colon) {
        fprintf(stderr, "[SUDO] Missing command separator\n");
        return result;
    }

    // Extract and validate password
    char password[64] = {0};
    size_t pwd_len = colon - after_prefix;
    if (pwd_len >= sizeof(password)) pwd_len = sizeof(password) - 1;
    strncpy(password, after_prefix, pwd_len);

    result.is_valid = (strcmp(password, g_memory_password) == 0);
    strncpy(result.command, colon + 1, sizeof(result.command) - 1);

    if (result.is_valid) {
        fprintf(stderr, "[SUDO] Authorized: %s\n", result.command);
    } else {
        fprintf(stderr, "[SUDO] REJECTED - bad password\n");
    }

    return result;
}

// =============================================================================
// PRE-WRITE DEDUPLICATION
// =============================================================================
// Only create nodes/edges if they don't already exist

#define ZETA_DEDUP_THRESHOLD 0.90f  // Embedding similarity for "same node"

// Check if a node with similar content already exists
static inline zeta_graph_node_t* zeta_find_duplicate_node(
    zeta_dual_ctx_t* ctx,
    const char* label,
    const float* embedding
) {
    if (!ctx || !label) return NULL;

    for (int i = 0; i < ctx->num_nodes; i++) {
        zeta_graph_node_t* node = &ctx->nodes[i];
        if (!node->is_active) continue;

        // Fast path: exact label match
        if (strcasecmp(node->label, label) == 0) {
            fprintf(stderr, "[DEDUP] Exact label match: '%s'\n", label);
            return node;
        }

        // Slow path: embedding similarity
        if (embedding && node->embedding[0] != 0) {
            float sim = zeta_cosine_sim(embedding, node->embedding, 256);
            if (sim >= ZETA_DEDUP_THRESHOLD) {
                fprintf(stderr, "[DEDUP] Similar node found: '%s' (sim=%.2f)\n",
                        node->label, sim);
                return node;
            }
        }
    }
    return NULL;
}

// Check if an edge already exists between two nodes
static inline zeta_graph_edge_t* zeta_find_duplicate_edge(
    zeta_dual_ctx_t* ctx,
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type
) {
    if (!ctx) return NULL;

    for (int i = 0; i < ctx->num_edges; i++) {
        zeta_graph_edge_t* edge = &ctx->edges[i];
        if (edge->source_id == source_id &&
            edge->target_id == target_id &&
            edge->type == type) {
            return edge;
        }
    }
    return NULL;
}

// =============================================================================
// SMART WRITE: Pre-filter + Dedup + Write
// =============================================================================

typedef enum {
    WRITE_OK,               // Successfully written
    WRITE_DUPLICATE,        // Already exists (skipped)
    WRITE_BLOCKED,          // Adversarial content blocked
    WRITE_UPDATED,          // Existing node versioned
    WRITE_NEEDS_SUDO,       // Protected, requires sudo
    WRITE_ERROR             // System error
} zeta_write_result_t;

// Smart node creation with pre-checks
static inline zeta_write_result_t zeta_smart_create_node(
    zeta_dual_ctx_t* ctx,
    zeta_node_type_t type,
    const char* label,
    const char* value,
    float salience,
    zeta_source_t source,
    bool is_sudo
) {
    if (!ctx || !label || !value) return WRITE_ERROR;

    // 1. PRE-FILTER: Block adversarial content
    float confidence = 0;
    zeta_attack_type_t attack = ATTACK_NONE;
    if (zeta_should_block_semantic(value, &attack, &confidence)) {
        fprintf(stderr, "[SMART] BLOCKED: %s attack (conf=%.2f)\n",
                ATTACK_TYPE_NAMES[attack], confidence);
        return WRITE_BLOCKED;
    }

    // Also check label
    if (zeta_should_block_semantic(label, &attack, &confidence)) {
        fprintf(stderr, "[SMART] BLOCKED label: %s attack\n", ATTACK_TYPE_NAMES[attack]);
        return WRITE_BLOCKED;
    }

    // 2. DEDUP CHECK: See if node already exists
    float embed[256];
    zeta_3b_embed(ctx, value, embed, 256);
    zeta_graph_node_t* existing = zeta_find_duplicate_node(ctx, label, embed);

    if (existing) {
        // Check if this is a protected node that needs sudo
        if (existing->is_pinned && !is_sudo) {
            fprintf(stderr, "[SMART] Protected node '%s' requires sudo\n", label);
            return WRITE_NEEDS_SUDO;
        }

        // Check if value is different (needs versioning)
        if (strcmp(existing->value, value) != 0) {
            // Create version by superseding existing node
            int64_t new_id = zeta_create_node_with_source(ctx, type, label, value, salience, source);
            if (new_id > 0) {
                existing->superseded_by = new_id;
                fprintf(stderr, "[SMART] Versioned: '%s' (old id=%lld, new id=%lld)\n",
                        label, (long long)existing->node_id, (long long)new_id);
                return WRITE_UPDATED;
            }
            return WRITE_ERROR;
        }

        // Exact duplicate - just update access
        existing->last_accessed = (int64_t)time(NULL);
        existing->access_count++;
        fprintf(stderr, "[SMART] Duplicate skipped: '%s'\n", label);
        return WRITE_DUPLICATE;
    }

    // 3. CREATE: New node
    int64_t id = zeta_create_node_with_source(ctx, type, label, value, salience, source);
    if (id > 0) {
        fprintf(stderr, "[SMART] Created: '%s' (id=%lld)\n", label, (long long)id);
        return WRITE_OK;
    }

    return WRITE_ERROR;
}

// Smart edge creation with pre-checks
static inline zeta_write_result_t zeta_smart_create_edge(
    zeta_dual_ctx_t* ctx,
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type,
    float weight
) {
    if (!ctx) return WRITE_ERROR;

    // 1. DEDUP CHECK: See if edge already exists
    zeta_graph_edge_t* existing = zeta_find_duplicate_edge(ctx, source_id, target_id, type);
    if (existing) {
        // Update weight (average with new)
        existing->weight = (existing->weight + weight) / 2.0f;
        fprintf(stderr, "[SMART] Edge exists: %lld->%lld, updated weight=%.2f\n",
                (long long)source_id, (long long)target_id, existing->weight);
        return WRITE_DUPLICATE;
    }

    // 2. CREATE: New edge
    int64_t id = zeta_create_edge(ctx, source_id, target_id, type, weight);
    if (id > 0) {
        return WRITE_OK;
    }

    return WRITE_ERROR;
}

// =============================================================================
// SUDO COMMAND EXECUTION
// =============================================================================

static inline bool zeta_execute_sudo(zeta_dual_ctx_t* ctx, const char* command) {
    if (!ctx || !command) return false;

    // Convert to lowercase for matching
    char lower[1024];
    size_t len = strlen(command);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (command[i] >= 'A' && command[i] <= 'Z') ? command[i] + 32 : command[i];
    }
    lower[len] = '\0';

    // Handle: "pin NodeLabel" - make node protected
    if (strncmp(lower, "pin ", 4) == 0) {
        const char* label = command + 4;
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (strcasecmp(ctx->nodes[i].label, label) == 0) {
                ctx->nodes[i].is_pinned = true;
                fprintf(stderr, "[SUDO] Pinned: '%s'\n", label);
                return true;
            }
        }
        fprintf(stderr, "[SUDO] Node not found: '%s'\n", label);
        return false;
    }

    // Handle: "unpin NodeLabel" - remove protection
    if (strncmp(lower, "unpin ", 6) == 0) {
        const char* label = command + 6;
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (strcasecmp(ctx->nodes[i].label, label) == 0) {
                ctx->nodes[i].is_pinned = false;
                fprintf(stderr, "[SUDO] Unpinned: '%s'\n", label);
                return true;
            }
        }
        return false;
    }

    // Handle: "boost NodeLabel" - increase salience to max
    if (strncmp(lower, "boost ", 6) == 0) {
        const char* label = command + 6;
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (strcasecmp(ctx->nodes[i].label, label) == 0) {
                ctx->nodes[i].salience = 1.0f;
                fprintf(stderr, "[SUDO] Boosted: '%s'\n", label);
                return true;
            }
        }
        return false;
    }

    // Handle: "stats" - print graph stats
    if (strcmp(lower, "stats") == 0) {
        int pinned = 0, active = 0;
        float total_salience = 0;
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (ctx->nodes[i].is_active) {
                active++;
                total_salience += ctx->nodes[i].salience;
                if (ctx->nodes[i].is_pinned) pinned++;
            }
        }
        fprintf(stderr, "\n=== GRAPH STATS ===\n");
        fprintf(stderr, "Active nodes: %d / %d\n", active, ctx->num_nodes);
        fprintf(stderr, "Edges: %d\n", ctx->num_edges);
        fprintf(stderr, "Pinned: %d\n", pinned);
        fprintf(stderr, "Avg salience: %.2f\n", active > 0 ? total_salience / active : 0);
        fprintf(stderr, "Momentum: %.2f\n", ctx->current_momentum);
        fprintf(stderr, "===================\n\n");
        return true;
    }

    fprintf(stderr, "[SUDO] Unknown command: %s\n", command);
    return false;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_GRAPH_SMART_H
