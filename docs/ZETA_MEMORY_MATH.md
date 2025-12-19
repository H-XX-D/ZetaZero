# 📐 Zeta Memory Math: The Unified Field Theory

This document defines the mathematical framework for the **ZetaLm Long-Term Memory System**, combining Quantum Entanglement, Zeta Decay, and Tunneling.

## 1. The Entanglement Score ($\mathcal{E}$)
*The "Trigger" - Determines IF a memory should be recalled.*

We define the entanglement between the current query vector $\mathbf{q}$ and a dormant memory signal $\mathbf{s}$ as:

$$ \mathcal{E}(\mathbf{q}, \mathbf{s}) = \text{ReLU}\left( \frac{\mathbf{q} \cdot \mathbf{s}}{\|\mathbf{q}\| \|\mathbf{s}\|} \right)^3 $$

*   **Cosine Similarity**: Measures directional alignment.
*   **Cubic Sharpening**: Acts as a soft threshold.
    *   $\cos \theta = 0.5 \implies \mathcal{E} = 0.125$ (Weak $\rightarrow$ Ignored)
    *   $\cos \theta = 0.9 \implies \mathcal{E} = 0.729$ (Strong $\rightarrow$ Recalled)
*   **ReLU**: Ensures no negative entanglement (anti-correlation doesn't trigger recall).

---

## 2. The Zeta Potential ($Z$)
*The "Lifecycle" - Determines how long a recalled memory stays active.*

When a memory is recalled at time $t_{recall}$, it is injected with a fresh potential. Its weight at a future time $t$ is governed by the Zeta Decay function:

$$ Z(t) = Z_0 \cdot e^{-\lambda (t - t_{recall})} $$

*   $Z_0$: Initial strength (usually $1.0$).
*   $\lambda$: The decay constant (e.g., $0.5$).
*   **Effect**: The memory is vivid immediately upon recall but fades exponentially as the conversation continues, preventing context clutter.

---

## 3. The Tunneling Gate ($\mathcal{T}$)
*The "Filter" - Determines WHICH parts of the memory are seen.*

Even if a memory block is recalled, we don't attend to all of it. We only attend to tokens where the attention energy exceeds the tunneling barrier.

Let $a_{ij}$ be the raw attention score between query $i$ and memory key $j$:
$$ a_{ij} = \frac{\mathbf{q}_i \cdot \mathbf{k}_j}{\sqrt{d_k}} $$

The Tunneling Probability $\mathcal{T}$ is:

$$ \mathcal{T}(a_{ij}) = \begin{cases} 
1 & \text{if } a_{ij} > \tau \text{ (Threshold)} \\
0 & \text{if } a_{ij} \le \tau 
\end{cases} $$

*   **Result**: The attention matrix is sparse. We only compute values that "tunnel" through the barrier.

---

## 4. The Unified Attention Equation
*Putting it all together.*

The final attention output $\mathbf{O}$ is a sum of the **Active Context** ($C$) and the **Recalled Memories** ($M_k$).

$$ \mathbf{O} = \text{Softmax}\left( \mathbf{Q}\mathbf{K}_C^\top \right)\mathbf{V}_C + \sum_{k \in \text{TopK}} \left[ \underbrace{\mathcal{E}(\mathbf{q}, \mathbf{s}_k)}_{\text{Entanglement}} \cdot \underbrace{Z_k(t)}_{\text{Decay}} \cdot \text{Softmax}\left( \mathcal{T}(\mathbf{Q}\mathbf{K}_{M_k}^\top) \right)\mathbf{V}_{M_k} \right] $$

### Interpretation:
1.  **Standard Attention**: $\text{Softmax}(\mathbf{Q}\mathbf{K}_C^\top)\mathbf{V}_C$ (Normal operation).
2.  **Memory Injection**: Added to the stream ONLY if:
    *   **Entanglement** $\mathcal{E}$ is high (Relevant topic).
    *   **Zeta Potential** $Z$ is high (Recently recalled).
    *   **Tunneling** $\mathcal{T}$ allows it (Specific details match).

This equation ensures that long-term memory is **context-aware**, **transient**, and **sparse**.

## 5. Resonant Superposition
*What happens when two thoughts are similar?*

When the query $\mathbf{q}$ resonates with multiple signals ($\mathbf{s}_1, \mathbf{s}_2$), the summation $\sum$ creates a **Superposition**.

*   **Constructive Interference**: If $\mathbf{s}_1$ and $\mathbf{s}_2$ contain compatible information, their contributions sum up, reinforcing the concept.
*   **Destructive Interference**: If they contain conflicting information, the model's self-attention mechanism must resolve the conflict based on the active context $C$.

This allows the AGI to synthesize information from multiple past events simultaneously, rather than being limited to a single "best match."
