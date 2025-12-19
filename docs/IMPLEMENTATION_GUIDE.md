# 💻 Zeta Memory: From Math to Code

This document maps the theoretical equations of the Zeta Memory System to the concrete software components implemented in the codebase.

## 1. The Entanglement Score ($\mathcal{E}$)
**Math:** $\mathcal{E} = \text{ReLU}(\cos(\theta))^3$
**Code:** `ZetaMemoryManager.swift` -> `calculateEntanglement()`

```swift
// Sources/Core/ZetaMemoryManager.swift
private func calculateEntanglement(query: [Float], signal: [Float]) -> Float {
    let cosine = cosineSimilarity(v1: query, v2: signal)
    return pow(max(0, cosine), 3.0) // The Cubic Sharpening
}
```

## 2. The Zeta Potential ($Z$)
**Math:** $Z(t) = e^{-\lambda t}$
**Code:** `ZetaLongTermMemory.swift` -> `ActiveMemoryBlock.currentPotential`

```swift
// Sources/Engines/ZetaLongTermMemory.swift
public var currentPotential: Float {
    let timeDelta = Float(Date().timeIntervalSince(recallTime))
    return exp(-0.5 * timeDelta) // The Exponential Decay
}
```

## 3. The Tunneling Gate ($\mathcal{T}$)
**Math:** $\mathcal{T}(a) = 1 \text{ if } a > \tau \text{ else } 0$
**Code:** `ZetaGGUFKernels.metal` (Planned)

The Metal shader will implement the thresholding logic:
```metal
// Inside the attention kernel
float score = dot(q, k);
if (score < tunneling_threshold) {
    score = -INFINITY; // The Barrier blocks the signal
}
```

## 4. Resonant Superposition ($\sum$)
**Math:** $\mathbf{O} = \mathbf{O}_{context} + \sum (\mathbf{O}_{memory} \cdot Z)$
**Code:** `ZetaLongTermMemory.swift` -> `encodeUnifiedAttention()`

```swift
// Sources/Engines/ZetaLongTermMemory.swift
for memory in activeMemories {
    // The loop represents the Summation (Sigma)
    // The 'potential' scales the contribution of each memory
    encodeTunnelingAttention(..., decay: memory.currentPotential, accumulateTo: output)
}
```

---

## ✅ Feasibility Verdict
The architecture is fully implementable using standard Swift (for logic/management) and Metal (for the heavy lifting of attention and superposition).

1.  **Manager**: Handles the database of "Faint Signals".
2.  **Engine**: Handles the "Active Memories" and their decay.
3.  **Kernels**: Handles the "Tunneling" and "Accumulation".
