# 🕸️ ZetaLm Entanglement Math

## 🌌 Concept
In the ZetaLm "Resonant Recall" architecture, **Entanglement** is the mechanism by which the active "stream of consciousness" (Working Memory) connects with dormant "faint signals" (Long-Term Memory).

Unlike simple vector search (Cosine Similarity), Entanglement accounts for the **energy** and **phase** of the signal, mimicking quantum resonance.

## 📐 The Math

We define the **Entanglement Score** ($\mathcal{E}$) between a Query state ($Q$) and a Memory Signal ($S$) as:

$$ \mathcal{E}(Q, S) = \underbrace{\left( \frac{Q \cdot S}{\|Q\| \|S\|} \right)}_{\text{Directional Alignment}} \times \underbrace{\exp\left( - \frac{\|Q - S\|^2}{2\sigma^2} \right)}_{\text{Spatial Coherence}} \times \underbrace{\left( 1 + \beta \cdot \Phi(Q, S) \right)}_{\text{Phase Resonance}} $$

### 1. Directional Alignment (Cosine Similarity)
Measures if the *topic* is the same.
- Range: $[-1, 1]$
- $1.0$ means identical direction.

### 2. Spatial Coherence (Gaussian Kernel)
Measures if the *intensity* and *location* in latent space are close.
- $\sigma$: The "bandwidth" of the memory field.
- Penalizes vectors that point in the same direction but have vastly different magnitudes (energies).

### 3. Phase Resonance ($\Phi$)
(Optional/Advanced)
If we treat the vectors as having a "frequency" based on their token positions or timestamp:
$$ \Phi(Q, S) = \cos(2\pi (t_Q - t_S) / T) $$
This creates a "temporal rhythm" where memories from specific intervals (e.g., "yesterday at this time") are more easily recalled.

## 💻 Implementation

In `ZetaMemoryManager.swift`, we approximate this as:

```swift
func calculateEntanglement(query: [Float], signal: [Float]) -> Float {
    // 1. Dot Product (Energy Interaction)
    let dot = dotProduct(query, signal)
    
    // 2. Magnitude Normalization (Wavefunction Normalization)
    let magQ = magnitude(query)
    let magS = magnitude(signal)
    let cosine = dot / (magQ * magS + 1e-6)
    
    // 3. Non-linear Activation (The "Collapse" Probability)
    // We use a sharp sigmoid or power function to filter noise
    let entanglement = pow(max(0, cosine), 3.0) 
    
    return entanglement
}
```

## 🔮 Why "Entanglement"?
We call it entanglement because:
1.  **Action at a Distance**: A thought here triggers a memory stored "far away" on disk.
2.  **Non-Locality**: The memory isn't physically in the neural net's weights; it's in an external superposition (disk) until observed (recalled).
3.  **Collapse**: When $\mathcal{E} > \text{Threshold}$, the "wavefunction collapses" — the faint signal becomes a full, detailed memory block in RAM.
