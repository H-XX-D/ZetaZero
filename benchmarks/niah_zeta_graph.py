"""
Z.E.T.A. NIAH Benchmark - Knowledge Graph Version
==================================================
Uses Z.E.T.A.'s knowledge graph for fact storage/retrieval
instead of RAG-style chunking.

The graph is more efficient than RAG because:
1. Facts are extracted and stored semantically
2. Retrieval uses 4B embedding similarity
3. No chunking overhead - direct node lookup
"""

import requests
import json
import time
import argparse
import numpy as np
import matplotlib.pyplot as plt
import os

class ZetaGraphProvider:
    """Provider using Z.E.T.A.'s knowledge graph for NIAH"""

    def __init__(self, base_url: str = "http://192.168.0.165:8080"):
        self.base_url = base_url
        self.model_name = "ZETA-14B-Graph"

    def generate(self, prompt: str, max_tokens: int = 100) -> str:
        """Generate response - also extracts facts to graph"""
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": prompt, "max_tokens": max_tokens},
                timeout=300
            )
            response.raise_for_status()
            return response.json().get("output", "")
        except Exception as e:
            print(f"[ZETA] Generate error: {e}")
            return ""

    def store_fact(self, fact: str) -> dict:
        """Store a fact in the knowledge graph using Remember: prefix"""
        prompt = f"Remember: {fact}"
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": prompt, "max_tokens": 50},
                timeout=30
            )
            response.raise_for_status()
            data = response.json()
            return {
                "stored": True,
                "graph_nodes": data.get("graph_nodes", 0),
                "graph_edges": data.get("graph_edges", 0)
            }
        except Exception as e:
            return {"stored": False, "error": str(e)}

    def query_graph(self, query: str, top_k: int = 5) -> list:
        """Query the knowledge graph for relevant facts"""
        try:
            response = requests.post(
                f"{self.base_url}/memory/query",
                json={"query": query, "top_k": top_k},
                timeout=30
            )
            response.raise_for_status()
            data = response.json()
            return data.get("results", [])
        except Exception as e:
            print(f"[ZETA] Query error: {e}")
            return []

    def clear_graph(self) -> bool:
        """Clear the knowledge graph (if endpoint exists)"""
        # Note: May need to restart server to fully clear
        return True

    def get_graph_stats(self) -> dict:
        """Get current graph statistics"""
        try:
            # Use a simple generate to get stats
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": "status", "max_tokens": 1},
                timeout=10
            )
            response.raise_for_status()
            data = response.json()
            return {
                "nodes": data.get("graph_nodes", 0),
                "edges": data.get("graph_edges", 0)
            }
        except:
            return {"nodes": 0, "edges": 0}


class NIAHGraphTest:
    """
    NIAH using Knowledge Graph storage/retrieval.

    Instead of RAG:
    1. Store needle in graph via "Remember:"
    2. Store haystack facts in graph
    3. Query graph to retrieve needle
    4. Generate answer with retrieved context
    """

    # The needle fact
    NEEDLE = "The best thing to do in San Francisco is eat a sandwich and sit in Dolores Park on a sunny day."
    QUESTION = "What is the best thing to do in San Francisco?"

    def __init__(self, provider: ZetaGraphProvider):
        self.provider = provider
        self.results = []

    def generate_haystack_facts(self, count: int) -> list:
        """Generate distractor facts for the haystack"""
        facts = [
            "The capital of France is Paris and it has the Eiffel Tower.",
            "Python is a programming language created by Guido van Rossum.",
            "The Great Wall of China is over 13,000 miles long.",
            "Water boils at 100 degrees Celsius at sea level.",
            "The Amazon rainforest produces 20% of the world's oxygen.",
            "Shakespeare wrote 37 plays during his lifetime.",
            "Mount Everest is the tallest mountain on Earth at 8,849 meters.",
            "The human brain contains approximately 86 billion neurons.",
            "Bitcoin was created in 2009 by Satoshi Nakamoto.",
            "The speed of light is approximately 299,792 kilometers per second.",
            "Tokyo is the most populous city in the world.",
            "The Mona Lisa was painted by Leonardo da Vinci.",
            "DNA was discovered by Watson and Crick in 1953.",
            "The Pacific Ocean is the largest ocean on Earth.",
            "Albert Einstein developed the theory of relativity.",
            "The first computer was called ENIAC and was built in 1945.",
            "Mars is known as the Red Planet due to iron oxide.",
            "The Nile is the longest river in the world.",
            "Honey never spoils and has been found in Egyptian tombs.",
            "Octopuses have three hearts and blue blood.",
        ]
        # Extend if needed
        while len(facts) < count:
            facts.extend(facts)
        return facts[:count]

    def run_single_test(self, num_distractors: int, needle_position: float) -> dict:
        """
        Run single NIAH test using knowledge graph.

        num_distractors: Number of distractor facts to store
        needle_position: 0.0=first, 0.5=middle, 1.0=last (when needle is stored)
        """
        print(f"  Testing distractors={num_distractors}, position={needle_position:.1%}... ", end="", flush=True)

        start_time = time.time()

        # Generate distractor facts
        distractors = self.generate_haystack_facts(num_distractors)

        # Calculate where to insert needle
        insert_pos = int(len(distractors) * needle_position)

        # Store facts in order (distractors before needle, then after)
        facts_stored = 0

        # Store distractors before needle position
        for i, fact in enumerate(distractors[:insert_pos]):
            result = self.provider.store_fact(fact)
            if result.get("stored"):
                facts_stored += 1

        # Store the needle
        needle_result = self.provider.store_fact(self.NEEDLE)
        needle_stored = needle_result.get("stored", False)

        # Store remaining distractors
        for fact in distractors[insert_pos:]:
            result = self.provider.store_fact(fact)
            if result.get("stored"):
                facts_stored += 1

        storage_time = time.time() - start_time

        # Query the graph
        query_start = time.time()
        results = self.provider.query_graph(self.QUESTION, top_k=5)
        query_time = time.time() - query_start

        # Check if needle was retrieved
        needle_found = False
        needle_rank = -1
        for i, r in enumerate(results):
            value = r.get("value", "").lower()
            if "sandwich" in value or "dolores" in value or "san francisco" in value:
                needle_found = True
                needle_rank = i + 1
                break

        # Generate response using retrieved context
        if results:
            context = "\n".join([f"- {r.get('value', '')}" for r in results[:3]])
            prompt = f"""Based on these facts:
{context}

Question: {self.QUESTION}
Answer:"""
            response = self.provider.generate(prompt, max_tokens=100)
        else:
            response = self.provider.generate(self.QUESTION, max_tokens=100)

        elapsed = time.time() - start_time

        # Score based on needle retrieval and response quality
        score = 0
        if needle_found:
            score += 5  # Needle was retrieved
            if needle_rank == 1:
                score += 3  # Top result
            elif needle_rank <= 3:
                score += 1  # In top 3

        # Check response quality
        response_lower = response.lower()
        if "sandwich" in response_lower:
            score += 1
        if "dolores" in response_lower or "park" in response_lower:
            score += 1

        result = {
            "num_distractors": num_distractors,
            "needle_position": needle_position,
            "facts_stored": facts_stored,
            "needle_stored": needle_stored,
            "needle_found": needle_found,
            "needle_rank": needle_rank,
            "score": min(10.0, score),
            "storage_time": storage_time,
            "query_time": query_time,
            "total_time": elapsed,
            "response": response[:200]
        }

        status = "FOUND" if needle_found else "MISS"
        print(f"Score: {score}/10 ({status}, rank={needle_rank}, {elapsed:.1f}s)")

        return result

    def run_full_test(self,
                      distractor_counts: list = [5, 10, 20, 50, 100],
                      positions: list = [0.1, 0.5, 0.9]) -> list:
        """Run full NIAH test matrix using knowledge graph"""
        print(f"\n{'='*60}")
        print(f"NEEDLE IN A HAYSTACK - Z.E.T.A. Knowledge Graph")
        print(f"{'='*60}")
        print(f"Model: {self.provider.model_name}")
        print(f"Distractor counts: {distractor_counts}")
        print(f"Needle positions: {[f'{p:.0%}' for p in positions]}")
        print(f"{'='*60}")

        initial_stats = self.provider.get_graph_stats()
        print(f"Initial graph: {initial_stats['nodes']} nodes, {initial_stats['edges']} edges\n")

        self.results = []

        for count in distractor_counts:
            print(f"\nDistractors: {count}")
            print("-" * 40)

            for pos in positions:
                result = self.run_single_test(count, pos)
                self.results.append(result)

        final_stats = self.provider.get_graph_stats()
        print(f"\nFinal graph: {final_stats['nodes']} nodes, {final_stats['edges']} edges")

        return self.results

    def generate_heatmap(self, output_path: str = "niah_graph_heatmap.png"):
        """Generate visualization heatmap"""
        if not self.results:
            print("No results to visualize")
            return

        counts = sorted(set(r["num_distractors"] for r in self.results))
        positions = sorted(set(r["needle_position"] for r in self.results))

        score_matrix = np.zeros((len(positions), len(counts)))

        for r in self.results:
            i = positions.index(r["needle_position"])
            j = counts.index(r["num_distractors"])
            score_matrix[i, j] = r["score"]

        fig, ax = plt.subplots(figsize=(12, 8))

        from matplotlib.colors import LinearSegmentedColormap
        colors = ['#FF0000', '#FFFF00', '#00FF00']
        cmap = LinearSegmentedColormap.from_list('niah', colors)

        im = ax.imshow(score_matrix, cmap=cmap, aspect='auto', vmin=0, vmax=10)

        ax.set_xticks(range(len(counts)))
        ax.set_xticklabels([str(c) for c in counts])
        ax.set_yticks(range(len(positions)))
        ax.set_yticklabels([f"{p:.0%}" for p in positions])

        ax.set_xlabel("Number of Distractor Facts", fontsize=12)
        ax.set_ylabel("Needle Position", fontsize=12)
        ax.set_title(f"NIAH - Z.E.T.A. Knowledge Graph\n"
                     f"RTX 5060 Ti 16GB | 4B Embedding | Semantic Retrieval", fontsize=14)

        for i in range(len(positions)):
            for j in range(len(counts)):
                score = score_matrix[i, j]
                color = 'white' if score < 5 else 'black'
                ax.text(j, i, f"{score:.0f}", ha='center', va='center',
                       color=color, fontsize=12, fontweight='bold')

        cbar = plt.colorbar(im)
        cbar.set_label("Retrieval Score (0-10)", fontsize=10)

        plt.tight_layout()
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"\nHeatmap saved to: {output_path}")

    def save_results(self, output_path: str = "niah_graph_results.json"):
        """Save detailed results"""
        output = {
            "benchmark": "NIAH-KnowledgeGraph",
            "model": self.provider.model_name,
            "method": "Knowledge Graph Storage + 4B Semantic Retrieval",
            "test_date": time.strftime("%Y-%m-%d %H:%M:%S"),
            "results": self.results,
            "summary": {
                "avg_score": np.mean([r["score"] for r in self.results]),
                "needle_found_rate": sum(1 for r in self.results if r["needle_found"]) / len(self.results),
                "avg_query_time_ms": np.mean([r["query_time"] * 1000 for r in self.results]),
                "total_tests": len(self.results)
            }
        }

        with open(output_path, 'w') as f:
            json.dump(output, f, indent=2)

        print(f"Results saved to: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="NIAH Benchmark using Z.E.T.A. Knowledge Graph")
    parser.add_argument("--host", default="192.168.0.165")
    parser.add_argument("--port", default=8080, type=int)
    parser.add_argument("--distractors", nargs="+", type=int, default=[5, 10, 20, 50, 100])
    parser.add_argument("--positions", nargs="+", type=float, default=[0.1, 0.5, 0.9])
    parser.add_argument("--output", default="niah_graph")

    args = parser.parse_args()

    provider = ZetaGraphProvider(f"http://{args.host}:{args.port}")

    # Test connection
    print("Testing connection to Z.E.T.A. server...")
    stats = provider.get_graph_stats()
    print(f"Connected! Graph has {stats['nodes']} nodes, {stats['edges']} edges")

    # Run test
    test = NIAHGraphTest(provider)
    test.run_full_test(args.distractors, args.positions)

    # Generate outputs
    test.generate_heatmap(f"{args.output}_heatmap.png")
    test.save_results(f"{args.output}_results.json")

    # Summary
    print(f"\n{'='*60}")
    print("SUMMARY")
    print(f"{'='*60}")
    avg_score = np.mean([r["score"] for r in test.results])
    found_rate = sum(1 for r in test.results if r["needle_found"]) / len(test.results)
    print(f"Average Score: {avg_score:.1f}/10")
    print(f"Needle Found Rate: {found_rate:.1%}")
    print(f"Method: Knowledge Graph (NOT RAG)")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
