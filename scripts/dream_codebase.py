#!/usr/bin/env python3
"""
Z.E.T.A. Codebase Dreaming System
---------------------------------
Feed code files through Z.E.T.A. with reflective prompts to generate
insights, improvements, and architectural observations.

Usage:
    python3 dream_codebase.py                    # Dream over zeta-demo/
    python3 dream_codebase.py --path /some/dir   # Dream over specific dir
    python3 dream_codebase.py --file foo.cpp     # Dream over single file
    python3 dream_codebase.py --prompt "Find security issues"
"""

import argparse
import json
import os
import sys
import time
import requests
from pathlib import Path
from datetime import datetime

ZETA_URL = os.environ.get("ZETA_URL", "http://192.168.0.165:8080")

# Default dream prompts for code analysis
DEFAULT_PROMPTS = [
    {
        "name": "architecture",
        "prompt": "Analyze this code's architecture. What patterns do you see? What could be improved? Be specific and concise.",
    },
    {
        "name": "bugs",
        "prompt": "Look for potential bugs, edge cases, or error conditions that aren't handled. List specific issues.",
    },
    {
        "name": "optimization",
        "prompt": "Identify performance bottlenecks or optimization opportunities in this code. Be specific.",
    },
    {
        "name": "security",
        "prompt": "Identify any security vulnerabilities (injection, overflow, race conditions, etc). Be specific.",
    },
    {
        "name": "simplification",
        "prompt": "What parts of this code are overly complex? How could they be simplified while maintaining functionality?",
    },
    {
        "name": "missing",
        "prompt": "What's missing from this code? What features or error handling should be added?",
    },
    {
        "name": "novel_capabilities",
        "prompt": "Based on the patterns and components in this code, propose 3 novel capabilities or features that don't exist yet. Think creatively about emergent behaviors or synergies. Be specific and technical - describe HOW each capability would work.",
    },
    {
        "name": "connections",
        "prompt": "How could this code connect with or enhance other parts of the system? What cross-cutting features or integrations would multiply its value? Think about unexpected combinations.",
    },
    {
        "name": "evolution",
        "prompt": "If this code could evolve itself, what would it change? What's the next logical step in its development? Propose specific modifications that would make it more powerful.",
    },
    {
        "name": "dormant",
        "prompt": "Look for features that appear to be coded but not fully integrated or used. Find dead code, unused functions, incomplete implementations, or capabilities that exist but aren't wired up. List what's dormant and how to activate it.",
    },
    {
        "name": "hidden_potential",
        "prompt": "What hidden potential exists in this code that isn't being exploited? What could this code do that it currently doesn't? Look for underutilized data structures, unused parameters, or capabilities hiding in plain sight.",
    },
    {
        "name": "documentation",
        "prompt": "Generate concise documentation for this code. Include: 1) Purpose and overview, 2) Key functions/classes with brief descriptions, 3) Usage examples, 4) Important configuration or parameters. Format as markdown.",
    },
]

# File extensions to analyze
CODE_EXTENSIONS = {'.cpp', '.h', '.hpp', '.c', '.py', '.js', '.ts', '.rs', '.go'}

def read_file(path: str, max_chars: int = 4000) -> str:
    """Read file content, truncating if too large."""
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        if len(content) > max_chars:
            content = content[:max_chars] + f"\n\n... [truncated, {len(content)} total chars]"
        return content
    except Exception as e:
        return f"Error reading file: {e}"

def dream_about_code(code: str, prompt: str, filename: str, server_url: str) -> dict:
    """Send code + prompt to Z.E.T.A. for analysis."""
    full_prompt = f"""You are analyzing code from {filename}.

CODE:
```
{code}
```

TASK: {prompt}

Provide your analysis in a clear, structured format. Be specific and reference line numbers or function names where relevant."""

    try:
        response = requests.post(
            f"{server_url}/generate",
            json={"prompt": full_prompt, "max_tokens": 1000, "no_context": True},
            timeout=120
        )
        data = response.json()
        return {
            "success": True,
            "output": data.get("output", ""),
            "tokens": data.get("tokens", 0),
            "route": data.get("route", "unknown"),
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def find_code_files(path: str, extensions: set = CODE_EXTENSIONS) -> list:
    """Recursively find all code files in a directory."""
    files = []
    path = Path(path)

    if path.is_file():
        return [str(path)]

    for ext in extensions:
        files.extend(str(p) for p in path.rglob(f"*{ext}"))

    # Sort by modification time (newest first)
    files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    return files

def run_dream_session(
    files: list,
    prompts: list,
    server_url: str,
    output_file: str = None,
    max_files: int = 20,
) -> dict:
    """Run a full dream session over multiple files."""

    results = {
        "timestamp": datetime.now().isoformat(),
        "files_analyzed": 0,
        "total_insights": 0,
        "dreams": [],
    }

    files = files[:max_files]  # Limit files to analyze

    print(f"\n{'='*60}")
    print(f"Z.E.T.A. CODEBASE DREAM SESSION")
    print(f"{'='*60}")
    print(f"Files to analyze: {len(files)}")
    print(f"Prompts per file: {len(prompts)}")
    print(f"Total dreams: {len(files) * len(prompts)}")
    print(f"{'='*60}\n")

    for i, filepath in enumerate(files, 1):
        filename = os.path.basename(filepath)
        print(f"\n[{i}/{len(files)}] Dreaming about: {filename}")
        print("-" * 40)

        code = read_file(filepath)
        if code.startswith("Error"):
            print(f"  Skipping: {code}")
            continue

        file_dreams = {
            "file": filepath,
            "filename": filename,
            "size": len(code),
            "insights": [],
        }

        for prompt_info in prompts:
            prompt_name = prompt_info["name"]
            prompt_text = prompt_info["prompt"]

            print(f"  [{prompt_name}] ", end="", flush=True)
            start = time.time()

            result = dream_about_code(code, prompt_text, filename, server_url)
            elapsed = time.time() - start

            if result["success"]:
                output = result["output"]
                # Clean up the output
                output = output.replace("\\n", "\n")

                insight = {
                    "type": prompt_name,
                    "output": output,
                    "tokens": result["tokens"],
                    "time": round(elapsed, 2),
                }
                file_dreams["insights"].append(insight)
                results["total_insights"] += 1

                # Show preview
                preview = output[:100].replace("\n", " ")
                print(f"({elapsed:.1f}s) {preview}...")
            else:
                print(f"FAILED: {result.get('error', 'unknown')}")

        results["dreams"].append(file_dreams)
        results["files_analyzed"] += 1

    # Save results
    if output_file:
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"\n\nResults saved to: {output_file}")

    return results

def print_summary(results: dict):
    """Print a summary of the dream session."""
    print(f"\n{'='*60}")
    print("DREAM SESSION SUMMARY")
    print(f"{'='*60}")
    print(f"Files analyzed: {results['files_analyzed']}")
    print(f"Total insights: {results['total_insights']}")

    # Group insights by type
    type_counts = {}
    for dream in results["dreams"]:
        for insight in dream["insights"]:
            t = insight["type"]
            type_counts[t] = type_counts.get(t, 0) + 1

    print("\nInsights by type:")
    for t, count in sorted(type_counts.items()):
        print(f"  {t}: {count}")

    # Show top findings per category
    print("\n" + "="*60)
    print("KEY FINDINGS")
    print("="*60)

    for dream in results["dreams"][:5]:  # Top 5 files
        print(f"\n>> {dream['filename']}")
        for insight in dream["insights"][:2]:  # First 2 insights per file
            output = insight["output"][:200].replace("\n", " ")
            print(f"   [{insight['type']}] {output}...")

def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. Codebase Dreaming")
    parser.add_argument("--path", default="/Users/hendrixx./ZetaZero/llama.cpp/tools/zeta-demo",
                        help="Directory to analyze")
    parser.add_argument("--file", help="Single file to analyze")
    parser.add_argument("--prompt", help="Custom prompt (replaces defaults)")
    parser.add_argument("--prompts", help="JSON file with custom prompts")
    parser.add_argument("--max-files", type=int, default=10, help="Max files to analyze")
    parser.add_argument("--output", default="dream_results.json", help="Output file")
    parser.add_argument("--url", default=ZETA_URL, help="Z.E.T.A. server URL")

    args = parser.parse_args()

    server_url = args.url

    # Check server
    try:
        r = requests.get(f"{server_url}/health", timeout=5)
        health = r.json()
        print(f"Connected to Z.E.T.A. v{health.get('version', '?')}")
    except Exception as e:
        print(f"Error: Cannot connect to Z.E.T.A. at {server_url}")
        print(f"  {e}")
        sys.exit(1)

    # Get files
    if args.file:
        files = [args.file]
    else:
        files = find_code_files(args.path)
        if not files:
            print(f"No code files found in {args.path}")
            sys.exit(1)

    # Get prompts
    if args.prompt:
        prompts = [{"name": "custom", "prompt": args.prompt}]
    elif args.prompts:
        with open(args.prompts) as f:
            prompts = json.load(f)
    else:
        prompts = DEFAULT_PROMPTS

    # Run session
    results = run_dream_session(
        files=files,
        prompts=prompts,
        server_url=server_url,
        output_file=args.output,
        max_files=args.max_files,
    )

    print_summary(results)

if __name__ == "__main__":
    main()
