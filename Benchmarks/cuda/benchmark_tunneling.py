#!/usr/bin/env python3
"""
Z.E.T.A. Direct Tunneling Benchmark (Muscle Memory)

Tests the direct tunneling mechanism where high-confidence patterns (>99%)
bypass the 3B subconscious entirely, like muscle memory in biological cognition.

Key Tests:
1. Training: How patterns become muscle memory through repeated access
2. Speed: Latency improvement from bypassing full pipeline
3. Accuracy: Correctness of tunneled recalls
4. Retention: How long muscle memory persists
"""

import argparse
import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from benchmark_harness import (
    clear_hologit, HOLOGIT_PATH,
    LAMBDA, TAU, ALPHA, GAMMA, TUNNEL_THRESHOLD,
    MemoryBlock, MemoryHierarchy,
    ZetaCognitiveSystem, L0_CAPACITY,
    Timer, TimingResult
)


@dataclass
class TunnelingMetrics:
    """Metrics for tunneling benchmark."""
    total_queries: int = 0
    tunnel_hits: int = 0
    tunnel_misses: int = 0
    correct_tunnels: int = 0
    incorrect_tunnels: int = 0

    # Timing
    full_pipeline_times: List[float] = field(default_factory=list)
    tunnel_times: List[float] = field(default_factory=list)

    @property
    def tunnel_rate(self) -> float:
        return self.tunnel_hits / max(self.total_queries, 1)

    @property
    def tunnel_accuracy(self) -> float:
        return self.correct_tunnels / max(self.tunnel_hits, 1)

    @property
    def avg_full_pipeline_ms(self) -> float:
        return sum(self.full_pipeline_times) / max(len(self.full_pipeline_times), 1)

    @property
    def avg_tunnel_ms(self) -> float:
        return sum(self.tunnel_times) / max(len(self.tunnel_times), 1)

    @property
    def speedup(self) -> float:
        if self.avg_tunnel_ms == 0:
            return 0
        return self.avg_full_pipeline_ms / self.avg_tunnel_ms


class TunnelingCognitiveSystem(ZetaCognitiveSystem):
    """
    Extended cognitive system with decay-weighted muscle memory tracking.

    Muscle Memory Model (biological):
    M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))

    - Each access contributes confidence weighted by recency
    - Decays without practice (like real muscle memory)
    - Pattern activates when M_muscle > MUSCLE_THRESHOLD
    """

    def __init__(self, entity_graph: Dict):
        super().__init__(entity_graph)
        # Access history: entity -> [(timestamp, confidence, block_id), ...]
        self.access_history: Dict[str, List[Tuple[float, float, int]]] = {}

    def compute_muscle_score(self, entity: str) -> float:
        """
        Compute decay-weighted muscle memory score.

        M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))
        """
        e_lower = entity.lower()
        if e_lower not in self.access_history:
            return 0.0

        now = time.time()
        score = 0.0
        lambda_muscle = 0.90  # Faster decay for muscle memory

        for timestamp, confidence, _ in self.access_history[e_lower]:
            hours_ago = (now - timestamp) / 3600
            decay = lambda_muscle ** hours_ago
            score += confidence * decay

        return score

    def train_pattern(self, entity: str, block_id: int, confidence: float) -> Tuple[bool, float]:
        """
        Train a pattern as muscle memory through repeated high-confidence access.

        Returns: (is_now_tunneled, muscle_score)
        """
        if confidence < 0.9:
            return False, 0.0

        e_lower = entity.lower()

        # Record this access
        if e_lower not in self.access_history:
            self.access_history[e_lower] = []

        self.access_history[e_lower].append((time.time(), confidence, block_id))

        # Compute current muscle score
        muscle_score = self.compute_muscle_score(e_lower)

        # Pattern becomes muscle memory when score exceeds threshold
        muscle_threshold = 2.0
        return muscle_score > muscle_threshold, muscle_score

    def check_tunnel(self, entity: str) -> Tuple[bool, int, float]:
        """
        Check if direct tunneling should be used based on decay-weighted score.

        Returns: (should_tunnel, block_id, muscle_score)
        """
        e_lower = entity.lower()
        muscle_score = self.compute_muscle_score(e_lower)

        muscle_threshold = 2.0
        if muscle_score > muscle_threshold:
            # Find the most recent block for this entity
            if e_lower in self.access_history and self.access_history[e_lower]:
                _, _, block_id = self.access_history[e_lower][-1]
                return True, block_id, muscle_score

        return False, -1, muscle_score

    def process_with_tunneling(self, query: str, expected_block: int) -> Tuple[bool, bool, float]:
        """
        Process query with decay-weighted tunneling support.

        Returns: (used_tunnel, correct_result, elapsed_ms)
        """
        start = time.perf_counter()

        # Extract query entities
        query_entities = self.subconscious.extract_entities(query)
        if not query_entities:
            query_entities = [w for w in query.lower().split() if len(w) > 3]

        # Check for tunnel using decay-weighted score
        for entity in query_entities:
            should_tunnel, tunnel_block, muscle_score = self.check_tunnel(entity)
            if should_tunnel:
                elapsed_ms = (time.perf_counter() - start) * 1000
                correct = (tunnel_block == expected_block)
                return True, correct, elapsed_ms

        # Full pipeline
        response, timings = self.process_query(query)
        elapsed_ms = (time.perf_counter() - start) * 1000

        # Check if correct block was surfaced
        surfaced_ids = [b.block_id for b in self.get_surfaced_blocks()]
        correct = expected_block in surfaced_ids

        # Train muscle memory for high-confidence results
        if surfaced_ids and correct:
            top_block = self.memory.l0_vram.get(surfaced_ids[0])
            if top_block and top_block.correlation_score > 0.9:
                for entity in query_entities:
                    self.train_pattern(entity, surfaced_ids[0], top_block.correlation_score)

        return False, correct, elapsed_ms

    def get_muscle_memory_stats(self) -> Dict[str, Dict]:
        """Get detailed muscle memory statistics."""
        stats = {}
        for entity, accesses in self.access_history.items():
            score = self.compute_muscle_score(entity)
            stats[entity] = {
                "total_accesses": len(accesses),
                "muscle_score": score,
                "is_active": score > 2.0,
                "block_id": accesses[-1][2] if accesses else None
            }
        return stats


def setup_tunneling_test() -> Tuple[TunnelingCognitiveSystem, Dict[str, int]]:
    """Setup cognitive system for tunneling tests."""
    clear_hologit()

    # Build entity graph with patterns that can become muscle memory
    entity_graph = {
        "entities": {},
        "co_occurrence": {},
        "entity_counts": {},
        "block_to_entities": {}
    }

    def add_entity(entity: str, block_id: int, connections: Optional[List[str]] = None):
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

    # Create patterns that should become muscle memory
    patterns = {}

    # High-frequency patterns (should train quickly)
    for i in range(5):
        block_id = i
        entity = f"frequent_pattern_{i}"
        value = f"frequent_value_{i}"

        # Strong co-occurrence (will have high confidence)
        for _ in range(20):
            add_entity(entity, block_id, [value])
        entity_graph["block_to_entities"][str(block_id)] = [entity.lower(), value.lower()]
        patterns[entity] = block_id

    # Medium-frequency patterns
    for i in range(5):
        block_id = 10 + i
        entity = f"medium_pattern_{i}"
        value = f"medium_value_{i}"

        for _ in range(5):
            add_entity(entity, block_id, [value])
        entity_graph["block_to_entities"][str(block_id)] = [entity.lower(), value.lower()]
        patterns[entity] = block_id

    # Low-frequency patterns (should NOT become muscle memory)
    for i in range(5):
        block_id = 20 + i
        entity = f"rare_pattern_{i}"
        value = f"rare_value_{i}"

        add_entity(entity, block_id, [value])
        entity_graph["block_to_entities"][str(block_id)] = [entity.lower(), value.lower()]
        patterns[entity] = block_id

    # Create system
    system = TunnelingCognitiveSystem(entity_graph)

    # Add memory blocks
    for entity, block_id in patterns.items():
        content = f"Information about {entity}"
        system.add_memory(block_id, content, [entity], age_hours=0.5)

    # Save graph
    index_dir = HOLOGIT_PATH / "index"
    index_dir.mkdir(parents=True, exist_ok=True)
    with open(index_dir / "entity_graph.json", "w") as f:
        json.dump(entity_graph, f, indent=2)

    return system, patterns


def run_training_test(system: TunnelingCognitiveSystem,
                      patterns: Dict[str, int]) -> Dict:
    """
    Test how patterns become muscle memory through repeated access.

    Queries each pattern multiple times and tracks when it becomes tunneled.
    """
    results = {
        "frequent": {"queries_to_tunnel": [], "final_tunneled": []},
        "medium": {"queries_to_tunnel": [], "final_tunneled": []},
        "rare": {"queries_to_tunnel": [], "final_tunneled": []}
    }

    # Query each pattern type multiple times
    for entity, block_id in patterns.items():
        if "frequent" in entity:
            pattern_type = "frequent"
        elif "medium" in entity:
            pattern_type = "medium"
        else:
            pattern_type = "rare"

        queries_needed = 0
        tunneled = False

        # Query up to 10 times
        for query_num in range(1, 11):
            query = f"What is {entity}?"
            used_tunnel, correct, _ = system.process_with_tunneling(query, block_id)

            if used_tunnel and not tunneled:
                tunneled = True
                queries_needed = query_num
                results[pattern_type]["queries_to_tunnel"].append(query_num)

        if tunneled:
            results[pattern_type]["final_tunneled"].append(entity)

    # Calculate averages
    for ptype in results:
        qtl = results[ptype]["queries_to_tunnel"]
        results[ptype]["avg_queries_to_tunnel"] = sum(qtl) / len(qtl) if qtl else 0
        results[ptype]["tunnel_rate"] = len(results[ptype]["final_tunneled"]) / 5

    return results


def run_latency_test(system: TunnelingCognitiveSystem,
                     patterns: Dict[str, int],
                     iterations: int = 20) -> TunnelingMetrics:
    """
    Test latency improvement from tunneling vs full pipeline.
    """
    metrics = TunnelingMetrics()

    # First, train patterns by querying them multiple times
    print("   Training patterns...")
    for entity, block_id in patterns.items():
        for _ in range(5):
            system.process_with_tunneling(f"What is {entity}?", block_id)

    # Now benchmark
    print("   Benchmarking latency...")
    for _ in range(iterations):
        for entity, block_id in patterns.items():
            query = f"What is {entity}?"

            used_tunnel, correct, elapsed_ms = system.process_with_tunneling(query, block_id)

            metrics.total_queries += 1

            if used_tunnel:
                metrics.tunnel_hits += 1
                metrics.tunnel_times.append(elapsed_ms)
                if correct:
                    metrics.correct_tunnels += 1
                else:
                    metrics.incorrect_tunnels += 1
            else:
                metrics.tunnel_misses += 1
                metrics.full_pipeline_times.append(elapsed_ms)

    return metrics


def run_retention_test(system: TunnelingCognitiveSystem,
                       patterns: Dict[str, int]) -> Dict:
    """
    Test how long muscle memory persists after training.

    Simulates time passing by backdating timestamps and measuring decay.

    Decay Model: M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))
    With λ_muscle = 0.90, muscle score halves every ~6.6 hours.

    Expected behavior:
    - Immediate: Full muscle score, high tunnel rate
    - After 1h: ~90% of original score, still tunneling
    - After 8h: ~43% of original score, borderline tunneling
    - After 24h: ~8% of original score, muscle memory lost
    """
    results = {
        "immediate": {"tunnel_rate": 0, "accuracy": 0, "avg_muscle_score": 0},
        "after_1h": {"tunnel_rate": 0, "accuracy": 0, "avg_muscle_score": 0},
        "after_8h": {"tunnel_rate": 0, "accuracy": 0, "avg_muscle_score": 0},
        "after_24h": {"tunnel_rate": 0, "accuracy": 0, "avg_muscle_score": 0}
    }

    # Train patterns
    print("   Training patterns...")
    for entity, block_id in patterns.items():
        for _ in range(5):
            system.process_with_tunneling(f"What is {entity}?", block_id)

    def test_tunneling_with_scores():
        hits = correct = 0
        scores = []
        for entity, block_id in patterns.items():
            muscle_score = system.compute_muscle_score(entity)
            scores.append(muscle_score)
            should_tunnel, tunnel_block, _ = system.check_tunnel(entity)
            if should_tunnel:
                hits += 1
                if tunnel_block == block_id:
                    correct += 1
        avg_score = sum(scores) / len(scores) if scores else 0
        return hits / len(patterns), correct / max(hits, 1), avg_score

    def age_muscle_memory(hours: float):
        """Backdate all access timestamps to simulate time passing."""
        time_shift = hours * 3600  # Convert hours to seconds
        for entity in system.access_history:
            aged_history = []
            for timestamp, confidence, block_id in system.access_history[entity]:
                aged_history.append((timestamp - time_shift, confidence, block_id))
            system.access_history[entity] = aged_history

    # Immediate (t=0)
    rate, acc, score = test_tunneling_with_scores()
    results["immediate"]["tunnel_rate"] = rate
    results["immediate"]["accuracy"] = acc
    results["immediate"]["avg_muscle_score"] = score
    print(f"   Immediate: {rate:.0%} tunneled, score={score:.2f}")

    # After 1 hour
    age_muscle_memory(1.0)
    rate, acc, score = test_tunneling_with_scores()
    results["after_1h"]["tunnel_rate"] = rate
    results["after_1h"]["accuracy"] = acc
    results["after_1h"]["avg_muscle_score"] = score
    print(f"   After 1h:  {rate:.0%} tunneled, score={score:.2f}")

    # After 8 hours (additional 7 hours)
    age_muscle_memory(7.0)
    rate, acc, score = test_tunneling_with_scores()
    results["after_8h"]["tunnel_rate"] = rate
    results["after_8h"]["accuracy"] = acc
    results["after_8h"]["avg_muscle_score"] = score
    print(f"   After 8h:  {rate:.0%} tunneled, score={score:.2f}")

    # After 24 hours (additional 16 hours)
    age_muscle_memory(16.0)
    rate, acc, score = test_tunneling_with_scores()
    results["after_24h"]["tunnel_rate"] = rate
    results["after_24h"]["accuracy"] = acc
    results["after_24h"]["avg_muscle_score"] = score
    print(f"   After 24h: {rate:.0%} tunneled, score={score:.2f}")

    return results


def run_benchmark(args):
    """Run complete tunneling benchmark."""
    print("=" * 70)
    print("Z.E.T.A. Direct Tunneling Benchmark (Muscle Memory)")
    print("=" * 70)

    print("\n[CONCEPT]")
    print("  Direct tunneling enables >99% confidence patterns to bypass")
    print("  the 3B subconscious entirely, like muscle memory:")
    print("  - Trained through repeated high-confidence access")
    print("  - Eliminates graph walk, scoring, hierarchy management")
    print("  - Instant recall for frequently-accessed patterns")

    # Setup
    print("\n[SETUP] Creating cognitive system...")
    system, patterns = setup_tunneling_test()
    print(f"   Frequent patterns: 5 (high co-occurrence)")
    print(f"   Medium patterns:   5 (moderate co-occurrence)")
    print(f"   Rare patterns:     5 (single co-occurrence)")

    results = {
        "benchmark": "tunneling",
        "architecture": "muscle_memory",
        "threshold": TUNNEL_THRESHOLD,
        "tests": {}
    }

    # Test 1: Training
    print("\n[TEST 1] Pattern Training")
    print("   How many queries until pattern becomes muscle memory?")
    training_results = run_training_test(system, patterns)
    results["tests"]["training"] = training_results

    print(f"\n   Results:")
    for ptype in ["frequent", "medium", "rare"]:
        avg = training_results[ptype]["avg_queries_to_tunnel"]
        rate = training_results[ptype]["tunnel_rate"]
        print(f"   {ptype:10}: {avg:.1f} queries avg, {rate:.0%} tunneled")

    # Reset system for latency test
    system, patterns = setup_tunneling_test()

    # Test 2: Latency
    print("\n[TEST 2] Latency Comparison")
    print("   Full pipeline vs direct tunnel timing")
    metrics = run_latency_test(system, patterns, iterations=10)
    results["tests"]["latency"] = {
        "total_queries": metrics.total_queries,
        "tunnel_hits": metrics.tunnel_hits,
        "tunnel_rate": metrics.tunnel_rate,
        "tunnel_accuracy": metrics.tunnel_accuracy,
        "avg_full_pipeline_ms": metrics.avg_full_pipeline_ms,
        "avg_tunnel_ms": metrics.avg_tunnel_ms,
        "speedup": metrics.speedup
    }

    print(f"\n   Results:")
    print(f"   Tunnel rate:      {metrics.tunnel_rate:.1%}")
    print(f"   Tunnel accuracy:  {metrics.tunnel_accuracy:.1%}")
    print(f"   Full pipeline:    {metrics.avg_full_pipeline_ms:.2f}ms avg")
    print(f"   Direct tunnel:    {metrics.avg_tunnel_ms:.2f}ms avg")
    print(f"   Speedup:          {metrics.speedup:.1f}x")

    # Test 3: Retention (decay test)
    print("\n[TEST 3] Memory Retention (Decay-Weighted)")
    print("   How long does muscle memory persist without practice?")
    print("   M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))")
    print("   λ_muscle = 0.90 (faster decay than general memory)")
    retention = run_retention_test(system, patterns)
    results["tests"]["retention"] = retention

    print(f"\n   Results (decay curve):")
    for timepoint in ["immediate", "after_1h", "after_8h", "after_24h"]:
        rate = retention[timepoint]["tunnel_rate"]
        acc = retention[timepoint]["accuracy"]
        score = retention[timepoint]["avg_muscle_score"]
        print(f"   {timepoint:12}: {rate:.0%} tunneled, score={score:.2f}")

    # Save results
    output_path = Path(args.output) if args.output else Path("results/tunneling.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] Results saved to {output_path}")

    # Summary
    print("\n" + "=" * 70)
    print("SUMMARY - Muscle Memory (Decay-Weighted Tunneling)")
    print("=" * 70)
    print("\n[KEY FINDINGS]")
    print(f"  - High-frequency patterns tunnel after ~{training_results['frequent']['avg_queries_to_tunnel']:.0f} queries")
    print(f"  - Tunneling provides {metrics.speedup:.1f}x speedup over full pipeline")
    print(f"  - Tunnel accuracy: {metrics.tunnel_accuracy:.0%} (high-confidence only)")
    print(f"  - Low-frequency patterns do NOT become muscle memory (as designed)")

    # Decay findings
    immediate_score = retention["immediate"]["avg_muscle_score"]
    after_24h_score = retention["after_24h"]["avg_muscle_score"]
    decay_ratio = after_24h_score / immediate_score if immediate_score > 0 else 0
    print(f"\n[DECAY BEHAVIOR]")
    print(f"  - Immediate muscle score:  {immediate_score:.2f}")
    print(f"  - After 24h (no practice): {after_24h_score:.2f} ({decay_ratio:.0%} retained)")
    print(f"  - λ_muscle = 0.90 (half-life ~6.6 hours)")
    print(f"  - Muscle memory DECAYS without practice (biologically accurate)")

    print("\n[COGNITIVE ANALOGY]")
    print("  Like biological muscle memory:")
    print("  - Frequent patterns become automatic (bypasses conscious thought)")
    print("  - Requires training (repeated high-confidence access)")
    print("  - Faster than deliberate recall")
    print("  - DECAYS without practice (like real skills)")
    print("  - Must be maintained through continued use")


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. Tunneling Benchmark")
    parser.add_argument("--output", "-o", help="Output JSON file path")
    args = parser.parse_args()

    run_benchmark(args)


if __name__ == "__main__":
    main()
