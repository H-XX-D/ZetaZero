#!/usr/bin/env python3
"""
HoloGit Entity Graph Manager
- Extracts entities from facts
- Builds semantic connections
- Supports intuition-based pre-loading
"""

import json
import os
import sys
import re
from pathlib import Path
from collections import defaultdict

HOLOGIT_DIR = Path("/mnt/HoloGit")
GRAPH_FILE = HOLOGIT_DIR / "index" / "entity_graph.json"
BLOCKS_DIR = HOLOGIT_DIR / "blocks"

def load_graph():
    if GRAPH_FILE.exists():
        with open(GRAPH_FILE) as f:
            return json.load(f)
    return {"entities": {}, "block_to_entities": {}}

def save_graph(graph):
    with open(GRAPH_FILE, "w") as f:
        json.dump(graph, f, indent=2)

def extract_entities(text):
    """Extract entity-like strings from text"""
    entities = set()
    # Names (capitalized words)
    for m in re.finditer(r'\b([A-Z][a-z]+)\b', text):
        entities.add(m.group(1).lower())
    # Numbers/codes
    for m in re.finditer(r'\b([A-Z]{2,}(?:-[A-Z0-9]+)+)\b', text):
        entities.add(m.group(1).lower())
    # Key-value entities
    for m in re.finditer(r'([\w_]+)=([\w-]+)', text):
        entities.add(m.group(2).lower())
    return list(entities)

def add_block_to_graph(block_id, facts_text):
    """Add a block's entities to the graph"""
    graph = load_graph()
    entities = extract_entities(facts_text)
    
    # Add block to entity map
    block_key = str(block_id)
    graph["block_to_entities"][block_key] = entities
    
    # Add entities
    for e in entities:
        if e not in graph["entities"]:
            graph["entities"][e] = {"blocks": [], "connections": []}
        if block_id not in graph["entities"][e]["blocks"]:
            graph["entities"][e]["blocks"].append(block_id)
        # Connect to other entities in same block
        for other in entities:
            if other != e and other not in graph["entities"][e]["connections"]:
                graph["entities"][e]["connections"].append(other)
    
    save_graph(graph)
    return entities

def get_related_blocks(query, depth=2):
    """Find blocks related to entities in query"""
    graph = load_graph()
    entities = extract_entities(query)
    
    related_blocks = set()
    visited = set()
    
    def walk(entity, d):
        if d == 0 or entity in visited:
            return
        visited.add(entity)
        if entity in graph["entities"]:
            for b in graph["entities"][entity]["blocks"]:
                related_blocks.add(b)
            if d > 1:
                for conn in graph["entities"][entity]["connections"]:
                    walk(conn, d-1)
    
    for e in entities:
        walk(e.lower(), depth)
    
    return list(related_blocks)

def rebuild_graph():
    """Rebuild graph from all facts files"""
    graph = {"entities": {}, "block_to_entities": {}}
    save_graph(graph)
    
    for f in BLOCKS_DIR.glob("facts_*.txt"):
        block_id = int(f.stem.split("_")[1])
        with open(f) as fp:
            add_block_to_graph(block_id, fp.read())
    
    return load_graph()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: hologit_graph.py <command> [args]")
        print("Commands:")
        print("  query <text>     - Find related blocks")
        print("  add <block_id>   - Add block to graph")
        print("  rebuild          - Rebuild entire graph")
        sys.exit(1)
    
    cmd = sys.argv[1]
    
    if cmd == "query":
        text = " ".join(sys.argv[2:])
        blocks = get_related_blocks(text)
        print(json.dumps({"query": text, "related_blocks": blocks}))
    
    elif cmd == "add":
        block_id = int(sys.argv[2])
        facts_file = BLOCKS_DIR / f"facts_{block_id}.txt"
        if facts_file.exists():
            with open(facts_file) as f:
                entities = add_block_to_graph(block_id, f.read())
            print(json.dumps({"block_id": block_id, "entities": entities}))
        else:
            print(f"No facts file for block {block_id}")
    
    elif cmd == "rebuild":
        graph = rebuild_graph()
        print(json.dumps({"entities": len(graph["entities"]), "blocks": len(graph["block_to_entities"])}))
