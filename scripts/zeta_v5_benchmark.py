import requests
import time
import statistics
import json
import sys
from datetime import datetime

# Configuration
SERVER_URL = "http://192.168.0.165:9000"
GENERATE_ENDPOINT = f"{SERVER_URL}/generate"
HEALTH_ENDPOINT = f"{SERVER_URL}/health"

# Benchmark Parameters
WARMUP_ROUNDS = 3
TEST_ROUNDS = 10
PROMPTS = [
    "What is the capital of France?",
    "Explain the concept of recursion in programming.",
    "Write a function to calculate the Fibonacci sequence.",
    "Who is the current president of the United States?", # Fact check
    "Summarize the plot of Romeo and Juliet."
]

def log(message):
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")

def check_health():
    try:
        response = requests.get(HEALTH_ENDPOINT, timeout=5)
        if response.status_code == 200:
            return response.json()
    except Exception as e:
        log(f"Health check failed: {e}")
    return None

def run_benchmark():
    log("=== ZETA ZERO v5.0 BENCHMARK SUITE ===")
    log(f"Target: {SERVER_URL}")
    
    # 1. Health Check
    health = check_health()
    if not health:
        log("Server is unreachable. Aborting.")
        return
    
    log(f"Server Version: {health.get('version')}")
    log(f"Graph Nodes: {health.get('graph_nodes')}")
    log(f"Parallel 3B: {health.get('parallel_3b')}")
    
    # 2. Warmup
    log(f"\n--- Warming up ({WARMUP_ROUNDS} rounds) ---")
    for i in range(WARMUP_ROUNDS):
        try:
            requests.post(GENERATE_ENDPOINT, json={"prompt": "warmup", "max_tokens": 10}, timeout=10)
            print(".", end="", flush=True)
        except:
            print("x", end="", flush=True)
    print()
    
    # 3. Latency & Throughput Test
    log(f"\n--- Latency & Throughput ({TEST_ROUNDS} rounds) ---")
    latencies = []
    throughputs = []
    
    for i, prompt in enumerate(PROMPTS):
        log(f"Prompt {i+1}: {prompt[:30]}...")
        start_time = time.time()
        try:
            response = requests.post(GENERATE_ENDPOINT, json={"prompt": prompt, "max_tokens": 100}, timeout=30)
            end_time = time.time()
            
            if response.status_code == 200:
                data = response.json()
                duration = end_time - start_time
                tokens = data.get("tokens", 0)
                tps = tokens / duration if duration > 0 else 0
                
                latencies.append(duration)
                throughputs.append(tps)
                
                log(f"  Duration: {duration:.2f}s | Tokens: {tokens} | TPS: {tps:.2f}")
            else:
                log(f"  Failed: {response.status_code}")
        except Exception as e:
            log(f"  Exception: {e}")
            
    if latencies:
        avg_latency = statistics.mean(latencies)
        avg_tps = statistics.mean(throughputs)
        log(f"\nResults:")
        log(f"  Avg Latency: {avg_latency:.2f}s")
        log(f"  Avg Throughput: {avg_tps:.2f} tokens/s")
        
    # 4. Memory Retrieval Test (Simulated)
    log(f"\n--- Memory Retrieval Test ---")
    # We check if the graph nodes count changes or if specific content is returned
    # This is harder to verify without ground truth, but we can check for stability
    
    initial_nodes = health.get('graph_nodes', 0)
    
    # Insert a fact
    fact = "The project codename is ZetaZero."
    requests.post(GENERATE_ENDPOINT, json={"prompt": f"Remember this: {fact}", "max_tokens": 50})
    
    # Query it
    start_time = time.time()
    response = requests.post(GENERATE_ENDPOINT, json={"prompt": "What is the project codename?", "max_tokens": 50})
    duration = time.time() - start_time
    
    if response.status_code == 200:
        output = response.json().get("output", "")
        if "ZetaZero" in output:
            log(f"  [SUCCESS] Retrieval verified in {duration:.2f}s")
        else:
            log(f"  [WARN] Retrieval might have failed. Output: {output}")
            
    # Check graph growth
    final_health = check_health()
    final_nodes = final_health.get('graph_nodes', 0)
    log(f"  Graph Growth: {initial_nodes} -> {final_nodes} nodes")

    # Save results
    results = {
        "timestamp": datetime.now().isoformat(),
        "avg_latency": avg_latency if latencies else 0,
        "avg_tps": avg_tps if throughputs else 0,
        "graph_nodes": final_nodes
    }
    
    with open("benchmark_results.json", "w") as f:
        json.dump(results, f, indent=2)
    log("\nBenchmark complete. Results saved to benchmark_results.json")

if __name__ == "__main__":
    run_benchmark()
