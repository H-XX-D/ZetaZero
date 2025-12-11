import XCTest
@testable import ZetaCore

final class ZetaCoreTests: XCTestCase {

    func testQ4Dequantization() throws {
        // Test Q4_0 dequantization matches reference
        // TODO: Implement with known test vectors from llama.cpp
    }

    func testQ6KDequantization() throws {
        // Test Q6_K dequantization matches reference
        // TODO: Implement with known test vectors from llama.cpp
    }

    func testRMSNorm() throws {
        // Test RMSNorm output matches llama.cpp
        // TODO: Implement
    }

    func testRoPE() throws {
        // Test Rotary Position Embedding
        // TODO: Implement
    }

    func testAttention() throws {
        // Test full attention pass
        // TODO: Implement
    }

    func testEndToEndGeneration() throws {
        // CRITICAL: Output must match llama-cli exactly
        // TODO: Compare token-by-token with reference
    }
}
