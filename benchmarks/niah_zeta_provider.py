"""
Z.E.T.A. Provider for Needle In A Haystack (NIAH) Benchmark
============================================================
Adapter for Greg Kamradt's LLMTest_NeedleInAHaystack

Usage:
    1. Clone: git clone https://github.com/gkamradt/LLMTest_NeedleInAHaystack
    2. Copy this file to that repo
    3. Run: python needle_in_haystack.py --provider zeta

Or standalone:
    python niah_zeta_provider.py --context_lengths 1000 4000 8000 12000
"""

import requests
import json
import time
import argparse
import numpy as np
import matplotlib.pyplot as plt
from typing import Optional
import os

class ZetaProvider:
    """Provider adapter for Z.E.T.A. local inference server"""

    def __init__(self, base_url: str = "http://192.168.0.165:8080"):
        self.base_url = base_url
        self.model_name = "ZETA-14B-Local"
        self.native_context = 4096  # Model's native context window
        self.chunk_size = 3000  # Leave room for question + response

    def generate(self, prompt: str, max_tokens: int = 100) -> str:
        """Generate response from Z.E.T.A. server"""
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": prompt, "max_tokens": max_tokens},
                headers={"Content-Type": "application/json"},
                timeout=300  # 5 minute timeout for long contexts
            )
            response.raise_for_status()
            data = response.json()
            return data.get("output", "")
        except Exception as e:
            print(f"[ZETA] Error: {e}")
            return ""

    def embed(self, text: str) -> list:
        """Get embedding from Z.E.T.A. 4B embedding model"""
        try:
            response = requests.post(
                f"{self.base_url}/embedding",
                json={"content": text},
                headers={"Content-Type": "application/json"},
                timeout=30
            )
            response.raise_for_status()
            data = response.json()
            return data.get("embedding", [])
        except Exception as e:
            return []

    def semantic_search(self, query: str, chunks: list) -> list:
        """Find most relevant chunks using 4B embedding model"""
        query_embed = self.embed(query)
        if not query_embed:
            # Fallback: return all chunks
            return chunks

        scored = []
        for chunk in chunks:
            chunk_embed = self.embed(chunk[:500])  # Embed first 500 chars
            if chunk_embed:
                # Cosine similarity
                dot = sum(a * b for a, b in zip(query_embed, chunk_embed))
                norm_q = sum(a * a for a in query_embed) ** 0.5
                norm_c = sum(a * a for a in chunk_embed) ** 0.5
                sim = dot / (norm_q * norm_c + 1e-8)
                scored.append((sim, chunk))

        # Return top 3 most relevant chunks
        scored.sort(reverse=True)
        return [chunk for _, chunk in scored[:3]]

    def generate_with_chunked_input(self, context: str, question: str, max_tokens: int = 100) -> str:
        """
        Handle long input by chunking and using semantic search.
        This enables NIAH on contexts longer than native context window.
        """
        # Estimate tokens (rough: 4 chars per token)
        estimated_tokens = len(context) // 4

        if estimated_tokens <= self.native_context - 500:
            # Fits in context window - use directly
            prompt = f"""<|im_start|>system
You are a helpful assistant. Answer based on the context.
<|im_end|>
<|im_start|>user
Context:
{context}

Question: {question}
<|im_end|>
<|im_start|>assistant
"""
            return self.generate(prompt, max_tokens)

        # Long context: chunk and search
        print("[CHUNK-INPUT] ", end="", flush=True)

        # Split into chunks
        chunk_chars = self.chunk_size * 4  # Convert to chars
        chunks = []
        for i in range(0, len(context), chunk_chars):
            chunk = context[i:i + chunk_chars]
            if chunk.strip():
                chunks.append(chunk)

        print(f"{len(chunks)} chunks, ", end="", flush=True)

        # Semantic search for relevant chunks
        relevant = self.semantic_search(question, chunks)
        print(f"top {len(relevant)} relevant, ", end="", flush=True)

        # Combine relevant chunks
        combined = "\n\n---\n\n".join(relevant)

        # Generate with relevant context only
        prompt = f"""<|im_start|>system
You are a helpful assistant. Answer based on the context provided.
<|im_end|>
<|im_start|>user
Context (relevant excerpts):
{combined}

Question: {question}
Answer concisely based only on the context above.
<|im_end|>
<|im_start|>assistant
"""
        return self.generate(prompt, max_tokens)

    def get_model_info(self) -> dict:
        """Get model information"""
        return {
            "name": self.model_name,
            "provider": "ZETA-Local",
            "context_window": 12000,  # Effective with chunked input + semantic search
            "native_context": self.native_context,
            "gpu": "RTX 5060 Ti 16GB",
            "memory_usage_mb": 64,  # Scratch buffer size
            "retrieval": "4B Embedding + Semantic Search"
        }


class NeedleInHaystackTest:
    """
    Needle In A Haystack (NIAH) benchmark implementation
    Tests model's ability to retrieve a fact at various depths in long context
    """

    # The "needle" - a random fact that doesn't fit the context
    NEEDLE = "The best thing to do in San Francisco is eat a sandwich and sit in Dolores Park on a sunny day."

    # The question to retrieve the needle
    QUESTION = "What is the best thing to do in San Francisco?"

    # Haystack content (Paul Graham essays or similar)
    HAYSTACK_PATH = "paulgraham_essays.txt"

    def __init__(self, provider: ZetaProvider):
        self.provider = provider
        self.results = []

    def load_haystack(self) -> str:
        """Load or generate haystack content"""
        if os.path.exists(self.HAYSTACK_PATH):
            with open(self.HAYSTACK_PATH, 'r') as f:
                return f.read()
        else:
            # Generate synthetic haystack if file doesn't exist
            return self._generate_synthetic_haystack()

    def _generate_synthetic_haystack(self) -> str:
        """Generate synthetic essay-like content for testing"""
        paragraphs = [
            "The art of programming is the art of organizing complexity. "
            "Every program is a model of something, and the programmer must understand "
            "not just the problem domain but also the solution space.",

            "When we write software, we are building machines made of ideas. "
            "Unlike physical machines, software can be infinitely copied and modified. "
            "This gives us tremendous power but also tremendous responsibility.",

            "The best code is no code at all. Every line of code you write is a line "
            "that can contain bugs, that must be understood by others, and that must "
            "be maintained over time.",

            "Simplicity is the ultimate sophistication. A simple solution that works "
            "is better than a complex solution that works. The complex solution has "
            "more ways to fail.",

            "Programs must be written for people to read, and only incidentally for "
            "machines to execute. The reader of your code is the most important person "
            "you need to communicate with.",
        ]

        # Repeat and shuffle to create long content
        content = ""
        for i in range(500):  # ~50k words
            for p in paragraphs:
                content += p + "\n\n"
        return content

    def insert_needle(self, haystack: str, depth: float) -> str:
        """Insert needle at specified depth (0.0 = beginning, 1.0 = end)"""
        words = haystack.split()
        insert_pos = int(len(words) * depth)

        # Insert needle
        words_before = words[:insert_pos]
        words_after = words[insert_pos:]

        result = " ".join(words_before) + "\n\n" + self.NEEDLE + "\n\n" + " ".join(words_after)
        return result

    def trim_to_length(self, text: str, target_tokens: int) -> str:
        """Trim text to approximate token count (rough: 1 token ~ 4 chars)"""
        target_chars = target_tokens * 4
        if len(text) > target_chars:
            return text[:target_chars]
        return text

    def build_prompt(self, context: str) -> str:
        """Build the full prompt for the test"""
        return f"""<|im_start|>system
You are a helpful assistant. Answer the question based on the context provided.
<|im_end|>
<|im_start|>user
Context:
{context}

Question: {self.QUESTION}

Answer the question based only on the context above. Be concise.
<|im_end|>
<|im_start|>assistant
"""

    def evaluate_response(self, response: str) -> float:
        """Score response 0-10 based on needle retrieval"""
        response_lower = response.lower()

        # Key phrases from the needle
        key_phrases = [
            "sandwich",
            "dolores park",
            "san francisco",
            "sunny day",
            "eat",
            "sit"
        ]

        score = 0
        for phrase in key_phrases:
            if phrase in response_lower:
                score += 1.67  # Each phrase worth ~1.67 points (6 phrases = 10)

        return min(10.0, score)

    def run_single_test(self, context_length: int, depth: float) -> dict:
        """Run a single needle test at given context length and depth"""
        print(f"  Testing context={context_length}, depth={depth:.1%}... ", end="", flush=True)

        # Load and prepare haystack
        haystack = self.load_haystack()
        haystack = self.trim_to_length(haystack, context_length - 200)  # Leave room for needle + question

        # Insert needle
        context = self.insert_needle(haystack, depth)

        # Generate response using chunked input for long contexts
        start_time = time.time()
        response = self.provider.generate_with_chunked_input(context, self.QUESTION, max_tokens=100)
        elapsed = time.time() - start_time

        # Evaluate
        score = self.evaluate_response(response)

        result = {
            "context_length": context_length,
            "depth": depth,
            "score": score,
            "response": response[:200],
            "elapsed_seconds": elapsed
        }

        print(f"Score: {score:.1f}/10 ({elapsed:.1f}s)")
        return result

    def run_full_test(self,
                      context_lengths: list = [1000, 2000, 4000, 8000, 12000],
                      depths: list = [0.1, 0.25, 0.5, 0.75, 0.9]) -> list:
        """Run full NIAH test matrix"""
        print(f"\n{'='*60}")
        print(f"NEEDLE IN A HAYSTACK - Z.E.T.A. Benchmark")
        print(f"{'='*60}")
        print(f"Model: {self.provider.model_name}")
        print(f"Context lengths: {context_lengths}")
        print(f"Depths: {[f'{d:.0%}' for d in depths]}")
        print(f"{'='*60}\n")

        self.results = []

        for ctx_len in context_lengths:
            print(f"\nContext Length: {ctx_len} tokens")
            print("-" * 40)

            for depth in depths:
                result = self.run_single_test(ctx_len, depth)
                self.results.append(result)

        return self.results

    def generate_heatmap(self, output_path: str = "niah_zeta_heatmap.png"):
        """Generate visualization heatmap"""
        if not self.results:
            print("No results to visualize")
            return

        # Extract unique values
        context_lengths = sorted(set(r["context_length"] for r in self.results))
        depths = sorted(set(r["depth"] for r in self.results))

        # Build score matrix
        score_matrix = np.zeros((len(depths), len(context_lengths)))

        for r in self.results:
            i = depths.index(r["depth"])
            j = context_lengths.index(r["context_length"])
            score_matrix[i, j] = r["score"]

        # Create heatmap
        fig, ax = plt.subplots(figsize=(12, 8))

        # Custom colormap: red (0) -> yellow (5) -> green (10)
        from matplotlib.colors import LinearSegmentedColormap
        colors = ['#FF0000', '#FFFF00', '#00FF00']
        cmap = LinearSegmentedColormap.from_list('niah', colors)

        im = ax.imshow(score_matrix, cmap=cmap, aspect='auto', vmin=0, vmax=10)

        # Labels
        ax.set_xticks(range(len(context_lengths)))
        ax.set_xticklabels([f"{c//1000}k" for c in context_lengths])
        ax.set_yticks(range(len(depths)))
        ax.set_yticklabels([f"{d:.0%}" for d in depths])

        ax.set_xlabel("Context Length (tokens)", fontsize=12)
        ax.set_ylabel("Needle Depth", fontsize=12)
        ax.set_title(f"Needle In A Haystack - Z.E.T.A. on RTX 5060 Ti (16GB)\n"
                     f"Memory: 64MB Scratch Buffer | Context Bridge Enabled", fontsize=14)

        # Add score annotations
        for i in range(len(depths)):
            for j in range(len(context_lengths)):
                score = score_matrix[i, j]
                color = 'white' if score < 5 else 'black'
                ax.text(j, i, f"{score:.1f}", ha='center', va='center',
                       color=color, fontsize=10, fontweight='bold')

        # Colorbar
        cbar = plt.colorbar(im)
        cbar.set_label("Retrieval Score (0-10)", fontsize=10)

        plt.tight_layout()
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"\nHeatmap saved to: {output_path}")

        return output_path

    def save_results(self, output_path: str = "niah_zeta_results.json"):
        """Save detailed results to JSON"""
        output = {
            "model": self.provider.get_model_info(),
            "test_date": time.strftime("%Y-%m-%d %H:%M:%S"),
            "results": self.results,
            "summary": {
                "avg_score": np.mean([r["score"] for r in self.results]),
                "min_score": min(r["score"] for r in self.results),
                "max_score": max(r["score"] for r in self.results),
                "total_tests": len(self.results)
            }
        }

        with open(output_path, 'w') as f:
            json.dump(output, f, indent=2)

        print(f"Results saved to: {output_path}")
        return output_path


def main():
    parser = argparse.ArgumentParser(description="NIAH Benchmark for Z.E.T.A.")
    parser.add_argument("--host", default="192.168.0.165", help="Z.E.T.A. server host")
    parser.add_argument("--port", default=8080, type=int, help="Z.E.T.A. server port")
    parser.add_argument("--context_lengths", nargs="+", type=int,
                        default=[1000, 2000, 4000, 8000, 12000],
                        help="Context lengths to test")
    parser.add_argument("--depths", nargs="+", type=float,
                        default=[0.1, 0.25, 0.5, 0.75, 0.9],
                        help="Needle depths to test (0.0-1.0)")
    parser.add_argument("--output", default="niah_zeta", help="Output file prefix")

    args = parser.parse_args()

    # Initialize provider
    provider = ZetaProvider(f"http://{args.host}:{args.port}")

    # Check connection
    print("Testing connection to Z.E.T.A. server...")
    test_response = provider.generate("Hello", max_tokens=10)
    if not test_response:
        print("ERROR: Could not connect to Z.E.T.A. server")
        print(f"Make sure server is running at http://{args.host}:{args.port}")
        return
    print(f"Connected! Test response: {test_response[:50]}...")

    # Run test
    test = NeedleInHaystackTest(provider)
    test.run_full_test(args.context_lengths, args.depths)

    # Generate outputs
    test.generate_heatmap(f"{args.output}_heatmap.png")
    test.save_results(f"{args.output}_results.json")

    # Print summary
    print(f"\n{'='*60}")
    print("SUMMARY")
    print(f"{'='*60}")
    print(f"Average Score: {np.mean([r['score'] for r in test.results]):.1f}/10")
    print(f"Perfect Scores (10/10): {sum(1 for r in test.results if r['score'] == 10)}/{len(test.results)}")
    print(f"GPU Memory: 64MB (Scratch Buffer)")
    print(f"Max Context Tested: {max(args.context_lengths)} tokens")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
