# ZetaLm Long-Term Memory Architecture: "Resonant Recall"

## 🧠 Core Concept
The AGI cannot maintain perfect recall of infinite context in RAM due to hardware constraints. Instead, it mimics human memory:
1.  **Disk (Hippocampus/Cortex)**: Stores "perfect copies" of detailed memories.
2.  **RAM (Working Memory)**: Maintains only "faint signals" (compressed representations) of these memories.
3.  **Resonance**: When current thoughts (inputs) align with a faint signal, the full memory is "recalled" (loaded) from disk.

## 🏗 Architecture

### 1. The "Faint Signal" (RAM)
Instead of keeping full Key/Value tensors for old history, we keep a **Resonant Key**.
- **Structure**: A single vector (or small set of vectors) representing a large block of text (e.g., 1024 tokens).
- **Size**: 1 vector (e.g., 4KB) vs 1024 KV pairs (e.g., 256MB).
- **Location**: Stored in a specialized `ResonantBuffer` in GPU/CPU RAM.

### 2. The "Perfect Copy" (Disk)
- **Structure**: The full raw text and/or the computed KV Cache for that block.
- **Format**: Compressed binary format (`.zmem`).
- **Indexing**: Mapped to the ID of the Resonant Key.

### 3. The Resonance Logic (The "Trigger")
During the attention mechanism of the forward pass:
1.  **Query**: The current token being generated produces a Query vector `Q`.
2.  **Scan**: `Q` is compared against active KV cache (standard attention) AND the `ResonantBuffer` (faint signals).
3.  **Resonance Check**:
    ```swift
    let resonanceScore = dotProduct(Q, resonantKey)
    if resonanceScore > resonanceThreshold {
        triggerRecall(memoryId)
    }
    ```
4.  **Recall**:
    - Pause generation (micro-pause).
    - **Page In**: Load the associated KV block from disk.
    - **Attend**: Perform attention against this restored block.
    - **Update**: Mix the result into the current context.

## 🛠 Implementation Plan

### Phase 1: Data Structures
Define the memory block and signal types.

```swift
struct ResonantSignal {
    let id: UUID
    let embedding: [Float] // The "faint signal" (e.g., average of Keys in the block)
    let diskPath: URL
}

struct MemoryBlock {
    let tokens: [Int]
    let kvCache: Data // Serialized KV data
}
```

### Phase 2: The Memory Manager
A new component `ZetaMemoryManager` to handle the lifecycle.

- **`compress(range: Range<Int>)`**:
    1. Take a range of tokens/KV cache from active context.
    2. Compute the "Signal" (e.g., Mean Pooling of Key states).
    3. Serialize the full KV data to disk.
    4. Register the Signal in RAM.
    5. Free the original KV RAM.

- **`recall(signal: ResonantSignal)`**:
    1. Load KV data from `diskPath`.
    2. Temporarily attach to the engine's attention mechanism.

### Phase 3: Engine Integration (`ZetaGGUFEngine.swift`)
Modify the `forwardPass` or `attend` function.

```swift
// Pseudocode inside attention kernel or host logic
func calculateAttention(...) {
    // 1. Standard Attention
    let contextScores = Q * K_context
    
    // 2. Resonant Attention (The "Faint Signal" check)
    let memoryScores = Q * K_resonant_signals
    
    if let maxSignal = memoryScores.max(), maxSignal > threshold {
        // 3. Dynamic Retrieval
        let detailedMemory = memoryManager.recall(maxSignal.id)
        let detailedScores = Q * detailedMemory.K
        // Merge results...
    }
}
```

## 🧩 Logic Flow for "Infinite" Context

1.  **Fill Context**: User chats, context fills up to limit (e.g., 4096).
2.  **Eviction Event**: When full, the oldest 1024 tokens are not deleted, but **sublimated**.
    - They are moved to Disk.
    - A "Signal" is left behind in RAM.
3.  **Dreaming/Indexing**: (Optional) Background process refines the signals to be more representative.
4.  **Recall**: User asks "What did we talk about regarding the patent?"
    - The query "patent" generates a `Q` vector.
    - `Q` aligns with the "Signal" of the patent discussion.
    - System fetches the patent block.
    - System answers with perfect recall.

## ⚠️ Challenges
- **Latency**: Disk I/O is slow. We need pre-fetching or async recall.
- **Signal Quality**: How to ensure the "faint signal" is actually representative? (Mean pooling might be too noisy).
- **Thrashing**: Avoid constantly loading/unloading the same memory.

