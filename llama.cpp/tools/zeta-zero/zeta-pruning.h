#ifndef ZETA_PRUNING_H
#define ZETA_PRUNING_H

#include "zeta-dual-process.h"
#include "zeta-graph-manager.h"
#include <vector>
#include <algorithm>
#include <iostream>

// Sleep-Pruning Mechanism
// Optimizes memory graph during low-demand states
class ZetaPruning {
public:
    // Configuration
    float prune_threshold = 0.3f; // Prune edges below this weight
    int max_prune_per_cycle = 100; // Max edges to prune at once

    void sleep_prune(zeta_dual_ctx_t* ctx) {
        if (!ctx) return;

        std::cout << "[ZetaPruning] Starting sleep cycle pruning..." << std::endl;

        // 1. Analyze Connections
        zeta_analyze_edges(ctx);
        
        // 2. Identify candidates
        std::vector<int> prune_candidates;
        for (int i = 0; i < ctx->num_edges; i++) {
            zeta_graph_edge_t* e = &ctx->edges[i];
            
            // Skip protected edges
            if (e->type & ZETA_PROTECTED_EDGE_TYPES) continue;
            
            // Check weight
            if (e->weight < prune_threshold) {
                prune_candidates.push_back(i);
            }
        }

        // 3. Prune
        int pruned_count = 0;
        // Sort candidates descending to remove from end (if we were using vector erase)
        // Here we just swap with last and decrement count
        
        // We need to be careful about invalidating indices if we swap.
        // Better approach: Iterate backwards through candidates.
        std::sort(prune_candidates.rbegin(), prune_candidates.rend());

        for (int idx : prune_candidates) {
            if (pruned_count >= max_prune_per_cycle) break;
            
            // Swap with last valid edge
            if (idx < ctx->num_edges - 1) {
                ctx->edges[idx] = ctx->edges[ctx->num_edges - 1];
            }
            ctx->num_edges--;
            pruned_count++;
        }

        if (pruned_count > 0) {
            std::cout << "[ZetaPruning] Pruned " << pruned_count << " weak connections." << std::endl;
        } else {
            std::cout << "[ZetaPruning] No weak connections found to prune." << std::endl;
        }
        
        // 4. Solidify Important Memories (Boost weight of frequently used or high-weight edges)
        int solidified_count = 0;
        for (int i = 0; i < ctx->num_edges; i++) {
            if (ctx->edges[i].weight > 0.8f) {
                ctx->edges[i].weight = std::min(1.0f, ctx->edges[i].weight + 0.01f); // Reinforce
                solidified_count++;
            }
        }
        if (solidified_count > 0) {
             std::cout << "[ZetaPruning] Solidified " << solidified_count << " strong connections." << std::endl;
        }
    }
};

#endif // ZETA_PRUNING_H
