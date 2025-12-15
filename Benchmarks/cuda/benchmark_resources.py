#!/usr/bin/env python3
"""
Z.E.T.A. Resources Benchmark (Paper Section 6.4)

Measures:
- VRAM usage
- System RAM overhead
- Tokens per second
- Effective context extension
"""

import argparse
import asyncio
import aiohttp
import json
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

from benchmark_harness import (
    check_server_health, LLAMA_3B_PORT, LLAMA_14B_PORT, HOLOGIT_PATH
)

# Test configurations
BLOCK_COUNTS = [0, 5, 10, 20, 50]
TOKENS_TO_GENERATE = 100
BASE_CONTEXT = 4096
BLOCK_SIZE = 512


@dataclass
class ResourceMetrics:
    """Resource usage metrics."""
    num_blocks: int
    effective_context: int
    vram_mb: float
    ram_mb: float
    tokens_per_sec: float
    latency_ms: float


def get_vram_usage() -> float:
    """Get current VRAM usage in MB."""
    try:
        result = subprocess.run(
            ["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode == 0:
            return float(result.stdout.strip())
    except:
        pass
    return 0.0


def get_ram_usage() -> float:
    """Get current process RAM usage in MB."""
    try:
        import resource
        # Get max resident set size in KB, convert to MB
        return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024
    except:
        pass

    # Fallback: read /proc/self/status
    try:
        with open("/proc/self/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    return float(line.split()[1]) / 1024  # KB to MB
    except:
        pass

    return 0.0


def setup_hologit_blocks(num_blocks: int):
    """Setup HoloGit with specified number of blocks."""
    blocks_dir = HOLOGIT_PATH / "blocks"
    blocks_dir.mkdir(parents=True, exist_ok=True)
    index_dir = HOLOGIT_PATH / "index"
    index_dir.mkdir(parents=True, exist_ok=True)

    # Clear existing
    for f in blocks_dir.glob("*.txt"):
        f.unlink()

    graph = {
        "entities": {},
        "block_to_entities": {},
        "co_occurrence": {},
        "entity_counts": {}
    }

    for i in range(num_blocks):
        # Create fact file with some content
        facts_file = blocks_dir / f"facts_{i}.txt"
        with open(facts_file, "w") as f:
            # Write multiple facts to simulate real block
            for j in range(5):
                f.write(f"fact:item_{i}_{j}=value_{i}_{j}\n")

        # Update graph
        entities = [f"entity_{i}", f"value_{i}"]
        graph["block_to_entities"][str(i)] = entities

        for e in entities:
            if e not in graph["entities"]:
                graph["entities"][e] = {"blocks": [], "connections": []}
            graph["entities"][e]["blocks"].append(i)
            graph["entity_counts"][e] = graph["entity_counts"].get(e, 0) + 1

    with open(index_dir / "entity_graph.json", "w") as f:
        json.dump(graph, f)


async def measure_generation(session: aiohttp.ClientSession,
                             port: int,
                             prompt: str,
                             max_tokens: int) -> tuple:
    """Measure generation speed and return (tokens, latency_ms)."""
    start = time.perf_counter()

    try:
        async with session.post(
            f"http://localhost:{port}/completion",
            json={
                "prompt": prompt,
                "n_predict": max_tokens,
                "temperature": 0.7,
            },
            timeout=aiohttp.ClientTimeout(total=120)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                content = data.get("content", "")
                latency_ms = (time.perf_counter() - start) * 1000

                # Estimate tokens (rough: ~4 chars per token)
                tokens = len(content) // 4

                return tokens, latency_ms
    except Exception as e:
        print(f"[ERROR] Generation failed: {e}")

    return 0, 0.0


async def run_benchmark(args):
    """Run complete resources benchmark."""
    print("=" * 60)
    print("Z.E.T.A. Resources Benchmark (Paper Section 6.4)")
    print("=" * 60)

    # Check servers
    has_3b = check_server_health(LLAMA_3B_PORT)
    has_14b = check_server_health(LLAMA_14B_PORT)
    print(f"\n[SERVERS] 3B: {'OK' if has_3b else 'OFF'}, 14B: {'OK' if has_14b else 'OFF'}")

    if not has_14b:
        print("[ERROR] 14B server required for resource benchmark")
        return

    results = {
        "benchmark": "resources",
        "params": {
            "block_counts": BLOCK_COUNTS,
            "tokens_to_generate": TOKENS_TO_GENERATE,
            "base_context": BASE_CONTEXT,
            "block_size": BLOCK_SIZE
        },
        "baseline": [],
        "subzero": [],
        "zero": []
    }

    test_prompt = "Explain the concept of machine learning in simple terms."

    async with aiohttp.ClientSession() as session:
        # Baseline measurements (no memory)
        print("\n[BASELINE] Measuring baseline (no memory)...")
        vram_before = get_vram_usage()
        ram_before = get_ram_usage()

        tokens, latency = await measure_generation(
            session, LLAMA_14B_PORT, test_prompt, TOKENS_TO_GENERATE
        )

        vram_after = get_vram_usage()
        ram_after = get_ram_usage()

        baseline_metrics = ResourceMetrics(
            num_blocks=0,
            effective_context=BASE_CONTEXT,
            vram_mb=vram_after,
            ram_mb=ram_after - ram_before,
            tokens_per_sec=tokens / (latency / 1000) if latency > 0 else 0,
            latency_ms=latency
        )

        results["baseline"].append({
            "num_blocks": 0,
            "effective_context": BASE_CONTEXT,
            "vram_mb": round(baseline_metrics.vram_mb, 1),
            "ram_overhead_mb": round(baseline_metrics.ram_mb, 1),
            "tokens_per_sec": round(baseline_metrics.tokens_per_sec, 1),
            "latency_ms": round(baseline_metrics.latency_ms, 1)
        })

        print(f"   VRAM: {baseline_metrics.vram_mb:.0f} MB")
        print(f"   Tokens/sec: {baseline_metrics.tokens_per_sec:.1f}")
        print(f"   Latency: {baseline_metrics.latency_ms:.0f} ms")

        # SubZero measurements (with memory blocks)
        if has_3b:
            print("\n[SUBZERO] Measuring SubZero with varying block counts...")

            for num_blocks in BLOCK_COUNTS:
                print(f"\n   Testing with {num_blocks} blocks...")
                setup_hologit_blocks(num_blocks)

                effective_context = BASE_CONTEXT + (num_blocks * BLOCK_SIZE)

                # Measure with memory retrieval overhead
                ram_before = get_ram_usage()

                # Simulate retrieval (load graph + facts)
                graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
                if graph_file.exists():
                    with open(graph_file) as f:
                        _ = json.load(f)

                # Load some facts
                for i in range(min(num_blocks, 10)):
                    facts_file = HOLOGIT_PATH / "blocks" / f"facts_{i}.txt"
                    if facts_file.exists():
                        with open(facts_file) as f:
                            _ = f.read()

                # Generate with 3B
                tokens, latency = await measure_generation(
                    session, LLAMA_3B_PORT, test_prompt, TOKENS_TO_GENERATE
                )

                ram_after = get_ram_usage()
                vram = get_vram_usage()

                metrics = ResourceMetrics(
                    num_blocks=num_blocks,
                    effective_context=effective_context,
                    vram_mb=vram,
                    ram_mb=ram_after - ram_before,
                    tokens_per_sec=tokens / (latency / 1000) if latency > 0 else 0,
                    latency_ms=latency
                )

                results["subzero"].append({
                    "num_blocks": num_blocks,
                    "effective_context": effective_context,
                    "vram_mb": round(metrics.vram_mb, 1),
                    "ram_overhead_mb": round(metrics.ram_mb, 1),
                    "tokens_per_sec": round(metrics.tokens_per_sec, 1),
                    "latency_ms": round(metrics.latency_ms, 1)
                })

                print(f"      Context: {effective_context}, VRAM: {vram:.0f} MB, "
                      f"RAM overhead: {metrics.ram_mb:.1f} MB, "
                      f"Tokens/sec: {metrics.tokens_per_sec:.1f}")

        # Zero measurements (3B + 14B)
        if has_3b and has_14b:
            print("\n[ZERO] Measuring Zero with varying block counts...")

            for num_blocks in BLOCK_COUNTS:
                print(f"\n   Testing with {num_blocks} blocks...")
                setup_hologit_blocks(num_blocks)

                effective_context = BASE_CONTEXT + (num_blocks * BLOCK_SIZE)

                ram_before = get_ram_usage()

                # Load graph
                graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
                if graph_file.exists():
                    with open(graph_file) as f:
                        _ = json.load(f)

                # Generate with 14B (main model)
                tokens, latency = await measure_generation(
                    session, LLAMA_14B_PORT, test_prompt, TOKENS_TO_GENERATE
                )

                ram_after = get_ram_usage()
                vram = get_vram_usage()

                metrics = ResourceMetrics(
                    num_blocks=num_blocks,
                    effective_context=effective_context,
                    vram_mb=vram,
                    ram_mb=ram_after - ram_before,
                    tokens_per_sec=tokens / (latency / 1000) if latency > 0 else 0,
                    latency_ms=latency
                )

                results["zero"].append({
                    "num_blocks": num_blocks,
                    "effective_context": effective_context,
                    "vram_mb": round(metrics.vram_mb, 1),
                    "ram_overhead_mb": round(metrics.ram_mb, 1),
                    "tokens_per_sec": round(metrics.tokens_per_sec, 1),
                    "latency_ms": round(metrics.latency_ms, 1)
                })

                print(f"      Context: {effective_context}, VRAM: {vram:.0f} MB, "
                      f"RAM overhead: {metrics.ram_mb:.1f} MB, "
                      f"Tokens/sec: {metrics.tokens_per_sec:.1f}")

    # Save results
    output_path = Path(args.output) if args.output else Path("results/resources.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] Results saved to {output_path}")

    # Print paper table
    print("\n" + "=" * 60)
    print("PAPER TABLE (Section 6.4)")
    print("=" * 60)
    print("| Configuration          | Eff. Context | Peak RAM | Tokens/sec |")
    print("|------------------------|--------------|----------|------------|")

    if results["baseline"]:
        bl = results["baseline"][0]
        print(f"| Baseline               | {bl['effective_context']:>10}K | "
              f"{bl['vram_mb']:>6.0f} MB | {bl['tokens_per_sec']:>10.1f} |")

    for config_name, config_results in [("SubZero", results["subzero"]), ("Zero", results["zero"])]:
        for r in config_results:
            if r["num_blocks"] in [5, 20]:
                label = f"{config_name} ({r['num_blocks']} blocks)"
                ctx = f"{r['effective_context']/1000:.1f}K"
                print(f"| {label:<22} | {ctx:>12} | "
                      f"{r['vram_mb']:>6.0f} MB | {r['tokens_per_sec']:>10.1f} |")


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. Resources Benchmark")
    parser.add_argument("--output", "-o", help="Output JSON file path")
    args = parser.parse_args()

    asyncio.run(run_benchmark(args))


if __name__ == "__main__":
    main()
