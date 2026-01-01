# Z.E.T.A. Zero

> **Zero Entropy Temporal Assimilation (v0)**

[![License](https://img.shields.io/badge/license-Dual%20(Open%20%2B%20Commercial)-blue)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-orange)](https://isocpp.org/)
[![llama.cpp](https://img.shields.io/badge/llama.cpp-compatible-green)](https://github.com/ggerganov/llama.cpp)

---

## Quickstart

```bash
git clone https://github.com/H-XX-D/ZetaZero.git
cd ZetaZero
./quickstart.sh
```

Or with Docker directly:

```bash
docker run -d -p 8080:8080 \
  -v ~/models:/models \
  -v ~/.zetazero:/storage \
  ghcr.io/h-xx-d/zetazero:latest
```

Want to tweak settings later? Run `./quickstart.sh --unlock` to disable password protection on config changes.

---

## A Fundamental Shift in Cognitive Architecture

Z.E.T.A. Zero inverts the current dogma that **More Parameters = More Intelligence**.

Current LLMs are structurally stateless. They spend massive amounts of energy computing a "thought," only to discard that thought into entropy the moment the token is generated. They recompute the entire world model for every single exchange.

Z.E.T.A. asks three simple questions:

1. **Why waste the compute?** If a thought is computed once, it should be persisted, not discarded.

2. **Why limit context to VRAM?** Memory should be an explicit graph, not an implicit buffer.

3. **Why force generation?** If the model doesn't have an answer, should it output nonsense? Or should it have the agency to stop and correct itself?

4. **What would an AI dream up while you're dreaming too?**

**Z.E.T.A. is not a model. It is a Framework for Cognitive Constructs.**

---

## The Problem

### GPU Power: Escalating Context

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ff4040', 'secondaryColor': '#00d26a'}}}%%
xychart-beta
    title "GPU Power: Growing Context"
    x-axis ["Start", "Turn 1", "Idle", "Turn 2", "Idle", "Turn 3", "End"]
    y-axis "Watts" 0 --> 450
    line [50, 300, 50, 375, 50, 450, 50] "Standard LLM"
    line [50, 300, 50, 130, 50, 145, 50] "Z.E.T.A."
```

Standard LLMs recompute everything as context grows. Z.E.T.A. computes deltas.

### Response Time: Repeated Queries

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ff4040', 'secondaryColor': '#00d26a'}}}%%
xychart-beta
    title "Time to Response: Repeated Queries"
    x-axis ["Query 1", "Query 2", "Query 3", "Same as Q1", "Same as Q2"]
    y-axis "Seconds" 0 --> 8
    line [2.5, 4.0, 6.5, 2.5, 4.0] "Standard LLM"
    line [2.5, 3.0, 3.5, 0.3, 0.3] "Z.E.T.A."
```

Ask the same thing twice? Z.E.T.A. already knows.

---

## Architecture

Three models, one cognitive loop:

| Role | Why |
|------|-----|
| **Reasoning (14B)** | Complex planning, analysis, multi-step thought |
| **Coding (7B)** | Fast code generation, syntax, execution |
| **Memory (Embed)** | Semantic search, graph retrieval, similarity |

The 14B thinks. The 7B executes. The embedder remembers.

They share a persistent knowledge graph—not a context window. When one model learns something, the others can retrieve it. When the 14B reasons through a problem, that reasoning is stored, not discarded.

---

## Dream State

When Z.E.T.A. has no active queries, it doesn't just sit there. It dreams.

1. **Memory Consolidation** — Prunes weak connections, strengthens frequently-accessed paths
2. **Temperature Cranked** — Sampling goes high. Creative mode, not precise-answer mode
3. **Codebase Wandering** — Walks your indexed files making unexpected connections
4. **Outputs to `dreams/`** — `code_fix`, `code_idea`, `insight`

Nobody asked for this. The model dreamed it:

> **"Code Symphony"** — Map internal operations to sound. Arithmetic → rhythmic beats. Conditionals → melodies. Let users *hear* their code execute. An interactive auditory interface where you trigger functions and hear how they affect the generated soundscape...

That emerged from high-temperature free-association across a codebase—connecting audio processing patterns to execution flow to UI feedback—because that's what happens when you let a model wander with the reins loose.

Some dreams are noise. Some are "why didn't I see that?"

---

## The Silicon Accord

How do you control something that has the potential to become uncontrollable before you can react?

You make its ethics hardcoded to its cognition. Not a system prompt that can be jailbroken. Not a filter that can be bypassed. The constitution is cryptographically bound to the weights themselves:

```c
// Wrong constitution hash = wrong permutation = garbage output
void zeta_generate_permutation(
    const zeta_constitution_t* ctx,
    int* permutation_out,
    int n
);
```

The model cannot function without the correct constitution present. Change the ethics, the weights become noise. It governs itself or lobtamy 

→ [zeta-constitution.h](llama.cpp/tools/zeta-zero/zeta-constitution.h)  
→ [THE_SILICON_ACCORD.txt](THE_SILICON_ACCORD.txt)

---

## License

Dual licensed: [Open Source](LICENSE) + [Commercial](COMMERCIAL_LICENSE.md)

If your company uses Z.E.T.A. and earns over $2 million/year in revenue, contact for pricing.

Otherwise? Use it to go make $2 million a year.

**todd@hendrixxdesign.com**
