# Edge System Improvements for Zeta

## Current Limitations
1. All edges are type 5 (SEMANTIC/RELATED) - no differentiation
2. All weights are 1.0 - no strength variation
3. No temporal decay on edges
4. No confidence scores
5. No bidirectional vs unidirectional distinction
6. Edges only link sequential conversation nodes

---

## Proposed Edge Schema

```c
typedef struct {
    int64_t src;              // Source node ID
    int64_t tgt;              // Target node ID

    // === TYPE (what kind of relationship) ===
    uint8_t type;             // Primary type (see below)
    uint8_t subtype;          // Refinement (e.g., TEMPORAL → before/after/during)

    // === STRENGTH ===
    float weight;             // Base strength [0.0 - 1.0]
    float confidence;         // How certain is this edge? [0.0 - 1.0]

    // === TEMPORAL ===
    uint32_t created_at;      // Unix timestamp
    uint32_t last_activated;  // Last time edge was traversed
    uint16_t activation_count;// How often used
    float decay_rate;         // Per-day decay factor

    // === PROVENANCE ===
    uint8_t source_mode;      // CHAT, CODE, RESEARCH, DREAM, etc.
    uint8_t evidence_type;    // STATED, INFERRED, CONTRADICTED
    int64_t evidence_node;    // Node ID that established this edge

    // === DIRECTIONALITY ===
    uint8_t flags;            // BIDIRECTIONAL, TRANSITIVE, EXCLUSIVE
} zeta_edge_v2_t;
```

---

## Enhanced Edge Types

### Primary Types (8 bits = 256 types)

```c
typedef enum {
    // === HIERARCHICAL (0-15) ===
    EDGE_IS_A          = 0,   // "dog IS_A animal" (taxonomy)
    EDGE_INSTANCE_OF   = 1,   // "Bruno INSTANCE_OF dog" (instantiation)
    EDGE_PART_OF       = 2,   // "wheel PART_OF car" (mereology)
    EDGE_CONTAINS      = 3,   // "car CONTAINS engine" (reverse of PART_OF)

    // === POSSESSION/ATTRIBUTION (16-31) ===
    EDGE_HAS           = 16,  // "user HAS pet"
    EDGE_属性         = 17,  // "Bruno HAS_ATTR age=3"
    EDGE_CREATED_BY    = 18,  // "Zeta CREATED_BY Alex"
    EDGE_OWNED_BY      = 19,  // "car OWNED_BY user"

    // === PREFERENCE/SENTIMENT (32-47) ===
    EDGE_LIKES         = 32,  // Positive preference
    EDGE_DISLIKES      = 33,  // Negative preference
    EDGE_PREFERS       = 34,  // Comparative preference (A over B)
    EDGE_NEUTRAL       = 35,  // Explicitly neutral

    // === CAUSAL (48-63) ===
    EDGE_CAUSES        = 48,  // A causes B
    EDGE_PREVENTS      = 49,  // A prevents B
    EDGE_ENABLES       = 50,  // A enables B (necessary but not sufficient)
    EDGE_REQUIRES      = 51,  // A requires B
    EDGE_CORRELATES    = 52,  // A correlates with B (non-causal)

    // === TEMPORAL (64-79) ===
    EDGE_BEFORE        = 64,  // A happened before B
    EDGE_AFTER         = 65,  // A happened after B
    EDGE_DURING        = 66,  // A happened during B
    EDGE_STARTS        = 67,  // A starts B
    EDGE_ENDS          = 68,  // A ends B
    EDGE_SUPERSEDES    = 69,  // A replaces B (version update)

    // === SEMANTIC (80-95) ===
    EDGE_SIMILAR       = 80,  // Semantically similar
    EDGE_OPPOSITE      = 81,  // Antonym/opposite
    EDGE_SYNONYM       = 82,  // Same meaning
    EDGE_RELATED       = 83,  // Generic semantic relation
    EDGE_CONTEXT       = 84,  // Contextual association

    // === LOGICAL (96-111) ===
    EDGE_IMPLIES       = 96,  // A implies B
    EDGE_CONTRADICTS   = 97,  // A contradicts B
    EDGE_SUPPORTS      = 98,  // A supports/evidence for B
    EDGE_REFUTES       = 99,  // A refutes B
    EDGE_UNCERTAIN     = 100, // A uncertain about B

    // === CODE/TECHNICAL (112-127) ===
    EDGE_CALLS         = 112, // Function A calls B
    EDGE_IMPLEMENTS    = 113, // A implements interface B
    EDGE_EXTENDS       = 114, // A extends/inherits B
    EDGE_DEPENDS_ON    = 115, // A depends on B
    EDGE_DEFINES       = 116, // A defines B

    // === CONVERSATIONAL (128-143) ===
    EDGE_MENTIONED_WITH = 128, // Co-occurred in same turn
    EDGE_FOLLOWED_BY   = 129, // Conversation flow
    EDGE_ANSWERED_BY   = 130, // Question → Answer
    EDGE_CLARIFIES     = 131, // Clarification
    EDGE_SUMMARIZES    = 132, // Summary relationship

    // === META (240-255) ===
    EDGE_DERIVED       = 240, // Computed/inferred edge
    EDGE_USER_DEFINED  = 241, // Explicit user annotation
    EDGE_SYSTEM        = 242, // System-generated
} zeta_edge_type_v2_t;
```

---

## Edge Extraction Improvements

### 1. LLM-Assisted Edge Typing

When creating edges, use the 7B model to classify:

```
Given: "Bruno is my 3-year-old dog who loves playing fetch"

Extract edges:
- user HAS Bruno (EDGE_HAS, confidence=0.95)
- Bruno IS_A dog (EDGE_IS_A, confidence=0.99)
- Bruno HAS_ATTR age=3 (EDGE_ATTRIBUTE, confidence=0.90)
- Bruno LIKES fetch (EDGE_LIKES, confidence=0.85)
```

### 2. Weight Calculation

```c
float calculate_edge_weight(
    float semantic_similarity,    // Embedding cosine similarity
    float co_occurrence_freq,     // How often mentioned together
    float recency_boost,          // Recent = higher weight
    float explicit_strength       // "really", "always" = boost
) {
    return 0.3 * semantic_similarity
         + 0.2 * co_occurrence_freq
         + 0.2 * recency_boost
         + 0.3 * explicit_strength;
}
```

### 3. Confidence Scoring

```c
typedef enum {
    EVIDENCE_STATED     = 0,  // User explicitly said it (conf=0.95)
    EVIDENCE_INFERRED   = 1,  // LLM inferred it (conf=0.70)
    EVIDENCE_RETRIEVED  = 2,  // From external source (conf=0.80)
    EVIDENCE_ASSUMED    = 3,  // Default assumption (conf=0.50)
    EVIDENCE_CONTESTED  = 4,  // Contradictory info (conf=0.30)
} evidence_type_t;
```

### 4. Temporal Decay

```c
float get_current_weight(zeta_edge_v2_t* edge, time_t now) {
    float days_since_activation = (now - edge->last_activated) / 86400.0f;
    float decay = powf(edge->decay_rate, days_since_activation);
    float activation_boost = logf(1 + edge->activation_count) / 10.0f;
    return edge->weight * decay * (1.0f + activation_boost);
}
```

---

## Query Improvements

### Path-Aware Retrieval

```c
// Find all causal chains from A
vector<path_t> find_causal_paths(node_id src, int max_depth) {
    return bfs_typed(src, {EDGE_CAUSES, EDGE_ENABLES, EDGE_REQUIRES}, max_depth);
}

// Find contradictions
vector<edge_t> find_contradictions(node_id node) {
    return get_edges_by_type(node, EDGE_CONTRADICTS);
}

// Temporal ordering
vector<node_t> get_timeline(node_id event) {
    return topological_sort(event, {EDGE_BEFORE, EDGE_AFTER, EDGE_DURING});
}
```

---

## Migration Path

1. **Phase 1**: Add confidence + timestamp fields to existing edges
2. **Phase 2**: Implement LLM edge classification on new edges
3. **Phase 3**: Batch reclassify existing SEMANTIC edges
4. **Phase 4**: Add temporal decay to retrieval scoring

---

## Storage Impact

Current: 16 bytes/edge (src + tgt + type + weight)
Proposed: 48 bytes/edge

For 10K edges: 160KB → 480KB (acceptable)

---

## Example: Improved Graph for "Bruno"

**Current:**
```
[pet_name:Bruno] --SEMANTIC--> [age:3] --SEMANTIC--> [favorite_lang:Rust]
```

**Improved:**
```
[entity:user] --HAS(0.95)--> [entity:Bruno]
[entity:Bruno] --IS_A(0.99)--> [concept:dog]
[entity:Bruno] --HAS_ATTR(0.90)--> [fact:age=3]
[entity:Bruno] --LIKES(0.85)--> [activity:fetch]
[entity:user] --LIKES(0.93)--> [language:Rust]
```

Now queries like "What pets does the user have?" or "What does Bruno like?" work correctly.
