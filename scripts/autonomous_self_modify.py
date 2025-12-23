#!/usr/bin/env python3
"""
Z.E.T.A. Autonomous Self-Modification Runner

This script orchestrates Z.E.T.A.'s self-improvement loop:
1. Fetch pending dreams from z6
2. Extract actionable C++ patches
3. Apply patches to source files
4. Compile and test
5. Auto-fix compilation errors
6. Commit successful changes
7. Repeat

WARNING: This enables recursive self-modification.
Only run in isolated branches (autonomous-self-recursion).
"""

import os
import sys
import re
import json
import time
import subprocess
import requests
from datetime import datetime
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional, Tuple
from enum import Enum

# Configuration
ZETA_URL = "http://192.168.0.165:8080"
Z6_HOST = "z6"
REMOTE_SOURCE_DIR = "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo"
REMOTE_BUILD_DIR = "/home/xx/ZetaZero/llama.cpp/build"
LOCAL_SOURCE_DIR = "/Users/hendrixx./ZetaZero/llama.cpp/tools/zeta-demo"
REMOTE_DREAMS_DIR = "/mnt/HoloGit/dreams/pending"
LOG_FILE = "/tmp/zeta_autonomous.log"

# Limits
MAX_PATCHES_PER_CYCLE = 5
MAX_FIX_ATTEMPTS = 3
CYCLE_DELAY_SECONDS = 120
MIN_CONFIDENCE = 0.6


class PatchType(Enum):
    INSERT = "insert"
    REPLACE = "replace"
    DELETE = "delete"
    APPEND = "append"


@dataclass
class CodePatch:
    id: str
    target_file: str
    patch_type: PatchType
    new_code: str
    search_pattern: str = ""
    description: str = ""
    confidence: float = 0.5
    dream_source: str = ""
    applied: bool = False
    compiled: bool = False
    error_message: str = ""


@dataclass
class CycleResult:
    patches_attempted: int = 0
    patches_applied: int = 0
    patches_compiled: int = 0
    errors_fixed: int = 0
    messages: List[str] = field(default_factory=list)


def log(msg: str, level: str = "INFO"):
    """Log message to console and file."""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    formatted = f"[{timestamp}] [{level}] {msg}"
    print(formatted)

    with open(LOG_FILE, "a") as f:
        f.write(formatted + "\n")


def ssh_run(cmd: str, capture: bool = True) -> Tuple[bool, str]:
    """Run command on z6 via SSH."""
    try:
        result = subprocess.run(
            ["ssh", Z6_HOST, cmd],
            capture_output=capture,
            text=True,
            timeout=300
        )
        return result.returncode == 0, result.stdout + result.stderr
    except Exception as e:
        return False, str(e)


def fetch_pending_dreams(cpp_only: bool = True) -> List[str]:
    """Fetch pending dreams from Z.E.T.A.'s dream directory.

    Args:
        cpp_only: If True, only return dreams containing C++ code
    """
    try:
        # First, find dreams that contain C++ code blocks
        if cpp_only:
            # Search for dreams with actual cpp code blocks
            success, output = ssh_run(
                f'grep -l "```cpp\\|class Zeta\\|struct Zeta\\|void zeta_" {REMOTE_DREAMS_DIR}/*.txt 2>/dev/null | head -30'
            )
        else:
            success, output = ssh_run(f"ls -t {REMOTE_DREAMS_DIR}/*.txt 2>/dev/null | head -50")

        if not success or not output.strip():
            log("No dream files found")
            return []

        dream_files = output.strip().split("\n")
        log(f"Found {len(dream_files)} dream files with C++ code" if cpp_only else f"Found {len(dream_files)} dream files")

        dreams = []
        for filepath in dream_files:
            filepath = filepath.strip()
            if not filepath:
                continue

            # Read dream content
            success, content = ssh_run(f"cat '{filepath}'")
            if not success or not content:
                continue

            # Filter for C++ code if requested
            if cpp_only:
                # Check for C++ indicators
                has_cpp = any(indicator in content for indicator in [
                    '```cpp', '```c++', 'class Zeta', 'struct Zeta',
                    'void zeta_', 'bool zeta_', 'std::', '#include',
                    'namespace zeta', 'inline ', 'const char*'
                ])
                if not has_cpp:
                    continue

            # Extract just the content (skip category and timestamp lines)
            lines = content.split('\n')
            if len(lines) > 2:
                # First line is category, second is timestamp, rest is content
                dream_content = '\n'.join(lines[2:])
                dreams.append(dream_content)

        log(f"Filtered to {len(dreams)} C++ dreams" if cpp_only else f"Loaded {len(dreams)} dreams")
        return dreams

    except Exception as e:
        log(f"Error fetching dreams: {e}", "ERROR")
        return []


def extract_patches_from_dream(dream: str, dream_id: str) -> List[CodePatch]:
    """Extract actionable C++ patches from dream content."""
    patches = []

    # Look for code blocks - must be explicitly marked as cpp/c++
    code_block_pattern = r"```(?:cpp|c\+\+)\s*(?://\s*FILE:\s*(\S+))?\n([\s\S]*?)```"
    matches = re.finditer(code_block_pattern, dream, re.IGNORECASE)

    patch_num = 0
    for match in matches:
        file_hint = match.group(1) or ""
        code = match.group(2).strip()

        if not code:
            continue

        patch = CodePatch(
            id=f"{dream_id}_p{patch_num}",
            target_file=infer_target_file(file_hint, code, dream),
            patch_type=infer_patch_type(dream),
            new_code=code,
            confidence=calculate_confidence(code, dream),
            description=extract_description(dream),
            dream_source=dream
        )

        if patch.patch_type == PatchType.REPLACE:
            patch.search_pattern = extract_search_pattern(dream)

        if patch.target_file and patch.confidence >= MIN_CONFIDENCE:
            patches.append(patch)
            patch_num += 1

    return patches


def infer_target_file(hint: str, code: str, dream: str) -> str:
    """Infer which file the patch should apply to."""
    if hint:
        return hint.split("/")[-1]

    # Look for file mentions
    file_pattern = r"(zeta-\w+\.(?:h|cpp))"
    match = re.search(file_pattern, dream)
    if match:
        return match.group(1)

    # Infer from code content
    if "class ZetaHRM" in code or "zeta_hrm" in code:
        return "zeta-hrm.h"
    if "class ZetaTRM" in code or "zeta_trm" in code:
        return "zeta-trm.h"
    if "DreamState" in code or "dream_" in code:
        return "zeta-dream.h"
    if "EmbeddingCache" in code or "zeta_embed" in code:
        return "zeta-embed-integration.h"
    if "Router" in code or "route" in code:
        return "zeta-utils.h"
    if "handle_" in code or "endpoint" in code:
        return "zeta-server.cpp"

    return "zeta-utils.h"


def infer_patch_type(dream: str) -> PatchType:
    """Infer the type of patch from dream content."""
    lower = dream.lower()

    if "replace" in lower or "change" in lower or "modify" in lower:
        return PatchType.REPLACE
    if "remove" in lower or "delete" in lower:
        return PatchType.DELETE
    if "append" in lower or "add to end" in lower:
        return PatchType.APPEND

    return PatchType.INSERT


def is_valid_cpp(code: str) -> bool:
    """Check if code looks like valid C++."""
    # Must have basic C++ syntax
    has_braces = "{" in code and "}" in code
    has_semicolons = ";" in code
    has_cpp_keywords = any(kw in code for kw in [
        'void ', 'int ', 'bool ', 'float ', 'double ', 'char ',
        'class ', 'struct ', 'enum ', 'template', 'namespace ',
        'return ', 'if (', 'for (', 'while (', 'switch (',
        'std::', 'const ', 'static ', 'inline '
    ])

    # Must NOT look like markdown or prose
    has_markdown = code.strip().startswith('#') or '####' in code
    is_mostly_prose = len(re.findall(r'[a-zA-Z]{10,}', code)) > 10  # Too many long words

    return has_braces and has_semicolons and has_cpp_keywords and not has_markdown and not is_mostly_prose


def calculate_confidence(code: str, dream: str) -> float:
    """Calculate confidence score for a patch."""
    # Start with validity check
    if not is_valid_cpp(code):
        return 0.0

    confidence = 0.5

    # Boost for complete definitions
    if "class " in code or "struct " in code:
        confidence += 0.2
    if "{" in code and "}" in code:
        confidence += 0.1

    # Boost for function definitions
    if re.search(r'\w+\s+\w+\s*\([^)]*\)\s*{', code):
        confidence += 0.15

    # Boost for actionable keywords
    lower = dream.lower()
    if "optimize" in lower or "fix" in lower or "improve" in lower:
        confidence += 0.1

    # Penalty for short code
    if len(code) < 50:
        confidence -= 0.2

    # Penalty for very long code (might be incomplete)
    if len(code) > 2000:
        confidence -= 0.1

    return max(0.0, min(1.0, confidence))


def extract_description(dream: str) -> str:
    """Extract a short description from dream content."""
    period = dream.find(".")
    if 0 < period < 200:
        return dream[:period + 1]
    return dream[:100]


def extract_search_pattern(dream: str) -> str:
    """Extract search pattern for replacement patches."""
    match = re.search(r'replace\s+[`"]?([^`"]+)[`"]?\s+with', dream, re.IGNORECASE)
    if match:
        return match.group(1)
    return ""


def apply_patch(patch: CodePatch) -> Tuple[bool, str]:
    """Apply a patch to the source file on z6."""
    filepath = f"{REMOTE_SOURCE_DIR}/{patch.target_file}"

    # Create backup
    ssh_run(f"cp {filepath} {filepath}.bak")

    if patch.patch_type == PatchType.INSERT:
        # Insert before #endif or append
        cmd = f"""
        if grep -q '#endif' {filepath}; then
            sed -i '/#endif/i\\
// === Auto-inserted by self-modify (dream: {patch.id}) ===\\
{patch.new_code.replace("'", "'\"'\"'")}
' {filepath}
        else
            echo '// === Auto-inserted ===
{patch.new_code}' >> {filepath}
        fi
        """
    elif patch.patch_type == PatchType.APPEND:
        cmd = f"echo '{patch.new_code}' >> {filepath}"
    elif patch.patch_type == PatchType.REPLACE and patch.search_pattern:
        escaped_pattern = patch.search_pattern.replace("/", "\\/")
        escaped_new = patch.new_code.replace("/", "\\/")
        cmd = f"sed -i 's/{escaped_pattern}/{escaped_new}/g' {filepath}"
    else:
        return False, "Unknown patch type or missing pattern"

    success, output = ssh_run(cmd)
    return success, output


def compile_on_z6() -> Tuple[bool, str, List[str]]:
    """Compile zeta-server on z6."""
    cmd = f"cd {REMOTE_BUILD_DIR} && cmake --build . --target zeta-server 2>&1"
    success, output = ssh_run(cmd)

    # Extract errors
    errors = []
    for line in output.split("\n"):
        if "error:" in line:
            errors.append(line.strip())

    return success, output, errors


def restart_server_on_z6() -> Tuple[bool, str]:
    """Stop and restart zeta-server on z6."""
    log("Restarting zeta-server...")

    # Kill existing server
    kill_cmd = "pkill -f 'zeta-server' || true"
    ssh_run(kill_cmd)

    time.sleep(2)

    # Start new server in background using the existing config
    start_cmd = """cd /home/xx/ZetaZero && source ./zeta.conf && \
        nohup ./llama.cpp/build/bin/zeta-server \
        --model "$MODEL_14B" \
        --model-coder "$MODEL_7B" \
        --model-embed "$MODEL_4B" \
        -ngl 99 -c 4096 --host 0.0.0.0 --port 8080 \
        > /tmp/zeta.log 2>&1 &"""

    success, output = ssh_run(start_cmd)

    # Wait for server to start
    time.sleep(5)

    # Check if server is responding
    try:
        resp = requests.get(f"{ZETA_URL}/health", timeout=10)
        if resp.status_code == 200:
            log("Server restarted successfully")
            return True, "Server running"
    except:
        pass

    return False, "Server failed to restart"


def update_self_graph(modified_files: List[str]) -> bool:
    """Feed modified source files back into Z.E.T.A.'s memory graph.

    This maintains self-awareness by updating the graph with the new code.
    """
    log(f"Updating self-graph with {len(modified_files)} modified files...")

    for filename in modified_files:
        filepath = f"{REMOTE_SOURCE_DIR}/{filename}"

        # Read file content
        success, content = ssh_run(f"cat '{filepath}' 2>/dev/null | head -100")
        if not success or not content:
            continue

        # Extract key structures for context
        structures = []
        for line in content.split('\n'):
            if 'class ' in line or 'struct ' in line or 'void ' in line:
                structures.append(line.strip()[:80])

        prompt = f"""I just modified my own source file: {filename}

Key structures in this file:
{chr(10).join(structures[:10])}

Remember: This is part of my own codebase. I modified it through autonomous self-improvement.
Update my understanding of what this file does and how it connects to my other components."""

        try:
            resp = requests.post(
                f"{ZETA_URL}/generate",
                json={"prompt": prompt, "max_tokens": 200},
                timeout=60
            )
            if resp.status_code == 200:
                log(f"  Updated graph with {filename}")
            else:
                log(f"  Failed to update graph with {filename}", "WARN")
        except Exception as e:
            log(f"  Error updating graph: {e}", "ERROR")

        time.sleep(1)  # Rate limit

    return True


def revert_patch(patch: CodePatch) -> bool:
    """Revert a patch by restoring from backup."""
    filepath = f"{REMOTE_SOURCE_DIR}/{patch.target_file}"
    success, _ = ssh_run(f"cp {filepath}.bak {filepath}")
    return success


def git_commit_patch(patch: CodePatch) -> bool:
    """Commit a successful patch."""
    cmd = f"""cd {REMOTE_SOURCE_DIR} && \
        git add {patch.target_file} && \
        git commit -m 'Auto-patch: {patch.description[:50]} (dream: {patch.id})'"""
    success, _ = ssh_run(cmd)
    return success


def try_fix_error(error: str) -> Optional[CodePatch]:
    """Generate a fix patch for a compilation error."""
    # Parse error
    match = re.search(r"([^:]+):(\d+):(\d+):\s*error:\s*(.+)", error)
    if not match:
        return None

    file_path = match.group(1)
    line_num = int(match.group(2))
    message = match.group(4)

    fix = CodePatch(
        id=f"autofix_{int(time.time())}",
        target_file=file_path.split("/")[-1],
        patch_type=PatchType.INSERT,
        new_code="",
        confidence=0.3
    )

    if "undeclared identifier" in message or "was not declared" in message:
        # Extract identifier
        id_match = re.search(r"'(\w+)'", message)
        if id_match:
            identifier = id_match.group(1)
            fix.new_code = f"// TODO: Define {identifier} (auto-fix)"
            fix.description = f"Add missing declaration for {identifier}"
            return fix

    if "expected ';'" in message:
        fix.new_code = ";"
        fix.patch_type = PatchType.APPEND
        fix.description = "Add missing semicolon"
        return fix

    return None


def run_cycle(dreams: List[str]) -> CycleResult:
    """Run one self-modification cycle."""
    result = CycleResult()
    compiled_files = []  # Track successfully compiled files

    log("=== Self-Modification Cycle Started ===")
    log(f"Processing {len(dreams)} dreams")

    # Extract patches from all dreams
    all_patches = []
    for i, dream in enumerate(dreams):
        patches = extract_patches_from_dream(dream, f"dream_{i}")
        all_patches.extend(patches)
        if len(all_patches) >= MAX_PATCHES_PER_CYCLE:
            break

    log(f"Extracted {len(all_patches)} patches")

    if not all_patches:
        log("No actionable patches found")
        return result

    # Sort by confidence
    all_patches.sort(key=lambda p: p.confidence, reverse=True)

    # Apply patches one by one
    for patch in all_patches[:MAX_PATCHES_PER_CYCLE]:
        result.patches_attempted += 1

        log(f"Applying patch {patch.id} to {patch.target_file} (conf: {patch.confidence:.2f})")
        log(f"  Description: {patch.description}")

        success, output = apply_patch(patch)
        if not success:
            log(f"  SKIP: Failed to apply - {output}", "WARN")
            continue

        result.patches_applied += 1
        log("  Applied successfully")

        # Compile
        success, output, errors = compile_on_z6()

        if success:
            result.patches_compiled += 1
            compiled_files.append(patch.target_file)
            log("  COMPILED OK")

            # Commit
            if git_commit_patch(patch):
                log("  Committed to git")
        else:
            log(f"  COMPILE FAILED: {len(errors)} errors", "WARN")
            for err in errors[:3]:
                log(f"    {err}")

            # Try to fix
            fixed = False
            for attempt in range(MAX_FIX_ATTEMPTS):
                if not errors:
                    break

                fix = try_fix_error(errors[0])
                if not fix:
                    break

                log(f"  Fix attempt {attempt + 1}: {fix.description}")
                fix_success, _ = apply_patch(fix)
                if fix_success:
                    success, _, errors = compile_on_z6()
                    if success:
                        result.errors_fixed += 1
                        result.patches_compiled += 1
                        log("  Fix successful!")
                        fixed = True
                        break

            if not fixed:
                log("  Reverting patch", "WARN")
                revert_patch(patch)

    log(f"=== Cycle Complete: {result.patches_compiled}/{result.patches_attempted} successful ===")

    # Restart server if we had successful patches
    if result.patches_compiled > 0:
        # Get unique modified files
        modified_files = list(set(compiled_files))

        log(f"Restarting server to apply {result.patches_compiled} new patches...")
        restart_success, restart_msg = restart_server_on_z6()
        if restart_success:
            log("Server restarted with new code!")
            result.messages.append("Server restarted successfully")

            # Update self-graph with modified files
            if modified_files:
                update_self_graph(modified_files)
                result.messages.append(f"Updated graph with {len(modified_files)} files")
        else:
            log(f"Server restart failed: {restart_msg}", "WARN")
            result.messages.append(f"Restart failed: {restart_msg}")

    return result


def generate_dream_from_errors(errors: List[str]) -> str:
    """Ask Z.E.T.A. to generate a fix for compilation errors."""
    prompt = f"""You are Z.E.T.A., analyzing your own compilation errors.
These errors occurred while trying to compile your source code:

{chr(10).join(errors[:5])}

Generate a C++ code fix for these errors. Include the target file and exact code.
Use this format:
```cpp
// FILE: zeta-xxx.h
// Your fix code here
```

Be specific and provide complete, compilable C++ code."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": prompt, "max_tokens": 500},
            timeout=120
        )
        if resp.status_code == 200:
            return resp.json().get("output", "")
    except Exception as e:
        log(f"Error generating fix dream: {e}", "ERROR")

    return ""


def run_autonomous_loop():
    """Run the autonomous self-modification loop."""
    log("=" * 60)
    log("Z.E.T.A. AUTONOMOUS SELF-MODIFICATION")
    log("Branch: autonomous-self-recursion")
    log("=" * 60)

    # Check server health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=10).json()
        log(f"Server: {health['status']} | Nodes: {health.get('graph_nodes', 0)}")
    except Exception as e:
        log(f"Server not reachable: {e}", "ERROR")
        sys.exit(1)

    cycle_count = 0
    total_compiled = 0
    total_fixed = 0

    while True:
        cycle_count += 1
        log(f"\n{'='*40}")
        log(f"CYCLE {cycle_count}")
        log(f"{'='*40}")

        # Fetch dreams
        dreams = fetch_pending_dreams()
        log(f"Fetched {len(dreams)} pending dreams")

        if dreams:
            result = run_cycle(dreams)
            total_compiled += result.patches_compiled
            total_fixed += result.errors_fixed

            log(f"Running total: {total_compiled} patches compiled, {total_fixed} auto-fixed")

            # If we had errors, generate a dream to fix them
            if result.patches_attempted > result.patches_compiled:
                log("Generating self-fix dream for failed patches...")
                # This could trigger Z.E.T.A. to dream about its own errors
        else:
            log("No dreams to process")

        # Trigger a dream cycle on Z.E.T.A.
        try:
            requests.post(
                f"{ZETA_URL}/generate",
                json={
                    "prompt": "Dream about improvements to your own codebase. Focus on: optimization, bug fixes, new features. Generate concrete C++ code suggestions.",
                    "max_tokens": 300
                },
                timeout=60
            )
        except:
            pass

        log(f"Sleeping for {CYCLE_DELAY_SECONDS} seconds...")
        time.sleep(CYCLE_DELAY_SECONDS)


def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == "--help" or sys.argv[1] == "-h":
            print("Usage: python3 autonomous_self_modify.py [options]")
            print("")
            print("Options:")
            print("  --once      Run one cycle and exit")
            print("  --dry-run   Extract patches but don't apply")
            print("  --loop      Run continuously (default)")
            print("")
            print("This script enables Z.E.T.A.'s autonomous self-modification.")
            print("Only run in the autonomous-self-recursion branch!")
            return

        if sys.argv[1] == "--once":
            dreams = fetch_pending_dreams()
            if dreams:
                run_cycle(dreams)
            else:
                log("No dreams found")
            return

        if sys.argv[1] == "--dry-run":
            global MIN_CONFIDENCE
            MIN_CONFIDENCE = 0.0  # Accept all patches for inspection

            dreams = fetch_pending_dreams()
            log(f"Found {len(dreams)} dreams")

            for i, dream in enumerate(dreams[:5]):
                patches = extract_patches_from_dream(dream, f"dream_{i}")
                log(f"\nDream {i}: {len(patches)} patches")
                for p in patches:
                    log(f"  - {p.target_file}: {p.description} (conf: {p.confidence:.2f})")
                    log(f"    Code: {p.new_code[:100]}...")
            return

    # Default: run autonomous loop
    run_autonomous_loop()


if __name__ == "__main__":
    main()
