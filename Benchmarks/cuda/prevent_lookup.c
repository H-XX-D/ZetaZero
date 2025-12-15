
    // Also find any PREVENTS edges that mention entities in our chain
    // This catches "Knight slayed Dragon" when querying about Sun->Giant->Dragon chain
    for (int pe = 0; pe < ctx->num_edges && num_chain < ZETA_CAUSAL_CHAIN_MAX; pe++) {
        zeta_graph_edge_t* pedge = &ctx->edges[pe];
        if (pedge->type != EDGE_PREVENTS) continue;

        // Get the prevented entity's value
        char prevented_val[128] = {0};
        for (int pn = 0; pn < ctx->num_nodes; pn++) {
            if (ctx->nodes[pn].node_id == pedge->target_id) {
                strncpy(prevented_val, ctx->nodes[pn].value, 127);
                break;
            }
        }

        // Check if prevented entity relates to any chain node by name
        for (int cn = 0; cn < num_chain && prevented_val[0]; cn++) {
            for (int cni = 0; cni < ctx->num_nodes; cni++) {
                if (ctx->nodes[cni].node_id != chain[cn]) continue;

                // Case-insensitive substring match
                char lower_p[128], lower_c[128];
                int plen = strlen(prevented_val);
                int clen = strlen(ctx->nodes[cni].value);
                if (plen > 127) plen = 127;
                if (clen > 127) clen = 127;
                for (int x = 0; x < plen; x++) lower_p[x] = tolower(prevented_val[x]);
                lower_p[plen] = '\0';
                for (int x = 0; x < clen; x++) lower_c[x] = tolower(ctx->nodes[cni].value[x]);
                lower_c[clen] = '\0';

                if (strstr(lower_p, lower_c) || strstr(lower_c, lower_p)) {
                    // Found prevention related to chain - add preventer
                    bool dup = false;
                    for (int d = 0; d < num_chain; d++) {
                        if (chain[d] == pedge->source_id) { dup = true; break; }
                    }
                    if (!dup) {
                        chain[num_chain++] = pedge->source_id;
                        fprintf(stderr, "[CAUSAL-PREVENT] Added preventer for '%s'\n", prevented_val);
                    }
                }
                break;
            }
        }
    }

