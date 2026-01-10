#!/usr/bin/env python3
"""ZetaZero system smoke test.

Goal: a fast end-to-end validation of the running Zeta server’s core surface:
- Health + mode
- OpenAI compatibility (/v1/models, /v1/chat/completions)
- Native generation (/generate)
- Graph inspection (/graph)
- Memory query (/memory/query)
- Dataset ingestion (/extract_text_7b)
- Code extraction (/extract_code_7b)

Defaults are intentionally NON-DESTRUCTIVE:
- Does not reset the graph unless --reset-graph is provided.
- Does not commit ingestion artifacts unless --commit-test-artifacts is provided.

Usage:
  ZETA_BASE_URL=http://192.168.0.165:9000 python3 scripts/zeta_system_smoke_test.py
  python3 scripts/zeta_system_smoke_test.py --base-url http://localhost:8080
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path
from dataclasses import dataclass
from typing import Any, Dict

try:
    import requests
except Exception as e:  # pragma: no cover
    print("ERROR: missing dependency 'requests'.")
    print("Install with: pip3 install requests")
    print(f"Import error: {e}")
    sys.exit(2)


@dataclass
class CheckResult:
    name: str
    ok: bool
    detail: str = ""


def _load_config_from_zeta_conf(repo_root: Path) -> Dict[str, str]:
    conf_path = repo_root / "zeta.conf"
    if not conf_path.exists():
        return {}

    config: Dict[str, str] = {}
    for raw in conf_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()

        # Strip trailing comments for unquoted values
        if not value.startswith(('"', "'")) and "#" in value:
            value = value.split("#", 1)[0].strip()

        # Strip wrapping quotes
        if (value.startswith('"') and '"' in value[1:]) or (value.startswith("'") and "'" in value[1:]):
            quote = value[0]
            end = value.find(quote, 1)
            if end > 0:
                value = value[1:end]
            else:
                value = value.strip(quote)

        # Simple ${VAR} expansion using what we've parsed so far
        # (good enough for ${ZETA_PORT} used by ZETA_URL)
        for k, v in list(config.items()):
            value = value.replace(f"${{{k}}}", v)

        config[key] = value

    # Second pass expansion in case order differs
    for key, value in list(config.items()):
        for k, v in list(config.items()):
            value = value.replace(f"${{{k}}}", v)
        config[key] = value

    return config


def default_base_url() -> str:
    # Env overrides first
    for k in ("ZETA_BASE_URL", "ZETA_URL"):
        v = os.environ.get(k)
        if v:
            return v

    # Repo config next
    repo_root = Path(__file__).resolve().parents[1]
    conf = _load_config_from_zeta_conf(repo_root)
    if conf.get("ZETA_URL"):
        return conf["ZETA_URL"]
    if conf.get("ZETA_HOST") and conf.get("ZETA_PORT"):
        host = conf["ZETA_HOST"]
        # If configured to bind all interfaces, the most useful client default is localhost
        if host == "0.0.0.0":
            host = "localhost"
        return f"http://{host}:{conf['ZETA_PORT']}"

    # Fallback
    return "http://localhost:8080"


def _get_json(resp: requests.Response) -> Dict[str, Any]:
    try:
        return resp.json()
    except Exception:
        raise AssertionError(f"Expected JSON, got: {resp.text[:200]}")


def _assert(cond: bool, msg: str) -> None:
    if not cond:
        raise AssertionError(msg)


def _url(base_url: str, path: str) -> str:
    return base_url.rstrip("/") + path


def check_health(base_url: str, timeout_s: float) -> Dict[str, Any]:
    r = requests.get(_url(base_url, "/health"), timeout=timeout_s)
    _assert(r.status_code == 200, f"/health HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("status") == "ok", f"/health status != ok: {j}")
    _assert("version" in j, f"/health missing version: {j}")
    _assert(isinstance(j.get("graph_nodes"), int), f"/health graph_nodes not int: {j}")
    _assert(isinstance(j.get("graph_edges"), int), f"/health graph_edges not int: {j}")
    return j


def check_mode(base_url: str, timeout_s: float) -> Dict[str, Any]:
    r = requests.get(_url(base_url, "/mode"), timeout=timeout_s)
    _assert(r.status_code == 200, f"/mode HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(isinstance(j.get("mode"), str) and j["mode"], f"/mode missing mode: {j}")
    _assert(isinstance(j.get("branching_level"), str), f"/mode missing branching_level: {j}")
    return j


def check_mode_switching(base_url: str, timeout_s: float) -> Dict[str, Any]:
    # Ensure the server accepts and reports all supported modes.
    # Keep this non-destructive by restoring CHAT at the end.
    modes = ["CHAT", "RESEARCH", "CODE", "CREATIVE", "DISCOVERY", "IDEAS"]
    seen: Dict[str, Any] = {}

    for m in modes:
        r = requests.post(_url(base_url, "/mode"), json={"mode": m}, timeout=timeout_s)
        _assert(r.status_code == 200, f"/mode (set {m}) HTTP {r.status_code}: {r.text[:200]}")
        j = _get_json(r)
        _assert(j.get("status") == "ok", f"/mode (set {m}) status != ok: {j}")

        cur = check_mode(base_url, timeout_s)
        _assert(cur.get("mode") == m, f"/mode mismatch after set {m}: {cur}")
        seen[m] = cur

    # Restore CHAT for subsequent checks.
    r = requests.post(_url(base_url, "/mode"), json={"mode": "CHAT"}, timeout=timeout_s)
    _assert(r.status_code == 200, f"/mode (restore CHAT) HTTP {r.status_code}: {r.text[:200]}")
    _assert(_get_json(r).get("status") == "ok", f"/mode restore CHAT failed: {r.text[:200]}")
    return {"status": "ok", "modes": list(seen.keys())}


def check_v1_models(base_url: str, timeout_s: float) -> Dict[str, Any]:
    r = requests.get(_url(base_url, "/v1/models"), timeout=timeout_s)
    _assert(r.status_code == 200, f"/v1/models HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("object") == "list", f"/v1/models object != list: {j}")
    _assert(isinstance(j.get("data"), list), f"/v1/models data not list: {j}")
    ids = {m.get("id") for m in j.get("data", []) if isinstance(m, dict)}
    _assert("zeta-cognitive" in ids, f"/v1/models missing zeta-cognitive: {ids}")
    return j


def check_chat_completions(base_url: str, timeout_s: float) -> Dict[str, Any]:
    payload = {
        "model": "zeta-cognitive",
        "messages": [
            {"role": "system", "content": "You are a helpful assistant."},
            {"role": "user", "content": "Reply with exactly: OK"},
        ],
        "max_tokens": 16,
        "temperature": 0.0,
    }
    r = requests.post(_url(base_url, "/v1/chat/completions"), json=payload, timeout=timeout_s)
    _assert(r.status_code == 200, f"/v1/chat/completions HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("object") == "chat.completion", f"chat completion object mismatch: {j}")
    choices = j.get("choices")
    _assert(isinstance(choices, list) and choices, f"chat completion missing choices: {j}")
    msg = choices[0].get("message") if isinstance(choices[0], dict) else None
    _assert(isinstance(msg, dict), f"chat completion missing message: {j}")
    _assert(msg.get("role") == "assistant", f"chat completion role != assistant: {j}")
    _assert(isinstance(msg.get("content"), str), f"chat completion content not string: {j}")
    return j


def check_generate(base_url: str, timeout_s: float) -> Dict[str, Any]:
    payload = {"prompt": "Say OK", "max_tokens": 32, "skip_hrm": True, "no_context": True}
    r = requests.post(_url(base_url, "/generate"), json=payload, timeout=timeout_s)
    _assert(r.status_code == 200, f"/generate HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(isinstance(j.get("output"), str), f"/generate missing output: {j}")
    _assert(isinstance(j.get("tokens"), int), f"/generate missing tokens: {j}")
    return j


def check_graph(base_url: str, timeout_s: float) -> Dict[str, Any]:
    r = requests.get(_url(base_url, "/graph"), timeout=timeout_s)
    _assert(r.status_code == 200, f"/graph HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(isinstance(j.get("nodes"), list), f"/graph nodes not list: {j}")
    _assert(isinstance(j.get("edges"), list), f"/graph edges not list: {j}")
    return j


def check_memory_query(base_url: str, timeout_s: float) -> Dict[str, Any]:
    payload = {"query": "zeta", "top_k": 5}
    r = requests.post(_url(base_url, "/memory/query"), json=payload, timeout=timeout_s)
    _assert(r.status_code == 200, f"/memory/query HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)

    # Back/forward compatibility: historically some scripts expected 'nodes';
    # current server returns 'results'. We accept either.
    if "results" in j:
        _assert(isinstance(j.get("results"), list), f"/memory/query results not list: {j}")
    elif "nodes" in j:
        _assert(isinstance(j.get("nodes"), list), f"/memory/query nodes not list: {j}")
    else:
        raise AssertionError(f"/memory/query missing results/nodes: {j}")

    return j


def check_extract_text(
    base_url: str,
    timeout_s: float,
    commit_test_artifacts: bool,
) -> Dict[str, Any]:
    payload: Dict[str, Any] = {
        "text": "ZetaZero smoke test fact: the sky is blue.",
        "source_id": "zeta_system_smoke_test",
        "max_links": 5,
        "min_link_salience": 0.95,
        "min_value_len": 8,
    }

    # In this endpoint, `commit:false` disables document linking (and thus reduces graph noise).
    if not commit_test_artifacts:
        payload["commit"] = False

    r = requests.post(_url(base_url, "/extract_text_7b"), json=payload, timeout=timeout_s)
    _assert(r.status_code == 200, f"/extract_text_7b HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("status") == "ok", f"/extract_text_7b status != ok: {j}")
    _assert(isinstance(j.get("facts_created"), int), f"/extract_text_7b facts_created not int: {j}")
    _assert(isinstance(j.get("nodes_before"), int), f"/extract_text_7b nodes_before not int: {j}")
    _assert(isinstance(j.get("nodes_after"), int), f"/extract_text_7b nodes_after not int: {j}")
    _assert(j["nodes_after"] >= j["nodes_before"], f"/extract_text_7b nodes_after < nodes_before: {j}")

    if not commit_test_artifacts:
        _assert(j.get("document_node_id") in (-1, 0, None), f"expected no document node when commit=false: {j}")

    return j


def check_extract_code(
    base_url: str,
    timeout_s: float,
    commit_test_artifacts: bool,
) -> Dict[str, Any]:
    code = """
#include <stdio.h>

static int zeta_smoke_add(int a, int b) {
    return a + b;
}

int main(void) {
    printf(\"%d\\n\", zeta_smoke_add(2, 3));
    return 0;
}
""".strip()

    payload: Dict[str, Any] = {
        "code": code,
        "filename": "zeta_system_smoke_test.c",
        "max_entities": 25,
        "max_rels": 25,
    }
    if not commit_test_artifacts:
        payload["commit"] = False

    r = requests.post(_url(base_url, "/extract_code_7b"), json=payload, timeout=timeout_s)
    _assert(r.status_code == 200, f"/extract_code_7b HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("status") == "ok", f"/extract_code_7b status != ok: {j}")
    _assert(j.get("filename") == payload["filename"], f"/extract_code_7b filename mismatch: {j}")
    _assert(isinstance(j.get("entities_found"), int), f"/extract_code_7b entities_found not int: {j}")
    _assert(isinstance(j.get("relationships_found"), int), f"/extract_code_7b relationships_found not int: {j}")
    _assert(isinstance(j.get("nodes_committed"), int), f"/extract_code_7b nodes_committed not int: {j}")

    if not commit_test_artifacts:
        _assert(j.get("nodes_committed") == 0, f"expected nodes_committed==0 when commit=false: {j}")
        # When commit=false, the server omits per-entity node_id fields.
        entities = j.get("entities", [])
        if isinstance(entities, list) and entities:
            any_has_node_id = any(isinstance(e, dict) and "node_id" in e for e in entities)
            _assert(not any_has_node_id, f"expected no node_id in entities when commit=false: {entities[:3]}")

    return j


def maybe_reset_graph(base_url: str, timeout_s: float) -> Dict[str, Any]:
    r = requests.get(_url(base_url, "/graph/reset"), timeout=timeout_s)
    _assert(r.status_code == 200, f"/graph/reset HTTP {r.status_code}: {r.text[:200]}")
    j = _get_json(r)
    _assert(j.get("status") == "ok", f"/graph/reset status != ok: {j}")
    return j


def run(base_url: str, timeout_s: float, reset_graph: bool, commit_test_artifacts: bool) -> int:
    checks = []

    def _run_check(name: str, fn) -> None:
        start = time.time()
        try:
            fn()
            checks.append(CheckResult(name=name, ok=True, detail=f"{(time.time() - start):.2f}s"))
        except Exception as e:
            checks.append(CheckResult(name=name, ok=False, detail=str(e)))

    if reset_graph:
        _run_check("graph/reset", lambda: maybe_reset_graph(base_url, timeout_s))

    health: Dict[str, Any] = {}

    def _health_capture() -> None:
        nonlocal health
        health = check_health(base_url, timeout_s)

    _run_check("health", _health_capture)
    _run_check("mode", lambda: check_mode(base_url, timeout_s))
    _run_check("mode/switching", lambda: check_mode_switching(base_url, timeout_s))
    _run_check("v1/models", lambda: check_v1_models(base_url, timeout_s))
    _run_check("v1/chat/completions", lambda: check_chat_completions(base_url, timeout_s))
    _run_check("generate", lambda: check_generate(base_url, timeout_s))
    _run_check("graph", lambda: check_graph(base_url, timeout_s))
    _run_check("memory/query", lambda: check_memory_query(base_url, timeout_s))
    _run_check(
        "extract_text_7b",
        lambda: check_extract_text(base_url, timeout_s, commit_test_artifacts=commit_test_artifacts),
    )
    _run_check(
        "extract_code_7b",
        lambda: check_extract_code(base_url, timeout_s, commit_test_artifacts=commit_test_artifacts),
    )

    failed = [c for c in checks if not c.ok]

    print("=== Zeta System Smoke Test ===")
    print(f"Base URL: {base_url}")
    if health:
        print(
            f"Server: {health.get('status')} v{health.get('version')} | "
            f"Nodes: {health.get('graph_nodes')} Edges: {health.get('graph_edges')}"
        )
    print(f"Reset graph: {reset_graph}")
    print(f"Commit test artifacts: {commit_test_artifacts}")
    print("")

    for c in checks:
        status = "PASS" if c.ok else "FAIL"
        print(f"[{status}] {c.name}: {c.detail}")

    print("")
    if failed:
        print(f"FAILED: {len(failed)}/{len(checks)} checks")
        return 1

    print(f"OK: {len(checks)}/{len(checks)} checks")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--base-url",
        default=default_base_url(),
        help="Base URL of Zeta server (default: $ZETA_BASE_URL, else zeta.conf ZETA_URL, else http://localhost:8080)",
    )
    ap.add_argument("--timeout", type=float, default=30.0, help="Request timeout seconds")
    ap.add_argument(
        "--reset-graph",
        action="store_true",
        help="Destructive: call /graph/reset before running checks",
    )
    ap.add_argument(
        "--commit-test-artifacts",
        action="store_true",
        help="Commits small test ingests to the graph (more coverage, more noise)",
    )

    args = ap.parse_args()
    return run(
        base_url=args.base_url,
        timeout_s=args.timeout,
        reset_graph=args.reset_graph,
        commit_test_artifacts=args.commit_test_artifacts,
    )


if __name__ == "__main__":
    raise SystemExit(main())
