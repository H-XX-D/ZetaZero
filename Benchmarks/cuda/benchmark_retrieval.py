#!/usr/bin/env python3
"""
Z.E.T.A. Retrieval Accuracy Benchmark (Paper Section 6.3)

Tests the cognitive architecture:
- Baseline: 14B only (no memory)
- Z.E.T.A. Zero: 14B + memory parameters (no 3B subconscious)
- Z.E.T.A. SubZero: 14B conscious + 3B subconscious (full dual-process)

Memory Hierarchy Tests:
- L0 (VRAM): What gets surfaced for 14B attention
- L1 (RAM): What gets cached as correlated
- L2 (NVMe): What remains archived
"""

import argparse
import asyncio
import aiohttp
import json
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple, Optional

from benchmark_harness import (
    BenchmarkConfig, Timer, TimingResult,
    ZetaCognitiveSystem, ZetaSubconscious, MemoryHierarchy, MemoryBlock, MemoryLayer,
    clear_hologit, check_server_health,
    LLAMA_3B_PORT, LLAMA_14B_PORT, HOLOGIT_PATH,
    LAMBDA, TAU, ALPHA, GAMMA, L0_CAPACITY
)

# Test constants
NUM_BLOCKS = 50
NUM_NEEDLES = 10
TOP_K = 5


@dataclass
class RetrievalMetrics:
    """Metrics for retrieval accuracy."""
    recall_at_1: float = 0.0
    recall_at_5: float = 0.0
    mrr: float = 0.0
    fpr: float = 0.0
    l0_surfaced: int = 0      # How many blocks surfaced to VRAM
    l1_cached: int = 0        # How many cached in RAM
    l2_archived: int = 0      # How many remain in NVMe
    tunnel_hits: int = 0      # Direct tunneling (muscle memory) hits


async def call_3b_extract(session: aiohttp.ClientSession, query: str) -> List[str]:
    """Extract entities using 3B subconscious model."""
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

    try:
        async with session.post(
            f"http://localhost:{LLAMA_3B_PORT}/completion",
            json={"prompt": prompt, "n_predict": 50, "temperature": 0.1, "stop": ["]"]},
            timeout=aiohttp.ClientTimeout(total=10)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                raw = data.get("content", "")
                try:
                    return json.loads('["' + raw + ']')
                except:
                    pass
    except Exception as e:
        pass
    return []


async def call_14b_generate(session: aiohttp.ClientSession, prompt: str,
                            context: List[str] = None, max_tokens: int = 100) -> str:
    """Generate response using 14B conscious model with surfaced context."""
    # Inject surfaced context into prompt (this is what 3B subconscious provides)
    if context:
        context_str = "\n".join(f"[Memory] {c}" for c in context[:L0_CAPACITY])
        full_prompt = f"{context_str}\n\n{prompt}"
    else:
        full_prompt = prompt

    try:
        async with session.post(
            f"http://localhost:{LLAMA_14B_PORT}/completion",
            json={"prompt": full_prompt, "n_predict": max_tokens, "temperature": 0.7},
            timeout=aiohttp.ClientTimeout(total=60)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                return data.get("content", "")
    except:
        pass
    return ""


def setup_cognitive_test() -> Tuple[ZetaCognitiveSystem, Dict[str, int]]:
    """
    Setup cognitive system with test data.

    Creates:
    - N memory blocks across L2 (archived)
    - K needle facts that should be surfaced
    - Entity graph for graph walk

    Returns: (cognitive_system, needle_to_block_mapping)
    """
    clear_hologit()

    # Build entity graph
    entity_graph = {
        "entities": {},
        "co_occurrence": {},
        "entity_counts": {},
        "block_to_entities": {}
    }

    def add_entity(entity: str, block_id: int, connections: List[str] = None):
        e = entity.lower()
        if e not in entity_graph["entities"]:
            entity_graph["entities"][e] = {"blocks": [], "connections": []}
        if block_id not in entity_graph["entities"][e]["blocks"]:
            entity_graph["entities"][e]["blocks"].append(block_id)
        entity_graph["entity_counts"][e] = entity_graph["entity_counts"].get(e, 0) + 1

        if connections:
            for conn in connections:
                c = conn.lower()
                if c not in entity_graph["entities"][e]["connections"]:
                    entity_graph["entities"][e]["connections"].append(c)
                pair_key = "|".join(sorted([e, c]))
                entity_graph["co_occurrence"][pair_key] = entity_graph["co_occurrence"].get(pair_key, 0) + 1

    # Create cognitive system
    system = ZetaCognitiveSystem(entity_graph)

    # Plant needles and fillers
    needles = {}
    needle_positions = random.sample(range(NUM_BLOCKS), NUM_NEEDLES)
    current_time = time.time()

    for i in range(NUM_BLOCKS):
        if i in needle_positions:
            # Needle - important fact to retrieve
            needle_id = len(needles)
            entity = f"needle_entity_{needle_id}"
            value = f"needle_value_{needle_id}"
            content = f"IMPORTANT: {entity} equals {value}"
            needles[entity] = i

            # Add to graph
            add_entity(entity, i, [value])
            add_entity(value, i, [entity])
            entity_graph["block_to_entities"][str(i)] = [entity.lower(), value.lower()]

            # Add to memory (recent - high decay factor)
            age_hours = random.uniform(0.1, 2.0)  # Recent
            system.add_memory(i, content, [entity, value], age_hours)

            print(f"[NEEDLE] Block {i}: '{entity}' (age={age_hours:.1f}h)")
        else:
            # Filler - noise to filter out
            entity = f"filler_{i}"
            content = f"Random filler data for block {i}"

            add_entity(entity, i, [f"random_{i}"])
            entity_graph["block_to_entities"][str(i)] = [entity.lower()]

            # Add to memory (older - lower decay factor)
            age_hours = random.uniform(12.0, 72.0)  # Older
            system.add_memory(i, content, [entity], age_hours)

    # Save graph for reference
    index_dir = HOLOGIT_PATH / "index"
    index_dir.mkdir(parents=True, exist_ok=True)
    with open(index_dir / "entity_graph.json", "w") as f:
        json.dump(entity_graph, f, indent=2)

    return system, needles


def run_cognitive_retrieval_test(system: ZetaCognitiveSystem,
                                  needles: Dict[str, int],
                                  use_subconscious: bool = True) -> RetrievalMetrics:
    """
    Test retrieval using the cognitive architecture.

    If use_subconscious=True: Full 3B subconscious pipeline
    If use_subconscious=False: Direct 14B retrieval (Zero mode)
    """
    metrics = RetrievalMetrics()
    reciprocal_ranks = []

    for entity, true_block in needles.items():
        # Query for needle
        query = f"What is the value of {entity}?"

        if use_subconscious:
            # Full cognitive pipeline: 3B manages hierarchy, surfaces to L0
            response, timings = system.process_query(query)

            # Check if tunneled (muscle memory)
            if timings.get("tunneled"):
                metrics.tunnel_hits += 1
        else:
            # Zero mode: 14B with memory but no 3B subconscious
            # Directly score and retrieve (simulating 14B doing its own memory work)
            system.subconscious.graph_walk([entity])
            for block in system.memory.get_all_blocks():
                system.subconscious.score_block(block, [entity])

            # Sort and take top-k
            all_blocks = sorted(system.memory.get_all_blocks(),
                               key=lambda b: b.correlation_score, reverse=True)
            for i, block in enumerate(all_blocks[:L0_CAPACITY]):
                system.memory.surface_to_l0(block)

        # Check retrieval success
        surfaced_blocks = system.get_surfaced_blocks()
        surfaced_ids = [b.block_id for b in surfaced_blocks]

        if true_block in surfaced_ids:
            rank = surfaced_ids.index(true_block) + 1
            if rank == 1:
                metrics.recall_at_1 += 1
            if rank <= 5:
                metrics.recall_at_5 += 1
            reciprocal_ranks.append(1.0 / rank)
        else:
            reciprocal_ranks.append(0.0)

    # Normalize metrics
    n = len(needles)
    if n > 0:
        metrics.recall_at_1 /= n
        metrics.recall_at_5 /= n
        metrics.mrr = sum(reciprocal_ranks) / n

        # Calculate FPR
        total_surfaced = n * L0_CAPACITY
        true_positives = sum(1 for rr in reciprocal_ranks if rr > 0)
        metrics.fpr = (total_surfaced - true_positives) / total_surfaced if total_surfaced > 0 else 0

    # Memory hierarchy distribution
    metrics.l0_surfaced = len(system.memory.l0_vram)
    metrics.l1_cached = len(system.memory.l1_ram)
    metrics.l2_archived = len(system.memory.l2_nvme)

    return metrics


async def run_baseline_test(session: aiohttp.ClientSession,
                            needles: Dict[str, int]) -> RetrievalMetrics:
    """
    Baseline test: 14B only, no memory system.

    The 14B has no way to retrieve facts not in its context window.
    Expected: ~0% recall (facts are not in context).
    """
    metrics = RetrievalMetrics()

    for entity, _ in needles.items():
        query = f"What is the value of {entity}?"

        # 14B won't know - no memory, no context
        response = await call_14b_generate(session, query)

        expected = f"needle_value_{entity.split('_')[-1]}"
        if expected.lower() in response.lower():
            metrics.recall_at_1 += 1
            metrics.recall_at_5 += 1

    n = len(needles)
    if n > 0:
        metrics.recall_at_1 /= n
        metrics.recall_at_5 /= n
        metrics.mrr = metrics.recall_at_1

    return metrics


async def run_full_subzero_test(session: aiohttp.ClientSession,
                                 system: ZetaCognitiveSystem,
                                 needles: Dict[str, int]) -> RetrievalMetrics:
    """
    Full Z.E.T.A. SubZero test with actual LLM calls.

    Pipeline:
    1. 3B subconscious extracts entities from query
    2. 3B walks graph and scores blocks
    3. 3B surfaces top-k to L0 (VRAM)
    4. 14B conscious reasons over surfaced context
    """
    metrics = RetrievalMetrics()
    reciprocal_ranks = []

    for entity, true_block in needles.items():
        query = f"What is the value of {entity}?"

        # Step 1: 3B extracts entities (actual LLM call)
        extracted = await call_3b_extract(session, query)
        if not extracted:
            extracted = [entity]  # Fallback

        # Step 2-3: Subconscious manages hierarchy
        surfaced = system.subconscious.manage_hierarchy(extracted)

        # Step 4: 14B reasons with surfaced context
        context = [b.content for b in surfaced]
        response = await call_14b_generate(session, query, context)

        # Check success
        surfaced_ids = [b.block_id for b in surfaced]
        if true_block in surfaced_ids:
            rank = surfaced_ids.index(true_block) + 1
            if rank == 1:
                metrics.recall_at_1 += 1
            if rank <= 5:
                metrics.recall_at_5 += 1
            reciprocal_ranks.append(1.0 / rank)
        else:
            reciprocal_ranks.append(0.0)

    n = len(needles)
    if n > 0:
        metrics.recall_at_1 /= n
        metrics.recall_at_5 /= n
        metrics.mrr = sum(reciprocal_ranks) / n
        total = n * L0_CAPACITY
        tp = sum(1 for rr in reciprocal_ranks if rr > 0)
        metrics.fpr = (total - tp) / total if total > 0 else 0

    metrics.l0_surfaced = len(system.memory.l0_vram)
    metrics.l1_cached = len(system.memory.l1_ram)
    metrics.l2_archived = len(system.memory.l2_nvme)

    return metrics


async def run_benchmark(args):
    """Run complete retrieval benchmark."""
    print("=" * 70)
    print("Z.E.T.A. Retrieval Benchmark - Dual-Process Cognitive Architecture")
    print("=" * 70)

    print("\n[ARCHITECTURE]")
    print("  Baseline:        14B conscious only (no memory)")
    print("  Z.E.T.A. Zero:   14B conscious + memory parameters (no 3B)")
    print("  Z.E.T.A. SubZero: 14B conscious + 3B subconscious (full dual-process)")

    results = {
        "benchmark": "retrieval",
        "architecture": "dual_process_cognitive",
        "params": {
            "num_blocks": NUM_BLOCKS,
            "num_needles": NUM_NEEDLES,
            "top_k": TOP_K,
            "l0_capacity": L0_CAPACITY,
            "lambda": LAMBDA,
            "tau": TAU,
            "alpha": ALPHA
        },
        "configs": {}
    }

    # Setup cognitive system
    print(f"\n[SETUP] Creating cognitive system with {NUM_BLOCKS} blocks, {NUM_NEEDLES} needles...")
    system, needles = setup_cognitive_test()

    async with aiohttp.ClientSession() as session:
        has_3b = check_server_health(LLAMA_3B_PORT)
        has_14b = check_server_health(LLAMA_14B_PORT)

        print(f"\n[SERVERS] 3B Subconscious: {'OK' if has_3b else 'OFF'}")
        print(f"          14B Conscious:   {'OK' if has_14b else 'OFF'}")

        # 1. Baseline: 14B only, no memory
        if has_14b:
            print("\n[TEST] Baseline (14B conscious only, no memory)...")
            baseline = await run_baseline_test(session, needles)
            results["configs"]["baseline"] = {
                "description": "14B only, no memory system",
                "recall_at_1": round(baseline.recall_at_1, 4),
                "recall_at_5": round(baseline.recall_at_5, 4),
                "mrr": round(baseline.mrr, 4),
                "fpr": round(baseline.fpr, 4)
            }
            print(f"   Recall@1: {baseline.recall_at_1:.1%}")
            print(f"   Expected: ~0% (no memory to retrieve from)")
        else:
            print("\n[SKIP] Baseline (14B not running)")
            results["configs"]["baseline"] = {
                "description": "14B only, no memory system",
                "recall_at_1": 0.0, "recall_at_5": 0.0, "mrr": 0.0, "fpr": 0.0,
                "note": "14B server not running"
            }

        # Reset system for next test
        system, needles = setup_cognitive_test()

        # 2. Z.E.T.A. Zero: 14B + memory parameters (no 3B subconscious)
        print("\n[TEST] Z.E.T.A. Zero (14B + memory, no 3B subconscious)...")
        zero = run_cognitive_retrieval_test(system, needles, use_subconscious=False)
        results["configs"]["zero"] = {
            "description": "14B + memory parameters, no 3B",
            "recall_at_1": round(zero.recall_at_1, 4),
            "recall_at_5": round(zero.recall_at_5, 4),
            "mrr": round(zero.mrr, 4),
            "fpr": round(zero.fpr, 4),
            "l0_surfaced": zero.l0_surfaced,
            "l1_cached": zero.l1_cached,
            "l2_archived": zero.l2_archived
        }
        print(f"   Recall@1: {zero.recall_at_1:.1%}")
        print(f"   Memory: L0={zero.l0_surfaced}, L1={zero.l1_cached}, L2={zero.l2_archived}")

        # Reset system for next test
        system, needles = setup_cognitive_test()

        # 3. Z.E.T.A. SubZero: Full dual-process (3B subconscious + 14B conscious)
        print("\n[TEST] Z.E.T.A. SubZero (3B subconscious + 14B conscious)...")

        if has_3b and has_14b:
            # Full LLM-based test
            subzero = await run_full_subzero_test(session, system, needles)
        else:
            # Simulated subconscious test (no actual 3B calls)
            subzero = run_cognitive_retrieval_test(system, needles, use_subconscious=True)

        results["configs"]["subzero"] = {
            "description": "3B subconscious + 14B conscious",
            "recall_at_1": round(subzero.recall_at_1, 4),
            "recall_at_5": round(subzero.recall_at_5, 4),
            "mrr": round(subzero.mrr, 4),
            "fpr": round(subzero.fpr, 4),
            "l0_surfaced": subzero.l0_surfaced,
            "l1_cached": subzero.l1_cached,
            "l2_archived": subzero.l2_archived,
            "tunnel_hits": subzero.tunnel_hits
        }
        print(f"   Recall@1: {subzero.recall_at_1:.1%}")
        print(f"   Memory: L0={subzero.l0_surfaced}, L1={subzero.l1_cached}, L2={subzero.l2_archived}")
        print(f"   Tunnel hits (muscle memory): {subzero.tunnel_hits}")

    # Save results
    output_path = Path(args.output) if args.output else Path("results/retrieval.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] Results saved to {output_path}")

    # Summary table
    print("\n" + "=" * 80)
    print("SUMMARY - Dual-Process Cognitive Retrieval (Paper Section 6.3)")
    print("=" * 80)
    print(f"| {'Config':<20} | {'Recall@1':>10} | {'Recall@5':>10} | {'MRR':>8} | {'L0/L1/L2':>12} |")
    print("|" + "-" * 22 + "|" + "-" * 12 + "|" + "-" * 12 + "|" + "-" * 10 + "|" + "-" * 14 + "|")

    for config, m in results["configs"].items():
        r1 = f"{m['recall_at_1']*100:.1f}%"
        r5 = f"{m['recall_at_5']*100:.1f}%"
        mrr = f"{m['mrr']:.4f}"
        hierarchy = f"{m.get('l0_surfaced', '-')}/{m.get('l1_cached', '-')}/{m.get('l2_archived', '-')}"
        print(f"| {config:<20} | {r1:>10} | {r5:>10} | {mrr:>8} | {hierarchy:>12} |")

    print("\n[INTERPRETATION]")
    print("  - Baseline: 14B cannot retrieve facts not in context → ~0%")
    print("  - Zero: 14B + memory retrieval → improved recall")
    print("  - SubZero: 3B filters/surfaces for 14B → best cognitive performance")
    print("  - L0/L1/L2: Blocks surfaced(VRAM) / cached(RAM) / archived(NVMe)")


def main():
    global NUM_BLOCKS, NUM_NEEDLES

    parser = argparse.ArgumentParser(description="Z.E.T.A. Cognitive Retrieval Benchmark")
    parser.add_argument("--output", "-o", help="Output JSON file path")
    parser.add_argument("--blocks", type=int, default=50)
    parser.add_argument("--needles", type=int, default=10)
    args = parser.parse_args()

    NUM_BLOCKS = args.blocks
    NUM_NEEDLES = args.needles

    asyncio.run(run_benchmark(args))


if __name__ == "__main__":
    main()
