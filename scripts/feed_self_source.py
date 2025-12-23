#!/usr/bin/env python3
"""Feed Z.E.T.A. its own source code to build a self-referential memory graph."""

import requests
import os
import time
import sys

ZETA_URL = "http://192.168.0.165:8080"
SOURCE_DIR = "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo"

# Key source files to ingest (in dependency order)
SOURCE_FILES = [
    "zeta-config.h",
    "zeta-dual-process.h",
    "zeta-embed-integration.h",
    "zeta-graph-manager.h",
    "zeta-graph-git.h",
    "zeta-conflict.h",
    "zeta-hrm.h",
    "zeta-trm.h",
    "zeta-dream.h",
    "zeta-critic.h",
    "zeta-semantic-attacks.h",
    "zeta-code-mode.h",
    "zeta-extract.h",
    "zeta-proactive-memory.h",
    "zeta-ternary.h",
    "zeta-pruning.h",        # Memory pruning
    "zeta-task-eval.h",      # Task evaluation
    "zeta-server.cpp",
]

# MODEL EFFICIENCY: Files that handle the 14B/7B/4B orchestration
MODEL_EFFICIENCY_PROMPTS = {
    "zeta-dual-process.h": """Focus on how the 14B conscious and 7B coder models interact.
What determines when to use which model? How could routing be more efficient?
Identify any bottlenecks in the dual-process architecture.""",

    "zeta-embed-integration.h": """Analyze the 4B embedding model usage.
How are embeddings computed? When are they cached?
What optimizations could reduce embedding computation overhead?""",

    "zeta-code-mode.h": """Examine the 7B coder model integration.
When does code mode activate vs the 14B model?
How could code generation be more efficient?""",

    "zeta-server.cpp": """Analyze the model orchestration in the main server.
How are requests routed between 14B, 7B, and 4B models?
What are the latency hotspots? How could parallel processing be improved?""",

    "zeta-critic.h": """Review the critic module that validates outputs.
How does it decide between 14B and 7B responses?
Could the evaluation be done more efficiently?""",

    "zeta-hrm.h": """Examine how HRM orchestrates multi-step reasoning.
When does it spawn parallel tasks vs sequential?
How could the planning phase be more efficient?""",
}

# TOOL USE: Files that handle tool/function calling
TOOL_USE_PROMPTS = {
    "zeta-extract.h": """Analyze the extraction and tool use capabilities.
How are tool calls identified and parsed?
What improvements could make tool use more reliable?""",

    "zeta-graph-git.h": """Review the Git-graph tool integration.
How does Z.E.T.A. interact with the memory graph as a tool?
What tool operations are most frequently used?""",

    "zeta-semantic-attacks.h": """Analyze the semantic attack detection.
How are malicious tool use attempts detected?
What patterns indicate tool misuse?""",
}

def read_remote_file(filepath):
    """Read file from z6 via the local copy."""
    local_path = filepath.replace("/home/xx/ZetaZero", "/Users/hendrixx./ZetaZero")
    try:
        with open(local_path, 'r') as f:
            return f.read()
    except Exception as e:
        print(f"  Error reading {local_path}: {e}")
        return None

def extract_key_structures(content, filename):
    """Extract key structures from source for summarization."""
    lines = content.split('\n')
    structures = []

    # Find structs, classes, key functions
    for i, line in enumerate(lines):
        if 'struct ' in line and '{' in line:
            structures.append(f"struct: {line.strip()}")
        elif 'class ' in line and '{' in line:
            structures.append(f"class: {line.strip()}")
        elif line.strip().startswith('void ') or line.strip().startswith('int ') or line.strip().startswith('bool '):
            if '(' in line and ')' in line:
                structures.append(f"function: {line.strip()[:80]}")
        elif '#define ' in line and '(' in line:
            structures.append(f"macro: {line.strip()[:60]}")

    return structures[:20]  # Limit to 20 key structures

def feed_source_file(filename, content):
    """Feed a source file to Z.E.T.A. for memory graph ingestion."""

    # Extract key structures for context
    structures = extract_key_structures(content, filename)
    struct_summary = "\n".join(structures[:10])

    # Truncate content if too long (keep first 3000 chars for context)
    if len(content) > 4000:
        content_preview = content[:3000] + "\n... [truncated] ..."
    else:
        content_preview = content

    prompt = f"""Analyze this Z.E.T.A. source file and remember its key components:

FILE: {filename}

KEY STRUCTURES FOUND:
{struct_summary}

SOURCE PREVIEW:
```cpp
{content_preview}
```

Extract and remember:
1. The main purpose of this file
2. Key data structures defined
3. Important functions and what they do
4. How this connects to other Z.E.T.A. components

Summarize what you learned about {filename}."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": prompt, "max_tokens": 300},
            timeout=120
        )
        if resp.status_code == 200:
            data = resp.json()
            return data.get("output", ""), data.get("graph_nodes", 0)
        else:
            return f"Error: {resp.status_code}", 0
    except Exception as e:
        return f"Exception: {e}", 0

def feed_focused(filename, content, focus_prompt):
    """Feed a source file with a specific focus area."""
    # Extract key structures for context
    structures = extract_key_structures(content, filename)
    struct_summary = "\n".join(structures[:10])

    # Truncate content if too long
    if len(content) > 4000:
        content_preview = content[:3000] + "\n... [truncated] ..."
    else:
        content_preview = content

    prompt = f"""Analyze this Z.E.T.A. source file with a specific focus:

FILE: {filename}

FOCUS AREA:
{focus_prompt}

KEY STRUCTURES FOUND:
{struct_summary}

SOURCE PREVIEW:
```cpp
{content_preview}
```

Provide specific insights and improvement suggestions for the focus area.
Be concrete: name functions, suggest optimizations, identify inefficiencies."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": prompt, "max_tokens": 400},
            timeout=120
        )
        if resp.status_code == 200:
            data = resp.json()
            return data.get("output", ""), data.get("graph_nodes", 0)
        else:
            return f"Error: {resp.status_code}", 0
    except Exception as e:
        return f"Exception: {e}", 0


def main():
    print("=" * 60)
    print("Z.E.T.A. Self-Source Ingestion")
    print("Building memory graph of own codebase")
    print("=" * 60)

    # Check server health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"\nServer: {health['status']} | Nodes: {health['graph_nodes']} | Edges: {health['graph_edges']}")
    except Exception as e:
        print(f"Server not reachable: {e}")
        sys.exit(1)

    initial_nodes = health['graph_nodes']

    # Check command line args for focus mode
    mode = "all"
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()

    if mode == "efficiency":
        print("\n[FOCUSED MODE: Model Efficiency (14B/7B/4B orchestration)]")
        files_to_process = MODEL_EFFICIENCY_PROMPTS
    elif mode == "tools":
        print("\n[FOCUSED MODE: Tool Use]")
        files_to_process = TOOL_USE_PROMPTS
    elif mode == "both":
        print("\n[FOCUSED MODE: Both Model Efficiency and Tool Use]")
        files_to_process = {**MODEL_EFFICIENCY_PROMPTS, **TOOL_USE_PROMPTS}
    else:
        # Default: basic ingestion of all files
        print(f"\nIngesting {len(SOURCE_FILES)} source files...\n")

        for i, filename in enumerate(SOURCE_FILES, 1):
            filepath = f"{SOURCE_DIR}/{filename}"
            print(f"[{i}/{len(SOURCE_FILES)}] {filename}...", end=" ", flush=True)

            content = read_remote_file(filepath)
            if not content:
                print("SKIP (not found)")
                continue

            response, nodes = feed_source_file(filename, content)

            # Truncate response for display
            response_preview = response[:100].replace('\n', ' ')
            print(f"OK ({nodes} nodes) - {response_preview}...")

            time.sleep(1)  # Rate limit

        # Final stats
        try:
            final_health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
            new_nodes = final_health['graph_nodes'] - initial_nodes
            print(f"\n{'=' * 60}")
            print(f"COMPLETE: Added {new_nodes} new nodes to memory graph")
            print(f"Total: {final_health['graph_nodes']} nodes, {final_health['graph_edges']} edges")
            print("=" * 60)
        except:
            pass
        return

    # Focused mode processing
    print(f"\nProcessing {len(files_to_process)} focused files...\n")

    for i, (filename, focus_prompt) in enumerate(files_to_process.items(), 1):
        filepath = f"{SOURCE_DIR}/{filename}"
        print(f"[{i}/{len(files_to_process)}] {filename} (focused)...", end=" ", flush=True)

        content = read_remote_file(filepath)
        if not content:
            print("SKIP (not found)")
            continue

        response, nodes = feed_focused(filename, content, focus_prompt)

        # Truncate response for display
        response_preview = response[:100].replace('\n', ' ')
        print(f"OK ({nodes} nodes) - {response_preview}...")

        time.sleep(2)  # Rate limit (longer for focused analysis)

    # Final stats
    try:
        final_health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        new_nodes = final_health['graph_nodes'] - initial_nodes
        print(f"\n{'=' * 60}")
        print(f"COMPLETE: Added {new_nodes} new nodes to memory graph")
        print(f"Total: {final_health['graph_nodes']} nodes, {final_health['graph_edges']} edges")
        print("=" * 60)
    except:
        pass


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] in ["-h", "--help"]:
        print("Usage: python3 feed_self_source.py [mode]")
        print("Modes:")
        print("  (none)     - Ingest all source files")
        print("  efficiency - Focus on 14B/7B/4B model orchestration")
        print("  tools      - Focus on tool use and function calling")
        print("  both       - Focus on both efficiency and tools")
    else:
        main()
