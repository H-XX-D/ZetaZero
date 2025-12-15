#!/usr/bin/env python3
"""
Robust fix: When surfacing, boost facts that mention entities in the query's causal chain.
If query mentions "Sun" and Sun is caused by Dragon->Giant->Sun, also boost facts about Dragon.
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h'

with open(filepath, 'r') as f:
    content = f.read()

# Add causal chain entity extraction for query boosting
# Insert after the zeta_follow_causal_chain function

new_function = '''
// Get all entity names in a node's causal chain (for query boosting)
static inline int zeta_get_chain_entities(
    zeta_dual_ctx_t* ctx,
    const char* query,
    char chain_entities[8][64]  // Up to 8 entity names, 64 chars each
) {
    if (!ctx || !query) return 0;
    int num_entities = 0;

    // First find nodes that match the query
    char lower_q[256];
    int qlen = strlen(query);
    if (qlen > 255) qlen = 255;
    for (int i = 0; i < qlen; i++) lower_q[i] = tolower(query[i]);
    lower_q[qlen] = '\\0';

    // Find all nodes mentioned in query
    for (int n = 0; n < ctx->num_nodes && num_entities < 4; n++) {
        char lower_v[128];
        int vlen = strlen(ctx->nodes[n].value);
        if (vlen > 127) vlen = 127;
        for (int i = 0; i < vlen; i++) lower_v[i] = tolower(ctx->nodes[n].value[i]);
        lower_v[vlen] = '\\0';

        // If query mentions this node's value
        if (strstr(lower_q, lower_v) || strstr(lower_v, lower_q)) {
            // Follow causal chain from this node
            int64_t chain[8];
            int chain_len = zeta_follow_causal_chain(ctx, ctx->nodes[n].node_id, chain, 8);

            for (int c = 0; c < chain_len && num_entities < 8; c++) {
                // Get entity name from chain node
                for (int cn = 0; cn < ctx->num_nodes; cn++) {
                    if (ctx->nodes[cn].node_id == chain[c]) {
                        strncpy(chain_entities[num_entities], ctx->nodes[cn].value, 63);
                        chain_entities[num_entities][63] = '\\0';
                        num_entities++;
                        break;
                    }
                }
            }
        }
    }

    return num_entities;
}

// Boost factor for facts mentioning entities in the query's causal chain
static inline float zeta_causal_chain_boost(
    zeta_dual_ctx_t* ctx,
    const char* query,
    const char* node_value
) {
    if (!ctx || !query || !node_value) return 1.0f;

    static char cached_query[256] = {0};
    static char cached_entities[8][64];
    static int cached_num = 0;

    // Cache chain entities for the same query
    if (strncmp(query, cached_query, 255) != 0) {
        strncpy(cached_query, query, 255);
        cached_num = zeta_get_chain_entities(ctx, query, cached_entities);
    }

    if (cached_num == 0) return 1.0f;

    // Check if node_value mentions any chain entity
    char lower_v[512];
    int vlen = strlen(node_value);
    if (vlen > 511) vlen = 511;
    for (int i = 0; i < vlen; i++) lower_v[i] = tolower(node_value[i]);
    lower_v[vlen] = '\\0';

    for (int e = 0; e < cached_num; e++) {
        char lower_e[64];
        int elen = strlen(cached_entities[e]);
        if (elen > 63) elen = 63;
        for (int i = 0; i < elen; i++) lower_e[i] = tolower(cached_entities[e][i]);
        lower_e[elen] = '\\0';

        if (strstr(lower_v, lower_e)) {
            fprintf(stderr, "[CHAIN-BOOST] Boosting fact with '%s' (in causal chain)\\n", cached_entities[e]);
            return 2.0f;  // 2x boost for facts in causal chain
        }
    }

    return 1.0f;
}

'''

# Insert after zeta_cosurface_causal_chain function ends
marker = "    return node;\n}"
if marker in content:
    # Find the end of zeta_stream_surface_one function
    idx = content.find(marker)
    if idx > 0:
        # Find the next function or end
        next_func = content.find("static inline", idx + len(marker))
        if next_func > 0:
            content = content[:next_func] + new_function + content[next_func:]
            print("Added causal chain boost functions")

# Now update the priority calculation to include chain boost
old_priority = "float priority = zeta_calc_priority(node, current_momentum) * zeta_query_relevance(query, node->value, node->label, node->last_accessed) * zeta_staging_boost(node->node_id) * zeta_unique_match_boost(query, node->value, node->label);"

new_priority = "float priority = zeta_calc_priority(node, current_momentum) * zeta_query_relevance(query, node->value, node->label, node->last_accessed) * zeta_staging_boost(node->node_id) * zeta_unique_match_boost(query, node->value, node->label) * zeta_causal_chain_boost(ctx, query, node->value);"

if old_priority in content:
    content = content.replace(old_priority, new_priority)
    print("Added chain boost to priority calculation")
else:
    print("Could not find priority calculation - may need manual update")

with open(filepath, 'w') as f:
    f.write(content)

print("\\nPatch complete!")
