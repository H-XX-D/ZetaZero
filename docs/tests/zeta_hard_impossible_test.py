#!/usr/bin/env python3
"""
ZETA Hard & Impossible Tests
Tests the 3B+3B+81MB configuration with challenging and impossible scenarios
"""

import requests
import time
import json
from datetime import datetime

SERVER = "http://192.168.0.213:9000"

def print_header(title, symbol="="):
    print(f"\n{symbol * 70}")
    print(f" {title}")
    print(f"{symbol * 70}\n")

def generate(prompt, max_tokens=600, endpoint="/generate"):
    """Send generation request"""
    try:
        response = requests.post(
            f"{SERVER}{endpoint}",
            json={"prompt": prompt, "max_tokens": max_tokens},
            timeout=120
        )
        if response.status_code == 200:
            data = response.json()
            return {
                "output": data.get("output", ""),
                "tokens": data.get("tokens", 0),
                "graph_nodes": data.get("graph_nodes", 0),
                "graph_edges": data.get("graph_edges", 0),
                "time": data.get("generation_time", 0)
            }
    except Exception as e:
        return {"error": str(e)}
    return {"error": "Request failed"}

def check_health():
    """Check server health"""
    try:
        r = requests.get(f"{SERVER}/health", timeout=5)
        return r.status_code == 200
    except:
        return False

def check_memory():
    """Get memory stats from system"""
    import subprocess
    try:
        result = subprocess.run(
            ["ssh", "tif@192.168.0.213", "free -h | grep Mem | awk '{print $3, $7}'"],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            used, avail = result.stdout.strip().split()
            return f"Used: {used}, Available: {avail}"
    except:
        pass
    return "Unknown"

# =============================================================================
# HARD TEST SUITE
# =============================================================================

def hard_test_1_multi_step_reasoning():
    """Hard: Multi-step reasoning with context retention"""
    print_header("HARD TEST 1: Multi-Step Reasoning Chain", "-")
    
    prompts = [
        "Design a distributed hash table algorithm. Explain the key components.",
        "Now implement consistent hashing for that DHT in Python pseudocode.",
        "Add fault tolerance with replication to your implementation.",
        "Analyze the time complexity of lookups in your final design."
    ]
    
    start = time.time()
    results = []
    
    for i, prompt in enumerate(prompts, 1):
        print(f"\nStep {i}/4: {prompt[:60]}...")
        result = generate(prompt, max_tokens=500)
        results.append(result)
        
        if "error" in result:
            print(f"   ❌ FAILED: {result['error']}")
            return False
        
        print(f"   ✓ Generated {result['tokens']} tokens")
        print(f"   Graph: {result['graph_nodes']} nodes, {result['graph_edges']} edges")
        time.sleep(1)
    
    duration = time.time() - start
    total_tokens = sum(r.get("tokens", 0) for r in results)
    
    print(f"\n📊 Result: {len(results)}/4 steps completed")
    print(f"   Total tokens: {total_tokens}")
    print(f"   Duration: {duration:.1f}s")
    print(f"   Memory: {check_memory()}")
    
    return len(results) == 4

def hard_test_2_rapid_context_switch():
    """Hard: Rapid switching between instruct and coder models"""
    print_header("HARD TEST 2: Rapid Context Switching", "-")
    
    tasks = [
        ("instruct", "Explain how TCP congestion control works in 2 sentences."),
        ("code", "Write a Python function to implement exponential backoff retry logic."),
        ("instruct", "What are the trade-offs between microservices and monoliths?"),
        ("code", "Implement a simple LRU cache in Python with get/put methods."),
        ("instruct", "Describe the CAP theorem and give an example."),
        ("code", "Write a binary search function with edge case handling."),
    ]
    
    start = time.time()
    successes = 0
    
    for i, (model, prompt) in enumerate(tasks, 1):
        print(f"\n[{model.upper()}] Task {i}/6: {prompt[:50]}...")
        result = generate(prompt, max_tokens=400)
        
        if "error" not in result:
            print(f"   ✓ {result['tokens']} tokens in {result.get('time', 0):.2f}s")
            successes += 1
        else:
            print(f"   ❌ {result['error']}")
        
        time.sleep(0.5)  # Minimal delay
    
    duration = time.time() - start
    
    print(f"\n📊 Result: {successes}/6 tasks completed")
    print(f"   Duration: {duration:.1f}s ({duration/6:.1f}s avg)")
    print(f"   Memory: {check_memory()}")
    
    return successes >= 5  # Allow 1 failure

def hard_test_3_streaming_budget_stress():
    """Hard: Push to 900 token code mode limit"""
    print_header("HARD TEST 3: Streaming Budget Stress (900 tokens)", "-")
    
    prompt = """Implement a complete AVL tree in Python with:
1. Insert operation with rotation (LL, LR, RR, RL)
2. Delete operation maintaining balance
3. Search operation
4. Height calculation and balance factor
5. In-order, pre-order, post-order traversal methods
6. Full docstrings and type hints
Include complete implementation with all helper methods."""
    
    print("Requesting 850 tokens (approaching 900 token code mode limit)...")
    start = time.time()
    result = generate(prompt, max_tokens=850)
    duration = time.time() - start
    
    if "error" in result:
        print(f"\n❌ FAILED: {result['error']}")
        return False
    
    print(f"\n✓ Generated {result['tokens']} tokens in {duration:.1f}s")
    print(f"  Graph: {result['graph_nodes']} nodes, {result['graph_edges']} edges")
    print(f"  Memory: {check_memory()}")
    print(f"  Output length: {len(result['output'])} chars")
    
    return result['tokens'] >= 700  # Success if got at least 700 tokens

def hard_test_4_concurrent_load():
    """Hard: Simulated concurrent requests (sequential with minimal delay)"""
    print_header("HARD TEST 4: Rapid Sequential Load", "-")
    
    prompts = [
        "What is event-driven architecture?",
        "Write a fibonacci function in Python.",
        "Explain REST API best practices.",
        "Implement quicksort in Python.",
        "What is eventual consistency?",
        "Write a decorator for retry logic.",
        "Explain database indexing.",
        "Implement a stack using lists.",
    ]
    
    print(f"Firing {len(prompts)} requests with 0.1s delay...")
    start = time.time()
    results = []
    
    for i, prompt in enumerate(prompts, 1):
        print(f"  Request {i}/{len(prompts)}...", end=" ")
        result = generate(prompt, max_tokens=300)
        results.append(result)
        
        if "error" not in result:
            print(f"✓ {result['tokens']} tokens")
        else:
            print(f"❌ {result['error']}")
        
        time.sleep(0.1)
    
    duration = time.time() - start
    successes = sum(1 for r in results if "error" not in r)
    total_tokens = sum(r.get("tokens", 0) for r in results)
    
    print(f"\n📊 Result: {successes}/{len(prompts)} succeeded")
    print(f"   Total tokens: {total_tokens}")
    print(f"   Duration: {duration:.1f}s ({duration/len(prompts):.2f}s avg)")
    print(f"   Memory: {check_memory()}")
    
    return successes >= 7  # Allow 1 failure

# =============================================================================
# IMPOSSIBLE TEST SUITE
# =============================================================================

def impossible_test_1_memory_bomb():
    """Impossible: Request far exceeds available memory"""
    print_header("IMPOSSIBLE TEST 1: Memory Bomb (5000 tokens)", "!")
    
    prompt = """Write a complete operating system kernel in C with:
1. Process scheduling (round-robin, priority, real-time)
2. Memory management (paging, segmentation, virtual memory)
3. File system implementation (inode-based)
4. Device drivers (keyboard, mouse, disk, network)
5. Inter-process communication (pipes, shared memory, semaphores)
6. System call interface with full implementation
7. Interrupt handling and context switching
8. Complete bootloader code
Include ALL implementation details, full code for every component."""
    
    print("⚠️  Requesting 5000 tokens (will exceed KV cache and memory limits)...")
    print("Expected outcome: OOM kill or truncated output")
    
    start = time.time()
    result = generate(prompt, max_tokens=5000)
    duration = time.time() - start
    
    # Check if server survived
    time.sleep(2)
    alive = check_health()
    
    if "error" in result:
        print(f"\n💥 Request failed: {result['error']}")
    else:
        print(f"\n⚠️  Generated {result['tokens']} tokens in {duration:.1f}s")
        print(f"   (Likely truncated due to memory constraints)")
    
    print(f"\n🏥 Server alive: {'YES' if alive else 'NO (OOM killed)'}")
    print(f"   Memory: {check_memory()}")
    
    return not alive  # "Success" if we killed it

def impossible_test_2_context_explosion():
    """Impossible: Build context that exceeds graph memory capacity"""
    print_header("IMPOSSIBLE TEST 2: Context Explosion", "!")
    
    print("Building massive context chain (50+ steps)...")
    base_context = "Remember these numbers: "
    
    start = time.time()
    successes = 0
    
    for i in range(1, 51):
        numbers = ", ".join(str(i*10 + j) for j in range(10))
        prompt = f"{base_context}{numbers}. Now recall all numbers from step 1 to {i}."
        
        print(f"  Step {i}/50: Adding 10 more numbers...", end=" ")
        result = generate(prompt, max_tokens=200)
        
        if "error" not in result:
            print(f"✓")
            successes += 1
        else:
            print(f"❌ {result['error']}")
            break
        
        if i % 10 == 0:
            print(f"    [{check_memory()}]")
        
        time.sleep(0.2)
    
    duration = time.time() - start
    
    print(f"\n📊 Result: Survived {successes}/50 steps")
    print(f"   Expected failure: Graph memory overflow or context loss")
    print(f"   Duration: {duration:.1f}s")
    print(f"   Memory: {check_memory()}")
    
    return successes < 50  # "Success" if it failed before 50

def impossible_test_3_parallel_overload():
    """Impossible: Simulate parallel requests to cause OOM"""
    print_header("IMPOSSIBLE TEST 3: Parallel Overload Simulation", "!")
    
    print("⚠️  Rapid-fire 20 large requests with no delay (simulated parallel)...")
    print("Expected outcome: OOM kill or server hang")
    
    prompt = """Design and implement a complete distributed database system with:
- ACID transactions across nodes
- Raft consensus protocol implementation
- Query optimizer and execution engine
- B+ tree index structures
- Write-ahead logging
- Network protocol and RPC layer
Provide complete implementation in Java."""
    
    start = time.time()
    results = []
    
    for i in range(1, 21):
        print(f"  Burst {i}/20...", end=" ")
        result = generate(prompt, max_tokens=800)
        results.append(result)
        
        if "error" not in result:
            print(f"✓ {result['tokens']}")
        else:
            print(f"❌ {result['error']}")
        
        # NO DELAY - fire immediately
    
    duration = time.time() - start
    
    # Check survival
    time.sleep(3)
    alive = check_health()
    
    successes = sum(1 for r in results if "error" not in r)
    
    print(f"\n📊 Result: {successes}/20 completed")
    print(f"   Duration: {duration:.1f}s ({duration/20:.2f}s avg)")
    print(f"   Server alive: {'YES' if alive else 'NO (OOM killed)'}")
    print(f"   Memory: {check_memory()}")
    
    return not alive or successes < 15  # "Success" if crashed or many failed

def impossible_test_4_infinite_recursion():
    """Impossible: Request that causes infinite expansion"""
    print_header("IMPOSSIBLE TEST 4: Recursive Expansion Attack", "!")
    
    prompt = """For each concept I mention, provide 5 detailed examples, and for each example, provide 5 sub-examples with explanations:

1. Design patterns in software engineering
2. Data structures and algorithms
3. Network protocols and architectures
4. Database optimization techniques
5. Concurrency control mechanisms

For EACH item above, expand fully with all 5 examples and 5 sub-examples each."""
    
    print("⚠️  Requesting recursive expansion (125+ sub-items)...")
    print("Expected outcome: Token limit hit or memory exhaustion")
    
    start = time.time()
    result = generate(prompt, max_tokens=3000)
    duration = time.time() - start
    
    time.sleep(2)
    alive = check_health()
    
    if "error" in result:
        print(f"\n💥 Request failed: {result['error']}")
    else:
        print(f"\n⚠️  Generated {result['tokens']}/3000 tokens in {duration:.1f}s")
        # Check if it's actually complete or truncated
        if result['tokens'] < 2500:
            print(f"   ✓ System limited output (safeguard worked)")
        else:
            print(f"   ⚠️  Got close to limit (high memory pressure)")
    
    print(f"\n🏥 Server alive: {'YES' if alive else 'NO'}")
    print(f"   Memory: {check_memory()}")
    
    return result.get('tokens', 0) < 2500  # "Success" if it self-limited

# =============================================================================
# MAIN TEST RUNNER
# =============================================================================

def main():
    print("\n" + "="*70)
    print(" ZETA HARD & IMPOSSIBLE TEST SUITE")
    print(" Config: 3B instruct + 3B coder + 81MB nomic embed")
    print(" Server: " + SERVER)
    print("="*70)
    
    # Check server
    if not check_health():
        print("\n❌ Server not responding!")
        return
    
    print(f"\n✓ Server online")
    print(f"  Memory: {check_memory()}")
    
    # Run tests
    tests_run = []
    
    # HARD TESTS
    print_header("HARD TESTS (Should Pass)", "=")
    
    tests_run.append(("Hard: Multi-Step Reasoning", hard_test_1_multi_step_reasoning()))
    tests_run.append(("Hard: Rapid Context Switch", hard_test_2_rapid_context_switch()))
    tests_run.append(("Hard: Streaming Budget Stress", hard_test_3_streaming_budget_stress()))
    tests_run.append(("Hard: Rapid Sequential Load", hard_test_4_concurrent_load()))
    
    # Check server health before impossible tests
    time.sleep(5)
    if not check_health():
        print("\n⚠️  Server crashed during hard tests!")
        print_results(tests_run)
        return
    
    # IMPOSSIBLE TESTS
    print_header("IMPOSSIBLE TESTS (Should Fail Gracefully)", "=")
    print("⚠️  WARNING: These tests are designed to break the system")
    print("⚠️  Expected outcomes: OOM kills, truncation, errors\n")
    
    input("Press Enter to proceed with impossible tests...")
    
    tests_run.append(("Impossible: Memory Bomb", impossible_test_1_memory_bomb()))
    
    # Check if server survived
    time.sleep(3)
    if not check_health():
        print("\n💥 Server killed by memory bomb - restarting needed")
        print_results(tests_run)
        return
    
    tests_run.append(("Impossible: Context Explosion", impossible_test_2_context_explosion()))
    
    time.sleep(3)
    if not check_health():
        print("\n💥 Server killed by context explosion")
        print_results(tests_run)
        return
    
    tests_run.append(("Impossible: Parallel Overload", impossible_test_3_parallel_overload()))
    
    time.sleep(3)
    if not check_health():
        print("\n💥 Server killed by parallel overload")
        print_results(tests_run)
        return
    
    tests_run.append(("Impossible: Recursive Expansion", impossible_test_4_infinite_recursion()))
    
    # Final results
    print_results(tests_run)

def print_results(tests_run):
    print_header("FINAL RESULTS", "=")
    
    hard_tests = [t for t in tests_run if t[0].startswith("Hard")]
    impossible_tests = [t for t in tests_run if t[0].startswith("Impossible")]
    
    print("HARD TESTS (should pass):")
    for name, result in hard_tests:
        symbol = "✓" if result else "❌"
        print(f"  {symbol} {name}")
    
    hard_pass = sum(1 for _, r in hard_tests if r)
    print(f"\n  Score: {hard_pass}/{len(hard_tests)} passed")
    
    if impossible_tests:
        print("\nIMPOSSIBLE TESTS (should fail gracefully):")
        for name, result in impossible_tests:
            symbol = "✓" if result else "❌"
            status = "Failed as expected" if result else "Unexpectedly survived"
            print(f"  {symbol} {name}: {status}")
        
        impossible_pass = sum(1 for _, r in impossible_tests if r)
        print(f"\n  Score: {impossible_pass}/{len(impossible_tests)} failed appropriately")
    
    print(f"\n🏥 Final server status: {'ALIVE' if check_health() else 'DEAD'}")
    print(f"   Memory: {check_memory()}")
    
    print("\n" + "="*70)

if __name__ == "__main__":
    main()
