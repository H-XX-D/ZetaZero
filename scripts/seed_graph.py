#!/usr/bin/env python3
"""Seed a Z.E.T.A. graph with codebase context.

This is a pragmatic bridge until full automated indexing/ingestion is stable.
It does two things for each selected file:

1) POST /extract_code_7b with file text (optionally truncated)
   - creates typed nodes (file/function/class/struct/etc) + relationships

2) POST /git/commit with a "file_excerpt:" node
   - stores *actual* excerpt text so dreams/queries can ground in real code

Usage:
  python3 scripts/seed_graph.py --path /Users/me/ZetaZero/llama.cpp/tools/zeta-zero
  python3 scripts/seed_graph.py --remote xx@192.168.0.165:/home/xx/ZetaZero --max-files 30

Notes:
- The server's /extract_code_7b parser is intentionally lightweight; keeping
  excerpts smaller reduces the chance of timeouts/restarts.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shlex
import subprocess
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple, cast

import requests  # type: ignore[import-untyped]

DEFAULT_URL = os.environ.get("ZETA_URL", "http://192.168.0.165:8080")

DEFAULT_EXTS = (".h", ".hpp", ".c", ".cpp", ".cu", ".cuh", ".m", ".mm", ".py", ".ts", ".js", ".md")
DEFAULT_EXCLUDE_DIRS = (".git", "build", "dist", "node_modules", "__pycache__", ".venv", "venv")


def parse_remote_spec(spec: str) -> Tuple[str, str]:
    if ":" not in spec:
        raise ValueError("remote spec must look like user@host:/abs/path")
    host, path = spec.split(":", 1)
    if not host or not path.startswith("/"):
        raise ValueError("remote spec must look like user@host:/abs/path")
    return host, path


def ssh_run(host: str, command: str, timeout_s: int = 60) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["ssh", host, command],
        capture_output=True,
        text=True,
        timeout=timeout_s,
    )


def remote_find_files(host: str, root: str, exts: Iterable[str], exclude_dirs: Iterable[str]) -> List[str]:
    name_parts = " -o ".join([f"-name '*{shlex.quote(ext)}'".replace("'", "") for ext in exts])
    prune_parts = " -o ".join([f"-path '*/{d}/*'" for d in exclude_dirs])
    cmd = (
        f"find {shlex.quote(root)} -type f \\\n            \\( {name_parts} \\) \\\n            -a ! \\\n            \\( {prune_parts} \\)"
    )
    result = ssh_run(host, cmd, timeout_s=120)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "remote find failed")
    files = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    files.sort()
    return files


def remote_read_file(host: str, path: str, timeout_s: int = 60) -> str:
    cmd = f"cat {shlex.quote(path)}"
    result = ssh_run(host, cmd, timeout_s=timeout_s)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"failed to read {path}")
    return result.stdout


def local_find_files(root: str, exts: Iterable[str], exclude_dirs: Iterable[str]) -> List[str]:
    root_path = Path(root)
    files: List[str] = []
    for p in root_path.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in exts:
            continue
        if any(part in exclude_dirs for part in p.parts):
            continue
        files.append(str(p))
    files.sort()
    return files


def truncate(text: str, max_chars: int) -> str:
    if max_chars <= 0:
        return text
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + f"\n\n... [truncated, {len(text)} chars total]"


def excerpt_lines(text: str, start_line_1based: int, max_lines: int = 80) -> str:
    """Extract a bounded snippet starting at start_line_1based.

    If start is out of range, returns the beginning of the file.
    """
    if max_lines <= 0:
        max_lines = 80
    lines = text.splitlines()
    if not lines:
        return ""
    start_idx = max(0, int(start_line_1based) - 1)
    if start_idx >= len(lines):
        start_idx = 0
    end_idx = min(len(lines), start_idx + max_lines)
    return "\n".join(lines[start_idx:end_idx])


def excerpt_range(text: str, start_line_1based: int, end_line_1based: int, max_lines: int = 200) -> str:
    """Extract inclusive [start_line_1based, end_line_1based] with a safety cap."""
    lines = text.splitlines()
    if not lines:
        return ""
    start_idx = max(0, int(start_line_1based) - 1)
    end_idx = max(start_idx, int(end_line_1based) - 1)
    if start_idx >= len(lines):
        start_idx = 0
        end_idx = min(len(lines) - 1, max(0, int(end_line_1based) - 1))
    end_idx = min(len(lines) - 1, end_idx)
    # Safety cap (inclusive indices)
    if max_lines > 0 and (end_idx - start_idx + 1) > max_lines:
        end_idx = min(len(lines) - 1, start_idx + max_lines - 1)
    return "\n".join(lines[start_idx : end_idx + 1])


def extract_code_7b(server_url: str, code: str, filename: str, commit: bool, timeout_s: int) -> Dict[str, Any]:
    payload: Dict[str, Any] = {"code": code, "filename": filename, "commit": bool(commit)}
    r = requests.post(f"{server_url}/extract_code_7b", json=payload, timeout=timeout_s)
    if r.status_code != 200:
        raise RuntimeError(f"/extract_code_7b failed: {r.status_code} {r.text[:200]}")
    return r.json()


def stable_snippet_label(relpath: str, ent_type: str, ent_name: str, line: int, line_end: int) -> str:
    """Return a stable label for a per-entity snippet node.

    We intentionally *do not* use graph node_id, since rerunning extraction can
    create a new entity node_id. This keeps snippet nodes reusable across runs.
    """
    rng = f"{int(line)}-{int(line_end) if int(line_end) else int(line)}"
    key = f"{relpath}|{ent_type}|{ent_name}|{rng}"
    digest = hashlib.blake2b(key.encode("utf-8"), digest_size=8).hexdigest()
    return f"code_snippet#{digest}"


def git_commit(server_url: str, label: str, value: str, salience: float, timeout_s: int = 10) -> int:
    r = requests.post(
        f"{server_url}/git/commit",
        json={"label": label, "value": value, "salience": float(salience)},
        timeout=timeout_s,
    )
    if r.status_code != 200:
        raise RuntimeError(f"/git/commit failed: {r.status_code} {r.text[:200]}")
    data = r.json()
    return int(data.get("node_id", 0) or 0)


def git_edge(
    server_url: str,
    source_id: int,
    target_id: int,
    edge_type: str = "has",
    weight: float = 1.0,
    timeout_s: int = 10,
) -> int:
    r = requests.post(
        f"{server_url}/git/edge",
        json={
            "source_id": int(source_id),
            "target_id": int(target_id),
            "type": str(edge_type),
            "weight": float(weight),
        },
        timeout=timeout_s,
    )
    if r.status_code != 200:
        raise RuntimeError(f"/git/edge failed: {r.status_code} {r.text[:200]}")
    data = r.json()
    return int(data.get("edge_id", 0) or 0)


def git_checkout(server_url: str, name: str) -> None:
    requests.post(f"{server_url}/git/checkout", json={"name": name}, timeout=10)


def git_branch(server_url: str, name: str) -> None:
    requests.post(f"{server_url}/git/branch", json={"name": name}, timeout=10)


def open_project(server_url: str, path: str, name: str, description: str = "") -> None:
    r = requests.post(
        f"{server_url}/project/open",
        params={"path": path, "name": name, "description": description},
        timeout=10,
    )
    if r.status_code != 200:
        raise RuntimeError(f"/project/open failed: {r.status_code} {r.text[:200]}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Seed Z.E.T.A. graph with codebase excerpts + extracted entities")
    ap.add_argument("--url", default=DEFAULT_URL, help="Z.E.T.A. server URL")

    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--path", help="Local root path to seed")
    src.add_argument("--remote", help="Remote spec user@host:/abs/path")

    ap.add_argument("--project-path", default="", help="Path to register with /project/open")
    ap.add_argument("--project-name", default="", help="Project name for /project/open")
    ap.add_argument("--project-description", default="", help="Project description for /project/open")

    ap.add_argument("--branch", default="seed", help="Branch to write seed nodes into")
    ap.add_argument("--max-files", type=int, default=40, help="Max files to seed")
    ap.add_argument("--max-chars", type=int, default=8000, help="Max chars per file excerpt")
    ap.add_argument("--extract-max-chars", type=int, default=12000, help="Max chars sent to /extract_code_7b")
    ap.add_argument("--delay", type=float, default=0.2, help="Delay between requests")
    ap.add_argument("--timeout", type=int, default=240, help="Timeout for /extract_code_7b")
    ap.add_argument("--no-extract", action="store_true", help="Skip /extract_code_7b; only commit excerpts")
    ap.add_argument("--no-excerpt", action="store_true", help="Skip excerpt commits; only /extract_code_7b")
    ap.add_argument("--ext", action="append", default=[], help="File extension to include (repeatable)")

    args = ap.parse_args()

    exts = tuple(args.ext) if args.ext else DEFAULT_EXTS

    # Health check
    try:
        h = requests.get(f"{args.url}/health", timeout=5).json()
        print(f"Connected: {args.url} | status={h.get('status')} | nodes={h.get('graph_nodes')} edges={h.get('graph_edges')}")
    except Exception as e:
        print(f"ERROR: cannot reach {args.url}/health: {e}")
        return 2

    # Optional: register project
    if args.project_path or args.project_name:
        if not args.project_path or not args.project_name:
            print("ERROR: --project-path and --project-name must be provided together")
            return 2
        open_project(args.url, args.project_path, args.project_name, args.project_description)
        print(f"Project opened: {args.project_name} ({args.project_path})")

    # Prepare branch
    try:
        git_branch(args.url, args.branch)
        git_checkout(args.url, args.branch)
        print(f"Checked out branch: {args.branch}")
    except Exception as e:
        print(f"WARN: branch/checkout failed (continuing): {e}")

    # Collect files
    if args.remote:
        host, remote_root = parse_remote_spec(args.remote)
        files = remote_find_files(host, remote_root, exts, DEFAULT_EXCLUDE_DIRS)
        root_for_rel = remote_root.rstrip("/") + "/"
        def read_fn(p: str) -> str:
            return remote_read_file(host, p, timeout_s=60)
        project_hint = remote_root
    else:
        local_root = os.path.abspath(args.path)
        files = local_find_files(local_root, exts, DEFAULT_EXCLUDE_DIRS)
        root_for_rel = local_root.rstrip(os.sep) + os.sep

        def read_fn(p: str) -> str:
            with open(p, "r", encoding="utf-8", errors="ignore") as f:
                return f.read()

        project_hint = local_root

    if not files:
        print("No files found.")
        return 0

    files = files[: max(0, args.max_files)]
    print(f"Seeding {len(files)} files from {project_hint}")

    ok = 0
    failed = 0
    total_entities = 0
    total_committed = 0

    for i, file_path in enumerate(files, 1):
        rel = file_path[len(root_for_rel) :] if file_path.startswith(root_for_rel) else os.path.basename(file_path)
        print(f"[{i}/{len(files)}] {rel}")
        try:
            content = read_fn(file_path)
            if len(content.strip()) < 50:
                continue

            extracted_file_node_id: Optional[int] = None
            extracted_entities: List[Dict[str, Any]] = []

            if not args.no_extract:
                extract_payload = truncate(content, args.extract_max_chars)
                result = extract_code_7b(args.url, extract_payload, rel, commit=True, timeout_s=args.timeout)
                total_entities += int(result.get("entities_found", 0))
                total_committed += int(result.get("nodes_committed", 0))

                # Try to locate the committed "file" entity node_id so we can link it to the excerpt.
                # Newer servers return node_id per entity; older servers won't.
                entities_any: Any = result.get("entities")
                if isinstance(entities_any, list):
                    entities_list: List[Dict[str, Any]] = []
                    for entity_obj in cast(List[Any], entities_any):
                        if isinstance(entity_obj, dict):
                            entities_list.append(cast(Dict[str, Any], entity_obj))

                    for ent in entities_list:
                        ent_type = str(ent.get("type", ""))
                        if ent_type not in {"file", "function", "struct", "class", "enum"}:
                            continue

                        node_id_raw: Any = ent.get("node_id")
                        if node_id_raw is None:
                            continue
                        try:
                            node_id_int = int(node_id_raw)
                        except Exception:
                            continue

                        if ent_type == "file" and extracted_file_node_id is None:
                            extracted_file_node_id = node_id_int
                            continue

                        # Save entity metadata for per-entity snippet creation.
                        line_raw: Any = ent.get("line", 1)
                        line_end_raw: Any = ent.get("line_end", 0)
                        extracted_entities.append(
                            {
                                "type": ent_type,
                                "node_id": node_id_int,
                                "name": str(ent.get("name", "")),
                                "line": int(line_raw or 1),
                                "line_end": int(line_end_raw or 0),
                            }
                        )

            if not args.no_excerpt:
                excerpt = truncate(content, args.max_chars)
                label = f"file_excerpt:{rel}"
                value = f"Project: {project_hint}\nFile: {rel}\n\n{excerpt}"
                excerpt_node_id = git_commit(args.url, label=label, value=value, salience=0.85)

                # If we also extracted code entities, link the file entity -> excerpt node.
                if extracted_file_node_id and excerpt_node_id:
                    try:
                        git_edge(args.url, source_id=extracted_file_node_id, target_id=excerpt_node_id, edge_type="has", weight=1.0)
                    except Exception as e:
                        # Non-fatal: graph linkage is a nice-to-have.
                        print(f"  WARN: failed to link file->excerpt edge: {e}")

                # Best way: create per-entity snippet nodes and link entity -> snippet.
                # This avoids "many entities -> same file excerpt" noise and grounds precisely.
                if extracted_entities:
                    # Cap to avoid exploding graph on huge files.
                    max_entities_per_file = 30
                    for ent in extracted_entities[:max_entities_per_file]:
                        try:
                            ent_node_id = int(ent["node_id"])
                            ent_type = str(ent.get("type", ""))
                            ent_name = str(ent.get("name", ""))
                            ent_line = int(ent.get("line", 1) or 1)
                            ent_line_end = int(ent.get("line_end", 0) or 0)

                            if ent_line_end and ent_line_end >= ent_line:
                                snippet = excerpt_range(content, ent_line, ent_line_end, max_lines=220)
                            else:
                                snippet = excerpt_lines(content, ent_line, max_lines=90)
                            if len(snippet.strip()) < 20:
                                continue

                            snippet_label = stable_snippet_label(rel, ent_type, ent_name, ent_line, ent_line_end)
                            snippet_value = (
                                f"Project: {project_hint}\n"
                                f"File: {rel}\n"
                                f"Entity: {ent_type}:{ent_name}\n"
                                f"Line: {ent_line}\n"
                                f"LineEnd: {ent_line_end}\n\n"
                                f"{truncate(snippet, 8000)}"
                            )

                            snippet_node_id = git_commit(args.url, label=snippet_label, value=snippet_value, salience=0.9)
                            if snippet_node_id:
                                git_edge(args.url, source_id=ent_node_id, target_id=snippet_node_id, edge_type="has", weight=1.0)
                                if extracted_file_node_id:
                                    git_edge(
                                        args.url,
                                        source_id=extracted_file_node_id,
                                        target_id=snippet_node_id,
                                        edge_type="has",
                                        weight=1.0,
                                    )
                        except Exception as e:
                            print(f"  WARN: failed to create/link entity snippet: {e}")

            ok += 1
            if args.delay > 0:
                time.sleep(args.delay)
        except Exception as e:
            failed += 1
            print(f"  ERROR: {e}")

    # Best effort: go back to main
    try:
        git_checkout(args.url, "main")
    except Exception:
        pass

    print("\nSummary")
    print(f"  files_ok: {ok}")
    print(f"  files_failed: {failed}")
    if not args.no_extract:
        print(f"  entities_found: {total_entities}")
        print(f"  nodes_committed: {total_committed}")
    if not args.no_excerpt:
        print(f"  excerpt_nodes_committed: {ok - failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
