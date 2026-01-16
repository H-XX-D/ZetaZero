#!/usr/bin/env python3
"""5-Run Comparison Benchmark - Shows Graph KV cache warming over repeated runs."""

import json, time, requests, subprocess

def pw():
    try:
        r = subprocess.run(["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
                          capture_output=True, text=True, timeout=5)
        return float(r.stdout.strip()) if r.returncode == 0 else 0.0
    except:
        return 0.0

def run_batch(url, queries, history):
    results = []
    h = history[:]
    total_e, total_tok = 0, 0
    for q in queries:
        m = h[-10:] + [{"role": "user", "content": q}]
        p0, t0 = pw(), time.time()
        try:
            r = requests.post(f"{url}/v1/chat/completions",
                json={"model": "llama", "messages": m, "max_tokens": 150},
                timeout=120)
            lat = time.time() - t0
            e = (p0 + pw()) / 2 * lat
            if r.ok:
                d = r.json()
                tok = d.get("usage", {}).get("total_tokens", 0)
                resp = d["choices"][0]["message"]["content"][:60]
                results.append({"ok": 1, "lat": lat, "tok": tok, "e": e})
                total_e += e
                total_tok += tok
                h += [{"role": "user", "content": q}, {"role": "assistant", "content": resp}]
            else:
                results.append({"ok": 0, "lat": lat, "e": e})
        except Exception as ex:
            results.append({"ok": 0, "lat": time.time() - t0, "e": 0})
    ok = [x for x in results if x.get("ok")]
    return {
        "ok": len(ok), "total": len(queries),
        "time": sum(x["lat"] for x in ok),
        "avg_lat": sum(x["lat"] for x in ok) / len(ok) if ok else 0,
        "energy": total_e,
        "tokens": total_tok
    }, h

QUERIES = [
    "Explain backpropagation in neural networks",
    "What is the difference between REST and GraphQL?",
    "How does garbage collection work?",
    "Explain the CAP theorem",
    "What is concurrency vs parallelism?",
    "How does TLS handshake work?",
    "When to use microservices?",
    "How do database indexes work?",
    "What is eventual consistency?",
    "How does a bloom filter work?",
]

NUM_RUNS = 5

print("=" * 60)
print(f"5-RUN COMPARISON: Baseline vs ZetaZero")
print(f"10 queries x {NUM_RUNS} runs = {10*NUM_RUNS} total per system")
print("=" * 60)

# Baseline runs
print("\n>>> BASELINE (no cache) <<<")
bl_runs = []
bl_history = []
for run in range(1, NUM_RUNS + 1):
    result, bl_history = run_batch("http://localhost:9000", QUERIES, bl_history)
    bl_runs.append(result)
    print(f"Run {run}: {result['ok']}/10 ok, {result['time']:.1f}s, {result['avg_lat']:.2f}s avg, {result['energy']:.0f}J")

# Zeta runs
print("\n>>> ZETAZERO (Graph KV Cache) <<<")
z_runs = []
z_history = []
for run in range(1, NUM_RUNS + 1):
    result, z_history = run_batch("http://localhost:8080", QUERIES, z_history)
    z_runs.append(result)
    print(f"Run {run}: {result['ok']}/10 ok, {result['time']:.1f}s, {result['avg_lat']:.2f}s avg, {result['energy']:.0f}J")

# Summary
print("\n" + "=" * 60)
print("SUMMARY BY RUN (Energy)")
print("=" * 60)
print(f"{'Run':<6} {'Baseline':>12} {'Zeta':>12} {'Savings':>12}")
print("-" * 45)
for i in range(NUM_RUNS):
    bl_e = bl_runs[i]["energy"]
    z_e = z_runs[i]["energy"]
    sav = (bl_e - z_e) / bl_e * 100 if bl_e > 0 else 0
    print(f"Run {i+1:<3} {bl_e:>10.0f}J {z_e:>10.0f}J {sav:>+10.1f}%")

bl_total = sum(r["energy"] for r in bl_runs)
z_total = sum(r["energy"] for r in z_runs)
total_sav = (bl_total - z_total) / bl_total * 100 if bl_total > 0 else 0
print("-" * 45)
print(f"TOTAL  {bl_total:>10.0f}J {z_total:>10.0f}J {total_sav:>+10.1f}%")

print("\n" + "=" * 60)
print("LATENCY TREND (avg per query)")
print("=" * 60)
print(f"{'Run':<6} {'Baseline':>12} {'Zeta':>12} {'Speedup':>12}")
print("-" * 45)
for i in range(NUM_RUNS):
    bl_lat = bl_runs[i]["avg_lat"]
    z_lat = z_runs[i]["avg_lat"]
    speedup = (bl_lat - z_lat) / bl_lat * 100 if bl_lat > 0 else 0
    print(f"Run {i+1:<3} {bl_lat:>10.2f}s {z_lat:>10.2f}s {speedup:>+10.1f}%")

# Show Zeta improvement across runs
print("\n" + "=" * 60)
print("ZETA IMPROVEMENT ACROSS RUNS")
print("=" * 60)
run1_lat = z_runs[0]["avg_lat"]
run1_e = z_runs[0]["energy"]
for i in range(NUM_RUNS):
    lat_imp = (run1_lat - z_runs[i]["avg_lat"]) / run1_lat * 100 if run1_lat > 0 else 0
    e_imp = (run1_e - z_runs[i]["energy"]) / run1_e * 100 if run1_e > 0 else 0
    print(f"Run {i+1}: {z_runs[i]['avg_lat']:.2f}s ({lat_imp:+.1f}% vs Run1), {z_runs[i]['energy']:.0f}J ({e_imp:+.1f}% vs Run1)")

# Check cache stats
print("\n>>> Zeta Cache Stats <<<")
try:
    h = requests.get("http://localhost:8080/health", timeout=5).json()
    print(f"Graph nodes: {h.get('graph_nodes', 'N/A')}")
    print(f"Graph edges: {h.get('graph_edges', 'N/A')}")
except:
    pass

json.dump({"baseline": bl_runs, "zeta": z_runs}, open("/runpod-volume/5run_comparison.json", "w"), indent=2)
print("\nSaved to /runpod-volume/5run_comparison.json")
print("DONE")
