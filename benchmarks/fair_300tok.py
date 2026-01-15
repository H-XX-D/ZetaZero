#!/usr/bin/env python3
"""Fair 20-turn benchmark with 300 token cap for both Baseline and Zeta."""
import json
import time
import requests
import subprocess

def get_power():
    try:
        r = subprocess.run(
            ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=5
        )
        return float(r.stdout.strip()) if r.returncode == 0 else 0.0
    except:
        return 0.0

def run_turn(url, query, history, max_tokens):
    messages = history[-10:] + [{"role": "user", "content": query}]
    p0 = get_power()
    t0 = time.time()
    try:
        r = requests.post(
            f"{url}/v1/chat/completions",
            json={"model": "llama", "messages": messages, "max_tokens": max_tokens, "temperature": 0.7},
            timeout=180
        )
        latency = time.time() - t0
        p1 = get_power()
        energy = ((p0 + p1) / 2) * latency
        
        if r.ok:
            d = r.json()
            u = d.get("usage", {})
            content = d["choices"][0]["message"]["content"]
            return {
                "ok": True,
                "latency": latency,
                "completion_tokens": u.get("completion_tokens", 0),
                "total_tokens": u.get("total_tokens", 0),
                "energy_j": energy,
                "response": content[:100]
            }
        return {"ok": False, "latency": latency, "energy_j": energy, "error": r.text[:100]}
    except Exception as e:
        latency = time.time() - t0
        return {"ok": False, "latency": latency, "energy_j": 0, "error": str(e)}

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

def run_benchmark(name, url, max_tokens):
    print(f"\n=== {name} (max_tokens={max_tokens}) ===")
    results = []
    history = []
    total_energy = 0.0
    total_comp_tokens = 0
    
    for i, query in enumerate(QUERIES):
        r = run_turn(url, query, history, max_tokens)
        results.append(r)
        
        if r["ok"]:
            total_energy += r["energy_j"]
            total_comp_tokens += r["completion_tokens"]
            history.append({"role": "user", "content": query})
            history.append({"role": "assistant", "content": r.get("response", "")})
        
        status = "OK" if r["ok"] else "FAIL"
        tok = r.get("completion_tokens", 0)
        print(f"[{i+1:02d}] {r['latency']:5.1f}s {r['energy_j']:6.0f}J {tok:3d}tok {status}")
    
    successful = [r for r in results if r["ok"]]
    return {
        "name": name,
        "success": len(successful),
        "total": len(results),
        "total_time_s": sum(r["latency"] for r in successful),
        "avg_latency_s": sum(r["latency"] for r in successful) / len(successful) if successful else 0,
        "total_energy_j": total_energy,
        "completion_tokens": total_comp_tokens,
        "total_tokens": sum(r.get("total_tokens", 0) for r in successful)
    }

if __name__ == "__main__":
    MAX_TOKENS = 300
    
    print("=" * 60)
    print("FAIR COMPARISON: 300 Token Cap for Both")
    print("=" * 60)
    
    # Run baseline
    baseline = run_benchmark("Baseline", "http://localhost:9000", MAX_TOKENS)
    
    # Run Zeta twice (R1 = cold, R2 = graph recall)
    zeta1 = run_benchmark("Zeta-R1", "http://localhost:8080", MAX_TOKENS)
    zeta2 = run_benchmark("Zeta-R2", "http://localhost:8080", MAX_TOKENS)
    
    print("\n" + "=" * 60)
    print("RESULTS (300 Token Cap)")
    print("=" * 60)
    print(f"{'Metric':<20} {'Baseline':>12} {'Zeta-R1':>12} {'Zeta-R2':>12}")
    print("-" * 60)
    print(f"{'Success':<20} {baseline['success']:>12} {zeta1['success']:>12} {zeta2['success']:>12}")
    print(f"{'Total Time (s)':<20} {baseline['total_time_s']:>12.1f} {zeta1['total_time_s']:>12.1f} {zeta2['total_time_s']:>12.1f}")
    print(f"{'Completion Tokens':<20} {baseline['completion_tokens']:>12} {zeta1['completion_tokens']:>12} {zeta2['completion_tokens']:>12}")
    print(f"{'Total Energy (J)':<20} {baseline['total_energy_j']:>12.0f} {zeta1['total_energy_j']:>12.0f} {zeta2['total_energy_j']:>12.0f}")
    
    if baseline['total_energy_j'] > 0:
        s1 = 100 * (baseline['total_energy_j'] - zeta1['total_energy_j']) / baseline['total_energy_j']
        s2 = 100 * (baseline['total_energy_j'] - zeta2['total_energy_j']) / baseline['total_energy_j']
        print(f"\nEnergy Savings vs Baseline:")
        print(f"  Zeta R1: {s1:+.1f}%")
        print(f"  Zeta R2: {s2:+.1f}%")
    
    if baseline['completion_tokens'] > 0 and zeta1['completion_tokens'] > 0:
        bl_per = baseline['total_energy_j'] / baseline['completion_tokens']
        z1_per = zeta1['total_energy_j'] / zeta1['completion_tokens']
        z2_per = zeta2['total_energy_j'] / zeta2['completion_tokens'] if zeta2['completion_tokens'] > 0 else 0
        print(f"\nEnergy per Token:")
        print(f"  Baseline: {bl_per:.3f} J/tok")
        print(f"  Zeta R1:  {z1_per:.3f} J/tok")
        print(f"  Zeta R2:  {z2_per:.3f} J/tok")
    
    # Save results
    output = {
        "benchmark": "Fair 300-Token Comparison",
        "max_tokens": MAX_TOKENS,
        "baseline": baseline,
        "zeta1": zeta1,
        "zeta2": zeta2
    }
    with open("/runpod-volume/fair_300tok_results.json", "w") as f:
        json.dump(output, f, indent=2)
    
    print("\nSaved to /runpod-volume/fair_300tok_results.json")
    print("DONE")
