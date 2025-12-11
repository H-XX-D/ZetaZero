import Foundation
import ZetaCore

/// Z.E.T.A. Features Layer
/// Phase 2: Novel attention mechanisms from research paper
///
/// Features:
/// - Temporal Decay: Z(t) = Z₀ · e^(-λt)
/// - Sparse Gating (Tunneling): a < τ → -∞
/// - Sharpened Similarity (Entanglement): E = ReLU(cos)³
/// - Semantic Momentum: I(t) = q + γ(q - q_prev)
/// - Unified Attention: O = Attn(active) + Σ Z_k·Attn(memory_k)
/// - Sublimation: KV cache → summary vector → NVMe

public final class ZetaFeaturesEngine {

    // MARK: - Configuration
    public var temporalDecayLambda: Float = 0.1
    public var tunnelingThreshold: Float = 1.5
    public var momentumGamma: Float = 0.5
    public var maxSuperpositions: Int = 5

    // MARK: - State
    private var previousQuery: [Float]?
    private var activeMemoryBlocks: [MemoryBlock] = []

    public init() {
        print("✅ ZetaFeatures initialized")
        print("   λ (temporal decay) = \(temporalDecayLambda)")
        print("   τ (tunneling threshold) = \(tunnelingThreshold)")
        print("   γ (momentum) = \(momentumGamma)")
    }

    // MARK: - Temporal Decay
    /// Apply temporal decay to attention scores
    /// Z(t) = Z₀ · e^(-λ·t)
    public func applyTemporalDecay(scores: inout [Float], distances: [Int]) {
        for i in scores.indices {
            let t = Float(distances[i])
            scores[i] *= exp(-temporalDecayLambda * t)
        }
    }

    // MARK: - Sparse Gating (Tunneling)
    /// Apply tunneling threshold: a < τ → -∞
    public func applyTunneling(scores: inout [Float]) {
        for i in scores.indices {
            if scores[i] < tunnelingThreshold {
                scores[i] = -.infinity
            }
        }
    }

    // MARK: - Sharpened Cosine Similarity (Entanglement)
    /// E(q, s) = ReLU(cos(q, s))³
    public func entanglementScore(query: [Float], summary: [Float]) -> Float {
        let dotProduct = zip(query, summary).reduce(0) { $0 + $1.0 * $1.1 }
        let qNorm = sqrt(query.reduce(0) { $0 + $1 * $1 })
        let sNorm = sqrt(summary.reduce(0) { $0 + $1 * $1 })

        guard qNorm > 0 && sNorm > 0 else { return 0 }

        let cos = dotProduct / (qNorm * sNorm)
        let relu = max(0, cos)
        return relu * relu * relu  // Cubic sharpening
    }

    // MARK: - Semantic Momentum
    /// I(t) = q_t + γ(q_t - q_{t-1})
    public func predictiveQuery(currentQuery: [Float]) -> [Float] {
        defer { previousQuery = currentQuery }

        guard let prev = previousQuery else {
            return currentQuery
        }

        return zip(currentQuery, prev).map { q, p in
            q + momentumGamma * (q - p)
        }
    }

    // MARK: - Memory Management
    public func addMemoryBlock(_ block: MemoryBlock) {
        activeMemoryBlocks.append(block)

        // Evict oldest if over capacity
        if activeMemoryBlocks.count > maxSuperpositions {
            activeMemoryBlocks.removeFirst()
        }
    }

    public func retrieveRelevantBlocks(query: [Float], topK: Int = 3) -> [MemoryBlock] {
        let predictive = predictiveQuery(currentQuery: query)

        return activeMemoryBlocks
            .map { block in
                (block, entanglementScore(query: predictive, summary: block.summaryVector))
            }
            .sorted { $0.1 > $1.1 }
            .prefix(topK)
            .map { $0.0 }
    }
}

// MARK: - Supporting Types

public struct MemoryBlock {
    public let id: UUID
    public let summaryVector: [Float]
    public let kvCachePath: URL?  // Path to sublimated KV cache on disk
    public let createdAt: Date
    public var lastAccessed: Date

    public init(summaryVector: [Float], kvCachePath: URL? = nil) {
        self.id = UUID()
        self.summaryVector = summaryVector
        self.kvCachePath = kvCachePath
        self.createdAt = Date()
        self.lastAccessed = Date()
    }
}
