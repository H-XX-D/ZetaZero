#!/usr/bin/env python3
"""
SWE-bench Harness for Z.E.T.A.

Thin wrapper that feeds SWE-bench instances to Z.E.T.A.'s existing endpoints
and collects patches for submission.

Z.E.T.A. handles orchestration internally:
  - 14B: Conscious reasoning
  - 7B: Code specialist
  - 4B: Embeddings for knowledge graph

Usage:
    python swe_bench_zeta.py --dataset princeton-nlp/SWE-bench_Lite --output predictions.jsonl
    python swe_bench_zeta.py --instance django__django-11099  # Single instance test
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Optional, Tuple

import requests

# Z.E.T.A. server configuration
ZETA_HOST = os.environ.get("ZETA_HOST", "192.168.0.165")
ZETA_PORT = os.environ.get("ZETA_PORT", "8080")
ZETA_URL = f"http://{ZETA_HOST}:{ZETA_PORT}"

# Timeouts
GENERATE_TIMEOUT = 300  # 5 minutes per generation
MAX_TOKENS = 4096


class ZetaSWEAgent:
    """Thin wrapper around Z.E.T.A. for SWE-bench tasks."""

    def __init__(self, base_url: str = ZETA_URL):
        self.base_url = base_url
        self.session_id = None

    def health_check(self) -> bool:
        """Check if Z.E.T.A. server is available."""
        try:
            resp = requests.get(f"{self.base_url}/health", timeout=10)
            return resp.status_code == 200
        except:
            return False

    def open_project(self, repo_path: str) -> dict:
        """Open a project in Z.E.T.A. code mode."""
        resp = requests.post(
            f"{self.base_url}/project/open",
            json={"path": repo_path},
            timeout=30
        )
        return resp.json()

    def close_project(self) -> dict:
        """Close current project."""
        resp = requests.post(f"{self.base_url}/project/close", timeout=10)
        return resp.json()

    def new_session(self) -> dict:
        """Start a fresh session to avoid context pollution between instances."""
        try:
            resp = requests.post(f"{self.base_url}/session/new", timeout=10)
            return resp.json()
        except:
            return {"status": "ok"}  # Endpoint might not exist

    def clear_scratch(self) -> dict:
        """Clear the scratch buffer between instances."""
        try:
            resp = requests.post(f"{self.base_url}/scratch/clear", timeout=10)
            return resp.json()
        except:
            return {"status": "ok"}

    def get_scratch_stats(self) -> dict:
        """Get scratch buffer statistics."""
        try:
            resp = requests.get(f"{self.base_url}/scratch/stats", timeout=10)
            return resp.json()
        except:
            return {}

    def generate(self, prompt: str, max_tokens: int = MAX_TOKENS) -> str:
        """Generate response from Z.E.T.A. using full 3-model stack (14B reasoning)."""
        resp = requests.post(
            f"{self.base_url}/generate",
            json={
                "prompt": prompt,
                "max_tokens": max_tokens
            },
            timeout=GENERATE_TIMEOUT
        )
        data = resp.json()
        return data.get("output", "")

    def generate_code(self, prompt: str, max_tokens: int = MAX_TOKENS) -> str:
        """Generate code directly using 7B coder (bypasses 14B reasoning)."""
        resp = requests.post(
            f"{self.base_url}/code",
            json={
                "prompt": prompt,
                "max_tokens": max_tokens
            },
            timeout=GENERATE_TIMEOUT
        )
        data = resp.json()
        return data.get("output", "")

    def store_context(self, context: str) -> dict:
        """Store file/issue context in knowledge graph."""
        resp = requests.post(
            f"{self.base_url}/generate",
            json={
                "prompt": f"Remember: {context}",
                "max_tokens": 50
            },
            timeout=60
        )
        return resp.json()


def clone_repo(repo: str, base_commit: str, work_dir: str) -> str:
    """Clone repo at specific commit for SWE-bench instance."""
    repo_url = f"https://github.com/{repo}.git"
    repo_name = repo.replace("/", "__")
    repo_path = os.path.join(work_dir, repo_name)

    if os.path.exists(repo_path):
        # Reset to specific commit
        subprocess.run(
            ["git", "checkout", "-f", base_commit],
            cwd=repo_path,
            capture_output=True
        )
    else:
        # Fresh clone
        subprocess.run(
            ["git", "clone", "--depth", "100", repo_url, repo_path],
            capture_output=True
        )
        subprocess.run(
            ["git", "checkout", "-f", base_commit],
            cwd=repo_path,
            capture_output=True
        )

    return repo_path


def extract_file_path(line: str) -> Optional[str]:
    """Extract file path from diff header lines."""
    if line.startswith('diff --git'):
        # diff --git a/path/file.py b/path/file.py
        parts = line.split()
        if len(parts) >= 3:
            return parts[2].lstrip('a/')
    elif line.startswith('--- '):
        # --- a/path/file.py or --- path/file.py
        rest = line[4:].strip().split()[0] if line[4:].strip() else None
        if rest:
            return rest.lstrip('a/')
    elif line.startswith('+++ '):
        # +++ b/path/file.py or +++ path/file.py
        rest = line[4:].strip().split()[0] if line[4:].strip() else None
        if rest:
            return rest.lstrip('b/')
    return None


def fix_malformed_diff(patch: str, file_path: str = None) -> str:
    """
    Robust multi-file diff repair.

    Handles:
    - Missing 'diff --git' headers (adds them when new file section detected)
    - Missing 'a/' and 'b/' prefixes in file paths
    - Multiple files in a single patch
    - Malformed hunk headers
    """
    if not patch or patch.startswith('#'):
        return patch

    lines = patch.split('\n')
    fixed_lines = []
    current_file = None
    i = 0

    while i < len(lines):
        line = lines[i]

        # Handle diff --git header
        if line.startswith('diff --git'):
            current_file = extract_file_path(line)
            fixed_lines.append(line)
            i += 1
            continue

        # Handle --- line (start of file section or new file in multi-file patch)
        if line.startswith('---'):
            new_file = extract_file_path(line)

            # Check if this is a new file (different from current)
            if new_file and new_file != current_file:
                # Insert missing diff --git header
                fixed_lines.append(f'diff --git a/{new_file} b/{new_file}')
                current_file = new_file

            # Ensure proper --- a/ format
            if new_file and not line.startswith('--- a/'):
                fixed_lines.append(f'--- a/{new_file}')
            else:
                fixed_lines.append(line)
            i += 1
            continue

        # Handle +++ line
        if line.startswith('+++'):
            new_file = extract_file_path(line)

            # Use current_file if we couldn't extract from this line
            target_file = new_file or current_file

            # Ensure proper +++ b/ format
            if target_file and not line.startswith('+++ b/'):
                fixed_lines.append(f'+++ b/{target_file}')
            else:
                fixed_lines.append(line)
            i += 1
            continue

        # Handle hunk headers - validate format
        if line.startswith('@@'):
            # Ensure proper @@ format: @@ -start,count +start,count @@
            if not re.match(r'^@@ -\d+(?:,\d+)? \+\d+(?:,\d+)? @@', line):
                # Try to fix malformed hunk header
                nums = re.findall(r'[-+]?\d+', line)
                if len(nums) >= 2:
                    fixed_lines.append(f'@@ -{nums[0]},1 +{nums[1]},1 @@')
                else:
                    fixed_lines.append(line)
            else:
                fixed_lines.append(line)
            i += 1
            continue

        # All other lines pass through
        fixed_lines.append(line)
        i += 1

    result = '\n'.join(fixed_lines)

    # If we still don't have a diff --git header and have file info, add it
    if 'diff --git' not in result and ('--- a/' in result or '---' in result):
        # Find first file path
        for line in fixed_lines:
            fp = extract_file_path(line)
            if fp:
                result = f'diff --git a/{fp} b/{fp}\n' + result
                break

    # Ensure patch ends with newline
    if result and not result.endswith('\n'):
        result += '\n'

    return result


def validate_patch(patch: str, repo_path: str) -> tuple[bool, str]:
    """Validate a patch can be applied cleanly using git apply --check."""
    if not patch or patch.startswith('#'):
        return False, "Empty or invalid patch"

    try:
        # Write patch to temp file
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', suffix='.patch', delete=False) as f:
            f.write(patch)
            patch_file = f.name

        # Test with git apply --check
        result = subprocess.run(
            ["git", "apply", "--check", patch_file],
            cwd=repo_path,
            capture_output=True,
            text=True,
            timeout=10
        )

        os.unlink(patch_file)

        if result.returncode == 0:
            return True, "OK"
        else:
            return False, result.stderr.strip()
    except Exception as e:
        return False, str(e)


def extract_patch(response: str) -> str:
    """Extract unified diff patch from Z.E.T.A. response."""
    # Look for diff blocks
    diff_patterns = [
        r'```diff\n(.*?)```',
        r'```patch\n(.*?)```',
        r'(diff --git.*?)(?=```|$)',
        r'(---.*?\+\+\+.*?(?:\n[-+ @].*)+)',
    ]

    for pattern in diff_patterns:
        matches = re.findall(pattern, response, re.DOTALL)
        if matches:
            patch = matches[0].strip()
            # Validate it looks like a real diff
            if '---' in patch and '+++' in patch:
                # Try to fix any formatting issues
                return fix_malformed_diff(patch)

    # Try to find inline diff markers
    lines = response.split('\n')
    diff_lines = []
    in_diff = False

    for line in lines:
        if line.startswith('diff --git') or line.startswith('---'):
            in_diff = True
        if in_diff:
            diff_lines.append(line)
            if line.strip() == '' and len(diff_lines) > 3:
                # End of diff block
                break

    if diff_lines:
        patch = '\n'.join(diff_lines)
        return fix_malformed_diff(patch)

    # Last resort: try to extract Python code and convert to pseudo-diff
    code_pattern = r'```python\n(.*?)```'
    code_matches = re.findall(code_pattern, response, re.DOTALL)
    if code_matches:
        # Take the first substantial code block as a patch hint
        for code in code_matches:
            if len(code.strip()) > 50 and ('def ' in code or 'class ' in code):
                # Create a minimal pseudo-diff
                return f"# Code suggestion (convert to proper diff):\n{code.strip()}"

    return ""


def chunk_file_to_graph(agent: 'ZetaSWEAgent', file_path: str, repo_path: str) -> int:
    """Store file summary in knowledge graph (fast - just key info)."""
    try:
        with open(file_path, 'r') as f:
            content = f.read()
    except:
        return 0

    rel_path = file_path.replace(repo_path + "/", "")
    lines = content.split('\n')

    # Skip very large files
    if len(lines) > 300:
        return 0

    # Extract key elements: classes, functions, docstrings
    import re
    classes = re.findall(r'^class\s+(\w+)', content, re.MULTILINE)
    functions = re.findall(r'^def\s+(\w+)', content, re.MULTILINE)

    # Store just one fact per file with key info
    summary = f"File {rel_path}: classes={classes[:5]}, functions={functions[:10]}, lines={len(lines)}"
    agent.store_context(summary)

    return 1


def index_repo_to_graph(agent: 'ZetaSWEAgent', repo_path: str, max_files: int = 10) -> int:
    """Index repository Python files into the knowledge graph (fast mode)."""
    total_indexed = 0

    # Find Python files, prioritizing likely important ones
    try:
        result = subprocess.run(
            ["find", repo_path, "-name", "*.py", "-type", "f", "-size", "-50k"],
            capture_output=True, text=True, timeout=30
        )
        all_files = [f for f in result.stdout.strip().split('\n') if f]
    except:
        return 0

    # Prioritize: not tests, not __pycache__, shorter paths (core modules)
    def priority(f):
        score = 0
        if 'test' in f.lower(): score += 100
        if '__pycache__' in f: score += 1000
        if 'example' in f.lower(): score += 50
        if 'contrib' in f.lower(): score += 30
        score += len(f.split('/'))  # Prefer shallower paths
        return score

    files = sorted(all_files, key=priority)[:max_files]

    for fpath in files:
        indexed = chunk_file_to_graph(agent, fpath, repo_path)
        total_indexed += indexed

    return total_indexed


def query_graph_for_code(agent: 'ZetaSWEAgent', query: str, top_k: int = 5) -> list:
    """Query the knowledge graph for relevant code snippets."""
    try:
        resp = requests.post(
            f"{agent.base_url}/memory/query",
            json={"query": query, "top_k": top_k},
            timeout=30
        )
        data = resp.json()
        return data.get("results", [])
    except:
        return []


def find_relevant_files(repo_path: str, problem: str, hints: str = "") -> list:
    """Find files likely related to the bug using grep and file patterns."""
    relevant_files = []

    # Extract potential file/class/function names from problem statement
    keywords = []

    # Look for Python identifiers in backticks or quotes
    import re
    identifiers = re.findall(r'`([a-zA-Z_][a-zA-Z0-9_]*)`', problem)
    identifiers += re.findall(r"'([a-zA-Z_][a-zA-Z0-9_]*)'", problem)
    keywords.extend(identifiers[:5])  # Top 5

    # Search for files containing these keywords
    for kw in keywords:
        if len(kw) > 3:  # Skip short keywords
            try:
                result = subprocess.run(
                    ["grep", "-rl", "--include=*.py", kw, repo_path],
                    capture_output=True, text=True, timeout=10
                )
                files = result.stdout.strip().split('\n')[:3]  # Top 3 per keyword
                relevant_files.extend([f for f in files if f])
            except:
                pass

    # Dedupe and limit
    seen = set()
    unique = []
    for f in relevant_files:
        if f not in seen:
            seen.add(f)
            unique.append(f)

    return unique[:5]  # Max 5 files


def read_file_content(file_path: str, max_lines: int = 200) -> str:
    """Read file content, truncated if too long."""
    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()

        if len(lines) > max_lines:
            # Return first and last portions
            content = ''.join(lines[:100])
            content += f"\n... ({len(lines) - 150} lines omitted) ...\n"
            content += ''.join(lines[-50:])
            return content
        return ''.join(lines)
    except:
        return ""


def build_prompt(instance: dict, repo_path: str) -> str:
    """Build the prompt for Z.E.T.A. from SWE-bench instance."""
    problem_statement = instance.get("problem_statement", "")
    hints = instance.get("hints_text", "")

    # Find and read relevant files
    relevant_files = find_relevant_files(repo_path, problem_statement, hints)

    file_contents = ""
    for fpath in relevant_files[:3]:  # Max 3 files in context
        content = read_file_content(fpath, max_lines=150)
        if content:
            rel_path = fpath.replace(repo_path + "/", "")
            file_contents += f"\n### {rel_path}\n```python\n{content}\n```\n"

    prompt = f"""Fix this bug. Output ONLY a diff patch, no explanation.

## Bug Report
{problem_statement[:2000]}

## Relevant Files
{file_contents if file_contents else "Search the repository to find relevant files."}

## Output Format
Output ONLY a unified diff patch. Nothing else. Example:

```diff
diff --git a/path/to/file.py b/path/to/file.py
--- a/path/to/file.py
+++ b/path/to/file.py
@@ -10,7 +10,7 @@
 context
-old_line
+new_line
 context
```

Generate the minimal patch to fix this bug:

```diff"""

    return prompt


def generate_diff(original: str, modified: str, file_path: str) -> str:
    """Generate a unified diff from original and modified content."""
    import difflib

    orig_lines = original.splitlines(keepends=True)
    mod_lines = modified.splitlines(keepends=True)

    diff = difflib.unified_diff(
        orig_lines, mod_lines,
        fromfile=f'a/{file_path}',
        tofile=f'b/{file_path}'
    )

    diff_text = ''.join(diff)
    if diff_text:
        return f'diff --git a/{file_path} b/{file_path}\n{diff_text}'
    return ""


def solve_instance(agent: ZetaSWEAgent, instance: dict, work_dir: str) -> dict:
    """Solve a single SWE-bench instance using Z.E.T.A.'s full 3-model stack with knowledge graph."""
    instance_id = instance["instance_id"]
    repo = instance["repo"]
    base_commit = instance["base_commit"]

    print(f"  Solving: {instance_id}")

    # Clone repo at correct commit
    print(f"    Cloning {repo}@{base_commit[:8]}...")
    repo_path = clone_repo(repo, base_commit, work_dir)

    # Clear scratch buffer for fresh working memory
    agent.clear_scratch()
    agent.new_session()

    # Open project in Z.E.T.A. code mode
    print(f"    Opening project in Z.E.T.A....")
    agent.open_project(repo_path)

    start_time = time.time()
    problem = instance.get("problem_statement", "")

    # PHASE 1: Index repository into knowledge graph (uses 4B embedding model)
    print(f"    Phase 1: Indexing repo to graph...")
    chunks_indexed = index_repo_to_graph(agent, repo_path, max_files=15)
    print(f"      Indexed {chunks_indexed} code chunks")

    # PHASE 2: Query graph for relevant code using semantic search
    print(f"    Phase 2: Semantic search for relevant code...")
    graph_results = query_graph_for_code(agent, problem[:500], top_k=5)

    # Build context from graph results
    graph_context = ""
    for r in graph_results:
        if r.get("value"):
            graph_context += f"\n{r['value'][:400]}\n"

    # Also get file-based context
    relevant_files = find_relevant_files(repo_path, problem)

    # PHASE 3: Identify which file needs fixing
    print(f"    Phase 3: Identifying target file...")

    # Try to extract file path from problem statement directly
    import re
    file_mentions = re.findall(r'[\w/]+\.py', problem)

    # Filter to files that exist
    target_file = None
    target_path = None

    # First, check problem statement mentions
    for fm in file_mentions:
        test_path = os.path.join(repo_path, fm)
        if os.path.exists(test_path) and 'test' not in fm.lower():
            target_file = fm
            target_path = test_path
            break

    # If not found, use best relevant file (non-test)
    if not target_file:
        for rf in relevant_files:
            rel = rf.replace(repo_path + "/", "")
            if 'test' not in rel.lower() and os.path.exists(rf):
                target_file = rel
                target_path = rf
                break

    # Last resort: any relevant file
    if not target_file and relevant_files:
        target_file = relevant_files[0].replace(repo_path + "/", "")
        target_path = relevant_files[0]

    if not target_file:
        print(f"    ✗ No target file found")

    print(f"      Target: {target_file}")

    # PHASE 4: Read the actual file content
    original_content = ""
    if target_path and os.path.exists(target_path):
        with open(target_path, 'r') as f:
            original_content = f.read()

    if not original_content:
        print(f"    ✗ Could not read target file")
        agent.close_project()
        return {
            "instance_id": instance_id,
            "model_name_or_path": "zeta-14b-7b-4b",
            "model_patch": "",
            "patch_valid": False,
            "validation_msg": "Could not read target file",
            "elapsed_time": time.time() - start_time,
            "chunks_indexed": chunks_indexed,
            "graph_hits": len(graph_results),
            "full_response": ""
        }

    # PHASE 5: Generate the fix as a diff directly
    print(f"    Phase 4: Generating fix...")

    # Truncate very long files for context
    lines = original_content.split('\n')
    if len(lines) > 300:
        original_content = '\n'.join(lines[:300])

    # Extract key function/class from problem
    keywords = re.findall(r'`([a-zA-Z_][a-zA-Z0-9_]+)`', problem)
    key_func = keywords[0] if keywords else "the function"

    # Use 7B coder directly for code generation (bypasses 14B reasoning)
    fix_prompt = f"""Fix this bug in {target_file}:

{problem[:600]}

Current code:
```python
{original_content[:3000]}
```

Output the corrected code as a unified diff:
```diff"""

    # Use generate_code for direct 7B coder access
    fix_response = agent.generate_code(fix_prompt, max_tokens=2000)

    # Z.E.T.A. often outputs "SECTION X:" analysis - filter to just code
    lines = fix_response.split('\n')
    code_lines = []
    in_code = False

    for line in lines:
        stripped = line.strip()
        # Skip SECTION headers and explanation text
        if stripped.startswith('SECTION') or stripped.startswith('To ') or stripped.startswith('The '):
            continue
        if stripped.startswith('- ') and not stripped.startswith('- #'):
            continue  # Skip bullet points
        if stripped.startswith('Next steps') or stripped.startswith('Furthermore'):
            continue

        # Detect code patterns
        if (stripped.startswith('def ') or stripped.startswith('class ') or
            stripped.startswith('import ') or stripped.startswith('from ') or
            stripped.startswith('@') or stripped.startswith('#') or
            stripped.startswith('if ') or stripped.startswith('return ') or
            stripped.startswith('self.') or stripped.startswith('raise ') or
            line.startswith('    ') or line.startswith('\t') or
            stripped == '' or stripped.startswith('"""') or stripped.startswith("'''")):
            in_code = True
            code_lines.append(line)
        elif in_code and (stripped.endswith(':') or '=' in stripped or '(' in stripped):
            code_lines.append(line)

    code = '\n'.join(code_lines).strip()

    # If we got code, try to make a diff - but only if it looks like a real fix
    if code and len(code) > 100 and ('def ' in code or 'class ' in code):
        import difflib
        orig_lines = original_content.split('\n')
        code_lines_list = code.split('\n')

        # Only generate diff if the code is similar enough to original
        matcher = difflib.SequenceMatcher(None, orig_lines[:50], code_lines_list[:50])
        ratio = matcher.ratio()

        if ratio > 0.3:  # At least 30% similar - likely a valid fix
            diff = difflib.unified_diff(
                orig_lines, code_lines_list,
                fromfile=f'a/{target_file}',
                tofile=f'b/{target_file}',
                lineterm=''
            )
            fix_response = f"diff --git a/{target_file} b/{target_file}\n" + '\n'.join(diff)

    elapsed = time.time() - start_time

    # Extract diff directly from response
    patch = extract_patch(fix_response)

    # Apply fix_malformed_diff to clean up formatting
    if patch:
        patch = fix_malformed_diff(patch, target_file)

    # Validate patch can be applied
    patch_valid = False
    validation_msg = ""
    if patch and not patch.startswith('#'):
        patch_valid, validation_msg = validate_patch(patch, repo_path)
        if patch_valid:
            print(f"    ✓ Patch validates OK")
        else:
            print(f"    ✗ Patch validation failed: {validation_msg[:60]}")

    # Close project
    agent.close_project()

    print(f"    Done in {elapsed:.1f}s, patch: {len(patch)} chars, graph hits: {len(graph_results)}")

    return {
        "instance_id": instance_id,
        "model_name_or_path": "zeta-14b-7b-4b",
        "model_patch": patch,
        "patch_valid": patch_valid,
        "validation_msg": validation_msg,
        "elapsed_time": elapsed,
        "chunks_indexed": chunks_indexed,
        "graph_hits": len(graph_results),
        "full_response": fix_response,
        "target_file": target_file
    }


def load_dataset(dataset_name: str, limit: Optional[int] = None) -> list:
    """Load SWE-bench dataset."""
    try:
        from datasets import load_dataset
        ds = load_dataset(dataset_name, split="test")
        instances = list(ds)
        if limit:
            instances = instances[:limit]
        return instances
    except ImportError:
        print("ERROR: 'datasets' library required. Install with: pip install datasets")
        return []


def run_benchmark(
    dataset_name: str = "princeton-nlp/SWE-bench_Lite",
    output_path: str = "predictions.jsonl",
    work_dir: Optional[str] = None,
    limit: Optional[int] = None,
    instance_id: Optional[str] = None
):
    """Run SWE-bench evaluation using Z.E.T.A."""

    agent = ZetaSWEAgent()

    # Check connection
    print("Connecting to Z.E.T.A. server...")
    if not agent.health_check():
        print(f"ERROR: Cannot connect to Z.E.T.A. at {ZETA_URL}")
        return
    print(f"Connected to {ZETA_URL}")

    # Setup work directory
    if work_dir is None:
        work_dir = tempfile.mkdtemp(prefix="swebench_")
    os.makedirs(work_dir, exist_ok=True)
    print(f"Work directory: {work_dir}")

    # Load dataset
    print(f"Loading dataset: {dataset_name}...")
    instances = load_dataset(dataset_name, limit)

    if not instances:
        print("No instances loaded!")
        return

    # Filter to single instance if specified
    if instance_id:
        instances = [i for i in instances if i["instance_id"] == instance_id]
        if not instances:
            print(f"Instance {instance_id} not found!")
            return

    print(f"Loaded {len(instances)} instances")
    print("=" * 60)

    # Process instances
    results = []
    predictions = []

    for i, instance in enumerate(instances):
        print(f"\n[{i+1}/{len(instances)}] {instance['instance_id']}")

        try:
            result = solve_instance(agent, instance, work_dir)
            results.append(result)

            # Write prediction in SWE-bench format
            prediction = {
                "instance_id": result["instance_id"],
                "model_name_or_path": result["model_name_or_path"],
                "model_patch": result["model_patch"]
            }
            predictions.append(prediction)

            # Append to output file (streaming writes)
            with open(output_path, "a") as f:
                f.write(json.dumps(prediction) + "\n")

        except Exception as e:
            print(f"    ERROR: {e}")
            # Write empty prediction for failed instance
            prediction = {
                "instance_id": instance["instance_id"],
                "model_name_or_path": "zeta-14b-7b-4b",
                "model_patch": ""
            }
            with open(output_path, "a") as f:
                f.write(json.dumps(prediction) + "\n")

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    patches_generated = sum(1 for r in results if r.get("model_patch"))
    patches_valid = sum(1 for r in results if r.get("patch_valid"))
    print(f"Instances processed: {len(results)}")
    print(f"Patches generated: {patches_generated}")
    print(f"Patches validated: {patches_valid} ({100*patches_valid/max(patches_generated,1):.0f}%)")
    print(f"Output: {output_path}")

    # Write detailed results
    results_path = output_path.replace(".jsonl", "_detailed.json")
    with open(results_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"Detailed results: {results_path}")


def main():
    parser = argparse.ArgumentParser(description="SWE-bench harness for Z.E.T.A.")
    parser.add_argument(
        "--dataset",
        default="princeton-nlp/SWE-bench_Lite",
        help="Dataset to evaluate on"
    )
    parser.add_argument(
        "--output",
        default="predictions.jsonl",
        help="Output predictions file (JSONL)"
    )
    parser.add_argument(
        "--work-dir",
        help="Directory for cloned repos (default: temp)"
    )
    parser.add_argument(
        "--limit",
        type=int,
        help="Limit number of instances (for testing)"
    )
    parser.add_argument(
        "--instance",
        help="Run single instance by ID"
    )
    parser.add_argument(
        "--host",
        default=ZETA_HOST,
        help="Z.E.T.A. server host"
    )
    parser.add_argument(
        "--port",
        default=ZETA_PORT,
        help="Z.E.T.A. server port"
    )

    args = parser.parse_args()

    # Update global config
    global ZETA_URL
    ZETA_URL = f"http://{args.host}:{args.port}"

    # Clear output file
    if os.path.exists(args.output) and not args.instance:
        os.remove(args.output)

    run_benchmark(
        dataset_name=args.dataset,
        output_path=args.output,
        work_dir=args.work_dir,
        limit=args.limit,
        instance_id=args.instance
    )


if __name__ == "__main__":
    main()
