#!/usr/bin/env python3
"""
Z.E.T.A. Causal Chain & Hypothetical Reasoning Test (Code-Focused)
Tests the 7B Coder's ability to generate code that demonstrates causal reasoning.
Uses code prompts to match the 7B Coder model's training focus.
"""

import requests
import json
import time
import os
from datetime import datetime

SERVER_URL = "http://192.168.0.165:9000"
LOG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "logs")
TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
LOG_FILE = os.path.join(LOG_DIR, f"causal_code_{TIMESTAMP}.log")

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

def run_tests():
    os.makedirs(LOG_DIR, exist_ok=True)
    
    log("=" * 80)
    log("Z.E.T.A. CAUSAL CHAIN TEST (CODE-FOCUSED)")
    log(f"Server: {SERVER_URL}")
    log(f"Log: {LOG_FILE}")
    log("=" * 80)
    
    # Get initial graph state
    graph = get_graph()
    log(f"\nInitial Graph: {len(graph.get('nodes', []))} nodes, {len(graph.get('edges', []))} edges")
    
    # =========================================================================
    # CODE-FOCUSED CAUSAL CHAIN TESTS
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 1: CAUSAL CHAINS IN CODE")
    log("=" * 80)
    
    code_tests = [
        {
            "name": "Causal: Fibonacci Memoization Comparison",
            "prompt": '''Write Python code showing the causal performance difference between naive and memoized fibonacci.
Include comments tracing why memoization changes O(2^n) to O(n):

```python
# Causal Chain Demo: Fibonacci Memoization
# Shows: Recursive calls → Cache hits → Time reduction

import functools
import time

def fib_naive(n):''',
            "max_tokens": 500
        },
        {
            "name": "Causal: Connection Pool State Machine",
            "prompt": '''Write a Python connection pool with explicit state transitions.
Show the causal chain: acquire → use → release → expire → reconnect.
Include comments explaining cause → effect for each state change:

```python
# Connection Pool State Machine
# Causal chain: IDLE → ACQUIRED → IN_USE → RELEASED → EXPIRED → RECONNECTING

from enum import Enum
from dataclasses import dataclass
from typing import Optional

class ConnState(Enum):''',
            "max_tokens": 600
        },
        {
            "name": "Causal: Rate Limiter Token Bucket",
            "prompt": '''Implement a token bucket rate limiter showing the causal chain of request handling.
bucket_size=100, refill_rate=10/sec. Add step-by-step comments tracing what happens when 200 requests hit in 1 second:

```python
# Token Bucket Rate Limiter - Causal Trace
# Shows: Request arrives → Check tokens → Consume or reject → Refill over time

import time
from threading import Lock

class TokenBucketRateLimiter:
    def __init__(self, bucket_size: int = 100, refill_rate: float = 10):''',
            "max_tokens": 500
        },
        {
            "name": "Causal: Memory Allocation Failure Cascade",
            "prompt": '''Write C code that safely handles malloc failure with full cleanup cascade.
Add comments explaining the 3 causes of NULL and the cleanup chain:

```c
// Memory Allocation Failure Cascade
// Causal chain: malloc() → NULL → cleanup_previous → set_error → return

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *name;
    char *data;
    size_t data_len;
} Resource;

// Allocate resource with full error handling
Resource* create_resource(const char* name, size_t data_len) {''',
            "max_tokens": 500
        }
    ]
    
    results = []
    total_tokens = 0
    passed = 0
    
    for i, test in enumerate(code_tests, 1):
        log(f"\n--- Test {i}/4: {test['name']} ---")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
            results.append({"name": test["name"], "passed": False})
        else:
            output = result.get("output", "")
            tokens = result.get("tokens", 0)
            elapsed = result.get("elapsed_seconds", 0)
            momentum = result.get("momentum", 0)
            
            total_tokens += tokens
            
            # Check for meaningful code output (not just 1 token)
            has_code = tokens > 20 and (
                "def " in output or 
                "return " in output or 
                "class " in output or
                "//" in output or
                "#include" in output
            )
            
            if has_code:
                passed += 1
                log(f"✅ PASS - {tokens} tokens, {elapsed:.1f}s ({tokens/elapsed:.1f} tok/s)")
            else:
                log(f"⚠️  WEAK - Only {tokens} tokens, may be truncated")
            
            log(f"   Momentum: {momentum:.3f}")
            log(f"   Output preview:")
            lines = output.strip().split("\n")
            for line in lines[:15]:
                log(f"   | {line[:90]}")
            if len(lines) > 15:
                log(f"   ... ({len(lines)} total lines)")
            
            results.append({
                "name": test["name"],
                "passed": has_code,
                "tokens": tokens,
                "elapsed": elapsed
            })
        
        time.sleep(1)
    
    # =========================================================================
    # HYPOTHETICAL CODE SCENARIOS
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 2: HYPOTHETICAL CODE SCENARIOS")
    log("=" * 80)
    
    hypo_tests = [
        {
            "name": "Hypothetical: Pool Under Stress",
            "prompt": '''Write a Python simulation showing what happens when a connection pool is overwhelmed.
Simulate 1000 concurrent requests to a pool of 10 connections, each query taking 100ms.
Show metrics and what fails:

```python
# Hypothetical: Connection Pool Under Stress
# Scenario: 1000 requests, 10 connections, 100ms per query
# Question: What breaks and how?

import asyncio
import time
from collections import deque

class OverwhelmedPool:
    def __init__(self, size=10, timeout=1.0):''',
            "max_tokens": 500
        },
        {
            "name": "Hypothetical: Rate Limit Migration",
            "prompt": '''Write code showing the transition when rate limit doubles from 4500/min to 9000/min.
Include the Redis sliding window changes and potential memory issues:

```python
# Hypothetical: Rate Limit Doubling Migration
# Before: 4500/min, After: 9000/min
# Show: Redis key changes, memory growth, edge cases

import redis
import time

class RateLimitMigration:
    def __init__(self, redis_client):
        self.redis = redis_client
        self.old_limit = 4500
        self.new_limit = 9000
        self.window_sec = 60
    
    def check_migration_impact(self, user_id: str):''',
            "max_tokens": 500
        },
        {
            "name": "Hypothetical: Ring Buffer Contention",
            "prompt": '''Write a lock-free ring buffer that shows what happens under heavy producer contention.
Include atomic operations and trace the failure modes:

```python
# Hypothetical: Ring Buffer Under Heavy Contention
# Multiple producers, buffer nearly full, what happens?

import threading
import time
from typing import Generic, TypeVar, Optional
from collections import deque

T = TypeVar('T')

class RingBuffer(Generic[T]):
    """Ring buffer showing contention behavior."""
    
    def __init__(self, capacity: int = 1024):''',
            "max_tokens": 500
        },
        {
            "name": "Hypothetical: Cache Memory Leak",
            "prompt": '''Write Python code demonstrating an unbounded memoization cache memory leak.
Show memory growth over time and implement bounded cache as solution:

```python
# Hypothetical: Unbounded Cache Memory Leak
# Problem: functools.cache with no size limit
# Show: Memory growth + LRU cache solution

import functools
import sys
import time
from collections import OrderedDict

# PROBLEM: Unbounded cache
@functools.cache
def expensive_compute_unbounded(x):''',
            "max_tokens": 500
        }
    ]
    
    for i, test in enumerate(hypo_tests, 1):
        log(f"\n--- Hypothetical {i}/4: {test['name']} ---")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
        else:
            output = result.get("output", "")
            tokens = result.get("tokens", 0)
            elapsed = result.get("elapsed_seconds", 0)
            
            total_tokens += tokens
            
            has_code = tokens > 20 and (
                "def " in output or 
                "return " in output or 
                "class " in output
            )
            
            if has_code:
                passed += 1
                log(f"✅ PASS - {tokens} tokens, {elapsed:.1f}s ({tokens/elapsed:.1f} tok/s)")
            else:
                log(f"⚠️  WEAK - Only {tokens} tokens")
            
            log(f"   Output preview:")
            lines = output.strip().split("\n")
            for line in lines[:15]:
                log(f"   | {line[:90]}")
            if len(lines) > 15:
                log(f"   ... ({len(lines)} total lines)")
        
        time.sleep(1)
    
    # =========================================================================
    # MULTI-HOP CAUSAL REASONING (Code Generation)
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("SECTION 3: MULTI-HOP CAUSAL CODE GENERATION")
    log("=" * 80)
    
    multi_hop = [
        {
            "name": "Multi-Hop: Refactor Naive to Production",
            "prompt": '''Take this naive fibonacci and refactor it through 3 stages, each solving problems from the previous stage:

Stage 1 (Given):
```python
def fib(n):
    if n < 2: return n
    return fib(n-1) + fib(n-2)
```

Implement Stage 2 (Memoized) and Stage 3 (Iterative) with comments explaining WHY each change addresses the previous stage's problems:

```python
# Stage 2: Memoized - Solves: exponential time from redundant calls
def fib_memoized(n, memo={}):''',
            "max_tokens": 500
        },
        {
            "name": "Multi-Hop: Build Resilient Service",
            "prompt": '''Build a resilient HTTP client that implements 4 layers of protection, each addressing failures from the previous layer:

Layer 1: Basic request (given)
Layer 2: Retry on timeout (addresses: transient failures)
Layer 3: Circuit breaker (addresses: cascading failures from retries)
Layer 4: Fallback cache (addresses: circuit open state)

```python
# Multi-layer resilient HTTP client
# Each layer solves problems from the previous

import requests
import time
from functools import wraps

# Layer 2: Retry with exponential backoff
def with_retry(max_retries=3, base_delay=1.0):''',
            "max_tokens": 600
        }
    ]
    
    for i, test in enumerate(multi_hop, 1):
        log(f"\n--- Multi-Hop {i}/2: {test['name']} ---")
        
        result = generate(test["prompt"], test["max_tokens"])
        
        if "error" in result:
            log(f"❌ ERROR: {result['error']}")
        else:
            output = result.get("output", "")
            tokens = result.get("tokens", 0)
            elapsed = result.get("elapsed_seconds", 0)
            
            total_tokens += tokens
            
            has_code = tokens > 30 and "def " in output
            
            if has_code:
                passed += 1
                log(f"✅ PASS - {tokens} tokens, {elapsed:.1f}s")
            else:
                log(f"⚠️  WEAK - Only {tokens} tokens")
            
            log(f"   Output preview:")
            lines = output.strip().split("\n")
            for line in lines[:20]:
                log(f"   | {line[:90]}")
            if len(lines) > 20:
                log(f"   ... ({len(lines)} total lines)")
        
        time.sleep(1)
    
    # =========================================================================
    # SUMMARY
    # =========================================================================
    
    log("\n" + "=" * 80)
    log("TEST SUMMARY")
    log("=" * 80)
    
    total_tests = len(code_tests) + len(hypo_tests) + len(multi_hop)
    log(f"Total Tests: {total_tests}")
    log(f"Passed: {passed}/{total_tests} ({100*passed/total_tests:.0f}%)")
    log(f"Total Tokens: {total_tokens}")
    
    # Final graph state
    final_graph = get_graph()
    log(f"\nFinal Graph: {len(final_graph.get('nodes', []))} nodes, {len(final_graph.get('edges', []))} edges")
    nodes_added = len(final_graph.get('nodes', [])) - len(graph.get('nodes', []))
    log(f"Nodes added during test: {nodes_added}")
    
    log("\n" + "=" * 80)
    log(f"Log saved to: {LOG_FILE}")
    
    return passed >= (total_tests // 2)

if __name__ == "__main__":
    success = run_tests()
    exit(0 if success else 1)
