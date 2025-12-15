#!/usr/bin/env python3
"""
Z.E.T.A. Benchmark Harness
Dual-Process Cognitive Architecture for CUDA benchmarks.

Architecture:
- 3B Subconscious (System 1): Fast, automatic memory management
- 14B Conscious (System 2): Slow, deliberate reasoning
- Memory Hierarchy: L0 (VRAM) <- L1 (RAM) <- L2 (NVMe)
- Direct Tunneling: >99% correlation bypasses 3B (muscle memory)
"""

import json
import math
import os
import subprocess
import sys
import time
from dataclasses import dataclass, field, asdict
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
import statistics

# Z.E.T.A. Parameters
LAMBDA = 0.95      # Temporal decay rate per hour
TAU = 0.1          # Tunneling threshold
GAMMA = 0.3        # Momentum coefficient
ALPHA = 0.85       # Damping factor (graph walk)
EPSILON = 0.1      # Smoothing constant
TUNNEL_THRESHOLD = 0.99  # Direct tunneling threshold
LAMBDA_MUSCLE = 0.90     # Muscle memory decay rate per hour (faster decay than general memory)
MUSCLE_THRESHOLD = 2.0   # Threshold for muscle memory activation (decay-weighted sum of confidences)

# Server configuration
LLAMA_3B_PORT = 9001
LLAMA_14B_PORT = 9000
HOLOGIT_PATH = Path("/mnt/HoloGit")

# Memory hierarchy sizes
L0_CAPACITY = 5    # Blocks in VRAM (immediate attention)
L1_CAPACITY = 20   # Blocks in RAM (working memory)
L2_CAPACITY = 1000 # Blocks on NVMe (long-term storage)


class MemoryLayer(Enum):
    """Three-layer memory hierarchy."""
    L0_VRAM = 0   # GPU VRAM - immediate attention
    L1_RAM = 1    # System RAM - working memory
    L2_NVME = 2   # NVMe SSD - long-term storage


@dataclass
class MemoryBlock:
    """A memory block that can migrate between layers."""
    block_id: int
    content: str
    entities: List[str]
    embedding: Optional[List[float]] = None
    timestamp: float = field(default_factory=time.time)
    access_count: int = 0
    layer: MemoryLayer = MemoryLayer.L2_NVME
    correlation_score: float = 0.0

    def age_hours(self) -> float:
        """Time since creation in hours."""
        return (time.time() - self.timestamp) / 3600

    def decay_factor(self) -> float:
        """Temporal decay D(t) = λ^t."""
        return LAMBDA ** self.age_hours()


@dataclass
class MemoryHierarchy:
    """Three-layer memory system managed by the 3B subconscious."""
    l0_vram: Dict[int, MemoryBlock] = field(default_factory=dict)  # Hot - in attention
    l1_ram: Dict[int, MemoryBlock] = field(default_factory=dict)   # Warm - correlated
    l2_nvme: Dict[int, MemoryBlock] = field(default_factory=dict)  # Cold - archived

    def surface_to_l0(self, block: MemoryBlock):
        """Surface a block to VRAM for 14B attention."""
        # Evict from current layer
        self._remove_from_all(block.block_id)

        # Add to L0
        if len(self.l0_vram) >= L0_CAPACITY:
            # Evict LRU to L1
            lru_id = min(self.l0_vram.keys(),
                        key=lambda k: self.l0_vram[k].access_count)
            evicted = self.l0_vram.pop(lru_id)
            self.cache_to_l1(evicted)

        block.layer = MemoryLayer.L0_VRAM
        block.access_count += 1
        self.l0_vram[block.block_id] = block

    def cache_to_l1(self, block: MemoryBlock):
        """Cache a correlated block to RAM."""
        self._remove_from_all(block.block_id)

        if len(self.l1_ram) >= L1_CAPACITY:
            # Evict LRU to L2
            lru_id = min(self.l1_ram.keys(),
                        key=lambda k: self.l1_ram[k].access_count)
            evicted = self.l1_ram.pop(lru_id)
            self.archive_to_l2(evicted)

        block.layer = MemoryLayer.L1_RAM
        self.l1_ram[block.block_id] = block

    def archive_to_l2(self, block: MemoryBlock):
        """Archive a block to NVMe."""
        self._remove_from_all(block.block_id)
        block.layer = MemoryLayer.L2_NVME
        self.l2_nvme[block.block_id] = block

    def _remove_from_all(self, block_id: int):
        """Remove block from all layers."""
        self.l0_vram.pop(block_id, None)
        self.l1_ram.pop(block_id, None)
        self.l2_nvme.pop(block_id, None)

    def get_all_blocks(self) -> List[MemoryBlock]:
        """Get all blocks across all layers."""
        return (list(self.l0_vram.values()) +
                list(self.l1_ram.values()) +
                list(self.l2_nvme.values()))

    def get_l0_context(self) -> List[MemoryBlock]:
        """Get blocks surfaced for 14B attention."""
        return list(self.l0_vram.values())

    def get_block_by_id(self, block_id: int) -> Optional[MemoryBlock]:
        """Find a block by ID across all layers."""
        if block_id in self.l0_vram:
            return self.l0_vram[block_id]
        if block_id in self.l1_ram:
            return self.l1_ram[block_id]
        if block_id in self.l2_nvme:
            return self.l2_nvme[block_id]
        return None


class ZetaSubconscious:
    """
    3B Subconscious (System 1) - Fast, automatic memory management.

    Responsibilities:
    1. Entity extraction from incoming context
    2. Graph walk to find related memories
    3. Relevance scoring (cosine × decay × momentum × entity prior)
    4. Memory hierarchy management:
       - Surface to VRAM: Most relevant for 14B attention
       - Cache in RAM: Correlated memories
       - Archive to NVMe: Unrelated memories
    """

    def __init__(self, memory: MemoryHierarchy, entity_graph: Dict):
        self.memory = memory
        self.entity_graph = entity_graph
        self.recent_entities: List[str] = []  # Last 10 entities for momentum
        self.entity_scores: Dict[str, float] = {}

    def extract_entities(self, text: str) -> List[str]:
        """Extract entities from text (simulated 3B extraction)."""
        # In production, this would call the 3B model
        # For benchmarking, we extract known entity patterns
        entities = []
        text_lower = text.lower()

        for entity in self.entity_graph.get("entities", {}).keys():
            if entity.lower() in text_lower:
                entities.append(entity.lower())

        return entities

    def graph_walk(self, query_entities: List[str], depth: int = 3) -> Dict[str, float]:
        """α-damped graph walk from query entities."""
        scores: Dict[str, float] = {}
        visited: set = set()

        def walk(entity: str, d: int, score: float):
            if d == 0 or entity in visited:
                return
            visited.add(entity)

            e_data = self.entity_graph.get("entities", {}).get(entity, {})
            if e_data:
                scores[entity] = max(scores.get(entity, 0), score)

                for conn in e_data.get("connections", []):
                    if conn not in visited:
                        # Edge weight from co-occurrence
                        pair_key = "|".join(sorted([entity, conn]))
                        co_count = self.entity_graph.get("co_occurrence", {}).get(pair_key, 0)
                        e_count = self.entity_graph.get("entity_counts", {}).get(entity, 1)

                        # P(B|A) with smoothing
                        neighbors = len(e_data.get("connections", []))
                        edge_prob = (co_count + EPSILON) / (e_count + EPSILON * neighbors)

                        # Damped propagation
                        next_score = ALPHA * score * edge_prob

                        # Tunneling gate
                        if next_score > TAU:
                            walk(conn, d - 1, next_score)

        for e in query_entities:
            walk(e.lower(), depth, 1.0)

        self.entity_scores = scores
        return scores

    def compute_momentum(self, entity: str) -> float:
        """Compute momentum M(e) from recent conversation."""
        if not self.recent_entities:
            return 0.0

        momentum = 0.0
        for r in self.recent_entities:
            pair_key = "|".join(sorted([entity.lower(), r.lower()]))
            co_count = self.entity_graph.get("co_occurrence", {}).get(pair_key, 0)
            r_count = self.entity_graph.get("entity_counts", {}).get(r.lower(), 1)
            momentum += co_count / max(r_count, 1)

        return GAMMA * momentum / len(self.recent_entities)

    def score_block(self, block: MemoryBlock, query_entities: List[str]) -> float:
        """
        Compute unified retrieval score R(b).

        R(b) = E(q,s) × D(t) × (1 + max[S(e) × (1 + M(e))])
        """
        # Entity prior from graph walk
        entity_prior = 0.0
        for entity in block.entities:
            e_lower = entity.lower()
            s_e = self.entity_scores.get(e_lower, 0.0)
            m_e = self.compute_momentum(e_lower)
            entity_prior = max(entity_prior, s_e * (1 + m_e))

        # Cosine similarity (simplified - use embedding in production)
        query_set = set(e.lower() for e in query_entities)
        block_set = set(e.lower() for e in block.entities)
        overlap = len(query_set & block_set)
        if overlap == 0:
            cosine_sim = 0.0
        else:
            cosine_sim = overlap / math.sqrt(len(query_set) * len(block_set))

        # Sharpened cosine (ReLU(cos)³)
        sharpened = max(0, cosine_sim) ** 3

        # Temporal decay
        decay = block.decay_factor()

        # Unified score
        score = sharpened * decay * (1 + entity_prior)
        block.correlation_score = score

        return score

    def manage_hierarchy(self, query_entities: List[str]) -> List[MemoryBlock]:
        """
        Core subconscious function: manage memory hierarchy based on query.

        1. Score all blocks
        2. Surface top-k to L0 (VRAM) for 14B attention
        3. Cache correlated to L1 (RAM)
        4. Archive unrelated to L2 (NVMe)

        Returns: Blocks surfaced to L0 for 14B
        """
        # Graph walk to get entity scores
        self.graph_walk(query_entities)

        # Score all blocks
        all_blocks = self.memory.get_all_blocks()
        for block in all_blocks:
            self.score_block(block, query_entities)

        # Sort by score
        scored_blocks = sorted(all_blocks, key=lambda b: b.correlation_score, reverse=True)

        # Surface top L0_CAPACITY to VRAM
        for i, block in enumerate(scored_blocks):
            if i < L0_CAPACITY:
                self.memory.surface_to_l0(block)
            elif block.correlation_score > TAU:
                self.memory.cache_to_l1(block)
            else:
                self.memory.archive_to_l2(block)

        # Update recent entities for momentum
        self.recent_entities = (query_entities + self.recent_entities)[:10]

        return self.memory.get_l0_context()


@dataclass
class MuscleMemoryAccess:
    """A single access event for muscle memory tracking."""
    timestamp: float
    confidence: float
    block_id: int


class ZetaConscious:
    """
    14B Conscious (System 2) - Slow, deliberate reasoning.

    Responsibilities:
    1. Receive pre-surfaced context from 3B subconscious
    2. Deliberate reasoning over curated context
    3. Response generation
    4. Direct tunneling for high-confidence (>99%) recalls

    Muscle Memory Model:
    Instead of hard access counts, muscle memory uses decay-weighted confidence:

    M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))

    Pattern becomes muscle memory when M_muscle > MUSCLE_THRESHOLD.
    This models biological muscle memory: requires practice AND decays without it.
    """

    def __init__(self, memory: MemoryHierarchy):
        self.memory = memory
        # Muscle memory: entity -> list of access events
        self.muscle_memory: Dict[str, List[MuscleMemoryAccess]] = {}
        # Cache of which blocks are currently tunneled
        self.tunnel_cache: Dict[str, int] = {}  # entity -> block_id

    def compute_muscle_score(self, entity: str) -> float:
        """
        Compute decay-weighted muscle memory score.

        M_muscle(e) = Σ (confidence_i × λ_muscle^(t_now - t_i))

        Higher score = stronger muscle memory.
        Decays without practice (like real muscle memory).
        """
        e_lower = entity.lower()
        if e_lower not in self.muscle_memory:
            return 0.0

        now = time.time()
        score = 0.0

        for access in self.muscle_memory[e_lower]:
            hours_ago = (now - access.timestamp) / 3600
            decay = LAMBDA_MUSCLE ** hours_ago
            score += access.confidence * decay

        return score

    def check_direct_tunnel(self, query_entities: List[str]) -> Optional[MemoryBlock]:
        """
        Check if any entity qualifies for direct tunneling (muscle memory).

        Uses decay-weighted muscle memory score instead of hard counts.
        Pattern activates when M_muscle(e) > MUSCLE_THRESHOLD.
        """
        # Check immediate high-confidence blocks
        for block in self.memory.get_all_blocks():
            if block.correlation_score > TUNNEL_THRESHOLD:
                return block

        # Check muscle memory for each entity
        for entity in query_entities:
            e_lower = entity.lower()
            muscle_score = self.compute_muscle_score(e_lower)

            if muscle_score > MUSCLE_THRESHOLD:
                # Muscle memory activated - find the cached block
                if e_lower in self.tunnel_cache:
                    block_id = self.tunnel_cache[e_lower]
                    block = self.memory.get_block_by_id(block_id)
                    if block:
                        return block

        return None

    def reason(self, query: str, surfaced_context: List[MemoryBlock]) -> str:
        """
        Deliberate reasoning over surfaced context.

        In production, this calls the 14B model with the surfaced blocks.
        For benchmarking, we simulate by extracting relevant facts.
        """
        # Check for direct tunnel first
        query_entities = [e for e in query.lower().split() if len(e) > 3]
        tunneled = self.check_direct_tunnel(query_entities)

        if tunneled:
            # Muscle memory - bypass full reasoning
            return f"[TUNNEL] {tunneled.content}"

        # Full reasoning over surfaced context
        context_text = "\n".join([b.content for b in surfaced_context])
        return f"[REASON] Context: {context_text[:200]}..."

    def train_muscle_memory(self, entity: str, block: MemoryBlock):
        """
        Train a pattern as muscle memory through repeated high-confidence access.

        Each access adds to the decay-weighted muscle score.
        Pattern becomes automatic when cumulative score exceeds threshold.

        Like biological muscle memory:
        - Requires practice (repeated access)
        - Higher confidence = stronger training
        - Decays without practice
        """
        if block.correlation_score < 0.9:
            # Only train on high-confidence accesses
            return

        e_lower = entity.lower()

        # Record this access
        access = MuscleMemoryAccess(
            timestamp=time.time(),
            confidence=block.correlation_score,
            block_id=block.block_id
        )

        if e_lower not in self.muscle_memory:
            self.muscle_memory[e_lower] = []

        self.muscle_memory[e_lower].append(access)

        # Update tunnel cache with the most accessed block
        self.tunnel_cache[e_lower] = block.block_id

    def get_muscle_memory_stats(self) -> Dict[str, Dict]:
        """Get statistics about current muscle memory state."""
        stats = {}
        for entity, accesses in self.muscle_memory.items():
            score = self.compute_muscle_score(entity)
            stats[entity] = {
                "total_accesses": len(accesses),
                "muscle_score": score,
                "is_active": score > MUSCLE_THRESHOLD,
                "block_id": self.tunnel_cache.get(entity)
            }
        return stats


class ZetaCognitiveSystem:
    """
    Complete Z.E.T.A. dual-process cognitive system.

    - Subconscious (3B): Memory management, entity extraction, hierarchy surfacing
    - Conscious (14B): Deliberate reasoning with pre-surfaced context
    - Direct Tunneling: High-confidence patterns bypass subconscious
    """

    def __init__(self, entity_graph: Dict):
        self.memory = MemoryHierarchy()
        self.subconscious = ZetaSubconscious(self.memory, entity_graph)
        self.conscious = ZetaConscious(self.memory)
        self.entity_graph = entity_graph

        # Timing metrics
        self.last_extraction_ms = 0.0
        self.last_graph_walk_ms = 0.0
        self.last_surfacing_ms = 0.0
        self.last_reasoning_ms = 0.0
        self.last_tunnel_used = False

    def add_memory(self, block_id: int, content: str, entities: List[str],
                   age_hours: float = 0.0):
        """Add a memory block to the system."""
        block = MemoryBlock(
            block_id=block_id,
            content=content,
            entities=entities,
            timestamp=time.time() - (age_hours * 3600)
        )
        self.memory.archive_to_l2(block)

    def process_query(self, query: str) -> Tuple[str, Dict[str, float]]:
        """
        Full cognitive pipeline:
        1. 3B extracts entities
        2. 3B walks graph
        3. 3B surfaces relevant to L0
        4. 14B reasons over surfaced context (or direct tunnel)

        Returns: (response, timing_dict)
        """
        timings = {}

        # Step 1: Entity extraction (3B)
        start = time.perf_counter()
        query_entities = self.subconscious.extract_entities(query)
        self.last_extraction_ms = (time.perf_counter() - start) * 1000
        timings["extraction_ms"] = self.last_extraction_ms

        # Check for direct tunnel first (muscle memory)
        tunneled = self.conscious.check_direct_tunnel(query_entities)
        if tunneled:
            self.last_tunnel_used = True
            timings["tunneled"] = True
            return tunneled.content, timings

        self.last_tunnel_used = False

        # Step 2: Graph walk (3B)
        start = time.perf_counter()
        self.subconscious.graph_walk(query_entities)
        self.last_graph_walk_ms = (time.perf_counter() - start) * 1000
        timings["graph_walk_ms"] = self.last_graph_walk_ms

        # Step 3: Manage hierarchy and surface to L0 (3B)
        start = time.perf_counter()
        surfaced = self.subconscious.manage_hierarchy(query_entities)
        self.last_surfacing_ms = (time.perf_counter() - start) * 1000
        timings["surfacing_ms"] = self.last_surfacing_ms

        # Step 4: Deliberate reasoning (14B)
        start = time.perf_counter()
        response = self.conscious.reason(query, surfaced)
        self.last_reasoning_ms = (time.perf_counter() - start) * 1000
        timings["reasoning_ms"] = self.last_reasoning_ms

        # Train muscle memory for high-confidence patterns
        for entity in query_entities:
            for block in surfaced:
                self.conscious.train_muscle_memory(entity, block)

        return response, timings

    def get_surfaced_blocks(self) -> List[MemoryBlock]:
        """Get blocks currently surfaced in L0 (VRAM)."""
        return self.memory.get_l0_context()


# =============================================================================
# Benchmark Configuration
# =============================================================================

@dataclass
class BenchmarkConfig:
    """Configuration for a benchmark run."""
    name: str
    mode: str  # "baseline", "zero", "subzero"
    use_subconscious: bool = False  # 3B memory manager
    use_conscious: bool = True      # 14B reasoning
    use_memory: bool = False
    use_direct_tunnel: bool = False

    @classmethod
    def baseline(cls) -> "BenchmarkConfig":
        """14B only - no memory system."""
        return cls(
            name="baseline",
            mode="baseline",
            use_subconscious=False,
            use_conscious=True,
            use_memory=False,
            use_direct_tunnel=False
        )

    @classmethod
    def zero(cls) -> "BenchmarkConfig":
        """14B + Z.E.T.A. memory parameters (no 3B)."""
        return cls(
            name="zero",
            mode="zero",
            use_subconscious=False,
            use_conscious=True,
            use_memory=True,
            use_direct_tunnel=True
        )

    @classmethod
    def subzero(cls) -> "BenchmarkConfig":
        """Full dual-process: 3B subconscious + 14B conscious."""
        return cls(
            name="subzero",
            mode="subzero",
            use_subconscious=True,
            use_conscious=True,
            use_memory=True,
            use_direct_tunnel=True
        )


@dataclass
class TimingResult:
    """Timing measurements for operations."""
    name: str
    times_ms: List[float] = field(default_factory=list)

    @property
    def mean_ms(self) -> float:
        return statistics.mean(self.times_ms) if self.times_ms else 0.0

    @property
    def std_ms(self) -> float:
        return statistics.stdev(self.times_ms) if len(self.times_ms) > 1 else 0.0

    @property
    def p95_ms(self) -> float:
        if not self.times_ms:
            return 0.0
        sorted_times = sorted(self.times_ms)
        idx = int(0.95 * len(sorted_times))
        return sorted_times[min(idx, len(sorted_times) - 1)]

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "count": len(self.times_ms),
            "mean_ms": round(self.mean_ms, 2),
            "std_ms": round(self.std_ms, 2),
            "p95_ms": round(self.p95_ms, 2)
        }


class Timer:
    """Context manager for timing."""

    def __init__(self, name: str, result: Optional[TimingResult] = None):
        self.name = name
        self.result = result
        self.elapsed_ms = 0.0

    def __enter__(self):
        self.start_time = time.perf_counter()
        return self

    def __exit__(self, *args):
        self.elapsed_ms = (time.perf_counter() - self.start_time) * 1000
        if self.result:
            self.result.times_ms.append(self.elapsed_ms)


# =============================================================================
# Utility Functions
# =============================================================================

def clear_hologit():
    """Clear HoloGit storage."""
    if HOLOGIT_PATH.exists():
        for subdir in ["blocks", "index", "state"]:
            d = HOLOGIT_PATH / subdir
            if d.exists():
                for f in d.glob("*"):
                    if f.is_file():
                        f.unlink()
        print("[HOLOGIT] Cleared storage")
    else:
        for subdir in ["blocks", "index", "state", "commits", "branches"]:
            (HOLOGIT_PATH / subdir).mkdir(parents=True, exist_ok=True)
        print("[HOLOGIT] Created structure")


def check_server_health(port: int) -> bool:
    """Check if llama.cpp server is running."""
    import urllib.request
    try:
        url = f"http://localhost:{port}/health"
        req = urllib.request.Request(url, method="GET")
        with urllib.request.urlopen(req, timeout=2) as resp:
            return resp.status == 200
    except:
        return False


def get_hardware_info() -> Dict:
    """Collect hardware info."""
    info = {"device": "Z6", "timestamp": datetime.now().isoformat()}

    try:
        result = subprocess.run(
            ["nvidia-smi", "--query-gpu=name,memory.total", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode == 0:
            parts = result.stdout.strip().split(", ")
            if len(parts) >= 2:
                info["gpu"] = parts[0]
                info["vram_mb"] = int(parts[1])
    except:
        pass

    return info


# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("Z.E.T.A. Dual-Process Cognitive Benchmark Harness")
    print("=" * 50)

    print("\n[Architecture]")
    print("  3B Subconscious (System 1): Memory management, extraction, surfacing")
    print("  14B Conscious (System 2): Deliberate reasoning")
    print("  Direct Tunneling: >99% correlation bypasses 3B (muscle memory)")

    print("\n[Memory Hierarchy]")
    print(f"  L0 (VRAM): {L0_CAPACITY} blocks - immediate attention")
    print(f"  L1 (RAM):  {L1_CAPACITY} blocks - working memory")
    print(f"  L2 (NVMe): {L2_CAPACITY} blocks - long-term storage")

    print("\n[Configurations]")
    for cfg in [BenchmarkConfig.baseline(), BenchmarkConfig.zero(), BenchmarkConfig.subzero()]:
        print(f"  {cfg.name}: subconscious={cfg.use_subconscious}, "
              f"conscious={cfg.use_conscious}, memory={cfg.use_memory}, "
              f"tunnel={cfg.use_direct_tunnel}")

    print("\n[Server Health]")
    print(f"  3B (:{LLAMA_3B_PORT}): {'OK' if check_server_health(LLAMA_3B_PORT) else 'NOT RUNNING'}")
    print(f"  14B (:{LLAMA_14B_PORT}): {'OK' if check_server_health(LLAMA_14B_PORT) else 'NOT RUNNING'}")

    # Demo cognitive system
    print("\n[Demo] Testing cognitive system...")
    demo_graph = {
        "entities": {
            "paris": {"blocks": [1], "connections": ["france", "capital"]},
            "france": {"blocks": [1], "connections": ["paris", "europe"]},
            "capital": {"blocks": [1], "connections": ["paris", "city"]}
        },
        "co_occurrence": {
            "france|paris": 10,
            "capital|paris": 8,
            "europe|france": 5
        },
        "entity_counts": {"paris": 10, "france": 8, "capital": 5, "europe": 3}
    }

    system = ZetaCognitiveSystem(demo_graph)
    system.add_memory(1, "Paris is the capital of France", ["paris", "france", "capital"])
    system.add_memory(2, "Berlin is in Germany", ["berlin", "germany"])

    response, timings = system.process_query("What is the capital of France?")
    print(f"  Query: 'What is the capital of France?'")
    print(f"  Response: {response}")
    print(f"  Timings: {timings}")
    print(f"  Surfaced to L0: {len(system.get_surfaced_blocks())} blocks")
