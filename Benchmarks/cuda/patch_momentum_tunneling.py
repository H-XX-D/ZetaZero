#!/usr/bin/env python3
"""
Implement momentum-guided tunneling with query relevance matching.
- High momentum → narrow tunnel (only highly relevant facts)
- Low momentum → wide tunnel (exploratory surfacing)
- Query keyword matching boosts priority of relevant facts
"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'zeta_query_relevance' in content:
    print("Already patched with query relevance!")
    sys.exit(0)

# Add query relevance scoring function after the includes
insert_after = '#define ZETA_EVICTION_THRESHOLD 0.1f  // Lowered to allow new nodes  // Below this momentum = immediate evict'

query_relevance_code = '''#define ZETA_EVICTION_THRESHOLD 0.1f  // Lowered to allow new nodes  // Below this momentum = immediate evict

// Momentum-guided tunnel width:
// - momentum >= 0.8 → NARROW: only top 30% priority facts (threshold = 0.7)
// - momentum 0.4-0.8 → MODERATE: top 60% facts (threshold = 0.4)
// - momentum < 0.4 → WIDE: exploratory, most facts pass (threshold = 0.15)
static inline float zeta_momentum_threshold(float momentum) {
    if (momentum >= 0.8f) return 0.7f;      // Narrow tunnel
    if (momentum >= 0.4f) return 0.4f;      // Moderate tunnel
    return 0.15f;                            // Wide tunnel (exploratory)
}

// Query relevance scoring: boost facts that match query keywords
// Returns multiplier 1.0 (no match) to 2.5 (strong match)
static inline float zeta_query_relevance(const char* query, const char* fact_value, const char* fact_label) {
    if (!query || !fact_value || strlen(query) < 3) return 1.0f;

    float score = 1.0f;

    // Extract keywords from query (skip common words)
    char query_lower[512];
    int qlen = strlen(query);
    if (qlen >= sizeof(query_lower)) qlen = sizeof(query_lower) - 1;
    for (int i = 0; i < qlen; i++) {
        query_lower[i] = (query[i] >= 'A' && query[i] <= 'Z') ? query[i] + 32 : query[i];
    }
    query_lower[qlen] = '\\0';

    char fact_lower[512];
    int flen = strlen(fact_value);
    if (flen >= sizeof(fact_lower)) flen = sizeof(fact_lower) - 1;
    for (int i = 0; i < flen; i++) {
        fact_lower[i] = (fact_value[i] >= 'A' && fact_value[i] <= 'Z') ? fact_value[i] + 32 : fact_value[i];
    }
    fact_lower[flen] = '\\0';

    // Direct substring match is strong signal
    if (strstr(fact_lower, query_lower)) {
        score += 1.5f;  // Strong boost for direct match
    }

    // Check individual keywords (3+ chars, skip common words)
    const char* skip_words[] = {"the", "is", "are", "was", "were", "what", "where", "when",
                                 "who", "how", "why", "can", "could", "would", "should",
                                 "do", "does", "did", "have", "has", "had", "will",
                                 "my", "your", "his", "her", "its", "our", "their", "and", "or"};
    int num_skip = sizeof(skip_words) / sizeof(skip_words[0]);

    char* token = strtok(query_lower, " .,?!");
    while (token) {
        if (strlen(token) >= 3) {
            // Skip common words
            bool skip = false;
            for (int i = 0; i < num_skip; i++) {
                if (strcmp(token, skip_words[i]) == 0) {
                    skip = true;
                    break;
                }
            }

            if (!skip) {
                // Check if keyword appears in fact
                if (strstr(fact_lower, token)) {
                    score += 0.3f;  // Boost per keyword match
                }
                // Check label too
                if (fact_label && strstr(fact_label, token)) {
                    score += 0.5f;  // Label match is stronger signal
                }
            }
        }
        token = strtok(NULL, " .,?!");
    }

    // Cap at 2.5x boost
    if (score > 2.5f) score = 2.5f;
    return score;
}'''

if insert_after in content:
    content = content.replace(insert_after, query_relevance_code)
    print("Added query relevance and momentum threshold functions")
else:
    print("Could not find insertion point for query relevance!")
    sys.exit(1)

# Now update the priority calculation to use query relevance and momentum threshold
old_priority_calc = '''// Calculate priority score: recency-weighted salience with momentum boost
static inline float zeta_calc_priority(zeta_graph_node_t* node, float momentum) {
    if (!node || !node->is_active) return 0.0f;

    time_t now = time(NULL);
    float age_hours = (float)(now - node->last_accessed) / 3600.0f;

    // Exponential decay: half-life of 2 hours (not 5 minutes!)
    float recency = expf(-0.35f * age_hours);

    // Priority = salience * recency * momentum^2 (momentum has outsized effect)
    // Priority = salience * recency + momentum boost (instead of multiplication)
    // This prevents low momentum from killing high-salience nodes
    return (node->salience * recency * 0.7f) + (momentum * 0.3f);
}'''

new_priority_calc = '''// Calculate priority score: recency-weighted salience with query relevance boost
// NOTE: Query relevance is applied separately in surface_one for tunneling
static inline float zeta_calc_priority_base(zeta_graph_node_t* node, float momentum) {
    if (!node || !node->is_active) return 0.0f;

    time_t now = time(NULL);
    float age_hours = (float)(now - node->last_accessed) / 3600.0f;

    // Exponential decay: half-life of 2 hours
    float recency = expf(-0.35f * age_hours);

    // Base priority without query boosting
    return (node->salience * recency * 0.7f) + (momentum * 0.3f);
}

// Full priority with query relevance boost
static inline float zeta_calc_priority(zeta_graph_node_t* node, float momentum, const char* query) {
    float base = zeta_calc_priority_base(node, momentum);
    if (base <= 0.0f) return 0.0f;

    // Apply query relevance multiplier (1.0 to 2.5x)
    float relevance = zeta_query_relevance(query, node->value, node->label);
    return base * relevance;
}'''

if old_priority_calc in content:
    content = content.replace(old_priority_calc, new_priority_calc)
    print("Updated priority calculation with query relevance")
else:
    print("Could not find priority calculation to update!")

# Update the surface_one function to use dynamic threshold
old_surface_priority = '''        float priority = zeta_calc_priority(node, current_momentum);

        // Apply eviction threshold
        if (priority < ZETA_EVICTION_THRESHOLD) continue;'''

new_surface_priority = '''        float priority = zeta_calc_priority(node, current_momentum, query);

        // MOMENTUM-GUIDED TUNNELING:
        // High momentum = narrow tunnel (higher threshold)
        // Low momentum = wide tunnel (lower threshold, exploratory)
        float dynamic_threshold = zeta_momentum_threshold(current_momentum);

        // Apply the dynamic threshold
        if (priority < dynamic_threshold) {
            // Below threshold, but log if close
            if (priority > dynamic_threshold * 0.7f) {
                fprintf(stderr, "[TUNNEL] Near-miss: %s (priority=%.2f, threshold=%.2f, momentum=%.2f)\\n",
                        node->label, priority, dynamic_threshold, current_momentum);
            }
            continue;
        }'''

if old_surface_priority in content:
    content = content.replace(old_surface_priority, new_surface_priority)
    print("Updated surface_one with momentum-guided tunneling")
else:
    print("Could not find surface_one priority check to update!")

# Add tunnel diagnostics at the start of surface_one
old_surface_start = '''// Surface TOP node that fits in budget
static inline zeta_graph_node_t* zeta_stream_surface_one(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    const char* query,
    float current_momentum
) {
    if (!ctx || !state) return NULL;'''

new_surface_start = '''// Surface TOP node that fits in budget - MOMENTUM-GUIDED TUNNELING
// High momentum = narrow (focused), Low momentum = wide (exploratory)
static inline zeta_graph_node_t* zeta_stream_surface_one(
    zeta_dual_ctx_t* ctx,
    zeta_stream_state_t* state,
    const char* query,
    float current_momentum
) {
    if (!ctx || !state) return NULL;

    // Log tunnel width for this query
    float threshold = zeta_momentum_threshold(current_momentum);
    const char* tunnel_width = (threshold >= 0.7f) ? "NARROW" :
                               (threshold >= 0.4f) ? "MODERATE" : "WIDE";
    fprintf(stderr, "[TUNNEL] Mode: %s (momentum=%.2f, threshold=%.2f, query=%.30s...)\\n",
            tunnel_width, current_momentum, threshold, query ? query : "(none)");'''

if old_surface_start in content:
    content = content.replace(old_surface_start, new_surface_start)
    print("Added tunnel diagnostics")
else:
    print("Could not find surface_one start to add diagnostics!")

with open(filepath, 'w') as f:
    f.write(content)

print("Done! Momentum-guided tunneling implemented.")
print("  - High momentum (>=0.8): NARROW tunnel, threshold=0.7")
print("  - Medium momentum (0.4-0.8): MODERATE tunnel, threshold=0.4")
print("  - Low momentum (<0.4): WIDE tunnel, threshold=0.15")
print("  - Query keywords boost matching facts by up to 2.5x")
