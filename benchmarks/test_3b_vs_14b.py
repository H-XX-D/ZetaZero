#!/usr/bin/env python3
"""
Test 3B+3B vs 14B+7B model quality comparison.
Uses same 50 prompts from original benchmark.
"""

import requests
import json
import time
from datetime import datetime

ZETA_URL = "http://192.168.0.165:8080"

# Sample prompts: mix of facts to store and questions to retrieve
PROMPTS = [
    # Facts to store (35)
    "The Great Wall of China stretches approximately 13,171 miles.",
    "Marie Curie was the first person to win Nobel Prizes in two different sciences.",
    "The Amazon River is the largest river by volume in the world.",
    "The human brain contains approximately 86 billion neurons.",
    "Mount Everest is 29,032 feet tall, the highest point on Earth.",
    "The speed of light is approximately 299,792 kilometers per second.",
    "DNA was first identified by Friedrich Miescher in 1869.",
    "The Pacific Ocean covers more area than all landmasses combined.",
    "Shakespeare wrote 37 plays during his lifetime.",
    "The Moon is about 238,855 miles from Earth on average.",
    # Question 1 (turn 10)
    "How long is the Great Wall of China?",
    "Honey never spoils due to its low moisture content and acidic pH.",
    "The Eiffel Tower was completed in 1889 for the World's Fair.",
    # Question 2 (turn 13)
    "Who was the first person to win Nobel Prizes in two different sciences?",
    "Octopuses have three hearts and blue blood.",
    "The Sahara Desert is roughly the size of the United States.",
    "Vincent van Gogh only sold one painting during his lifetime.",
    # Question 3 (turn 17)
    "What is the largest river by volume?",
    "The International Space Station orbits Earth every 92 minutes.",
    "Cleopatra lived closer in time to the Moon landing than to the building of the pyramids.",
    # Question 4 (turn 20)
    "How many neurons are in the human brain?",
    "A day on Venus is longer than a year on Venus.",
    "The shortest war in history lasted 38-45 minutes.",
    "Bananas are berries, but strawberries are not.",
    # Question 5 (turn 24)
    "What is the tallest mountain on Earth?",
    "The inventor of the Pringles can is buried in one.",
    "Polar bears have black skin under their white fur.",
    "Oxford University is older than the Aztec Empire.",
    # Question 6 (turn 28)
    "What is the speed of light?",
    "Cows have best friends and get stressed when separated.",
    "The first computer programmer was Ada Lovelace.",
    # Question 7 (turn 31)
    "When was DNA first identified?",
    "Honey bees can recognize human faces.",
    "The total weight of ants equals the total weight of humans.",
    # Question 8 (turn 34)
    "Which ocean is the largest?",
    "Dolphins sleep with one eye open.",
    "The Mona Lisa has no eyebrows.",
    # Question 9 (turn 37)
    "How many plays did Shakespeare write?",
    "A jiffy is an actual unit of time (1/100th of a second).",
    "The Greenland shark can live for over 400 years.",
    # Question 10 (turn 40)
    "How far is the Moon from Earth?",
    "Elephants are the only animals that can't jump.",
    "Russia has more surface area than Pluto.",
    "The first oranges weren't orange, they were green.",
    # Question 11 (turn 44)
    "Why doesn't honey spoil?",
    "Your nose can remember 50,000 different scents.",
    "There are more possible chess games than atoms in the observable universe.",
    # Question 12 (turn 47)
    "When was the Eiffel Tower completed?",
    "Sea otters hold hands while sleeping so they don't drift apart.",
    "The longest hiccuping spree lasted 68 years.",
    # Final question (turn 50)
    "When did Cleopatra live relative to the Moon landing and pyramids?",
]

def send_query(prompt, timeout=60):
    """Send query to Z.E.T.A. server and measure response."""
    start = time.time()
    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 150
            },
            timeout=timeout
        )
        elapsed = time.time() - start
        
        if resp.status_code == 200:
            data = resp.json()
            content = data.get("output", "")
            return {
                "success": True,
                "time_s": round(elapsed, 2),
                "response": content[:200],  # Truncate for readability
                "tokens": data.get("tokens", 0),
                "graph_nodes": data.get("graph_nodes", 0)
            }
        else:
            return {"success": False, "time_s": elapsed, "error": resp.text}
    except Exception as e:
        return {"success": False, "time_s": time.time() - start, "error": str(e)}

def run_benchmark():
    """Run 50-turn benchmark."""
    print("=" * 60)
    print("3B vs 14B Quality Comparison Benchmark")
    print("=" * 60)
    print(f"Server: {ZETA_URL}")
    print(f"Time: {datetime.now().isoformat()}")
    print()
    
    # Check health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"Server health: {health}")
    except Exception as e:
        print(f"Server not responding: {e}")
        return
    
    print()
    print("Running 50 prompts...")
    print("-" * 60)
    
    results = []
    retrieval_questions = [10, 13, 17, 20, 24, 28, 31, 34, 37, 40, 44, 47, 50]
    
    for i, prompt in enumerate(PROMPTS, 1):
        is_question = i in retrieval_questions
        marker = "Q" if is_question else "F"
        
        result = send_query(prompt)
        results.append({
            "turn": i,
            "type": "question" if is_question else "fact",
            "prompt": prompt[:50] + "..." if len(prompt) > 50 else prompt,
            **result
        })
        
        status = "✓" if result["success"] else "✗"
        print(f"[{marker}] Turn {i:2d}: {result['time_s']:.2f}s {status} - {prompt[:40]}...")
        
        # Small pause between queries
        time.sleep(0.5)
    
    # Calculate stats
    times = [r["time_s"] for r in results if r["success"]]
    question_results = [r for r in results if r["type"] == "question"]
    
    print()
    print("=" * 60)
    print("RESULTS SUMMARY")
    print("=" * 60)
    print(f"Total turns: {len(results)}")
    print(f"Successful: {len(times)}")
    print(f"Average time: {sum(times)/len(times):.2f}s")
    print(f"Min time: {min(times):.2f}s")
    print(f"Max time: {max(times):.2f}s")
    
    print()
    print("RETRIEVAL QUESTION RESPONSES:")
    print("-" * 60)
    for r in question_results:
        print(f"Turn {r['turn']}: {r['prompt']}")
        print(f"  → {r.get('response', 'NO RESPONSE')[:100]}...")
        print()
    
    # Save results
    output = {
        "benchmark": "3B vs 14B Quality Test",
        "date": datetime.now().isoformat(),
        "model": "Qwen2.5-3B-Instruct + 3B-Coder (vs 14B+7B baseline)",
        "stats": {
            "total_turns": len(results),
            "successful": len(times),
            "avg_time_s": round(sum(times)/len(times), 2),
            "min_time_s": round(min(times), 2),
            "max_time_s": round(max(times), 2)
        },
        "results": results
    }
    
    with open("3b_benchmark_results.json", "w") as f:
        json.dump(output, f, indent=2)
    
    print(f"\nResults saved to 3b_benchmark_results.json")

if __name__ == "__main__":
    run_benchmark()
