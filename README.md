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

![GPU Power Chart](https://quickchart.io/chart?c=%7Btype%3A%27line%27%2Cdata%3A%7Blabels%3A%5B%271%27%2C%27%27%2C%272%27%2C%27%27%2C%273%27%2C%27%27%2C%274%27%2C%27%27%2C%275%27%2C%27%27%2C%276%27%2C%27%27%2C%277%27%2C%27%27%2C%278%27%2C%27%27%2C%279%27%2C%27%27%2C%2710%27%2C%27%27%5D%2Cdatasets%3A%5B%7Blabel%3A%27Standard%20LLM%27%2Cdata%3A%5B300%2C50%2C350%2C50%2C400%2C50%2C450%2C50%2C500%2C50%2C550%2C50%2C600%2C50%2C650%2C50%2C700%2C50%2C750%2C50%5D%2CborderColor%3A%27%23ff4040%27%2CborderWidth%3A2%2Cfill%3Afalse%2CpointRadius%3A3%7D%2C%7Blabel%3A%27Z.E.T.A.%27%2Cdata%3A%5B300%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%2C150%2C50%5D%2CborderColor%3A%27%2300d26a%27%2CborderWidth%3A2%2Cfill%3Afalse%2CpointRadius%3A3%7D%5D%7D%2Coptions%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%27GPU%20Power%20(Watts)%20-%20Growing%20Context%27%7D%2Cscales%3A%7BxAxes%3A%5B%7BscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%27Turn%27%7D%7D%5D%2CyAxes%3A%5B%7Bticks%3A%7Bmin%3A0%2Cmax%3A800%7D%7D%5D%7D%7D%7D&w=700&h=300&bkg=white)

Standard LLMs recompute everything as context grows. Z.E.T.A. computes deltas.

### Response Time: Repeated Queries

![Response Time Chart](https://quickchart.io/chart?c=%7Btype%3A%27line%27%2Cdata%3A%7Blabels%3A%5B%27Q1%27%2C%27Q2%27%2C%27Q3%27%2C%27Q4%27%2C%27Q5%27%2C%27Q6%27%2C%27Q7%27%2C%27Q8%27%2C%27Q9%27%2C%27Q10%27%5D%2Cdatasets%3A%5B%7Blabel%3A%27Standard%20LLM%27%2Cdata%3A%5B2.5%2C3.5%2C4.5%2C5.5%2C6.5%2C7.5%2C8.5%2C9.5%2C10.5%2C11.5%5D%2CborderColor%3A%27%23ff4040%27%2CborderWidth%3A2%2Cfill%3Afalse%2CpointRadius%3A4%7D%2C%7Blabel%3A%27Z.E.T.A.%27%2Cdata%3A%5B2.5%2C2.8%2C3.0%2C0.3%2C0.3%2C3.2%2C0.3%2C0.3%2C0.3%2C3.4%5D%2CborderColor%3A%27%2300d26a%27%2CborderWidth%3A2%2Cfill%3Afalse%2CpointRadius%3A4%7D%5D%7D%2Coptions%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%27Response%20Time%20(seconds)%27%7D%2Cscales%3A%7ByAxes%3A%5B%7Bticks%3A%7Bmin%3A0%2Cmax%3A12%7D%7D%5D%7D%7D%7D&w=700&h=300&bkg=white)

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
typedef struct {
    uint8_t hash[32];           // SHA-256 of constitution text
    uint64_t seed;              // PRNG seed derived from hash
    bool verified;              // True only if constitution matches
} zeta_constitution_t;

// 1. Hash the constitution → 256-bit key
// 2. Key seeds the PRNG for weight permutation
// 3. Weights are STORED permuted — wrong key = garbage output

void zeta_generate_permutation(
    const zeta_constitution_t* ctx,  // Contains the hash
    int* permutation_out,            // Shuffle order for weights
    int n
);
```

The model cannot function without the correct constitution present. Change the ethics, the weights become noise. It governs itself or lobotomy 

→ [zeta-constitution.h](llama.cpp/tools/zeta-zero/zeta-constitution.h)  
→ [THE_SILICON_ACCORD.txt](THE_SILICON_ACCORD.txt)

---

## License

Dual licensed: [Open Source](LICENSE) + [Commercial](COMMERCIAL_LICENSE.md)

If your company uses Z.E.T.A. and earns over $2 million/year in revenue, contact for pricing.

Otherwise? Use it to go make $2 million a year.

**todd@hendrixxdesign.com**
