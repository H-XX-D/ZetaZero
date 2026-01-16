#!/usr/bin/env python3
"""
Fair benchmark: Run Zeta twice, then run baseline with matched token limits
"""
import requests
import json
import time
import subprocess

def pw():
    try:
        return float(subprocess.run(
            ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
            capture_output=True, text=True
        ).stdout.strip())
    except:
        return 0

def test(url, q, mt):
    p0, t0 = pw(), time.time()
    r = requests.post(
        url + "/v1/chat/completions",
        json={"model": "llama", "messages": [{"role": "user", "content": q}], "max_tokens": mt},
        timeout=120
    )
    lat, p1 = time.time() - t0, pw()
    e = (p0 + p1) / 2 * lat
    if r.ok:
        d = r.json()
        u = d.get("usage", {})
        return {
            "lat": lat,
            "e": e,
            "tok": u.get("completion_tokens", 0),
            "finish": d.get("choices", [{}])[0].get("finish_reason", "")
        }
    return {"lat": lat, "e": e, "tok": 0, "finish": "error"}

Q = [
    "Explain backpropagation",
    "What is REST vs GraphQL?",
    "How does garbage collection work?",
    "Explain the CAP theorem",
    "What is eventual consistency?",
    "Compare microservices and monolith",
    "What is a bloom filter?",
    "Explain TLS handshake",
    "What is dependency injection?",
    "How do database indexes work?"
]

print("=" * 60)
print("MATCHED TOKEN BENCHMARK")
print("=" * 60)

# ZETA RUN 1
print("\n=== ZETA RUN 1 (cold graph) ===")
z1_toks = []
z1_time, z1_energy = 0, 0
for i, q in enumerate(Q, 1):
    r = test("http://localhost:8080", q, 300)
    z1_toks.append(r["tok"])
    z1_time += r["lat"]
    z1_energy += r["e"]
    print(f"[{i:2d}] {r['tok']:3d}tok {r['lat']:.1f}s {r['finish']}")
print(f"Z1 Total: {sum(z1_toks)}tok {z1_time:.1f}s {z1_energy:.0f}J")

# ZETA RUN 2
print("\n=== ZETA RUN 2 (warm graph) ===")
z2_toks = []
z2_time, z2_energy = 0, 0
for i, q in enumerate(Q, 1):
    r = test("http://localhost:8080", q, 300)
    z2_toks.append(r["tok"])
    z2_time += r["lat"]
    z2_energy += r["e"]
    print(f"[{i:2d}] {r['tok']:3d}tok {r['lat']:.1f}s {r['finish']}")
print(f"Z2 Total: {sum(z2_toks)}tok {z2_time:.1f}s {z2_energy:.0f}J")

# Use max of both runs for each question
max_toks = [max(z1_toks[i], z2_toks[i]) for i in range(len(Q))]
print(f"\nToken limits for baseline: {max_toks}")

# BASELINE with matched limits
print("\n=== BASELINE (matched token limits) ===")
b_time, b_energy, b_toks = 0, 0, 0
b_results = []
for i, q in enumerate(Q, 1):
    mt = max_toks[i - 1]
    r = test("http://localhost:9000", q, mt)
    b_time += r["lat"]
    b_energy += r["e"]
    b_toks += r["tok"]
    b_results.append(r)
    print(f"[{i:2d}] limit={mt:3d} got={r['tok']:3d}tok {r['lat']:.1f}s {r['finish']}")
print(f"B  Total: {b_toks}tok {b_time:.1f}s {b_energy:.0f}J")

# SUMMARY
print("\n" + "=" * 60)
print("SUMMARY (Equal Output Tokens)")
print("=" * 60)
print(f"{'Run':<12} {'Tokens':>8} {'Time':>8} {'Energy':>8} {'J/tok':>8}")
print("-" * 44)
print(f"{'Zeta R1':<12} {sum(z1_toks):>8} {z1_time:>7.1f}s {z1_energy:>7.0f}J {z1_energy/sum(z1_toks):>7.2f}")
print(f"{'Zeta R2':<12} {sum(z2_toks):>8} {z2_time:>7.1f}s {z2_energy:>7.0f}J {z2_energy/sum(z2_toks):>7.2f}")
print(f"{'Baseline':<12} {b_toks:>8} {b_time:>7.1f}s {b_energy:>7.0f}J {b_energy/b_toks:>7.2f}")

print("\n--- Comparison ---")
if b_energy > 0:
    print(f"Energy savings vs Baseline: Z1={100*(b_energy-z1_energy)/b_energy:.1f}% Z2={100*(b_energy-z2_energy)/b_energy:.1f}%")
if b_time > 0:
    print(f"Time savings vs Baseline: Z1={100*(b_time-z1_time)/b_time:.1f}% Z2={100*(b_time-z2_time)/b_time:.1f}%")

# Graph warming effect
if z1_time > 0:
    print(f"\nGraph warming effect (Z2 vs Z1): {100*(z1_time-z2_time)/z1_time:.1f}% faster")

print("\nDONE")
