import Foundation
import Metal
import MetalPerformanceShaders

/// ZetaCore: Apple Native LLM Inference Engine
/// Phase 1: Port llama.cpp math to Metal - MUST produce identical output
public final class ZetaCoreEngine {

    // MARK: - Metal Infrastructure
    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private let library: MTLLibrary

    // MARK: - Compute Pipelines (Core)
    private var q4MatMulPipeline: MTLComputePipelineState!
    private var f32MatMulPipeline: MTLComputePipelineState!
    private var q6kDequantPipeline: MTLComputePipelineState!
    private var rmsnormPipeline: MTLComputePipelineState!
    private var rmsnormParallelPipeline: MTLComputePipelineState!  // Optimized parallel version
    private var ropePipeline: MTLComputePipelineState!
    private var ropeLutPipeline: MTLComputePipelineState!          // Optimized RoPE with precomputed frequencies
    private var softmaxPipeline: MTLComputePipelineState!
    private var softmaxParallelPipeline: MTLComputePipelineState!  // Optimized parallel softmax
    private var swigluPipeline: MTLComputePipelineState!
    private var addPipeline: MTLComputePipelineState!
    private var embeddingPipeline: MTLComputePipelineState!
    private var attnScoresPipeline: MTLComputePipelineState!
    private var attnOutputPipeline: MTLComputePipelineState!
    private var copyPipeline: MTLComputePipelineState!
    private var q4DequantPipeline: MTLComputePipelineState!

    // MARK: - Compute Pipelines (Z.E.T.A. Features)
    private var entanglementPipeline: MTLComputePipelineState!      // F3: ReLU(cos)³
    private var semanticMomentumPipeline: MTLComputePipelineState!  // F4: q + γ(q - q_prev)
    private var memoryAccumulatePipeline: MTLComputePipelineState!  // F5: Superposition
    private var kvSummarizePipeline: MTLComputePipelineState!       // F6: Sublimation prep
    private var argmaxPipeline: MTLComputePipelineState!            // GPU sampling

    // MARK: - Sampling Buffer
    private var argmaxResultBuffer: MTLBuffer?

    // MARK: - Precomputed LUTs
    private var ropeFreqLUT: MTLBuffer?  // Precomputed RoPE frequencies [head_dim/2]

    // MARK: - Model State
    private var model: ZetaModel?
    private var buffers: InferenceBuffers?

    // MARK: - Dequantized weight cache
    private var embeddingBuffer: MTLBuffer?   // For embedding lookup (token_embd.weight)
    private var outputProjBuffer: MTLBuffer?  // For output projection (output.weight)

    // MARK: - Config
    public private(set) var dim: Int = 0
    public private(set) var hiddenDim: Int = 0
    public private(set) var nLayers: Int = 0
    public private(set) var nHeads: Int = 0
    public private(set) var nKVHeads: Int = 0
    public private(set) var headDim: Int = 0
    public private(set) var vocabSize: Int = 0
    public private(set) var maxContextLength: Int = 4096
    private var rmsNormEps: Float = 1e-5
    private var ropeTheta: Float = 10000.0

    // MARK: - Z.E.T.A. Features
    /// Feature 1: Temporal Decay rate λ - controls how much older tokens are down-weighted
    /// Z(t) = Z₀ · e^(-λt) where t = distance from current position
    /// Default 0.1 = gentle decay, 0.0 = disabled, higher = stronger recency bias
    public var temporalLambda: Float = 0.1

    /// Feature 2: Tunneling Gate threshold τ - creates sparse attention
    /// Scores below τ are set to -∞ (become 0 after softmax)
    /// Default 0.0 = disabled, positive values enable sparsification
    public var tunnelingTau: Float = 0.0

    /// Feature 4: Semantic Momentum coefficient γ
    /// I(t) = q + γ(q - q_prev) - predicts semantic drift direction
    /// Default 0.5 = moderate momentum, 0.0 = disabled
    public var momentumGamma: Float = 0.5

    // MARK: - Init
    public init() throws {
        guard let device = MTLCreateSystemDefaultDevice() else {
            throw ZetaError.noMetalDevice
        }
        self.device = device

        guard let queue = device.makeCommandQueue() else {
            throw ZetaError.commandQueueFailed
        }
        self.commandQueue = queue

        // Load and compile Metal shaders from source
        self.library = try Self.compileShaders(device: device)

        try loadPipelines()
        print("✅ ZetaCore initialized (Metal Native)")
    }

    // Compile Metal shaders from source files in bundle
    private static func compileShaders(device: MTLDevice) throws -> MTLLibrary {
        var shaderSource = ""

        let shaderNames = ["ZetaQuants", "ZetaAttention", "ZetaFFN"]
        for name in shaderNames {
            if let url = Bundle.module.url(forResource: name, withExtension: "metal"),
               let source = try? String(contentsOf: url, encoding: .utf8) {
                shaderSource += "\n// === \(name).metal ===\n"
                shaderSource += source
            }
        }

        guard !shaderSource.isEmpty else {
            throw ZetaError.shaderLoadFailed
        }

        let options = MTLCompileOptions()
        options.fastMathEnabled = true

        do {
            let library = try device.makeLibrary(source: shaderSource, options: options)
            print("✅ Compiled Metal shaders from source")
            return library
        } catch {
            print("❌ Shader compilation error: \(error)")
            throw ZetaError.shaderLoadFailed
        }
    }

    // MARK: - Pipeline Setup
    private func loadPipelines() throws {
        // Core inference pipelines
        q4MatMulPipeline = try makePipeline("zeta_q4_matmul")
        f32MatMulPipeline = try makePipeline("zeta_f32_matmul")
        q6kDequantPipeline = try makePipeline("zeta_q6k_dequant")
        rmsnormPipeline = try makePipeline("zeta_rmsnorm")
        rmsnormParallelPipeline = try makePipeline("zeta_rmsnorm_parallel")
        ropePipeline = try makePipeline("zeta_rope")
        ropeLutPipeline = try makePipeline("zeta_rope_lut")
        softmaxPipeline = try makePipeline("zeta_softmax")
        softmaxParallelPipeline = try makePipeline("zeta_softmax_parallel")
        swigluPipeline = try makePipeline("zeta_swiglu")
        addPipeline = try makePipeline("zeta_add")
        embeddingPipeline = try makePipeline("zeta_embedding")
        attnScoresPipeline = try makePipeline("zeta_attn_scores")
        attnOutputPipeline = try makePipeline("zeta_attn_output")
        copyPipeline = try makePipeline("zeta_copy")
        q4DequantPipeline = try makePipeline("zeta_q4_dequant")

        // Z.E.T.A. Feature pipelines
        entanglementPipeline = try makePipeline("zeta_entanglement")
        semanticMomentumPipeline = try makePipeline("zeta_semantic_momentum")
        memoryAccumulatePipeline = try makePipeline("zeta_memory_accumulate")
        kvSummarizePipeline = try makePipeline("zeta_kv_summarize")

        // Sampling pipeline
        argmaxPipeline = try makePipeline("zeta_argmax")

        // Allocate argmax result buffer (single uint32)
        argmaxResultBuffer = device.makeBuffer(length: 4, options: .storageModeShared)

        print("✅ All compute pipelines loaded")
    }

    private func makePipeline(_ name: String) throws -> MTLComputePipelineState {
        guard let function = library.makeFunction(name: name) else {
            throw ZetaError.functionNotFound(name)
        }
        return try device.makeComputePipelineState(function: function)
    }

    // MARK: - Model Loading
    public func loadModel(path: String) throws {
        let loader = ZetaCoreLoader(device: device)
        self.model = try loader.load(path: path)

        guard let model = self.model else { throw ZetaError.modelLoadFailed }
        self.dim = model.dim
        self.hiddenDim = model.hiddenDim
        self.nLayers = model.nLayers
        self.nHeads = model.nHeads
        self.nKVHeads = model.nKVHeads
        self.headDim = model.headDim
        self.vocabSize = model.vocabSize
        self.rmsNormEps = model.rmsNormEps
        self.ropeTheta = model.ropeTheta

        // Allocate inference buffers
        self.buffers = try InferenceBuffers(device: device, dim: dim, hiddenDim: hiddenDim,
                                            kvDim: nKVHeads * headDim, vocabSize: vocabSize,
                                            nLayers: nLayers, contextLen: maxContextLength)

        // Prepare embedding and output projection buffers
        try prepareEmbeddingBuffer()
        try prepareOutputProjBuffer()

        // Precompute RoPE frequency LUT (avoids pow() per token)
        try prepareRoPEFrequencyLUT()

        print("✅ Model loaded: \(nLayers) layers, dim=\(dim), vocab=\(vocabSize)")
    }

    /// Dequantize token_embd.weight to F32 for embedding lookup
    private func prepareEmbeddingBuffer() throws {
        guard let model = self.model else { throw ZetaError.modelNotLoaded }

        guard let weightBuf = model.weights["token_embd.weight"],
              let weightType = model.weightTypes["token_embd.weight"] else {
            throw ZetaError.modelLoadFailed
        }

        let totalElements = dim * vocabSize
        self.embeddingBuffer = try dequantizeWeight(weightBuf, type: weightType, totalElements: totalElements, name: "token_embd.weight")
    }

    /// Dequantize output.weight to F32 for output projection (logits)
    private func prepareOutputProjBuffer() throws {
        guard let model = self.model else { throw ZetaError.modelNotLoaded }

        guard let weightBuf = model.weights["output.weight"],
              let weightType = model.weightTypes["output.weight"] else {
            throw ZetaError.modelLoadFailed
        }

        let totalElements = dim * vocabSize
        self.outputProjBuffer = try dequantizeWeight(weightBuf, type: weightType, totalElements: totalElements, name: "output.weight")
    }

    /// Precompute RoPE frequency LUT: freq[i] = 1.0 / pow(theta, 2i / head_dim)
    /// This eliminates the expensive pow() call in every RoPE kernel invocation
    private func prepareRoPEFrequencyLUT() throws {
        let halfDim = headDim / 2
        guard let lutBuffer = device.makeBuffer(length: halfDim * MemoryLayout<Float>.size, options: .storageModeShared) else {
            throw ZetaError.bufferAllocationFailed
        }

        // Precompute frequencies on CPU (only done once at model load)
        let freqPtr = lutBuffer.contents().bindMemory(to: Float.self, capacity: halfDim)
        for i in 0..<halfDim {
            // freq[i] = 1.0 / theta^(2i / head_dim)
            let exponent = Float(i * 2) / Float(headDim)
            freqPtr[i] = 1.0 / pow(ropeTheta, exponent)
        }

        self.ropeFreqLUT = lutBuffer
        print("✅ Precomputed RoPE frequency LUT (\(halfDim) entries)")
    }

    /// Helper to dequantize any weight to F32
    private func dequantizeWeight(_ weightBuf: MTLBuffer, type: TensorType, totalElements: Int, name: String) throws -> MTLBuffer {
        if type == .F32 {
            print("✅ Using \(name) (F32) directly")
            return weightBuf
        } else if type == .Q6_K {
            let nBlocks = totalElements / 256
            guard let outBuf = device.makeBuffer(length: totalElements * 4, options: .storageModeShared) else {
                throw ZetaError.bufferAllocationFailed
            }

            let cb = commandQueue.makeCommandBuffer()!
            let enc = cb.makeComputeCommandEncoder()!

            enc.setComputePipelineState(q6kDequantPipeline)
            enc.setBuffer(weightBuf, offset: 0, index: 0)
            enc.setBuffer(outBuf, offset: 0, index: 1)
            var nBlocksU = UInt32(nBlocks)
            enc.setBytes(&nBlocksU, length: 4, index: 2)
            enc.dispatchThreads(MTLSize(width: nBlocks, height: 1, depth: 1),
                               threadsPerThreadgroup: MTLSize(width: min(nBlocks, 256), height: 1, depth: 1))

            enc.endEncoding()
            cb.commit()
            cb.waitUntilCompleted()

            print("✅ Dequantized \(name) (Q6_K → F32)")
            return outBuf
        } else if type == .Q4_0 {
            let nBlocks = totalElements / 32
            guard let outBuf = device.makeBuffer(length: totalElements * 4, options: .storageModeShared) else {
                throw ZetaError.bufferAllocationFailed
            }

            let cb = commandQueue.makeCommandBuffer()!
            let enc = cb.makeComputeCommandEncoder()!

            enc.setComputePipelineState(q4DequantPipeline)
            enc.setBuffer(weightBuf, offset: 0, index: 0)
            enc.setBuffer(outBuf, offset: 0, index: 1)
            var nBlocksU = UInt32(nBlocks)
            enc.setBytes(&nBlocksU, length: 4, index: 2)
            enc.dispatchThreads(MTLSize(width: nBlocks, height: 1, depth: 1),
                               threadsPerThreadgroup: MTLSize(width: min(nBlocks, 256), height: 1, depth: 1))

            enc.endEncoding()
            cb.commit()
            cb.waitUntilCompleted()

            print("✅ Dequantized \(name) (Q4_0 → F32)")
            return outBuf
        } else {
            throw ZetaError.invalidTensorType
        }
    }

    // MARK: - Benchmark Results
    public struct BenchmarkResults {
        public let promptTokens: Int
        public let generatedTokens: Int
        public let prefillTimeMs: Double
        public let generateTimeMs: Double
        public let ttftMs: Double              // Time to first token
        public let tokensPerSecond: Double
        public let output: String

        public var summary: String {
            """
            ┌─────────────────────────────────────┐
            │       Z.E.T.A. Benchmark Results    │
            ├─────────────────────────────────────┤
            │ Prompt tokens:    \(String(format: "%6d", promptTokens))            │
            │ Generated tokens: \(String(format: "%6d", generatedTokens))            │
            │ TTFT:             \(String(format: "%6.1f", ttftMs)) ms         │
            │ Prefill:          \(String(format: "%6.1f", prefillTimeMs)) ms         │
            │ Generation:       \(String(format: "%6.1f", generateTimeMs)) ms         │
            │ Throughput:       \(String(format: "%6.2f", tokensPerSecond)) tok/s     │
            └─────────────────────────────────────┘
            """
        }
    }

    // MARK: - Inference
    public func generate(prompt: String, maxTokens: Int = 100) throws -> String {
        let results = try generateWithBenchmark(prompt: prompt, maxTokens: maxTokens, silent: false)
        return results.output
    }

    /// Generate with timing metrics
    public func generateWithBenchmark(prompt: String, maxTokens: Int = 100, silent: Bool = false) throws -> BenchmarkResults {
        guard let model = self.model, let _ = self.buffers else {
            throw ZetaError.modelNotLoaded
        }

        let tokens = model.tokenizer.encode(prompt)
        var outputTokens: [Int] = []

        // Prefill timing - OPTIMIZED: batch all tokens into single command buffer
        let prefillStart = CFAbsoluteTimeGetCurrent()
        try batchPrefill(tokens: tokens)
        let prefillEnd = CFAbsoluteTimeGetCurrent()
        let prefillTimeMs = (prefillEnd - prefillStart) * 1000.0

        // Generate timing
        var pos = tokens.count
        var ttftMs: Double = 0.0
        let generateStart = CFAbsoluteTimeGetCurrent()

        var nextToken = try sampleNext()

        // TTFT = time from generate start to first token
        let firstTokenTime = CFAbsoluteTimeGetCurrent()
        ttftMs = (firstTokenTime - generateStart) * 1000.0

        while outputTokens.count < maxTokens && nextToken != model.tokenizer.eosToken {
            outputTokens.append(nextToken)
            if !silent {
                print(" → \(model.tokenizer.decode([nextToken]))", terminator: "")
                fflush(stdout)
            }
            try forward(token: nextToken, position: pos)
            pos += 1
            nextToken = try sampleNext()
        }
        if !silent { print() }

        let generateEnd = CFAbsoluteTimeGetCurrent()
        let generateTimeMs = (generateEnd - generateStart) * 1000.0

        // Calculate tokens per second (generation phase only)
        let tokensPerSecond = generateTimeMs > 0 ? Double(outputTokens.count) / (generateTimeMs / 1000.0) : 0.0

        return BenchmarkResults(
            promptTokens: tokens.count,
            generatedTokens: outputTokens.count,
            prefillTimeMs: prefillTimeMs,
            generateTimeMs: generateTimeMs,
            ttftMs: ttftMs,
            tokensPerSecond: tokensPerSecond,
            output: model.tokenizer.decode(outputTokens)
        )
    }

    // MARK: - Batch Prefill (OPTIMIZED)
    /// Encode all prompt tokens into a single command buffer
    /// Reduces command buffer overhead from O(n) to O(1)
    private func batchPrefill(tokens: [Int]) throws {
        guard let model = self.model, let buffers = self.buffers else {
            throw ZetaError.modelNotLoaded
        }

        let cb = commandQueue.makeCommandBuffer()!
        let enc = cb.makeComputeCommandEncoder()!

        // Encode all prompt tokens sequentially into single command buffer
        for (pos, token) in tokens.enumerated() {
            // 1. Embedding lookup
            encodeEmbedding(enc, token: token, output: buffers.x)

            // 2. Transformer layers
            for layer in 0..<nLayers {
                // Attention block
                if let normWeight = model.weights["blk.\(layer).attn_norm.weight"] {
                    encodeRMSNorm(enc, input: buffers.x, weight: normWeight, output: buffers.xb)
                }

                encodeQKVProjection(enc, input: buffers.xb, layer: layer)
                encodeRoPE(enc, position: pos)
                encodeKVCacheStore(enc, layer: layer, position: pos)
                encodeAttention(enc, layer: layer, position: pos)

                if let outWeight = model.weights["blk.\(layer).attn_output.weight"] {
                    encodeMatmul(enc, input: buffers.attnOut, weight: outWeight,
                                weightType: model.weightTypes["blk.\(layer).attn_output.weight"] ?? .Q4_0,
                                output: buffers.xb2, inDim: dim, outDim: dim)
                }
                encodeAdd(enc, a: buffers.x, b: buffers.xb2, output: buffers.x)

                // FFN block
                if let ffnNorm = model.weights["blk.\(layer).ffn_norm.weight"] {
                    encodeRMSNorm(enc, input: buffers.x, weight: ffnNorm, output: buffers.xb)
                }
                encodeFFN(enc, input: buffers.xb, layer: layer, output: buffers.ffnOut)
                encodeAdd(enc, a: buffers.x, b: buffers.ffnOut, output: buffers.x)
            }
        }

        // Final norm + output projection (only for last token)
        if let outNorm = model.weights["output_norm.weight"] {
            encodeRMSNorm(enc, input: buffers.x, weight: outNorm, output: buffers.xb)
        }

        if let outProjBuf = outputProjBuffer {
            encodeMatmul(enc, input: buffers.xb, weight: outProjBuf,
                        weightType: .F32, output: buffers.logits, inDim: dim, outDim: vocabSize)
        }

        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()
    }

    // MARK: - Forward Pass
    private func forward(token: Int, position: Int) throws {
        guard let model = self.model, let buffers = self.buffers else {
            throw ZetaError.modelNotLoaded
        }

        let cb = commandQueue.makeCommandBuffer()!
        let enc = cb.makeComputeCommandEncoder()!

        // 1. Embedding lookup
        encodeEmbedding(enc, token: token, output: buffers.x)

        // 2. Transformer layers
        for layer in 0..<nLayers {
            // Attention block
            if let normWeight = model.weights["blk.\(layer).attn_norm.weight"] {
                encodeRMSNorm(enc, input: buffers.x, weight: normWeight, output: buffers.xb)
            }

            encodeQKVProjection(enc, input: buffers.xb, layer: layer)
            encodeRoPE(enc, position: position)
            encodeKVCacheStore(enc, layer: layer, position: position)
            encodeAttention(enc, layer: layer, position: position)

            if let outWeight = model.weights["blk.\(layer).attn_output.weight"] {
                encodeMatmul(enc, input: buffers.attnOut, weight: outWeight,
                            weightType: model.weightTypes["blk.\(layer).attn_output.weight"] ?? .Q4_0,
                            output: buffers.xb2, inDim: dim, outDim: dim)
            }
            encodeAdd(enc, a: buffers.x, b: buffers.xb2, output: buffers.x)

            // FFN block
            if let ffnNorm = model.weights["blk.\(layer).ffn_norm.weight"] {
                encodeRMSNorm(enc, input: buffers.x, weight: ffnNorm, output: buffers.xb)
            }
            encodeFFN(enc, input: buffers.xb, layer: layer, output: buffers.ffnOut)
            encodeAdd(enc, a: buffers.x, b: buffers.ffnOut, output: buffers.x)
        }

        // 3. Final norm + output projection
        if let outNorm = model.weights["output_norm.weight"] {
            encodeRMSNorm(enc, input: buffers.x, weight: outNorm, output: buffers.xb)
        }

        // Output projection uses dequantized output.weight (not embedding buffer!)
        if let outProjBuf = outputProjBuffer {
            encodeMatmul(enc, input: buffers.xb, weight: outProjBuf,
                        weightType: .F32, output: buffers.logits, inDim: dim, outDim: vocabSize)
        }

        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()
    }

    // MARK: - Sampling
    /// GPU-accelerated argmax sampling
    private func sampleNext() throws -> Int {
        guard let buffers = self.buffers,
              let resultBuf = argmaxResultBuffer else {
            throw ZetaError.modelNotLoaded
        }

        let cb = commandQueue.makeCommandBuffer()!
        let enc = cb.makeComputeCommandEncoder()!

        enc.setComputePipelineState(argmaxPipeline)
        enc.setBuffer(buffers.logits, offset: 0, index: 0)
        enc.setBuffer(resultBuf, offset: 0, index: 1)
        var vocabU = UInt32(vocabSize)
        enc.setBytes(&vocabU, length: 4, index: 2)

        // Threadgroup memory for parallel reduction
        let threadgroupSize = 256
        enc.setThreadgroupMemoryLength(threadgroupSize * MemoryLayout<Float>.size, index: 0)  // shared_max
        enc.setThreadgroupMemoryLength(threadgroupSize * MemoryLayout<UInt32>.size, index: 1) // shared_idx

        enc.dispatchThreadgroups(MTLSize(width: 1, height: 1, depth: 1),
                                threadsPerThreadgroup: MTLSize(width: threadgroupSize, height: 1, depth: 1))

        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        // Read result from GPU buffer
        let resultPtr = resultBuf.contents().bindMemory(to: UInt32.self, capacity: 1)
        return Int(resultPtr[0])
    }

    // MARK: - Kernel Encoders

    /// Embedding lookup: out[i] = embedding_table[token * dim + i]
    private func encodeEmbedding(_ enc: MTLComputeCommandEncoder, token: Int, output: MTLBuffer) {
        guard let embBuf = embeddingBuffer else { return }

        enc.setComputePipelineState(embeddingPipeline)
        enc.setBuffer(embBuf, offset: 0, index: 0)
        enc.setBuffer(output, offset: 0, index: 1)
        var tokenU = UInt32(token)
        var dimU = UInt32(dim)
        enc.setBytes(&tokenU, length: 4, index: 2)
        enc.setBytes(&dimU, length: 4, index: 3)
        enc.dispatchThreads(MTLSize(width: dim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(dim, 256), height: 1, depth: 1))
    }

    /// RMSNorm: out = x * rsqrt(mean(x^2) + eps) * weight
    /// OPTIMIZED: Uses parallel threadgroup reduction
    private func encodeRMSNorm(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, weight: MTLBuffer, output: MTLBuffer) {
        enc.setComputePipelineState(rmsnormParallelPipeline)
        enc.setBuffer(input, offset: 0, index: 0)
        enc.setBuffer(weight, offset: 0, index: 1)
        enc.setBuffer(output, offset: 0, index: 2)
        var dimU = UInt32(dim)
        var eps = rmsNormEps
        enc.setBytes(&dimU, length: 4, index: 3)
        enc.setBytes(&eps, length: 4, index: 4)

        // Parallel reduction with 256 threads per threadgroup
        let threadgroupSize = 256
        enc.setThreadgroupMemoryLength(threadgroupSize * MemoryLayout<Float>.size, index: 0)
        enc.dispatchThreadgroups(MTLSize(width: 1, height: 1, depth: 1),
                                threadsPerThreadgroup: MTLSize(width: threadgroupSize, height: 1, depth: 1))
    }

    /// Q, K, V projections
    private func encodeQKVProjection(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, layer: Int) {
        guard let model = self.model, let buffers = self.buffers else { return }

        let kvDim = nKVHeads * headDim

        // Q projection: [dim] -> [dim]
        if let qWeight = model.weights["blk.\(layer).attn_q.weight"] {
            encodeMatmul(enc, input: input, weight: qWeight,
                        weightType: model.weightTypes["blk.\(layer).attn_q.weight"] ?? .Q4_0,
                        output: buffers.q, inDim: dim, outDim: dim)
        }

        // K projection: [dim] -> [kv_dim]
        if let kWeight = model.weights["blk.\(layer).attn_k.weight"] {
            encodeMatmul(enc, input: input, weight: kWeight,
                        weightType: model.weightTypes["blk.\(layer).attn_k.weight"] ?? .Q4_0,
                        output: buffers.k, inDim: dim, outDim: kvDim)
        }

        // V projection: [dim] -> [kv_dim]
        if let vWeight = model.weights["blk.\(layer).attn_v.weight"] {
            encodeMatmul(enc, input: input, weight: vWeight,
                        weightType: model.weightTypes["blk.\(layer).attn_v.weight"] ?? .Q4_0,
                        output: buffers.v, inDim: dim, outDim: kvDim)
        }
    }

    /// Rotary Position Embedding - OPTIMIZED with precomputed frequency LUT
    private func encodeRoPE(_ enc: MTLComputeCommandEncoder, position: Int) {
        guard let buffers = self.buffers, let freqLUT = ropeFreqLUT else { return }

        enc.setComputePipelineState(ropeLutPipeline)
        enc.setBuffer(buffers.q, offset: 0, index: 0)
        enc.setBuffer(buffers.k, offset: 0, index: 1)
        enc.setBuffer(freqLUT, offset: 0, index: 2)  // Precomputed frequencies
        var headDimU = UInt32(headDim)
        var nHeadsU = UInt32(nHeads)
        var nKVHeadsU = UInt32(nKVHeads)
        var posU = UInt32(position)
        enc.setBytes(&headDimU, length: 4, index: 3)
        enc.setBytes(&nHeadsU, length: 4, index: 4)
        enc.setBytes(&nKVHeadsU, length: 4, index: 5)
        enc.setBytes(&posU, length: 4, index: 6)
        enc.dispatchThreads(MTLSize(width: nHeads, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(nHeads, 32), height: 1, depth: 1))
    }

    /// Store K, V to cache
    private func encodeKVCacheStore(_ enc: MTLComputeCommandEncoder, layer: Int, position: Int) {
        guard let buffers = self.buffers else { return }

        let kvDim = nKVHeads * headDim
        let layerOffset = layer * maxContextLength * kvDim * 4
        let posOffset = position * kvDim * 4

        // Copy K to cache
        enc.setComputePipelineState(copyPipeline)
        enc.setBuffer(buffers.k, offset: 0, index: 0)
        enc.setBuffer(buffers.kvCacheK, offset: layerOffset + posOffset, index: 1)
        var kvDimU = UInt32(kvDim)
        enc.setBytes(&kvDimU, length: 4, index: 2)
        enc.dispatchThreads(MTLSize(width: kvDim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(kvDim, 256), height: 1, depth: 1))

        // Copy V to cache
        enc.setBuffer(buffers.v, offset: 0, index: 0)
        enc.setBuffer(buffers.kvCacheV, offset: layerOffset + posOffset, index: 1)
        enc.dispatchThreads(MTLSize(width: kvDim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(kvDim, 256), height: 1, depth: 1))
    }

    /// Attention: scores = Q @ K^T + temporal_decay, out = softmax(scores) @ V
    /// Z.E.T.A. Feature 1: Temporal Decay applied here
    private func encodeAttention(_ enc: MTLComputeCommandEncoder, layer: Int, position: Int) {
        guard let buffers = self.buffers else { return }

        let kvDim = nKVHeads * headDim
        let seqLen = position + 1

        // Compute attention scores with Z.E.T.A. Temporal Decay
        enc.setComputePipelineState(attnScoresPipeline)
        enc.setBuffer(buffers.q, offset: 0, index: 0)
        enc.setBuffer(buffers.kvCacheK, offset: layer * maxContextLength * kvDim * 4, index: 1)
        enc.setBuffer(buffers.attnScores, offset: 0, index: 2)
        var headDimU = UInt32(headDim)
        var nHeadsU = UInt32(nHeads)
        var nKVHeadsU = UInt32(nKVHeads)
        var seqLenU = UInt32(seqLen)
        var scale = 1.0 / sqrt(Float(headDim))
        var lambda = temporalLambda      // Z.E.T.A. Feature 1: decay rate
        var posU = UInt32(position)      // Z.E.T.A. Feature 1: current position
        enc.setBytes(&headDimU, length: 4, index: 3)
        enc.setBytes(&nHeadsU, length: 4, index: 4)
        enc.setBytes(&nKVHeadsU, length: 4, index: 5)
        enc.setBytes(&seqLenU, length: 4, index: 6)
        enc.setBytes(&scale, length: 4, index: 7)
        var tau = tunnelingTau                       // Z.E.T.A. Feature 2: tunneling threshold
        enc.setBytes(&lambda, length: 4, index: 8)   // Z.E.T.A. F1: temporal_lambda
        enc.setBytes(&posU, length: 4, index: 9)     // Z.E.T.A. F1: current_pos
        enc.setBytes(&tau, length: 4, index: 10)     // Z.E.T.A. F2: tunneling_tau
        enc.dispatchThreads(MTLSize(width: nHeads, height: seqLen, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(nHeads, 8), height: min(seqLen, 8), depth: 1))

        // Softmax per head - OPTIMIZED: parallel threadgroup reduction
        enc.setComputePipelineState(softmaxParallelPipeline)
        enc.setBuffer(buffers.attnScores, offset: 0, index: 0)
        enc.setBytes(&seqLenU, length: 4, index: 1)
        enc.setBytes(&nHeadsU, length: 4, index: 2)

        // One threadgroup per head, 256 threads for parallel reduction
        let softmaxThreadgroupSize = 256
        enc.setThreadgroupMemoryLength(softmaxThreadgroupSize * MemoryLayout<Float>.size, index: 0)
        enc.dispatchThreadgroups(MTLSize(width: nHeads, height: 1, depth: 1),
                                threadsPerThreadgroup: MTLSize(width: softmaxThreadgroupSize, height: 1, depth: 1))

        // Attention output
        enc.setComputePipelineState(attnOutputPipeline)
        enc.setBuffer(buffers.attnScores, offset: 0, index: 0)
        enc.setBuffer(buffers.kvCacheV, offset: layer * maxContextLength * kvDim * 4, index: 1)
        enc.setBuffer(buffers.attnOut, offset: 0, index: 2)
        enc.setBytes(&headDimU, length: 4, index: 3)
        enc.setBytes(&nHeadsU, length: 4, index: 4)
        enc.setBytes(&nKVHeadsU, length: 4, index: 5)
        enc.setBytes(&seqLenU, length: 4, index: 6)
        enc.dispatchThreads(MTLSize(width: nHeads, height: headDim, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(nHeads, 8), height: min(headDim, 8), depth: 1))
    }

    /// Matrix-vector multiply with type dispatch
    private func encodeMatmul(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, weight: MTLBuffer,
                              weightType: TensorType, output: MTLBuffer, inDim: Int, outDim: Int) {
        switch weightType {
        case .Q4_0:
            enc.setComputePipelineState(q4MatMulPipeline)
        case .F32:
            enc.setComputePipelineState(f32MatMulPipeline)
        default:
            enc.setComputePipelineState(f32MatMulPipeline)  // Fallback
        }

        enc.setBuffer(input, offset: 0, index: 0)
        enc.setBuffer(weight, offset: 0, index: 1)
        enc.setBuffer(output, offset: 0, index: 2)
        var inDimU = UInt32(inDim)
        var outDimU = UInt32(outDim)
        enc.setBytes(&inDimU, length: 4, index: 3)
        enc.setBytes(&outDimU, length: 4, index: 4)
        enc.dispatchThreads(MTLSize(width: outDim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(outDim, 256), height: 1, depth: 1))
    }

    /// Element-wise add
    private func encodeAdd(_ enc: MTLComputeCommandEncoder, a: MTLBuffer, b: MTLBuffer, output: MTLBuffer) {
        enc.setComputePipelineState(addPipeline)
        enc.setBuffer(a, offset: 0, index: 0)
        enc.setBuffer(b, offset: 0, index: 1)
        enc.setBuffer(output, offset: 0, index: 2)
        var sizeU = UInt32(dim)
        enc.setBytes(&sizeU, length: 4, index: 3)
        enc.dispatchThreads(MTLSize(width: dim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(dim, 256), height: 1, depth: 1))
    }

    /// SwiGLU FFN: out = down(silu(gate(x)) * up(x))
    private func encodeFFN(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, layer: Int, output: MTLBuffer) {
        guard let model = self.model, let buffers = self.buffers else { return }

        // Gate projection
        if let gateWeight = model.weights["blk.\(layer).ffn_gate.weight"] {
            encodeMatmul(enc, input: input, weight: gateWeight,
                        weightType: model.weightTypes["blk.\(layer).ffn_gate.weight"] ?? .Q4_0,
                        output: buffers.gate, inDim: dim, outDim: hiddenDim)
        }

        // Up projection
        if let upWeight = model.weights["blk.\(layer).ffn_up.weight"] {
            encodeMatmul(enc, input: input, weight: upWeight,
                        weightType: model.weightTypes["blk.\(layer).ffn_up.weight"] ?? .Q4_0,
                        output: buffers.up, inDim: dim, outDim: hiddenDim)
        }

        // SwiGLU: silu(gate) * up
        enc.setComputePipelineState(swigluPipeline)
        enc.setBuffer(buffers.gate, offset: 0, index: 0)
        enc.setBuffer(buffers.up, offset: 0, index: 1)
        enc.setBuffer(buffers.ffnHidden, offset: 0, index: 2)
        var hiddenDimU = UInt32(hiddenDim)
        enc.setBytes(&hiddenDimU, length: 4, index: 3)
        enc.dispatchThreads(MTLSize(width: hiddenDim, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(hiddenDim, 256), height: 1, depth: 1))

        // Down projection
        if let downWeight = model.weights["blk.\(layer).ffn_down.weight"] {
            encodeMatmul(enc, input: buffers.ffnHidden, weight: downWeight,
                        weightType: model.weightTypes["blk.\(layer).ffn_down.weight"] ?? .Q4_0,
                        output: output, inDim: hiddenDim, outDim: dim)
        }
    }
}

// MARK: - Supporting Types

public enum ZetaError: Error {
    case noMetalDevice
    case commandQueueFailed
    case shaderLoadFailed
    case functionNotFound(String)
    case modelLoadFailed
    case modelNotLoaded
    case invalidTensorType
    case bufferAllocationFailed
}

/// Pre-allocated buffers for inference (zero hot-path allocations)
struct InferenceBuffers {
    let x: MTLBuffer          // Current hidden state [dim]
    let xb: MTLBuffer         // Scratch buffer [dim]
    let xb2: MTLBuffer        // Second scratch buffer [dim]
    let q: MTLBuffer          // Query [dim]
    let k: MTLBuffer          // Key [kv_dim]
    let v: MTLBuffer          // Value [kv_dim]
    let attnOut: MTLBuffer    // Attention output [dim]
    let attnScores: MTLBuffer // Attention scores [n_heads * context]
    let ffnOut: MTLBuffer     // FFN output [dim]
    let gate: MTLBuffer       // FFN gate [hidden_dim]
    let up: MTLBuffer         // FFN up [hidden_dim]
    let ffnHidden: MTLBuffer  // FFN hidden [hidden_dim]
    let logits: MTLBuffer     // Output logits [vocab]
    let kvCacheK: MTLBuffer   // KV cache keys [layers * context * kv_dim]
    let kvCacheV: MTLBuffer   // KV cache values [layers * context * kv_dim]

    init(device: MTLDevice, dim: Int, hiddenDim: Int, kvDim: Int, vocabSize: Int, nLayers: Int, contextLen: Int) throws {
        guard let x = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let xb = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let xb2 = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let q = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let k = device.makeBuffer(length: kvDim * 4, options: .storageModeShared),
              let v = device.makeBuffer(length: kvDim * 4, options: .storageModeShared),
              let attnOut = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let attnScores = device.makeBuffer(length: 32 * contextLen * 4, options: .storageModeShared),
              let ffnOut = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let gate = device.makeBuffer(length: hiddenDim * 4, options: .storageModeShared),
              let up = device.makeBuffer(length: hiddenDim * 4, options: .storageModeShared),
              let ffnHidden = device.makeBuffer(length: hiddenDim * 4, options: .storageModeShared),
              let logits = device.makeBuffer(length: vocabSize * 4, options: .storageModeShared),
              let kvK = device.makeBuffer(length: nLayers * contextLen * kvDim * 4, options: .storageModeShared),
              let kvV = device.makeBuffer(length: nLayers * contextLen * kvDim * 4, options: .storageModeShared)
        else {
            throw ZetaError.bufferAllocationFailed
        }

        self.x = x
        self.xb = xb
        self.xb2 = xb2
        self.q = q
        self.k = k
        self.v = v
        self.attnOut = attnOut
        self.attnScores = attnScores
        self.ffnOut = ffnOut
        self.gate = gate
        self.up = up
        self.ffnHidden = ffnHidden
        self.logits = logits
        self.kvCacheK = kvK
        self.kvCacheV = kvV
    }
}
