# Z.E.T.A. Model Family Analysis

> **Generated:** December 16, 2025  
> **Purpose:** Optimal model configurations for Z.E.T.A. architecture across different VRAM budgets

---

## Table of Contents
- [Architecture Overview](#architecture-overview)
- [Model Family Comparison](#model-family-comparison)
- [16GB Configurations](#16gb-configurations-rtx-4060-ti-16gb--similar)
- [24GB Configurations](#24gb-configurations-rtx-4090--a5000--similar)
- [Quantization Guide](#quantization-guide)
- [Recommendations](#recommendations)

---

## Architecture Overview

Z.E.T.A. uses a multi-model architecture:

```
┌─────────────────────────────────────────┐
│           Z.E.T.A. Server               │
├─────────────────────────────────────────┤
│  GENERATIVE MODEL (Main Brain)          │
│  └─ Handles reasoning, generation       │
├─────────────────────────────────────────┤
│  EXTRACTOR MODEL (Code Specialist)      │
│  └─ Handles code extraction, analysis   │
├─────────────────────────────────────────┤
│  EMBEDDING MODEL (Optional)             │
│  └─ Semantic search, memory retrieval   │
└─────────────────────────────────────────┘
```

### Critical Constraints
- **Tokenizer Compatibility:** Models MUST share tokenizers or be same family
- **Working Memory:** Z.E.T.A. caps at ~200MB before aggressive eviction
- **VRAM Budget:** Leave 1-2GB headroom for KV cache overhead

---

## Model Family Comparison

### Families with Code-Specialized Variants

| Family | Code Variant | Sizes Available | Embedding Model | Notes |
|--------|--------------|-----------------|-----------------|-------|
| **Qwen 2.5** | ✅ Qwen-Coder | 0.5B-72B | GTE-Qwen | Best ecosystem for code |
| **DeepSeek-V2** | ✅ Native code | 16B-Lite (MoE) | ❌ None | MoE efficiency, 128K context |
| **CodeLlama** | ✅ Native code | 7B/13B/34B | ❌ None | Mature, Llama 2 based |

### General-Purpose Families

| Family | Sizes Available | Embedding Model | Notes |
|--------|-----------------|-----------------|-------|
| **Llama 3.1** | 8B/70B/405B | ❌ None | Fast, proven |
| **Mistral** | 7B/12B Nemo/22B Small | ❌ None | Multilingual |
| **Gemma 2** | 2B/9B/27B | ❌ None | No system prompt |
| **Phi-3** | 4B mini/14B medium | ❌ None | 128K native context |
| **Yi 1.5** | 6B/9B/34B | ❌ None | Llama tokenizer |

---

## 16GB Configurations (RTX 4060 Ti 16GB / Similar)

**Budget:** 15.25GB usable (after driver overhead + KV cache)

### Qwen 2.5 Family (RECOMMENDED)

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **Current** | 14B Q4_K_M (8.99GB) | 7B-Coder Q4_K_M (4.4GB) | 13.4GB | 1.85GB |
| **Upgraded** | 14B Q5_K_M (10.5GB) | 7B-Coder Q4_K_M (4.4GB) | 14.9GB | 350MB 🔥 |
| **Safe** | 14B Q4_K_M (8.99GB) | 7B-Coder Q5_K_M (~5.0GB) | ~14GB | 1.25GB |

### DeepSeek-Coder-V2 Family

| Config | Model | Total | Margin |
|--------|-------|-------|--------|
| **Solo Q4** | 16B-Lite Q4_K_M (10.36GB) | 10.36GB | 4.9GB |
| **Solo IQ4** | 16B-Lite IQ4_XS (8.57GB) | 8.57GB | 6.7GB |

### CodeLlama Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **13B+7B Q4** | 13B Q4_K_M (7.87GB) | 7B Q4_K_M (4.08GB) | 11.95GB | 3.3GB |
| **13B Q6 + 7B Q4** | 13B Q6_K (10.68GB) | 7B Q4_K_M (4.08GB) | 14.76GB | 490MB 🔥 |
| **13B Q8 Solo** | 13B Q8_0 (13.83GB) | - | 13.83GB | 1.42GB |

### Llama 3.1 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **Twin Q6** | 8B Q6_K (6.6GB) | 8B Q6_K (6.6GB) | 13.2GB | 2.05GB |
| **Twin Q5** | 8B Q5_K_M (5.73GB) | 8B Q5_K_M (5.73GB) | 11.46GB | 3.8GB |

### Gemma 2 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **9B+9B Q5** | 9B Q5_K_M (6.65GB) | 9B Q5_K_M (6.65GB) | 13.3GB | 1.95GB |
| **9B+9B Q6** | 9B Q6_K (7.59GB) | 9B Q6_K (7.59GB) | 15.18GB | 70MB ⚠️ |

### Phi-3 Family

| Config | Model | Total | Margin |
|--------|-------|-------|--------|
| **14B Q5 Solo** | 14B Q5_K_M (10.07GB) | 10.07GB | 5.2GB |
| **14B Q6 Solo** | 14B Q6_K (11.45GB) | 11.45GB | 3.8GB |
| **14B Q8 Solo** | 14B Q8_0 (14.83GB) | 14.83GB | 420MB 🔥 |

### Mistral Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **Nemo+7B** | 12B Nemo Q4_K_M (7.48GB) | 7B Q4 (~4.1GB) | ~11.6GB | 3.65GB |
| **22B Solo IQ3** | Small-22B IQ3_M (10.06GB) | - | 10.06GB | 5.2GB |
| **22B Solo IQ4** | Small-22B IQ4_XS (11.94GB) | - | 11.94GB | 3.3GB |

### Yi 1.5 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **9B+9B Q5** | 9B Q5_K_M (6.25GB) | 9B Q5_K_M (6.25GB) | 12.5GB | 2.75GB |
| **9B+9B Q6** | 9B Q6_K (7.24GB) | 9B Q6_K (7.24GB) | 14.48GB | 770MB |

---

## 24GB Configurations (RTX 4090 / A5000 / Similar)

**Budget:** ~22-23GB usable (after driver overhead + KV cache)

### Qwen 2.5 Family (RECOMMENDED)

| Config | Generative | Extractor | Embed | Total | Margin |
|--------|------------|-----------|-------|-------|--------|
| **BEAST** | 14B Q8_0 (15.7GB) | 7B-Coder Q4_K_M (4.4GB) | - | 20.1GB | 2.9GB |
| **ULTRA** | 14B Q6_K (12.1GB) | 7B-Coder Q6_K (~6.0GB) | 1.5B GTE (1.1GB) | 19.2GB | 3.8GB |
| **MAX** | 14B Q8_0 (15.7GB) | 7B-Coder Q5_K_M (~5.0GB) | - | 20.7GB | 2.3GB |
| **TRIPLE** | 14B Q5_K_M (10.5GB) | 7B-Coder Q5_K_M (~5.0GB) | 4B GTE Q4 (2.4GB) | 17.9GB | 5.1GB |

### DeepSeek-Coder-V2 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **16B+16B Q4** | 16B-Lite Q4_K_M (10.36GB) | 16B-Lite Q4_K_M (10.36GB) | 20.72GB | 2.3GB |
| **16B Q5 + 16B Q4** | 16B-Lite Q5_K_M (11.85GB) | 16B-Lite Q4_K_M (10.36GB) | 22.21GB | 800MB 🔥 |
| **16B Q6 Solo** | 16B-Lite Q6_K (14.06GB) | - | 14.06GB | 8.9GB |
| **16B Q8 Solo** | 16B-Lite Q8_0 (16.70GB) | - | 16.70GB | 6.3GB |

### CodeLlama Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **13B+13B Q6** | 13B Q6_K (10.68GB) | 13B Q6_K (10.68GB) | 21.36GB | 1.6GB |
| **13B+13B Q5** | 13B Q5_K_M (9.23GB) | 13B Q5_K_M (9.23GB) | 18.46GB | 4.5GB |
| **13B Q8 + 7B Q8** | 13B Q8_0 (13.83GB) | 7B Q8_0 (7.16GB) | 20.99GB | 2GB |
| **13B+7B Q8** | 13B Q8_0 (13.83GB) | 7B Q6_K (5.53GB) | 19.36GB | 3.6GB |

### Llama 3.1 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **Twin Q8** | 8B Q8_0 (8.54GB) | 8B Q8_0 (8.54GB) | 17.08GB | 5.9GB |
| **8B Q8 + 8B Q8 + Buffer** | 8B Q8_0 (8.54GB) | 8B Q8_0 (8.54GB) | 17.08GB | Lots of context! |

### Gemma 2 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **9B+9B Q8** | 9B Q8_0 (9.83GB) | 9B Q8_0 (9.83GB) | 19.66GB | 3.3GB |
| **27B IQ4 Solo** | 27B IQ4_XS (14.81GB) | - | 14.81GB | 8.2GB |
| **27B Q4 Solo** | 27B Q4_K_S (15.74GB) | - | 15.74GB | 7.3GB |
| **27B Q4 + 9B Q4** | 27B Q4_K_S (15.74GB) | 9B Q4_K_M (5.76GB) | 21.5GB | 1.5GB |

### Phi-3 Family

| Config | Model | Total | Margin |
|--------|-------|-------|--------|
| **14B Q8 + 4B mini Q8** | 14B Q8 (14.83GB) + mini Q8 (~4.5GB) | ~19.3GB | 3.7GB |
| **Twin 14B Q5** | 14B Q5_K_M × 2 (20.14GB) | 20.14GB | 2.8GB |

### Mistral Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **22B Q4 Solo** | Small-22B Q4_K_M (13.34GB) | - | 13.34GB | 9.6GB |
| **22B Q5 Solo** | Small-22B Q5_K_M (15.72GB) | - | 15.72GB | 7.3GB |
| **22B Q4 + 7B Q4** | Small-22B Q4_K_M (13.34GB) | 7B Q4 (~4.1GB) | 17.44GB | 5.5GB |
| **Nemo+Nemo Q8** | 12B Nemo Q8 × 2 | ~19GB | 4GB |

### Yi 1.5 Family

| Config | Generative | Extractor | Total | Margin |
|--------|------------|-----------|-------|--------|
| **9B+9B Q8** | 9B Q8_0 (9.38GB) | 9B Q8_0 (9.38GB) | 18.76GB | 4.2GB |

---

## Quantization Guide

### Quality vs Size Tradeoffs

| Quant | Bits | Quality | Use Case |
|-------|------|---------|----------|
| **Q8_0** | 8-bit | ⭐⭐⭐⭐⭐ | Max quality, if VRAM allows |
| **Q6_K** | 6-bit | ⭐⭐⭐⭐½ | Near-perfect, recommended |
| **Q5_K_M** | 5-bit | ⭐⭐⭐⭐ | High quality, sweet spot |
| **Q4_K_M** | 4-bit | ⭐⭐⭐½ | Good quality, most common |
| **IQ4_XS** | 4-bit | ⭐⭐⭐½ | Same as Q4, slightly smaller |
| **Q3_K_M** | 3-bit | ⭐⭐⭐ | Noticeable degradation |
| **IQ3_M** | 3-bit | ⭐⭐⭐ | Better than Q3 at same size |
| **Q2_K** | 2-bit | ⭐⭐ | Emergency only |

### Impact on Different Tasks

| Task | Quant Sensitivity | Notes |
|------|-------------------|-------|
| Code extraction | Medium | Q4+ is fine for syntax |
| Complex reasoning | High | Q5+ noticeably sharper |
| Semantic routing | Low | Even Q4 handles well |
| Numerical accuracy | High | Q5+ better for math |
| Long-form coherence | High | Q5+ maintains thread better |

---

## Recommendations

### For 16GB (RTX 4060 Ti 16GB / M2 Max)

| Priority | Family | Config | Total | Why |
|----------|--------|--------|-------|-----|
| **#1** | Qwen 2.5 | 14B Q5 + 7B-Coder Q4 | 14.9GB | Best code extractor ecosystem |
| **#2** | DeepSeek-V2 | 16B-Lite Q4 Solo | 10.4GB | MoE efficiency, huge context |
| **#3** | CodeLlama | 13B Q6 + 7B Q4 | 14.76GB | Both models code-native |
| **#4** | Phi-3 | 14B Q8 Solo | 14.83GB | 128K context, single model |

### For 24GB (RTX 4090 / A5000)

| Priority | Family | Config | Total | Why |
|----------|--------|--------|-------|-----|
| **#1** | Qwen 2.5 | 14B Q8 + 7B-Coder Q5 | 20.7GB | Max quality Qwen stack |
| **#2** | DeepSeek-V2 | 16B+16B Q4 | 20.7GB | Dual code-native MoE |
| **#3** | CodeLlama | 13B Q8 + 7B Q8 | 21GB | Both at max quant |
| **#4** | Gemma 2 | 27B Q4 + 9B Q4 | 21.5GB | Biggest single brain |
| **#5** | Mistral | 22B Q5 Solo | 15.7GB | Headroom for massive context |

---

## Model Size Reference Table

### Qwen 2.5 14B Instruct
| Quant | Size |
|-------|------|
| Q4_K_M | 8.99GB |
| Q5_K_M | 10.5GB |
| Q6_K | 12.1GB |
| Q8_0 | 15.7GB |

### Qwen 2.5 7B Coder
| Quant | Size |
|-------|------|
| Q4_K_M | 4.4GB |
| Q5_K_M | ~5.0GB |
| Q6_K | ~6.0GB |

### DeepSeek-Coder-V2-Lite (16B MoE)
| Quant | Size |
|-------|------|
| IQ4_XS | 8.57GB |
| Q4_K_M | 10.36GB |
| Q5_K_M | 11.85GB |
| Q6_K | 14.06GB |
| Q8_0 | 16.70GB |

### CodeLlama 13B
| Quant | Size |
|-------|------|
| Q4_K_M | 7.87GB |
| Q5_K_M | 9.23GB |
| Q6_K | 10.68GB |
| Q8_0 | 13.83GB |

### CodeLlama 7B
| Quant | Size |
|-------|------|
| Q4_K_M | 4.08GB |
| Q5_K_M | 4.78GB |
| Q6_K | 5.53GB |
| Q8_0 | 7.16GB |

### Llama 3.1 8B
| Quant | Size |
|-------|------|
| Q4_K_M | 4.92GB |
| Q5_K_M | 5.73GB |
| Q6_K | 6.60GB |
| Q8_0 | 8.54GB |

### Gemma 2 9B
| Quant | Size |
|-------|------|
| Q4_K_M | 5.76GB |
| Q5_K_M | 6.65GB |
| Q6_K | 7.59GB |
| Q8_0 | 9.83GB |

### Gemma 2 27B
| Quant | Size |
|-------|------|
| IQ4_XS | 14.81GB |
| Q4_K_S | 15.74GB |
| Q4_K_M | 16.65GB |
| Q5_K_M | 19.41GB |

### Phi-3 Medium 14B
| Quant | Size |
|-------|------|
| Q4_K_M | 8.57GB |
| Q5_K_M | 10.07GB |
| Q6_K | 11.45GB |
| Q8_0 | 14.83GB |

### Mistral Small 22B
| Quant | Size |
|-------|------|
| IQ3_M | 10.06GB |
| IQ4_XS | 11.94GB |
| Q4_K_M | 13.34GB |
| Q5_K_M | 15.72GB |

### Mistral Nemo 12B
| Quant | Size |
|-------|------|
| Q4_K_M | 7.48GB |
| Q5_K_M | 8.73GB |

### Yi 1.5 9B
| Quant | Size |
|-------|------|
| Q4_K_M | 5.32GB |
| Q5_K_M | 6.25GB |
| Q6_K | 7.24GB |
| Q8_0 | 9.38GB |

---

## Embedding Models

| Model | Size | Family Compatible |
|-------|------|-------------------|
| GTE-Qwen2-1.5B Q4 | 1.1GB | Qwen |
| Qwen3-Embedding-4B Q4 | 2.4GB | Qwen |
| BGE-Small-EN Q8 | 36MB | Any (cross-family) |
| BGE-Large-EN Q8 | ~1.3GB | Any (cross-family) |

**Note:** Cross-family embeddings work for retrieval but won't share tokenizer benefits.

---

## Conclusion

For Z.E.T.A.'s code-focused extraction architecture:

1. **Qwen 2.5** remains the optimal choice due to:
   - Dedicated Coder variant
   - Same tokenizer across all sizes
   - Native embedding model (GTE)
   
2. **DeepSeek-V2-Lite** is the dark horse:
   - MoE architecture = more params per GB
   - Code-native training
   - 128K context window
   
3. **CodeLlama** is the safe bet:
   - Proven code performance
   - Both generative and extractor are code-trained
   - Mature ecosystem

**24GB unlocks Q8 quants** which provide noticeable improvements in:
- Complex multi-step reasoning
- Numerical accuracy
- Long-form coherence
- Edge case handling
