# ZetaLm

**Z.E.T.A. Zero** - Apple Native LLM Inference Engine

Pure Metal + Core ML implementation for Apple Silicon.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         ZetaLm                              │
├─────────────────────────────────────────────────────────────┤
│  Phase 3: ZetaUI                                            │
│  └── SwiftUI native interface                               │
├─────────────────────────────────────────────────────────────┤
│  Phase 2: ZetaFeatures                                      │
│  ├── Temporal Decay: Z(t) = Z₀·e^(-λt)                      │
│  ├── Tunneling Gate: a < τ → -∞                             │
│  ├── Entanglement: E = ReLU(cos)³                           │
│  ├── Semantic Momentum: I(t) = q + γ(q - q_prev)            │
│  └── Superposition: O = Attn(active) + Σ Z_k·Attn(mem_k)    │
├─────────────────────────────────────────────────────────────┤
│  Phase 1: ZetaCore                                          │
│  ├── ZetaCoreEngine.swift    (orchestration)                │
│  ├── ZetaCoreLoader.swift    (GGUF parsing)                 │
│  ├── ZetaModel.swift         (weights + tokenizer)          │
│  └── Shaders/                                               │
│      ├── ZetaQuants.metal    (Q4_0, Q6_K, Q8_0)            │
│      ├── ZetaAttention.metal (RoPE, KV cache, softmax)      │
│      └── ZetaFFN.metal       (SwiGLU activation)            │
└─────────────────────────────────────────────────────────────┘
```

## Build

```bash
cd ZetaLm
swift build
```

## Run

```bash
swift run ZetaCLI Models/tinyllama.gguf "Hello" 50
```

## Phase 1 Milestone

**Exit Criteria**: Output must match `llama-cli` exactly on same prompt.

```bash
# Reference
llama-cli -m model.gguf -p "The capital of France is" -n 10

# ZetaLm (must match)
swift run ZetaCLI model.gguf "The capital of France is" 10
```

## License

Proprietary - All rights reserved
