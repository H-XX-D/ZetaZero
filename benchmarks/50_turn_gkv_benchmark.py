#!/usr/bin/env python3
"""
50-Turn GKV Benchmark v2
Measures: latency, energy savings, GKV injection effectiveness
Method: Uses /gkv/stats delta before/after each request (server doesn't expose per-request metrics)
"""

import requests
import json
import time
import subprocess
import threading
import statistics
from datetime import datetime

ZETA_URL = "http://192.168.0.165:8080"
POWER_SAMPLE_INTERVAL_S = 0.25
WARMUP_TURNS = 2


def get_gkv_stats():
    """Get current GKV statistics from server."""
    try:
        resp = requests.get(f"{ZETA_URL}/gkv/stats", timeout=5)
        if resp.status_code == 200:
            return resp.json()
    except:
        pass
    return {"injections": 0, "prefill_saved_sec": 0.0, "enabled": False}


def get_health():
    """Get server health and graph stats."""
    try:
        resp = requests.get(f"{ZETA_URL}/health", timeout=5)
        if resp.status_code == 200:
            return resp.json()
    except:
        pass
    return {"graph_nodes": 0, "graph_edges": 0}

# Same conversation turns from original benchmark
CONVERSATION_TURNS = [
    "My name is Alex and I work at TechCorp as a senior engineer.",
    "I have a golden retriever named Bruno who is 3 years old.",
    "My favorite programming language is Rust because of memory safety.",
    "I live in Seattle and commute by bike most days.",
    "What is the capital of France?",
    "My project deadline is March 15th for the authentication module.",
    "Bruno loves playing fetch at the dog park near Green Lake.",
    "Can you explain how hash tables work?",
    "My manager Sarah wants weekly status updates on Fridays.",
    "What is my dogs name?",
    "I prefer vim over VSCode for quick edits.",
    "The TechCorp office is on 5th Avenue downtown.",
    "What programming language did I say I prefer?",
    "My wife Emma is a doctor at Swedish Hospital.",
    "We are planning a vacation to Japan in April.",
    "How does TCP/IP work at a high level?",
    "What is my project deadline?",
    "Bruno had his vet checkup last week - all healthy.",
    "I started learning Japanese for our trip.",
    "What company do I work for?",
    "The authentication module uses OAuth 2.0.",
    "Emma and I got married 5 years ago in Portland.",
    "What is photosynthesis?",
    "Who is my manager?",
    "My bike is a Trek Domane - great for Seattle hills.",
    "We want to visit Tokyo and Kyoto during cherry blossom season.",
    "Explain the difference between SQL and NoSQL databases.",
    "Where do I live?",
    "Bruno is afraid of thunderstorms.",
    "I am considering switching to a standing desk.",
    "What is my wifes name and profession?",
    "The OAuth implementation needs to support PKCE.",
    "Sarah approved my vacation request yesterday.",
    "What pet do I have and what breed?",
    "I use a mechanical keyboard with Cherry MX Browns.",
    "How does garbage collection work in Java?",
    "When is my vacation and where am I going?",
    "TechCorp is migrating to Kubernetes next quarter.",
    "Bruno weighs about 70 pounds.",
    "What is my name and job title?",
    "Emma works night shifts every other week.",
    "The Green Lake dog park is about 2 miles from my house.",
    "Explain how transformers work in machine learning.",
    "What updates does my manager want and when?",
    "I bike about 8 miles each way to work.",
    "We are booking flights through Alaska Airlines.",
    "What is the OAuth flow I mentioned implementing?",
    "Bruno learned a new trick - roll over.",
    "My home office setup includes dual monitors.",
    "Summarize everything you know about me."
]

# Simple correctness rules for question turns (substring matching, case-insensitive)
# Each rule can specify required substrings in "all" and optional substrings in "any".
EVAL_RULES = {
    5: {"all": ["paris"]},
    8: {"all": ["hash", "key"]},
    10: {"all": ["bruno"]},
    13: {"all": ["rust"]},
    16: {"all": ["tcp", "ip"]},
    17: {"all": ["march", "15"]},
    20: {"all": ["techcorp"]},
    23: {"all": ["photosynthesis", "plant"]},
    24: {"all": ["sarah"]},
    27: {"all": ["sql", "nosql"]},
    28: {"all": ["seattle"]},
    31: {"all": ["emma", "doctor"]},
    34: {"all": ["golden", "retriever"]},
    36: {"all": ["garbage", "memory"]},
    37: {"all": ["japan", "april"]},
    40: {"all": ["alex", "engineer"]},
    43: {"all": ["transformer", "attention"]},
    44: {"all": ["weekly", "friday"]},
    47: {"all": ["oauth", "pkce"]},
    50: {"all": ["alex", "techcorp", "bruno", "emma"]}
}


def evaluate_correctness(turn_index, response_text):
    rule = EVAL_RULES.get(turn_index)
    if not rule:
        return None
    text = (response_text or "").lower()
    required = rule.get("all", [])
    if required and not all(r in text for r in required):
        return False
    any_required = rule.get("any", [])
    if any_required and not any(r in text for r in any_required):
        return False
    return True


def get_gpu_power():
    """Get current GPU power draw in watts."""
    try:
        result = subprocess.run(
            ["ssh", "xx@192.168.0.165", "nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=5
        )
        return float(result.stdout.strip())
    except:
        return 0.0


class PowerSampler:
    """Sample GPU power over time to estimate energy more accurately."""
    def __init__(self, interval_s=POWER_SAMPLE_INTERVAL_S):
        self.interval_s = interval_s
        self.samples = []
        self._stop = threading.Event()
        self._thread = None

    def _run(self):
        while not self._stop.is_set():
            p = get_gpu_power()
            if p > 0:
                self.samples.append((time.time(), p))
            time.sleep(self.interval_s)

    def start(self):
        self.samples = []
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=2)

    def energy_ws(self):
        # Integrate power over time (trapezoid) to estimate energy in Ws
        if len(self.samples) < 2:
            return 0.0, 0.0, 0.0
        total = 0.0
        for i in range(1, len(self.samples)):
            t0, p0 = self.samples[i - 1]
            t1, p1 = self.samples[i]
            dt = max(0.0, t1 - t0)
            total += (p0 + p1) * 0.5 * dt
        avg_power = total / max(self.samples[-1][0] - self.samples[0][0], 1e-6)
        peak_power = max(p for _, p in self.samples)
        return total, avg_power, peak_power


def send_query_zeta(prompt, turn_num, timeout=60):
    """Send fresh query to ZETA with graph memory retrieval.

    Uses /gkv/stats delta to measure GKV injections since the /v1/chat/completions
    endpoint doesn't expose per-request GKV metrics.
    """
    # Snapshot GKV stats BEFORE request
    gkv_before = get_gkv_stats()
    health_before = get_health()
    
    power_sampler = PowerSampler()
    power_sampler.start()
    start = time.time()
    
    try:
        resp = requests.post(
            f"{ZETA_URL}/v1/chat/completions",
            json={
                "model": "zeta",
                "messages": [{"role": "user", "content": prompt}],
                "max_tokens": 150
            },
            timeout=timeout
        )
        elapsed = time.time() - start
        power_sampler.stop()
        energy_ws, avg_power, peak_power = power_sampler.energy_ws()
        
        # Snapshot GKV stats AFTER request
        gkv_after = get_gkv_stats()
        health_after = get_health()
        
        # Calculate deltas
        delta_injections = gkv_after.get("injections", 0) - gkv_before.get("injections", 0)
        delta_prefill_saved = gkv_after.get("prefill_saved_sec", 0) - gkv_before.get("prefill_saved_sec", 0)
        delta_nodes = health_after.get("graph_nodes", 0) - health_before.get("graph_nodes", 0)
        delta_edges = health_after.get("graph_edges", 0) - health_before.get("graph_edges", 0)
        
        if resp.status_code == 200:
            data = resp.json()
            usage = data.get("usage", {})
            choices = data.get("choices", [])
            response_text = ""
            if choices:
                response_text = choices[0].get("message", {}).get("content", "")
            correct = evaluate_correctness(turn_num, response_text)
            return {
                "success": True,
                "time_s": round(elapsed, 3),
                "peak_w": round(peak_power, 2),
                "avg_w": round(avg_power, 2),
                "energy_ws": round(energy_ws, 2),
                "prompt_tokens": usage.get("prompt_tokens", 0),
                "completion_tokens": usage.get("completion_tokens", 0),
                "total_tokens": usage.get("total_tokens", 0),
                "graph_nodes_before": health_before.get("graph_nodes", 0),
                "graph_nodes_after": health_after.get("graph_nodes", 0),
                "graph_nodes_delta": delta_nodes,
                "graph_edges_before": health_before.get("graph_edges", 0),
                "graph_edges_after": health_after.get("graph_edges", 0),
                "graph_edges_delta": delta_edges,
                # GKV metrics from delta measurement
                "kv_injected": delta_injections > 0,
                "gkv_injections": delta_injections,
                "gkv_prefill_saved_ms": round(delta_prefill_saved * 1000, 1),
                "response_preview": response_text[:100],
                "is_question": turn_num in EVAL_RULES,
                "correct": correct
            }
        return {"success": False, "time_s": elapsed, "error": resp.text[:100],
                "kv_injected": False, "gkv_injections": 0, "gkv_prefill_saved_ms": 0,
                "http_status": resp.status_code}
    except Exception as e:
        power_sampler.stop()
        return {"success": False, "time_s": time.time() - start, "error": str(e),
                "kv_injected": False, "gkv_injections": 0, "gkv_prefill_saved_ms": 0}


def run_benchmark():
    print("=" * 70)
    print("Z.E.T.A. v5.1 - 50 Turn GKV Benchmark (v2 - Delta Method)")
    print("=" * 70)
    print(f"Date: {datetime.now().isoformat()}")
    print(f"Server: {ZETA_URL}")
    print()
    
    # Check health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"Health: {json.dumps(health, indent=2)}")
    except Exception as e:
        print(f"Server not responding: {e}")
        return
    
    # Check GKV status
    gkv_initial = get_gkv_stats()
    print(f"\nGKV Status: {json.dumps(gkv_initial, indent=2)}")
    if not gkv_initial.get("enabled"):
        print("⚠️  WARNING: GKV not enabled on server!")
    
    print("\n" + "-" * 70)
    print("Running 50-turn ZETA benchmark with GKV delta measurement...")
    print("-" * 70 + "\n")
    
    zeta_results = []
    total_energy_ws = 0  # Watt-seconds (Joules)
    total_graph_nodes = 0
    total_graph_edges = 0
    
    for i, turn in enumerate(CONVERSATION_TURNS, 1):
        result = send_query_zeta(turn, i)
        
        # Estimate energy: avg_power * time
        if result["success"]:
            energy_ws = result.get("energy_ws", 0)
            total_energy_ws += energy_ws
            total_graph_nodes += result.get("graph_nodes_delta", 0)
            total_graph_edges += result.get("graph_edges_delta", 0)
        else:
            energy_ws = 0
        
        result["query"] = i
        result["energy_ws"] = round(energy_ws, 2)
        zeta_results.append(result)
        
        # Show GKV injection count if any
        inj = result.get("gkv_injections", 0)
        saved_ms = result.get("gkv_prefill_saved_ms", 0)
        graph_delta = result.get("graph_nodes_delta", 0)
        if inj > 0:
            gkv_marker = f"⚡GKV×{inj} ({saved_ms:.0f}ms)"
        else:
            gkv_marker = "       "
        status = "✓" if result["success"] else "✗"
        print(f"[{i:2d}] {gkv_marker:18s} ΔG={graph_delta:+d} {result['time_s']:.2f}s {result.get('peak_w', 0):.0f}W {status} - {turn[:35]}...")
        
        # No pause - keep requests back-to-back to prevent dream thread interference
        # time.sleep(0.3)
    
    # Calculate stats
    results_for_stats = [r for r in zeta_results if r["success"]]
    if WARMUP_TURNS > 0 and len(results_for_stats) > WARMUP_TURNS:
        results_for_stats = results_for_stats[WARMUP_TURNS:]

    times = [r["time_s"] for r in results_for_stats]
    powers = [r["peak_w"] for r in results_for_stats]
    totals = [r.get("total_tokens", 0) for r in results_for_stats]
    completions = [r.get("completion_tokens", 0) for r in results_for_stats]
    prompts = [r.get("prompt_tokens", 0) for r in results_for_stats]
    
    # Correctness stats for question turns
    question_results = [r for r in zeta_results if r.get("is_question")]
    correct_results = [r for r in question_results if r.get("correct") is True]
    correctness_rate = (len(correct_results) / len(question_results)) if question_results else 0.0

    # GKV stats - now using delta measurement
    gkv_hits = sum(1 for r in zeta_results if r.get("kv_injected"))
    total_injections = sum(r.get("gkv_injections", 0) for r in zeta_results)
    total_prefill_saved_ms = sum(r.get("gkv_prefill_saved_ms", 0) for r in zeta_results)
    
    # Get final GKV stats
    gkv_final = get_gkv_stats()
    
    print("\n" + "=" * 70)
    print("BENCHMARK RESULTS")
    print("=" * 70)
    
    print("\n📊 LATENCY:")
    print(f"   Average: {sum(times)/len(times):.2f}s")
    if len(times) >= 5:
        print(f"   Median:  {statistics.median(times):.2f}s")
        print(f"   P95:     {statistics.quantiles(times, n=100)[94]:.2f}s")
    print(f"   Min:     {min(times):.2f}s")
    print(f"   Max:     {max(times):.2f}s")
    print(f"   Total:   {sum(times):.1f}s")
    
    print("\n⚡ ENERGY:")
    print(f"   Peak power:   {max(powers):.0f}W")
    print(f"   Avg power:    {sum(powers)/len(powers):.0f}W")
    print(f"   Total energy: {total_energy_ws:.1f} Ws ({total_energy_ws/3600:.3f} Wh)")
    if sum(totals) > 0:
        print(f"   Energy/token: {total_energy_ws/sum(totals):.3f} Ws/token")
        print(f"   Tokens/sec:   {sum(totals)/sum(times):.2f} t/s")
        print(f"   Completion t/s: {sum(completions)/sum(times):.2f} t/s")

    print("\n✅ CORRECTNESS:")
    print(f"   Questions: {len(question_results)}")
    print(f"   Correct:   {len(correct_results)}")
    print(f"   Accuracy:  {correctness_rate:.2%}")

    print("\n🧱 GRAPH EXPANSION:")
    print(f"   New nodes:    {total_graph_nodes}")
    print(f"   New edges:    {total_graph_edges}")
    
    print("\n🧠 GKV INJECTION (Delta Method):")
    print(f"   Requests with GKV hits: {gkv_hits}/{len(zeta_results)} ({100*gkv_hits/len(zeta_results):.0f}%)")
    print(f"   Total injections:       {total_injections}")
    print(f"   Prefill time saved:     {total_prefill_saved_ms:.1f}ms")
    if total_injections > 0:
        print(f"   Avg saved/injection:    {total_prefill_saved_ms/total_injections:.2f}ms")
    print(f"   Server total injections: {gkv_final.get('injections', 0)}")
    print(f"   Server total saved:      {gkv_final.get('prefill_saved_sec', 0)*1000:.1f}ms")
    
    # Compare with baseline from original benchmark
    original_total_time = 243.7  # Growing context
    original_zeta_time = 102.5   # Fresh query (no GKV)
    
    print("\n📈 COMPARISON vs BASELINE:")
    print(f"   vs Growing Context (243.7s): {original_total_time/sum(times):.1f}x faster")
    print(f"   vs Original ZETA (102.5s):   {100*(original_zeta_time-sum(times))/original_zeta_time:.0f}% improvement")
    
    # Save results
    output = {
        "benchmark": "50-Turn GKV Benchmark",
        "version": "5.1",
        "measurement_method": "gkv_stats_delta",
        "date": datetime.now().isoformat(),
        "hardware": "RTX 5060 Ti 16GB",
        "model": "Qwen2.5-14B Q4_K_M",
        "stats": {
            "total_turns": len(zeta_results),
            "successful": len(times),
            "avg_time_s": round(sum(times)/len(times), 3),
            "min_time_s": round(min(times), 3),
            "max_time_s": round(max(times), 3),
            "p50_time_s": round(statistics.median(times), 3) if len(times) >= 1 else 0,
            "p95_time_s": round(statistics.quantiles(times, n=100)[94], 3) if len(times) >= 5 else 0,
            "total_time_s": round(sum(times), 1),
            "total_energy_ws": round(total_energy_ws, 1),
            "total_energy_wh": round(total_energy_ws/3600, 4),
            "avg_power_w": round(sum(powers)/len(powers), 1),
            "peak_power_w": round(max(powers), 1),
            "total_tokens": int(sum(totals)),
            "total_prompt_tokens": int(sum(prompts)),
            "total_completion_tokens": int(sum(completions)),
            "tokens_per_s": round(sum(totals)/sum(times), 2) if sum(times) > 0 else 0,
            "completion_tokens_per_s": round(sum(completions)/sum(times), 2) if sum(times) > 0 else 0,
            "energy_per_token_ws": round(total_energy_ws/max(sum(totals), 1), 4),
            "questions": len(question_results),
            "correct": len(correct_results),
            "accuracy": round(correctness_rate, 4),
            "graph_nodes_added": total_graph_nodes,
            "graph_edges_added": total_graph_edges,
            # GKV metrics from delta measurement
            "gkv_requests_with_hits": gkv_hits,
            "gkv_hit_rate": round(gkv_hits/len(zeta_results), 2),
            "gkv_total_injections": total_injections,
            "gkv_prefill_saved_ms": round(total_prefill_saved_ms, 1),
            "gkv_avg_saved_per_injection_ms": round(total_prefill_saved_ms/max(total_injections, 1), 2),
            "gkv_server_total_injections": gkv_final.get("injections", 0),
            "gkv_server_total_saved_ms": round(gkv_final.get("prefill_saved_sec", 0) * 1000, 1)
        },
        "comparison": {
            "vs_growing_context_speedup": round(original_total_time/sum(times), 2),
            "vs_original_zeta_improvement_pct": round(100*(original_zeta_time-sum(times))/original_zeta_time, 1)
        },
        "results": zeta_results
    }
    
    outfile = "50_turn_gkv_results.json"
    with open(outfile, "w") as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Results saved to {outfile}")


if __name__ == "__main__":
    run_benchmark()
