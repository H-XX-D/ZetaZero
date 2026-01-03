#!/usr/bin/env python3
"""
GKV Diagnostic Test
Proves (or disproves) that GKV injection is working by:
1. Making a simple query
2. Extracting active_node_ids from response
3. Checking if those node_ids have .lkv files
4. Showing why kv_injected might be false
"""

import requests
import json
import subprocess
import sys

SERVER = "http://192.168.0.165:9000"
REMOTE_HOST = "xx@192.168.0.165"
GKV_DIR = "/mnt/GitGraph/graph_kv"

def ssh_command(cmd):
    """Run command on remote host"""
    result = subprocess.run(
        ["ssh", REMOTE_HOST, cmd],
        capture_output=True, text=True, timeout=10
    )
    return result.stdout.strip()

def get_lkv_files():
    """Get list of node_ids that have .lkv files"""
    output = ssh_command(f"ls {GKV_DIR}/*.lkv 2>/dev/null | xargs -n1 basename | sed 's/.lkv//'")
    if output:
        return set(int(x) for x in output.split('\n') if x.isdigit())
    return set()

def query_server(prompt):
    """Send query and get streaming metadata"""
    try:
        response = requests.post(
            f"{SERVER}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 50,
                "temperature": 0.3
            },
            timeout=30
        )
        return response.json()
    except Exception as e:
        return {"error": str(e)}

def main():
    print("=" * 60)
    print("Z.E.T.A. GKV DIAGNOSTIC")
    print("=" * 60)
    
    # Step 1: Check server health
    print("\n[1] Server Health Check...")
    try:
        r = requests.get(f"{SERVER}/health", timeout=5)
        health = r.json()
        print(f"    Status: {health.get('status', 'unknown')}")
        print(f"    GKV Enabled: {health.get('gkv_enabled', False)}")
    except Exception as e:
        print(f"    ERROR: {e}")
        sys.exit(1)
    
    # Step 2: List .lkv files
    print("\n[2] Existing .lkv Files...")
    lkv_nodes = get_lkv_files()
    print(f"    Count: {len(lkv_nodes)} files")
    if lkv_nodes:
        sample = sorted(lkv_nodes)[:10]
        print(f"    Sample node_ids: {sample}")
    
    # Step 3: Query with neutral prompt (no identity contamination)
    print("\n[3] Test Query (neutral topic)...")
    test_prompt = "Explain the concept of recursion in programming."
    print(f"    Prompt: {test_prompt}")
    
    result = query_server(test_prompt)
    
    if "error" in result:
        print(f"    ERROR: {result['error']}")
        sys.exit(1)
    
    # Step 4: Extract streaming metadata
    print("\n[4] Response Metadata...")
    print(f"    Response preview: {result.get('text', '')[:100]}...")
    
    stream = result.get("streaming", {})
    if not stream:
        print("    WARNING: No streaming metadata in response!")
        print(f"    Full response keys: {result.keys()}")
    else:
        active_nodes = stream.get("active_node_ids", [])
        gkv_injected = stream.get("gkv_injected", False)
        gkv_tokens = stream.get("gkv_tokens_injected", 0)
        
        print(f"    Active Nodes: {active_nodes}")
        print(f"    GKV Injected: {gkv_injected}")
        print(f"    GKV Tokens: {gkv_tokens}")
        
        # Step 5: Cross-reference active nodes with .lkv files
        print("\n[5] GKV Cross-Reference...")
        if active_nodes:
            for node_id in active_nodes:
                has_lkv = node_id in lkv_nodes
                status = "✅ HAS .lkv" if has_lkv else "❌ NO .lkv"
                print(f"    Node {node_id}: {status}")
            
            matching = set(active_nodes) & lkv_nodes
            if matching:
                print(f"\n    MATCHING NODES: {matching}")
                print("    → These should have been injected!")
            else:
                print(f"\n    NO OVERLAP between active nodes and .lkv files")
                print("    → This explains why kv_injected is false")
        else:
            print("    No active nodes reported")
    
    # Step 6: Check server logs for GKV activity
    print("\n[6] Recent GKV Server Logs...")
    logs = ssh_command("tail -20 ~/ZetaZero/zeta_server.log 2>/dev/null | grep -E 'GKV|cache|inject' || echo 'No GKV logs found'")
    print(f"    {logs}")
    
    print("\n" + "=" * 60)
    print("DIAGNOSTIC COMPLETE")
    print("=" * 60)

if __name__ == "__main__":
    main()
