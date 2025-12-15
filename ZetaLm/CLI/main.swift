import Foundation
import ZetaCore
import ZetaFeatures

/// ZetaLm CLI - Apple Native LLM Inference
@main
struct ZetaCLI {
    static func main() async throws {
        print("""
        ╔═══════════════════════════════════════════╗
        ║     Z.E.T.A. Zero - Apple Native LLM      ║
        ║         Metal + Core ML Engine            ║
        ╚═══════════════════════════════════════════╝
        """)

        let args = CommandLine.arguments

        guard args.count >= 2 else {
            printUsage()
            return
        }

        // Check for benchmark flag
        let isBenchmark = args.contains("--benchmark") || args.contains("-b")
        let filteredArgs = args.filter { $0 != "--benchmark" && $0 != "-b" }

        let modelPath = filteredArgs[1]
        let prompt = filteredArgs.count >= 3 ? filteredArgs[2] : "Hello"
        let maxTokens = filteredArgs.count >= 4 ? Int(filteredArgs[3]) ?? 50 : 50

        // Initialize engine
        print("Initializing ZetaCore...")
        let engine = try ZetaCoreEngine()

        // Load model
        print("Loading model: \(modelPath)")
        try engine.loadModel(path: modelPath)

        if isBenchmark {
            // Run benchmark
            print("\n--- Benchmark Mode ---")
            print("Prompt: \"\(prompt)\"")
            print("Max tokens: \(maxTokens)")
            print("Running 3 iterations...\n")

            var totalTTFT: Double = 0
            var totalTokS: Double = 0
            var totalPrefill: Double = 0
            var totalGenerate: Double = 0

            for i in 1...3 {
                print("Run \(i):", terminator: " ")
                let results = try engine.generateWithBenchmark(prompt: prompt, maxTokens: maxTokens, silent: true)
                print("\(String(format: "%.1f", results.tokensPerSecond)) tok/s, TTFT: \(String(format: "%.1f", results.ttftMs))ms")

                totalTTFT += results.ttftMs
                totalTokS += results.tokensPerSecond
                totalPrefill += results.prefillTimeMs
                totalGenerate += results.generateTimeMs
            }

            print("\n" + String(repeating: "═", count: 40))
            print("         AVERAGE (3 runs)")
            print(String(repeating: "═", count: 40))
            print("TTFT:         \(String(format: "%6.1f", totalTTFT / 3.0)) ms")
            print("Prefill:      \(String(format: "%6.1f", totalPrefill / 3.0)) ms")
            print("Generation:   \(String(format: "%6.1f", totalGenerate / 3.0)) ms")
            print("Throughput:   \(String(format: "%6.2f", totalTokS / 3.0)) tok/s")
            print(String(repeating: "═", count: 40))
        } else {
            // Normal generation
            print("\n--- Generating ---")
            print("Prompt: \(prompt)")
            print("Max tokens: \(maxTokens)\n")

            let results = try engine.generateWithBenchmark(prompt: prompt, maxTokens: maxTokens, silent: false)
            print("\nOutput: \(results.output)")
            print(results.summary)
        }

        print("\n✅ Complete")
    }

    static func printUsage() {
        print("""
        Usage: ZetaCLI <model.gguf> [prompt] [max_tokens] [--benchmark]

        Arguments:
          model.gguf    Path to GGUF model file
          prompt        Input prompt (default: "Hello")
          max_tokens    Maximum tokens to generate (default: 50)
          --benchmark   Run benchmark mode (3 iterations, averages)

        Examples:
          ZetaCLI Models/tinyllama.gguf "The capital of France is" 20
          ZetaCLI Models/tinyllama.gguf "Hello" 50 --benchmark
        """)
    }
}
