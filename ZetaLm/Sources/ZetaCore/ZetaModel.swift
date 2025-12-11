import Foundation
import Metal

/// Model weights and configuration
public final class ZetaModel {

    // MARK: - Configuration
    public var dim: Int = 0
    public var hiddenDim: Int = 0
    public var nLayers: Int = 0
    public var nHeads: Int = 0
    public var nKVHeads: Int = 0
    public var headDim: Int = 0
    public var vocabSize: Int = 0
    public var contextLength: Int = 2048
    public var rmsNormEps: Float = 1e-5
    public var ropeTheta: Float = 10000.0

    // MARK: - Weights (Metal buffers)
    public var weights: [String: MTLBuffer] = [:]
    public var weightTypes: [String: TensorType] = [:]

    // MARK: - Tokenizer
    public var tokenizer: ZetaTokenizer!

    public init() {}
}

/// Supported quantization types
public enum TensorType: UInt32 {
    case F32 = 0
    case F16 = 1
    case Q4_0 = 2
    case Q4_1 = 3
    case Q5_0 = 6
    case Q5_1 = 7
    case Q8_0 = 8
    case Q6_K = 14

    /// Bytes per block for quantized types
    var blockSize: Int {
        switch self {
        case .F32: return 4
        case .F16: return 2
        case .Q4_0: return 18      // 2 (scale) + 16 (32 nibbles)
        case .Q4_1: return 20
        case .Q5_0: return 22
        case .Q5_1: return 24
        case .Q8_0: return 34
        case .Q6_K: return 210     // 128 + 64 + 16 + 2
        }
    }

    /// Elements per block
    var elementsPerBlock: Int {
        switch self {
        case .F32, .F16: return 1
        case .Q4_0, .Q4_1: return 32
        case .Q5_0, .Q5_1: return 32
        case .Q8_0: return 32
        case .Q6_K: return 256
        }
    }
}

/// Simple BPE tokenizer
public final class ZetaTokenizer {
    private var vocab: [String]
    private var tokenToId: [String: Int]
    private var merges: [(String, String)]

    public var eosToken: Int { 2 }
    public var bosToken: Int { 1 }

    public init(vocab: [String], merges: [String]) {
        self.vocab = vocab
        // Handle duplicate tokens by keeping first occurrence
        var tokenMap: [String: Int] = [:]
        for (idx, token) in vocab.enumerated() {
            if tokenMap[token] == nil {
                tokenMap[token] = idx
            }
        }
        self.tokenToId = tokenMap
        self.merges = merges.compactMap { line in
            let parts = line.split(separator: " ")
            guard parts.count == 2 else { return nil }
            return (String(parts[0]), String(parts[1]))
        }
    }

    public func encode(_ text: String) -> [Int] {
        // Simple character-level fallback + BPE
        var tokens: [Int] = [bosToken]

        // Split into words
        let words = text.split(separator: " ", omittingEmptySubsequences: false)

        for (idx, word) in words.enumerated() {
            let wordStr = (idx > 0 ? " " : "") + String(word)

            // Try direct vocab lookup first
            if let id = tokenToId[wordStr] {
                tokens.append(id)
            } else {
                // Fall back to character-level
                for char in wordStr {
                    if let id = tokenToId[String(char)] {
                        tokens.append(id)
                    }
                }
            }
        }

        return tokens
    }

    public func decode(_ tokens: [Int]) -> String {
        return tokens.map { id in
            guard id >= 0 && id < vocab.count else { return "" }
            return vocab[id]
        }.joined()
    }
}
