#!/usr/bin/env python3
"""Energy over time benchmark - 5 runs each for Zeta and Baseline"""
import json, time, requests, subprocess

def pw():
    try:
        r = subprocess.run(["nvidia-smi","--query-gpu=power.draw","--format=csv,noheader,nounits"],
                          capture_output=True, text=True, timeout=5)
        return float(r.stdout.strip()) if r.returncode == 0 else 0
    except: return 0

def turn(url, q, h):
    m = h[-10:] + [dict(role="user", content=q)]
    p0, t0 = pw(), time.time()
    r = requests.post(url + "/v1/chat/completions",
                     json=dict(model="llama", messages=m, max_tokens=150), timeout=120)
    lat = time.time() - t0
    p1 = pw()
    e = (p0 + p1) / 2 * lat
    if r.ok:
        d = r.json()
        return dict(ok=1, lat=lat, tok=d.get("usage",{}).get("total_tokens",0), e=e, pwr=(p0+p1)/2)
    return dict(ok=0, lat=lat, e=e, pwr=0)

Q = ["Explain backpropagation", "REST vs GraphQL", "How does GC work", "CAP theorem",
     "Concurrency vs parallelism", "TLS handshake", "Microservices pros/cons",
     "Database indexes", "Eventual consistency", "Bloom filters"]

def run_pass(name, url, queries):
    print(f"\n=== {name} ===")
    res, h, E = [], [], 0
    for i, q in enumerate(queries):
        r = turn(url, q, h)
        res.append(r)
        if r["ok"]:
            E += r["e"]
            h += [dict(role="user", content=q), dict(role="assistant", content="ok")]
        print(f"[{i+1:02d}] {r['lat']:5.2f}s {r['pwr']:5.0f}W {r['e']:6.1f}J")
    ok = [x for x in res if x["ok"]]
    return dict(ok=len(ok), time=sum(x["lat"] for x in ok), E=E,
                avg_lat=sum(x["lat"] for x in ok)/len(ok) if ok else 0)

print("=" * 60)
print("ENERGY OVER TIME TEST - 5 Runs Each")
print("=" * 60)

zeta_runs = []
base_runs = []

for run in range(1, 6):
    print(f"\n{'='*60}")
    print(f"RUN {run}/5")
    print("=" * 60)
    z = run_pass(f"Zeta-R{run}", "http://localhost:8080", Q)
    b = run_pass(f"Base-R{run}", "http://localhost:9000", Q)
    zeta_runs.append(z)
    base_runs.append(b)
    sav = 100 * (b['E'] - z['E']) / b['E'] if b['E'] > 0 else 0
    print(f"\nRun {run}: Zeta={z['E']:.0f}J vs Base={b['E']:.0f}J -> {sav:+.1f}% savings")

print("\n" + "=" * 60)
print("FINAL RESULTS")
print("=" * 60)
print(f"{'Run':<8} {'Zeta E(J)':<12} {'Base E(J)':<12} {'Savings':<12} {'Zeta Lat':<12} {'Base Lat':<12}")
print("-" * 72)
for i, (z, b) in enumerate(zip(zeta_runs, base_runs), 1):
    sav = 100 * (b['E'] - z['E']) / b['E'] if b['E'] > 0 else 0
    print(f"{i:<8} {z['E']:<12.0f} {b['E']:<12.0f} {sav:+.1f}%{'':<6} {z['avg_lat']:<12.2f} {b['avg_lat']:<12.2f}")

z_total = sum(r['E'] for r in zeta_runs)
b_total = sum(r['E'] for r in base_runs)
print("-" * 72)
print(f"{'TOTAL':<8} {z_total:<12.0f} {b_total:<12.0f} {100*(b_total-z_total)/b_total:+.1f}%")

json.dump(dict(zeta=zeta_runs, baseline=base_runs, total_zeta_j=z_total, total_baseline_j=b_total),
          open("/runpod-volume/energy_over_time.json", "w"), indent=2)
print("\nDONE")
