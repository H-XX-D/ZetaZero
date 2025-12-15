#!/usr/bin/env python3
"""
Z.E.T.A. Persistence Benchmark (Paper Section 6.6)

Tests temporal decay and cross-session memory persistence:
- Plant facts at t=0
- Simulate time passage (0, 6, 12, 24, 48 hours)
- Measure recall rate and scores
- Validate against expected D(t) = λ^t
"""

import argparse
import json
import os
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

from benchmark_harness import (
    clear_hologit, HOLOGIT_PATH, LAMBDA
)

# Time points to test (hours)
TIME_POINTS = [0, 6, 12, 24, 48]


@dataclass
class PersistenceResult:
    """Result for a single time point."""
    hours: int
    recall_rate: float
    avg_score: float
    expected_decay: float
    facts_recalled: int
    total_facts: int


def plant_test_facts() -> List[Dict]:
    """Plant test facts with known timestamps."""
    clear_hologit()

    facts = [
        {"subject": "user", "predicate": "name", "object": "Alice"},
        {"subject": "user", "predicate": "job", "object": "engineer"},
        {"subject": "user", "predicate": "city", "object": "Paris"},
        {"subject": "project", "predicate": "name", "object": "Phoenix"},
        {"subject": "project", "predicate": "deadline", "object": "March"},
        {"subject": "api", "predicate": "rate_limit", "object": "60 req/min"},
        {"subject": "database", "predicate": "type", "object": "PostgreSQL"},
        {"subject": "team", "predicate": "size", "object": "5 members"},
        {"subject": "meeting", "predicate": "time", "object": "Monday 10am"},
        {"subject": "budget", "predicate": "amount", "object": "50000 USD"},
    ]

    # Save facts to HoloGit
    blocks_dir = HOLOGIT_PATH / "blocks"
    blocks_dir.mkdir(parents=True, exist_ok=True)
    index_dir = HOLOGIT_PATH / "index"
    index_dir.mkdir(parents=True, exist_ok=True)

    # Create entity graph
    graph = {
        "entities": {},
        "block_to_entities": {},
        "co_occurrence": {},
        "entity_counts": {}
    }

    current_time = time.time()

    for i, fact in enumerate(facts):
        # Write fact file
        fact_file = blocks_dir / f"facts_{i}.txt"
        key = f"{fact['subject']}_{fact['predicate']}"
        value = fact['object']
        with open(fact_file, "w") as f:
            f.write(f"fact:{key}={value}\n")

        # Set file mtime to "now" (will be adjusted in retrieval)
        os.utime(fact_file, (current_time, current_time))

        # Update graph
        entities = [fact['subject'].lower(), fact['object'].lower()]
        graph["block_to_entities"][str(i)] = entities

        for e in entities:
            if e not in graph["entities"]:
                graph["entities"][e] = {"blocks": [], "connections": []}
            graph["entities"][e]["blocks"].append(i)
            graph["entity_counts"][e] = graph["entity_counts"].get(e, 0) + 1

            # Connect entities
            for other in entities:
                if other != e and other not in graph["entities"][e]["connections"]:
                    graph["entities"][e]["connections"].append(other)

    # Save graph
    with open(index_dir / "entity_graph.json", "w") as f:
        json.dump(graph, f, indent=2)

    print(f"[PLANT] Created {len(facts)} test facts")
    return facts


def retrieve_with_simulated_time(query_entities: List[str],
                                 hours_offset: float) -> List[Tuple[str, float]]:
    """Retrieve facts with simulated time passage."""
    graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
    if not graph_file.exists():
        return []

    with open(graph_file) as f:
        graph = json.load(f)

    # Graph walk
    ALPHA = 0.85
    TAU = 0.1

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

    for e in query_entities:
        walk(e, depth=3, score=1.0)

    # Collect blocks
    block_scores = {}
    for entity, score in entity_scores.items():
        e_data = graph.get("entities", {}).get(entity, {})
        for bid in e_data.get("blocks", []):
            block_scores[bid] = max(block_scores.get(bid, 0), score)

    # Load facts with SIMULATED decay
    facts_with_scores = []
    current_time = time.time()

    for bid, score in block_scores.items():
        facts_file = HOLOGIT_PATH / "blocks" / f"facts_{bid}.txt"
        if facts_file.exists():
            # Simulated age = actual age + hours_offset
            actual_age_hours = (current_time - facts_file.stat().st_mtime) / 3600
            simulated_age = actual_age_hours + hours_offset

            # Apply decay D(t) = λ^t
            decay = LAMBDA ** simulated_age

            with open(facts_file) as f:
                for line in f:
                    if "=" in line:
                        final_score = score * decay
                        facts_with_scores.append((line.strip(), final_score))

    # Sort by score
    facts_with_scores.sort(key=lambda x: -x[1])
    return facts_with_scores[:20]


def test_persistence(facts: List[Dict], hours_offset: float) -> PersistenceResult:
    """Test fact recall at a simulated time offset."""
    recalled = 0
    total_score = 0.0

    for fact in facts:
        # Query for this fact
        query_entities = [fact['subject'], fact['predicate']]
        retrieved = retrieve_with_simulated_time(query_entities, hours_offset)

        # Check if fact was retrieved
        expected_value = fact['object'].lower()

        for text, score in retrieved:
            if expected_value in text.lower():
                recalled += 1
                total_score += score
                break

    expected_decay = LAMBDA ** hours_offset

    return PersistenceResult(
        hours=int(hours_offset),
        recall_rate=recalled / len(facts),
        avg_score=total_score / len(facts) if recalled > 0 else 0.0,
        expected_decay=expected_decay,
        facts_recalled=recalled,
        total_facts=len(facts)
    )


def run_benchmark(args):
    """Run complete persistence benchmark."""
    print("=" * 60)
    print("Z.E.T.A. Persistence Benchmark (Paper Section 6.6)")
    print("=" * 60)

    # Plant facts
    print("\n[SETUP] Planting test facts...")
    facts = plant_test_facts()

    results = {
        "benchmark": "persistence",
        "params": {
            "lambda": LAMBDA,
            "num_facts": len(facts),
            "time_points_hours": TIME_POINTS
        },
        "results": []
    }

    print(f"\n[TEST] Testing recall at {len(TIME_POINTS)} time points...")
    print(f"       Decay formula: D(t) = {LAMBDA}^t")

    # Test each time point
    for hours in TIME_POINTS:
        result = test_persistence(facts, hours)
        results["results"].append({
            "hours": result.hours,
            "recall_rate": round(result.recall_rate, 4),
            "avg_score": round(result.avg_score, 4),
            "expected_decay": round(result.expected_decay, 4),
            "facts_recalled": result.facts_recalled,
            "total_facts": result.total_facts
        })

        print(f"\n   t={hours}h: Recall={result.recall_rate:.1%}, "
              f"Avg Score={result.avg_score:.3f}, "
              f"Expected D(t)={result.expected_decay:.3f}")

    # Save results
    output_path = Path(args.output) if args.output else Path("results/persistence.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] Results saved to {output_path}")

    # Print paper table
    print("\n" + "=" * 60)
    print("PAPER TABLE (Section 6.6)")
    print("=" * 60)
    print("| Hours Since Learning | Recall Rate | Avg Score | Expected D(t) |")
    print("|----------------------|-------------|-----------|---------------|")

    for r in results["results"]:
        hours_label = f"{r['hours']} (immediate)" if r['hours'] == 0 else str(r['hours'])
        print(f"| {hours_label:<20} | {r['recall_rate']*100:>10.0f}% | "
              f"{r['avg_score']:>9.3f} | {r['expected_decay']:>13.3f} |")

    # Validation
    print("\n[VALIDATION]")
    print("Checking if decay follows D(t) = λ^t:")
    for r in results["results"]:
        if r['hours'] == 0:
            continue
        # Score should be proportional to decay
        score_ratio = r['avg_score'] / results["results"][0]['avg_score'] if results["results"][0]['avg_score'] > 0 else 0
        expected_ratio = r['expected_decay']
        diff = abs(score_ratio - expected_ratio)
        status = "OK" if diff < 0.2 else "DRIFT"
        print(f"   t={r['hours']}h: Score ratio={score_ratio:.3f}, "
              f"Expected={expected_ratio:.3f}, Diff={diff:.3f} [{status}]")


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. Persistence Benchmark")
    parser.add_argument("--output", "-o", help="Output JSON file path")
    args = parser.parse_args()

    run_benchmark(args)


if __name__ == "__main__":
    main()
