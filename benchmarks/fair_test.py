#!/usr/bin/env python3
"""
Fair comparison test: Same max_tokens for Zeta and Baseline
"""
import requests
import json
import time
import subprocess

ZETA_URL = "http://localhost:8080"
BASE_URL = "http://localhost:9000"

def get_power():
    try:
        r = subprocess.run(["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
                          capture_output=True, text=True, timeout=5)
        return float(r.stdout.strip()) if r.returncode == 0 else 0.0
    except:
        return 0.0

def test_single(url, query, max_tokens):
    p0 = get_power()
    t0 = time.time()
    r = requests.post(f"{url}/v1/chat/completions", json={
        "model": "llama",
        "messages": [{"role": "user", "content": query}],
        "max_tokens": max_tokens
    }, timeout=120)
    lat = time.time() - t0
    p1 = get_power()
    energy = (p0 + p1) / 2 * lat
    
    if r.ok:
        d = r.json()
        u = d.get("usage", {})
        c = d.get("choices", [{}])[0].get("message", {}).get("content", "")
        return {
            "ok": True,
            "lat": lat,
            "energy": energy,
            "prompt_tokens": u.get("prompt_tokens", 0),
            "completion_tokens": u.get("completion_tokens", 0),
            "total_tokens": u.get("total_tokens", 0),
            "response": c[:200]
        }
    return {"ok": False, "lat": lat, "energy": energy, "error": r.text[:100]}

# Test with various max_tokens to find where output diverges
print("=" * 60)
print("FAIR COMPARISON TEST: Equal max_tokens")
print("=" * 60)

QUERIES = [
    "Explain backpropagation in neural networks",
    "What is the CAP theorem in distributed systems?",
    "How does garbage collection work in Python?",
    "Compare REST and GraphQL APIs",
    "Explain eventual consistency",
]

for max_tok in [200]:  # Test with 200 max_tokens
    print(f"\n=== max_tokens = {max_tok} ===")
    z_total_e, b_total_e = 0, 0
    z_total_t, b_total_t = 0, 0
    z_total_tok, b_total_tok = 0, 0
    
    for i, q in enumerate(QUERIES, 1):
        print(f"\n[{i}] {q[:40]}...")
        
        # Zeta
        z = test_single(ZETA_URL, q, max_tok)
        if z["ok"]:
            z_total_e += z["energy"]
            z_total_t += z["lat"]
            z_total_tok += z["completion_tokens"]
            print(f"  ZETA: {z['completion_tokens']:3d} tok, {z['lat']:.2f}s, {z['energy']:.0f}J")
        else:
            print(f"  ZETA: FAILED - {z.get('error', 'unknown')}")
        
        # Baseline
        b = test_single(BASE_URL, q, max_tok)
        if b["ok"]:
            b_total_e += b["energy"]
            b_total_t += b["lat"]
            b_total_tok += b["completion_tokens"]
            print(f"  BASE: {b['completion_tokens']:3d} tok, {b['lat']:.2f}s, {b['energy']:.0f}J")
        else:
            print(f"  BASE: FAILED - {b.get('error', 'unknown')}")
        
        if z["ok"] and b["ok"]:
            delta = z["completion_tokens"] - b["completion_tokens"]
            print(f"  DIFF: {delta:+d} tokens")
    
    print(f"\n--- Summary for max_tokens={max_tok} ---")
    print(f"ZETA: {z_total_tok} total tokens, {z_total_t:.1f}s, {z_total_e:.0f}J")
    print(f"BASE: {b_total_tok} total tokens, {b_total_t:.1f}s, {b_total_e:.0f}J")
    if b_total_e > 0:
        savings = (b_total_e - z_total_e) / b_total_e * 100
        print(f"Energy savings: {savings:+.1f}%")
    if b_total_t > 0:
        speedup = (b_total_t - z_total_t) / b_total_t * 100
        print(f"Time diff: {speedup:+.1f}%")

print("\nDONE")
