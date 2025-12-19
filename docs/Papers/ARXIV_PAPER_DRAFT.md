# Z.E.T.A. Zero: Extending LLM Context via Hierarchical Memory Compression and Weighted Attention Injection

**Abstract**

Large Language Models (LLMs) are fundamentally constrained by their context window size. As sequence length increases, the computational cost of the attention mechanism grows quadratically $O(n^2)$, and the memory footprint of the Key-Value (KV) cache becomes prohibitive. We propose **Z.E.T.A.** (**Z**ero **E**ntropy **T**emporal **A**ssimilation), a memory architecture that decouples active working memory from long-term storage. By compressing older context blocks into summary vectors stored on disk and retrieving them via sharpened cosine similarity, we achieve extended context with constant RAM usage. We further introduce a momentum-based query prediction mechanism that anticipates retrieval needs based on the trajectory of the query vector.

## 1. Introduction

The context window of an LLM defines the boundary at which past tokens are evicted from the KV cache to accommodate new ones. Once evicted, this information is permanently lost, resetting the model's working state. This creates a fundamental inefficiency: the computation invested in processing tokens is discarded. Per Landauer's principle (Landauer, 1961), erasing information has an irreducible thermodynamic cost—each bit erased releases a minimum heat of $k_B T \ln(2)$. When an LLM evicts thousands of tokens from its KV cache, this information is irreversibly destroyed. While compression and storage operations still dissipate energy, they preserve the computational work in a recoverable form—the energy invested remains accessible rather than being lost as pure waste heat.

Current solutions such as RAG (Retrieval Augmented Generation) rely on external vector databases that are disjoint from the model's internal attention state—retrieved text is inserted into the context window, displacing other tokens.

We present an integrated approach where memory retrieval is fused directly into the attention computation. Our architecture:

1. **Compresses** old KV cache blocks into summary vectors and writes full data to disk
2. **Retrieves** relevant blocks via sharpened cosine similarity against the current query
3. **Injects** retrieved attention values directly into the output, weighted by recency and relevance

This allows extended context without the linear memory growth typically required for larger windows.

## 2. Methodology

### 2.1 Hierarchical Memory Compression

When the active context approaches capacity, we trigger compression of the oldest token blocks:

1. **Serialize**: Write the full KV cache tensors $(K_{block}, V_{block}) \in \mathbb{R}^{L_{block} \times d}$ to disk storage
2. **Summarize**: Compute a summary vector $\mathbf{s} \in \mathbb{R}^{d}$ via mean pooling:
   $$\mathbf{s} = \frac{1}{L_{block}} \sum_{i=1}^{L_{block}} \mathbf{k}_i$$
3. **Evict**: Remove the original block from GPU memory, retaining only $\mathbf{s}$ in RAM

This reduces memory per archived block from $O(L_{block} \cdot d)$ to $O(d)$.

### 2.2 Similarity-Based Retrieval

During inference, we compute the current query vector $\mathbf{q}_t$ (the mean of query heads for the current token). This is compared against all summary vectors $S = \{\mathbf{s}_1, \mathbf{s}_2, ..., \mathbf{s}_M\}$ to identify relevant historical blocks.

We use **Sharpened Cosine Similarity** to filter weak matches:

$$\mathcal{E}(\mathbf{q}, \mathbf{s}) = \max\left(0, \frac{\mathbf{q} \cdot \mathbf{s}}{\|\mathbf{q}\| \|\mathbf{s}\|}\right)^3$$

The cubic exponent acts as a soft gate: scores below ~0.5 are suppressed to near-zero, while strong matches (>0.8) remain high. This prevents noisy retrievals from polluting the attention output.

**Retrieval threshold**: If $\mathcal{E}(\mathbf{q}, \mathbf{s}_k) > \tau_{retrieve}$ (default 0.3), we load $(K_k, V_k)$ from disk into an active memory slot.

### 2.3 Momentum-Based Query Prediction

Standard retrieval uses only the current query $\mathbf{q}_t$. We observe that conversations have directional momentum—topics shift gradually. To anticipate upcoming retrieval needs, we define the **Prediction Vector**:

$$\mathbf{p}_t = \mathbf{q}_t + \gamma (\mathbf{q}_t - \mathbf{q}_{t-1})$$

where $\gamma \in [0, 1]$ is the momentum coefficient (default 0.3). This extrapolates the query trajectory, allowing pre-fetching of memories likely to become relevant.

### 2.4 Temporal Decay Weighting

Retrieved memory blocks are weighted by recency. For a block last accessed at time $t_{access}$, we compute:

$$w(t) = w_0 \cdot e^{-\lambda (t - t_{access})}$$

where:
- $w_0 = 1.0$ (initial weight)
- $\lambda$ = decay rate (default 0.5)
- $t$ = current inference step

Recently accessed memories contribute more strongly; older unreinforced memories fade exponentially.

### 2.5 Sparse Attention Gating

When injecting retrieved memory into attention, we apply a hard threshold to prevent weak token-level matches from adding noise:

$$\text{Gate}(a_{ij}) = \begin{cases} a_{ij} & \text{if } a_{ij} > \tau \\ -\infty & \text{otherwise} \end{cases}$$

where $\tau$ is the gating threshold (default 0.15). After softmax, gated scores become zero, ensuring only strongly-matching tokens contribute.

### 2.6 Weighted Attention Injection

The core mechanism: instead of inserting retrieved text into the context (displacing tokens), we inject the **attention output** directly.

For $N$ active memory blocks, the unified output is:

$$O_{final} = \text{Attention}(Q, K_{ctx}, V_{ctx}) + \sum_{k=1}^{N} \alpha_k \cdot \text{SparseAttention}(Q, K_k, V_k)$$

where:
- $\text{Attention}(Q, K_{ctx}, V_{ctx})$ = standard attention over active context
- $\text{SparseAttention}$ = attention with gating applied
- $\alpha_k = \mathcal{E}_k \cdot w_k(t)$ = combined similarity and decay weight

This allows the model to "consult" archived memories without expanding the context window.

## 3. Mathematical Summary

| Component | Formula | Purpose |
|-----------|---------|---------|
| Summary Vector | $\mathbf{s} = \frac{1}{L}\sum_i \mathbf{k}_i$ | Compress block to single vector |
| Sharpened Similarity | $\mathcal{E} = \text{ReLU}(\cos(\mathbf{q}, \mathbf{s}))^3$ | Filter weak matches |
| Query Momentum | $\mathbf{p}_t = \mathbf{q}_t + \gamma(\mathbf{q}_t - \mathbf{q}_{t-1})$ | Predict retrieval needs |
| Temporal Decay | $w(t) = e^{-\lambda(t - t_{access})}$ | Weight by recency |
| Sparse Gate | $a < \tau \rightarrow -\infty$ | Remove noisy attention |
| Unified Output | $O = O_{ctx} + \sum_k \alpha_k \cdot O_k$ | Inject memory attention |

## 4. Implementation

We implemented Z.E.T.A. in **ZetaLm**, a Swift/Metal inference engine for Apple Silicon.

### 4.1 Metal Kernels

Custom GPU kernels perform fused operations:
- `zeta_score_calc`: Computes attention scores with temporal decay bias and sparse gating in a single pass
- `zeta_kv_summarize`: Mean-pools KV blocks into summary vectors
- `zeta_superposition_inject`: Accumulates weighted memory attention into the output buffer

### 4.2 Memory Architecture

| Parameter | Value |
|-----------|-------|
| Active KV Cache | 4K tokens (Standard) / 16K tokens (Pro) |
| Max Concurrent Memory Blocks | 5 |
| Block Size | 512 tokens |
| Effective Context | $L_{active} + 5 \times L_{block}$ |

### 4.3 Compatibility

Z.E.T.A. operates at the inference layer only. It requires no model retraining and works with existing GGUF/GGML quantized models (Llama, Mistral, Qwen, etc.). The memory system wraps around the frozen model weights.

**Caveat**: Disk I/O during retrieval adds latency proportional to block size and storage speed. On NVMe, overhead is ~2-5ms per block retrieval.

## 5. Experimental Results

### 5.1 Test Configuration
- Model: TinyLlama-1.1B (Q4_0 quantization)
- Base context: 2048 tokens
- Archived blocks: 26 (~13K tokens compressed)
- Hardware: Apple M1 Pro, 16GB RAM

### 5.2 Retrieval Accuracy

| Metric | Result |
|--------|--------|
| Recall Rate | 100% (5/5 targeted queries) |
| Mean Similarity Score | 0.98 |
| False Positive Rate | 0% (no irrelevant blocks retrieved) |

The cubic sharpening effectively isolated correct memories from noise.

### 5.3 Context Extension

| Configuration | Effective Context | RAM Usage |
|---------------|-------------------|-----------|
| Baseline | 2K tokens | 1.2 GB |
| Z.E.T.A. | ~6K tokens (3x) | 1.3 GB |

Extended context with <10% RAM increase. The compressed summary vectors add negligible overhead.

### 5.4 Latency Impact

| Operation | Time |
|-----------|------|
| Summary vector comparison (26 blocks) | 0.3ms |
| Block retrieval from NVMe | 2-5ms |
| Attention injection | 0.1ms |

Total retrieval overhead: <6ms per generation step when retrieval triggers.

## 6. Applications

- **Long-form dialogue**: Agents that maintain conversation history across sessions
- **Document analysis**: Process documents exceeding context limits by archiving sections
- **Narrative generation**: Maintain plot consistency over novel-length outputs

## 7. Conclusion

Z.E.T.A. extends LLM context through three mechanisms:

1. **Hierarchical compression**: KV blocks → summary vectors + disk storage
2. **Sharpened retrieval**: Cubic cosine similarity filters noise
3. **Weighted injection**: Memory attention added directly to output

The result: 3x context extension with constant RAM usage, compatible with existing quantized models, no retraining required.

---

Z.E.T.A.™ | Patent Pending | © 2025 All rights reserved.
