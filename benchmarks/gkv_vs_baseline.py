#!/usr/bin/env python3
"""
GKV vs Baseline Benchmark (Sequential Mode)
============================================
A/B comparison: Z.E.T.A. with GKV vs vanilla llama.cpp
Runs servers SEQUENTIALLY to fit on single GPU (16GB RTX 5060 Ti)

Measures:
- Latency (total, avg, p50, p95, p99)
- Energy consumption (Joules, kJ)
- Tokens/second (throughput)
- GKV acceleration (injections, prefill saved)
- GPU utilization & memory
- Time to first token (TTFT)

Hypothesis: ZETA should be faster with better energy efficiency
"""

import requests
import json
import time
import subprocess
import sys
import statistics
import threading
from datetime import datetime
from typing import Dict, List, Any

# Server configs
ZETA_URL = "http://192.168.0.165:8080"
BASELINE_URL = "http://192.168.0.165:8080"  # Same port - sequential execution
REMOTE_HOST = "xx@192.168.0.165"
POWER_SAMPLE_INTERVAL_S = 0.25

# Test queries - mix of recall and knowledge
TEST_QUERIES = [
    # Personal facts (should benefit from GKV)
    "My name is Alex and I work at TechCorp as a senior engineer.",
    "I have a golden retriever named Bruno who is 3 years old.",
    "My wife Emma is a doctor at Swedish Hospital.",
    "My project deadline is March 15th for the authentication module.",
    "I live in Seattle and commute by bike.",
    
    # Recall queries (should hit GKV cache)
    "What is my dogs name?",
    "What company do I work for?",
    "What is my wifes name and profession?",
    "When is my project deadline?",
    
    # Knowledge queries (baseline should be similar)
    "What is the capital of France?",
    "Can you explain how hash tables work?",
    "Explain the difference between SQL and NoSQL databases.",
    "How does garbage collection work in Java?",
    "What is the difference between TCP and UDP?",
    
    # Summary query (should benefit most from GKV)
    "Summarize everything you know about me.",
]


def ssh(cmd, timeout=60):
    """Run command on remote host"""
    try:
        result = subprocess.run(
            ["ssh", REMOTE_HOST, cmd],
            capture_output=True, text=True, timeout=timeout
        )
        return result.stdout.strip(), result.returncode
    except subprocess.TimeoutExpired:
        return "", 1


def kill_all_servers():
    """Kill any running LLM servers and clear GPU"""
    print("  Killing any running servers...")
    ssh("pkill -9 -x zeta-zero-server 2>/dev/null || true")
    ssh("pkill -9 -x llama-server 2>/dev/null || true")
    ssh("pkill -9 -f 'llama.*server' 2>/dev/null || true")
    time.sleep(2)
    # Force clear GPU memory
    out, _ = ssh("nvidia-smi --query-compute-apps=pid --format=csv,noheader")
    if out.strip():
        for pid in out.strip().split('\n'):
            if pid.strip():
                ssh(f"kill -9 {pid.strip()} 2>/dev/null || true")
    time.sleep(2)


def get_gpu_stats():
    """Get comprehensive GPU stats"""
    try:
        out, _ = ssh("nvidia-smi --query-gpu=power.draw,temperature.gpu,memory.used,memory.total,utilization.gpu --format=csv,noheader,nounits")
        parts = [p.strip() for p in out.split(',')]
        return {
            "power_w": float(parts[0]),
            "temp_c": float(parts[1]),
            "mem_used_mb": float(parts[2]),
            "mem_total_mb": float(parts[3]),
            "utilization_pct": float(parts[4])
        }
    except:
        return {"power_w": 100.0, "temp_c": 50, "mem_used_mb": 0, "mem_total_mb": 16000, "utilization_pct": 0}


class PowerSampler:
    """Sample GPU power over time for energy integration."""
    def __init__(self, interval_s=POWER_SAMPLE_INTERVAL_S):
        self.interval_s = interval_s
        self.samples = []
        self._stop = threading.Event()
        self._thread = None

    def _run(self):
        while not self._stop.is_set():
            stats = get_gpu_stats()
            p = stats.get("power_w", 0)
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

    def energy_j(self):
        # Integrate power over time (trapezoid rule) -> Joules
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


def get_gkv_stats():
    """Get ZETA GKV statistics"""
    try:
        r = requests.get(f"{ZETA_URL}/gkv/stats", timeout=5)
        return r.json() if r.status_code == 200 else {}
    except:
        return {}


def start_zeta_server():
    """Start ZETA server"""
    print("  Starting ZETA server...")
    kill_all_servers()
    
    cmd = (
        "cd ~/ZetaZero && "
        "nohup ./llama.cpp/build/bin/zeta-zero-server "
        "> ~/ZetaZero/logs/zeta_benchmark.log 2>&1 &"
    )
    ssh(cmd)
    
    print("  Waiting for ZETA server...")
    for i in range(90):
        time.sleep(1)
        try:
            r = requests.get(f"{ZETA_URL}/health", timeout=3)
            if r.status_code == 200:
                data = r.json()
                print(f"  ✓ ZETA ready after {i+1}s (nodes: {data.get('graph_nodes', 0)})")
                return True
        except:
            pass
        if (i + 1) % 10 == 0:
            print(f"    [{i+1}/90] Waiting...")
    
    print("  ✗ ZETA failed to start")
    return False


def start_baseline_server():
    """Start vanilla llama-server"""
    print("  Starting baseline llama-server...")
    kill_all_servers()
    
    cmd = (
        "cd ~/ZetaZero/llama.cpp/build/bin && "
        "nohup ./llama-server "
        "-m ~/models/qwen2.5-14b-instruct-q4_k_m.gguf "
        "--port 8080 --host 0.0.0.0 "
        "-ngl 999 -c 4096 "
        "> ~/baseline_server.log 2>&1 &"
    )
    ssh(cmd)
    
    print("  Waiting for baseline server...")
    for i in range(90):
        time.sleep(1)
        try:
            r = requests.get(f"{BASELINE_URL}/health", timeout=3)
            if r.status_code == 200:
                print(f"  ✓ Baseline ready after {i+1}s")
                return True
        except:
            pass
        if (i + 1) % 10 == 0:
            print(f"    [{i+1}/90] Waiting...")
    
    print("  ✗ Baseline failed to start")
    return False


def query_zeta(prompt, max_tokens=150) -> Dict[str, Any]:
    """Query ZETA server using OpenAI-compatible endpoint for accurate metrics"""
    gkv_before = get_gkv_stats()
    gpu_before = get_gpu_stats()
    sampler = PowerSampler()
    sampler.start()
    start = time.time()
    
    try:
        # Use OpenAI-compatible endpoint for accurate token counting
        # This still uses ZETA's graph memory via the same backend
        r = requests.post(
            f"{ZETA_URL}/v1/chat/completions",
            json={
                "model": "zeta",
                "messages": [{"role": "user", "content": prompt}],
                "max_tokens": max_tokens,
                "temperature": 0.7
            },
            timeout=120
        )
        elapsed = time.time() - start
        sampler.stop()
        gpu_after = get_gpu_stats()
        gkv_after = get_gkv_stats()
        energy_j, avg_power, peak_power = sampler.energy_j()
        
        if r.status_code == 200:
            data = r.json()
            gkv_inj = gkv_after.get("injections", 0) - gkv_before.get("injections", 0)
            gkv_saved = (gkv_after.get("prefill_saved_sec", 0) - gkv_before.get("prefill_saved_sec", 0)) * 1000
            
            usage = data.get("usage", {})
            tokens = usage.get("completion_tokens", 0)
            prompt_tokens = usage.get("prompt_tokens", 0)
            content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
            
            return {
                "success": True,
                "time_s": elapsed,
                "tokens": tokens,
                "prompt_tokens": prompt_tokens,
                "tokens_per_sec": tokens / elapsed if elapsed > 0 and tokens > 0 else 0,
                "power_avg_w": avg_power if avg_power > 0 else (gpu_before["power_w"] + gpu_after["power_w"]) / 2,
                "power_peak_w": peak_power if peak_power > 0 else max(gpu_before["power_w"], gpu_after["power_w"]),
                "energy_j": energy_j if energy_j > 0 else elapsed * (gpu_before["power_w"] + gpu_after["power_w"]) / 2,
                "gpu_mem_mb": gpu_after["mem_used_mb"],
                "gpu_util_pct": (gpu_before["utilization_pct"] + gpu_after["utilization_pct"]) / 2,
                "gkv_injections": gkv_inj,
                "gkv_saved_ms": gkv_saved,
                "response_preview": content[:100]
            }
    except Exception as e:
        elapsed = time.time() - start
        sampler.stop()
        return {"success": False, "time_s": elapsed, "error": str(e)}
    
    return {"success": False, "time_s": elapsed, "error": "Request failed"}


def query_baseline(prompt, max_tokens=150) -> Dict[str, Any]:
    """Query vanilla llama-server with detailed metrics"""
    gpu_before = get_gpu_stats()
    sampler = PowerSampler()
    sampler.start()
    start = time.time()
    
    try:
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
        sampler.stop()
        gpu_after = get_gpu_stats()
        energy_j, avg_power, peak_power = sampler.energy_j()
        
        if r.status_code == 200:
            data = r.json()
            tokens = data.get("usage", {}).get("completion_tokens", 0)
            content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
            
            return {
                "success": True,
                "time_s": elapsed,
                "tokens": tokens,
                "tokens_per_sec": tokens / elapsed if elapsed > 0 else 0,
                "power_avg_w": avg_power if avg_power > 0 else (gpu_before["power_w"] + gpu_after["power_w"]) / 2,
                "power_peak_w": peak_power if peak_power > 0 else max(gpu_before["power_w"], gpu_after["power_w"]),
                "energy_j": energy_j if energy_j > 0 else elapsed * (gpu_before["power_w"] + gpu_after["power_w"]) / 2,
                "gpu_mem_mb": gpu_after["mem_used_mb"],
                "gpu_util_pct": (gpu_before["utilization_pct"] + gpu_after["utilization_pct"]) / 2,
                "response_preview": content[:100]
            }
    except Exception as e:
        elapsed = time.time() - start
        sampler.stop()
        return {"success": False, "time_s": elapsed, "error": str(e)}
    
    return {"success": False, "time_s": elapsed, "error": "Request failed"}


def percentile(data: List[float], p: float) -> float:
    """Calculate percentile"""
    if not data:
        return 0
    sorted_data = sorted(data)
    k = (len(sorted_data) - 1) * p / 100
    f = int(k)
    c = f + 1 if f + 1 < len(sorted_data) else f
    return sorted_data[f] + (k - f) * (sorted_data[c] - sorted_data[f]) if c != f else sorted_data[f]


def run_benchmark():
    print("=" * 70)
    print("Z.E.T.A. GKV vs Vanilla llama.cpp (Sequential Mode)")
    print("=" * 70)
    print(f"Date: {datetime.now().isoformat()}")
    print(f"Model: Qwen2.5-14B-Instruct Q4_K_M")
    print(f"GPU: NVIDIA RTX 5060 Ti 16GB")
    print(f"Queries: {len(TEST_QUERIES)}")
    print()
    
    # ========================================
    # PHASE 1: ZETA Benchmark
    # ========================================
    print("[1] Starting ZETA Phase...")
    print("-" * 70)
    
    if not start_zeta_server():
        print("✗ Failed to start ZETA server. Aborting.")
        return
    
    # Get initial stats
    gkv_start = get_gkv_stats()
    gpu_start = get_gpu_stats()
    print(f"  Initial GKV: {gkv_start.get('injections', 0)} injections")
    print(f"  GPU Memory: {gpu_start['mem_used_mb']:.0f} MB")
    print()
    
    print("[2] Running ZETA queries...")
    print("-" * 70)
    zeta_results = []
    for i, query in enumerate(TEST_QUERIES, 1):
        result = query_zeta(query)
        result["query_num"] = i
        result["query"] = query[:50] + "..." if len(query) > 50 else query
        zeta_results.append(result)
        
        gkv_mark = f"⚡×{result.get('gkv_injections', 0)}" if result.get("gkv_injections", 0) > 0 else "   "
        status = "✓" if result["success"] else "✗"
        tps = result.get("tokens_per_sec", 0)
        print(f"  [{i:2d}] {result['time_s']:5.2f}s {tps:5.1f}t/s {result.get('power_avg_w', 0):3.0f}W {gkv_mark:6s} {status}")
        time.sleep(0.3)
    
    gkv_end = get_gkv_stats()
    
    # ========================================
    # PHASE 2: Baseline Benchmark
    # ========================================
    print()
    print("[3] Stopping ZETA, Starting Baseline Phase...")
    print("-" * 70)
    
    if not start_baseline_server():
        print("✗ Failed to start baseline server. Saving ZETA-only results.")
        baseline_results = []
    else:
        gpu_baseline = get_gpu_stats()
        print(f"  GPU Memory: {gpu_baseline['mem_used_mb']:.0f} MB")
        print()
        
        print("[4] Running Baseline queries...")
        print("-" * 70)
        baseline_results = []
        for i, query in enumerate(TEST_QUERIES, 1):
            result = query_baseline(query)
            result["query_num"] = i
            result["query"] = query[:50] + "..." if len(query) > 50 else query
            baseline_results.append(result)
            
            status = "✓" if result["success"] else "✗"
            tps = result.get("tokens_per_sec", 0)
            print(f"  [{i:2d}] {result['time_s']:5.2f}s {tps:5.1f}t/s {result.get('power_avg_w', 0):3.0f}W        {status}")
            time.sleep(0.3)
    
    # Cleanup
    print()
    print("[5] Cleanup...")
    kill_all_servers()
    
    # ========================================
    # RESULTS ANALYSIS
    # ========================================
    print()
    print("=" * 70)
    print("BENCHMARK RESULTS")
    print("=" * 70)
    
    # Extract metrics
    zeta_times = [r["time_s"] for r in zeta_results if r["success"]]
    zeta_tps = [r.get("tokens_per_sec", 0) for r in zeta_results if r["success"]]
    zeta_energy = [r.get("energy_j", 0) for r in zeta_results if r["success"]]
    zeta_power = [r.get("power_avg_w", 0) for r in zeta_results if r["success"]]
    zeta_gkv = sum(r.get("gkv_injections", 0) for r in zeta_results)
    zeta_gkv_saved = sum(r.get("gkv_saved_ms", 0) for r in zeta_results)
    
    base_times = [r["time_s"] for r in baseline_results if r["success"]] if baseline_results else []
    base_tps = [r.get("tokens_per_sec", 0) for r in baseline_results if r["success"]] if baseline_results else []
    base_energy = [r.get("energy_j", 0) for r in baseline_results if r["success"]] if baseline_results else []
    base_power = [r.get("power_avg_w", 0) for r in baseline_results if r["success"]] if baseline_results else []
    
    print("\n📊 LATENCY (seconds):")
    print(f"                    {'ZETA':>12s}  {'Baseline':>12s}  {'Delta':>10s}")
    print(f"   Total:           {sum(zeta_times):>12.1f}  {sum(base_times):>12.1f}  {sum(base_times)-sum(zeta_times):>+10.1f}")
    print(f"   Average:         {statistics.mean(zeta_times):>12.2f}  {statistics.mean(base_times) if base_times else 0:>12.2f}")
    print(f"   Median (p50):    {percentile(zeta_times, 50):>12.2f}  {percentile(base_times, 50):>12.2f}")
    print(f"   p95:             {percentile(zeta_times, 95):>12.2f}  {percentile(base_times, 95):>12.2f}")
    print(f"   p99:             {percentile(zeta_times, 99):>12.2f}  {percentile(base_times, 99):>12.2f}")
    print(f"   Min:             {min(zeta_times):>12.2f}  {min(base_times) if base_times else 0:>12.2f}")
    print(f"   Max:             {max(zeta_times):>12.2f}  {max(base_times) if base_times else 0:>12.2f}")
    
    if base_times and sum(zeta_times) > 0:
        speedup = sum(base_times) / sum(zeta_times)
        print(f"\n   ⚡ SPEEDUP: {speedup:.2f}x {'✓' if speedup >= 2 else ''}")
    
    print("\n📈 THROUGHPUT (tokens/sec):")
    print(f"                    {'ZETA':>12s}  {'Baseline':>12s}")
    print(f"   Average:         {statistics.mean(zeta_tps):>12.1f}  {statistics.mean(base_tps) if base_tps else 0:>12.1f}")
    print(f"   Peak:            {max(zeta_tps):>12.1f}  {max(base_tps) if base_tps else 0:>12.1f}")
    
    print("\n⚡ ENERGY:")
    zeta_kj = sum(zeta_energy) / 1000
    base_kj = sum(base_energy) / 1000 if base_energy else 0
    print(f"   ZETA Total:      {zeta_kj:.4f} kJ ({sum(zeta_energy):.0f} J)")
    print(f"   Baseline Total:  {base_kj:.4f} kJ ({sum(base_energy):.0f} J)")
    print(f"   ZETA Avg Power:  {statistics.mean(zeta_power):.0f} W")
    print(f"   Baseline Avg:    {statistics.mean(base_power) if base_power else 0:.0f} W")
    
    if base_kj > 0 and zeta_kj > 0:
        energy_ratio = base_kj / zeta_kj
        savings_pct = (1 - zeta_kj / base_kj) * 100
        print(f"\n   🔋 ENERGY REDUCTION: {energy_ratio:.2f}x ({savings_pct:.0f}% less)")
    
    print("\n🧠 GKV ACCELERATION:")
    print(f"   Total injections:    {zeta_gkv}")
    print(f"   Avg per query:       {zeta_gkv/len(TEST_QUERIES):.1f}")
    print(f"   Total prefill saved: {zeta_gkv_saved:.1f} ms")
    print(f"   Queries with GKV:    {sum(1 for r in zeta_results if r.get('gkv_injections', 0) > 0)}/{len(zeta_results)}")
    
    # Save results
    output = {
        "benchmark": "GKV vs Baseline (Sequential)",
        "date": datetime.now().isoformat(),
        "config": {
            "model": "Qwen2.5-14B-Instruct Q4_K_M",
            "gpu": "NVIDIA RTX 5060 Ti 16GB",
            "num_queries": len(TEST_QUERIES)
        },
        "zeta": {
            "success_rate": f"{len(zeta_times)}/{len(TEST_QUERIES)}",
            "latency": {
                "total_s": round(sum(zeta_times), 2),
                "avg_s": round(statistics.mean(zeta_times), 3),
                "median_s": round(percentile(zeta_times, 50), 3),
                "p95_s": round(percentile(zeta_times, 95), 3),
                "p99_s": round(percentile(zeta_times, 99), 3),
                "min_s": round(min(zeta_times), 3),
                "max_s": round(max(zeta_times), 3)
            },
            "throughput": {
                "avg_tps": round(statistics.mean(zeta_tps), 1),
                "peak_tps": round(max(zeta_tps), 1)
            },
            "energy": {
                "total_kj": round(zeta_kj, 4),
                "avg_power_w": round(statistics.mean(zeta_power), 0)
            },
            "gkv": {
                "total_injections": zeta_gkv,
                "prefill_saved_ms": round(zeta_gkv_saved, 1),
                "queries_with_gkv": sum(1 for r in zeta_results if r.get("gkv_injections", 0) > 0)
            },
            "results": zeta_results
        },
        "baseline": {
            "success_rate": f"{len(base_times)}/{len(TEST_QUERIES)}" if baseline_results else "0/0",
            "latency": {
                "total_s": round(sum(base_times), 2) if base_times else 0,
                "avg_s": round(statistics.mean(base_times), 3) if base_times else 0,
                "median_s": round(percentile(base_times, 50), 3),
                "p95_s": round(percentile(base_times, 95), 3),
                "p99_s": round(percentile(base_times, 99), 3),
                "min_s": round(min(base_times), 3) if base_times else 0,
                "max_s": round(max(base_times), 3) if base_times else 0
            },
            "throughput": {
                "avg_tps": round(statistics.mean(base_tps), 1) if base_tps else 0,
                "peak_tps": round(max(base_tps), 1) if base_tps else 0
            },
            "energy": {
                "total_kj": round(base_kj, 4),
                "avg_power_w": round(statistics.mean(base_power), 0) if base_power else 0
            },
            "results": baseline_results
        },
        "comparison": {
            "speedup_x": round(sum(base_times) / sum(zeta_times), 2) if base_times and sum(zeta_times) > 0 else 0,
            "energy_reduction_x": round(base_kj / zeta_kj, 2) if base_kj > 0 and zeta_kj > 0 else 0,
            "latency_improvement_pct": round((1 - sum(zeta_times) / sum(base_times)) * 100, 1) if base_times else 0,
            "energy_savings_pct": round((1 - zeta_kj / base_kj) * 100, 1) if base_kj > 0 else 0
        }
    }
    
    outfile = "gkv_vs_baseline_results.json"
    with open(outfile, "w") as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Results saved to {outfile}")
    
    # Final verdict
    print()
    print("=" * 70)
    if base_times and sum(zeta_times) > 0:
        speedup = sum(base_times) / sum(zeta_times)
        energy_ratio = base_kj / zeta_kj if zeta_kj > 0 else 0
        
        if speedup >= 2:
            print(f"🎉 ZETA is {speedup:.1f}x faster than vanilla llama.cpp")
        else:
            print(f"📊 ZETA speedup: {speedup:.2f}x")
        
        if energy_ratio >= 1.5:
            print(f"🔋 ZETA uses {100/energy_ratio:.0f}% of baseline energy ({energy_ratio:.1f}x more efficient)")
    else:
        print("⚠️  Baseline comparison unavailable")
    print("=" * 70)


if __name__ == "__main__":
    run_benchmark()
