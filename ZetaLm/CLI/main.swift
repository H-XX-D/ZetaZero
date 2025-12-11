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

        let modelPath = args[1]
        let prompt = args.count >= 3 ? args[2] : "Hello"
        let maxTokens = args.count >= 4 ? Int(args[3]) ?? 50 : 50

        // Initialize engine
        print("Initializing ZetaCore...")
        let engine = try ZetaCoreEngine()

        // Load model
        print("Loading model: \(modelPath)")
        try engine.loadModel(path: modelPath)

        // Generate
        print("\n--- Generating ---")
        print("Prompt: \(prompt)")
        print("Max tokens: \(maxTokens)\n")

        let output = try engine.generate(prompt: prompt, maxTokens: maxTokens)
        print("Output: \(output)")

        print("\n✅ Generation complete")
    }

    static func printUsage() {
        print("""
        Usage: ZetaCLI <model.gguf> [prompt] [max_tokens]

        Arguments:
          model.gguf    Path to GGUF model file
          prompt        Input prompt (default: "Hello")
          max_tokens    Maximum tokens to generate (default: 50)

        Example:
          ZetaCLI Models/tinyllama.gguf "The capital of France is" 20
        """)
    }
}
