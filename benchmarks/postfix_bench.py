#!/usr/bin/env python3
"""Post-fix benchmark: KV cache features re-enabled"""
import json, time, requests, subprocess

def pw():
    try:
        r = subprocess.run(["nvidia-smi","--query-gpu=power.draw","--format=csv,noheader,nounits"],
                          capture_output=True, text=True)
        return float(r.stdout.strip())
    except:
        return 0

def turn(url, q, h):
    m = h[-10:] + [{"role": "user", "content": q}]
    p0 = pw()
    t0 = time.time()
    r = requests.post(f"{url}/v1/chat/completions",
                     json={"model": "llama", "messages": m, "max_tokens": 150},
                     timeout=120)
    lat = time.time() - t0
    e = (p0 + pw()) / 2 * lat
    if r.ok:
        d = r.json()
        return {"ok": 1, "lat": lat, "tok": d.get("usage", {}).get("total_tokens", 0), "e": e}
    return {"ok": 0, "lat": lat, "e": e}

Q = ["Backprop", "REST vs GraphQL", "GC", "CAP", "Concurrency",
     "TLS", "Microservices", "Indexes", "Eventual consistency", "Bloom filter"] * 2

def bench(name, url):
    print(f"=== {name} ===")
    res, h, E = [], [], 0
    for i, q in enumerate(Q):
        r = turn(url, q, h)
        res.append(r)
        if r["ok"]:
            E += r["e"]
            h += [{"role": "user", "content": q}, {"role": "assistant", "content": "ok"}]
        lat = r["lat"]
        e = r["e"]
        print(f"[{i+1:02d}] {lat:.1f}s {e:.0f}J")
    ok = [x for x in res if x["ok"]]
    avg = sum(x["lat"] for x in ok) / len(ok) if ok else 0
    return {"n": name, "ok": len(ok), "time": sum(x["lat"] for x in ok), "avg": avg, "E": E}

print("\n" + "=" * 50)
print("POST-FIX BENCHMARK: KV Cache Re-enabled")
print("=" * 50)

bl = bench("Baseline", "http://localhost:9000")
z1 = bench("Zeta-Run1", "http://localhost:8080")
z2 = bench("Zeta-Run2", "http://localhost:8080")

print("\n" + "=" * 50)
print("RESULTS")
print("=" * 50)
print(f"Baseline:  {bl['ok']}/20, {bl['time']:.0f}s total, {bl['avg']:.2f}s avg, {bl['E']:.0f}J")
print(f"Zeta-R1:   {z1['ok']}/20, {z1['time']:.0f}s total, {z1['avg']:.2f}s avg, {z1['E']:.0f}J")
print(f"Zeta-R2:   {z2['ok']}/20, {z2['time']:.0f}s total, {z2['avg']:.2f}s avg, {z2['E']:.0f}J")

if bl["E"] > 0:
    s1 = 100 * (bl["E"] - z1["E"]) / bl["E"]
    s2 = 100 * (bl["E"] - z2["E"]) / bl["E"]
    print(f"\nEnergy vs Baseline: R1={s1:.1f}% savings, R2={s2:.1f}% savings")

if z1["avg"] > 0:
    speedup = 100 * (z1["avg"] - z2["avg"]) / z1["avg"]
    print(f"Graph Recall Speedup (R2 vs R1): {speedup:.1f}% faster")

json.dump({"baseline": bl, "zeta1": z1, "zeta2": z2}, open("/runpod-volume/postfix_results.json", "w"))
print("\nDONE")
