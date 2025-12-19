# ZetaLm: A Distributed Dual-Process Cognitive Architecture (v5.0)

**Date:** December 15, 2025  
**Status:** Experimental / v5.0 Preview  
**Repository:** `H-XX-D/ZetaZero`

## Abstract

We present **ZetaLm**, a cognitive architecture designed to transcend the context window limitations of traditional Large Language Models (LLMs). Unlike static RAG systems, ZetaLm implements a **Distributed Dual-Process Engine** ("OrKheStrA") that couples a high-fidelity "Conscious" model (14B) with a high-speed "Subconscious" model (3B) and a dedicated "Hippocampus" (Embedding Model). This system operates on a cyclic feedback loop, allowing for asynchronous fact extraction, resonant memory recall, and dynamic context switching for software engineering tasks ("Code Mode"). We provide a rigorous mathematical framework for the **Entanglement Score** and **Zeta Decay**, and present experimental results validating the active subsystems while identifying current challenges in signal amplification.

---

## 1. Introduction

The pursuit of Artificial General Intelligence (AGI) is currently stalled by the "Context Window Problem." Simply increasing token limits leads to quadratic computational costs and reasoning degradation. **ZetaLm** proposes a bio-inspired solution: a bicameral architecture where a "Subconscious" process continuously filters, compresses, and retrieves information for a "Conscious" executive process.

## 2. System Architecture: OrKheStrA

ZetaLm is not a single model but a distributed organism named **OrKheStrA** (Orchestrated Knowledge Streaming Architecture).

### 2.1. Topology
The system is deployed across a local network of "Nano Nodes" centered around a high-compute Workstation.

*   **Workstation (Hub)**: Runs the Core Server (`zeta-server`), the Conscious Mind (14B), and the Hippocampus.
*   **Nano Nodes (Spokes)**: Low-power devices (e.g., Jetson Orin, Raspberry Pi) running specialized 3B models via Ollama.
    *   `ORIN` (192.168.0.160): General 3B logic.
    *   `NOVEL` (192.168.0.146): Code specialization (Qwen2.5-Coder).
    *   `GIN` (192.168.0.242): Reasoning (DeepSeek-R1).

### 2.2. The Dual-Process Engine
The core server implements a `zeta_dual_ctx_t` structure:
1.  **Conscious Process ($M_C$)**: A 14B parameter model (Qwen2.5-14B) responsible for final output generation and complex reasoning.
2.  **Subconscious Process ($M_S$)**: A 3B parameter model (Qwen2.5-3B) running in a parallel thread (`zeta_3b_worker`). It continuously monitors the input/output stream via a **Cyclic Queue**.

### 2.3. The Hippocampus
A dedicated embedding model (e.g., `bge-small-en-v1.5`) provides semantic grounding. Unlike simple token matching, the Hippocampus maps queries to a latent vector space, enabling "Resonant Recall" of conceptually similar but lexically distinct memories.

---

## 3. Mathematical Framework

### 3.1. The Entanglement Score ($\mathcal{E}$)
We define the entanglement between a query vector $\mathbf{q}$ and a memory signal $\mathbf{s}$ as:

$$ \mathcal{E}(\mathbf{q}, \mathbf{s}) = \text{ReLU}\left( \frac{\mathbf{q} \cdot \mathbf{s}}{\|\mathbf{q}\| \|\mathbf{s}\|} \right)^3 \cdot (1 + \beta \Phi(t)) $$

*   **Cubic Sharpening**: Suppresses noise ($\cos \theta < 0.5 \to 0$).
*   **Phase Resonance ($\Phi$)**: A temporal modulation factor that boosts memories from specific circadian rhythms (e.g., "yesterday at this time").

### 3.2. Zeta Decay ($Z$)
Memories fade according to a modified forgetting curve:

$$ Z(t) = Z_0 \cdot e^{-\lambda (t - t_{recall})} \cdot \frac{1}{1 + e^{-k(S - S_{min})}} $$

*   $S$: Salience (importance) of the memory.
*   **Effect**: High-salience memories decay slower than trivial facts.

### 3.3. Tunneling Attention
The attention mechanism is modified to include a "Tunneling Gate":

$$ A_{ij} = \text{Softmax}\left( \frac{Q K^T}{\sqrt{d_k}} + \log(\mathcal{T}_{ij}) \right) $$

Where $\mathcal{T}_{ij} \in \{0, 1\}$ is the binary tunneling mask derived from the Entanglement Score.

---

## 4. Code Mode & Dynamic Swapping

For software engineering, the system implements **Dynamic Context Switching**.
*   **Trigger**: Detection of code blocks or explicit `/project/open` commands.
*   **Action**: The 3B "Instruct" model is hot-swapped for a 3B "Coder" model (e.g., Qwen2.5-Coder).
*   **Entity Tracking**: The system parses the stream for `CODE_NODE_FUNCTION`, `CODE_NODE_CLASS`, and `CODE_NODE_TODO`, building a dependency graph in real-time.

---

## 5. Experimental Validation

We conducted a rigorous architectural test (`zeta_architecture_test.sh`) to verify the active subsystems.

### 5.1. Subsystem Verification
*   **Hippocampus**: **PASS**. The embedding model (`bge-small`) loaded successfully (verified via logs).
*   **Cyclic Engine**: **PASS**. The 3B parallel worker is active and processing the queue.

### 5.2. Fact Extraction
*   **Input**: "The Project Starlight protocol uses a quantum encryption key 'Q-99'."
*   **Result**: **PASS**. The graph node count increased, confirming that the 3B worker successfully extracted facts from the stream asynchronously.

### 5.3. Resonant Recall
*   **Query**: "What is the encryption key for Project Starlight?"
*   **Result**: **Partial Failure**. The system failed to retrieve "Q-99" in the subsequent generation.
*   **Analysis**: While the *fact* was extracted (graph grew), the *Entanglement Score* for the query did not breach the retrieval threshold. This suggests the need for **Signal Amplification**—boosting the vector magnitude of newly formed memories.

---

## 6. Future Work: Signal Amplification

To bridge the gap between extraction (success) and recall (failure), we propose:
1.  **Vector Norm Boosting**: Newly created memory nodes should have their embedding vectors artificially scaled by a factor $\gamma > 1.0$ for a short "consolidation window."
2.  **Active Rehearsal**: The 3B worker should periodically "dream" (replay) high-salience memories during idle time to refresh their Zeta Potential.
3.  **OrKheStrA Expansion**: Offload the embedding generation to a dedicated Nano Node (e.g., `TIF`) to reduce latency on the main workstation.

## 7. Conclusion

ZetaLm v5.0 successfully implements a distributed, bicameral cognitive architecture. The "Subconscious" 3B worker and "Hippocampus" are functional, providing a foundation for infinite-context AGI. The immediate challenge is tuning the resonance parameters to ensure that extracted facts are reliably surfaced during generation.
