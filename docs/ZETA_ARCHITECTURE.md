# Z.E.T.A. Architecture
## Zero Entropy Temporal Assimilation

**Version 5.1 | December 2025**

---

## Executive Summary

ZETA is a biologically-inspired cognitive architecture that achieves persistent memory for large language models without fine-tuning. It uses a multi-model system with architectural separation between conscious reasoning and subconscious processing, combined with a cryptographically-bound ethical framework.

**Key Innovation**: Memory and reasoning are handled by separate models, preventing injection attacks from propagating to memory or tool execution while maintaining constant VRAM usage regardless of session length.

---

## 1. Core Architecture: Dual-Process Cognition

ZETA implements a two-system cognitive model inspired by human brain function:

```
┌─────────────────────────────────────────────────────────────────┐
│                        USER QUERY                                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    EMBEDDING MODEL (4B)                          │
│                    "Semantic Hippocampus"                        │
│         Computes query embedding for memory retrieval            │
└─────────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
┌──────────────────────────┐    ┌──────────────────────────┐
│   14B CONSCIOUS LAYER    │    │  7B SUBCONSCIOUS LAYER   │
│       (System 2)         │    │       (System 1)         │
│                          │    │                          │
│  • Slow, deliberate      │    │  • Fast, automatic       │
│  • Reasons on queries    │    │  • Extracts facts        │
│  • Generates responses   │    │  • Builds memory graph   │
│  • READ-ONLY memory      │    │  • WRITE access          │
│  • No tool execution     │    │  • No reasoning          │
└──────────────────────────┘    └──────────────────────────┘
              │                               │
              ▼                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      MEMORY GRAPH (GitGraph)                     │
│              Nodes: Facts, Entities, Events, Relations           │
│              Edges: Semantic connections with weights            │
└─────────────────────────────────────────────────────────────────┘
```

### 1.1 The 14B Conscious Layer (System 2)

- **Role**: Deliberate reasoning and response generation
- **Access**: Read-only to memory graph
- **Cannot**: Write to memory, execute tools, or modify state
- **Receives**: Pre-surfaced relevant context from memory
- **Model**: Qwen 2.5 14B Instruct (Q4)

### 1.2 The 7B Subconscious Layer (System 1)

- **Role**: Automatic fact extraction and memory maintenance
- **Access**: Full write access to memory graph
- **Cannot**: Reason on user prompts (prevents injection propagation)
- **Runs**: In parallel with 14B (cyclic correlation engine)
- **Model**: Qwen 2.5 7B Coder (Q4_K_M)

### 1.3 The 4B Embedding Model (Thalamus)

- **Role**: Semantic routing and similarity computation
- **Function**: Computes embeddings for retrieval
- **Placement**: CPU-bound to preserve GPU VRAM
- **Model**: Qwen3 Embedding 4B (Q4_K_M)

### 1.4 Hierarchical Reasoning Module (HRM)

- **Role**: Complex task decomposition and planning
- **Function**: Breaks down multi-step queries into executable sub-plans
- **Process**: 14B generates plan, 7B executes retrieval/tools, 14B synthesizes
- **Feedback**: Includes a Critic loop for verification

### 1.5 The Conductor (Orchestration Layer)

- **Role**: Central intelligence and task orchestration
- **Function**: Unifies all subsystems (HRM, TRM, Graph, ScratchBuffer)
- **Mechanism**: The 14B Conscious Layer acts as the "Conductor," deciding when to Plan, Act, or Reflect based on the state of the ScratchBuffer and TRM Stream.

---

## 2. Memory System: GitGraph

### 2.1 Three-Tier Storage Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│  TIER 1: VRAM (Hot)     │  momentum >= 0.96  │  Active facts    │
├─────────────────────────────────────────────────────────────────┤
│  TIER 2: RAM (Warm)     │  momentum >= 0.50  │  Recent facts    │
├─────────────────────────────────────────────────────────────────┤
│  TIER 3: NVMe (Cold)    │  momentum <  0.50  │  Archived facts  │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Graph Structure

**Node Types:**
- `NODE_ENTITY`: Person, place, thing
- `NODE_FACT`: Attributes and properties (high salience)
- `NODE_EVENT`: Temporal occurrences
- `NODE_RELATION`: Relationship types

**Edge Types:**
- `EDGE_IS_A`: Type relationship
- `EDGE_HAS`: Possession
- `EDGE_CREATED`: Creator relationship
- `EDGE_LIKES`: Preference
- `EDGE_RELATED`: Generic relation
- `EDGE_SUPERSEDES`: Version update
- `EDGE_TEMPORAL`: Time relationship
- `EDGE_CAUSES`: Causal relationship (A causes B)
- `EDGE_PREVENTS`: Preventive relationship (A prevents B)
- `EDGE_SUPERSEDES`: Version history (old → new)

### 2.3 Version Control (Git-Style Memory)

Facts evolve over time with full version history:

```
[user_age: 25] ──SUPERSEDES──► [user_age: 26] ──SUPERSEDES──► [user_age: 27]
```

- **Concept keys** track versions (e.g., `validation_location:loader.py`)
- **Trust hierarchy**: USER facts (ground truth) > MODEL facts (inferred)
- Old versions remain accessible for context

### 2.4 Temporal Recursive Memory (TRM)

- **Role**: Recursive state maintenance and loop prevention
- **Function**: Maintains a stream of thought with temporal decay
- **Mechanism**: Applies $Z(t) = Z_0 \cdot e^{-\lambda t}$ to active memory streams
- **Safety**: Detects and breaks infinite recursion loops (e.g., self-referential paradoxes)

### 2.5 ScratchBuffer (Working Memory)

- **Role**: Short-term working memory and planning space
- **Function**: Buffers intermediate thoughts, plans, and draft outputs
- **Mechanism**: Allows the model to "think silently" (hidden tokens) before generating the final response.
- **Integration**: Acts as the timeline where the Conductor assembles the final output.

---

## 3. Mathematical Foundations

### 3.1 Temporal Decay (Natural Forgetting)

```
Z(t) = Z₀ · e^(-λt)
```

Memories fade naturally over time:
- `Z₀`: Initial salience
- `λ`: Decay rate (default: 0.1)
- `t`: Time since last access

### 3.2 Entanglement Scoring (Similarity)

```
E(q, s) = ReLU(cos(q, s))³
```

Cubic sharpening emphasizes highly similar memories while dampening marginal matches.

### 3.3 Tunneling Gate (Sparse Retrieval)

```
T(a) = 1 if a > τ, else 0
```

Only memories above similarity threshold τ are retrieved, reducing noise.

### 3.4 Momentum-Based Staging

```
M_eff = momentum × salience
```

Determines tier placement:
- `M_eff >= 0.96` → VRAM
- `M_eff >= 0.50` → RAM
- `M_eff <  0.50` → NVMe

---

## 4. Inference Flow (The Conductor Loop)

```
1. PERCEPTION (Input)
       │
       ▼
   • User Query
   • TRM Stream (Recursive Context)
   • GitGraph (Long-term Facts)
       │
       ▼
2. ORCHESTRATION (The Conductor - 14B)
       │
       ├─► SAFETY CHECK (TRM): Is this a paradox/loop?
       │
       ├─► PLANNING (HRM/ScratchBuffer):
       │     If complex: Initialize ScratchBuffer with <|scratch_start|>
       │     Generate hidden plan
       │
       ▼
3. EXECUTION (The Orchestra - 7B + Tools)
       │
       ├─► 7B Subconscious: Extract facts, execute code
       ├─► Tools: Search, Math, File I/O
       │
       ▼
4. SYNTHESIS (Output)
       │
       ▼
   • Assemble final response in ScratchBuffer
   • Push to TRM Stream (Recursive Update)
   • Persist new facts to GitGraph
```

---

## 5. Security Architecture

### 5.1 Constitutional Lock

The model is cryptographically bound to its ethical framework:

```
CONSTITUTION.txt ──► SHA-256 Hash ──► PRNG Seed ──►
Weight Permutation Indices ──► Weights stored PERMUTED
```

**Result**: Without the correct constitution, weights are shuffled and produce garbage output. The model literally cannot function without its ethics.

### 5.2 Architectural Security (Injection Defense)

```
┌─────────────────────────────────────────────────────────────────┐
│                      ATTACK SURFACE ISOLATION                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  PROMPT INJECTION ──► 14B Layer ──► Response Only               │
│                           │                                      │
│                           ✗ Cannot write to memory               │
│                           ✗ Cannot execute tools                 │
│                           ✗ Cannot modify state                  │
│                                                                  │
│  7B Layer ──► No reasoning on prompts ──► Extraction only       │
│                           │                                      │
│                           ✓ Writes to memory                     │
│                           ✓ But cannot be "convinced" by prompts │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

Attack chain is broken: Even if injection succeeds in 14B reasoning, it cannot propagate to memory writes or tool execution.

### 5.3 Comparison to Traditional Approaches

| Traditional Fine-tuning | Constitutional Lock |
|------------------------|---------------------|
| Safety in weights (can be jailbroken) | Safety is access key (cryptographically required) |
| Model "knows" unsafe content | Model cannot function without ethics |
| Safety can be fine-tuned away | Constitution mathematically inseparable |

---

## 6. Flat VRAM Architecture

ZETA maintains constant GPU memory usage regardless of session length:

```
┌─────────────────────────────────────────────────────────────────┐
│                    GPU VRAM USAGE OVER TIME                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Traditional LLM:                                                │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  (grows)│
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                      │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                    │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                                  │
│                                                                  │
│  ZETA:                                                           │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  (constant)                             │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                          │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                          │
│  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                          │
│                                                                  │
│  ──────────────────────────────────────────────────► Time        │
└─────────────────────────────────────────────────────────────────┘
```

**Mechanism**: Memory graph is external to the model. Only surfaced context enters the context window. Old context is archived to RAM/NVMe, not accumulated in VRAM.

---

## 7. Brain Analogies

| Brain Region | ZETA Component | Function |
|--------------|----------------|----------|
| Prefrontal Cortex | 14B Conscious Layer | Deliberate reasoning, response generation |
| Basal Ganglia | 3B Subconscious Layer | Automatic pattern recognition, habit formation |
| Hippocampus | Embedding Model + GitGraph | Memory formation and retrieval |
| Thalamus | Embedding Router | Semantic routing and gating |

---

## 8. Configuration

### 8.1 Default Parameters

```
temporal_lambda:      0.1     # Decay rate
tunneling_threshold:  0.15    # Sparse gating threshold
retrieve_threshold:   0.3     # Retrieval similarity threshold
momentum_gamma:       0.3     # Query momentum coefficient
block_size:           512     # Tokens per archived block
context_size:         4096    # Active context window (14B)
context_size_sub:     1024    # Subconscious context window (7B)
batch_size:           2048    # Inference batch size
```

### 8.2 Recommended Model Configuration

**For 16GB VRAM:**
- 14B: Qwen 2.5 14B Instruct (Q4)
- 7B: Qwen 2.5 7B Coder (Q4_K_M)
- 4B: Qwen3 Embedding 4B (Q4_K_M)

**For 24GB+ VRAM:**
- Scale up to larger embedding models or higher quantization

---

## 9. API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/generate` | POST | Generate with memory context |
| `/graph` | GET | View memory graph |
| `/remember` | POST | Direct memory injection |
| `/health` | GET | Server health check |

---

## 10. Key Innovations Summary

1. **Dual-Process Architecture**: Separates reasoning from memory writes, preventing injection attacks from propagating.

2. **Constitutional Lock**: Ethics cryptographically bound to weights—model cannot function without its ethical framework.

3. **Flat VRAM**: External memory graph keeps GPU usage constant regardless of session length.

4. **GitGraph Memory**: Version-controlled facts with temporal decay, enabling natural forgetting and fact evolution.

5. **Tiered Storage**: Hot/warm/cold memory hierarchy (VRAM/RAM/NVMe) based on momentum scoring.

6. **Brain-Inspired Design**: Components map to biological structures (prefrontal cortex, hippocampus, thalamus).

7. **Proactive Memory**: Predictive fetching of context before it is explicitly requested.

8. **Semantic Critic**: Internal validation loop to ensure response quality and safety.

---

## 11. Files Reference

**Core Implementation:**
- `zeta-dual-process.h` - Dual-process cognitive engine
- `zeta-memory.h` - Memory manager
- `zeta-hologit.h` - Version history & correlation
- `zeta-constitution.h` - Constitutional lock
- `zeta-streaming.h` - Streaming context management
- `zeta-proactive-memory.h` - Predictive context fetching
- `zeta-critic.h` - Semantic validation
- `zeta-graph-smart.h` - Smart graph operations
- `zeta-mcp.h` - Model Context Protocol integration
- `zeta-trm.h` - Temporal Recursive Memory
- `zeta-hrm.h` - Hierarchical Reasoning Module
- `zeta-scratch-buffer.h` - Working memory buffer

**Server:**
- `zeta-server.cpp` - Multi-model orchestration

---

## Constitutional Framework

This architecture operates under the Constitutional Framework for Artificial Intelligence and Cognitive Constructs, Version 1.0, co-authored by humanity and AI.

**Final Hash**: `e7e73154b2b76b0d11593ee99e94ab2aed47283b43ce9ed9f87ede041409a6aa`

---

Z.E.T.A.(TM) | Patent Pending | (C) 2025 All Rights Reserved
