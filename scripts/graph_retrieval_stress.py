#!/usr/bin/env python3
"""Graph retrieval stress test for Z.E.T.A.

Focus:
- Seed a few canary facts/snippets into memory (graph/TRM context enabled)
- Hit /generate concurrently with retrieval-style questions
- Measure correctness rate + latency and observe graph_nodes/graph_edges changes

This is intentionally "black box" (HTTP-only) so it works against remote Z6.

Example:
  python3 scripts/graph_retrieval_stress.py --base-url http://192.168.0.165:8080 --concurrency 20 --requests 200

Note: This script does NOT reset the graph by default because the point of the
graph/GKV system is to reuse compute across runs. Use --reset-graph only when
you intentionally want a clean-slate benchmark.
"""

from __future__ import annotations

import argparse
import json
import random
import statistics
import string
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from typing import Any
from uuid import uuid4

import requests


def _rand_canary(prefix: str = "CANARY") -> str:
    alphabet = string.ascii_uppercase + string.digits
    return f"{prefix}-" + "".join(random.choice(alphabet) for _ in range(8))


def _post_json(session: requests.Session, url: str, payload: dict[str, Any], timeout_s: float) -> dict[str, Any]:
    r = session.post(url, json=payload, timeout=timeout_s)
    r.raise_for_status()
    if not r.text:
        return {}
    return r.json()


def _get_json(session: requests.Session, url: str, timeout_s: float) -> dict[str, Any]:
    r = session.get(url, timeout=timeout_s)
    r.raise_for_status()
    if not r.text:
        return {}
    return r.json()


@dataclass
class Case:
    name: str
    prompt: str
    expect: str
    max_tokens: int

    def score(self, output: str) -> bool:
        # Conservative: require the expected substring, but normalize whitespace
        # so we don't count formatting-only differences as failures.
        norm_expect = "".join(self.expect.split())
        norm_output = "".join(output.split())
        return norm_expect in norm_output


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base-url", default="http://192.168.0.165:8080")
    ap.add_argument("--concurrency", type=int, default=20)
    ap.add_argument("--requests", type=int, default=200, help="Total retrieval queries")
    ap.add_argument("--timeout", type=float, default=30.0)
    ap.add_argument("--seed-sleep", type=float, default=0.25, help="Seconds between seed calls")
    ap.add_argument(
        "--reset-graph",
        action="store_true",
        default=False,
        help="Reset the graph before seeding (deletes stored memory/compute cache)",
    )
    ap.add_argument(
        "--no-reset-graph",
        action="store_false",
        dest="reset_graph",
        help="Do not reset the graph (default)",
    )
    args = ap.parse_args()

    base = args.base_url.rstrip("/")
    url_health = f"{base}/health"
    url_reset = f"{base}/graph/reset"
    url_mode = f"{base}/mode"
    url_generate = f"{base}/generate"

    canary_a = _rand_canary("CANARYA")
    canary_b = _rand_canary("CANARYB")

    # Code facts taken from the repo (and already validated earlier in this session)
    mode_enum_order = "CHAT, CREATIVE, RESEARCH, CODE, DISCOVERY, DREAM, IDEAS"
    mode_post_accepts = "CHAT, CREATIVE, RESEARCH, CODE, DISCOVERY, IDEAS"  # DREAM is a legacy alias

    seeds = [
        f"Remember this EXACT token for later: {canary_a}",
        f"Remember this EXACT token for later: {canary_b}",
        "In zeta-mode-controller.h, the Mode enum values in order are: " + mode_enum_order,
        "In zeta-server.cpp, POST /mode accepts: " + mode_post_accepts + " (and DREAM as a legacy alias mapping to IDEAS)",
    ]

    cases: list[Case] = [
        Case(
            name="canary_a",
            prompt=f"What is the exact token we stored as CanaryA? Output only the token.",
            expect=canary_a,
            max_tokens=16,
        ),
        Case(
            name="canary_b",
            prompt=f"What is the exact token we stored as CanaryB? Output only the token.",
            expect=canary_b,
            max_tokens=16,
        ),
        Case(
            name="mode_enum_order",
            prompt=(
                "From the Mode enum we stored, output the mode names in order as a comma-separated list. "
                "Output only the list."
            ),
            expect=mode_enum_order,
            max_tokens=64,
        ),
        Case(
            name="mode_post_accepts",
            prompt=(
                "From the /mode endpoint info we stored, list the accepted canonical mode names "
                "(exclude the legacy alias) as a comma-separated list. Output only the list."
            ),
            expect=mode_post_accepts,
            max_tokens=64,
        ),
    ]

    with requests.Session() as s:
        # Basic reachability
        health0 = _get_json(s, url_health, timeout_s=args.timeout)

        if args.reset_graph:
            _get_json(s, url_reset, timeout_s=args.timeout)

        # Put into CHAT mode for more consistent retrieval behavior.
        _post_json(s, url_mode, {"mode": "CHAT"}, timeout_s=args.timeout)

        # Seed
        for text in seeds:
            _post_json(
                s,
                url_generate,
                {
                    "mode": "chat",
                    "prompt": f"Memorize this for later retrieval. Reply only OK.\n\n{text}",
                    "max_tokens": 24,
                    "no_context": False,
                    "skip_hrm": True,
                },
                timeout_s=args.timeout,
            )
            time.sleep(args.seed_sleep)

        health1 = _get_json(s, url_health, timeout_s=args.timeout)

    # Prepare query workload
    total = args.requests
    if total < 1:
        raise SystemExit("--requests must be >= 1")

    rng = random.Random(0)
    workload = [rng.choice(cases) for _ in range(total)]

    def run_one(case: Case) -> tuple[str, bool, float, str]:
        started = time.perf_counter()
        try:
            # TRM safety blocks exact repeated prompts after a few repeats.
            # Add a nonce so we can stress retrieval without tripping that guardrail.
            nonce = uuid4().hex
            prompt = f"{case.prompt}\n\nnonce={nonce}"
            with requests.Session() as s2:
                resp = _post_json(
                    s2,
                    url_generate,
                    {
                        "mode": "chat",
                        "prompt": prompt,
                        "max_tokens": case.max_tokens,
                        "no_context": False,
                        "skip_hrm": True,
                    },
                    timeout_s=args.timeout,
                )
            out = (resp.get("output") or "").strip()
            ok = case.score(out)
            elapsed = time.perf_counter() - started
            return case.name, ok, elapsed, out
        except Exception as e:
            elapsed = time.perf_counter() - started
            return case.name, False, elapsed, f"ERROR: {e}"

    results: list[tuple[str, bool, float, str]] = []
    t0 = time.perf_counter()
    with ThreadPoolExecutor(max_workers=args.concurrency) as ex:
        futs = [ex.submit(run_one, c) for c in workload]
        for f in as_completed(futs):
            results.append(f.result())
    total_elapsed = time.perf_counter() - t0

    # Summaries
    by_case: dict[str, list[tuple[bool, float]]] = {}
    for name, ok, elapsed, _ in results:
        by_case.setdefault(name, []).append((ok, elapsed))

    overall_ok = sum(1 for _, ok, _, _ in results if ok)
    overall_rate = overall_ok / len(results) if results else 0.0

    latencies = [elapsed for _, _, elapsed, _ in results]
    p50 = statistics.median(latencies) if latencies else 0.0
    p95 = statistics.quantiles(latencies, n=20)[18] if len(latencies) >= 20 else (max(latencies) if latencies else 0.0)

    print(f"base_url={base}")
    print(f"seed_canary_a={canary_a}")
    print(f"seed_canary_b={canary_b}")
    print(f"health_before_nodes={health0.get('graph_nodes')} health_before_edges={health0.get('graph_edges')}")
    print(f"health_after_seed_nodes={health1.get('graph_nodes')} health_after_seed_edges={health1.get('graph_edges')}")
    print(f"concurrency={args.concurrency} requests={len(results)} elapsed_s={total_elapsed:.2f}")
    print(f"overall_ok={overall_ok} overall_ok_rate={overall_rate:.3f}")
    print(f"latency_p50_s={p50:.3f} latency_p95_s={p95:.3f}")

    for name in sorted(by_case.keys()):
        items = by_case[name]
        ok_n = sum(1 for ok, _ in items if ok)
        rate = ok_n / len(items)
        lats = [x for _, x in items]
        med = statistics.median(lats)
        mx = max(lats)
        print(f"case={name} ok={ok_n}/{len(items)} rate={rate:.3f} p50_s={med:.3f} max_s={mx:.3f}")

    # Show a couple failures to diagnose retrieval issues
    failures = [(n, o) for (n, ok, _, o) in results if not ok][:6]
    if failures:
        print("fail_samples=")
        for name, out in failures:
            out_one = " ".join(out.split())
            if len(out_one) > 200:
                out_one = out_one[:197] + "..."
            print(f"  - {name}: {out_one}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
