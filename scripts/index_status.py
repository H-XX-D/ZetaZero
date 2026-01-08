#!/usr/bin/env python3
"""Check whether code indexing/seeding has already been done.

This is a lightweight status probe for a running Z.E.T.A. server.
It uses:
- GET /git/status  (node counts, branch)
- GET /git/log     (recent commit labels)

Heuristic:
- If recent commits include labels like "file", "function", ... or "file_excerpt:*",
  then indexing/seeding has been run at least once.

Usage:
  python3 scripts/index_status.py --url http://192.168.0.165:8080
"""

from __future__ import annotations

import argparse
import os
from collections import Counter
from typing import Any, Dict, List

import requests

DEFAULT_URL = os.environ.get("ZETA_URL", "http://192.168.0.165:8080")

CODE_LABELS = {"file", "function", "struct", "class", "enum"}


def get_json(url: str, path: str, params: Dict[str, Any] | None = None, timeout: int = 10) -> Dict[str, Any]:
    r = requests.get(f"{url}{path}", params=params, timeout=timeout)
    r.raise_for_status()
    return r.json()


def main() -> int:
    ap = argparse.ArgumentParser(description="Check whether indexing/seeding has been done")
    ap.add_argument("--url", default=DEFAULT_URL, help="Z.E.T.A. server URL")
    ap.add_argument("--count", type=int, default=200, help="How many git log entries to inspect")
    args = ap.parse_args()

    try:
        status = get_json(args.url, "/git/status", timeout=10)
    except Exception as e:
        print(f"ERROR: cannot query {args.url}/git/status: {e}")
        return 2

    branch = status.get("branch", "?")
    total_nodes = status.get("total_nodes", "?")
    branch_commits = status.get("branch_commits", "?")
    ahead = status.get("ahead", "?")
    parent = status.get("parent", "?")

    try:
        log = get_json(args.url, "/git/log", params={"count": args.count}, timeout=20)
        commits: List[Dict[str, Any]] = log.get("commits", []) or []
    except Exception as e:
        commits = []
        print(f"WARN: cannot query {args.url}/git/log: {e}")

    counts = Counter()
    excerpt_count = 0
    code_entity_count = 0

    for c in commits:
        label = str(c.get("label", ""))
        if not label:
            continue
        if label.startswith("file_excerpt:"):
            excerpt_count += 1
        if label in CODE_LABELS:
            code_entity_count += 1
        counts[label] += 1

    indexed = (excerpt_count > 0) or (code_entity_count > 0)

    print("Index status")
    print("===========")
    print(f"Server: {args.url}")
    print(f"Branch: {branch} (ahead={ahead} parent={parent})")
    print(f"Total nodes: {total_nodes}")
    print(f"Branch commits: {branch_commits}")
    print(f"Recent commits scanned: {len(commits)}")
    print(f"Recent file excerpts: {excerpt_count}")
    print(f"Recent code entity commits: {code_entity_count}")
    print(f"Indexed before (heuristic): {'YES' if indexed else 'NO'}")

    if commits:
        print("\nRecent commit label sample:")
        for label, n in counts.most_common(10):
            print(f"  {label}: {n}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
