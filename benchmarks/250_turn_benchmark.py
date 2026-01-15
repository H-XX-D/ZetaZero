#!/usr/bin/env python3
"""
Z.E.T.A. 250-Turn Conversation Benchmark
Compares ZetaZero (with graph memory) vs Baseline llama.cpp (no memory)
Tracks: latency, tokens, accumulated power consumption
"""

import json
import time
import requests
import subprocess
from pathlib import Path
from datetime import datetime
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional
import threading

# Configuration
ZETA_URL = "https://l5s0tvdyr1dty6-8080.proxy.runpod.net"
BASELINE_URL = "https://l5s0tvdyr1dty6-9000.proxy.runpod.net"
RUNPOD_SSH = "l5s0tvdyr1dty6-64410d4f@ssh.runpod.io"
SSH_KEY = "/Users/hendrixx./.ssh/id_ed25519"
NUM_TURNS = 250
MAX_TOKENS = 150
TIMEOUT = 120

# Realistic conversation turns simulating extended user session
CONVERSATION_TOPICS = [
    # Technical discussions
    "Explain how neural networks learn through backpropagation",
    "What are the key differences between REST and GraphQL APIs?",
    "How does garbage collection work in modern programming languages?",
    "Describe the CAP theorem and its implications for distributed systems",
    "What is the difference between concurrency and parallelism?",
    "Explain how TLS/SSL handshake works",
    "What are microservices and when should you use them?",
    "How do database indexes improve query performance?",
    "Explain the concept of eventual consistency",
    "What is a bloom filter and when would you use one?",
    
    # Code questions
    "Write a function to find the longest common subsequence",
    "How would you implement a thread-safe singleton in Python?",
    "Explain async/await and how it differs from threading",
    "What's the time complexity of quicksort?",
    "How do you handle race conditions in concurrent code?",
    "Write a function to detect a cycle in a linked list",
    "Explain the observer pattern with an example",
    "How would you implement rate limiting in an API?",
    "What are the SOLID principles?",
    "Explain dependency injection and its benefits",
    
    # Follow-up patterns
    "Can you give me a more detailed example?",
    "What are the trade-offs of that approach?",
    "How would this work at scale?",
    "What's the industry standard for this?",
    "Can you compare that to the alternative?",
    "What are common mistakes to avoid?",
    "How do I test this properly?",
    "What's the performance impact?",
    "Are there security considerations?",
    "How would you debug this?",
    
    # General knowledge
    "What causes the northern lights?",
    "How do quantum computers differ from classical computers?",
    "Explain the concept of machine learning bias",
    "What is the Turing test?",
    "How does natural language processing work?",
    "What are transformers in ML?",
    "Explain the attention mechanism",
    "What is reinforcement learning?",
    "How do recommendation systems work?",
    "What is federated learning?",
    
    # Context-building turns
    "Remember we discussed {topic} earlier. Can you expand on that?",
    "Going back to your previous point about {topic}",
    "You mentioned something about {topic} - can you clarify?",
    "How does this relate to what we talked about before?",
    "Can you summarize our discussion so far?",
    "What's the most important takeaway from our conversation?",
    "Based on everything we've discussed, what would you recommend?",
    "I'm still confused about the earlier point on {topic}",
    "Let's revisit the {topic} concept",
    "Connect this to the previous topics we covered",
]


@dataclass
class TurnResult:
    turn_num: int
    query: str
    success: bool
    latency_s: float
    prompt_tokens: int
    completion_tokens: int
    total_tokens: int
    power_w: float
    energy_j: float
    response_preview: str
    error: Optional[str] = None


class PowerMonitor:
    """Monitors GPU power via SSH to Runpod"""
    def __init__(self, interval_s=0.5):
        self.interval_s = interval_s
        self.samples = []
        self._stop = False
        self._thread = None
    
    def _get_power(self) -> float:
        try:
            result = subprocess.run(
                ["ssh", "-o", "BatchMode=yes", "-o", "StrictHostKeyChecking=no",
                 "-i", SSH_KEY, RUNPOD_SSH,
                 "nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode == 0:
                return float(result.stdout.strip())
        except Exception:
            pass
        return 0.0
    
    def start(self):
        self.samples = []
        self._stop = False
        def _run():
            while not self._stop:
                p = self._get_power()
                if p > 0:
                    self.samples.append((time.time(), p))
                time.sleep(self.interval_s)
        self._thread = threading.Thread(target=_run, daemon=True)
        self._thread.start()
    
    def stop(self):
        self._stop = True
        if self._thread:
            self._thread.join(timeout=2)
    
    def get_stats(self) -> tuple:
        """Returns (energy_j, avg_power_w, peak_power_w)"""
        if len(self.samples) < 2:
            return 0.0, 0.0, 0.0
        total_energy = 0.0
        for i in range(1, len(self.samples)):
            t0, p0 = self.samples[i-1]
            t1, p1 = self.samples[i]
            dt = max(0.0, t1 - t0)
            total_energy += (p0 + p1) * 0.5 * dt  # Trapezoidal integration
        duration = self.samples[-1][0] - self.samples[0][0]
        avg_power = total_energy / max(duration, 1e-6)
        peak_power = max(p for _, p in self.samples)
        return total_energy, avg_power, peak_power


def generate_turn(turn_num: int, prev_topics: List[str]) -> str:
    """Generate a conversation turn, sometimes referencing previous context"""
    base_idx = turn_num % len(CONVERSATION_TOPICS)
    query = CONVERSATION_TOPICS[base_idx]
    
    # Sometimes reference previous topics for context continuity
    if "{topic}" in query and prev_topics:
        topic = prev_topics[-1] if prev_topics else "the previous concept"
        query = query.format(topic=topic)
    
    return query


def run_turn(url: str, query: str, turn_num: int, conversation_history: List[Dict]) -> TurnResult:
    """Execute a single conversation turn"""
    power_monitor = PowerMonitor()
    power_monitor.start()
    
    # Build messages with conversation history for context
    messages = conversation_history[-10:] + [{"role": "user", "content": query}]
    
    start = time.time()
    try:
        response = requests.post(
            f"{url}/v1/chat/completions",
            json={
                "model": "llama",
                "messages": messages,
                "max_tokens": MAX_TOKENS,
                "temperature": 0.7,
            },
            timeout=TIMEOUT
        )
        latency = time.time() - start
        power_monitor.stop()
        energy_j, avg_power, peak_power = power_monitor.get_stats()
        
        if response.status_code == 200:
            data = response.json()
            usage = data.get("usage", {})
            content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
            
            return TurnResult(
                turn_num=turn_num,
                query=query,
                success=True,
                latency_s=latency,
                prompt_tokens=usage.get("prompt_tokens", 0),
                completion_tokens=usage.get("completion_tokens", 0),
                total_tokens=usage.get("total_tokens", 0),
                power_w=avg_power,
                energy_j=energy_j,
                response_preview=content[:100]
            )
        else:
            return TurnResult(
                turn_num=turn_num, query=query, success=False, latency_s=latency,
                prompt_tokens=0, completion_tokens=0, total_tokens=0,
                power_w=avg_power, energy_j=energy_j, response_preview="",
                error=response.text[:200]
            )
    except Exception as e:
        power_monitor.stop()
        latency = time.time() - start
        energy_j, avg_power, _ = power_monitor.get_stats()
        return TurnResult(
            turn_num=turn_num, query=query, success=False, latency_s=latency,
            prompt_tokens=0, completion_tokens=0, total_tokens=0,
            power_w=avg_power, energy_j=energy_j, response_preview="",
            error=str(e)
        )


def run_benchmark(name: str, url: str, num_turns: int) -> Dict:
    """Run full benchmark for one server"""
    print(f"\n{'='*60}")
    print(f"Running {name} Benchmark ({num_turns} turns)")
    print(f"URL: {url}")
    print(f"{'='*60}")
    
    results = []
    conversation_history = []
    prev_topics = []
    total_energy = 0.0
    
    for i in range(1, num_turns + 1):
        query = generate_turn(i, prev_topics)
        result = run_turn(url, query, i, conversation_history)
        results.append(result)
        
        if result.success:
            total_energy += result.energy_j
            # Update conversation history
            conversation_history.append({"role": "user", "content": query})
            conversation_history.append({"role": "assistant", "content": result.response_preview})
            # Track topics for context references
            if len(query) > 20:
                prev_topics.append(query[:30])
        
        status = "✓" if result.success else "✗"
        print(f"[{i:03d}/{num_turns}] {result.latency_s:5.2f}s {result.power_w:5.0f}W {result.energy_j:6.1f}J {status} - {query[:40]}...")
        
        # Progress checkpoint every 50 turns
        if i % 50 == 0:
            successful = [r for r in results if r.success]
            print(f"\n--- Checkpoint at turn {i} ---")
            print(f"  Success: {len(successful)}/{i}")
            print(f"  Total Energy: {total_energy:.1f}J ({total_energy/3600:.4f} Wh)")
            print(f"  Avg Latency: {sum(r.latency_s for r in successful)/len(successful):.2f}s")
            print()
    
    # Calculate summary statistics
    successful = [r for r in results if r.success]
    summary = {
        "name": name,
        "url": url,
        "num_turns": num_turns,
        "successful_turns": len(successful),
        "success_rate": f"{len(successful)}/{num_turns}",
        "total_time_s": sum(r.latency_s for r in successful),
        "avg_latency_s": sum(r.latency_s for r in successful) / len(successful) if successful else 0,
        "total_tokens": sum(r.total_tokens for r in successful),
        "avg_tokens_per_turn": sum(r.total_tokens for r in successful) / len(successful) if successful else 0,
        "total_energy_j": total_energy,
        "total_energy_wh": total_energy / 3600,
        "avg_power_w": sum(r.power_w for r in successful) / len(successful) if successful else 0,
        "energy_per_turn_j": total_energy / len(successful) if successful else 0,
    }
    
    return {
        "summary": summary,
        "results": [asdict(r) for r in results]
    }


def main():
    print("="*70)
    print("Z.E.T.A. 250-Turn Conversation Benchmark")
    print(f"Date: {datetime.now().isoformat()}")
    print("="*70)
    
    # Check server health
    print("\nChecking server health...")
    try:
        zeta_health = requests.get(f"{ZETA_URL}/health", timeout=10).json()
        print(f"  ZetaZero: {zeta_health.get('status', 'unknown')}")
    except Exception as e:
        print(f"  ZetaZero: FAILED - {e}")
        return
    
    try:
        baseline_health = requests.get(f"{BASELINE_URL}/health", timeout=10).json()
        print(f"  Baseline: {baseline_health.get('status', 'unknown')}")
    except Exception as e:
        print(f"  Baseline: FAILED - {e}")
        return
    
    # Run benchmarks
    zeta_results = run_benchmark("ZetaZero", ZETA_URL, NUM_TURNS)
    baseline_results = run_benchmark("Baseline", BASELINE_URL, NUM_TURNS)
    
    # Comparison
    print("\n" + "="*70)
    print("BENCHMARK COMPARISON")
    print("="*70)
    
    zs = zeta_results["summary"]
    bs = baseline_results["summary"]
    
    print(f"\n{'Metric':<30} {'ZetaZero':>15} {'Baseline':>15} {'Delta':>15}")
    print("-"*75)
    print(f"{'Success Rate':<30} {zs['success_rate']:>15} {bs['success_rate']:>15}")
    print(f"{'Total Time (s)':<30} {zs['total_time_s']:>15.1f} {bs['total_time_s']:>15.1f} {zs['total_time_s']-bs['total_time_s']:>+15.1f}")
    print(f"{'Avg Latency (s)':<30} {zs['avg_latency_s']:>15.2f} {bs['avg_latency_s']:>15.2f} {zs['avg_latency_s']-bs['avg_latency_s']:>+15.2f}")
    print(f"{'Total Tokens':<30} {zs['total_tokens']:>15,} {bs['total_tokens']:>15,} {zs['total_tokens']-bs['total_tokens']:>+15,}")
    print(f"{'Total Energy (J)':<30} {zs['total_energy_j']:>15.1f} {bs['total_energy_j']:>15.1f} {zs['total_energy_j']-bs['total_energy_j']:>+15.1f}")
    print(f"{'Total Energy (Wh)':<30} {zs['total_energy_wh']:>15.4f} {bs['total_energy_wh']:>15.4f} {zs['total_energy_wh']-bs['total_energy_wh']:>+15.4f}")
    print(f"{'Avg Power (W)':<30} {zs['avg_power_w']:>15.1f} {bs['avg_power_w']:>15.1f} {zs['avg_power_w']-bs['avg_power_w']:>+15.1f}")
    print(f"{'Energy per Turn (J)':<30} {zs['energy_per_turn_j']:>15.2f} {bs['energy_per_turn_j']:>15.2f} {zs['energy_per_turn_j']-bs['energy_per_turn_j']:>+15.2f}")
    
    # Calculate efficiency
    if bs['total_energy_j'] > 0:
        energy_savings = (bs['total_energy_j'] - zs['total_energy_j']) / bs['total_energy_j'] * 100
        print(f"\n{'Energy Savings (ZetaZero vs Baseline)':<30} {energy_savings:>+15.1f}%")
    
    if bs['total_time_s'] > 0:
        time_diff = (bs['total_time_s'] - zs['total_time_s']) / bs['total_time_s'] * 100
        print(f"{'Time Difference':<30} {time_diff:>+15.1f}%")
    
    # Save results
    output = {
        "benchmark": "250-Turn Conversation",
        "date": datetime.now().isoformat(),
        "config": {
            "num_turns": NUM_TURNS,
            "max_tokens": MAX_TOKENS,
            "zeta_url": ZETA_URL,
            "baseline_url": BASELINE_URL,
        },
        "zeta": zeta_results,
        "baseline": baseline_results,
    }
    
    output_path = Path(__file__).parent / "250_turn_results.json"
    output_path.write_text(json.dumps(output, indent=2))
    print(f"\nResults saved to: {output_path}")


if __name__ == "__main__":
    main()
