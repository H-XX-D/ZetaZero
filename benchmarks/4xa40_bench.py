#!/usr/bin/env python3
"""4x A40 Energy Benchmark: Baseline vs Zeta (2 runs)"""
import json, time, requests, subprocess

def get_power():
    try:
        r = subprocess.run(
            ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
            capture_output=True, text=True
        )
        return sum(float(x) for x in r.stdout.strip().split("\n") if x)
    except:
        return 0

def run_turn(url, query, history):
    msgs = history[-10:] + [{"role": "user", "content": query}]
    p0, t0 = get_power(), time.time()
    try:
        r = requests.post(
            url + "/v1/chat/completions",
            json={"model": "llama", "messages": msgs, "max_tokens": 150},
            timeout=120
        )
        lat = time.time() - t0
        p1 = get_power()
        avg_pwr = (p0 + p1) / 2
        energy = avg_pwr * lat
        if r.ok:
            d = r.json()
            tok = d.get("usage", {}).get("total_tokens", 0)
            return {"ok": True, "lat": lat, "tok": tok, "e": energy, "pwr": avg_pwr}
        return {"ok": False, "lat": lat, "e": energy, "pwr": avg_pwr}
    except Exception as ex:
        lat = time.time() - t0
        return {"ok": False, "lat": lat, "e": 0, "pwr": 0, "err": str(ex)}

QUERIES = [
    "Backprop", "REST vs GraphQL", "GC", "CAP", "Concurrency",
    "TLS", "Microservices", "Indexes", "Eventual consistency", "Bloom filter"
] * 2  # 20 turns

def run_benchmark(name, url):
    print(f"=== {name} ===")
    results, history, total_e = [], [], 0
    for i, q in enumerate(QUERIES):
        r = run_turn(url, q, history)
        results.append(r)
        if r["ok"]:
            total_e += r["e"]
            history.append({"role": "user", "content": q})
            history.append({"role": "assistant", "content": "ok"})
        status = "OK" if r["ok"] else "FAIL"
        print(f"[{i+1:02d}] {r['lat']:.1f}s {r['pwr']:.0f}W {r['e']:.0f}J {status}")
    
    ok = [x for x in results if x["ok"]]
    avg_lat = sum(x["lat"] for x in ok) / len(ok) if ok else 0
    return {
        "name": name,
        "success": len(ok),
        "total_time": sum(x["lat"] for x in ok),
        "avg_lat": avg_lat,
        "total_energy_j": total_e,
        "total_tokens": sum(x.get("tok", 0) for x in ok)
    }

if __name__ == "__main__":
    print("=" * 50)
    print("4x A40 BENCHMARK: Baseline vs Zeta (2 runs)")
    print("=" * 50)
    
    bl = run_benchmark("Baseline", "http://localhost:9000")
    z1 = run_benchmark("Zeta-Run1", "http://localhost:8080")
    z2 = run_benchmark("Zeta-Run2", "http://localhost:8080")
    
    print("\n" + "=" * 50)
    print("RESULTS")
    print("=" * 50)
    print(f"Baseline:  {bl['success']}/20, {bl['total_time']:.0f}s, {bl['avg_lat']:.2f}s avg, {bl['total_energy_j']:.0f}J")
    print(f"Zeta-R1:   {z1['success']}/20, {z1['total_time']:.0f}s, {z1['avg_lat']:.2f}s avg, {z1['total_energy_j']:.0f}J")
    print(f"Zeta-R2:   {z2['success']}/20, {z2['total_time']:.0f}s, {z2['avg_lat']:.2f}s avg, {z2['total_energy_j']:.0f}J")
    
    if bl["total_energy_j"] > 0:
        sav1 = 100 * (bl["total_energy_j"] - z1["total_energy_j"]) / bl["total_energy_j"]
        sav2 = 100 * (bl["total_energy_j"] - z2["total_energy_j"]) / bl["total_energy_j"]
        print(f"\nEnergy vs Baseline: R1={sav1:+.1f}%, R2={sav2:+.1f}%")
    
    if z1["avg_lat"] > 0:
        speedup = 100 * (z1["avg_lat"] - z2["avg_lat"]) / z1["avg_lat"]
        print(f"Cache Warmup (R2 vs R1): {speedup:+.1f}% faster")
    
    out = {"baseline": bl, "zeta1": z1, "zeta2": z2, "gpus": "4xA40"}
    with open("/runpod-volume/4xa40_results.json", "w") as f:
        json.dump(out, f, indent=2)
    print("\nSaved to /runpod-volume/4xa40_results.json")
    print("DONE")
