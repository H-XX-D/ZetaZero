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
    private var q4MatMulPipeline: MTLComputePipelineState?
    private var q6kDequantPipeline: MTLComputePipelineState?
    private var rmsnormPipeline: MTLComputePipelineState?
    private var ropePipeline: MTLComputePipelineState?
    private var softmaxPipeline: MTLComputePipelineState?
    private var swigluPipeline: MTLComputePipelineState?
    private var addPipeline: MTLComputePipelineState?
    private var embeddingPipeline: MTLComputePipelineState?

    // MARK: - Model State
    private var model: ZetaModel?
    private var buffers: InferenceBuffers?

    // MARK: - Config
    public private(set) var dim: Int = 0
    public private(set) var hiddenDim: Int = 0
    public private(set) var nLayers: Int = 0
    public private(set) var nHeads: Int = 0
    public private(set) var nKVHeads: Int = 0
    public private(set) var headDim: Int = 0
    public private(set) var vocabSize: Int = 0
    public private(set) var maxContextLength: Int = 4096

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

        // Load Metal library from bundle
        guard let library = try? device.makeDefaultLibrary(bundle: Bundle.module) else {
            throw ZetaError.shaderLoadFailed
        }
        self.library = library

        try loadPipelines()
        print("✅ ZetaCore initialized (Metal Native)")
    }

    // MARK: - Pipeline Setup
    private func loadPipelines() throws {
        q4MatMulPipeline = try makePipeline("zeta_q4_matmul")
        q6kDequantPipeline = try makePipeline("zeta_q6k_dequant")
        rmsnormPipeline = try makePipeline("zeta_rmsnorm")
        ropePipeline = try makePipeline("zeta_rope")
        softmaxPipeline = try makePipeline("zeta_softmax")
        swigluPipeline = try makePipeline("zeta_swiglu")
        addPipeline = try makePipeline("zeta_add")
        embeddingPipeline = try makePipeline("zeta_embedding")
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

        // Extract config
        guard let model = self.model else { throw ZetaError.modelLoadFailed }
        self.dim = model.dim
        self.hiddenDim = model.hiddenDim
        self.nLayers = model.nLayers
        self.nHeads = model.nHeads
        self.nKVHeads = model.nKVHeads
        self.headDim = model.headDim
        self.vocabSize = model.vocabSize

        // Allocate inference buffers
        self.buffers = try InferenceBuffers(device: device, config: model)

        print("✅ Model loaded: \(nLayers) layers, dim=\(dim), vocab=\(vocabSize)")
    }

    // MARK: - Inference
    public func generate(prompt: String, maxTokens: Int = 100) throws -> String {
        guard let model = self.model, let buffers = self.buffers else {
            throw ZetaError.modelNotLoaded
        }

        // Tokenize
        let tokens = model.tokenizer.encode(prompt)
        var outputTokens: [Int] = []

        // Prefill
        for (pos, token) in tokens.enumerated() {
            _ = try forward(token: token, position: pos)
        }

        // Generate
        var pos = tokens.count
        var nextToken = try sampleNext()

        while outputTokens.count < maxTokens && nextToken != model.tokenizer.eosToken {
            outputTokens.append(nextToken)
            _ = try forward(token: nextToken, position: pos)
            pos += 1
            nextToken = try sampleNext()
        }

        return model.tokenizer.decode(outputTokens)
    }

    // MARK: - Forward Pass
    private func forward(token: Int, position: Int) throws -> MTLBuffer {
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
            encodeRMSNorm(enc, input: buffers.x, weight: model.weights["blk.\(layer).attn_norm.weight"]!, output: buffers.xb)
            encodeQKVProjection(enc, input: buffers.xb, layer: layer)
            encodeRoPE(enc, q: buffers.q, k: buffers.k, position: position)
            encodeAttention(enc, layer: layer, position: position)
            encodeAttnOutput(enc, input: buffers.xb, layer: layer, output: buffers.attnOut)
            encodeAdd(enc, a: buffers.x, b: buffers.attnOut, output: buffers.x)

            // FFN block
            encodeRMSNorm(enc, input: buffers.x, weight: model.weights["blk.\(layer).ffn_norm.weight"]!, output: buffers.xb)
            encodeFFN(enc, input: buffers.xb, layer: layer, output: buffers.ffnOut)
            encodeAdd(enc, a: buffers.x, b: buffers.ffnOut, output: buffers.x)
        }

        // 3. Final norm + output projection
        encodeRMSNorm(enc, input: buffers.x, weight: model.weights["output_norm.weight"]!, output: buffers.xb)
        encodeOutputProjection(enc, input: buffers.xb, output: buffers.logits)

        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        return buffers.logits
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

    // MARK: - Kernel Encoders (stubs - implement in Phase 1)
    private func encodeEmbedding(_ enc: MTLComputeCommandEncoder, token: Int, output: MTLBuffer) {
        // TODO: Implement embedding lookup
    }

    private func encodeRMSNorm(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, weight: MTLBuffer, output: MTLBuffer) {
        // TODO: Implement RMSNorm
    }

    private func encodeQKVProjection(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, layer: Int) {
        // TODO: Implement Q, K, V projections
    }

    private func encodeRoPE(_ enc: MTLComputeCommandEncoder, q: MTLBuffer, k: MTLBuffer, position: Int) {
        // TODO: Implement Rotary Position Embedding
    }

    private func encodeAttention(_ enc: MTLComputeCommandEncoder, layer: Int, position: Int) {
        // TODO: Implement attention with KV cache
    }

    private func encodeAttnOutput(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, layer: Int, output: MTLBuffer) {
        // TODO: Implement attention output projection
    }

    private func encodeAdd(_ enc: MTLComputeCommandEncoder, a: MTLBuffer, b: MTLBuffer, output: MTLBuffer) {
        // TODO: Implement element-wise add
    }

    private func encodeFFN(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, layer: Int, output: MTLBuffer) {
        // TODO: Implement SwiGLU FFN
    }

    private func encodeOutputProjection(_ enc: MTLComputeCommandEncoder, input: MTLBuffer, output: MTLBuffer) {
        // TODO: Implement output logits projection
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
    let q: MTLBuffer          // Query [dim]
    let k: MTLBuffer          // Key [kv_dim]
    let v: MTLBuffer          // Value [kv_dim]
    let attnOut: MTLBuffer    // Attention output [dim]
    let ffnOut: MTLBuffer     // FFN output [dim]
    let logits: MTLBuffer     // Output logits [vocab]
    let kvCacheK: MTLBuffer   // KV cache keys [layers * context * kv_dim]
    let kvCacheV: MTLBuffer   // KV cache values [layers * context * kv_dim]

    init(device: MTLDevice, config: ZetaModel) throws {
        let dim = config.dim
        let kvDim = config.nKVHeads * config.headDim
        let vocabSize = config.vocabSize
        let contextLen = 4096
        let nLayers = config.nLayers

        guard let x = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let xb = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let q = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let k = device.makeBuffer(length: kvDim * 4, options: .storageModeShared),
              let v = device.makeBuffer(length: kvDim * 4, options: .storageModeShared),
              let attnOut = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let ffnOut = device.makeBuffer(length: dim * 4, options: .storageModeShared),
              let logits = device.makeBuffer(length: vocabSize * 4, options: .storageModeShared),
              let kvK = device.makeBuffer(length: nLayers * contextLen * kvDim * 4, options: .storageModeShared),
              let kvV = device.makeBuffer(length: nLayers * contextLen * kvDim * 4, options: .storageModeShared)
        else {
            throw ZetaError.bufferAllocationFailed
        }

        self.x = x
        self.xb = xb
        self.q = q
        self.k = k
        self.v = v
        self.attnOut = attnOut
        self.ffnOut = ffnOut
        self.logits = logits
        self.kvCacheK = kvK
        self.kvCacheV = kvV
    }
}
