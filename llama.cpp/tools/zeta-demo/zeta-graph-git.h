// Z.E.T.A. Git-Style Graph Branching
// Nodes are immutable "commits". Branches are heads pointing to nodes.
// Different contexts/interpretations can fork from the same facts.
//
// Git concepts mapped to knowledge graph:
//   - Node = Commit (immutable fact/knowledge unit)
//   - Branch = Named head pointing to a node chain
//   - Fork = Create new branch from existing node
//   - Merge = Combine branches (conflict resolution via salience)
//   - HEAD = Current active branch for writes
//
// Unlike git, we support:
//   - Semantic similarity for auto-linking (tunnel edges)
//   - Salience-weighted conflict resolution
//   - Multi-parent merges (knowledge synthesis)
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_GRAPH_GIT_H
#define ZETA_GRAPH_GIT_H

#include "zeta-dual-process.h"
#include <time.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// BRANCH SYSTEM
// =============================================================================

#define ZETA_MAX_BRANCHES       64
#define ZETA_BRANCH_NAME_LEN    128
#define ZETA_DEFAULT_BRANCH     "main"

typedef struct {
    char name[ZETA_BRANCH_NAME_LEN];    // Branch name (e.g., "main", "user/alex", "topic/physics")
    int64_t head_node_id;               // Current head (latest node in this branch)
    int64_t parent_branch_id;           // Branch we forked from (-1 for root)
    int64_t fork_point_node_id;         // Node we forked from
    int64_t created_at;                 // Timestamp
    int64_t last_commit_at;             // Last write timestamp
    int32_t commit_count;               // Number of nodes in this branch
    bool is_active;                     // Branch exists
    bool is_protected;                  // Requires sudo to modify
} zeta_branch_t;

typedef struct {
    zeta_branch_t branches[ZETA_MAX_BRANCHES];
    int num_branches;
    int current_branch_idx;             // HEAD - which branch we're on
    zeta_dual_ctx_t* graph;             // Underlying graph storage
} zeta_git_ctx_t;

// =============================================================================
// INITIALIZATION
// =============================================================================

static inline zeta_git_ctx_t* zeta_git_init(zeta_dual_ctx_t* graph) {
    zeta_git_ctx_t* ctx = (zeta_git_ctx_t*)calloc(1, sizeof(zeta_git_ctx_t));
    if (!ctx) return NULL;

    ctx->graph = graph;
    ctx->num_branches = 0;
    ctx->current_branch_idx = -1;

    // Create default "main" branch
    zeta_branch_t* main = &ctx->branches[0];
    strncpy(main->name, ZETA_DEFAULT_BRANCH, ZETA_BRANCH_NAME_LEN - 1);
    main->head_node_id = -1;            // Empty branch
    main->parent_branch_id = -1;        // Root branch
    main->fork_point_node_id = -1;
    main->created_at = (int64_t)time(NULL);
    main->last_commit_at = main->created_at;
    main->commit_count = 0;
    main->is_active = true;
    main->is_protected = false;

    ctx->num_branches = 1;
    ctx->current_branch_idx = 0;

    fprintf(stderr, "[GIT-GRAPH] Initialized with branch: %s\n", ZETA_DEFAULT_BRANCH);
    return ctx;
}

// =============================================================================
// BRANCH OPERATIONS
// =============================================================================

// Find branch by name
static inline int zeta_git_find_branch(zeta_git_ctx_t* ctx, const char* name) {
    if (!ctx || !name) return -1;
    for (int i = 0; i < ctx->num_branches; i++) {
        if (ctx->branches[i].is_active &&
            strcasecmp(ctx->branches[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Create new branch from current HEAD
static inline int zeta_git_branch(zeta_git_ctx_t* ctx, const char* name) {
    if (!ctx || !name) return -1;
    if (ctx->num_branches >= ZETA_MAX_BRANCHES) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Max branches reached\n");
        return -1;
    }
    if (zeta_git_find_branch(ctx, name) >= 0) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Branch '%s' already exists\n", name);
        return -1;
    }

    zeta_branch_t* current = &ctx->branches[ctx->current_branch_idx];
    zeta_branch_t* new_branch = &ctx->branches[ctx->num_branches];

    strncpy(new_branch->name, name, ZETA_BRANCH_NAME_LEN - 1);
    new_branch->head_node_id = current->head_node_id;  // Same head as parent
    new_branch->parent_branch_id = ctx->current_branch_idx;
    new_branch->fork_point_node_id = current->head_node_id;
    new_branch->created_at = (int64_t)time(NULL);
    new_branch->last_commit_at = new_branch->created_at;
    new_branch->commit_count = 0;  // Own commits start at 0
    new_branch->is_active = true;
    new_branch->is_protected = false;

    int idx = ctx->num_branches;
    ctx->num_branches++;

    fprintf(stderr, "[GIT-GRAPH] Created branch '%s' from '%s' at node %lld\n",
            name, current->name, (long long)current->head_node_id);
    return idx;
}

// Switch to branch (checkout)
static inline bool zeta_git_checkout(zeta_git_ctx_t* ctx, const char* name) {
    int idx = zeta_git_find_branch(ctx, name);
    if (idx < 0) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Branch '%s' not found\n", name);
        return false;
    }

    ctx->current_branch_idx = idx;
    fprintf(stderr, "[GIT-GRAPH] Switched to branch '%s' (head=%lld)\n",
            name, (long long)ctx->branches[idx].head_node_id);
    return true;
}

// Get current branch name
static inline const char* zeta_git_current_branch(zeta_git_ctx_t* ctx) {
    if (!ctx || ctx->current_branch_idx < 0) return NULL;
    return ctx->branches[ctx->current_branch_idx].name;
}

// =============================================================================
// COMMIT (WRITE TO GRAPH)
// =============================================================================

// Commit a new node to current branch
static inline int64_t zeta_git_commit(
    zeta_git_ctx_t* ctx,
    zeta_node_type_t type,
    const char* label,
    const char* value,
    float salience,
    zeta_source_t source
) {
    if (!ctx || ctx->current_branch_idx < 0) return -1;

    zeta_branch_t* branch = &ctx->branches[ctx->current_branch_idx];

    // Create node in underlying graph
    int64_t node_id = zeta_create_node_with_source(
        ctx->graph, type, label, value, salience, source);

    if (node_id < 0) return -1;

    // Link to previous head (creates chain)
    if (branch->head_node_id >= 0) {
        zeta_create_edge(ctx->graph, node_id, branch->head_node_id,
                        EDGE_SUPERSEDES, 1.0f);
    }

    // Update branch head
    branch->head_node_id = node_id;
    branch->last_commit_at = (int64_t)time(NULL);
    branch->commit_count++;

    // Note: Branch tracking is done via edge chain, not in node fields
    // The EDGE_SUPERSEDES chain from branch head traces the branch history

    fprintf(stderr, "[GIT-GRAPH] Committed '%s' to branch '%s' (id=%lld, commits=%d)\n",
            label, branch->name, (long long)node_id, branch->commit_count);
    return node_id;
}

// =============================================================================
// HISTORY TRAVERSAL
// =============================================================================

// Walk back through branch history
typedef void (*zeta_git_walk_fn)(zeta_graph_node_t* node, void* user_data);

static inline int zeta_git_log(
    zeta_git_ctx_t* ctx,
    const char* branch_name,
    int max_count,
    zeta_git_walk_fn callback,
    void* user_data
) {
    int branch_idx = branch_name ?
        zeta_git_find_branch(ctx, branch_name) :
        ctx->current_branch_idx;

    if (branch_idx < 0) return 0;

    zeta_branch_t* branch = &ctx->branches[branch_idx];
    int64_t node_id = branch->head_node_id;
    int count = 0;

    while (node_id >= 0 && count < max_count) {
        zeta_graph_node_t* node = zeta_find_node_by_id(ctx->graph, node_id);
        if (!node || !node->is_active) break;

        if (callback) callback(node, user_data);
        count++;

        // Find parent via DERIVES_FROM edge
        int64_t parent_id = -1;
        for (int i = 0; i < ctx->graph->num_edges; i++) {
            zeta_graph_edge_t* edge = &ctx->graph->edges[i];
            if (edge->source_id == node_id && edge->type == EDGE_SUPERSEDES) {
                parent_id = edge->target_id;
                break;
            }
        }
        node_id = parent_id;
    }

    return count;
}

// =============================================================================
// MERGE
// =============================================================================

typedef enum {
    MERGE_OK,
    MERGE_NO_CHANGES,       // Already up to date
    MERGE_CONFLICT,         // Need resolution
    MERGE_ERROR
} zeta_merge_result_t;

// Merge source branch into current branch
static inline zeta_merge_result_t zeta_git_merge(
    zeta_git_ctx_t* ctx,
    const char* source_branch_name
) {
    int source_idx = zeta_git_find_branch(ctx, source_branch_name);
    if (source_idx < 0) return MERGE_ERROR;

    zeta_branch_t* source = &ctx->branches[source_idx];
    zeta_branch_t* target = &ctx->branches[ctx->current_branch_idx];

    // If source has no commits beyond fork point, nothing to merge
    if (source->head_node_id == source->fork_point_node_id) {
        fprintf(stderr, "[GIT-GRAPH] Nothing to merge from '%s'\n", source_branch_name);
        return MERGE_NO_CHANGES;
    }

    // Create merge commit
    char merge_label[256];
    snprintf(merge_label, sizeof(merge_label),
             "Merge '%s' into '%s'", source_branch_name, target->name);

    int64_t merge_id = zeta_create_node_with_source(
        ctx->graph, NODE_FACT, merge_label, "merge", 0.8f, SOURCE_MODEL);

    if (merge_id < 0) return MERGE_ERROR;

    // Link to both heads (multi-parent merge)
    zeta_create_edge(ctx->graph, merge_id, target->head_node_id,
                    EDGE_SUPERSEDES, 1.0f);
    zeta_create_edge(ctx->graph, merge_id, source->head_node_id,
                    EDGE_RELATED, 1.0f);

    // Update target branch
    target->head_node_id = merge_id;
    target->last_commit_at = (int64_t)time(NULL);
    target->commit_count++;

    fprintf(stderr, "[GIT-GRAPH] Merged '%s' into '%s' (merge commit=%lld)\n",
            source_branch_name, target->name, (long long)merge_id);
    return MERGE_OK;
}

// =============================================================================
// DIFF / STATUS
// =============================================================================

typedef struct {
    int total_nodes;
    int branch_nodes;           // Nodes unique to this branch
    int shared_nodes;           // Nodes shared with parent
    int ahead_count;            // Commits ahead of parent
    int64_t fork_point;
    char parent_branch[ZETA_BRANCH_NAME_LEN];
} zeta_branch_status_t;

static inline zeta_branch_status_t zeta_git_status(zeta_git_ctx_t* ctx) {
    zeta_branch_status_t status = {0};

    if (!ctx || ctx->current_branch_idx < 0) return status;

    zeta_branch_t* branch = &ctx->branches[ctx->current_branch_idx];

    status.branch_nodes = branch->commit_count;
    status.fork_point = branch->fork_point_node_id;
    status.ahead_count = branch->commit_count;

    if (branch->parent_branch_id >= 0) {
        zeta_branch_t* parent = &ctx->branches[branch->parent_branch_id];
        strncpy(status.parent_branch, parent->name, ZETA_BRANCH_NAME_LEN - 1);
    } else {
        strncpy(status.parent_branch, "(root)", ZETA_BRANCH_NAME_LEN - 1);
    }

    // Count total active nodes in graph
    for (int i = 0; i < ctx->graph->num_nodes; i++) {
        if (ctx->graph->nodes[i].is_active) {
            status.total_nodes++;
        }
    }

    return status;
}

// Print all branches
static inline void zeta_git_branch_list(zeta_git_ctx_t* ctx) {
    fprintf(stderr, "\n=== BRANCHES ===\n");
    for (int i = 0; i < ctx->num_branches; i++) {
        if (!ctx->branches[i].is_active) continue;

        zeta_branch_t* b = &ctx->branches[i];
        fprintf(stderr, "%s %s (head=%lld, commits=%d)%s\n",
                (i == ctx->current_branch_idx) ? "*" : " ",
                b->name,
                (long long)b->head_node_id,
                b->commit_count,
                b->is_protected ? " [protected]" : "");
    }
    fprintf(stderr, "================\n\n");
}

// =============================================================================
// DOMAIN BRANCHES (auto-create branches for knowledge domains)
// =============================================================================

// Auto-route commits to domain branches based on content
static inline const char* zeta_git_infer_domain(const char* label, const char* value) {
    // Simple keyword-based domain detection
    // In production, use embedding similarity to domain anchors
    const char* text = value ? value : label;
    if (!text) return ZETA_DEFAULT_BRANCH;

    // Convert to lowercase for matching
    char lower[512];
    size_t len = strlen(text);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    }
    lower[len] = '\0';

    // Domain keywords (expandable)
    if (strstr(lower, "code") || strstr(lower, "function") ||
        strstr(lower, "class") || strstr(lower, "variable")) {
        return "domain/code";
    }
    if (strstr(lower, "file") || strstr(lower, "directory") ||
        strstr(lower, "path") || strstr(lower, "project")) {
        return "domain/filesystem";
    }
    if (strstr(lower, "user") || strstr(lower, "prefer") ||
        strstr(lower, "like") || strstr(lower, "want")) {
        return "domain/preferences";
    }
    if (strstr(lower, "error") || strstr(lower, "bug") ||
        strstr(lower, "fix") || strstr(lower, "issue")) {
        return "domain/debugging";
    }
    if (strstr(lower, "learn") || strstr(lower, "teach") ||
        strstr(lower, "explain") || strstr(lower, "understand")) {
        return "domain/education";
    }

    return ZETA_DEFAULT_BRANCH;
}

// Commit to appropriate domain branch (auto-routing)
static inline int64_t zeta_git_commit_auto(
    zeta_git_ctx_t* ctx,
    zeta_node_type_t type,
    const char* label,
    const char* value,
    float salience,
    zeta_source_t source
) {
    const char* domain = zeta_git_infer_domain(label, value);

    // Create domain branch if needed
    if (zeta_git_find_branch(ctx, domain) < 0) {
        zeta_git_branch(ctx, domain);
    }

    // Save current branch
    int prev_branch = ctx->current_branch_idx;
    const char* prev_name = ctx->branches[prev_branch].name;

    // Switch to domain, commit, switch back
    zeta_git_checkout(ctx, domain);
    int64_t node_id = zeta_git_commit(ctx, type, label, value, salience, source);
    zeta_git_checkout(ctx, prev_name);

    return node_id;
}

// Wrapper for function pointer (matches zeta_git_commit_fn signature)
static inline int64_t zeta_git_commit_auto_wrapper(
    void* ctx,
    int type,
    const char* label,
    const char* value,
    float salience,
    int source
) {
    return zeta_git_commit_auto(
        (zeta_git_ctx_t*)ctx,
        (zeta_node_type_t)type,
        label,
        value,
        salience,
        (zeta_source_t)source
    );
}

// Wire up automatic GitGraph integration with dual-process layer
static inline void zeta_git_wire_auto_commit(zeta_git_ctx_t* ctx) {
    if (ctx) {
        zeta_set_git_ctx(ctx);
        zeta_set_git_commit_fn(zeta_git_commit_auto_wrapper);
        fprintf(stderr, "[GIT-GRAPH] Auto-commit wired: facts will auto-branch by domain\n");
    }
}

// =============================================================================
// TAGS (Immutable named references)
// =============================================================================

#define ZETA_MAX_TAGS 256

typedef struct {
    char name[128];
    int64_t node_id;
    int64_t created_at;
    char message[256];
    bool is_active;
} zeta_tag_t;

static zeta_tag_t g_tags[ZETA_MAX_TAGS] = {0};
static int g_num_tags = 0;

static inline bool zeta_git_tag(zeta_git_ctx_t* ctx, const char* name, const char* message) {
    if (!ctx || !name || g_num_tags >= ZETA_MAX_TAGS) return false;

    zeta_branch_t* branch = &ctx->branches[ctx->current_branch_idx];
    if (branch->head_node_id < 0) return false;

    zeta_tag_t* tag = &g_tags[g_num_tags];
    strncpy(tag->name, name, sizeof(tag->name) - 1);
    tag->node_id = branch->head_node_id;
    tag->created_at = (int64_t)time(NULL);
    if (message) strncpy(tag->message, message, sizeof(tag->message) - 1);
    tag->is_active = true;

    g_num_tags++;
    fprintf(stderr, "[GIT-GRAPH] Tagged node %lld as '%s'\n",
            (long long)tag->node_id, name);
    return true;
}

static inline int64_t zeta_git_tag_resolve(const char* name) {
    for (int i = 0; i < g_num_tags; i++) {
        if (g_tags[i].is_active && strcmp(g_tags[i].name, name) == 0) {
            return g_tags[i].node_id;
        }
    }
    return -1;
}

// =============================================================================
// STASH (Temporary uncommitted work)
// =============================================================================

#define ZETA_MAX_STASH 32

typedef struct {
    int64_t node_ids[64];
    int num_nodes;
    int64_t created_at;
    char message[256];
    int branch_idx;
    bool is_active;
} zeta_stash_entry_t;

static zeta_stash_entry_t g_stash[ZETA_MAX_STASH] = {0};
static int g_stash_top = 0;

static inline bool zeta_git_stash_push(zeta_git_ctx_t* ctx, const char* message) {
    if (!ctx || g_stash_top >= ZETA_MAX_STASH) return false;

    // Find uncommitted nodes (nodes in working state)
    // For now, we stash the current HEAD - in practice would track working nodes
    zeta_branch_t* branch = &ctx->branches[ctx->current_branch_idx];

    zeta_stash_entry_t* entry = &g_stash[g_stash_top];
    entry->node_ids[0] = branch->head_node_id;
    entry->num_nodes = 1;
    entry->created_at = (int64_t)time(NULL);
    entry->branch_idx = ctx->current_branch_idx;
    if (message) strncpy(entry->message, message, sizeof(entry->message) - 1);
    entry->is_active = true;

    g_stash_top++;
    fprintf(stderr, "[GIT-GRAPH] Stashed work: %s\n", message ? message : "(no message)");
    return true;
}

static inline bool zeta_git_stash_pop(zeta_git_ctx_t* ctx) {
    if (g_stash_top <= 0) {
        fprintf(stderr, "[GIT-GRAPH] No stash entries\n");
        return false;
    }

    g_stash_top--;
    zeta_stash_entry_t* entry = &g_stash[g_stash_top];

    // Restore stashed nodes
    fprintf(stderr, "[GIT-GRAPH] Popped stash: %s (%d nodes)\n",
            entry->message, entry->num_nodes);

    entry->is_active = false;
    return true;
}

// =============================================================================
// CHERRY-PICK (Apply specific commit to current branch)
// =============================================================================

static inline int64_t zeta_git_cherry_pick(zeta_git_ctx_t* ctx, int64_t source_node_id) {
    if (!ctx) return -1;

    zeta_graph_node_t* source = zeta_find_node_by_id(ctx->graph, source_node_id);
    if (!source) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Node %lld not found\n", (long long)source_node_id);
        return -1;
    }

    // Create copy of node on current branch
    int64_t new_id = zeta_git_commit(ctx,
        source->type, source->label, source->value,
        source->salience, SOURCE_MODEL);

    if (new_id >= 0) {
        // Mark as cherry-picked (link to original)
        zeta_create_edge(ctx->graph, new_id, source_node_id, EDGE_SUPERSEDES, 0.9f);
        fprintf(stderr, "[GIT-GRAPH] Cherry-picked node %lld as %lld\n",
                (long long)source_node_id, (long long)new_id);
    }

    return new_id;
}

// =============================================================================
// REVERT (Create anti-commit that undoes a node)
// =============================================================================

static inline int64_t zeta_git_revert(zeta_git_ctx_t* ctx, int64_t node_id) {
    if (!ctx) return -1;

    zeta_graph_node_t* node = zeta_find_node_by_id(ctx->graph, node_id);
    if (!node) return -1;

    // Create revert commit
    char revert_label[256];
    snprintf(revert_label, sizeof(revert_label), "Revert: %s", node->label);

    int64_t revert_id = zeta_git_commit(ctx,
        NODE_FACT, revert_label, "(reverted)",
        node->salience, SOURCE_MODEL);

    if (revert_id >= 0) {
        // Mark original as superseded by revert
        node->superseded_by = revert_id;
        // Create contradiction edge
        zeta_create_edge(ctx->graph, revert_id, node_id, EDGE_SUPERSEDES, 1.0f);
        fprintf(stderr, "[GIT-GRAPH] Reverted node %lld with %lld\n",
                (long long)node_id, (long long)revert_id);
    }

    return revert_id;
}

// =============================================================================
// RESET (Move HEAD to different commit)
// =============================================================================

typedef enum {
    RESET_SOFT,     // Move HEAD only, keep working changes
    RESET_MIXED,    // Move HEAD, unstage changes
    RESET_HARD      // Move HEAD, discard all changes
} zeta_reset_mode_t;

static inline bool zeta_git_reset(zeta_git_ctx_t* ctx, int64_t target_node_id, zeta_reset_mode_t mode) {
    if (!ctx || ctx->current_branch_idx < 0) return false;

    zeta_branch_t* branch = &ctx->branches[ctx->current_branch_idx];
    int64_t old_head = branch->head_node_id;

    // Verify target exists
    zeta_graph_node_t* target = zeta_find_node_by_id(ctx->graph, target_node_id);
    if (!target) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Target node %lld not found\n",
                (long long)target_node_id);
        return false;
    }

    // Move HEAD
    branch->head_node_id = target_node_id;

    // Handle mode-specific behavior
    if (mode == RESET_HARD) {
        // Mark nodes between old head and target as inactive
        // (In a real impl, would walk the chain and deactivate)
        fprintf(stderr, "[GIT-GRAPH] Hard reset from %lld to %lld\n",
                (long long)old_head, (long long)target_node_id);
    } else {
        fprintf(stderr, "[GIT-GRAPH] Soft reset from %lld to %lld\n",
                (long long)old_head, (long long)target_node_id);
    }

    return true;
}

// =============================================================================
// REFLOG (Track all HEAD movements)
// =============================================================================

#define ZETA_REFLOG_SIZE 1024

typedef struct {
    int64_t node_id;
    int branch_idx;
    int64_t timestamp;
    char action[64];        // "commit", "checkout", "merge", "reset", etc.
    char message[128];
} zeta_reflog_entry_t;

static zeta_reflog_entry_t g_reflog[ZETA_REFLOG_SIZE] = {0};
static int g_reflog_head = 0;
static int g_reflog_count = 0;

static inline void zeta_git_reflog_add(zeta_git_ctx_t* ctx, const char* action, const char* message) {
    if (!ctx) return;

    zeta_reflog_entry_t* entry = &g_reflog[g_reflog_head];
    entry->node_id = ctx->branches[ctx->current_branch_idx].head_node_id;
    entry->branch_idx = ctx->current_branch_idx;
    entry->timestamp = (int64_t)time(NULL);
    strncpy(entry->action, action, sizeof(entry->action) - 1);
    if (message) strncpy(entry->message, message, sizeof(entry->message) - 1);

    g_reflog_head = (g_reflog_head + 1) % ZETA_REFLOG_SIZE;
    if (g_reflog_count < ZETA_REFLOG_SIZE) g_reflog_count++;
}

static inline void zeta_git_reflog_show(int count) {
    fprintf(stderr, "\n=== REFLOG ===\n");
    int shown = 0;
    int idx = (g_reflog_head - 1 + ZETA_REFLOG_SIZE) % ZETA_REFLOG_SIZE;

    while (shown < count && shown < g_reflog_count) {
        zeta_reflog_entry_t* e = &g_reflog[idx];
        fprintf(stderr, "HEAD@{%d}: %s - %s (node=%lld)\n",
                shown, e->action, e->message, (long long)e->node_id);
        idx = (idx - 1 + ZETA_REFLOG_SIZE) % ZETA_REFLOG_SIZE;
        shown++;
    }
    fprintf(stderr, "==============\n\n");
}

// =============================================================================
// BLAME (Track which commit introduced content)
// =============================================================================

typedef struct {
    int64_t node_id;
    int branch_idx;
    int64_t timestamp;
    char author[64];
} zeta_blame_entry_t;

static inline zeta_blame_entry_t zeta_git_blame(zeta_git_ctx_t* ctx, int64_t node_id) {
    zeta_blame_entry_t result = {0};

    zeta_graph_node_t* node = zeta_find_node_by_id(ctx->graph, node_id);
    if (!node) return result;

    result.node_id = node_id;
    result.branch_idx = 0;  // Would need reverse lookup from branch heads
    result.timestamp = node->created_at;

    // Source attribution
    switch (node->source) {
        case SOURCE_USER:  strncpy(result.author, "user", 64); break;
        case SOURCE_MODEL: strncpy(result.author, "model", 64); break;
        default:           strncpy(result.author, "unknown", 64); break;
    }

    return result;
}

// =============================================================================
// DIFF (Compare two branches/commits)
// =============================================================================

typedef struct {
    int64_t added[256];
    int num_added;
    int64_t removed[256];
    int num_removed;
    int64_t modified[256];
    int num_modified;
} zeta_diff_result_t;

static inline zeta_diff_result_t zeta_git_diff(
    zeta_git_ctx_t* ctx,
    const char* branch_a,
    const char* branch_b
) {
    zeta_diff_result_t diff = {0};

    int idx_a = zeta_git_find_branch(ctx, branch_a);
    int idx_b = zeta_git_find_branch(ctx, branch_b);
    if (idx_a < 0 || idx_b < 0) return diff;

    // Collect nodes from each branch
    int64_t nodes_a[512], nodes_b[512];
    int count_a = 0, count_b = 0;

    // Walk branch A
    int64_t node_id = ctx->branches[idx_a].head_node_id;
    while (node_id >= 0 && count_a < 512) {
        nodes_a[count_a++] = node_id;

        // Find parent
        int64_t parent = -1;
        for (int i = 0; i < ctx->graph->num_edges; i++) {
            if (ctx->graph->edges[i].source_id == node_id &&
                ctx->graph->edges[i].type == EDGE_SUPERSEDES) {
                parent = ctx->graph->edges[i].target_id;
                break;
            }
        }
        node_id = parent;
    }

    // Walk branch B
    node_id = ctx->branches[idx_b].head_node_id;
    while (node_id >= 0 && count_b < 512) {
        nodes_b[count_b++] = node_id;

        int64_t parent = -1;
        for (int i = 0; i < ctx->graph->num_edges; i++) {
            if (ctx->graph->edges[i].source_id == node_id &&
                ctx->graph->edges[i].type == EDGE_SUPERSEDES) {
                parent = ctx->graph->edges[i].target_id;
                break;
            }
        }
        node_id = parent;
    }

    // Find nodes in A but not B (added)
    for (int i = 0; i < count_a; i++) {
        bool found = false;
        for (int j = 0; j < count_b; j++) {
            if (nodes_a[i] == nodes_b[j]) { found = true; break; }
        }
        if (!found && diff.num_added < 256) {
            diff.added[diff.num_added++] = nodes_a[i];
        }
    }

    // Find nodes in B but not A (removed)
    for (int i = 0; i < count_b; i++) {
        bool found = false;
        for (int j = 0; j < count_a; j++) {
            if (nodes_b[i] == nodes_a[j]) { found = true; break; }
        }
        if (!found && diff.num_removed < 256) {
            diff.removed[diff.num_removed++] = nodes_b[i];
        }
    }

    fprintf(stderr, "[GIT-GRAPH] Diff %s..%s: +%d -%d\n",
            branch_a, branch_b, diff.num_added, diff.num_removed);
    return diff;
}

// =============================================================================
// REBASE (Replay commits onto another branch)
// =============================================================================

static inline bool zeta_git_rebase(zeta_git_ctx_t* ctx, const char* onto_branch) {
    if (!ctx) return false;

    int onto_idx = zeta_git_find_branch(ctx, onto_branch);
    if (onto_idx < 0) {
        fprintf(stderr, "[GIT-GRAPH] ERROR: Branch '%s' not found\n", onto_branch);
        return false;
    }

    zeta_branch_t* current = &ctx->branches[ctx->current_branch_idx];
    zeta_branch_t* onto = &ctx->branches[onto_idx];

    // Collect commits unique to current branch
    int64_t commits[256];
    int num_commits = 0;

    int64_t node_id = current->head_node_id;
    int64_t fork_point = current->fork_point_node_id;

    while (node_id >= 0 && node_id != fork_point && num_commits < 256) {
        commits[num_commits++] = node_id;

        int64_t parent = -1;
        for (int i = 0; i < ctx->graph->num_edges; i++) {
            if (ctx->graph->edges[i].source_id == node_id &&
                ctx->graph->edges[i].type == EDGE_SUPERSEDES) {
                parent = ctx->graph->edges[i].target_id;
                break;
            }
        }
        node_id = parent;
    }

    // Reset current branch to onto head
    current->head_node_id = onto->head_node_id;
    current->fork_point_node_id = onto->head_node_id;

    // Replay commits (in reverse order - oldest first)
    for (int i = num_commits - 1; i >= 0; i--) {
        zeta_git_cherry_pick(ctx, commits[i]);
    }

    fprintf(stderr, "[GIT-GRAPH] Rebased %d commits onto '%s'\n", num_commits, onto_branch);
    zeta_git_reflog_add(ctx, "rebase", onto_branch);
    return true;
}

// =============================================================================
// BISECT (Binary search for problematic commit)
// =============================================================================

typedef struct {
    int64_t good;           // Known good commit
    int64_t bad;            // Known bad commit
    int64_t current;        // Current test commit
    int64_t candidates[256];
    int num_candidates;
    int step;
    bool in_progress;
} zeta_bisect_state_t;

static zeta_bisect_state_t g_bisect = {0};

static inline bool zeta_git_bisect_start(zeta_git_ctx_t* ctx, int64_t good, int64_t bad) {
    if (!ctx) return false;

    g_bisect.good = good;
    g_bisect.bad = bad;
    g_bisect.num_candidates = 0;
    g_bisect.step = 0;
    g_bisect.in_progress = true;

    // Collect all commits between good and bad
    int64_t node_id = bad;
    while (node_id >= 0 && node_id != good && g_bisect.num_candidates < 256) {
        g_bisect.candidates[g_bisect.num_candidates++] = node_id;

        int64_t parent = -1;
        for (int i = 0; i < ctx->graph->num_edges; i++) {
            if (ctx->graph->edges[i].source_id == node_id &&
                ctx->graph->edges[i].type == EDGE_SUPERSEDES) {
                parent = ctx->graph->edges[i].target_id;
                break;
            }
        }
        node_id = parent;
    }

    // Start at middle
    int mid = g_bisect.num_candidates / 2;
    g_bisect.current = g_bisect.candidates[mid];

    fprintf(stderr, "[GIT-GRAPH] Bisect started: %d commits to check, testing %lld\n",
            g_bisect.num_candidates, (long long)g_bisect.current);
    return true;
}

static inline int64_t zeta_git_bisect_good(zeta_git_ctx_t* ctx) {
    if (!g_bisect.in_progress) return -1;

    // Current is good, problem is after this
    g_bisect.good = g_bisect.current;

    // Remove candidates before current
    int idx = 0;
    for (int i = 0; i < g_bisect.num_candidates; i++) {
        if (g_bisect.candidates[i] == g_bisect.current) break;
        idx++;
    }
    g_bisect.num_candidates = idx;

    if (g_bisect.num_candidates <= 1) {
        fprintf(stderr, "[GIT-GRAPH] Bisect complete! First bad: %lld\n",
                (long long)g_bisect.candidates[0]);
        g_bisect.in_progress = false;
        return g_bisect.candidates[0];
    }

    g_bisect.current = g_bisect.candidates[g_bisect.num_candidates / 2];
    g_bisect.step++;
    fprintf(stderr, "[GIT-GRAPH] Bisect step %d: %d remaining, testing %lld\n",
            g_bisect.step, g_bisect.num_candidates, (long long)g_bisect.current);
    return -1;  // Not done yet
}

static inline int64_t zeta_git_bisect_bad(zeta_git_ctx_t* ctx) {
    if (!g_bisect.in_progress) return -1;

    // Current is bad, problem is before this
    g_bisect.bad = g_bisect.current;

    // Remove candidates after current
    int idx = 0;
    for (int i = 0; i < g_bisect.num_candidates; i++) {
        if (g_bisect.candidates[i] == g_bisect.current) {
            idx = i + 1;
            break;
        }
    }

    int new_count = 0;
    for (int i = idx; i < g_bisect.num_candidates; i++) {
        g_bisect.candidates[new_count++] = g_bisect.candidates[i];
    }
    g_bisect.num_candidates = new_count;

    if (g_bisect.num_candidates <= 0) {
        fprintf(stderr, "[GIT-GRAPH] Bisect complete! First bad: %lld\n",
                (long long)g_bisect.bad);
        g_bisect.in_progress = false;
        return g_bisect.bad;
    }

    g_bisect.current = g_bisect.candidates[g_bisect.num_candidates / 2];
    g_bisect.step++;
    fprintf(stderr, "[GIT-GRAPH] Bisect step %d: %d remaining, testing %lld\n",
            g_bisect.step, g_bisect.num_candidates, (long long)g_bisect.current);
    return -1;
}

// =============================================================================
// CLEANUP
// =============================================================================

static inline void zeta_git_free(zeta_git_ctx_t* ctx) {
    if (ctx) {
        // Note: Does NOT free the underlying graph (owned externally)
        free(ctx);
    }
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_GRAPH_GIT_H
