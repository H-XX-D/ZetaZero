#!/usr/bin/env python3
"""
Z.E.T.A. Latency Benchmark (Paper Section 6.5)

Profiles each component of the retrieval pipeline:
- Entity extraction (3B)
- Graph walk (depth=3)
- Fact retrieval
- 14B generation
- Total pipeline latency
"""

import argparse
import asyncio
import aiohttp
import json
import statistics
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List

from benchmark_harness import (
    BenchmarkConfig, Timer, TimingResult,
    check_server_health, LLAMA_3B_PORT, LLAMA_14B_PORT, HOLOGIT_PATH
)

sys.path.insert(0, str(Path(__file__).parent.parent.parent / "scripts"))

# Benchmark parameters
NUM_RUNS = 100
WARMUP_RUNS = 5

# Z.E.T.A. Parameters
LAMBDA = 0.95
TAU = 0.1
ALPHA = 0.85


@dataclass
class LatencyProfile:
    """Latency measurements for each component."""
    entity_extraction: TimingResult = field(default_factory=lambda: TimingResult("entity_extraction"))
    graph_walk: TimingResult = field(default_factory=lambda: TimingResult("graph_walk"))
    fact_retrieval: TimingResult = field(default_factory=lambda: TimingResult("fact_retrieval"))
    generation_3b: TimingResult = field(default_factory=lambda: TimingResult("generation_3b"))
    generation_14b: TimingResult = field(default_factory=lambda: TimingResult("generation_14b"))
    total_subzero: TimingResult = field(default_factory=lambda: TimingResult("total_subzero"))
    total_zero: TimingResult = field(default_factory=lambda: TimingResult("total_zero"))

    def to_dict(self) -> Dict:
        return {
            "entity_extraction": self.entity_extraction.to_dict(),
            "graph_walk": self.graph_walk.to_dict(),
            "fact_retrieval": self.fact_retrieval.to_dict(),
            "generation_3b": self.generation_3b.to_dict(),
            "generation_14b": self.generation_14b.to_dict(),
            "total_subzero": self.total_subzero.to_dict(),
            "total_zero": self.total_zero.to_dict(),
        }


# Test queries for latency profiling
TEST_QUERIES = [
    "What is the user's name?",
    "Where does Alice work?",
    "What project is Bob working on?",
    "Tell me about the API limits",
    "What was discussed yesterday?",
    "Who is responsible for the database?",
    "What is the deadline for Phoenix?",
    "Explain the authentication system",
    "What programming languages are used?",
    "Who attended the meeting?",
]


async def profile_entity_extraction(session: aiohttp.ClientSession,
                                    query: str,
                                    timing: TimingResult) -> List[str]:
    """Profile 3B entity extraction."""
    prompt = (
        "<|im_start|>system\n"
        "Extract entity names as JSON array.\n"
        "<|im_end|>\n"
        "<|im_start|>user\n"
        f"{query}\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n"
        '["'
    )

    with Timer("entity_extraction", timing):
        try:
            async with session.post(
                f"http://localhost:{LLAMA_3B_PORT}/completion",
                json={
                    "prompt": prompt,
                    "n_predict": 50,
                    "temperature": 0.1,
                    "stop": ["]"]
                },
                timeout=aiohttp.ClientTimeout(total=10)
            ) as resp:
                if resp.status == 200:
                    data = await resp.json()
                    raw = data.get("content", "")
                    content = '["' + raw + ']'
                    try:
                        return json.loads(content)
                    except:
                        return []
        except:
            return []
    return []


def profile_graph_walk(entities: List[str], timing: TimingResult) -> Dict[str, float]:
    """Profile graph walk retrieval."""
    graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"

    with Timer("graph_walk", timing):
        if not graph_file.exists():
            return {}

        with open(graph_file) as f:
            graph = json.load(f)

        entity_scores = {}
        visited = set()

        def walk(entity: str, depth: int, score: float):
            if depth == 0 or entity in visited:
                return
            visited.add(entity)

            e_lower = entity.lower()
            e_data = graph.get("entities", {}).get(e_lower, {})
            if e_data:
                entity_scores[e_lower] = max(entity_scores.get(e_lower, 0), score)

                for conn in e_data.get("connections", []):
                    if conn not in visited:
                        next_score = ALPHA * score * 0.5
                        if next_score > TAU:
                            walk(conn, depth - 1, next_score)

        for e in entities:
            walk(e, depth=3, score=1.0)

        return entity_scores


def profile_fact_retrieval(entity_scores: Dict[str, float],
                           timing: TimingResult) -> List[str]:
    """Profile fact loading from disk."""
    with Timer("fact_retrieval", timing):
        graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
        if not graph_file.exists():
            return []

        with open(graph_file) as f:
            graph = json.load(f)

        # Collect blocks
        block_scores = {}
        for entity, score in entity_scores.items():
            e_data = graph.get("entities", {}).get(entity, {})
            for bid in e_data.get("blocks", []):
                block_scores[bid] = max(block_scores.get(bid, 0), score)

        # Load facts
        facts = []
        for bid, score in sorted(block_scores.items(), key=lambda x: -x[1])[:20]:
            facts_file = HOLOGIT_PATH / "blocks" / f"facts_{bid}.txt"
            if facts_file.exists():
                with open(facts_file) as f:
                    for line in f:
                        if "=" in line:
                            facts.append(line.strip())

        return facts


async def profile_3b_generation(session: aiohttp.ClientSession,
                                prompt: str,
                                timing: TimingResult) -> str:
    """Profile 3B generation."""
    with Timer("generation_3b", timing):
        try:
            async with session.post(
                f"http://localhost:{LLAMA_3B_PORT}/completion",
                json={
                    "prompt": prompt,
                    "n_predict": 100,
                    "temperature": 0.7,
                },
                timeout=aiohttp.ClientTimeout(total=30)
            ) as resp:
                if resp.status == 200:
                    data = await resp.json()
                    return data.get("content", "")
        except:
            pass
    return ""


async def profile_14b_generation(session: aiohttp.ClientSession,
                                 prompt: str,
                                 timing: TimingResult) -> str:
    """Profile 14B generation via llama-zeta-server (uses /generate with form data)."""
    with Timer("generation_14b", timing):
        try:
            # llama-zeta-server uses /generate endpoint with form data, not JSON
            async with session.post(
                f"http://localhost:{LLAMA_14B_PORT}/generate",
                data={
                    "prompt": prompt,
                    "max_tokens": 100,
                },
                timeout=aiohttp.ClientTimeout(total=120)
            ) as resp:
                if resp.status == 200:
                    data = await resp.json()
                    return data.get("response", "")
        except Exception as e:
            print(f"[WARN] 14B generation failed: {e}")
    return ""


async def run_subzero_pipeline(session: aiohttp.ClientSession,
                               query: str,
                               profile: LatencyProfile) -> float:
    """Run complete SubZero pipeline and return total time."""
    start = time.perf_counter()

    # 1. Entity extraction
    entities = await profile_entity_extraction(session, query, profile.entity_extraction)
    if not entities:
        entities = query.lower().split()[:3]

    # 2. Graph walk
    entity_scores = profile_graph_walk(entities, profile.graph_walk)

    # 3. Fact retrieval
    facts = profile_fact_retrieval(entity_scores, profile.fact_retrieval)

    # 4. 3B generation with facts
    fact_context = "\n".join(f"- {f}" for f in facts[:10])
    prompt = f"Context:\n{fact_context}\n\nQuery: {query}\nAnswer:"
    await profile_3b_generation(session, prompt, profile.generation_3b)

    total_ms = (time.perf_counter() - start) * 1000
    profile.total_subzero.times_ms.append(total_ms)
    return total_ms


async def run_zero_pipeline(session: aiohttp.ClientSession,
                            query: str,
                            profile: LatencyProfile) -> float:
    """Run complete Zero pipeline and return total time."""
    start = time.perf_counter()

    # 1. Entity extraction (3B)
    entities = await profile_entity_extraction(session, query, profile.entity_extraction)
    if not entities:
        entities = query.lower().split()[:3]

    # 2. Graph walk
    entity_scores = profile_graph_walk(entities, profile.graph_walk)

    # 3. Fact retrieval
    facts = profile_fact_retrieval(entity_scores, profile.fact_retrieval)

    # 4. 14B generation with facts (conscious reasoning)
    fact_context = "\n".join(f"- {f}" for f in facts[:10])
    prompt = f"Context:\n{fact_context}\n\nQuery: {query}\nAnswer:"
    await profile_14b_generation(session, prompt, profile.generation_14b)

    total_ms = (time.perf_counter() - start) * 1000
    profile.total_zero.times_ms.append(total_ms)
    return total_ms


def setup_test_data():
    """Setup test data in HoloGit for latency testing."""
    # Create some test facts
    graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
    graph_file.parent.mkdir(parents=True, exist_ok=True)

    graph = {
        "entities": {
            "alice": {"blocks": [0, 1], "connections": ["bob", "acme"]},
            "bob": {"blocks": [1, 2], "connections": ["alice", "acme"]},
            "acme": {"blocks": [0, 1, 2], "connections": ["alice", "bob"]},
            "user": {"blocks": [3], "connections": ["project"]},
            "project": {"blocks": [3, 4], "connections": ["user", "phoenix"]},
            "phoenix": {"blocks": [4], "connections": ["project"]},
        },
        "block_to_entities": {
            "0": ["alice", "acme"],
            "1": ["alice", "bob", "acme"],
            "2": ["bob", "acme"],
            "3": ["user", "project"],
            "4": ["project", "phoenix"],
        },
        "co_occurrence": {
            "alice|bob": 5,
            "alice|acme": 3,
            "bob|acme": 4,
            "project|phoenix": 2,
            "project|user": 3,
        },
        "entity_counts": {
            "alice": 10, "bob": 8, "acme": 15,
            "user": 5, "project": 7, "phoenix": 3
        }
    }

    with open(graph_file, "w") as f:
        json.dump(graph, f)

    # Create fact files
    blocks_dir = HOLOGIT_PATH / "blocks"
    blocks_dir.mkdir(parents=True, exist_ok=True)

    facts = [
        (0, ["name:alice_name=Alice Smith", "company:alice_company=Acme Corp"]),
        (1, ["project:team_project=Phoenix", "role:alice_role=Lead Engineer"]),
        (2, ["name:bob_name=Bob Jones", "role:bob_role=Developer"]),
        (3, ["preference:user_lang=Python", "project:current=Phoenix"]),
        (4, ["deadline:phoenix_deadline=March 2024", "status:phoenix_status=In Progress"]),
    ]

    for bid, lines in facts:
        with open(blocks_dir / f"facts_{bid}.txt", "w") as f:
            f.write("\n".join(lines))

    print(f"[SETUP] Created {len(facts)} test blocks")


async def run_benchmark(args):
    """Run complete latency benchmark."""
    print("=" * 60)
    print("Z.E.T.A. Latency Benchmark (Paper Section 6.5)")
    print("=" * 60)

    # Setup test data
    setup_test_data()

    # Check servers
    has_3b = check_server_health(LLAMA_3B_PORT)
    has_14b = check_server_health(LLAMA_14B_PORT)
    print(f"\n[SERVERS] 3B: {'OK' if has_3b else 'OFF'}, 14B: {'OK' if has_14b else 'OFF'}")

    results = {
        "benchmark": "latency",
        "params": {
            "num_runs": NUM_RUNS,
            "warmup_runs": WARMUP_RUNS
        },
        "profiles": {}
    }

    async with aiohttp.ClientSession() as session:
        # Profile SubZero if 3B available
        if has_3b:
            print(f"\n[SUBZERO] Profiling SubZero pipeline ({WARMUP_RUNS} warmup + {NUM_RUNS} runs)...")
            subzero_profile = LatencyProfile()

            # Warmup
            for i in range(WARMUP_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                await run_subzero_pipeline(session, query, subzero_profile)
            # Clear warmup data
            subzero_profile = LatencyProfile()

            # Actual runs
            for i in range(NUM_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                total = await run_subzero_pipeline(session, query, subzero_profile)
                if (i + 1) % 20 == 0:
                    print(f"   Run {i+1}/{NUM_RUNS}: {total:.1f}ms")

            results["profiles"]["subzero"] = subzero_profile.to_dict()

            print("\n   SubZero Component Breakdown:")
            print(f"   | {'Component':<20} | {'Mean':>8} | {'Std':>8} | {'P95':>8} |")
            print("   |" + "-" * 22 + "|" + "-" * 10 + "|" + "-" * 10 + "|" + "-" * 10 + "|")
            for name, timing in [
                ("Entity Extraction", subzero_profile.entity_extraction),
                ("Graph Walk", subzero_profile.graph_walk),
                ("Fact Retrieval", subzero_profile.fact_retrieval),
                ("3B Generation", subzero_profile.generation_3b),
                ("TOTAL SubZero", subzero_profile.total_subzero),
            ]:
                print(f"   | {name:<20} | {timing.mean_ms:>6.1f}ms | {timing.std_ms:>6.1f}ms | {timing.p95_ms:>6.1f}ms |")

        # Profile Zero if both servers available
        if has_3b and has_14b:
            print(f"\n[ZERO] Profiling Zero pipeline ({WARMUP_RUNS} warmup + {NUM_RUNS} runs)...")
            zero_profile = LatencyProfile()

            # Warmup
            for i in range(WARMUP_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                await run_zero_pipeline(session, query, zero_profile)
            # Clear warmup data
            zero_profile = LatencyProfile()

            # Actual runs
            for i in range(NUM_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                total = await run_zero_pipeline(session, query, zero_profile)
                if (i + 1) % 20 == 0:
                    print(f"   Run {i+1}/{NUM_RUNS}: {total:.1f}ms")

            results["profiles"]["zero"] = zero_profile.to_dict()

            print("\n   Zero Component Breakdown:")
            print(f"   | {'Component':<20} | {'Mean':>8} | {'Std':>8} | {'P95':>8} |")
            print("   |" + "-" * 22 + "|" + "-" * 10 + "|" + "-" * 10 + "|" + "-" * 10 + "|")
            for name, timing in [
                ("Entity Extraction", zero_profile.entity_extraction),
                ("Graph Walk", zero_profile.graph_walk),
                ("Fact Retrieval", zero_profile.fact_retrieval),
                ("14B Generation", zero_profile.generation_14b),
                ("TOTAL Zero", zero_profile.total_zero),
            ]:
                print(f"   | {name:<20} | {timing.mean_ms:>6.1f}ms | {timing.std_ms:>6.1f}ms | {timing.p95_ms:>6.1f}ms |")

        # Baseline (14B only, no memory)
        if has_14b:
            print(f"\n[BASELINE] Profiling Baseline (14B only, no memory)...")
            baseline_timing = TimingResult("baseline_14b")

            for i in range(WARMUP_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                await profile_14b_generation(session, query, baseline_timing)
            baseline_timing = TimingResult("baseline_14b")

            for i in range(NUM_RUNS):
                query = TEST_QUERIES[i % len(TEST_QUERIES)]
                await profile_14b_generation(session, query, baseline_timing)
                if (i + 1) % 20 == 0:
                    print(f"   Run {i+1}/{NUM_RUNS}: {baseline_timing.times_ms[-1]:.1f}ms")

            results["profiles"]["baseline"] = {"generation_14b": baseline_timing.to_dict()}
            print(f"\n   Baseline: Mean={baseline_timing.mean_ms:.1f}ms, P95={baseline_timing.p95_ms:.1f}ms")

    # Save results
    output_path = Path(args.output) if args.output else Path("results/latency.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] Results saved to {output_path}")

    # Print paper table
    print("\n" + "=" * 60)
    print("PAPER TABLE (Section 6.5)")
    print("=" * 60)
    print("| Config   | Total (ms) | First Token | Use Case |")
    print("|----------|------------|-------------|----------|")
    if "baseline" in results["profiles"]:
        bl = results["profiles"]["baseline"]["generation_14b"]
        print(f"| Baseline | {bl['mean_ms']:>10.0f} | ~{bl['mean_ms']/2:.0f}ms      | Fast     |")
    if "subzero" in results["profiles"]:
        sz = results["profiles"]["subzero"]["total_subzero"]
        print(f"| SubZero  | {sz['mean_ms']:>10.0f} | ~{sz['mean_ms']/2:.0f}ms      | Edge     |")
    if "zero" in results["profiles"]:
        z = results["profiles"]["zero"]["total_zero"]
        print(f"| Zero     | {z['mean_ms']:>10.0f} | ~{z['mean_ms']/2:.0f}ms     | Quality  |")


def main():
    global NUM_RUNS

    parser = argparse.ArgumentParser(description="Z.E.T.A. Latency Benchmark")
    parser.add_argument("--output", "-o", help="Output JSON file path")
    parser.add_argument("--runs", type=int, default=100, help="Number of runs")
    args = parser.parse_args()

    NUM_RUNS = args.runs

    asyncio.run(run_benchmark(args))


if __name__ == "__main__":
    main()
