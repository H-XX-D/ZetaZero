#!/usr/bin/env python3
"""
Wikipedia Dream Run v2 - Faster startup with smaller dataset
Uses wikipedia-simple for faster download, still millions of facts
"""

import requests
import json
import time
import os
import sys
from datetime import datetime, timedelta

# Configuration
ZETA_URL = "http://localhost:8080"
DREAM_DIR = "/mnt/GitGraph/dreams/pending"
BATCH_SIZE = 25  # Articles per batch before dream trigger
MAX_ARTICLES = 5000  # Total articles for 1 hour
RUN_MINUTES = 60  # Run for 1 hour

def load_simple_wikipedia():
    """Load Simple Wikipedia - smaller, faster."""
    print("[CORPUS] Loading Simple Wikipedia (faster than full)...")
    
    from datasets import load_dataset
    
    # Simple Wikipedia is smaller and faster to stream
    dataset = load_dataset(
        "wikipedia",
        "20220301.simple",
        split="train",
        streaming=True,
        trust_remote_code=True
    )
    
    print("[CORPUS] Simple Wikipedia ready (~200K articles)")
    return dataset

def ingest_article(title, content):
    """Send article to Z.E.T.A. for knowledge extraction."""
    
    # Truncate long content
    if len(content) > 2500:
        content = content[:2500] + "..."
    
    prompt = f"""Learn from this Wikipedia article:

**{title}**
{content}

Extract key facts:
1. Main subject and definition
2. Important dates/numbers
3. Related concepts
4. Notable facts

Commit important facts to memory graph."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 200,
                "temperature": 0.3,
            },
            timeout=45
        )
        return resp.status_code == 200
    except Exception as e:
        return False

def trigger_dreams(n=3):
    """Trigger dream consolidation."""
    for i in range(n):
        try:
            requests.post(
                f"{ZETA_URL}/generate",
                json={
                    "prompt": "[CONSOLIDATE] Review recent knowledge. Generate insights, connections, and novel ideas. Dream freely.",
                    "max_tokens": 400,
                    "temperature": 0.95,
                },
                timeout=60
            )
        except:
            pass
        time.sleep(1)

def count_dreams():
    """Count pending dreams."""
    try:
        return len([f for f in os.listdir(DREAM_DIR) if f.startswith('dream_')])
    except:
        return 0

def main():
    print("=" * 60)
    print("Z.E.T.A. Wikipedia Dream Run v2")
    print("=" * 60)
    print(f"Target: {MAX_ARTICLES} articles over {RUN_MINUTES} minutes")
    print(f"Batch size: {BATCH_SIZE}")
    print()
    
    # Health check
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"[SERVER] Status: {health.get('status')}")
        print(f"[SERVER] Graph: {health.get('graph_nodes', 0)} nodes")
    except Exception as e:
        print(f"[ERROR] Server not reachable: {e}")
        return
    
    # Load dataset
    try:
        dataset = load_simple_wikipedia()
    except Exception as e:
        print(f"[ERROR] Failed to load dataset: {e}")
        return
    
    start_time = time.time()
    end_time = start_time + (RUN_MINUTES * 60)
    articles = 0
    ingested = 0
    batches = 0
    initial_dreams = count_dreams()
    
    print(f"\n[START] Dreams at start: {initial_dreams}")
    print(f"[START] Will run until: {datetime.fromtimestamp(end_time).strftime('%H:%M:%S')}")
    print("-" * 60)
    
    for article in dataset:
        if time.time() >= end_time or articles >= MAX_ARTICLES:
            break
        
        title = article.get('title', '')
        text = article.get('text', '')
        
        # Skip stubs
        if len(text) < 300:
            continue
        
        # Get first paragraphs
        paragraphs = text.split('\n\n')[:2]
        content = '\n'.join(paragraphs)
        
        if ingest_article(title, content):
            ingested += 1
        articles += 1
        
        # Progress every 10
        if articles % 10 == 0:
            dreams_now = count_dreams()
            new_dreams = dreams_now - initial_dreams
            elapsed = (time.time() - start_time) / 60
            rate = articles / elapsed if elapsed > 0 else 0
            print(f"[{articles}/{MAX_ARTICLES}] {ingested} ingested | +{new_dreams} dreams | {rate:.0f}/min")
        
        # Dream after each batch
        if articles % BATCH_SIZE == 0:
            batches += 1
            print(f"\n[BATCH {batches}] Triggering dream consolidation...")
            trigger_dreams(3)
            print()
    
    # Final dream burst
    print("\n[FINAL] Final consolidation burst...")
    trigger_dreams(5)
    
    # Stats
    elapsed = (time.time() - start_time) / 60
    final_dreams = count_dreams()
    new_dreams = final_dreams - initial_dreams
    
    print("\n" + "=" * 60)
    print("COMPLETE")
    print("=" * 60)
    print(f"Time: {elapsed:.1f} minutes")
    print(f"Articles: {articles}")
    print(f"Ingested: {ingested}")
    print(f"Batches: {batches}")
    print(f"Dreams: {initial_dreams} -> {final_dreams}")
    print(f"NEW DREAMS: {new_dreams}")
    print(f"Rate: {new_dreams/max(elapsed/60, 0.1):.0f} dreams/hour")
    print("=" * 60)
    
    # Save log
    log = {
        "timestamp": datetime.now().isoformat(),
        "minutes": elapsed,
        "articles": articles,
        "ingested": ingested,
        "new_dreams": new_dreams,
        "dreams_per_hour": new_dreams/max(elapsed/60, 0.1)
    }
    
    with open(f"/mnt/GitGraph/corpus/run_{datetime.now().strftime('%H%M%S')}.json", 'w') as f:
        json.dump(log, f, indent=2)

if __name__ == "__main__":
    main()
