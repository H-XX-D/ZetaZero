import Foundation
import Metal

/// GGUF model loader - extracts tensors directly to Metal buffers
public final class ZetaCoreLoader {
    private let device: MTLDevice
    private var handle: FileHandle?
    private var currentOffset: UInt64 = 0

    public init(device: MTLDevice) {
        self.device = device
    }

    public func load(path: String) throws -> ZetaModel {
        let url = URL(fileURLWithPath: path)
        self.handle = try FileHandle(forReadingFrom: url)
        self.currentOffset = 0

        let model = ZetaModel()

        // 1. Read GGUF header
        let (version, nTensors, nKV) = try readHeader()
        print("GGUF v\(version) | Tensors: \(nTensors) | KV: \(nKV)")

        // 2. Read metadata
        let metadata = try readMetadata(count: nKV)
        extractConfig(from: metadata, to: model)

        // 3. Read tensor info
        let tensorInfos = try readTensorInfo(count: nTensors)

        // 4. Calculate data offset (aligned)
        let alignment: UInt64 = (metadata["general.alignment"] as? UInt64) ?? 32
        let dataOffset = (currentOffset + alignment - 1) & ~(alignment - 1)

        print("Data starts at: \(dataOffset)")

        // 5. Load tensors to Metal buffers
        for info in tensorInfos {
            let absOffset = dataOffset + info.offset
            try loadTensor(info: info, offset: absOffset, into: model)
        }

        // 6. Load tokenizer
        if let vocab = metadata["tokenizer.ggml.tokens"] as? [String] {
            let merges = (metadata["tokenizer.ggml.merges"] as? [String]) ?? []
            model.tokenizer = ZetaTokenizer(vocab: vocab, merges: merges)
            model.vocabSize = vocab.count
            print("Tokenizer: \(vocab.count) tokens, \(merges.count) merges")
        }

        handle?.closeFile()
        print("✅ Loaded \(model.weights.count) tensors")
        return model
    }

    // MARK: - Header Parsing

    private func readHeader() throws -> (version: UInt32, nTensors: UInt64, nKV: UInt64) {
        // Magic "GGUF"
        let magic = try readBytes(4)
        guard String(data: magic, encoding: .ascii) == "GGUF" else {
            throw ZetaError.modelLoadFailed
        }

        let version = try readUInt32()
        let nTensors = try readUInt64()
        let nKV = try readUInt64()

        return (version, nTensors, nKV)
    }

    // MARK: - Metadata

    private func readMetadata(count: UInt64) throws -> [String: Any] {
        var metadata: [String: Any] = [:]

        for _ in 0..<count {
            let key = try readString()
            let valueType = try readUInt32()
            let value = try readValue(type: valueType)
            metadata[key] = value
        }

        return metadata
    }

    private func extractConfig(from metadata: [String: Any], to model: ZetaModel) {
        if let v = metadata["llama.embedding_length"] as? UInt32 { model.dim = Int(v) }
        if let v = metadata["llama.feed_forward_length"] as? UInt32 { model.hiddenDim = Int(v) }
        if let v = metadata["llama.block_count"] as? UInt32 { model.nLayers = Int(v) }
        if let v = metadata["llama.attention.head_count"] as? UInt32 { model.nHeads = Int(v) }
        if let v = metadata["llama.attention.head_count_kv"] as? UInt32 { model.nKVHeads = Int(v) }
        if let v = metadata["llama.context_length"] as? UInt32 { model.contextLength = Int(v) }
        if let v = metadata["llama.attention.layer_norm_rms_epsilon"] as? Float { model.rmsNormEps = v }
        if let v = metadata["llama.rope.freq_base"] as? Float { model.ropeTheta = v }

        model.headDim = model.dim / max(model.nHeads, 1)

        print("Config: dim=\(model.dim), layers=\(model.nLayers), heads=\(model.nHeads)")
    }

    // MARK: - Tensor Info

    struct TensorInfo {
        let name: String
        let dims: [UInt64]
        let type: TensorType
        let offset: UInt64
        var byteSize: Int {
            let elements = dims.reduce(1, *)
            let blocks = (Int(elements) + type.elementsPerBlock - 1) / type.elementsPerBlock
            return blocks * type.blockSize
        }
    }

    private func readTensorInfo(count: UInt64) throws -> [TensorInfo] {
        var infos: [TensorInfo] = []

        for _ in 0..<count {
            let name = try readString()
            let nDims = try readUInt32()
            var dims: [UInt64] = []
            for _ in 0..<nDims {
                dims.append(try readUInt64())
            }
            let typeRaw = try readUInt32()
            let offset = try readUInt64()

            guard let type = TensorType(rawValue: typeRaw) else {
                print("⚠️ Unknown tensor type \(typeRaw) for \(name)")
                continue
            }

            infos.append(TensorInfo(name: name, dims: dims, type: type, offset: offset))
        }

        return infos
    }

    // MARK: - Tensor Loading

    private func loadTensor(info: TensorInfo, offset: UInt64, into model: ZetaModel) throws {
        guard let handle = self.handle else { throw ZetaError.modelLoadFailed }

        try handle.seek(toOffset: offset)
        guard let data = try handle.read(upToCount: info.byteSize) else {
            throw ZetaError.modelLoadFailed
        }

        // Create Metal buffer directly from raw data
        guard let buffer = device.makeBuffer(bytes: (data as NSData).bytes,
                                              length: data.count,
                                              options: .storageModeShared) else {
            throw ZetaError.bufferAllocationFailed
        }

        model.weights[info.name] = buffer
        model.weightTypes[info.name] = info.type
    }

    // MARK: - Primitive Readers

    private func readBytes(_ count: Int) throws -> Data {
        guard let handle = self.handle else { throw ZetaError.modelLoadFailed }
        guard let data = try handle.read(upToCount: count), data.count == count else {
            throw ZetaError.modelLoadFailed
        }
        currentOffset += UInt64(count)
        return data
    }

    private func readUInt32() throws -> UInt32 {
        let data = try readBytes(4)
        return data.withUnsafeBytes { $0.load(as: UInt32.self) }
    }

    private func readUInt64() throws -> UInt64 {
        let data = try readBytes(8)
        return data.withUnsafeBytes { $0.load(as: UInt64.self) }
    }

    private func readString() throws -> String {
        let length = try readUInt64()
        let data = try readBytes(Int(length))
        return String(data: data, encoding: .utf8) ?? ""
    }

    private func readValue(type: UInt32) throws -> Any {
        switch type {
        case 0: return try readBytes(1)[0]                      // UINT8
        case 1: return Int8(bitPattern: try readBytes(1)[0])    // INT8
        case 2: return try readUInt32() & 0xFFFF                // UINT16
        case 3: return Int16(bitPattern: UInt16(try readUInt32() & 0xFFFF)) // INT16
        case 4: return try readUInt32()                         // UINT32
        case 5: return Int32(bitPattern: try readUInt32())      // INT32
        case 6:                                                 // FLOAT32
            let bits = try readUInt32()
            return Float(bitPattern: bits)
        case 7: return try readBytes(1)[0] != 0                 // BOOL
        case 8: return try readString()                         // STRING
        case 9:                                                 // ARRAY
            let arrayType = try readUInt32()
            let count = try readUInt64()
            var values: [Any] = []
            for _ in 0..<count {
                values.append(try readValue(type: arrayType))
            }
            return values
        case 10: return try readUInt64()                        // UINT64
        case 11: return Int64(bitPattern: try readUInt64())     // INT64
        case 12:                                                // FLOAT64
            let bits = try readUInt64()
            return Double(bitPattern: bits)
        default:
            throw ZetaError.modelLoadFailed
        }
    }
}
