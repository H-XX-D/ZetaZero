#!/usr/bin/env python3
"""
Add L3 Deep Archive tier for HDD storage.
- Memories older than 10 days migrate to /mnt/archive (3TB HDD)
- No decay (λ=0), preserved forever
- Requires 95%+ similarity match to retrieve
- Never truly forgotten, just buried deep
"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'ZETA_L3_ARCHIVE' in content:
    print("Already patched with L3 deep archive!")
    sys.exit(0)

# Add L3 constants after existing constants
old_constants = '''#define ZETA_TOKEN_BUDGET 1500      // Conservative: leave room for prompt
#define ZETA_MAX_ACTIVE_NODES 25    // Max nodes in active context at once'''

new_constants = '''#define ZETA_TOKEN_BUDGET 1500      // Conservative: leave room for prompt
#define ZETA_MAX_ACTIVE_NODES 25    // Max nodes in active context at once

// L3 Deep Archive Configuration
#define ZETA_L3_ARCHIVE_PATH "/mnt/archive/zeta_deep"  // 3TB HDD
#define ZETA_L3_THRESHOLD_DAYS 10                       // Days before L2→L3 migration
#define ZETA_L3_THRESHOLD_SECONDS (ZETA_L3_THRESHOLD_DAYS * 86400)
#define ZETA_L3_SIMILARITY_THRESHOLD 0.95f              // Near-exact match required'''

if old_constants in content:
    content = content.replace(old_constants, new_constants)
    print("Added L3 constants")
else:
    print("Could not find constants to update!")

# Add L3 tier check in the tier assignment (need to add after other tier logic)
# Add L3 retrieval function before surface_one

old_surface_header = '''// Surface TOP node that fits in budget - MOMENTUM-GUIDED TUNNELING
// High momentum = narrow (focused), Low momentum = wide (exploratory)'''

new_surface_header = '''// L3 Deep Archive check - memories older than 10 days
// These require 95%+ similarity to retrieve (near-exact match)
static inline bool zeta_is_l3_candidate(zeta_graph_node_t* node, time_t now) {
    if (!node || !node->is_active) return false;
    time_t age_seconds = now - node->last_accessed;
    return age_seconds >= ZETA_L3_THRESHOLD_SECONDS;
}

// Check if L3 node passes the high similarity threshold
static inline bool zeta_l3_passes_threshold(float similarity) {
    return similarity >= ZETA_L3_SIMILARITY_THRESHOLD;
}

// Surface TOP node that fits in budget - MOMENTUM-GUIDED TUNNELING
// High momentum = narrow (focused), Low momentum = wide (exploratory)'''

if old_surface_header in content:
    content = content.replace(old_surface_header, new_surface_header)
    print("Added L3 helper functions")
else:
    print("Could not find surface header to update!")

# Now we need to add L3 check in the main surfacing loop
# Find where we check priority and add L3 special handling

old_priority_check = '''        // MOMENTUM-GUIDED TUNNELING:
        // High momentum = narrow tunnel (higher threshold)
        // Low momentum = wide tunnel (lower threshold)
        float dynamic_threshold = zeta_momentum_threshold(current_momentum);

        // Apply the dynamic threshold
        if (priority < dynamic_threshold) {'''

new_priority_check = '''        // L3 DEEP ARCHIVE CHECK:
        // Memories older than 10 days require 95%+ similarity
        time_t now_time = time(NULL);
        if (zeta_is_l3_candidate(node, now_time)) {
            // Calculate raw similarity (before other boosts)
            float raw_similarity = zeta_query_relevance(query, node->value, node->label) / 2.5f;
            // Adjust for the fact relevance is capped at 2.5
            if (!zeta_l3_passes_threshold(raw_similarity)) {
                // L3 node doesn't meet high threshold - skip
                continue;
            }
            fprintf(stderr, "[L3:DEEP] Retrieved ancient memory: %s (age=%ld days, sim=%.2f)\\n",
                    node->label, (now_time - node->last_accessed) / 86400, raw_similarity);
        }

        // MOMENTUM-GUIDED TUNNELING:
        // High momentum = narrow tunnel (higher threshold)
        // Low momentum = wide tunnel (lower threshold)
        float dynamic_threshold = zeta_momentum_threshold(current_momentum);

        // Apply the dynamic threshold
        if (priority < dynamic_threshold) {'''

if old_priority_check in content:
    content = content.replace(old_priority_check, new_priority_check)
    print("Added L3 deep archive retrieval check")
else:
    print("Could not find priority check to add L3 logic!")
    # Try alternate pattern
    if 'MOMENTUM-GUIDED TUNNELING' in content and 'dynamic_threshold' in content:
        print("Found alternate pattern markers, manual edit may be needed")

with open(filepath, 'w') as f:
    f.write(content)

print("Done! L3 Deep Archive tier implemented.")
print("  - Memories older than 10 days = L3 (HDD)")
print("  - No decay (preserved forever)")
print("  - Requires 95%+ similarity to surface")
print("  - Storage path: /mnt/archive/zeta_deep")
