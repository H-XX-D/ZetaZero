#!/usr/bin/env python3
"""Energy benchmark: 20 turns, 300 max tokens, Baseline vs Zeta (2 runs)"""
import json
import time
import requests
import subprocess
from datetime import datetime

def get_power():
    try:
        r = subprocess.run(
            ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=5
        )
        return float(r.stdout.strip()) if r.returncode == 0 else 0.0
    except:
        return 0.0

def run_turn(url, query, history, max_tok=300):
    messages = history[-10:] + [{"role": "user", "content": query}]
    p0 = get_power()
    t0 = time.time()
    try:
        r = requests.post(
            url + "/v1/chat/completions",
            json={"model": "llama", "messages": messages, "max_tokens": max_tok, "temperature": 0.7},
            timeout=180
        )
        lat = time.time() - t0
        p1 = get_power()
        energy = (p0 + p1) / 2 * lat
        if r.ok:
            d = r.json()
            u = d.get("usage", {})
            c = d["choices"][0]["message"]["content"]
            return {
                "ok": True, "lat": lat, "tok": u.get("completion_tokens", 0),
                "ttok": u.get("total_tokens", 0), "e": energy, "pwr": (p0 + p1) / 2,
                "resp": c[:80]
            }
        return {"ok": False, "lat": lat, "e": energy, "err": r.text[:100]}
    except Exception as ex:
        lat = time.time() - t0
        return {"ok": False, "lat": lat, "e": 0, "err": str(ex)}

QUERIES = [
    "Explain how neural networks learn through backpropagation",
    "What are the key differences between REST and GraphQL APIs?",
    "How does garbage collection work in programming languages?",
    "Describe the CAP theorem for distributed systems",
    "What is the difference between concurrency and parallelism?",
    "Explain how TLS/SSL handshake works",
    "What are microservices and when should you use them?",
    "How do database indexes improve query performance?",
    "Explain eventual consistency in distributed databases",
    "What is a bloom filter and when would you use it?",
    "Write a function to find longest common subsequence",
    "How to implement a thread-safe singleton in Python?",
    "Explain async/await vs threading",
    "What is the time complexity of quicksort?",
    "How to handle race conditions in concurrent code?",
    "Write a function to detect cycle in linked list",
    "Explain the observer pattern with example",
    "How to implement rate limiting in an API?",
    "What are the SOLID principles?",
    "Explain dependency injection with examples"
]

def run_benchmark(name, url, max_tok=300):
    print(f"\n{'='*50}")
    print(f"{name} (max_tokens={max_tok})")
    print("="*50)
    results = []
    history = []
    total_energy = 0.0
    total_tokens = 0
    
    for i, q in enumerate(QUERIES):
        r = run_turn(url, q, history, max_tok)
        results.append(r)
        if r["ok"]:
            total_energy += r["e"]
            total_tokens += r["tok"]
            history.append({"role": "user", "content": q})
            history.append({"role": "assistant", "content": r.get("resp", "")})
        status = "OK" if r["ok"] else "FAIL"
        tok = r.get("tok", 0)
        pwr = r.get("pwr", 0)
        print(f"[{i+1:02d}] {r['lat']:5.1f}s {pwr:5.0f}W {r['e']:6.0f}J {tok:3d}tok {status}")
    
    ok = [x for x in results if x["ok"]]
    return {
        "name": name,
        "success": len(ok),
        "total": len(QUERIES),
        "time_s": sum(x["lat"] for x in ok),
        "avg_lat": sum(x["lat"] for x in ok) / len(ok) if ok else 0,
        "energy_j": total_energy,
        "tokens": total_tokens,
        "j_per_tok": total_energy / total_tokens if total_tokens > 0 else 0,
        "avg_pwr": sum(x.get("pwr", 0) for x in ok) / len(ok) if ok else 0
    }

if __name__ == "__main__":
    print("="*60)
    print(f"Z.E.T.A. Energy Benchmark - {datetime.now()}")
    print("20 turns, 300 max tokens, fair comparison")
    print("="*60)
    
    bl = run_benchmark("Baseline", "http://localhost:9000", 300)
    z1 = run_benchmark("Zeta-Run1", "http://localhost:8080", 300)
    z2 = run_benchmark("Zeta-Run2", "http://localhost:8080", 300)
    
    print("\n" + "="*60)
    print("RESULTS SUMMARY")
    print("="*60)
    print(f"{'Metric':<20} {'Baseline':>12} {'Zeta-R1':>12} {'Zeta-R2':>12}")
    print("-"*58)
    print(f"{'Success':<20} {bl['success']:>12} {z1['success']:>12} {z2['success']:>12}")
    print(f"{'Time (s)':<20} {bl['time_s']:>12.1f} {z1['time_s']:>12.1f} {z2['time_s']:>12.1f}")
    print(f"{'Avg Latency (s)':<20} {bl['avg_lat']:>12.2f} {z1['avg_lat']:>12.2f} {z2['avg_lat']:>12.2f}")
    print(f"{'Comp Tokens':<20} {bl['tokens']:>12,} {z1['tokens']:>12,} {z2['tokens']:>12,}")
    print(f"{'Energy (J)':<20} {bl['energy_j']:>12.0f} {z1['energy_j']:>12.0f} {z2['energy_j']:>12.0f}")
    print(f"{'J/token':<20} {bl['j_per_tok']:>12.2f} {z1['j_per_tok']:>12.2f} {z2['j_per_tok']:>12.2f}")
    print(f"{'Avg Power (W)':<20} {bl['avg_pwr']:>12.0f} {z1['avg_pwr']:>12.0f} {z2['avg_pwr']:>12.0f}")
    
    if bl["energy_j"] > 0:
        s1 = (bl["energy_j"] - z1["energy_j"]) / bl["energy_j"] * 100
        s2 = (bl["energy_j"] - z2["energy_j"]) / bl["energy_j"] * 100
        print(f"\nEnergy vs Baseline:")
        print(f"  Zeta-R1: {s1:+.1f}% {'savings' if s1 > 0 else 'overhead'}")
        print(f"  Zeta-R2: {s2:+.1f}% {'savings' if s2 > 0 else 'overhead'}")
    
    if z1["avg_lat"] > 0:
        speedup = (z1["avg_lat"] - z2["avg_lat"]) / z1["avg_lat"] * 100
        print(f"  Graph Warmup Speedup (R2 vs R1): {speedup:.1f}%")
    
    results = {"baseline": bl, "zeta_r1": z1, "zeta_r2": z2, "date": str(datetime.now())}
    with open("/runpod-volume/energy_results.json", "w") as f:
        json.dump(results, f, indent=2)
    print("\nSaved to /runpod-volume/energy_results.json")
    print("BENCHMARK_DONE")
