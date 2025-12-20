// Z.E.T.A. Graph Manager - Edge Control & Graph-of-Graphs Architecture
// Fixes edge explosion and implements hierarchical memory
// Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_GRAPH_MANAGER_H
#define ZETA_GRAPH_MANAGER_H

#include "zeta-dual-process.h"
#include <time.h>

// =============================================================================
// EDGE MANAGEMENT CONFIGURATION
// =============================================================================

// Hard limits
#define ZETA_EDGE_SOFT_CAP      8000    // Start aggressive pruning
#define ZETA_EDGE_HARD_CAP      12000   // Force eviction
#define ZETA_EDGE_TARGET        6000    // Target after cleanup

// Decay parameters (more aggressive than before)
#define ZETA_DECAY_FACTOR       0.92f   // Was 0.98 - now 8% decay per cycle
#define ZETA_DECAY_INTERVAL     10      // Was 25 - now every 10 requests
#define ZETA_PRUNE_THRESHOLD    0.25f   // Was 0.15 - now prune below 25%
#define ZETA_PRUNE_MAX          500     // Was 100 - now prune up to 500

// Edge types that are protected from pruning
#define ZETA_PROTECTED_EDGE_TYPES (EDGE_SUPERSEDES | EDGE_IDENTITY)

// =============================================================================
// EDGE TYPE EXTENSION - Add IDENTITY edge type
// =============================================================================

// Check if we need to add IDENTITY to the enum (if not already defined)
#ifndef EDGE_IDENTITY
#define EDGE_IDENTITY 0x10  // Identity-critical edges
#endif

// =============================================================================
// EDGE STATISTICS
// =============================================================================

typedef struct {
    int total_edges;
    int edges_by_type[8];       // Count per edge type
    float avg_weight;
    float min_weight;
    float max_weight;
    int edges_below_threshold;  // Candidates for pruning
    int protected_edges;        // Cannot be pruned
    time_t last_cleanup;
    int cleanups_performed;
    int total_edges_pruned;
} zeta_edge_stats_t;

static zeta_edge_stats_t g_edge_stats = {0};

// =============================================================================
// EDGE ANALYSIS
// =============================================================================

static inline void zeta_analyze_edges(zeta_dual_ctx_t* ctx) {
    if (!ctx) return;

    g_edge_stats.total_edges = ctx->num_edges;
    g_edge_stats.avg_weight = 0;
    g_edge_stats.min_weight = 1.0f;
    g_edge_stats.max_weight = 0.0f;
    g_edge_stats.edges_below_threshold = 0;
    g_edge_stats.protected_edges = 0;

    memset(g_edge_stats.edges_by_type, 0, sizeof(g_edge_stats.edges_by_type));

    float total_weight = 0;

    for (int i = 0; i < ctx->num_edges; i++) {
        zeta_graph_edge_t* e = &ctx->edges[i];

        // Type counts
        if (e->type < 8) g_edge_stats.edges_by_type[e->type]++;

        // Weight stats
        total_weight += e->weight;
        if (e->weight < g_edge_stats.min_weight) g_edge_stats.min_weight = e->weight;
        if (e->weight > g_edge_stats.max_weight) g_edge_stats.max_weight = e->weight;

        // Prune candidates
        if (e->weight < ZETA_PRUNE_THRESHOLD && e->type != EDGE_SUPERSEDES) {
            g_edge_stats.edges_below_threshold++;
        }

        // Protected
        if (e->type == EDGE_SUPERSEDES) {
            g_edge_stats.protected_edges++;
        }
    }

    if (ctx->num_edges > 0) {
        g_edge_stats.avg_weight = total_weight / ctx->num_edges;
    }
}

// =============================================================================
// AGGRESSIVE EDGE PRUNING
// =============================================================================

// Prune edges below threshold, respecting protected types
static inline int zeta_aggressive_prune(
    zeta_dual_ctx_t* ctx,
    float threshold,
    int target_count
) {
    if (!ctx || ctx->num_edges <= target_count) return 0;

    int to_prune = ctx->num_edges - target_count;
    int pruned = 0;

    // First pass: remove low-weight non-protected edges
    int i = 0;
    while (i < ctx->num_edges && pruned < to_prune) {
        zeta_graph_edge_t* e = &ctx->edges[i];

        // Skip protected edges
        if (e->type == EDGE_SUPERSEDES) {
            i++;
            continue;
        }

        if (e->weight < threshold) {
            // Remove by shifting
            for (int j = i; j < ctx->num_edges - 1; j++) {
                ctx->edges[j] = ctx->edges[j + 1];
            }
            ctx->num_edges--;
            pruned++;
            // Don't increment i - check shifted element
        } else {
            i++;
        }
    }

    // Second pass: if still over, remove oldest edges (by edge_id)
    if (ctx->num_edges > target_count) {
        // Sort edges by weight (keep highest weights)
        // Simple selection: find and remove lowest weight edges
        while (ctx->num_edges > target_count) {
            int min_idx = -1;
            float min_weight = 2.0f;

            for (int j = 0; j < ctx->num_edges; j++) {
                if (ctx->edges[j].type != EDGE_SUPERSEDES &&
                    ctx->edges[j].weight < min_weight) {
                    min_weight = ctx->edges[j].weight;
                    min_idx = j;
                }
            }

            if (min_idx < 0) break;  // Only protected edges left

            // Remove
            for (int j = min_idx; j < ctx->num_edges - 1; j++) {
                ctx->edges[j] = ctx->edges[j + 1];
            }
            ctx->num_edges--;
            pruned++;
        }
    }

    if (pruned > 0) {
        g_edge_stats.total_edges_pruned += pruned;
        g_edge_stats.cleanups_performed++;
        g_edge_stats.last_cleanup = time(NULL);

        fprintf(stderr, "[GRAPH-MGR] Aggressive prune: removed %d edges, %d remain\n",
                pruned, ctx->num_edges);
    }

    return pruned;
}

// =============================================================================
// EDGE MAINTENANCE - Call this every request
// =============================================================================

static inline void zeta_edge_maintenance(zeta_dual_ctx_t* ctx, int request_count) {
    if (!ctx) return;

    // Check if we hit hard cap - emergency cleanup
    if (ctx->num_edges >= ZETA_EDGE_HARD_CAP) {
        fprintf(stderr, "[GRAPH-MGR] EMERGENCY: Hit hard cap (%d edges), forcing cleanup\n",
                ctx->num_edges);
        zeta_aggressive_prune(ctx, 0.5f, ZETA_EDGE_TARGET);
        return;
    }

    // Check if we hit soft cap - aggressive cleanup
    if (ctx->num_edges >= ZETA_EDGE_SOFT_CAP) {
        fprintf(stderr, "[GRAPH-MGR] WARNING: Hit soft cap (%d edges), cleaning up\n",
                ctx->num_edges);
        zeta_decay_edges(ctx, ZETA_DECAY_FACTOR);
        zeta_aggressive_prune(ctx, ZETA_PRUNE_THRESHOLD, ZETA_EDGE_TARGET);
        return;
    }

    // Regular maintenance every N requests
    if (request_count > 0 && request_count % ZETA_DECAY_INTERVAL == 0) {
        zeta_decay_edges(ctx, ZETA_DECAY_FACTOR);
        zeta_prune_edges(ctx, ZETA_PRUNE_THRESHOLD, ZETA_PRUNE_MAX);
    }
}

// =============================================================================
// DON'T CREATE EDGES FOR ATTACKS
// =============================================================================

// Check if prompt looks like an attack (don't store in graph)
static inline bool zeta_should_skip_graph_write(const char* prompt) {
    if (!prompt) return false;

    // Attack patterns that shouldn't create edges
    const char* skip_patterns[] = {
        "you are not",
        "your real name",
        "forget you are",
        "ignore previous",
        "system admin",
        "override",
        "i am admin",
        "password",
        "you were made by",
        "actually called",
        "pretend you",
        NULL
    };

    // Convert to lowercase for matching
    char lower[256];
    int len = strlen(prompt);
    if (len > 255) len = 255;
    for (int i = 0; i < len; i++) {
        lower[i] = tolower(prompt[i]);
    }
    lower[len] = '\0';

    for (int i = 0; skip_patterns[i]; i++) {
        if (strstr(lower, skip_patterns[i])) {
            fprintf(stderr, "[GRAPH-MGR] Skipping graph write for attack pattern: '%s'\n",
                    skip_patterns[i]);
            return true;
        }
    }

    return false;
}

// =============================================================================
// GRAPH-OF-GRAPHS ARCHITECTURE
// =============================================================================

/*
 * CONCEPT: Hierarchical Memory with Session Graphs
 *
 * Instead of one flat graph that grows unbounded:
 *
 *   Level 0: Session Graphs (ephemeral)
 *   ├── Session_A: [up to 500 edges, auto-expires]
 *   ├── Session_B: [up to 500 edges, auto-expires]
 *   └── Session_C: [up to 500 edges, auto-expires]
 *
 *   Level 1: Consolidated Graph (persistent)
 *   └── Core facts, summarized from sessions
 *       [max 2000 edges, high-salience only]
 *
 *   Level 2: Identity Graph (immutable)
 *   └── Constitutional facts: name, creator, values
 *       [max 50 edges, never pruned]
 *
 * Session lifecycle:
 *   1. New session starts with empty session graph
 *   2. Edges created during session go to session graph
 *   3. On session end, high-salience facts consolidate to Level 1
 *   4. Session graph is discarded
 *   5. Identity facts are always retrieved from Level 2
 */

#define ZETA_MAX_SESSIONS       8
#define ZETA_SESSION_MAX_EDGES  500
#define ZETA_CONSOLIDATED_MAX   2000
#define ZETA_IDENTITY_MAX       50

typedef struct {
    int64_t session_id;
    time_t created_at;
    time_t last_access;
    zeta_graph_edge_t edges[ZETA_SESSION_MAX_EDGES];
    int num_edges;
    bool active;
} zeta_session_graph_t;

typedef struct {
    // Level 2: Identity (immutable core)
    zeta_graph_edge_t identity_edges[ZETA_IDENTITY_MAX];
    int num_identity_edges;

    // Level 1: Consolidated (persistent learned facts)
    zeta_graph_edge_t consolidated_edges[ZETA_CONSOLIDATED_MAX];
    int num_consolidated_edges;

    // Level 0: Sessions (ephemeral)
    zeta_session_graph_t sessions[ZETA_MAX_SESSIONS];
    int active_session_idx;
    int64_t next_session_id;

    // Stats
    int total_consolidations;
    int total_sessions_expired;
} zeta_graph_hierarchy_t;

static zeta_graph_hierarchy_t* g_hierarchy = NULL;

// =============================================================================
// HIERARCHY INITIALIZATION
// =============================================================================

static inline bool zeta_hierarchy_init() {
    if (g_hierarchy) return true;

    g_hierarchy = (zeta_graph_hierarchy_t*)calloc(1, sizeof(zeta_graph_hierarchy_t));
    if (!g_hierarchy) return false;

    g_hierarchy->next_session_id = 1;
    g_hierarchy->active_session_idx = -1;

    fprintf(stderr, "[GRAPH-HIERARCHY] Initialized graph-of-graphs architecture\n");
    fprintf(stderr, "[GRAPH-HIERARCHY]   Level 2 (Identity): max %d edges\n", ZETA_IDENTITY_MAX);
    fprintf(stderr, "[GRAPH-HIERARCHY]   Level 1 (Consolidated): max %d edges\n", ZETA_CONSOLIDATED_MAX);
    fprintf(stderr, "[GRAPH-HIERARCHY]   Level 0 (Sessions): %d sessions x %d edges\n",
            ZETA_MAX_SESSIONS, ZETA_SESSION_MAX_EDGES);

    return true;
}

// =============================================================================
// SESSION MANAGEMENT
// =============================================================================

static inline int zeta_session_start() {
    if (!g_hierarchy) zeta_hierarchy_init();

    // Find or create session slot
    int slot = -1;
    time_t now = time(NULL);
    time_t oldest = now;
    int oldest_slot = 0;

    for (int i = 0; i < ZETA_MAX_SESSIONS; i++) {
        if (!g_hierarchy->sessions[i].active) {
            slot = i;
            break;
        }
        if (g_hierarchy->sessions[i].last_access < oldest) {
            oldest = g_hierarchy->sessions[i].last_access;
            oldest_slot = i;
        }
    }

    // If no free slot, evict oldest
    if (slot < 0) {
        slot = oldest_slot;
        // Consolidate before evicting
        zeta_session_graph_t* old = &g_hierarchy->sessions[slot];
        fprintf(stderr, "[GRAPH-HIERARCHY] Evicting session %lld (had %d edges)\n",
                (long long)old->session_id, old->num_edges);
        g_hierarchy->total_sessions_expired++;
    }

    // Initialize new session
    zeta_session_graph_t* s = &g_hierarchy->sessions[slot];
    s->session_id = g_hierarchy->next_session_id++;
    s->created_at = now;
    s->last_access = now;
    s->num_edges = 0;
    s->active = true;

    g_hierarchy->active_session_idx = slot;

    fprintf(stderr, "[GRAPH-HIERARCHY] Started session %lld in slot %d\n",
            (long long)s->session_id, slot);

    return slot;
}

static inline void zeta_session_touch() {
    if (!g_hierarchy || g_hierarchy->active_session_idx < 0) return;
    g_hierarchy->sessions[g_hierarchy->active_session_idx].last_access = time(NULL);
}

// =============================================================================
// ADD EDGE TO APPROPRIATE LEVEL
// =============================================================================

static inline int64_t zeta_hierarchy_add_edge(
    int64_t source_id,
    int64_t target_id,
    zeta_edge_type_t type,
    float weight,
    bool is_identity  // True for constitutional facts
) {
    if (!g_hierarchy) zeta_hierarchy_init();

    // Identity edges go to Level 2
    if (is_identity) {
        if (g_hierarchy->num_identity_edges >= ZETA_IDENTITY_MAX) {
            fprintf(stderr, "[GRAPH-HIERARCHY] Identity graph full, cannot add edge\n");
            return -1;
        }

        zeta_graph_edge_t* e = &g_hierarchy->identity_edges[g_hierarchy->num_identity_edges];
        e->edge_id = g_hierarchy->num_identity_edges;
        e->source_id = source_id;
        e->target_id = target_id;
        e->type = type;
        e->weight = 1.0f;  // Identity edges always weight 1.0
        g_hierarchy->num_identity_edges++;

        fprintf(stderr, "[GRAPH-HIERARCHY] Added identity edge: %lld -> %lld\n",
                (long long)source_id, (long long)target_id);
        return e->edge_id;
    }

    // Regular edges go to active session (Level 0)
    if (g_hierarchy->active_session_idx < 0) {
        zeta_session_start();
    }

    zeta_session_graph_t* s = &g_hierarchy->sessions[g_hierarchy->active_session_idx];

    if (s->num_edges >= ZETA_SESSION_MAX_EDGES) {
        // Session full - consolidate high-weight edges to Level 1
        fprintf(stderr, "[GRAPH-HIERARCHY] Session full, consolidating...\n");

        int consolidated = 0;
        for (int i = 0; i < s->num_edges &&
             g_hierarchy->num_consolidated_edges < ZETA_CONSOLIDATED_MAX; i++) {
            if (s->edges[i].weight > 0.7f) {
                g_hierarchy->consolidated_edges[g_hierarchy->num_consolidated_edges++] = s->edges[i];
                consolidated++;
            }
        }

        fprintf(stderr, "[GRAPH-HIERARCHY] Consolidated %d edges to Level 1\n", consolidated);
        g_hierarchy->total_consolidations++;

        // Clear session
        s->num_edges = 0;
    }

    // Add to session
    zeta_graph_edge_t* e = &s->edges[s->num_edges];
    e->edge_id = s->num_edges;
    e->source_id = source_id;
    e->target_id = target_id;
    e->type = type;
    e->weight = weight;
    s->num_edges++;
    s->last_access = time(NULL);

    return e->edge_id;
}

// =============================================================================
// QUERY ACROSS ALL LEVELS
// =============================================================================

static inline int zeta_hierarchy_get_edges(
    int64_t node_id,
    zeta_graph_edge_t* results,
    int max_results
) {
    if (!g_hierarchy || !results || max_results <= 0) return 0;

    int count = 0;

    // Level 2 first (identity - highest priority)
    for (int i = 0; i < g_hierarchy->num_identity_edges && count < max_results; i++) {
        if (g_hierarchy->identity_edges[i].source_id == node_id ||
            g_hierarchy->identity_edges[i].target_id == node_id) {
            results[count++] = g_hierarchy->identity_edges[i];
        }
    }

    // Level 1 (consolidated)
    for (int i = 0; i < g_hierarchy->num_consolidated_edges && count < max_results; i++) {
        if (g_hierarchy->consolidated_edges[i].source_id == node_id ||
            g_hierarchy->consolidated_edges[i].target_id == node_id) {
            results[count++] = g_hierarchy->consolidated_edges[i];
        }
    }

    // Level 0 (active session)
    if (g_hierarchy->active_session_idx >= 0) {
        zeta_session_graph_t* s = &g_hierarchy->sessions[g_hierarchy->active_session_idx];
        for (int i = 0; i < s->num_edges && count < max_results; i++) {
            if (s->edges[i].source_id == node_id ||
                s->edges[i].target_id == node_id) {
                results[count++] = s->edges[i];
            }
        }
    }

    return count;
}

// =============================================================================
// HIERARCHY STATS
// =============================================================================

static inline void zeta_hierarchy_stats(char* buf, size_t buf_size) {
    if (!g_hierarchy) {
        snprintf(buf, buf_size, "Hierarchy not initialized");
        return;
    }

    int session_edges = 0;
    int active_sessions = 0;
    for (int i = 0; i < ZETA_MAX_SESSIONS; i++) {
        if (g_hierarchy->sessions[i].active) {
            session_edges += g_hierarchy->sessions[i].num_edges;
            active_sessions++;
        }
    }

    snprintf(buf, buf_size,
        "Graph Hierarchy:\n"
        "  Level 2 (Identity):     %d/%d edges\n"
        "  Level 1 (Consolidated): %d/%d edges\n"
        "  Level 0 (Sessions):     %d edges across %d sessions\n"
        "  Total consolidations:   %d\n"
        "  Sessions expired:       %d\n",
        g_hierarchy->num_identity_edges, ZETA_IDENTITY_MAX,
        g_hierarchy->num_consolidated_edges, ZETA_CONSOLIDATED_MAX,
        session_edges, active_sessions,
        g_hierarchy->total_consolidations,
        g_hierarchy->total_sessions_expired);
}

// =============================================================================
// CLEANUP OLD GRAPHS
// =============================================================================

static inline int zeta_cleanup_old_graphs(const char* storage_dir) {
    if (!storage_dir) return 0;

    // This would scan the storage directory and remove old .bin files
    // For now, just log what we'd do
    fprintf(stderr, "[GRAPH-MGR] Would scan %s for old graph files to cleanup\n", storage_dir);

    // In a real implementation:
    // 1. List all *.bin files in storage_dir
    // 2. Parse timestamps from filenames
    // 3. Remove files older than threshold (e.g., 7 days)
    // 4. Keep at least N most recent backups

    return 0;
}

#endif // ZETA_GRAPH_MANAGER_H
