# ZetaLm: Comprehensive File Inventory

**Last Updated:** December 9, 2025  
**Total Files:** 50+ source files + configs + models  
**Architecture:** Metal-first Swift inference engine with quantum-inspired optimizations

---

## 📋 Directory Structure & File Purposes

### 📦 **Root Level Configuration**

| File | Purpose | Status |
|------|---------|--------|
| `Package.swift` | Swift Package Manager manifest with dependencies | ✅ Active |
| `README.md` | Primary project documentation | ✅ Active |
| `.gitignore` | Git ignore rules | ✅ Active |
| `LICENSE` | Apache License 2.0 | ✅ Active |
| `CODE_OF_CONDUCT.md` | Community guidelines | ✅ Reference |
| `CONTRIBUTING.md` | Contribution guidelines | ✅ Reference |
| `SECURITY.md` | Security policy | ✅ Reference |
| `CHANGELOG.md` | Version history and changes | ✅ Reference |

---

## 🔧 Core Engine Files (Root)

### Primary Inference Engines

| File | Lines | Purpose | Key Features |
|------|-------|---------|--------------|
| `ZetaGGUFEngine.swift` | 600+ | **Main inference orchestrator** | • Variational gating<br>• Quantum superposition sampling<br>• Pre-allocated buffer management<br>• Multi-mode generation (optimistic, variational, ultimate)<br>• Entropy-based sparse/dense switching |
| `ZetaMPSEngine.swift` | 533 | Metal Performance Shaders hybrid engine | • MPSMatrixMultiplication<br>• MPSMatrixSoftMax<br>• Fallback lightweight kernels<br>• Experimental alternative to core engine |
| `UniversalInferenceEngine.swift` | ? | Format-agnostic inference wrapper | (see Engines/) |

### Supporting Core Files

| File | Lines | Purpose | Key Features |
|------|-------|---------|--------------|
| `ZetaModel.swift` | 263 | Model loading and weight management | • GGUF tensor parsing<br>• Q4_0, Q5_0, Q6_K dequantization<br>• KV cache initialization<br>• Tokenizer integration<br>• Sparsification parameters |
| `ZetaSampler.swift` | 150+ | Token sampling logic | • Accelerate-based vectorized sampling<br>• Entropy calculation (confusion metric)<br>• Energy/NLL calculation (variational)<br>• Top-K/Top-P filtering<br>• Banned token masking<br>• Superposition amplitude tracking |
| `ZetaTokenizer.swift` | ? | BPE tokenizer | • GGUF-compatible tokenization<br>• Encoding/decoding<br>• Special tokens handling |
| `QuantumLayers.swift` | ? | Quantum-inspired layer implementations | • Tunneling probability<br>• Resonance thresholds<br>• Sparsification config |
| `GGUFStubs.swift` | ? | GGUF format definitions | • Struct stubs for GGUF spec |
| `CompressionSupport.swift` | 255 | Model compression utilities | • Quantization helpers<br>• Sparsification analysis |
| `ModelFormatConverter.swift` | ? | Format conversion tools | • GGUF ↔ MLX<br>• Quantization-aware handling |

---

## 🎯 Metal Kernel Files (`Kernels/`)

### Metal Shader Kernels

| File | Purpose | Key Kernels |
|------|---------|-------------|
| `ZetaGGUFKernels.metal` | Core inference kernels | • `zeta_rms_norm_row` (RMS normalization)<br>• `zeta_q4_matmul` (quantized matmul)<br>• `zeta_float_matmul` (full precision matmul)<br>• `zeta_rope` (rotary positional embeddings)<br>• `zeta_score_calc` (attention scoring with Fine's theorem)<br>• `zeta_softmax` (attention softmax)<br>• `zeta_weighted_sum` (attention aggregation)<br>• `zeta_silu` (activation)<br>• `zeta_elem_mul` (element-wise multiply)<br>• `zeta_add` (element-wise add)<br>• `zeta_cache_update` (KV cache management)<br>• `zeta_gpu_sparsify` (GPU-accelerated sparsification)<br>• `zeta_q4_matmul_simd` (SIMD-optimized quantized matmul) |
| `ZetaOptimizedKernels.metal` | Advanced optimized kernels | • `zeta_fused_rmsnorm_matmul` (fused norm+matmul)<br>• `zeta_score_calc_simd` (SIMD attention scoring)<br>• Kernel fusion patterns |
| `ZetaUltraKernels.metal` | Extreme performance kernels | • `zeta_matmul_simdgroup_matrix` (AMX-accelerated matmul)<br>• `zeta_matmul_tiled` (tiled matmul)<br>• `zeta_flash_attention_v2` (Flash Attention)<br>• `zeta_fused_norm_rope_matmul` (3-way fusion)<br>• `zeta_swiglu_ultra` (fused gate/up/mul)<br>• `zeta_attention_async` (async operations) |

**Status:** Comprehensive kernel coverage with advanced optimizations

---

## 🚀 CLI & Executable Files (`CLI/`)

| File | Purpose | Arguments |
|------|---------|-----------|
| `main.swift` | Primary CLI interface | `--model <path>`, `--prompt <text>`, `--steps N`, `--variational <ε>`, `--ultimate`, `--allow-unk`, `--bench`, `--topk K`, `--topp P` |
| `ZetaBenchmarkCLI.swift` | GPU benchmarking tool | Model path, prompt, `--steps N`, `--zeta-on/off` |
| `ZetaSurgeryCLI.swift` | Sparsification tool | Input model, output path, sparsity config |

---

## 📂 Loader Files (`Loaders/`)

### Universal Model Format Support

| File | Format | Status | Purpose |
|------|--------|--------|---------|
| `UniversalModelLoader.swift` | Protocol/Registry | ✅ Core | Base loader interface & factory pattern |
| `MLXUniversalLoader.swift` | MLX/Safetensors | ✅ Active | Apple MLX format support |
| `ONNXUniversalLoader.swift` | ONNX | ⚠️ Partial | Basic ONNX loading |
| `PyTorchUniversalLoader.swift` | PyTorch | 🔧 In Progress | PyTorch model support |
| `TensorFlowUniversalLoader.swift` | TensorFlow | 🔧 Planned | TensorFlow format support |
| `SafetensorsUniversalLoader.swift` | Safetensors | 🔧 Planned | Hugging Face safetensors format |
| `CoreMLUniversalLoader.swift` | CoreML | 🔧 Planned | Apple CoreML format |
| `StreamingModelLoader.swift` | Streaming I/O | 🔧 Planned | Memory-efficient streaming load |
| `ShardedModelLoader.swift` | Sharded models | 🔧 Planned | Multi-file model support |
| `UNIVERSAL_LOADER_README.md` | Documentation | ✅ Reference | Loader architecture guide |

---

## 🔌 Format Parsers & Integrations

| File | Purpose | Status |
|------|---------|--------|
| `ONNXProtobufParser.swift` | ONNX protobuf parsing | ⚠️ Partial |
| `ONNXRuntimeIntegration.swift` | ONNX Runtime bridge | 🔧 Experimental |
| `GGUFStubs.swift` | GGUF format stubs | ✅ Active |

---

## 📊 Test & Benchmark Files (`Tests/`)

### Test Suite

| File | Type | Purpose |
|------|------|---------|
| `ZetaAttentionTests.swift` | Unit | Test attention mechanism correctness |
| `ZetaInferenceTests.swift` | Integration | End-to-end inference validation |
| `ZetaQ4KernelTests.swift` | Unit | Q4_0 quantization kernel tests |
| `ZetaTests.swift` | Unit | General component tests |
| `test_init.swift` | Integration | Engine initialization |
| `test_loader.swift` | Integration | Universal loader testing |
| `test_metal.swift` | Unit | Metal kernel validation |
| `test_model.swift` | Unit | Model loading & parsing |

### Benchmark Scripts

| File | Purpose | What It Tests |
|------|---------|---------------|
| `benchmark_throughput.sh` | Standard throughput | Tokens/sec with standard, optimized, and high-throughput modes |
| `benchmark_against_llamacpp.sh` | Comparative | Performance vs. llama.cpp baseline |
| `benchmark_fine_theory.sh` | Quantum metrics | Fine's Theorem quantum sparsification effectiveness |
| `quick_variational_test.sh` | Fast smoke test | Variational gating mode |
| `test_coherence_simple.sh` | Output quality | Generation coherence measurement |
| `test_variational_coherence.sh` | Variational quality | Coherence under energy-based gating |

---

## 🧠 Model Files (`models/`)

### PyTorch Models

```
models/pytorch/
├── TinyLlama-1.1B-Chat-v1.0/
│   ├── model.safetensors        (1.1B params, full precision)
│   ├── config.json              (model architecture)
│   ├── generation_config.json    (sampling params)
│   ├── special_tokens_map.json   (special token defs)
│   ├── tokenizer.json           (BPE vocabulary)
│   ├── tokenizer_config.json     (tokenizer settings)
│   └── chat_template.jinja      (chat prompt template)
│
└── Zeta-Sparse-TinyLlama/
    └── (Same structure, 28-42% sparsified version)
```

### ONNX Models

```
models/tinyllama-1.1b-chat-v1.0-onnx/
├── onnx/
│   ├── model.onnx                    (main model)
│   ├── model.onnx_data               (tensor data)
│   ├── decoder_model_merged.onnx     (decoder variant)
│   ├── decoder_model_merged_quantized.onnx
│   ├── model_fp16.onnx               (FP16 variant)
│   ├── model_int8.onnx               (INT8 quantized)
│   ├── model_q4.onnx                 (Q4 quantized)
│   ├── model_bnb4.onnx               (bitsandbytes 4-bit)
│   └── ...                           (other quantization variants)
├── config.json
├── generation_config.json
├── special_tokens_map.json
├── tokenizer.json
└── README.md
```

### GGUF Configurations

```
models/
└── tinyllama-zeta.gguf.zeta-config    (Sparsification parameters)
    ├── zeta_s: 0.5
    ├── resonance_threshold: 0.3
    ├── tunneling_probability: 0.001
    └── target_sparsity: 0.28
```

---

## 🔧 Tools & Scripts (`Tools/`)

| File | Purpose |
|------|---------|
| `zeta_surgery.py` | Python script for Zeta-sparse pruning on PyTorch models |
| `ONNXUniversalLoader.swift` | Alternative ONNX loader implementation |

---

## 📄 Documentation Files

| File | Content | Audience |
|------|---------|----------|
| `README.md` | Main project overview | Everyone |
| `CONTRIBUTING.md` | Developer contribution guide | Contributors |
| `CODE_OF_CONDUCT.md` | Community standards | Community |
| `SECURITY.md` | Security policy & reporting | Security researchers |
| `CHANGELOG.md` | Version history | Maintainers, users |
| `FILE_INVENTORY.md` | This file | Developers, architects |
| `Loaders/UNIVERSAL_LOADER_README.md` | Loader architecture | Backend developers |

---

## 🏗️ File Organization by Function

### **Inference Path (User Query → Generation)**
```
CLI/main.swift
    ↓
ZetaGGUFEngine.swift (main orchestrator)
    ├─→ ZetaModel.swift (weights loading)
    ├─→ Kernels/*.metal (GPU compute)
    ├─→ ZetaSampler.swift (token selection)
    └─→ ZetaTokenizer.swift (decode to text)
```

### **Model Loading Path (File → Metal Buffers)**
```
Loaders/UniversalModelLoader.swift (factory)
    ├─→ MLXUniversalLoader.swift (if .mlx)
    ├─→ ONNXUniversalLoader.swift (if .onnx)
    ├─→ PyTorchUniversalLoader.swift (if .pt/.pth)
    └─→ (Other format loaders)
        ↓
    ZetaModel.swift (GGUF parsing)
        ↓
    GPU: Metal buffers via ZetaGGUFEngine.swift
```

### **Testing Path**
```
Tests/*.swift (unit tests)
    ↓
Tests/*.sh (integration benchmarks)
    ↓
Results compared against llama.cpp baseline
```

---

## 📊 File Statistics Summary

| Category | Count | Notes |
|----------|-------|-------|
| **Core Engine Files** | 5 | GGUF engine, MPS engine, model, sampler, tokenizer |
| **Metal Kernels** | 3 | Core, optimized, ultra kernels |
| **Loaders** | 8 | GGUF, MLX, ONNX, PyTorch, TF, Safetensors, CoreML, Streaming/Sharded |
| **CLI/Tools** | 3 | Main CLI, benchmark, surgery |
| **Tests** | 11 | Unit tests + shell scripts |
| **Documentation** | 8 | README, guides, policies |
| **Model Files** | 20+ | PyTorch safetensors, ONNX variants, configs, tokenizers |

---

## 🔑 Key Implementation Locations

### Quantum Features

| Feature | Primary File | Kernel |
|---------|--------------|--------|
| **Zeta Resonant Recall** | `ZetaGGUFEngine.swift` (`encodeZetaAttention`) | `zeta_score_calc`, `zeta_softmax` |
| **Quantum Tunneling** | `ZetaGGUFKernels.metal` | `zeta_score_calc` (Bell zone logic) |
| **Fine's Theorem Sparsification** | `ZetaGGUFKernels.metal` | `zeta_gpu_sparsify` |
| **Quantum Superposition Sampling** | `ZetaSampler.swift` (`sampleSuperposition`) | CPU-based amplitude tracking |
| **Variational Gating** | `ZetaGGUFEngine.swift` (`variationalGate`) | Dual sparse/dense forward pass |
| **Entropy Calculation** | `ZetaSampler.swift` (`calculateEntropy`) | Accelerate framework |
| **Energy/NLL Calculation** | `ZetaSampler.swift` (`calculateNegativeLogLikelihood`) | Accelerate framework |

### Metal Optimizations

| Optimization | Files | Approach |
|--------------|-------|----------|
| **SIMD Vectorization** | `ZetaOptimizedKernels.metal`, `ZetaSampler.swift` | `float4`, `simd_sum`, NEON |
| **Kernel Fusion** | `ZetaOptimizedKernels.metal`, `ZetaUltraKernels.metal` | Combined ops (norm+matmul, gate+mul, etc.) |
| **Flash Attention** | `ZetaUltraKernels.metal` | Tiled attention with online softmax |
| **AMX Acceleration** | `ZetaUltraKernels.metal` | `simdgroup_matrix` for matmul |
| **Command Buffer Batching** | `ZetaGGUFEngine.swift` (`forwardPass`) | Single command buffer per token |
| **Buffer Pooling** | `ZetaGGUFEngine.swift` (`PreallocatedBuffers`) | Pre-allocated persistent buffers |

---

## ⚠️ File Status & Maintenance Notes

### Active & Stable ✅
- `ZetaGGUFEngine.swift` - Core inference engine
- `ZetaModel.swift` - Model loading
- `ZetaSampler.swift` - Sampling logic
- `ZetaGGUFKernels.metal` - Essential kernels
- All CLI tools
- All test files

### Experimental ⚠️
- `ZetaMPSEngine.swift` - Alternative to main engine
- `ZetaOptimizedKernels.metal` - Advanced but not always used
- `ZetaUltraKernels.metal` - Extreme optimizations
- `ONNXRuntimeIntegration.swift` - Not fully tested

### In Progress 🔧
- Several loaders in `Loaders/` directory
- `StreamingModelLoader.swift` - Not yet production-ready
- `ShardedModelLoader.swift` - Not yet production-ready
- `PyTorchUniversalLoader.swift` - Partial support

### Deprecated/Removed ❌
- Old Metal kernel files (combined into Kernels/)
- Legacy optimizations

---

## 📝 Notes for Development

1. **File Organization**: Code is split between root level (for now) and subdirectories (Engines/, Kernels/, Loaders/) for scalability
2. **Metal Compilation**: All `.metal` files are resources in `Package.swift` and compiled at runtime
3. **Loader Pattern**: Using Swift protocol + factory for extensible model format support
4. **Buffer Management**: Pre-allocated buffers reused across forward passes for zero hot-path allocation
5. **Testing**: Comprehensive test suite with both unit tests and shell-based benchmarks
6. **Documentation**: README and CONTRIBUTING guide users; inline comments document quantum features

---

**Generated:** December 9, 2025  
**Maintainer:** ZetaLm Team  
**License:** Apache License 2.0




















