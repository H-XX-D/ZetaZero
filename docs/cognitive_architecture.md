# Zeta Cognitive Architecture: The Pac-Man Model

## Mental Model

The cognitive construct operates like an 8-bit Pac-Man game:

| Game Element | Cognitive Equivalent | Function |
|--------------|---------------------|----------|
| **Pac-Man** | Transformer/Query | Traverses the graph seeking knowledge |
| **Dots** | Nodes | Basic knowledge units to visit/collect |
| **Fruit** | High-salience nodes | Recent, relevant, high-momentum targets |
| **Ghosts** | Negative signals | Contradictions, low-confidence, avoid |
| **Walls** | Edges (structure) | Constrain valid traversal paths |
| **Openings** | Edge connections | Enable movement between nodes |
| **Tunnels** | Operations | Teleport to distant graph regions |
| **Maze** | Graph structure | The knowledge space |
| **Clear map** | Answer query | Successfully resolved the question |

---

## 4-Layer Graph Structure

```
┌─────────────────────────────────────────────────────────────────────┐
│                        LAYER 4: OPERATIONS GRAPH                     │
│   Contains reasoning chains and logic paths between nodes            │
│   "How did we get from A to B? What operations were applied?"        │
├─────────────────────────────────────────────────────────────────────┤
│                        LAYER 3: BRANCH GRAPH                         │
│   Evolving associations: node_id ←→ node_id with ternary belief     │
│   "What connects to what? How strongly? Positive or negative?"       │
├─────────────────────────────────────────────────────────────────────┤
│                        LAYER 2: GRAPH-KV CACHE                       │
│   KV states extracted when each node was created                     │
│   "What was the transformer's internal state at this node?"          │
├─────────────────────────────────────────────────────────────────────┤
│                        LAYER 1: TOKENIZED NODE GRAPH                 │
│   Nodes with: ID, Version, Timestamp, Content, Embedding            │
│   "What facts/entities/concepts exist?"                              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Layer 1: Tokenized Node Graph

The foundation - raw knowledge units.

```c
typedef struct {
    int64_t id;              // Unique identifier
    uint32_t version;        // For updates/supersession
    uint32_t timestamp;      // When created

    char label[64];          // Node type: "entity", "fact", "event"
    char value[512];         // Content: "Bruno is a 3-year-old dog"

    float embedding[256];    // Semantic vector
    float salience;          // Current importance (decays)
    float momentum;          // Rate of change (active = high)

    int64_t kv_segment_id;   // Link to Layer 2
} zeta_node_t;
```

**Pac-Man analog**: These are the **dots** - each one a piece of knowledge to potentially collect.

---

## Layer 2: Graph-KV Cache

Transformer internal state snapshots.

```c
typedef struct {
    int64_t node_id;         // Which node this KV belongs to
    int64_t segment_id;      // Unique segment identifier

    uint32_t num_tokens;     // Tokens in this segment
    uint32_t num_layers;     // Transformer layers captured

    // Quantized KV cache (Q8_0 format)
    uint8_t* k_cache;        // Key states
    uint8_t* v_cache;        // Value states

    uint32_t created_at;     // When captured
    uint32_t last_injected;  // Last time used in generation
} zeta_kv_segment_t;
```

**Pac-Man analog**: The **power state** - when Pac-Man eats this node, he inherits the transformer's learned context from when it was created.

---

## Layer 3: Branch Graph (Ternary Edges)

Dynamic associations between nodes.

```c
typedef struct {
    int64_t src_id;          // From node
    int64_t tgt_id;          // To node

    float belief;            // -1.0 to +1.0 (ternary)
                             //  +1 = strong positive connection
                             //   0 = uncertain/neutral
                             //  -1 = strong negative (avoid!)

    uint8_t edge_type;       // IS_A, HAS, CAUSES, CONTRADICTS...
    uint32_t last_traversed; // Recency
    uint16_t traverse_count; // Frequency (momentum)
} zeta_branch_edge_t;
```

**Pac-Man analog**:
- **Walls** (belief < -0.5) = Can't go this way, blocked
- **Openings** (belief > 0.3) = Valid path, can traverse
- **Ghosts** (belief < -0.7) = Danger! Contradiction/wrong path

---

## Layer 4: Operations Graph

Reasoning chains and transformations.

```c
typedef enum {
    OP_RETRIEVE,      // Fetch node by similarity
    OP_TRAVERSE,      // Follow edge to adjacent node
    OP_INFER,         // Generate new knowledge from existing
    OP_CONTRADICT,    // Detected conflict, backtrack
    OP_TELEPORT,      // Jump to distant related node (tunnel!)
    OP_MERGE,         // Combine multiple paths
    OP_PRUNE,         // Abandon low-value branch
    OP_ANSWER,        // Terminal: emit response
} zeta_op_type_t;

typedef struct {
    int64_t op_id;           // Unique operation ID
    zeta_op_type_t type;     // What operation

    int64_t from_node;       // Starting point
    int64_t to_node;         // Ending point (or -1 if terminal)

    float confidence;        // How sure about this step
    char reasoning[256];     // Why this operation was chosen

    int64_t parent_op;       // Previous operation (chain)
    uint32_t timestamp;      // When executed
} zeta_operation_t;
```

**Pac-Man analog**: The **game replay** - records every move made, every tunnel taken, every ghost avoided. Can replay to understand HOW the query was answered.

---

## Query Execution: "Clearing the Map"

```
START: User query arrives
  │
  ▼
┌─────────────────────────────────────┐
│  1. EMBED QUERY                     │  Convert query to vector
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│  2. FIND ENTRY POINTS               │  Top-K similar nodes (starting dots)
│     └─ Layer 1: Node embeddings     │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│  3. INJECT KV CONTEXT               │  Load transformer state
│     └─ Layer 2: KV segments         │  (Pac-Man powers up)
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│  4. TRAVERSE GRAPH                  │  Follow edges, collect nodes
│     └─ Layer 3: Branch edges        │
│                                     │
│     WHILE map not cleared:          │
│       - Move toward high-salience   │  (chase fruit)
│       - Follow positive edges       │  (through openings)
│       - Avoid negative edges        │  (walls block)
│       - Flee contradictions         │  (ghosts chase)
│       - Use operations to teleport  │  (tunnels)
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│  5. RECORD OPERATIONS               │  Log reasoning path
│     └─ Layer 4: Operations graph    │  (game replay)
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│  6. SYNTHESIZE ANSWER               │  Combine collected nodes
│     └─ "Map cleared" = sufficient   │
│         knowledge gathered          │
└─────────────────────────────────────┘
  │
  ▼
END: Response emitted
```

---

## Scoring: What Makes a "Fruit" Node?

```c
float compute_node_priority(zeta_node_t* node, query_context_t* ctx) {
    float salience = node->salience;           // Base importance
    float relevance = cosine_sim(node->embedding, ctx->query_embedding);
    float recency = time_decay(node->timestamp, ctx->now, HALF_LIFE_DAYS);
    float momentum = node->momentum;           // Activity level

    // Fruit score = all factors combined
    return salience * 0.3f
         + relevance * 0.4f
         + recency * 0.2f
         + momentum * 0.1f;
}

// Priority levels:
//   > 0.8 = FRUIT (high-value target, chase it!)
//   > 0.5 = ENERGIZER (worth pursuing)
//   > 0.2 = DOT (collect if on path)
//   < 0.2 = SKIP (not worth the detour)
```

---

## Ghost Avoidance: Negative Signals

```c
bool should_avoid_edge(zeta_branch_edge_t* edge) {
    // Strong negative belief = GHOST
    if (edge->belief < -0.7f) return true;

    // Contradiction edge type = GHOST
    if (edge->edge_type == EDGE_CONTRADICTS) return true;

    // Recently traversed and led to dead end = GHOST
    if (edge->traverse_count > 3 && edge->last_score < 0.1f) return true;

    return false;
}

// When ghost encountered:
//   1. Record contradiction in operations graph
//   2. Backtrack to last fork
//   3. Try alternative path
//   4. If no alternatives, report uncertainty (belief → 0)
```

---

## Tunnels: Operation Teleportation

```c
// When stuck or need distant knowledge:
int64_t find_tunnel_exit(zeta_node_t* current, query_context_t* ctx) {
    // Option 1: Semantic jump (similar but distant)
    int64_t semantic_exit = find_similar_unvisited(current, ctx);

    // Option 2: Causal chain (follow CAUSES edges far)
    int64_t causal_exit = follow_causal_chain(current, MAX_HOPS);

    // Option 3: Temporal jump (same entity, different time)
    int64_t temporal_exit = find_temporal_sibling(current);

    // Record operation
    log_operation(OP_TELEPORT, current->id, best_exit, "tunnel");

    return best_exit;
}
```

---

## Map Cleared: Query Completion

```c
typedef struct {
    int visited_count;           // Nodes collected
    int fruit_count;             // High-value nodes found
    float coverage;              // % of relevant nodes visited
    float confidence;            // Overall answer confidence
    int ghost_encounters;        // Contradictions hit
    int tunnel_uses;             // Operations invoked
} query_completion_t;

bool is_map_cleared(query_completion_t* status) {
    // Enough high-value nodes collected
    if (status->fruit_count >= 3 && status->confidence > 0.7f) return true;

    // High coverage of relevant space
    if (status->coverage > 0.8f) return true;

    // Exhausted search with best effort
    if (status->visited_count > MAX_VISITS) return true;

    return false;
}
```

---

## Visual: Complete Query Traversal

```
Query: "What is Bruno?"

Layer 1 (Nodes):          Layer 3 (Edges):         Layer 4 (Operations):

[user]─────────●[Bruno]   belief: +0.95 (HAS)     OP1: RETRIEVE Bruno
                │                                  OP2: TRAVERSE user→Bruno
                │ +0.99                            OP3: TRAVERSE Bruno→dog
                ▼         belief: +0.99 (IS_A)    OP4: TELEPORT to "pet" cluster
●[dog]◄────────┘                                  OP5: ANSWER synthesize
    │
    │ +0.85               belief: +0.85           Layer 2 (KV):
    ▼                     (HAS_ATTR)
[age:3]                                           KV_23: Bruno context
                                                  KV_45: dog taxonomy
👻[cat] ✗                 belief: -0.90           KV_12: user preferences
                          (IS_A) BLOCKED!

Answer: "Bruno is your 3-year-old dog"
Confidence: +0.92
Operations: 5
Ghosts avoided: 1 (cat)
```

---

## Summary

| Layer | Contains | Pac-Man Role |
|-------|----------|--------------|
| **1. Tokenized Nodes** | Facts, entities, concepts | Dots & Fruit |
| **2. Graph-KV** | Transformer states | Power-ups |
| **3. Branch Graph** | Ternary edges (-1 to +1) | Walls & Openings & Ghosts |
| **4. Operations** | Reasoning chains | Game replay & Tunnels |

**Goal**: Transformer (Pac-Man) navigates the maze (graph), collecting relevant knowledge (dots), prioritizing high-value nodes (fruit), avoiding contradictions (ghosts), using operations to teleport (tunnels), until the query is answered (map cleared).
