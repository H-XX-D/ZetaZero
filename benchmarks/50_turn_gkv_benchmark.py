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
from datetime import datetime

ZETA_URL = "http://192.168.0.165:8080"


def get_gkv_stats():
    """Get current GKV statistics from server."""
    try:
        resp = requests.get(f"{ZETA_URL}/gkv/stats", timeout=5)
        if resp.status_code == 200:
            return resp.json()
    except:
        pass
    return {"injections": 0, "prefill_saved_sec": 0.0, "enabled": False}

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


def send_query_zeta(prompt, turn_num, timeout=60):
    """Send fresh query to ZETA with graph memory retrieval.
    
    Uses /gkv/stats delta to measure GKV injections since the /generate
    endpoint doesn't expose per-request GKV metrics.
    """
    # Snapshot GKV stats BEFORE request
    gkv_before = get_gkv_stats()
    
    start = time.time()
    power_start = get_gpu_power()
    
    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 150
            },
            timeout=timeout
        )
        elapsed = time.time() - start
        power_end = get_gpu_power()
        
        # Snapshot GKV stats AFTER request
        gkv_after = get_gkv_stats()
        
        # Calculate deltas
        delta_injections = gkv_after.get("injections", 0) - gkv_before.get("injections", 0)
        delta_prefill_saved = gkv_after.get("prefill_saved_sec", 0) - gkv_before.get("prefill_saved_sec", 0)
        
        if resp.status_code == 200:
            data = resp.json()
            return {
                "success": True,
                "time_s": round(elapsed, 3),
                "peak_w": round(max(power_start, power_end), 2),
                "avg_w": round((power_start + power_end) / 2, 2),
                "tokens": data.get("tokens", 0),
                "graph_nodes": data.get("graph_nodes", 0),
                # GKV metrics from delta measurement
                "kv_injected": delta_injections > 0,
                "gkv_injections": delta_injections,
                "gkv_prefill_saved_ms": round(delta_prefill_saved * 1000, 1),
                "response_preview": data.get("output", "")[:100]
            }
        return {"success": False, "time_s": elapsed, "error": resp.text[:100], 
                "kv_injected": False, "gkv_injections": 0, "gkv_prefill_saved_ms": 0}
    except Exception as e:
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
    
    for i, turn in enumerate(CONVERSATION_TURNS, 1):
        result = send_query_zeta(turn, i)
        
        # Estimate energy: avg_power * time
        if result["success"]:
            energy_ws = result["avg_w"] * result["time_s"]
            total_energy_ws += energy_ws
        else:
            energy_ws = 0
        
        result["query"] = i
        result["energy_ws"] = round(energy_ws, 2)
        zeta_results.append(result)
        
        # Show GKV injection count if any
        inj = result.get("gkv_injections", 0)
        saved_ms = result.get("gkv_prefill_saved_ms", 0)
        if inj > 0:
            gkv_marker = f"⚡GKV×{inj} ({saved_ms:.0f}ms)"
        else:
            gkv_marker = "       "
        status = "✓" if result["success"] else "✗"
        print(f"[{i:2d}] {gkv_marker:18s} {result['time_s']:.2f}s {result.get('peak_w', 0):.0f}W {status} - {turn[:35]}...")
        
        # Brief pause
        time.sleep(0.3)
    
    # Calculate stats
    times = [r["time_s"] for r in zeta_results if r["success"]]
    powers = [r["peak_w"] for r in zeta_results if r["success"]]
    
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
    print(f"   Min:     {min(times):.2f}s")
    print(f"   Max:     {max(times):.2f}s")
    print(f"   Total:   {sum(times):.1f}s")
    
    print("\n⚡ ENERGY:")
    print(f"   Peak power:   {max(powers):.0f}W")
    print(f"   Avg power:    {sum(powers)/len(powers):.0f}W")
    print(f"   Total energy: {total_energy_ws:.1f} Ws ({total_energy_ws/3600:.3f} Wh)")
    
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
            "total_time_s": round(sum(times), 1),
            "total_energy_ws": round(total_energy_ws, 1),
            "total_energy_wh": round(total_energy_ws/3600, 4),
            "avg_power_w": round(sum(powers)/len(powers), 1),
            "peak_power_w": round(max(powers), 1),
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
