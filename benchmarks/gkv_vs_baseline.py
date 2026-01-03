#!/usr/bin/env python3
"""
GKV vs Baseline Benchmark
========================
A/B comparison: Z.E.T.A. with GKV vs vanilla llama.cpp
Measures: latency, energy consumption (kJ), GKV acceleration

Hypothesis: ZETA should be ~11x faster with half the cumulative energy
"""

import requests
import json
import time
import subprocess
import sys
from datetime import datetime

# Server configs
ZETA_URL = "http://192.168.0.165:8080"
BASELINE_URL = "http://192.168.0.165:9090"  # Vanilla llama-server on different port
REMOTE_HOST = "xx@192.168.0.165"

# Test queries - mix of recall and knowledge
TEST_QUERIES = [
    "My name is Alex and I work at TechCorp as a senior engineer.",
    "I have a golden retriever named Bruno who is 3 years old.",
    "What is the capital of France?",
    "What is my dogs name?",
    "Can you explain how hash tables work?",
    "What company do I work for?",
    "Explain the difference between SQL and NoSQL databases.",
    "My wife Emma is a doctor at Swedish Hospital.",
    "What is my wifes name and profession?",
    "How does garbage collection work in Java?",
    "Summarize everything you know about me.",
]


def ssh(cmd):
    """Run command on remote host"""
    result = subprocess.run(
        ["ssh", REMOTE_HOST, cmd],
        capture_output=True, text=True, timeout=30
    )
    return result.stdout.strip(), result.returncode


def get_gpu_power():
    """Get current GPU power draw in watts"""
    try:
        out, _ = ssh("nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits")
        return float(out.strip())
    except:
        return 100.0  # Default estimate


def get_gkv_stats():
    """Get ZETA GKV statistics"""
    try:
        r = requests.get(f"{ZETA_URL}/gkv/stats", timeout=5)
        return r.json() if r.status_code == 200 else {}
    except:
        return {}


def start_baseline_server():
    """Start vanilla llama-server on port 9090"""
    print("Starting baseline llama-server on port 9090...")
    
    # Kill any existing baseline server
    ssh("pkill -f 'llama-server.*9090' 2>/dev/null || true")
    time.sleep(1)
    
    # Start vanilla llama-server
    cmd = (
        "cd ~/ZetaZero/llama.cpp/build/bin && "
        "nohup ./llama-server "
        "-m ~/models/qwen2.5-14b-instruct-q4.gguf "
        "--port 9090 --host 0.0.0.0 "
        "-ngl 999 -c 4096 "
        "> ~/baseline_server.log 2>&1 &"
    )
    ssh(cmd)
    
    # Wait for startup
    print("Waiting for baseline server to initialize...")
    for i in range(60):
        time.sleep(2)
        try:
            r = requests.get(f"{BASELINE_URL}/health", timeout=3)
            if r.status_code == 200:
                print(f"✓ Baseline server ready after {(i+1)*2}s")
                return True
        except:
            pass
        print(f"  [{i+1}/60] Waiting...")
    
    print("✗ Baseline server failed to start")
    return False


def stop_baseline_server():
    """Stop the baseline server"""
    ssh("pkill -f 'llama-server.*9090' 2>/dev/null || true")


def query_zeta(prompt, max_tokens=150):
    """Query ZETA server with GKV"""
    gkv_before = get_gkv_stats()
    power_start = get_gpu_power()
    start = time.time()
    
    try:
        r = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": prompt, "max_tokens": max_tokens},
            timeout=120
        )
        elapsed = time.time() - start
        power_end = get_gpu_power()
        gkv_after = get_gkv_stats()
        
        if r.status_code == 200:
            data = r.json()
            gkv_inj = gkv_after.get("injections", 0) - gkv_before.get("injections", 0)
            gkv_saved = (gkv_after.get("prefill_saved_sec", 0) - gkv_before.get("prefill_saved_sec", 0)) * 1000
            
            return {
                "success": True,
                "time_s": elapsed,
                "avg_power_w": (power_start + power_end) / 2,
                "energy_j": elapsed * (power_start + power_end) / 2,
                "tokens": data.get("tokens", 0),
                "gkv_injections": gkv_inj,
                "gkv_saved_ms": gkv_saved,
                "response": data.get("output", "")[:100]
            }
    except Exception as e:
        elapsed = time.time() - start
        return {"success": False, "time_s": elapsed, "error": str(e)}
    
    return {"success": False, "time_s": elapsed, "error": "Request failed"}


def query_baseline(prompt, max_tokens=150):
    """Query vanilla llama-server"""
    power_start = get_gpu_power()
    start = time.time()
    
    try:
        # Use OpenAI-compatible endpoint
        r = requests.post(
            f"{BASELINE_URL}/v1/chat/completions",
            json={
                "model": "qwen2.5-14b",
                "messages": [{"role": "user", "content": prompt}],
                "max_tokens": max_tokens,
                "temperature": 0.7
            },
            timeout=120
        )
        elapsed = time.time() - start
        power_end = get_gpu_power()
        
        if r.status_code == 200:
            data = r.json()
            tokens = data.get("usage", {}).get("completion_tokens", 0)
            content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
            
            return {
                "success": True,
                "time_s": elapsed,
                "avg_power_w": (power_start + power_end) / 2,
                "energy_j": elapsed * (power_start + power_end) / 2,
                "tokens": tokens,
                "response": content[:100]
            }
    except Exception as e:
        elapsed = time.time() - start
        return {"success": False, "time_s": elapsed, "error": str(e)}
    
    return {"success": False, "time_s": elapsed, "error": "Request failed"}


def run_benchmark():
    print("=" * 70)
    print("Z.E.T.A. GKV vs Vanilla llama.cpp Baseline")
    print("=" * 70)
    print(f"Date: {datetime.now().isoformat()}")
    print(f"Model: Qwen2.5-14B-Instruct Q4_K_M")
    print(f"Queries: {len(TEST_QUERIES)}")
    print()
    
    # Check ZETA health
    print("[1] Checking ZETA server...")
    try:
        r = requests.get(f"{ZETA_URL}/health", timeout=5)
        print(f"    ZETA: OK - {r.json()}")
    except Exception as e:
        print(f"    ZETA: FAILED - {e}")
        return
    
    gkv = get_gkv_stats()
    print(f"    GKV: enabled={gkv.get('enabled')}, injections={gkv.get('injections')}")
    
    # Start baseline server
    print("\n[2] Starting baseline server...")
    if not start_baseline_server():
        print("    Failed to start baseline. Aborting.")
        return
    
    # Run ZETA benchmark
    print("\n[3] Running ZETA benchmark...")
    print("-" * 70)
    zeta_results = []
    for i, query in enumerate(TEST_QUERIES, 1):
        result = query_zeta(query)
        result["query_num"] = i
        zeta_results.append(result)
        
        gkv_mark = f"⚡×{result.get('gkv_injections', 0)}" if result.get("gkv_injections", 0) > 0 else "   "
        status = "✓" if result["success"] else "✗"
        print(f"  [{i:2d}] ZETA  {gkv_mark:6s} {result['time_s']:.2f}s {result.get('avg_power_w', 0):.0f}W {status}")
        time.sleep(0.3)
    
    # Run baseline benchmark  
    print("\n[4] Running baseline benchmark...")
    print("-" * 70)
    baseline_results = []
    for i, query in enumerate(TEST_QUERIES, 1):
        result = query_baseline(query)
        result["query_num"] = i
        baseline_results.append(result)
        
        status = "✓" if result["success"] else "✗"
        print(f"  [{i:2d}] BASE        {result['time_s']:.2f}s {result.get('avg_power_w', 0):.0f}W {status}")
        time.sleep(0.3)
    
    # Stop baseline server
    print("\n[5] Stopping baseline server...")
    stop_baseline_server()
    
    # Calculate results
    print("\n" + "=" * 70)
    print("RESULTS")
    print("=" * 70)
    
    zeta_times = [r["time_s"] for r in zeta_results if r["success"]]
    zeta_energy = [r.get("energy_j", 0) for r in zeta_results if r["success"]]
    zeta_gkv = sum(r.get("gkv_injections", 0) for r in zeta_results)
    
    base_times = [r["time_s"] for r in baseline_results if r["success"]]
    base_energy = [r.get("energy_j", 0) for r in baseline_results if r["success"]]
    
    print("\n📊 LATENCY:")
    print(f"   ZETA Total:     {sum(zeta_times):.1f}s (avg {sum(zeta_times)/len(zeta_times):.2f}s)")
    print(f"   Baseline Total: {sum(base_times):.1f}s (avg {sum(base_times)/len(base_times):.2f}s)")
    if sum(zeta_times) > 0:
        speedup = sum(base_times) / sum(zeta_times)
        print(f"   SPEEDUP:        {speedup:.1f}x {'✓' if speedup >= 10 else '⚠️'}")
    
    print("\n⚡ ENERGY:")
    zeta_kj = sum(zeta_energy) / 1000
    base_kj = sum(base_energy) / 1000
    print(f"   ZETA Total:     {zeta_kj:.3f} kJ")
    print(f"   Baseline Total: {base_kj:.3f} kJ")
    if zeta_kj > 0:
        energy_ratio = base_kj / zeta_kj
        print(f"   REDUCTION:      {energy_ratio:.1f}x (ZETA uses {100/energy_ratio:.0f}% of baseline)")
    
    print("\n🧠 GKV:")
    print(f"   Total injections: {zeta_gkv}")
    print(f"   Avg per query:    {zeta_gkv/len(TEST_QUERIES):.1f}")
    
    # Save results
    output = {
        "benchmark": "GKV vs Baseline",
        "date": datetime.now().isoformat(),
        "model": "Qwen2.5-14B-Instruct Q4_K_M",
        "num_queries": len(TEST_QUERIES),
        "zeta": {
            "total_time_s": round(sum(zeta_times), 2),
            "avg_time_s": round(sum(zeta_times)/len(zeta_times), 3),
            "total_energy_kj": round(zeta_kj, 4),
            "gkv_injections": zeta_gkv,
            "results": zeta_results
        },
        "baseline": {
            "total_time_s": round(sum(base_times), 2),
            "avg_time_s": round(sum(base_times)/len(base_times), 3),
            "total_energy_kj": round(base_kj, 4),
            "results": baseline_results
        },
        "comparison": {
            "speedup_x": round(sum(base_times) / sum(zeta_times), 2) if sum(zeta_times) > 0 else 0,
            "energy_reduction_x": round(base_kj / zeta_kj, 2) if zeta_kj > 0 else 0
        }
    }
    
    outfile = "gkv_vs_baseline_results.json"
    with open(outfile, "w") as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Results saved to {outfile}")
    
    # Final verdict
    print("\n" + "=" * 70)
    speedup = sum(base_times) / sum(zeta_times) if sum(zeta_times) > 0 else 0
    energy_ratio = base_kj / zeta_kj if zeta_kj > 0 else 0
    
    if speedup >= 10 and energy_ratio >= 2:
        print("🎉 HYPOTHESIS CONFIRMED: ≥10x faster, ≥2x energy efficient")
    else:
        print(f"⚠️  Results: {speedup:.1f}x speed, {energy_ratio:.1f}x energy")
        print("   (Target: 11x faster, 2x energy reduction)")
    print("=" * 70)


if __name__ == "__main__":
    run_benchmark()
