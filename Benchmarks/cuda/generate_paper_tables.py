#!/usr/bin/env python3
"""
Generate Paper Tables from Benchmark Results

Reads JSON results from benchmarks and generates markdown tables
suitable for inclusion in the arxiv paper.
"""

import argparse
import json
from datetime import datetime
from pathlib import Path


def load_results(results_dir: Path) -> dict:
    """Load all benchmark result files."""
    results = {}

    for name in ["retrieval", "latency", "persistence", "ablation", "resources"]:
        result_file = results_dir / f"{name}.json"
        if result_file.exists():
            with open(result_file) as f:
                results[name] = json.load(f)

    return results


def generate_retrieval_table(data: dict) -> str:
    """Generate Section 6.3 table."""
    if not data:
        return "No retrieval data available.\n"

    lines = [
        "### Section 6.3: Retrieval Accuracy",
        "",
        "| Metric | Baseline | SubZero | Zero |",
        "|--------|----------|---------|------|",
    ]

    configs = data.get("configs", {})
    baseline = configs.get("baseline", {})
    subzero = configs.get("subzero", configs.get("pure_retrieval", {}))
    zero = configs.get("zero", {})

    for metric, label in [
        ("recall_at_1", "Recall@1"),
        ("recall_at_5", "Recall@5"),
        ("mrr", "MRR"),
        ("fpr", "FPR"),
    ]:
        bl = f"{baseline.get(metric, 0)*100:.1f}%" if metric != "mrr" else f"{baseline.get(metric, 0):.3f}"
        sz = f"{subzero.get(metric, 0)*100:.1f}%" if metric != "mrr" else f"{subzero.get(metric, 0):.3f}"
        z = f"{zero.get(metric, 0)*100:.1f}%" if metric != "mrr" else f"{zero.get(metric, 0):.3f}"

        if not baseline:
            bl = "[N/A]"
        if not subzero:
            sz = "[N/A]"
        if not zero:
            z = "[N/A]"

        lines.append(f"| {label} | {bl} | {sz} | {z} |")

    return "\n".join(lines) + "\n"


def generate_latency_table(data: dict) -> str:
    """Generate Section 6.5 tables."""
    if not data:
        return "No latency data available.\n"

    lines = [
        "### Section 6.5: Latency Breakdown",
        "",
    ]

    profiles = data.get("profiles", {})

    # SubZero table
    if "subzero" in profiles:
        lines.extend([
            "**SubZero Pipeline:**",
            "",
            "| Operation | Mean (ms) | Std (ms) | P95 (ms) |",
            "|-----------|-----------|----------|----------|",
        ])

        sz = profiles["subzero"]
        for name, key in [
            ("Entity extraction (3B)", "entity_extraction"),
            ("Graph walk (depth=3)", "graph_walk"),
            ("Fact retrieval", "fact_retrieval"),
            ("3B generation", "generation_3b"),
            ("**Total SubZero**", "total_subzero"),
        ]:
            d = sz.get(key, {})
            lines.append(f"| {name} | {d.get('mean_ms', 0):.1f} | {d.get('std_ms', 0):.1f} | {d.get('p95_ms', 0):.1f} |")

        lines.append("")

    # Zero table
    if "zero" in profiles:
        lines.extend([
            "**Zero Pipeline:**",
            "",
            "| Operation | Mean (ms) | Std (ms) | P95 (ms) |",
            "|-----------|-----------|----------|----------|",
        ])

        z = profiles["zero"]
        for name, key in [
            ("Entity extraction (3B)", "entity_extraction"),
            ("Graph walk (depth=3)", "graph_walk"),
            ("Fact retrieval", "fact_retrieval"),
            ("14B generation", "generation_14b"),
            ("**Total Zero**", "total_zero"),
        ]:
            d = z.get(key, {})
            lines.append(f"| {name} | {d.get('mean_ms', 0):.1f} | {d.get('std_ms', 0):.1f} | {d.get('p95_ms', 0):.1f} |")

        lines.append("")

    # Summary table
    lines.extend([
        "**Latency Comparison:**",
        "",
        "| Config | Total (ms) | Use Case |",
        "|--------|------------|----------|",
    ])

    if "baseline" in profiles:
        bl = profiles["baseline"].get("generation_14b", {})
        lines.append(f"| Baseline | {bl.get('mean_ms', 0):.0f} | Fast, no memory |")

    if "subzero" in profiles:
        sz = profiles["subzero"].get("total_subzero", {})
        lines.append(f"| SubZero | {sz.get('mean_ms', 0):.0f} | Edge, real-time |")

    if "zero" in profiles:
        z = profiles["zero"].get("total_zero", {})
        lines.append(f"| Zero | {z.get('mean_ms', 0):.0f} | Quality, async |")

    return "\n".join(lines) + "\n"


def generate_persistence_table(data: dict) -> str:
    """Generate Section 6.6 table."""
    if not data:
        return "No persistence data available.\n"

    lines = [
        "### Section 6.6: Memory Persistence",
        "",
        "| Hours Since Learning | Recall Rate | Avg Score | Expected D(t) |",
        "|----------------------|-------------|-----------|---------------|",
    ]

    for r in data.get("results", []):
        hours_label = f"{r['hours']} (immediate)" if r['hours'] == 0 else str(r['hours'])
        lines.append(
            f"| {hours_label} | {r['recall_rate']*100:.0f}% | "
            f"{r['avg_score']:.3f} | {r['expected_decay']:.3f} |"
        )

    return "\n".join(lines) + "\n"


def generate_ablation_table(data: dict) -> str:
    """Generate Section 7.1 table."""
    if not data:
        return "No ablation data available.\n"

    lines = [
        "### Section 7.1: Component Contribution",
        "",
        "| Configuration | Recall@5 | MRR | Notes |",
        "|---------------|----------|-----|-------|",
    ]

    notes = {
        "cosine_only": "Pure similarity",
        "plus_decay": "Recency bias",
        "plus_entity": "Graph walk",
        "full_subzero": "Full retrieval",
    }

    for r in data.get("configs", []):
        name = r["config"]
        note = notes.get(name, "")
        lines.append(f"| {name} | {r['recall_at_5']*100:.1f}% | {r['mrr']:.4f} | {note} |")

    return "\n".join(lines) + "\n"


def generate_resources_table(data: dict) -> str:
    """Generate Section 6.4 table."""
    if not data:
        return "No resources data available.\n"

    lines = [
        "### Section 6.4: Context Extension",
        "",
        "| Configuration | Effective Context | Peak RAM | Tokens/sec |",
        "|---------------|-------------------|----------|------------|",
    ]

    # Baseline
    for bl in data.get("baseline", []):
        lines.append(
            f"| Baseline | {bl['effective_context']/1000:.0f}K | "
            f"{bl['vram_mb']:.0f} MB | {bl['tokens_per_sec']:.1f} |"
        )

    # SubZero
    for r in data.get("subzero", []):
        if r["num_blocks"] in [5, 20]:
            lines.append(
                f"| SubZero ({r['num_blocks']} blocks) | {r['effective_context']/1000:.1f}K | "
                f"{r['vram_mb']:.0f} MB | {r['tokens_per_sec']:.1f} |"
            )

    # Zero
    for r in data.get("zero", []):
        if r["num_blocks"] in [5, 20]:
            lines.append(
                f"| Zero ({r['num_blocks']} blocks) | {r['effective_context']/1000:.1f}K | "
                f"{r['vram_mb']:.0f} MB | {r['tokens_per_sec']:.1f} |"
            )

    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Generate paper tables from benchmark results")
    parser.add_argument("-d", "--results-dir", default="results",
                        help="Directory containing result JSON files")
    parser.add_argument("-o", "--output", help="Output markdown file (default: stdout)")
    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    results = load_results(results_dir)

    output_lines = [
        "# Z.E.T.A. Benchmark Results",
        "",
        f"Generated: {datetime.now().isoformat()}",
        "",
        "---",
        "",
    ]

    # Generate each table
    output_lines.append(generate_retrieval_table(results.get("retrieval", {})))
    output_lines.append("")
    output_lines.append(generate_latency_table(results.get("latency", {})))
    output_lines.append("")
    output_lines.append(generate_persistence_table(results.get("persistence", {})))
    output_lines.append("")
    output_lines.append(generate_ablation_table(results.get("ablation", {})))
    output_lines.append("")
    output_lines.append(generate_resources_table(results.get("resources", {})))

    output = "\n".join(output_lines)

    if args.output:
        with open(args.output, "w") as f:
            f.write(output)
        print(f"Tables written to {args.output}")
    else:
        print(output)


if __name__ == "__main__":
    main()
