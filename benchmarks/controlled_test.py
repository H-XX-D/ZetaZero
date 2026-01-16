#!/usr/bin/env python3
"""Controlled test: Same query, same max_tokens, measure energy per token"""
import requests, time, subprocess

def get_power():
    r = subprocess.run(
        ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
        capture_output=True, text=True
    )
    return sum(float(x) for x in r.stdout.strip().split("\n") if x)

QUERY = "Explain backpropagation in neural networks. Cover the chain rule, gradient descent, and how weights are updated. Be thorough."

print("=" * 60)
print("CONTROLLED TOKEN-MATCHED ENERGY TEST")
print("=" * 60)

results = {}
for name, url in [("Baseline", "http://localhost:9000"), ("Zeta", "http://localhost:8080")]:
    # Run 5 times for average
    runs = []
    for i in range(5):
        p0, t0 = get_power(), time.time()
        r = requests.post(
            url + "/v1/chat/completions",
            json={"model": "llama", "messages": [{"role": "user", "content": QUERY}], "max_tokens": 100},
            timeout=60
        )
        lat = time.time() - t0
        p1 = get_power()
        d = r.json()
        tok = d.get("usage", {}).get("total_tokens", 0)
        comp = d.get("usage", {}).get("completion_tokens", 0)
        e = (p0 + p1) / 2 * lat
        runs.append({"lat": lat, "tok": tok, "comp": comp, "e": e, "pwr": (p0+p1)/2})
        print(f"  {name} run {i+1}: {lat:.2f}s, {comp} completion tokens, {e:.0f}J")
    
    avg_lat = sum(r["lat"] for r in runs) / len(runs)
    avg_e = sum(r["e"] for r in runs) / len(runs)
    avg_tok = sum(r["tok"] for r in runs) / len(runs)
    avg_comp = sum(r["comp"] for r in runs) / len(runs)
    results[name] = {"avg_lat": avg_lat, "avg_e": avg_e, "avg_tok": avg_tok, "avg_comp": avg_comp}
    print(f"{name} AVG: {avg_lat:.2f}s, {avg_comp:.0f} completion tokens, {avg_e:.0f}J\n")

print("=" * 60)
print("COMPARISON (Token-Matched)")
print("=" * 60)
b, z = results["Baseline"], results["Zeta"]
print(f"Baseline: {b['avg_lat']:.2f}s, {b['avg_comp']:.0f} tokens, {b['avg_e']:.0f}J")
print(f"Zeta:     {z['avg_lat']:.2f}s, {z['avg_comp']:.0f} tokens, {z['avg_e']:.0f}J")

if b["avg_e"] > 0:
    savings = 100 * (b["avg_e"] - z["avg_e"]) / b["avg_e"]
    print(f"\nEnergy Savings: {savings:+.1f}%")

if b["avg_comp"] > 0 and z["avg_comp"] > 0:
    b_jpt = b["avg_e"] / b["avg_comp"]
    z_jpt = z["avg_e"] / z["avg_comp"]
    print(f"Energy per token: Baseline={b_jpt:.1f}J/tok, Zeta={z_jpt:.1f}J/tok")
    print(f"Efficiency gain: {100*(b_jpt-z_jpt)/b_jpt:+.1f}% per token")

print("\nDONE")
