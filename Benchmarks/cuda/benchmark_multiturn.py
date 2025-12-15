#!/usr/bin/env python3
"""
Z.E.T.A. Multi-Turn Conversation Benchmark - FIXED

Tests:
1. Multi-turn context tracking (momentum) 
2. Harder cubic cosine test (competing similar-strength candidates)
3. Harder tunneling test (graph with noisy edges)
"""

import argparse
import json
import math
import os
import random
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Tuple

from benchmark_harness import (
    clear_hologit, HOLOGIT_PATH, LAMBDA, TAU, ALPHA, GAMMA
)

TOP_K = 5


@dataclass
class ConversationTurn:
    user_query: str
    entities_mentioned: List[str]
    target_block: int
    turn_number: int


@dataclass
class MultiTurnTest:
    conversation_id: str
    turns: List[ConversationTurn]
    description: str


@dataclass 
class AblationConfig:
    name: str
    use_cubic_cosine: bool = False
    use_tunneling: bool = False
    use_decay: bool = False
    use_graph_walk: bool = False
    use_momentum: bool = False


CONFIGS = [
    AblationConfig("baseline"),
    AblationConfig("plus_cubic", use_cubic_cosine=True),
    AblationConfig("plus_tunnel", use_cubic_cosine=True, use_tunneling=True),
    AblationConfig("plus_decay", use_cubic_cosine=True, use_tunneling=True, use_decay=True),
    AblationConfig("plus_graph", use_cubic_cosine=True, use_tunneling=True, use_decay=True, use_graph_walk=True),
    AblationConfig("full", use_cubic_cosine=True, use_tunneling=True, use_decay=True, use_graph_walk=True, use_momentum=True),
]


def setup_multiturn_data() -> Tuple[List[MultiTurnTest], Dict]:
    clear_hologit()
    
    blocks_dir = HOLOGIT_PATH / "blocks"
    blocks_dir.mkdir(parents=True, exist_ok=True)
    index_dir = HOLOGIT_PATH / "index"
    index_dir.mkdir(parents=True, exist_ok=True)
    
    graph = {
        "entities": {},
        "block_to_entities": {},
        "co_occurrence": {},
        "entity_counts": {},
        "block_entity_strength": {}
    }
    
    current_time = time.time()
    tests = []
    
    def add_entity(entity: str, block_id: int, connections: List[str] = None, strength: float = 1.0):
        e = entity.lower()
        if e not in graph["entities"]:
            graph["entities"][e] = {"blocks": [], "connections": []}
        if block_id not in graph["entities"][e]["blocks"]:
            graph["entities"][e]["blocks"].append(block_id)
        graph["entity_counts"][e] = graph["entity_counts"].get(e, 0) + 1
        
        bid_str = str(block_id)
        if bid_str not in graph["block_entity_strength"]:
            graph["block_entity_strength"][bid_str] = {}
        graph["block_entity_strength"][bid_str][e] = graph["block_entity_strength"][bid_str].get(e, 0) + strength
        
        if connections:
            for conn in connections:
                c = conn.lower()
                if c not in graph["entities"][e]["connections"]:
                    graph["entities"][e]["connections"].append(c)
                pair_key = "|".join(sorted([e, c]))
                graph["co_occurrence"][pair_key] = graph["co_occurrence"].get(pair_key, 0) + 1
    
    # === MULTI-TURN CONVERSATION TEST (FIXED) ===
    # Key fix: Use generic entity names that appear in BOTH project blocks
    # so the momentum from recent context makes the difference
    
    # Block 0: Project Alpha - has "deadline", "budget", "team" entities
    add_entity("project_alpha", 0, ["deadline", "budget", "team", "march", "alice"])
    add_entity("deadline", 0, ["project_alpha", "march"])  # Generic deadline
    add_entity("budget", 0, ["project_alpha", "100k"])
    add_entity("team", 0, ["project_alpha", "alice"])
    add_entity("march", 0, ["deadline", "project_alpha"])
    add_entity("alice", 0, ["team", "project_alpha"])
    with open(blocks_dir / "facts_0.txt", "w") as f:
        f.write("project:alpha deadline=march budget=100k team=alice\n")
    os.utime(blocks_dir / "facts_0.txt", (current_time - 3600, current_time - 3600))
    graph["block_to_entities"]["0"] = ["project_alpha", "deadline", "budget", "team"]
    
    # Block 1: Project Beta - ALSO has "deadline", "budget", "team" entities
    add_entity("project_beta", 1, ["deadline", "budget", "team", "june", "bob"])
    add_entity("deadline", 1, ["project_beta", "june"])  # Same generic deadline
    add_entity("budget", 1, ["project_beta", "200k"])
    add_entity("team", 1, ["project_beta", "bob"])
    add_entity("june", 1, ["deadline", "project_beta"])
    add_entity("bob", 1, ["team", "project_beta"])
    with open(blocks_dir / "facts_1.txt", "w") as f:
        f.write("project:beta deadline=june budget=200k team=bob\n")
    os.utime(blocks_dir / "facts_1.txt", (current_time - 3600, current_time - 3600))
    graph["block_to_entities"]["1"] = ["project_beta", "deadline", "budget", "team"]
    
    # Now querying "deadline" will match BOTH blocks equally
    # Only momentum from "project_alpha" context should differentiate them
    
    tests.append(MultiTurnTest(
        conversation_id="project_context",
        turns=[
            ConversationTurn("Tell me about project alpha", ["project_alpha"], 0, 1),
            ConversationTurn("What's the deadline?", ["deadline"], 0, 2),
            ConversationTurn("And the budget?", ["budget"], 0, 3),
            ConversationTurn("Who's on the team?", ["team"], 0, 4),
        ],
        description="Multi-turn project discussion - momentum should maintain context"
    ))
    
    # === TEST 2: Topic switching ===
    # Start with alpha, switch to beta, should track context switch
    tests.append(MultiTurnTest(
        conversation_id="topic_switch",
        turns=[
            ConversationTurn("Tell me about project alpha", ["project_alpha"], 0, 1),
            ConversationTurn("What's the deadline?", ["deadline"], 0, 2),  # Alpha
            ConversationTurn("Now tell me about beta", ["project_beta"], 1, 3),  # Switch
            ConversationTurn("What's that deadline?", ["deadline"], 1, 4),  # Should be Beta now
        ],
        description="Context switch test - should track when topic changes"
    ))
    
    # === HARDER CUBIC COSINE TEST ===
    # Target strength 5, competitors strength 3.5 (very close)
    for i in range(3):
        target_block = 10 + i
        query = f"cubic_test_{i}"
        
        # Target: strength 5
        for _ in range(5):
            add_entity(query, target_block, [f"target_answer_{i}"])
        with open(blocks_dir / f"facts_{target_block}.txt", "w") as f:
            f.write(f"cubic:{query}=correct_answer_{i}\n")
        os.utime(blocks_dir / f"facts_{target_block}.txt", (current_time - 1800, current_time - 1800))
        graph["block_to_entities"][str(target_block)] = [query.lower()]
        
        # Competitors: strength 3.5 each (very close competition)
        for j in range(4):
            comp_block = 20 + i * 10 + j
            for _ in range(3):
                add_entity(query, comp_block, [f"wrong_answer_{i}_{j}"])
            # Add 0.5 extra
            add_entity(query, comp_block, [f"wrong_answer_{i}_{j}"], strength=0.5)
            with open(blocks_dir / f"facts_{comp_block}.txt", "w") as f:
                f.write(f"cubic:{query}=wrong_{i}_{j}\n")
            os.utime(blocks_dir / f"facts_{comp_block}.txt", (current_time - 1800, current_time - 1800))
            graph["block_to_entities"][str(comp_block)] = [query.lower()]
    
    # === HARDER TUNNELING TEST (FIXED) ===
    # Direct query entity to target via strong bridge
    for i in range(3):
        target_block = 60 + i
        query = f"tunnel_query_{i}"
        bridge = f"strong_bridge_{i}"
        target_entity = f"tunnel_answer_{i}"
        
        # Query -> bridge -> target (strong path)
        for _ in range(10):
            add_entity(query, 55 + i, [bridge])  # Query in intermediate block
            add_entity(bridge, 55 + i, [query, target_entity])
        
        # Target block has bridge and answer
        add_entity(bridge, target_block, [target_entity])
        add_entity(target_entity, target_block, [bridge])
        
        with open(blocks_dir / f"facts_{target_block}.txt", "w") as f:
            f.write(f"tunnel:{query}={target_entity}\n")
        os.utime(blocks_dir / f"facts_{target_block}.txt", (current_time - 1800, current_time - 1800))
        graph["block_to_entities"][str(target_block)] = [bridge.lower(), target_entity.lower()]
        
        # Weak distractors: query -> weak connection -> noise
        for j in range(6):
            weak_entity = f"weak_{i}_{j}"
            distractor_block = 70 + i * 10 + j
            
            # Very weak connection (strength 0.3)
            add_entity(query, distractor_block, [weak_entity], strength=0.3)
            add_entity(weak_entity, distractor_block, [query], strength=0.3)
            
            with open(blocks_dir / f"facts_{distractor_block}.txt", "w") as f:
                f.write(f"tunnel:{query}=noise_{i}_{j}\n")
            os.utime(blocks_dir / f"facts_{distractor_block}.txt", (current_time - 3600, current_time - 3600))
            graph["block_to_entities"][str(distractor_block)] = [weak_entity.lower()]
    
    with open(index_dir / "entity_graph.json", "w") as f:
        json.dump(graph, f, indent=2)
    
    return tests, graph


def cubic_cosine(sim: float) -> float:
    return max(0.0, sim) ** 3


def retrieve_with_config(query_entities: List[str], config: AblationConfig, recent_entities: List[str] = None) -> List[Tuple[int, float]]:
    graph_file = HOLOGIT_PATH / "index" / "entity_graph.json"
    if not graph_file.exists():
        return []
    
    with open(graph_file) as f:
        graph = json.load(f)
    
    recent = recent_entities or []
    entity_scores: Dict[str, float] = {}
    
    if config.use_graph_walk:
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
                        pair_key = "|".join(sorted([e_lower, conn]))
                        co_count = graph.get("co_occurrence", {}).get(pair_key, 0)
                        e_count = graph.get("entity_counts", {}).get(e_lower, 1)
                        raw_sim = (co_count + 0.1) / (e_count + 0.1 * len(e_data.get("connections", [])))
                        
                        if config.use_cubic_cosine:
                            sim = cubic_cosine(raw_sim)
                        else:
                            sim = raw_sim
                        
                        next_score = ALPHA * score * sim
                        
                        if config.use_tunneling:
                            if next_score > TAU:
                                walk(conn, depth - 1, next_score)
                        else:
                            if next_score > 0.001:
                                walk(conn, depth - 1, next_score)
        
        for e in query_entities:
            walk(e, depth=3, score=1.0)
    else:
        for e in query_entities:
            e_lower = e.lower()
            if e_lower in graph.get("entities", {}):
                base_score = 1.0
                if config.use_cubic_cosine:
                    base_score = cubic_cosine(base_score)
                entity_scores[e_lower] = base_score
    
    block_scores: Dict[int, float] = {}
    block_strengths = graph.get("block_entity_strength", {})
    
    for entity, score in entity_scores.items():
        e_data = graph.get("entities", {}).get(entity, {})
        for bid in e_data.get("blocks", []):
            bid_str = str(bid)
            strength = block_strengths.get(bid_str, {}).get(entity, 1.0)
            raw_sim = max(0.0, min(1.0, 0.5 + 0.5 * math.log10(max(0.01, strength))))
            
            if config.use_cubic_cosine:
                sim = cubic_cosine(raw_sim)
            else:
                sim = raw_sim
            
            if config.use_tunneling and sim < TAU:
                continue
            
            weighted_score = score * sim
            block_scores[bid] = max(block_scores.get(bid, 0), weighted_score)
    
    current_time = time.time()
    final_scores: Dict[int, float] = {}
    
    for bid, base_score in block_scores.items():
        score = base_score
        
        if config.use_decay:
            facts_file = HOLOGIT_PATH / "blocks" / f"facts_{bid}.txt"
            if facts_file.exists():
                age_hours = (current_time - facts_file.stat().st_mtime) / 3600
                decay = LAMBDA ** age_hours
                score *= decay
        
        # MOMENTUM: Boost blocks that share entities with recent context
        if config.use_momentum and recent:
            block_entities = graph.get("block_to_entities", {}).get(str(bid), [])
            momentum = 0.0
            
            for r in recent:
                r_lower = r.lower()
                # Direct entity match in block
                if r_lower in block_entities:
                    momentum += 2.0  # Strong boost for direct match
                
                # Co-occurrence boost
                for e in block_entities:
                    pair_key = "|".join(sorted([e, r_lower]))
                    co_count = graph.get("co_occurrence", {}).get(pair_key, 0)
                    if co_count > 0:
                        momentum += co_count * 0.5
            
            if momentum > 0:
                score *= (1 + GAMMA * min(momentum, 5.0))
        
        final_scores[bid] = score
    
    sorted_blocks = sorted(final_scores.items(), key=lambda x: -x[1])
    return sorted_blocks[:TOP_K]


def run_multiturn_test(test: MultiTurnTest, config: AblationConfig) -> Dict:
    results = []
    recent_entities = []
    
    for turn in test.turns:
        retrieved = retrieve_with_config(turn.entities_mentioned, config, recent_entities)
        retrieved_blocks = [bid for bid, _ in retrieved]
        
        found = turn.target_block in retrieved_blocks
        rank = retrieved_blocks.index(turn.target_block) + 1 if found else 0
        
        results.append({
            "turn": turn.turn_number,
            "query": turn.user_query,
            "found": found,
            "rank": rank,
            "target": turn.target_block,
            "retrieved": retrieved_blocks[:3]
        })
        
        recent_entities.extend(turn.entities_mentioned)
        recent_entities = recent_entities[-10:]
    
    return {
        "conversation_id": test.conversation_id,
        "config": config.name,
        "turns": results,
        "accuracy": sum(1 for r in results if r["found"]) / len(results)
    }


def run_benchmark(args):
    print("=" * 60)
    print("Z.E.T.A. Multi-Turn & Harder Tests Benchmark")
    print("=" * 60)
    
    print("\n[SETUP] Creating test data...")
    tests, graph = setup_multiturn_data()
    
    results = {"benchmark": "multiturn", "tests": [], "cubic_test": {}, "tunnel_test": {}}
    
    print("\n[TEST] Multi-turn conversation tests...")
    for test in tests:
        print(f"\n  Conversation: {test.conversation_id}")
        print(f"  {test.description}")
        
        for config in CONFIGS:
            result = run_multiturn_test(test, config)
            results["tests"].append(result)
            
            print(f"    {config.name}: {result['accuracy']*100:.0f}%", end="")
            turns_str = " ".join(["✓" if t["found"] else "✗" for t in result["turns"]])
            print(f"  [{turns_str}]")
    
    print("\n[TEST] Harder cubic cosine test...")
    cubic_results = {"baseline": [], "plus_cubic": []}
    for i in range(3):
        query = f"cubic_test_{i}"
        target = 10 + i
        for config in [CONFIGS[0], CONFIGS[1]]:
            retrieved = retrieve_with_config([query], config)
            retrieved_blocks = [bid for bid, _ in retrieved]
            found = target in retrieved_blocks
            rank = retrieved_blocks.index(target) + 1 if found else 0
            cubic_results[config.name].append({"found": found, "rank": rank})
    
    baseline_r1 = sum(1 for r in cubic_results["baseline"] if r["rank"] == 1) / 3
    cubic_r1 = sum(1 for r in cubic_results["plus_cubic"] if r["rank"] == 1) / 3
    print(f"  Baseline: {baseline_r1*100:.0f}%  |  +Cubic: {cubic_r1*100:.0f}%")
    results["cubic_test"] = {"baseline": baseline_r1, "plus_cubic": cubic_r1}
    
    print("\n[TEST] Harder tunneling test...")
    tunnel_results = {c.name: [] for c in CONFIGS[:4]}
    for i in range(3):
        query = f"tunnel_query_{i}"
        target = 60 + i
        for config in CONFIGS[:4]:
            retrieved = retrieve_with_config([query], config)
            retrieved_blocks = [bid for bid, _ in retrieved]
            found = target in retrieved_blocks
            rank = retrieved_blocks.index(target) + 1 if found else 0
            tunnel_results[config.name].append({"found": found, "rank": rank, "retrieved": retrieved_blocks[:3]})
    
    for name in ["baseline", "plus_cubic", "plus_tunnel", "plus_decay"]:
        r1 = sum(1 for r in tunnel_results[name] if r["rank"] == 1) / 3
        print(f"  {name}: {r1*100:.0f}%")
        results["tunnel_test"][name] = r1
    
    # Summary
    print("\n" + "=" * 70)
    print("SUMMARY - Multi-turn by Config")
    print("=" * 70)
    print("| Config      | project_ctx | topic_switch |")
    print("|-------------|-------------|--------------|" )
    
    for config in CONFIGS:
        row = f"| {config.name:<11} |"
        for test in tests:
            for r in results["tests"]:
                if r["config"] == config.name and r["conversation_id"] == test.conversation_id:
                    row += f" {r['accuracy']*100:>10.0f}% |"
        print(row)
    
    output_path = Path(args.output) if args.output else Path("results/multiturn.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\n[SAVE] {output_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o")
    args = parser.parse_args()
    run_benchmark(args)


if __name__ == "__main__":
    main()
