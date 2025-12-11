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

    // MARK: - Compute Pipelines
    private var q4MatMulPipeline: MTLComputePipelineState!
    private var f32MatMulPipeline: MTLComputePipelineState!
    private var q6kDequantPipeline: MTLComputePipelineState!
    private var rmsnormPipeline: MTLComputePipelineState!
    private var ropePipeline: MTLComputePipelineState!
    private var softmaxPipeline: MTLComputePipelineState!
    private var swigluPipeline: MTLComputePipelineState!
    private var addPipeline: MTLComputePipelineState!
    private var embeddingPipeline: MTLComputePipelineState!
    private var attnScoresPipeline: MTLComputePipelineState!
    private var attnOutputPipeline: MTLComputePipelineState!
    private var copyPipeline: MTLComputePipelineState!
    private var q4DequantPipeline: MTLComputePipelineState!

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
        q4MatMulPipeline = try makePipeline("zeta_q4_matmul")
        f32MatMulPipeline = try makePipeline("zeta_f32_matmul")
        q6kDequantPipeline = try makePipeline("zeta_q6k_dequant")
        rmsnormPipeline = try makePipeline("zeta_rmsnorm")
        ropePipeline = try makePipeline("zeta_rope")
        softmaxPipeline = try makePipeline("zeta_softmax")
        swigluPipeline = try makePipeline("zeta_swiglu")
        addPipeline = try makePipeline("zeta_add")
        embeddingPipeline = try makePipeline("zeta_embedding")
        attnScoresPipeline = try makePipeline("zeta_attn_scores")
        attnOutputPipeline = try makePipeline("zeta_attn_output")
        copyPipeline = try makePipeline("zeta_copy")
        q4DequantPipeline = try makePipeline("zeta_q4_dequant")
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

    // MARK: - Inference
    public func generate(prompt: String, maxTokens: Int = 100) throws -> String {
        guard let model = self.model, let _ = self.buffers else {
            throw ZetaError.modelNotLoaded
        }

        let tokens = model.tokenizer.encode(prompt)
        var outputTokens: [Int] = []

        // Prefill
        for (pos, token) in tokens.enumerated() {
            try forward(token: token, position: pos)
        }

        // Generate
        var pos = tokens.count
        var nextToken = try sampleNext()

        while outputTokens.count < maxTokens && nextToken != model.tokenizer.eosToken {
            outputTokens.append(nextToken)
            print(" → \(model.tokenizer.decode([nextToken]))", terminator: "")
            try forward(token: nextToken, position: pos)
            pos += 1
            nextToken = try sampleNext()
        }
        print()

        return model.tokenizer.decode(outputTokens)
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
    private func sampleNext() throws -> Int {
        guard let buffers = self.buffers else { throw ZetaError.modelNotLoaded }

        let ptr = buffers.logits.contents().bindMemory(to: Float.self, capacity: vocabSize)
        var maxIdx = 0
        var maxVal = ptr[0]

        for i in 1..<vocabSize {
            if ptr[i] > maxVal {
                maxVal = ptr[i]
                maxIdx = i
            }
        }

        return maxIdx
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
    private func encodeRMSNorm(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, weight: MTLBuffer, output: MTLBuffer) {
        enc.setComputePipelineState(rmsnormPipeline)
        enc.setBuffer(input, offset: 0, index: 0)
        enc.setBuffer(weight, offset: 0, index: 1)
        enc.setBuffer(output, offset: 0, index: 2)
        var dimU = UInt32(dim)
        var eps = rmsNormEps
        enc.setBytes(&dimU, length: 4, index: 3)
        enc.setBytes(&eps, length: 4, index: 4)
        // Single thread for simple implementation
        enc.dispatchThreads(MTLSize(width: 1, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: 1, height: 1, depth: 1))
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

    /// Rotary Position Embedding
    private func encodeRoPE(_ enc: MTLComputeCommandEncoder, position: Int) {
        guard let buffers = self.buffers else { return }

        enc.setComputePipelineState(ropePipeline)
        enc.setBuffer(buffers.q, offset: 0, index: 0)
        enc.setBuffer(buffers.k, offset: 0, index: 1)
        var headDimU = UInt32(headDim)
        var nHeadsU = UInt32(nHeads)
        var nKVHeadsU = UInt32(nKVHeads)
        var posU = UInt32(position)
        var theta = ropeTheta
        enc.setBytes(&headDimU, length: 4, index: 2)
        enc.setBytes(&nHeadsU, length: 4, index: 3)
        enc.setBytes(&nKVHeadsU, length: 4, index: 4)
        enc.setBytes(&posU, length: 4, index: 5)
        enc.setBytes(&theta, length: 4, index: 6)
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

    /// Attention: scores = Q @ K^T, out = softmax(scores) @ V
    private func encodeAttention(_ enc: MTLComputeCommandEncoder, layer: Int, position: Int) {
        guard let buffers = self.buffers else { return }

        let kvDim = nKVHeads * headDim
        let seqLen = position + 1

        // For now, simplified single-position attention
        // Full KV cache attention needs more buffers

        // Compute attention scores
        enc.setComputePipelineState(attnScoresPipeline)
        enc.setBuffer(buffers.q, offset: 0, index: 0)
        enc.setBuffer(buffers.kvCacheK, offset: layer * maxContextLength * kvDim * 4, index: 1)
        enc.setBuffer(buffers.attnScores, offset: 0, index: 2)
        var headDimU = UInt32(headDim)
        var nHeadsU = UInt32(nHeads)
        var nKVHeadsU = UInt32(nKVHeads)
        var seqLenU = UInt32(seqLen)
        var scale = 1.0 / sqrt(Float(headDim))
        enc.setBytes(&headDimU, length: 4, index: 3)
        enc.setBytes(&nHeadsU, length: 4, index: 4)
        enc.setBytes(&nKVHeadsU, length: 4, index: 5)
        enc.setBytes(&seqLenU, length: 4, index: 6)
        enc.setBytes(&scale, length: 4, index: 7)
        enc.dispatchThreads(MTLSize(width: nHeads, height: seqLen, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(nHeads, 8), height: min(seqLen, 8), depth: 1))

        // Softmax per head
        enc.setComputePipelineState(softmaxPipeline)
        enc.setBuffer(buffers.attnScores, offset: 0, index: 0)
        enc.setBytes(&seqLenU, length: 4, index: 1)
        enc.setBytes(&nHeadsU, length: 4, index: 2)
        enc.dispatchThreads(MTLSize(width: nHeads, height: 1, depth: 1),
                           threadsPerThreadgroup: MTLSize(width: min(nHeads, 32), height: 1, depth: 1))

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
