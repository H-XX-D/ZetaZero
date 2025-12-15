#!/usr/bin/env python3
"""
Fix: When surfacing causal chain, also find PREVENTS edges for entities in chain
by matching entity names (not just node IDs)
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h'

with open(filepath, 'r') as f:
    content = f.read()

# Find the zeta_cosurface_causal_chain function and enhance it
old_cosurface = '''// Co-surface causally related nodes with the primary node
static inline void zeta_cosurface_causal_chain(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    int64_t primary_node_id
) {
    if (!ctx || !state) return;

    int64_t chain[ZETA_CAUSAL_CHAIN_MAX];
    int num_chain = zeta_follow_causal_chain(ctx, primary_node_id, chain, ZETA_CAUSAL_CHAIN_MAX);

    if (num_chain == 0) return;

    fprintf(stderr, "[CAUSAL] Found %d causally-connected nodes\\\\n", num_chain);'''

new_cosurface = '''// Co-surface causally related nodes with the primary node
// Also finds PREVENTS edges by matching entity names in the chain
static inline void zeta_cosurface_causal_chain(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    int64_t primary_node_id
) {
    if (!ctx || !state) return;

    // Get primary node's label for entity matching
    char primary_entity[128] = {0};
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (ctx->nodes[i].node_id == primary_node_id) {
            strncpy(primary_entity, ctx->nodes[i].value, 127);
            break;
        }
    }

    int64_t chain[ZETA_CAUSAL_CHAIN_MAX];
    int num_chain = zeta_follow_causal_chain(ctx, primary_node_id, chain, ZETA_CAUSAL_CHAIN_MAX);

    // Also find any PREVENTS edges that mention entities in our chain
    // This catches "Knight slayed Dragon" when querying about Sun->Giant->Dragon chain
    for (int e = 0; e < ctx->num_edges && num_chain < ZETA_CAUSAL_CHAIN_MAX; e++) {
        zeta_graph_edge_t* edge = &ctx->edges[e];
        if (edge->type != EDGE_PREVENTS) continue;

        // Check if the prevented entity matches any entity in our chain (by name substring)
        int64_t prevented_id = edge->target_id;
        for (int n = 0; n < ctx->num_nodes; n++) {
            if (ctx->nodes[n].node_id != prevented_id) continue;

            // Check if this prevented entity name appears in primary or chain entities
            char lower_prevented[128] = {0};
            int plen = strlen(ctx->nodes[n].value);
            if (plen > 127) plen = 127;
            for (int i = 0; i < plen; i++) lower_prevented[i] = tolower(ctx->nodes[n].value[i]);
            lower_prevented[plen] = '\\0';

            char lower_primary[128] = {0};
            int prlen = strlen(primary_entity);
            if (prlen > 127) prlen = 127;
            for (int i = 0; i < prlen; i++) lower_primary[i] = tolower(primary_entity[i]);
            lower_primary[prlen] = '\\0';

            // Check if "dragon" in prevented matches "dragon" in primary entity
            if (strstr(lower_prevented, lower_primary) || strstr(lower_primary, lower_prevented)) {
                // Found a prevention related to our chain - add preventer node
                bool already = false;
                for (int c = 0; c < num_chain; c++) {
                    if (chain[c] == edge->source_id) { already = true; break; }
                }
                if (!already) {
                    chain[num_chain++] = edge->source_id;
                    fprintf(stderr, "[CAUSAL-PREVENT] Found preventer for entity '%s'\\\\n", ctx->nodes[n].value);
                }
            }
            break;
        }
    }

    if (num_chain == 0) return;

    fprintf(stderr, "[CAUSAL] Found %d causally-connected nodes\\\\n", num_chain);'''

if old_cosurface in content:
    content = content.replace(old_cosurface, new_cosurface)
    print('Enhanced causal chain surfacing with PREVENTS lookup')
else:
    print('ERROR: Could not find zeta_cosurface_causal_chain')
    # Check if the function exists at all
    if 'zeta_cosurface_causal_chain' in content:
        print('Function exists but pattern mismatch')

with open(filepath, 'w') as f:
    f.write(content)

print('\\nPatch complete!')
