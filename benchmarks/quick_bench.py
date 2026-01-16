#!/usr/bin/env python3
"""Quick 10-question benchmark: Zeta R1, Zeta R2, Baseline"""
import json, time, requests, subprocess, threading

def pw():
    try:
        r = subprocess.run(["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
                          capture_output=True, text=True)
        return float(r.stdout.strip()) if r.returncode == 0 else 0
    except:
        return 0

class PowerSampler:
    def __init__(self):
        self.samples = []
        self.stop_flag = False
    
    def start(self):
        self.samples = []
        self.stop_flag = False
        def run():
            while not self.stop_flag:
                p = pw()
                if p > 0:
                    self.samples.append((time.time(), p))
                time.sleep(0.1)
        self.thread = threading.Thread(target=run, daemon=True)
        self.thread.start()
    
    def stop(self):
        self.stop_flag = True
        self.thread.join(2)
    
    def energy(self):
        if len(self.samples) < 2:
            return 0
        total = 0
        for i in range(1, len(self.samples)):
            dt = self.samples[i][0] - self.samples[i-1][0]
            total += (self.samples[i-1][1] + self.samples[i][1]) / 2 * dt
        return total

def turn(url, query, history):
    msgs = history[-6:] + [{"role": "user", "content": query}]
    ps = PowerSampler()
    ps.start()
    t0 = time.time()
    try:
        r = requests.post(
            f"{url}/v1/chat/completions",
            json={"model": "llama", "messages": msgs, "max_tokens": 200, "temperature": 0.7},
            timeout=180
        )
        lat = time.time() - t0
        ps.stop()
        en = ps.energy()
        if r.ok:
            d = r.json()
            return {
                "ok": True,
                "lat": lat,
                "tok": d.get("usage", {}).get("total_tokens", 0),
                "e": en,
                "resp": d["choices"][0]["message"]["content"][:60]
            }
        return {"ok": False, "lat": lat, "e": en}
    except Exception as ex:
        ps.stop()
        return {"ok": False, "lat": time.time() - t0, "e": ps.energy(), "err": str(ex)}

QUESTIONS = [
    "Explain backpropagation in neural networks",
    "What is the CAP theorem?",
    "How do B-trees work?",
    "Explain eventual consistency",
    "What are microservices?",
    "How does TLS handshake work?",
    "What is a bloom filter?",
    "Explain the actor model",
    "How do LSM trees work?",
    "What is consensus in distributed systems?"
]

def bench(name, url):
    print(f"=== {name} ===")
    results = []
    history = []
    total_energy = 0
    
    for i, q in enumerate(QUESTIONS):
        r = turn(url, q, history)
        results.append(r)
        if r["ok"]:
            total_energy += r["e"]
            history.append({"role": "user", "content": q})
            history.append({"role": "assistant", "content": r.get("resp", "")})
        status = "OK" if r["ok"] else "X"
        print(f"[{i+1:02d}] {r['lat']:5.2f}s {r['e']:6.0f}J {r.get('tok', 0):4d}tok {status}")
    
    ok = [x for x in results if x["ok"]]
    return {
        "name": name,
        "ok": len(ok),
        "time": sum(x["lat"] for x in ok),
        "avg": sum(x["lat"] for x in ok) / len(ok) if ok else 0,
        "energy": total_energy,
        "tokens": sum(x.get("tok", 0) for x in ok)
    }

if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("ZETA BENCHMARK - 10 questions x 2 runs + Baseline")
    print("=" * 60)
    
    z1 = bench("Zeta-Run1", "http://localhost:8080")
    z2 = bench("Zeta-Run2", "http://localhost:8080")
    b = bench("Baseline", "http://localhost:9000")
    
    print("\n" + "=" * 60)
    print("RESULTS")
    print("=" * 60)
    print(f"{'Run':<12} {'Time':>7} {'Avg':>6} {'Energy':>7} {'Tok':>6}")
    print("-" * 45)
    print(f"{'Zeta-R1':<12} {z1['time']:>7.1f} {z1['avg']:>6.2f} {z1['energy']:>7.0f} {z1['tokens']:>6d}")
    print(f"{'Zeta-R2':<12} {z2['time']:>7.1f} {z2['avg']:>6.2f} {z2['energy']:>7.0f} {z2['tokens']:>6d}")
    print(f"{'Baseline':<12} {b['time']:>7.1f} {b['avg']:>6.2f} {b['energy']:>7.0f} {b['tokens']:>6d}")
    
    if b["energy"] > 0:
        print("\nEnergy Savings:")
        print(f"  R1 vs Base: {(b['energy'] - z1['energy']) / b['energy'] * 100:.1f}%")
        print(f"  R2 vs Base: {(b['energy'] - z2['energy']) / b['energy'] * 100:.1f}%")
    
    if z1["time"] > 0:
        print("\nGraph-KV Warmup Effect:")
        print(f"  R2 vs R1: {(z1['time'] - z2['time']) / z1['time'] * 100:.1f}% faster")
    
    result = {"z1": z1, "z2": z2, "baseline": b, "date": time.strftime("%Y-%m-%d %H:%M:%S")}
    with open("/runpod-volume/latest_bench.json", "w") as f:
        json.dump(result, f, indent=2)
    
    print("\nDONE")
