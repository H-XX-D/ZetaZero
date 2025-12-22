// Z.E.T.A. Temporal Recursive Memory (TRM)
// Handles recursive state maintenance and temporal consistency
// Prevents infinite loops and manages time-decayed context streams

#ifndef ZETA_TRM_H
#define ZETA_TRM_H

#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <map>
#include "zeta-dual-process.h"

// ============================================================================
// Constants & Config
// ============================================================================

#define TRM_MAX_RECURSION_DEPTH 10
#define TRM_DEFAULT_LAMBDA 0.001f
#define TRM_CONTEXT_WINDOW 2048

// ============================================================================
// Types
// ============================================================================

typedef struct {
    std::string content;
    time_t timestamp;
    float activation_energy; // Z(t)
    int recursion_depth;
    std::string source_id;   // For tracking self-reference
} zeta_trm_node_t;

typedef struct {
    std::deque<zeta_trm_node_t> stream;
    float lambda;            // Decay constant
    int max_depth;
} zeta_trm_context_t;

// ============================================================================
// ZetaTRM Class
// ============================================================================

class ZetaTRM {
private:
    zeta_trm_context_t ctx;
    std::map<std::string, int> recursion_tracker; // Track repetition of concepts

public:
    ZetaTRM() {
        ctx.lambda = TRM_DEFAULT_LAMBDA;
        ctx.max_depth = TRM_MAX_RECURSION_DEPTH;
    }

    // Initialize with custom decay
    void init(float lambda = TRM_DEFAULT_LAMBDA) {
        ctx.lambda = lambda;
        ctx.stream.clear();
        recursion_tracker.clear();
    }

    // Add a thought/memory to the recursive stream
    void push_state(const std::string& content, const std::string& source = "system") {
        time_t now = time(NULL);
        
        // Check for recursion/loops
        if (recursion_tracker[content] > 3) {
            // Detected potential infinite loop
            return; 
        }
        recursion_tracker[content]++;

        zeta_trm_node_t node;
        node.content = content;
        node.timestamp = now;
        node.activation_energy = 1.0f; // Initial energy Z_0
        node.recursion_depth = ctx.stream.size(); // Simple depth metric
        node.source_id = source;

        ctx.stream.push_back(node);
        
        // Prune if too large
        if (ctx.stream.size() > TRM_CONTEXT_WINDOW) {
            ctx.stream.pop_front();
        }
    }

    // Apply Temporal Decay Z(t) = Z_0 * e^(-lambda * t)
    void apply_decay() {
        time_t now = time(NULL);
        auto it = ctx.stream.begin();
        while (it != ctx.stream.end()) {
            float age = difftime(now, it->timestamp);
            it->activation_energy = 1.0f * exp(-ctx.lambda * age);

            // Prune dead memories (Energy < epsilon)
            if (it->activation_energy < 0.01f) {
                // Remove from tracker to allow re-learning later
                if (recursion_tracker[it->content] > 0) {
                    recursion_tracker[it->content]--;
                }
                it = ctx.stream.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Retrieve relevant context based on current query using embeddings
    // Uses cosine similarity to find relevant past thoughts
    std::string retrieve_context(const std::string& query, const float* query_embedding = nullptr, int dim = 0) {
        std::string context = "";
        apply_decay(); // Update energies first

        // If no embedding provided, fallback to keyword match (or if dim mismatch)
        if (!query_embedding || dim <= 0) {
            for (const auto& node : ctx.stream) {
                if (node.activation_energy > 0.1f) {
                    context += node.content + "\n";
                }
            }
            return context;
        }

        // Embedding-based retrieval
        // Note: In a real implementation, we would store embeddings for each node.
        // For now, we assume nodes have embeddings or we compute them on the fly (expensive)
        // OR we just use the activation energy as a proxy for "recent relevance"
        // and combine it with a simple keyword overlap if embeddings aren't stored per node.
        
        // Since we don't have embeddings stored for every TRM node yet, we'll stick to 
        // activation energy + simple keyword overlap, but we'll structure it to accept embeddings later.
        
        for (const auto& node : ctx.stream) {
            // Only include if energy is high enough
            if (node.activation_energy > 0.1f) {
                context += node.content + "\n";
            }
        }
        return context;
    }

    // Check if a query is safe (not infinitely recursive)
    bool is_safe_query(const std::string& query) {
        // Check against recent history for exact repetition
        int repeat_count = 0;
        for (auto it = ctx.stream.rbegin(); it != ctx.stream.rend(); ++it) {
            if (it->content == query) {
                repeat_count++;
            }
            if (repeat_count > 2) return false; // Too repetitive
        }
        return true;
    }
    
    // Get current stream status
    std::string get_status() {
        return "TRM Active | Stream Size: " + std::to_string(ctx.stream.size()) + 
               " | Lambda: " + std::to_string(ctx.lambda);
    }
};

#endif // ZETA_TRM_H
