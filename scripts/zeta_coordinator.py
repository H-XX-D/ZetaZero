#!/usr/bin/env python3
"""ZETA Coordinator - Parallel 3B extraction + Hard Injection"""

import asyncio
import aiohttp
import json
import sys
import time
from pathlib import Path

ZETA_14B = "http://localhost:9000"
LLAMA_3B = "http://localhost:9001"
HOLOGIT = Path("/mnt/HoloGit")

# Import versioning system
sys.path.insert(0, str(HOLOGIT / "versions"))
try:
    from memory_graph import MemoryVersion, FactCorrelator
    HAS_VERSIONING = True
except:
    HAS_VERSIONING = False

EXTRACT_PROMPT = (
    "<|im_start|>system\n"
    "Extract entity names as JSON array.\n"
    "<|im_end|>\n"
    "<|im_start|>user\n"
    "{query}\n"
    "<|im_end|>\n"
    "<|im_start|>assistant\n"
    '["'
)

# Prompt for extracting facts from conversation
# Note: {{ and }} escape braces in f-string/format
FACT_EXTRACT_PROMPT = (
    "<|im_start|>system\n"
    "Extract facts as JSON array. Format:\n"
    '[{{"type": "name", "key": "user_name", "value": "Alice"}}]\n'
    "Types: name, preference, code, place, number. Only explicit facts.\n"
    "<|im_end|>\n"
    "<|im_start|>user\n"
    "{user_msg}\n"
    "Response: {assistant_msg}\n"
    "<|im_end|>\n"
    "<|im_start|>assistant\n"
    "[{{"
)

async def extract_entities_3b(session, query):
    start = time.time()
    try:
        async with session.post(
            f"{LLAMA_3B}/completion",
            json={
                "prompt": EXTRACT_PROMPT.format(query=query),
                "n_predict": 50,
                "temperature": 0.1,
                "stop": ["]"]
            },
            timeout=aiohttp.ClientTimeout(total=5)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                raw = data.get("content", "")
                content = '["' + raw + ']'
                try:
                    entities = json.loads(content)
                    ms = (time.time() - start) * 1000
                    print(f"[3B] {ms:.0f}ms: {entities}", file=sys.stderr)
                    return entities
                except:
                    pass
    except Exception as e:
        print(f"[3B] Error: {e}", file=sys.stderr)
    return []

async def extract_and_save_facts(session, user_msg, assistant_msg):
    """3B extracts facts from conversation and saves to HoloGit"""
    start = time.time()
    try:
        async with session.post(
            f"{LLAMA_3B}/completion",
            json={
                "prompt": FACT_EXTRACT_PROMPT.format(user_msg=user_msg, assistant_msg=assistant_msg),
                "n_predict": 200,
                "temperature": 0.1,
                "stop": ["]}", "}\n"]
            },
            timeout=aiohttp.ClientTimeout(total=10)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                raw = data.get("content", "").strip()
                # Response starts after "[{" so we add that back
                content = "[{" + raw
                # Ensure proper JSON closure
                if not content.endswith("]"):
                    if content.endswith("}"):
                        content += "]"
                    else:
                        content += "}]"

                try:
                    facts = json.loads(content)

                    if facts:
                        ms = (time.time() - start) * 1000
                        print(f"[3B-SAVE] {ms:.0f}ms: extracted {len(facts)} facts", file=sys.stderr)

                        # Save facts to HoloGit
                        saved = save_facts_to_hologit(facts)
                        if saved:
                            print(f"[HOLOGIT] Saved to block {saved}", file=sys.stderr)
                        return facts
                except Exception as e:
                    print(f"[3B-SAVE] Parse error: {e}", file=sys.stderr)
    except Exception as e:
        print(f"[3B-SAVE] Error: {e}", file=sys.stderr)
    return []

def get_existing_facts():
    """Load all existing facts as key->value dict"""
    existing = {}
    blocks_dir = HOLOGIT / "blocks"
    for facts_file in blocks_dir.glob("facts_*.txt"):
        with open(facts_file) as f:
            for line in f:
                if "=" in line:
                    parts = line.strip().split("=", 1)
                    if len(parts) == 2:
                        key = parts[0].split(":")[-1]
                        existing[key] = parts[1]
    return existing

def check_conflicts(new_facts):
    """Check if new facts conflict with existing ones"""
    existing = get_existing_facts()
    conflicts = []

    for fact in new_facts:
        key = fact.get("key", "")
        new_val = fact.get("value", "")

        if key in existing and existing[key].lower() != new_val.lower():
            conflicts.append({
                "key": key,
                "old_value": existing[key],
                "new_value": new_val
            })

    return conflicts

def save_facts_to_hologit(facts):
    """Save extracted facts to HoloGit with versioning"""
    if not facts:
        return None

    # Check for conflicts
    conflicts = check_conflicts(facts)
    branch_created = None

    if conflicts and HAS_VERSIONING:
        mv = MemoryVersion()
        # Create branch for conflicting info
        branch_name = f"conflict-{int(time.time())}"
        if mv.branch(branch_name):
            mv.checkout(branch_name)
            branch_created = branch_name
            print(f"[BRANCH] Created {branch_name} for conflicts: {conflicts}", file=sys.stderr)

    # Get next block ID
    blocks_dir = HOLOGIT / "blocks"
    existing = list(blocks_dir.glob("facts_*.txt"))
    next_id = max([int(f.stem.split("_")[1]) for f in existing], default=-1) + 1

    # Format facts
    lines = []
    for f in facts:
        t = f.get("type", "info")
        k = f.get("key", "unknown")
        v = f.get("value", "")
        if v:
            lines.append(f"{t}:{k}={v}")

    if not lines:
        return None

    # Write facts file
    facts_file = blocks_dir / f"facts_{next_id}.txt"
    with open(facts_file, "w") as fp:
        fp.write("\n".join(lines))

    # Update entity graph
    update_entity_graph(next_id, facts)

    # Commit and update correlations
    if HAS_VERSIONING:
        mv = MemoryVersion()
        fc = FactCorrelator()

        # Auto-commit
        entities = [f.get("value", "") for f in facts if f.get("value")]
        commit_msg = f"Add facts: {', '.join(entities[:3])}"
        commit_id = mv.commit(facts, commit_msg)
        print(f"[COMMIT] {commit_id[:8]}", file=sys.stderr)

        # Update correlations
        clusters = fc.find_clusters()
        if clusters:
            print(f"[CORRELATE] {len(clusters)} clusters", file=sys.stderr)

        # If on conflict branch, try auto-merge if confidence high
        if branch_created:
            mv.checkout("main")  # Switch back to main

    return next_id

def update_entity_graph(block_id, facts):
    """Update entity graph with new facts"""
    graph_file = HOLOGIT / "index" / "entity_graph.json"

    if graph_file.exists():
        with open(graph_file) as f:
            graph = json.load(f)
    else:
        graph = {"entities": {}, "block_to_entities": {}}

    # Extract entities from facts
    entities = []
    for f in facts:
        val = f.get("value", "").lower()
        key = f.get("key", "").lower()
        if val:
            entities.append(val)
        if key:
            entities.append(key)

    # Add to graph
    block_key = str(block_id)
    graph["block_to_entities"][block_key] = entities

    for e in entities:
        if e not in graph["entities"]:
            graph["entities"][e] = {"blocks": [], "connections": []}
        if block_id not in graph["entities"][e]["blocks"]:
            graph["entities"][e]["blocks"].append(block_id)
        # Connect to other entities in same block
        for other in entities:
            if other != e and other not in graph["entities"][e]["connections"]:
                graph["entities"][e]["connections"].append(other)

    with open(graph_file, "w") as f:
        json.dump(graph, f, indent=2)

def get_facts_for_entities(entities):
    graph_file = HOLOGIT / "index" / "entity_graph.json"
    if not graph_file.exists():
        return []

    with open(graph_file) as f:
        graph = json.load(f)

    blocks = set()
    for e in entities:
        e_lower = e.lower()
        if e_lower in graph.get("entities", {}):
            for b in graph["entities"][e_lower].get("blocks", []):
                blocks.add(b)

    facts = []
    for bid in blocks:
        facts_file = HOLOGIT / "blocks" / f"facts_{bid}.txt"
        if facts_file.exists():
            with open(facts_file) as f:
                for line in f:
                    if ":" in line and "=" in line:
                        parts = line.strip().split("=", 1)
                        if len(parts) == 2:
                            key = parts[0].split(":")[-1]
                            val = parts[1]
                            facts.append(f"{key}: {val}")
    return facts

def build_injected_prompt(query, facts):
    if not facts:
        return query

    memory_context = "\n".join(f"- {f}" for f in facts)
    return (
        "<|im_start|>system\n"
        "You remember these facts:\n"
        f"{memory_context}\n"
        "Use them to answer if relevant.\n"
        "<|im_end|>\n"
        "<|im_start|>user\n"
        f"{query}\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n"
    )

async def generate_14b(session, prompt, max_tokens):
    start = time.time()
    try:
        async with session.post(
            f"{ZETA_14B}/generate",
            data={"prompt": prompt, "max_tokens": str(max_tokens)},
            timeout=aiohttp.ClientTimeout(total=60)
        ) as resp:
            if resp.status == 200:
                data = await resp.json()
                ms = (time.time() - start) * 1000
                print(f"[14B] {ms:.0f}ms", file=sys.stderr)
                return data
    except Exception as e:
        print(f"[14B] Error: {e}", file=sys.stderr)
    return {"error": "generation failed"}

async def process_query(query, max_tokens=100):
    start = time.time()

    async with aiohttp.ClientSession() as session:
        # 3B extracts entities (fast ~10ms)
        entities = await extract_entities_3b(session, query)

        # Get facts for entities from graph
        facts = get_facts_for_entities(entities) if entities else []

        if facts:
            print(f"[INJECT] Facts: {facts}", file=sys.stderr)

        # Build prompt with injected facts
        full_prompt = build_injected_prompt(query, facts)

        # Generate with 14B
        result = await generate_14b(session, full_prompt, max_tokens)

        response_text = ""
        if isinstance(result, dict):
            response_text = result.get("response", "")
            result["memory"] = {"entities": entities, "facts_recalled": facts}

        # REAL-TIME EXTRACTION: 3B extracts new facts from this exchange
        # Run in background - don't block response
        if response_text and not response_text.startswith("I'm sorry"):
            new_facts = await extract_and_save_facts(session, query, response_text)
            if isinstance(result, dict):
                result["memory"]["facts_saved"] = new_facts

        total_ms = (time.time() - start) * 1000
        print(f"[TOTAL] {total_ms:.0f}ms", file=sys.stderr)

        return result

async def main():
    if len(sys.argv) < 2:
        print("Usage: zeta_coordinator.py <query> [max_tokens]")
        sys.exit(1)

    query = sys.argv[1]
    max_tokens = int(sys.argv[2]) if len(sys.argv) > 2 else 100

    result = await process_query(query, max_tokens)
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    asyncio.run(main())
