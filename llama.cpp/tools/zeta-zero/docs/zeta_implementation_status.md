
# Z.E.T.A. llama.cpp Implementation Status

## Overview

Z.E.T.A. (Zero Entropy Temporal Assimilation) provides extended context through:
- **Temporal Decay**: Exponential attention decay based on token age
- **Sparse Gating**: Thresholding weak attention connections
- **Memory Sublimation**: Archiving KV cache to disk with retrieval
- **Metal GPU Acceleration**: GPU-accelerated similarity search and kernels

## Current Status: WORKING

### What's Implemented

1. **Metal Kernel Infrastructure** - Custom GPU kernels for Z.E.T.A. operations
2. **Memory Management** - Async prefetch + mmap tiered storage
3. **Constitutional Lock** - SHA-256 hash verification
4. **Model Binding** - Vocabulary permutation using constitution hash
5. **GPU-Accelerated Retrieval** - Metal cosine similarity kernel
6. **Superposition Injection** - Memory attention injection into hidden state

### Performance

From testing (100 tokens on M2 Pro):
```
Total generation:     10,593 ms
- Decode time:        10,545 ms (99.5%)  <- llama.cpp inference
- Retrieval time:        26 ms (0.2%)   <- Z.E.T.A. memory (Metal GPU)

GPU retrievals:       100
Avg retrieval:        0.256 ms/token
Metal acceleration:   ACTIVE
```

## Files

### Core Z.E.T.A. (NEW)

| File | Description |
|------|-------------|
| `zeta-memory.h/c` | Memory manager with mmap tiered storage |
| `zeta-integration.h/c` | Integration layer for llama inference |
| `zeta-constitution.h/c` | Constitutional lock (SHA-256) |
| `zeta-model-bind.h/c` | Model weight binding |
| `zeta-kv-extract.h/c` | KV cache extraction |
| `zeta-metal.h/m` | Metal GPU kernel dispatch (Obj-C) |
| `zeta-kernels.metal` | Metal compute shaders |

### Metal Kernels (zeta-kernels.metal)

| Kernel | Function |
|--------|----------|
| `zeta_temporal_decay` | Z(t) = Z0 * e^(-λt) attention decay |
| `zeta_sparse_gate` | Zero sub-threshold attention |
| `zeta_attention_modifier` | Combined decay + gating |
| `zeta_generate_mask` | Pre-compute attention mask |
| `zeta_memory_injection` | O_final = O_ctx + α·O_mem |
| `zeta_sparse_softmax` | Softmax with sparse cleanup |
| `zeta_cosine_similarity` | Block similarity search |

### Demo Tool

| File | Description |
|------|-------------|
| `tools/zeta-zero/zeta-zero.cpp` | Full demo with Metal acceleration |
| `tools/zeta-zero/CMakeLists.txt` | Build config with Metal linking |

## Build Instructions

```bash
cd llama.cpp/build
cmake .. -DGGML_METAL=ON -DCMAKE_BUILD_TYPE=Release
make llama-zeta-zero -j8
```

## Usage

```bash
./bin/llama-zeta-zero \
  -m model.gguf \
  -p "Your prompt" \
  -n 100 \
  --zeta-constitution ../CONSTITUTION.txt \
  --zeta-lambda 0.01 \
  --zeta-tau 0.01
```

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--zeta-lambda` | 0.01 | Temporal decay rate (0 = disabled) |
| `--zeta-tau` | 0.01 | Sparse gating threshold |
| `--zeta-retrieve` | 0.3 | Memory retrieval similarity threshold |
| `--zeta-momentum` | 0.3 | Query momentum for prediction |
| `--zeta-storage` | /tmp/zeta | Storage directory for memory blocks |
| `--zeta-constitution` | (embedded) | Path to constitution file |

## Architecture

### Integration Points

```
llama_decode() ────────────────────> llama.cpp Metal kernels (99.5% time)
       │
       ├──> zeta_pre_decode()        Query update, prefetch trigger
       │
       └──> [after decode]
            │
            ├──> zeta_kernel_block_similarities()  GPU similarity search
            │
            ├──> zeta_inject_superposition()       Memory injection
            │
            └──> logit_bias()                      Confidence adjustment
```

### External vs Internal Integration

**Current: External (Non-invasive)**
- Works alongside llama.cpp without modification
- Memory retrieval and injection on GPU
- ~0.2% overhead

**Future: Internal (Fork Required)**
- Modify `set_input_kq_mask()` for temporal decay
- Inject directly into attention computation
- Could improve memory integration quality

## Known Issues & Fixes

### Sparse Gating NaN Prevention

The sparse gating kernel uses `-INFINITY` instead of `0` for masked positions.
This prevents NaN when all positions are masked (since -inf - (-inf) = NaN).

**Safeguard**: The current position is never masked, ensuring at least one valid attention target.

```metal
// Never mask the most recent token to prevent all-masked rows
if (score < threshold && k_idx != current_pos) {
    score = -INFINITY;
}
```

## Testing

```bash
# Basic test
./bin/llama-zeta-zero -m model.gguf -p "The capital of France is" -n 30 \
  --zeta-constitution ../CONSTITUTION.txt

# Extended generation
./bin/llama-zeta-zero -m model.gguf -p "Once upon a time" -n 500 \
  --zeta-constitution ../CONSTITUTION.txt
```

---

Z.E.T.A.™ | Patent Pending | © 2025 All rights reserved.

---

# 2026-01-14 Codebase Audit (Brutally Honest)

## Summary (No sugarcoating)
- The core server is real and works, but it’s a monolith: `zeta-server.cpp` is ~8.7k lines with deep coupling and mixed concerns. This makes integration fragile and slows safe iteration.
- The repo is packed with ambitious modules, but many are **present, not wired**. There’s a visible gap between design intent and runtime behavior.
- Several systems overlap in responsibility (memory, embeddings, conflict detection, output control). Overlap isn’t inherently bad, but here it’s mostly undocumented and not policy-driven.
- Testing is sparse relative to surface area; most modules have no dedicated tests or integration gates.
- Documentation is aspirational and partially stale; it does not match what’s actually wired in `zeta-server.cpp` today.

## What’s wired vs. not wired (from UNWIRED.md + code scan)
**Wired (active in `zeta-server.cpp`)**
- zeta-cyclic, zeta-graph-kv (+ integration), zeta-gitgraph (+ traversal), zeta-dedup, zeta-tunnel-search, zeta-ternary
- zeta-output-control, zeta-trm, zeta-critic, zeta-scratch-buffer, zeta-memory, zeta-kv-extract, zeta-mode-controller
- zeta-embed-memory
- zeta-code-mode / code endpoints (confirmed by `/code/*` and `/extract_code_7b` handlers)

**Recently wired**
- zeta-causal-embeddings (CAUSES/PREVENTS extraction)
- zeta-gitgraph-persist (co-retrieval graph persistence)

**Partially wired / limited use**
- Graph-KV capture relies on warmup/lazy capture; branch-aware capture not implemented

**Present but not wired (declared, included, or listed but not executed)**
- zeta-deliberation, zeta-embed-integration, zeta-proactive-memory, zeta-self-modify
- zeta-research-graph-integration, zeta-swarm, zeta-text-memory, zeta-story-integration
- zeta-semantic-attacks, aura-gkv
- zeta-branching-engine + BRANCHING_ENGINE_INTEGRATION.cpp (integration guide only)

## Overlaps & potential conflicts
1. **Memory Systems Overlap**
  - zeta-memory / zeta-graph-kv / zeta-gitgraph / zeta-text-memory / zeta-fact-store / zeta-scratch-buffer all touch persistence, retrieval, or context injection.
  - There’s no single canonical “source of truth” or prioritization policy, which makes correctness and latency tradeoffs opaque.

2. **Embedding / Similarity Overlap**
  - zeta-embed-integration, zeta-embed-memory, zeta-causal-embeddings, zeta-semantic-attacks all compute or rely on embeddings, but wiring and caching are inconsistent.
  - Embedding cache and dedup systems are not consolidated, which risks redundant compute and diverging thresholds.

3. **Conflict & Safety Overlap**
  - zeta-conflict (general), zeta-code-conflict (code-specific), zeta-ontology (domain authority) overlap but are not orchestrated under a unified decision pipeline.

4. **Output Control Overlap**
  - zeta-output-control and zeta-format-discovery both shape output, but the order of enforcement and failure modes are not standardized.

## What is likely not working (or not working reliably)
- Branching engine: full design exists, but the actual server integration is stubbed in a guide file. No live endpoints or observable effects.
- Deliberation engine: declared, but not initialized or run in output flow.
- Proactive memory: present but not integrated into routing or KV injection.
- Semantic attacks: present but not enforced (no gating in observed request flow).
- Text memory: exists, but no clear ingestion and retrieval path in server.
- AURA-GKV compression: implemented, but not wired to storage or persistence paths.

## TODO List (actionable, prioritized)
**P0 — correctness & operability**
1. Add a **single authoritative memory policy** (source-of-truth + precedence) and enforce it in `zeta-server.cpp`.
2. Wire (or delete) **zeta-embed-integration** and make embedding cache authoritative for all embedding consumers.
3. Create a **minimal integration test suite** that hits: `/generate`, `/extract_code_7b`, graph-kv inject, gitgraph persist, and conflict detection.

**P1 — reduce architectural risk**
4. Break `zeta-server.cpp` into modules (routing, memory, code mode, tools, safety). Add a thin integration layer.
5. Decide the **official branch policy**: either wire `zeta-branching-engine` or remove it from build to avoid false capability.
6. Unify conflict detection (general + code + ontology) into a single enforcement chain with explicit priority rules.

**P2 — performance/quality**
7. Wire AURA-GKV compression into Graph-KV persistence, with a config toggle and metrics.
8. Add tracing around embedding and retrieval calls; expose latency breakdown in a `/metrics` endpoint.
9. Convert “present but not wired” modules into explicit feature flags, surfaced in `/status`.

---

# File Inventory (per file)

## Root files
| File | Purpose | Status |
|------|---------|--------|
| BRANCHING_ENGINE_INTEGRATION.cpp | Integration guide and patch plan for branching engine in server. | Not wired (guide only) |
| CMakeLists.txt | Build configuration for zeta-zero tool. | Build |
| UNWIRED.md | Current wiring status and integration notes. | Docs (source of truth) |
| aura-gkv.h | Graph‑KV compression (AURA‑GKV pipeline). | Present, not wired |
| test-ontology.cpp | CLI test harness for `zeta-ontology.h`. | Test |

## Docs
| File | Purpose | Status |
|------|---------|--------|
| docs/architecture_review.md | High‑level architecture narrative. | Docs |
| docs/classes_functions.puml | Class/function diagram (PlantUML). | Docs |
| docs/deployment.puml | Deployment diagram (PlantUML). | Docs |
| docs/overview.puml | Overview diagram (PlantUML). | Docs |
| docs/sequence.puml | Sequence diagram (PlantUML). | Docs |
| docs/use_case.puml | Use‑case diagram (PlantUML). | Docs |
| docs/zeta_implementation_status.md | Implementation status & build notes. | Docs |
| docs/zeta_modification_guide.md | Modification guidance. | Docs |
| docs/zeta_test_results.md | Historical test results (2024). | Docs (stale) |

## Core server / integration
| File | Purpose | Status |
|------|---------|--------|
| zeta-server.cpp | Main HTTP server + orchestration; includes most subsystems. | Wired (core) |
| zeta-config.h | Reads zeta.conf (priority search order + overrides). | Wired |
| zeta-integration.h/c | Hooks into llama.cpp inference lifecycle. | Wired |
| zeta-kv-extract.h/c | KV cache extraction from llama state for memory blocks. | Wired |
| zeta-memory.h/c | Tiered memory manager (mmap/prefetch). | Wired |
| zeta-token-storage.h | Persistent token cache for snippet/token counts. | Wired |

## Graph / KV / persistence
| File | Purpose | Status |
|------|---------|--------|
| zeta-graph-kv.h/c | Graph‑KV storage and injection. | Wired (partial capture) |
| zeta-graph-kv-integration.h | Bridges Graph‑KV into streaming retrieval. | Wired |
| zeta-gitgraph.h/c | Versioned memory graph (git‑style). | Wired |
| zeta-gitgraph-persist.h | NVMe persistence for graph blocks/edges. | Recently wired |
| zeta-git-traversal.h | Branch/tunneling integration and traversal helpers. | Wired |
| zeta-graph-git.h | Git‑style branch semantics for knowledge graph. | Wired |
| zeta-graph-manager.h | Edge control + graph‑of‑graphs management. | Included; wiring unclear |
| zeta-graph-smart.h | Sudo parsing, dedup, adversarial filter for writes. | Included; wiring unclear |
| zeta-dedup.h/c | Dedup engine (hash/LSH/bloom). | Wired |
| zeta-tunnel-search.h/c | Momentum‑driven tunneling retrieval. | Wired |
| zeta-version.h/c | Fact versioning and chain history. | Included; wiring unclear |
| zeta-text-memory.h | Stores prompt text alongside memory blocks. | Present, not wired |
| zeta-text-inject.h | Text‑injection helpers for saved text blocks. | Included; wiring unclear |
| zeta-fact-store.h | Entity‑fact persistence store. | Included; wiring unclear |

## Models / cognition
| File | Purpose | Status |
|------|---------|--------|
| zeta-dual-process.h | Dual‑process orchestration (3B + 14B). | Wired |
| zeta-3b-extract.h | 3B fact extractor via structured prompts. | Included; wiring unclear |
| zeta-code-extract.h | 7B code entity extraction (parallel). | Wired (via `/extract_code_7b`) |
| zeta-code-mode.h | Code‑mode graph & project tracking. | Wired |
| zeta-code-streaming.h | Streaming code‑context surfacing. | Wired |
| zeta-code-conflict.h | Code‑specific conflict detection. | Wired (via `/code/check`) |
| zeta-cyclic.h | 3B parallel worker + cyclic correlation. | Wired |
| zeta-trm.h | Temporal Recursive Memory. | Wired |
| zeta-hrm.h | Hierarchical Reasoning Module. | Included; wiring unclear |
| zeta-system.h | System integration layer (HRM/TRM/Dream). | Included; wiring unclear |
| zeta-dream.h | Dream state orchestration. | Included; wiring unclear |
| zeta-pruning.h | Graph pruning hooks. | Included; wiring unclear |
| zeta-task-eval.h | Task evaluation scoring. | Included; wiring unclear |
| zeta-deliberation.h | Deliberation engine (pre‑output). | Present, not wired |
| zeta-branching-engine.h | Branching primitives + merge policy. | Present, not wired |
| zeta-mode-controller.h/cpp | Mode policies + controller wiring. | Wired (controller exists) |
| zeta-research.h | Research mode structures and logic. | Included; wiring unclear |
| zeta-research-graph-integration.h | Research mode ↔ graph/KV integration. | Present, not wired |
| zeta-discovery.h | Discovery mode (research + dream loop). | Included; wiring unclear |
| zeta-story-integration.h | Story consistency via graph. | Present, not wired |
| zeta-scratch-buffer.h | Working memory buffer for staged generation. | Wired |
| zeta-scratch-integration.h | Scratch buffer integration with server. | Wired |

## Embeddings, safety, and governance
| File | Purpose | Status |
|------|---------|--------|
| zeta-embed-integration.h | Embedding model integration + cache. | Present, not wired |
| zeta-embed-memory.h | Semantic dedup / consolidation helpers. | Wired (debug endpoint) |
| zeta-causal-embeddings.h | Cause/prevent semantic extraction. | Recently wired |
| zeta-semantic-attacks.h | Embedding‑based attack detection. | Present, not wired |
| zeta-ontology.h | Domain authority classifier (system vs personal). | Included; test exists |
| zeta-conflict.h | General conflict detection / safety helpers. | Wired |
| zeta-critic.h | Semantic critic for output verification. | Wired |
| zeta-output-control.h | Dynamic output limits + runaway detection. | Wired |
| zeta-format-discovery.h | Auto format discovery for outputs. | Included; wiring unclear |

## Tools / MCP / cloud / swarm
| File | Purpose | Status |
|------|---------|--------|
| zeta-tools.h | Tool API + permissions. | Wired (server integration) |
| zeta-mcp.h | MCP protocol wrapper for tools. | Included; wiring unclear |
| zeta-cloud.h | Optional cloud routing. | Included; wiring unclear |
| zeta-litellm.h | LiteLLM proxy client. | Included; wiring unclear |
| zeta-swarm.h | Distributed swarm coordination. | Present, not wired |

## GPU / kernel / constitution
| File | Purpose | Status |
|------|---------|--------|
| zeta-kernels.metal | Metal kernels (decay/gating/similarity). | Wired (Metal path) |
| zeta-metal.h/m | Metal kernel dispatch layer. | Wired (Metal path) |
| zeta-constitution.h/c | Constitutional lock (hashing + PRNG). | Wired |
| zeta-constitution-bridge.h/c | CPU↔GPU constitution bridge. | Wired (CUDA path) |
| zeta-constitution-cuda.cu/cuh | CUDA implementation for constitution binding. | Wired (CUDA path) |
| zeta-model-bind.h/c | Model binding (vocab permutation). | Wired |
| aura-gkv.h | KV compression for storage. | Present, not wired |

## Misc utilities
| File | Purpose | Status |
|------|---------|--------|
| zeta-domains.h | Keyword‑based domain helpers. | Included; wiring unclear |
| zeta-extract.h | Simple regex‑based fact extraction helpers. | Included; wiring unclear |
| zeta-proactive-memory.h | Momentum‑driven prefetch via tunneling. | Present, not wired |
| zeta-semantic-tools.h | Semantic tool operations (extract/store/query). | Included; wiring unclear |
| zeta-ternary.h | Ternary logic + consensus. | Wired |
| zeta-utils.h | Utility helpers. | Wired |
| zeta-graph-kv.h | (See Graph/KV section) | Wired |

---

## Notes on confidence of status labels
If a module is marked “included; wiring unclear,” it is either included in `zeta-server.cpp` or present in the folder but I did not find a live call site in the small scan. Those should be verified by targeted grep or instrumentation before claiming “working.”
