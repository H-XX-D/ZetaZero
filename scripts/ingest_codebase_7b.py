#!/usr/bin/env python3
"""
Z.E.T.A. Codebase Ingestion - 4B Embed + 7B Extraction Pipeline
Feeds codebase into GitGraph with typed nodes (function, struct, class, file)
NO 14B involvement - uses /extract_code_7b endpoint
"""

import argparse
import os
import sys

import requests
from pathlib import Path

DEFAULT_ZETA_URL = "http://192.168.0.165:8080"
DEFAULT_SOURCE_DIR = "./llama.cpp/tools/zeta-zero"

def extract_file(filepath: str, zeta_url: str) -> dict | None:
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
    payload: dict[str, object] = {
        "code": code,
        "filename": filename,
        "commit": True  # Commit to graph
    }
    
    try:
        resp = requests.post(
            f"{zeta_url}/extract_code_7b",
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
    ap = argparse.ArgumentParser(description="Z.E.T.A. codebase ingestion via /extract_code_7b")
    ap.add_argument("--zeta-url", default=DEFAULT_ZETA_URL, help="Base URL of the Z.E.T.A. server")
    ap.add_argument("--source-dir", default=DEFAULT_SOURCE_DIR, help="Directory to ingest")
    args = ap.parse_args()

    zeta_url = args.zeta_url.rstrip("/")
    source_dir = args.source_dir

    print("=" * 60)
    print("Z.E.T.A. Codebase Ingestion")
    print(f"Source: {source_dir}")
    print(f"Target: {zeta_url}")
    print("=" * 60)
    
    # Check server health
    try:
        health = requests.get(f"{zeta_url}/health", timeout=5).json()
        print(f"Server: {health['status']} | Nodes: {health['graph_nodes']} | Edges: {health['graph_edges']}")
    except Exception as e:
        print(f"ERROR: Cannot connect to server: {e}")
        sys.exit(1)
    
    # Find all source files
    extensions = ['.h', '.cpp', '.c', '.hpp', '.cu', '.cuh', '.m']
    files = []
    for ext in extensions:
        files.extend(Path(source_dir).glob(f"*{ext}"))
    
    print(f"\nFound {len(files)} source files")
    print("-" * 60)
    
    total_entities = 0
    total_committed = 0
    
    for filepath in sorted(files):
        print(f"\nProcessing: {filepath.name}")
        result = extract_file(str(filepath), zeta_url)
        
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
        health = requests.get(f"{zeta_url}/health", timeout=5).json()
        print(f"Final Graph: {health['graph_nodes']} nodes, {health['graph_edges']} edges")
    except:
        pass
    
    print(f"Total Entities Extracted: {total_entities}")
    print(f"Total Nodes Committed: {total_committed}")

if __name__ == "__main__":
    main()
