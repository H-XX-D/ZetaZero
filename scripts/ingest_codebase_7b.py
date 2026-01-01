#!/usr/bin/env python3
"""
Z.E.T.A. Codebase Ingestion - 4B Embed + 7B Extraction Pipeline
Feeds codebase into GitGraph with typed nodes (function, struct, class, file)
NO 14B involvement - uses /extract_code_7b endpoint
"""

import os
import sys
import json
import requests
from pathlib import Path

ZETA_URL = "http://192.168.0.165:8080"
SOURCE_DIR = "/home/xx/ZetaZero/llama.cpp/tools/zeta-zero"

def extract_file(filepath: str) -> dict:
    """Send file to /extract_code_7b endpoint"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            code = f.read()
    except Exception as e:
        print(f"  ERROR reading {filepath}: {e}")
        return None
    
    if len(code) < 50:
        print(f"  SKIP {filepath} (too small)")
        return None
    
    filename = os.path.basename(filepath)
    
    # Use JSON body
    payload = {
        "code": code,
        "filename": filename,
        "commit": True  # Commit to graph
    }
    
    try:
        resp = requests.post(
            f"{ZETA_URL}/extract_code_7b",
            json=payload,
            timeout=60
        )
        if resp.status_code == 200:
            return resp.json()
        else:
            print(f"  ERROR {resp.status_code}: {resp.text[:100]}")
            return None
    except Exception as e:
        print(f"  ERROR: {e}")
        return None

def main():
    print("=" * 60)
    print("Z.E.T.A. Codebase Ingestion")
    print(f"Source: {SOURCE_DIR}")
    print(f"Target: {ZETA_URL}")
    print("=" * 60)
    
    # Check server health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"Server: {health['status']} | Nodes: {health['graph_nodes']} | Edges: {health['graph_edges']}")
    except Exception as e:
        print(f"ERROR: Cannot connect to server: {e}")
        sys.exit(1)
    
    # Find all source files
    extensions = ['.h', '.cpp', '.c', '.hpp', '.cu', '.cuh', '.m']
    files = []
    for ext in extensions:
        files.extend(Path(SOURCE_DIR).glob(f"*{ext}"))
    
    print(f"\nFound {len(files)} source files")
    print("-" * 60)
    
    total_entities = 0
    total_committed = 0
    
    for filepath in sorted(files):
        print(f"\nProcessing: {filepath.name}")
        result = extract_file(str(filepath))
        
        if result and result.get('status') == 'ok':
            entities = result.get('entities_found', 0)
            committed = result.get('nodes_committed', 0)
            time_ms = result.get('extraction_time_ms', 0)
            summary = result.get('summary', {})
            
            total_entities += entities
            total_committed += committed
            
            print(f"  Found: {entities} entities ({summary.get('functions', 0)} funcs, "
                  f"{summary.get('structs', 0)} structs, {summary.get('classes', 0)} classes)")
            print(f"  Committed: {committed} nodes in {time_ms}ms")
        else:
            print(f"  FAILED")
    
    # Final stats
    print("\n" + "=" * 60)
    print("INGESTION COMPLETE")
    print("=" * 60)
    
    # Check final graph state
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"Final Graph: {health['graph_nodes']} nodes, {health['graph_edges']} edges")
    except:
        pass
    
    print(f"Total Entities Extracted: {total_entities}")
    print(f"Total Nodes Committed: {total_committed}")

if __name__ == "__main__":
    main()
