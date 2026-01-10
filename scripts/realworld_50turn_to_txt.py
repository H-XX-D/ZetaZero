#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from typing import Any


def _safe(obj: dict[str, Any], key: str, default: Any) -> Any:
    val = obj.get(key, default)
    return default if val is None else val


def _fmt_kv_block(title: str, d: dict[str, Any]) -> list[str]:
    lines: list[str] = []
    lines.append(title)
    for k, v in d.items():
        lines.append(f"  {k}: {v}")
    return lines


def _fmt_results_table(rows: list[dict[str, Any]], key_name: str) -> list[str]:
    # key_name is either 'turn' or 'query'
    lines: list[str] = []
    lines.append(f"  {key_name:>5}  time_s  peak_w")
    lines.append(f"  {'-'*5}  {'-'*6}  {'-'*6}")
    for r in rows:
        idx = r.get(key_name, "")
        time_s = r.get("time_s", "")
        peak_w = r.get("peak_w", "")
        lines.append(f"  {idx:>5}  {time_s:>6}  {peak_w:>6}")
    return lines


def render_realworld_50turn_txt(data: dict[str, Any]) -> str:
    lines: list[str] = []

    lines.append(f"BENCHMARK: {_safe(data, 'benchmark', '')}")
    date = _safe(data, "date", "")
    if date:
        lines.append(f"DATE: {date}")

    hardware = _safe(data, "hardware", {})
    if isinstance(hardware, dict) and hardware:
        lines.append("")
        lines.extend(_fmt_kv_block("HARDWARE:", hardware))

    model = _safe(data, "model", {})
    if isinstance(model, dict) and model:
        lines.append("")
        lines.extend(_fmt_kv_block("MODEL:", model))

    params = _safe(data, "test_parameters", {})
    if isinstance(params, dict) and params:
        lines.append("")
        lines.extend(_fmt_kv_block("TEST_PARAMETERS:", params))

    design = _safe(data, "conversation_design", {})
    if isinstance(design, dict) and design:
        lines.append("")
        lines.extend(_fmt_kv_block("CONVERSATION_DESIGN:", design))

    growing = _safe(data, "growing_context", {})
    if isinstance(growing, dict) and growing:
        lines.append("")
        lines.append("GROWING_CONTEXT:")
        desc = growing.get("description")
        if desc:
            lines.append(f"  description: {desc}")
        results = growing.get("results", [])
        if isinstance(results, list) and results:
            lines.append("  results:")
            lines.extend(_fmt_results_table(results, "turn"))

    fresh = _safe(data, "fresh_query_zeta", {})
    if isinstance(fresh, dict) and fresh:
        lines.append("")
        lines.append("FRESH_QUERY_ZETA:")
        desc = fresh.get("description")
        if desc:
            lines.append(f"  description: {desc}")
        results = fresh.get("results", [])
        if isinstance(results, list) and results:
            lines.append("  results:")
            lines.extend(_fmt_results_table(results, "query"))

    summary = _safe(data, "summary", {})
    if isinstance(summary, dict) and summary:
        lines.append("")
        lines.append("SUMMARY:")
        for section_name, section in summary.items():
            if isinstance(section, dict):
                lines.append(f"  {section_name}:")
                for k, v in section.items():
                    lines.append(f"    {k}: {v}")

    turns = _safe(data, "conversation_turns", [])
    retrieval_indices = set()
    if isinstance(design, dict):
        idxs = design.get("retrieval_question_indices", [])
        if isinstance(idxs, list):
            for x in idxs:
                if isinstance(x, int):
                    retrieval_indices.add(x)

    if isinstance(turns, list) and turns:
        lines.append("")
        lines.append("CONVERSATION_TURNS:")
        for i, t in enumerate(turns, start=1):
            tag = " [RETRIEVAL_Q]" if i in retrieval_indices else ""
            # Keep each turn as a single line for grep-ability.
            text = str(t).replace("\n", " ").strip()
            lines.append(f"  {i:02d}{tag}: {text}")

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert 50_turn_realworld.json benchmark to a readable .txt")
    ap.add_argument("input", type=Path, help="Path to benchmarks/50_turn_realworld.json")
    ap.add_argument("-o", "--output", type=Path, default=None, help="Output .txt path (default: alongside input)")
    args = ap.parse_args()

    input_path: Path = args.input
    out_path = args.output if args.output is not None else input_path.with_suffix(".txt")

    data = json.loads(input_path.read_text(encoding="utf-8"))
    txt = render_realworld_50turn_txt(data)
    out_path.write_text(txt, encoding="utf-8")
    print(str(out_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
