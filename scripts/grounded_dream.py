#!/usr/bin/env python3
"""Generate a "grounded dream" from the current graph.

This script pulls the live graph via GET /graph, filters out garbage/token
nodes (e.g. <|im_start|> leakage), then prompts /generate using that filtered
context.

Usage:
  python3 scripts/grounded_dream.py --n 1
  python3 scripts/grounded_dream.py --url http://192.168.0.165:8080 --type code_idea

Output format matches the review tooling:
  line1: category
  line2: timestamp
  line3+: content
"""

from __future__ import annotations

import argparse
import os
import re
from datetime import datetime
from pathlib import Path
from typing import Dict, List

import requests

DEFAULT_URL = os.environ.get("ZETA_URL", "http://192.168.0.165:8080")

GARBAGE_PATTERNS = [
    re.compile(r"<\|im_(start|end)\|>", re.IGNORECASE),
    re.compile(r"<\|.*?\|>", re.IGNORECASE),
    re.compile(r"\b(im_start|im_end)\b", re.IGNORECASE),
]

DEFAULT_PROMPTS: Dict[str, str] = {
    "code_fix": "Propose a concrete code fix grounded in the provided graph context. Be specific: mention relevant nodes and what should change.",
    "code_idea": "Propose a concrete new capability grounded in the provided graph context. Be specific: mention relevant nodes and how to implement.",
    "insight": "Extract one high-leverage insight grounded in the provided graph context. Cite which nodes support it.",
}


def is_garbage(text: str) -> bool:
    if not text:
        return True
    for pat in GARBAGE_PATTERNS:
        if pat.search(text):
            return True
    return False


def fetch_graph(server_url: str) -> dict:
    r = requests.get(f"{server_url}/graph", timeout=30)
    r.raise_for_status()
    return r.json()


def select_nodes(graph: dict, max_nodes: int, min_value_len: int) -> List[dict]:
    nodes = graph.get("nodes", []) or []

    def score(n: dict) -> float:
        try:
            return float(n.get("salience", 0.0))
        except Exception:
            return 0.0

    selected: List[dict] = []
    for n in sorted(nodes, key=score, reverse=True):
        label = str(n.get("label", ""))
        value = str(n.get("value", ""))
        if is_garbage(label) or is_garbage(value):
            continue
        if len(value.strip()) < min_value_len:
            continue
        selected.append(n)
        if len(selected) >= max_nodes:
            break
    return selected


def build_context(nodes: List[dict], max_value_chars: int) -> str:
    lines: List[str] = []
    for n in nodes:
        node_id = n.get("id")
        label = str(n.get("label", ""))
        value = str(n.get("value", ""))
        if len(value) > max_value_chars:
            value = value[:max_value_chars] + "…"
        lines.append(f"[Node {node_id}] {label}: {value}")
    return "\n".join(lines)


def generate(server_url: str, prompt: str, max_tokens: int = 900) -> dict:
    r = requests.post(
        f"{server_url}/generate",
        json={"prompt": prompt, "max_tokens": max_tokens, "no_context": True},
        timeout=180,
    )
    r.raise_for_status()
    return r.json()


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate grounded dreams from graph context")
    ap.add_argument("--url", default=DEFAULT_URL, help="Z.E.T.A. server URL")
    ap.add_argument("--n", type=int, default=1, help="Number of dreams to generate")
    ap.add_argument("--type", default="code_idea", choices=sorted(DEFAULT_PROMPTS.keys()), help="Dream category")
    ap.add_argument("--out-dir", default=os.path.expanduser("~/ZetaOne/data/grounded_dreams"), help="Output directory")
    ap.add_argument("--max-nodes", type=int, default=25, help="Max graph nodes to include")
    ap.add_argument("--min-value-len", type=int, default=30, help="Minimum node value length")
    ap.add_argument("--max-value-chars", type=int, default=320, help="Max chars per node value")
    ap.add_argument("--max-tokens", type=int, default=900, help="Max tokens for /generate")
    args = ap.parse_args()

    # Quick health check
    try:
        health = requests.get(f"{args.url}/health", timeout=5).json()
        print(f"Connected: {args.url} | status={health.get('status')} | nodes={health.get('graph_nodes')} edges={health.get('graph_edges')}")
    except Exception as e:
        print(f"ERROR: cannot reach {args.url}/health: {e}")
        return 2

    Path(args.out_dir).mkdir(parents=True, exist_ok=True)

    for _ in range(max(1, args.n)):
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        graph = fetch_graph(args.url)
        nodes = select_nodes(graph, max_nodes=args.max_nodes, min_value_len=args.min_value_len)
        context = build_context(nodes, max_value_chars=args.max_value_chars)

        task = DEFAULT_PROMPTS[args.type]
        prompt = (
            "You are Z.E.T.A. running a grounded dream cycle.\n"
            "Only use the provided GRAPH CONTEXT; do not invent code entities.\n\n"
            "GRAPH CONTEXT:\n"
            f"{context}\n\n"
            "TASK:\n"
            f"{task}\n\n"
            "Output: a concise, actionable proposal with 3-7 bullets.\n"
        )

        resp = generate(args.url, prompt, max_tokens=args.max_tokens)
        text = resp.get("output", "").replace("\\n", "\n").strip()

        out_path = Path(args.out_dir) / f"grounded_{ts}_{args.type}.txt"
        out_path.write_text(f"{args.type}\n{ts}\n{text}\n", encoding="utf-8")
        print(f"Wrote: {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
