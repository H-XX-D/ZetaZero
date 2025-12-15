#!/usr/bin/env python3
"""
Add causal chain traversal to Z.E.T.A. streaming
When a fact is surfaced, also surface its causal predecessors and successors
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h'

with open(filepath, 'r') as f:
    content = f.read()

# Add causal chain function after zeta_stage_related
if 'zeta_follow_causal_chain' not in content:
    old_marker = '''static inline void zeta_stage_related(zeta_dual_ctx_t* ctx, int64_t node_id, float momentum) {
    if (!ctx || momentum < 0.7f) return;
    g_staged_count = 0;
    for (int i = 0; i < ctx->num_edges && g_staged_count < ZETA_STAGING_MAX; i++) {
        zeta_graph_edge_t* e = &ctx->edges[i];
        int64_t related = (e->source_id == node_id) ? e->target_id : ((e->target_id == node_id) ? e->source_id : -1);
        if (related >= 0) {
            bool dup = false;
            for (int j = 0; j < g_staged_count; j++) if (g_staged_ids[j] == related) { dup = true; break; }
            if (!dup) g_staged_ids[g_staged_count++] = related;
        }
    }
}'''

    new_marker = '''static inline void zeta_stage_related(zeta_dual_ctx_t* ctx, int64_t node_id, float momentum) {
    if (!ctx || momentum < 0.7f) return;
    g_staged_count = 0;
    for (int i = 0; i < ctx->num_edges && g_staged_count < ZETA_STAGING_MAX; i++) {
        zeta_graph_edge_t* e = &ctx->edges[i];
        int64_t related = (e->source_id == node_id) ? e->target_id : ((e->target_id == node_id) ? e->source_id : -1);
        if (related >= 0) {
            bool dup = false;
            for (int j = 0; j < g_staged_count; j++) if (g_staged_ids[j] == related) { dup = true; break; }
            if (!dup) g_staged_ids[g_staged_count++] = related;
        }
    }
}

// ============================================================
// CAUSAL CHAIN TRAVERSAL: Follow CAUSES/PREVENTS edges both directions
// Returns node IDs that are causally connected to the given node
// This ensures the 14B sees the full cause->effect chain for reasoning
// ============================================================
#define ZETA_CAUSAL_CHAIN_MAX 8

static inline int zeta_follow_causal_chain(
    zeta_dual_ctx_t* ctx,
    int64_t node_id,
    int64_t* chain_ids,
    int max_chain
) {
    if (!ctx || !chain_ids || max_chain <= 0) return 0;
    int count = 0;

    // Follow all CAUSES and PREVENTS edges both directions
    for (int i = 0; i < ctx->num_edges && count < max_chain; i++) {
        zeta_graph_edge_t* e = &ctx->edges[i];

        // Only follow causal edges
        if (e->type != EDGE_CAUSES && e->type != EDGE_PREVENTS) continue;

        int64_t related = -1;
        const char* direction = "";

        if (e->source_id == node_id) {
            // This node CAUSES/PREVENTS something
            related = e->target_id;
            direction = (e->type == EDGE_CAUSES) ? "causes" : "prevents";
        } else if (e->target_id == node_id) {
            // Something CAUSES/PREVENTS this node
            related = e->source_id;
            direction = (e->type == EDGE_CAUSES) ? "caused by" : "prevented by";
        }

        if (related >= 0) {
            // Check not duplicate
            bool dup = false;
            for (int j = 0; j < count; j++) {
                if (chain_ids[j] == related) { dup = true; break; }
            }
            if (!dup) {
                chain_ids[count++] = related;
                fprintf(stderr, "[CAUSAL] Node %lld %s %lld\\n",
                        (long long)node_id, direction, (long long)related);
            }
        }
    }

    return count;
}

// Co-surface causally related nodes with the primary node
static inline void zeta_cosurface_causal_chain(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    int64_t primary_node_id
) {
    if (!ctx || !state) return;

    int64_t chain[ZETA_CAUSAL_CHAIN_MAX];
    int num_chain = zeta_follow_causal_chain(ctx, primary_node_id, chain, ZETA_CAUSAL_CHAIN_MAX);

    if (num_chain == 0) return;

    fprintf(stderr, "[CAUSAL] Found %d causally-connected nodes\\n", num_chain);

    for (int c = 0; c < num_chain && state->num_active < ZETA_MAX_ACTIVE_NODES; c++) {
        // Find node in graph
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (ctx->nodes[i].node_id != chain[c]) continue;
            if (!ctx->nodes[i].is_active) continue;

            // Check not already active
            bool already = false;
            for (int a = 0; a < state->num_active; a++) {
                if (state->active[a].node_id == chain[c]) {
                    already = true;
                    break;
                }
            }
            if (already) break;

            // Check budget
            int tokens = (strlen(ctx->nodes[i].label) + strlen(ctx->nodes[i].value) + 20) / 4;
            if (state->total_tokens + tokens > ZETA_TOKEN_BUDGET) {
                fprintf(stderr, "[CAUSAL] Budget exceeded, skipping causal node %s\\n",
                        ctx->nodes[i].label);
                break;
            }

            // Add to active set
            state->active[state->num_active].node_id = chain[c];
            state->active[state->num_active].priority = ctx->nodes[i].salience;
            state->active[state->num_active].tokens_consumed = tokens;
            state->active[state->num_active].served = false;
            state->num_active++;
            state->total_tokens += tokens;

            ctx->nodes[i].last_accessed = (int64_t)time(NULL);

            fprintf(stderr, "[CAUSAL] Co-surfaced: %s (via causal edge, tokens=%d)\\n",
                    ctx->nodes[i].label, tokens);
            break;
        }
    }
}'''

    if old_marker in content:
        content = content.replace(old_marker, new_marker)
        print('Added causal chain functions')
    else:
        print('ERROR: Could not find staging marker')
        print('Looking for zeta_stage_related...')
        if 'zeta_stage_related' in content:
            print('Found zeta_stage_related but pattern mismatch')

# Add call to co-surface after a node is surfaced
if 'zeta_cosurface_causal_chain' in content and 'zeta_cosurface_causal_chain(ctx, state, node->node_id)' not in content:
    old_surface_end = '''    zeta_stage_related(ctx, node->node_id, current_momentum);
    return node;
}'''

    new_surface_end = '''    zeta_stage_related(ctx, node->node_id, current_momentum);

    // Co-surface causally related nodes so 14B can reason through cause->effect
    zeta_cosurface_causal_chain(ctx, state, node->node_id);

    return node;
}'''

    if old_surface_end in content:
        content = content.replace(old_surface_end, new_surface_end)
        print('Added causal co-surfacing call')
    else:
        print('ERROR: Could not find surface end marker')

with open(filepath, 'w') as f:
    f.write(content)

print('\\nCausal chain patch complete!')
print('Features:')
print('  - zeta_follow_causal_chain(): Traverse CAUSES/PREVENTS edges')
print('  - zeta_cosurface_causal_chain(): Auto-surface related causal facts')
print('  - 14B will now see full cause->effect chains for reasoning')
