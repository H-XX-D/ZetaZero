#!/usr/bin/env python3
"""20-turn triple benchmark: Baseline vs Zeta (2 runs) with KV cache ENABLED"""
import json, time, requests, subprocess

def get_power():
    try:
        r = subprocess.run(["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
                          capture_output=True, text=True, timeout=5)
        return float(r.stdout.strip()) if r.returncode == 0 else 0.0
    except:
        return 0.0

def run_turn(url, query, history):
    msgs = history[-10:] + [{"role": "user", "content": query}]
    p0 = get_power()
    t0 = time.time()
    try:
        r = requests.post(f"{url}/v1/chat/completions",
            json={"model": "llama", "messages": msgs, "max_tokens": 150, "temperature": 0.7},
            timeout=120)
        lat = time.time() - t0
        p1 = get_power()
        avg_power = (p0 + p1) / 2
        energy = avg_power * lat
        if r.status_code == 200:
            d = r.json()
            usage = d.get("usage", {})
            content = d.get("choices", [{}])[0].get("message", {}).get("content", "")
            return {"ok": True, "lat": lat, "tok": usage.get("total_tokens", 0), 
                    "pwr": avg_power, "e": energy, "resp": content[:60]}
        return {"ok": False, "lat": lat, "pwr": avg_power, "e": energy, "err": r.text[:100]}
    except Exception as ex:
        lat = time.time() - t0
        return {"ok": False, "lat": lat, "pwr": 0, "e": 0, "err": str(ex)}

QUESTIONS = [
    "Backprop", "REST vs GraphQL", "GC", "CAP", "Concurrency",
    "TLS", "Microservices", "Indexes", "Eventual consistency", "Bloom filter"
] * 2  # 20 turns

def run_benchmark(name, url):
    print(f"\n=== {name} ===")
    results = []
    history = []
    total_energy = 0.0
    
    for i, q in enumerate(QUESTIONS, 1):
        r = run_turn(url, q, history)
        results.append(r)
        if r["ok"]:
            total_energy += r["e"]
            history.append({"role": "user", "content": q})
            history.append({"role": "assistant", "content": r.get("resp", "")})
        status = "OK" if r["ok"] else "FAIL"
        print(f"[{i:02d}] {r['lat']:.1f}s {r.get('pwr', 0):.0f}W {r['e']:.0f}J {status}")
    
    ok = [x for x in results if x["ok"]]
    avg_lat = sum(x["lat"] for x in ok) / len(ok) if ok else 0
    return {
        "name": name,
        "success": len(ok),
        "total": len(results),
        "time": sum(x["lat"] for x in ok),
        "avg_lat": avg_lat,
        "energy_j": total_energy,
        "tokens": sum(x.get("tok", 0) for x in ok)
    }

if __name__ == "__main__":
    print("=" * 60)
    print("KV-CACHE ENABLED BENCHMARK: Baseline vs Zeta (2 runs)")
    print("=" * 60)
    
    baseline = run_benchmark("Baseline", "http://localhost:9000")
    zeta1 = run_benchmark("Zeta-Run1", "http://localhost:8080")
    zeta2 = run_benchmark("Zeta-Run2", "http://localhost:8080")
    
    print("\n" + "=" * 60)
    print("RESULTS (KV Cache ENABLED)")
    print("=" * 60)
    print(f"{'Run':<12} {'Success':<10} {'Time(s)':<10} {'Avg(s)':<10} {'Energy(J)':<12}")
    print("-" * 60)
    print(f"{'Baseline':<12} {baseline['success']}/20      {baseline['time']:.0f}        {baseline['avg_lat']:.2f}       {baseline['energy_j']:.0f}")
    print(f"{'Zeta-R1':<12} {zeta1['success']}/20      {zeta1['time']:.0f}        {zeta1['avg_lat']:.2f}       {zeta1['energy_j']:.0f}")
    print(f"{'Zeta-R2':<12} {zeta2['success']}/20      {zeta2['time']:.0f}        {zeta2['avg_lat']:.2f}       {zeta2['energy_j']:.0f}")
    
    if baseline["energy_j"] > 0:
        r1_savings = (baseline["energy_j"] - zeta1["energy_j"]) / baseline["energy_j"] * 100
        r2_savings = (baseline["energy_j"] - zeta2["energy_j"]) / baseline["energy_j"] * 100
        print(f"\nEnergy Savings vs Baseline:")
        print(f"  Zeta-R1: {r1_savings:+.1f}%")
        print(f"  Zeta-R2: {r2_savings:+.1f}%")
    
    if zeta1["avg_lat"] > 0:
        speedup = (zeta1["avg_lat"] - zeta2["avg_lat"]) / zeta1["avg_lat"] * 100
        print(f"\nGraph Recall Speedup (R2 vs R1): {speedup:+.1f}%")
    
    output = {"baseline": baseline, "zeta1": zeta1, "zeta2": zeta2, "kv_cache": "ENABLED"}
    with open("/runpod-volume/kv_fixed_results.json", "w") as f:
        json.dump(output, f, indent=2)
    print("\nSaved to /runpod-volume/kv_fixed_results.json")
    print("DONE")
