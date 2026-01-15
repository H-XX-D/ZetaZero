#!/usr/bin/env python3
"""
50-turn time-series benchmark
Tracks latency + energy over time for:
- Baseline llama.cpp (single prompts)
- Baseline llama.cpp (growing context)
- Zeta (fresh prompts)
"""

import argparse
import importlib.util
import json
import time
from pathlib import Path
import subprocess
import requests

REMOTE_HOST = "xx@192.168.0.165"
ZETA_URL = "http://192.168.0.165:8080"
BASELINE_URL = "http://192.168.0.165:8080"
POWER_SAMPLE_INTERVAL_S = 0.25


def ssh(cmd, timeout=60):
    return subprocess.run(
        ["ssh", "-n", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", REMOTE_HOST, cmd],
        capture_output=True,
        text=True,
        stdin=subprocess.DEVNULL,
        timeout=timeout,
    )


def start_baseline_server():
    ssh("pkill -9 -x zeta-zero-server 2>/dev/null || true")
    ssh("pkill -9 -x zeta-server 2>/dev/null || true")
    ssh("pkill -9 -x llama-server 2>/dev/null || true")
    time.sleep(2)
    cmd = (
        "cd ~/ZetaZero/llama.cpp/build/bin && "
        "nohup ./llama-server -m ~/models/qwen2.5-14b-instruct-q4_k_m.gguf "
        "--port 8080 --host 0.0.0.0 -ngl 999 -c 4096 "
        "> ~/baseline_server.log 2>&1 < /dev/null & echo started"
    )
    ssh(cmd, timeout=10)
    for _ in range(90):
        time.sleep(1)
        try:
            r = requests.get(f"{BASELINE_URL}/health", timeout=3)
            if r.status_code == 200:
                return True
        except Exception:
            pass
    return False


def start_zeta_server():
    ssh("pkill -9 -x zeta-zero-server 2>/dev/null || true")
    ssh("pkill -9 -x zeta-server 2>/dev/null || true")
    ssh("pkill -9 -x llama-server 2>/dev/null || true")
    time.sleep(2)
    cmd = (
        "cd ~/ZetaZero && "
        "nohup ./llama.cpp/build/bin/zeta-zero-server "
        "> ~/ZetaZero/logs/zeta_benchmark.log 2>&1 < /dev/null & echo started"
    )
    ssh(cmd, timeout=10)
    for _ in range(90):
        time.sleep(1)
        try:
            r = requests.get(f"{ZETA_URL}/health", timeout=3)
            if r.status_code == 200:
                return True
        except Exception:
            pass
    return False


def get_gpu_power():
    try:
        r = ssh("nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits", timeout=10)
        if r.returncode == 0:
            return float(r.stdout.strip())
    except Exception:
        pass
    return 0.0


class PowerSampler:
    def __init__(self, interval_s=POWER_SAMPLE_INTERVAL_S):
        self.interval_s = interval_s
        self.samples = []
        self._stop = False

    def start(self):
        self.samples = []
        self._stop = False
        import threading
        def _run():
            while not self._stop:
                p = get_gpu_power()
                if p > 0:
                    self.samples.append((time.time(), p))
                time.sleep(self.interval_s)
        self._thread = threading.Thread(target=_run, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop = True
        if getattr(self, "_thread", None):
            self._thread.join(timeout=2)

    def energy_j(self):
        if len(self.samples) < 2:
            return 0.0, 0.0, 0.0
        total = 0.0
        for i in range(1, len(self.samples)):
            t0, p0 = self.samples[i - 1]
            t1, p1 = self.samples[i]
            dt = max(0.0, t1 - t0)
            total += (p0 + p1) * 0.5 * dt
        avg_power = total / max(self.samples[-1][0] - self.samples[0][0], 1e-6)
        peak_power = max(p for _, p in self.samples)
        return total, avg_power, peak_power


def load_turns():
    bench_path = Path("benchmarks/50_turn_gkv_benchmark.py")
    spec = importlib.util.spec_from_file_location("bench_50_turn", bench_path)
    bench = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bench)
    return bench.CONVERSATION_TURNS


def run_mode(name, url, turns, growing_context=False):
    results = []
    cumulative_energy = 0.0
    messages = []

    for i, turn in enumerate(turns, 1):
        if growing_context:
            messages.append({"role": "user", "content": turn})
            payload_messages = messages
        else:
            payload_messages = [{"role": "user", "content": turn}]

        sampler = PowerSampler()
        sampler.start()
        start = time.time()
        try:
            r = requests.post(
                f"{url}/v1/chat/completions",
                json={
                    "model": "llama" if "baseline" in name else "zeta",
                    "messages": payload_messages,
                    "max_tokens": 150,
                    "temperature": 0.7,
                },
                timeout=180,
            )
            elapsed = time.time() - start
            sampler.stop()
            energy_j, avg_power, peak_power = sampler.energy_j()
            cumulative_energy += energy_j

            if r.status_code == 200:
                data = r.json()
                usage = data.get("usage", {})
                content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
                results.append({
                    "turn": i,
                    "time_s": elapsed,
                    "energy_j": energy_j,
                    "energy_j_cumulative": cumulative_energy,
                    "power_avg_w": avg_power,
                    "power_peak_w": peak_power,
                    "prompt_tokens": usage.get("prompt_tokens", 0),
                    "completion_tokens": usage.get("completion_tokens", 0),
                    "total_tokens": usage.get("total_tokens", 0),
                    "response_preview": content[:100],
                })
                print(f"[{name}] {i:02d} {elapsed:.2f}s {avg_power:.0f}W")
            else:
                results.append({
                    "turn": i,
                    "time_s": elapsed,
                    "energy_j": energy_j,
                    "energy_j_cumulative": cumulative_energy,
                    "error": r.text[:120],
                })
                print(f"[{name}] {i:02d} {elapsed:.2f}s ERROR")
        except Exception as e:
            sampler.stop()
            elapsed = time.time() - start
            results.append({
                "turn": i,
                "time_s": elapsed,
                "energy_j": 0.0,
                "energy_j_cumulative": cumulative_energy,
                "error": str(e),
            })
            print(f"[{name}] {i:02d} {elapsed:.2f}s EXCEPTION")

    summary = {
        "total_time_s": round(sum(r.get("time_s", 0) for r in results), 2),
        "total_energy_j": round(cumulative_energy, 1),
        "total_energy_wh": round(cumulative_energy / 3600, 4),
    }
    return {"name": name, "summary": summary, "results": results}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--modes",
        nargs="+",
        default=["baseline_single", "baseline_growing_context", "zeta_fresh"],
        choices=["baseline_single", "baseline_growing_context", "zeta_fresh"],
    )
    parser.add_argument("--skip-baseline-start", action="store_true")
    parser.add_argument("--skip-zeta-start", action="store_true")
    args = parser.parse_args()

    turns = load_turns()
    out_path = Path("benchmarks/50_turn_timeseries_results.json")
    if out_path.exists():
        output = json.loads(out_path.read_text())
    else:
        output = {
            "benchmark": "50-turn time-series",
            "date": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "modes": {}
        }

    if any(m.startswith("baseline") for m in args.modes) and not args.skip_baseline_start:
        if not start_baseline_server():
            raise SystemExit("Baseline server failed to start")

    if "baseline_single" in args.modes:
        output["modes"]["baseline_single"] = run_mode(
            "baseline_single", BASELINE_URL, turns, growing_context=False
        )

    if "baseline_growing_context" in args.modes:
        output["modes"]["baseline_growing_context"] = run_mode(
            "baseline_growing_context", BASELINE_URL, turns, growing_context=True
        )

    if "zeta_fresh" in args.modes:
        if not args.skip_zeta_start:
            if not start_zeta_server():
                raise SystemExit("Zeta server failed to start")
        output["modes"]["zeta_fresh"] = run_mode(
            "zeta_fresh", ZETA_URL, turns, growing_context=False
        )

    out_path.write_text(json.dumps(output, indent=2))
    print(f"Saved to {out_path}")
