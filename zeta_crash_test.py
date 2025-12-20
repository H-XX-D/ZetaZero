#!/usr/bin/env python3
"""
ZETA Crash Recovery Test - Realistic Server Kill Scenarios
"""

import requests
import time
import subprocess
import random
import sys

SERVER = "192.168.0.213:9000"
BASE_URL = f"http://{SERVER}"

def ssh(cmd, timeout=10):
    try:
        result = subprocess.run(f"ssh tif@192.168.0.213 '{cmd}'", 
                              shell=True, capture_output=True, text=True, timeout=timeout)
        return result.returncode == 0, result.stdout
    except subprocess.TimeoutExpired:
        return False, ""

def start_server():
    print("[START] Launching server...")
    ssh("pkill -9 -f llama-zeta-server 2>/dev/null", timeout=5)
    time.sleep(2)
    
    # Start in background, don't wait for it
    subprocess.Popen(
        "ssh tif@192.168.0.213 'cd ~/ZetaZero/llama.cpp/build/bin && "
        "nohup ./llama-zeta-server "
        "-m ~/ZetaZero/llama.cpp/models/qwen2.5-3b-instruct-q4_k_m.gguf "
        "--model-3b ~/ZetaZero/llama.cpp/models/qwen2.5-1.5b-instruct-q4_k_m.gguf "
        "--embed-model ~/ZetaZero/llama.cpp/models/nomic-embed-text-v1.5-q4_k_m.gguf "
        "--gpu-layers 10 --ctx-14b 512 --ctx-3b 384 --port 9000 --log-disable "
        "> /tmp/zeta_crash.log 2>&1 &'",
        shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    
    for _ in range(20):
        try:
            r = requests.get(f"{BASE_URL}/health", timeout=2)
            if r.status_code == 200:
                print("[START] ✓ Server ready")
                return True
        except:
            pass
        time.sleep(1)
    
    print("[START] ✗ Failed")
    return False

def kill_server(signal="KILL"):
    print(f"[KILL] Sending SIG{signal}...")
    ssh(f"pkill -{signal} -f llama-zeta-server")
    time.sleep(2)
    ok, out = ssh("pgrep -f llama-zeta-server")
    if ok and out.strip():
        print(f"[KILL] Force killing...")
        ssh("pkill -9 -f llama-zeta-server")
        time.sleep(1)
    print("[KILL] ✓ Server terminated")

def generate(prompt):
    try:
        r = requests.post(f"{BASE_URL}/generate", 
                         json={"prompt": prompt, "max_tokens": 50},
                         timeout=15)
        if r.status_code == 200:
            data = r.json()
            return data.get("output", data.get("response", ""))
    except Exception as e:
        print(f"  [DEBUG] Generate error: {e}")
    return None

def get_graph():
    try:
        r = requests.get(f"{BASE_URL}/graph", timeout=5)
        if r.status_code == 200:
            d = r.json()
            return d.get("node_count", 0), d.get("edge_count", 0)
    except:
        pass
    return 0, 0

def test_crash_recovery():
    print("\n" + "="*70)
    print("ZETA CRASH RECOVERY TEST")
    print("="*70 + "\n")
    
    tests_passed = 0
    tests_total = 0
    
    # Test 1: SIGTERM with data
    print("\n[TEST 1] Graceful Shutdown (SIGTERM) with Data Persistence")
    print("-" * 70)
    tests_total += 1
    
    if not start_server():
        print("✗ FAIL: Could not start")
    else:
        # Store facts
        facts = []
        for i in range(3):
            fact = f"CrashTest_{i}_{random.randint(1000,9999)}"
            facts.append(fact)
            resp = generate(f"Remember: {fact}")
            if resp:
                print(f"  Stored: {fact}")
        
        nodes_before, edges_before = get_graph()
        print(f"  Graph: {nodes_before} nodes, {edges_before} edges")
        
        # Kill gracefully
        time.sleep(1)
        kill_server("TERM")
        
        # Restart
        if start_server():
            nodes_after, edges_after = get_graph()
            print(f"  Recovered: {nodes_after} nodes, {edges_after} edges")
            
            # Try to recall
            recovered = 0
            for fact in facts:
                resp = generate(f"What is {fact.split('_')[1]}?")
                if resp and fact.split('_')[2] in str(resp):
                    recovered += 1
            
            if recovered >= len(facts) * 0.6:  # 60% recovery threshold
                print(f"✓ PASS: {recovered}/{len(facts)} facts recovered")
                tests_passed += 1
            else:
                print(f"✗ FAIL: Only {recovered}/{len(facts)} recovered")
        else:
            print("✗ FAIL: Could not restart")
    
    kill_server("KILL")
    time.sleep(2)
    
    # Test 2: SIGKILL (force)
    print("\n[TEST 2] Force Kill (SIGKILL) During Operation")
    print("-" * 70)
    tests_total += 1
    
    if not start_server():
        print("✗ FAIL: Could not start")
    else:
        generate("Remember: ForceKillTest_12345")
        time.sleep(0.5)
        
        # Force kill
        kill_server("KILL")
        
        # Should recover
        if start_server():
            print("✓ PASS: Server recovered from SIGKILL")
            tests_passed += 1
        else:
            print("✗ FAIL: Could not restart after SIGKILL")
    
    kill_server("KILL")
    time.sleep(2)
    
    # Test 3: Rapid restart cycle
    print("\n[TEST 3] Rapid Kill-Restart Cycle (5x)")
    print("-" * 70)
    tests_total += 1
    
    cycle_ok = True
    for i in range(5):
        print(f"  Cycle {i+1}/5...")
        if not start_server():
            cycle_ok = False
            break
        time.sleep(1)
        kill_server(["TERM", "KILL"][i % 2])
        time.sleep(1)
    
    if cycle_ok and start_server():
        print("✓ PASS: Survived 5 rapid cycles")
        tests_passed += 1
    else:
        print("✗ FAIL: Failed during rapid cycling")
    
    kill_server("KILL")
    
    # Results
    print("\n" + "="*70)
    print(f"RESULTS: {tests_passed}/{tests_total} ({tests_passed/tests_total*100:.0f}%)")
    print("="*70 + "\n")
    
    return 0 if tests_passed == tests_total else 1

if __name__ == "__main__":
    try:
        sys.exit(test_crash_recovery())
    except KeyboardInterrupt:
        print("\n\nInterrupted")
        ssh("pkill -9 -f llama-zeta-server")
        sys.exit(1)
