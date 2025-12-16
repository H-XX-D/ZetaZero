#!/usr/bin/env python3
"""
Z.E.T.A. Causal Chain & Hypothetical Reasoning Test
Tests the 7B Coder's ability to:
1. Follow causal chains from existing graph nodes
2. Reason about hypothetical scenarios
3. Build on prior knowledge in memory
"""

import requests
import json
import time
import os
from datetime import datetime

SERVER_URL = "http://192.168.0.165:9000"
LOG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "logs")
TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
LOG_FILE = os.path.join(LOG_DIR, f"causal_hypothetical_{TIMESTAMP}.log")

def log(msg, also_print=True):
    timestamp = datetime.now().strftime("%H:%M:%S")
    line = f"[{timestamp}] {msg}"
    with open(LOG_FILE, "a") as f:
        f.write(line + "\n")
    if also_print:
        print(line)

def generate(prompt, max_tokens=600):
    try:
        start = time.time()
        r = requests.post(
            f"{SERVER_URL}/generate",
            headers={"Content-Type": "application/json"},
            json={"prompt": prompt, "max_tokens": max_tokens},
            timeout=180
        )
        elapsed = time.time() - start
        result = r.json()
        result["elapsed_seconds"] = elapsed
        return result
    except Exception as e:
        return {"error": str(e)}

def get_graph():
    try:
        r = requests.get(f"{SERVER_URL}/graph", timeout=30)
        return r.json()
    except:
        return {"nodes": [], "edges": []}

def run_causal_hypothetical_tests():
    os.makedirs(LOG_DIR, exist_ok=True)
    
    log("=" * 80)
    log("Z.E.T.A. CAUSAL CHAIN & HYPOTHETICAL REASONING TEST")
    log(f"Server: {SERVER_URL}")
    log(f"Log: {LOG_FILE}")
    log("=" * 80)
    
    # Get initial graph state
    graph = get_graph()
    log(f"\nInitial Graph: {len(graph.get('nodes', []))} nodes, {len(graph.get('edges', []))} edges")
    
    tests = []
    
    # =========================================================================
    # CAUSAL CHAIN TESTS - Build on existing knowledge
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 1: CAUSAL CHAIN REASONING")
    log("=" * 80)
    
    causal_tests = [
        {
            "name": "Chain 1: Fibonacci → Memoization → Performance",
            "prompt": """Based on the fibonacci function we discussed earlier with memoization:
            
If we call fibonacci(100), what is the time complexity difference between:
1. The naive recursive approach
2. The memoized approach

Explain the causal chain: why does memoization improve performance?""",
            "max_tokens": 500
        },
        {
            "name": "Chain 2: Connection Pool → Health Check → Reconnection",
            "prompt": """Regarding the async database connection pool with health checking:

Trace the causal chain:
1. What triggers a health check?
2. What happens when a health check fails?
3. How does auto-reconnection get triggered?
4. What state changes occur in the pool?

Show the cause-effect relationships.""",
            "max_tokens": 600
        },
        {
            "name": "Chain 3: Rate Limiter → Sliding Window → Redis",
            "prompt": """For the sliding window rate limiter with Redis backend:

Build the causal chain from:
Request arrives → Check rate limit → Decision made

Include:
- What data is read from Redis?
- How is the sliding window calculated?
- What causes a request to be rejected vs allowed?""",
            "max_tokens": 600
        },
        {
            "name": "Chain 4: Ring Buffer → Producer → Consumer",
            "prompt": """For the lock-free concurrent ring buffer:

Trace the causal chain when:
1. Producer A tries to push when buffer is 90% full
2. Producer B tries to push simultaneously
3. Consumer reads during this contention

What atomic operations occur and in what order?""",
            "max_tokens": 600
        },
    ]
    
    for i, test in enumerate(causal_tests, 1):
        log(f"\n--- Test {i}: {test['name']} ---")
        log(f"Prompt: {test['prompt'][:80]}...")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
        else:
            output = result.get("output", "")
            log(f"✅ Response ({result.get('tokens', 0)} tokens, {result.get('elapsed_seconds', 0):.2f}s):")
            log(f"   Momentum: {result.get('momentum', 0):.3f}")
            # Log full output
            for line in output.split("\n")[:20]:
                log(f"   {line[:100]}")
            if len(output.split("\n")) > 20:
                log(f"   ... ({len(output.split(chr(10)))} total lines)")
        
        tests.append({"name": test["name"], "result": result})
        time.sleep(1)
    
    # =========================================================================
    # HYPOTHETICAL REASONING TESTS
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 2: HYPOTHETICAL REASONING")
    log("=" * 80)
    
    hypothetical_tests = [
        {
            "name": "Hypothetical 1: What if the rate limit doubles?",
            "prompt": """HYPOTHETICAL SCENARIO:

Marcus Chen's NovaTech rate limit is currently 4500/min.
What if it suddenly doubled to 9000/min?

Analyze:
1. What infrastructure changes would be needed?
2. What could go wrong with the sliding window algorithm?
3. How would Redis memory usage change?
4. What are the downstream effects on the service?""",
            "max_tokens": 600
        },
        {
            "name": "Hypothetical 2: What if connection pool fails entirely?",
            "prompt": """HYPOTHETICAL SCENARIO:

The async database connection pool loses ALL connections simultaneously.

Trace the hypothetical chain:
1. What error state does the pool enter?
2. How does the health check respond?
3. What happens to pending queries?
4. How long until recovery?
5. What would graceful degradation look like?""",
            "max_tokens": 600
        },
        {
            "name": "Hypothetical 3: What if ring buffer overflows?",
            "prompt": """HYPOTHETICAL SCENARIO:

The lock-free ring buffer reaches capacity and producers keep trying to push.

Explore:
1. What happens to the non-blocking push calls?
2. How do producers handle False returns?
3. What if ALL producers spin-wait for space?
4. Could this cause a deadlock? Why or why not?
5. What backpressure mechanism would help?""",
            "max_tokens": 600
        },
        {
            "name": "Hypothetical 4: What if memoization cache grows unbounded?",
            "prompt": """HYPOTHETICAL SCENARIO:

The fibonacci memoization dictionary grows to 10 million entries.

Consider:
1. Memory impact?
2. Lookup time complexity change?
3. What if we add LRU eviction?
4. Optimal cache size for fibonacci specifically?
5. Trade-offs between memory and recomputation?""",
            "max_tokens": 600
        },
        {
            "name": "Hypothetical 5: Counterfactual - Different Algorithm Choice",
            "prompt": """COUNTERFACTUAL REASONING:

Earlier, we chose the Sieve of Eratosthenes for finding primes.
What if we had chosen the Miller-Rabin primality test instead?

Compare:
1. Time complexity differences
2. Space complexity differences  
3. Correctness guarantees (deterministic vs probabilistic)
4. For what input sizes does each excel?
5. When would the decision flip?""",
            "max_tokens": 700
        },
    ]
    
    for i, test in enumerate(hypothetical_tests, 1):
        log(f"\n--- Hypothetical {i}: {test['name']} ---")
        log(f"Prompt: {test['prompt'][:80]}...")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
        else:
            output = result.get("output", "")
            log(f"✅ Response ({result.get('tokens', 0)} tokens, {result.get('elapsed_seconds', 0):.2f}s):")
            log(f"   Momentum: {result.get('momentum', 0):.3f}")
            for line in output.split("\n")[:20]:
                log(f"   {line[:100]}")
            if len(output.split("\n")) > 20:
                log(f"   ... ({len(output.split(chr(10)))} total lines)")
        
        tests.append({"name": test["name"], "result": result})
        time.sleep(1)
    
    # =========================================================================
    # MULTI-HOP REASONING (Combining nodes)
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 3: MULTI-HOP REASONING (Combining Graph Nodes)")
    log("=" * 80)
    
    multihop_tests = [
        {
            "name": "Multi-hop 1: User + System Integration",
            "prompt": """Connect these facts from memory:
- Marcus Chen works at NovaTech Corporation
- NovaTech has a rate limit of 4500/min
- There's a NovaTechAuthenticator class with api_key and region params

Design a complete authentication flow that:
1. Authenticates Marcus
2. Respects the rate limit
3. Uses the correct region
4. Handles failures gracefully

Show how these components interact.""",
            "max_tokens": 700
        },
        {
            "name": "Multi-hop 2: Architecture Synthesis",
            "prompt": """Synthesize these components from our conversations:
- Async database connection pool
- Lock-free ring buffer  
- Rate limiter with Redis
- Circuit breaker pattern

Design a resilient microservice architecture that uses ALL of these.
Show the data flow and failure handling paths.""",
            "max_tokens": 800
        },
        {
            "name": "Multi-hop 3: Algorithm Composition",
            "prompt": """Combine algorithms we've discussed:
- Fibonacci with memoization
- Binary search tree
- Quick sort with partition

Create a hybrid algorithm that:
1. Uses BST for caching fibonacci results
2. Sorts cached values using quicksort
3. Enables binary search for range queries

What are the time complexities at each stage?""",
            "max_tokens": 700
        },
    ]
    
    for i, test in enumerate(multihop_tests, 1):
        log(f"\n--- Multi-hop {i}: {test['name']} ---")
        log(f"Prompt: {test['prompt'][:80]}...")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
        else:
            output = result.get("output", "")
            log(f"✅ Response ({result.get('tokens', 0)} tokens, {result.get('elapsed_seconds', 0):.2f}s):")
            log(f"   Momentum: {result.get('momentum', 0):.3f}")
            for line in output.split("\n")[:25]:
                log(f"   {line[:100]}")
            if len(output.split("\n")) > 25:
                log(f"   ... ({len(output.split(chr(10)))} total lines)")
        
        tests.append({"name": test["name"], "result": result})
        time.sleep(1)
    
    # =========================================================================
    # SUMMARY
    # =========================================================================
    
    # Get final graph state
    final_graph = get_graph()
    
    log("\n" + "=" * 80)
    log("TEST SUMMARY")
    log("=" * 80)
    
    passed = sum(1 for t in tests if "error" not in t["result"])
    failed = len(tests) - passed
    total_tokens = sum(t["result"].get("tokens", 0) for t in tests if "error" not in t["result"])
    total_time = sum(t["result"].get("elapsed_seconds", 0) for t in tests if "error" not in t["result"])
    
    log(f"Total Tests: {len(tests)}")
    log(f"Passed: {passed}")
    log(f"Failed: {failed}")
    log(f"Total Tokens: {total_tokens}")
    log(f"Total Time: {total_time:.2f}s")
    if total_time > 0:
        log(f"Avg Tokens/Sec: {total_tokens / total_time:.1f}")
    
    log(f"\nGraph Growth:")
    log(f"  Initial: {len(graph.get('nodes', []))} nodes, {len(graph.get('edges', []))} edges")
    log(f"  Final: {len(final_graph.get('nodes', []))} nodes, {len(final_graph.get('edges', []))} edges")
    node_growth = len(final_graph.get('nodes', [])) - len(graph.get('nodes', []))
    edge_growth = len(final_graph.get('edges', [])) - len(graph.get('edges', []))
    log(f"  Growth: +{node_growth} nodes, +{edge_growth} edges")
    
    log(f"\nLog saved to: {LOG_FILE}")
    log("=" * 80)
    
    return tests

if __name__ == "__main__":
    run_causal_hypothetical_tests()
