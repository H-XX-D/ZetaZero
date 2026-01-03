#!/usr/bin/env python3
"""
Wikipedia Dream Run - Ingest Wikipedia and dream at max rate
Target: Generate thousands of synthetic data points in 1 hour
"""

import requests
import json
import time
import os
import sys
from datetime import datetime, timedelta
from concurrent.futures import ThreadPoolExecutor
import random

# Configuration
ZETA_URL = "http://localhost:8080"
CORPUS_DIR = "/mnt/GitGraph/corpus"
DREAM_DIR = "/mnt/GitGraph/dreams/pending"
BATCH_SIZE = 50  # Articles per batch before dream cycle
MAX_ARTICLES = 10000  # Total articles to process in 1 hour
DREAM_CYCLES_PER_BATCH = 3  # Dream iterations after each batch

# Wikipedia categories to focus on (diverse knowledge)
FOCUS_CATEGORIES = [
    "science", "technology", "history", "mathematics", 
    "philosophy", "medicine", "engineering", "computing",
    "physics", "chemistry", "biology", "economics"
]

def load_wikipedia_sample():
    """Load Wikipedia dataset from HuggingFace."""
    print("[CORPUS] Loading Wikipedia dataset...")
    
    try:
        from datasets import load_dataset
        
        # Load Wikipedia - using 20220301.en for English Wikipedia
        # Stream to avoid downloading entire dataset
        dataset = load_dataset(
            "wikipedia", 
            "20220301.en",
            split="train",
            streaming=True,
            trust_remote_code=True
        )
        
        print("[CORPUS] Wikipedia stream ready")
        return dataset
        
    except Exception as e:
        print(f"[ERROR] Failed to load Wikipedia: {e}")
        print("[FALLBACK] Using simple-wikipedia instead...")
        
        try:
            dataset = load_dataset(
                "wikipedia",
                "20220301.simple", 
                split="train",
                streaming=True,
                trust_remote_code=True
            )
            return dataset
        except Exception as e2:
            print(f"[ERROR] Fallback failed: {e2}")
            return None

def extract_facts(article):
    """Extract key facts from a Wikipedia article for graph ingestion."""
    title = article.get('title', 'Unknown')
    text = article.get('text', '')
    
    # Get first few paragraphs (most info-dense)
    paragraphs = text.split('\n\n')[:3]
    content = '\n'.join(paragraphs)
    
    # Truncate if too long
    if len(content) > 2000:
        content = content[:2000] + "..."
    
    return {
        "title": title,
        "content": content,
        "url": article.get('url', f"wikipedia:{title}")
    }

def ingest_article(article_data):
    """Send article to Z.E.T.A. for processing and graph ingestion."""
    
    prompt = f"""Process this Wikipedia article and extract key facts for the knowledge graph.

ARTICLE: {article_data['title']}
---
{article_data['content']}
---

Extract and commit to memory:
1. Key entities (people, places, concepts)
2. Important relationships between entities  
3. Notable facts and dates
4. Connections to other knowledge domains

Respond with a brief summary of what you learned."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 256,
                "temperature": 0.3,  # Lower temp for fact extraction
                "stop": ["</s>", "\n\n\n"]
            },
            timeout=30
        )
        
        if resp.status_code == 200:
            return True, resp.json().get('response', '')[:100]
        else:
            return False, f"HTTP {resp.status_code}"
            
    except Exception as e:
        return False, str(e)

def trigger_dream_cycle():
    """Trigger a dream consolidation cycle."""
    
    # The dream system triggers on idle, so we just need to 
    # let it know we're pausing for consolidation
    prompt = """[SYSTEM: Entering dream consolidation mode]
    
Review the recently ingested Wikipedia knowledge and generate insights:
- Identify patterns across articles
- Find unexpected connections between concepts
- Generate hypotheses about relationships
- Create novel combinations of ideas

Dream freely and save interesting discoveries."""

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 512,
                "temperature": 0.95,  # High creativity for dreaming
                "stop": ["</s>"]
            },
            timeout=60
        )
        return resp.status_code == 200
    except:
        return False

def force_dream_generation():
    """Directly trigger dream generation via the dream endpoint if available."""
    try:
        # Try dream endpoint
        resp = requests.post(
            f"{ZETA_URL}/dream",
            json={"iterations": 3, "temperature": 0.9},
            timeout=120
        )
        if resp.status_code == 200:
            return True, "dream endpoint"
    except:
        pass
    
    # Fallback: Use idle trigger
    return trigger_dream_cycle(), "idle trigger"

def count_dreams():
    """Count current dreams in pending folder."""
    try:
        files = os.listdir(DREAM_DIR)
        return len([f for f in files if f.startswith('dream_')])
    except:
        return 0

def main():
    print("=" * 60)
    print("Z.E.T.A. Wikipedia Dream Run")
    print("=" * 60)
    print(f"Start time: {datetime.now()}")
    print(f"Target: {MAX_ARTICLES} articles, max dreaming")
    print(f"Batch size: {BATCH_SIZE} articles")
    print()
    
    # Check server health
    try:
        health = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        print(f"[SERVER] Status: {health.get('status')}")
        print(f"[SERVER] Graph nodes: {health.get('graph_nodes', 0)}")
    except Exception as e:
        print(f"[ERROR] Server not reachable: {e}")
        sys.exit(1)
    
    # Load Wikipedia
    dataset = load_wikipedia_sample()
    if dataset is None:
        print("[ERROR] Could not load Wikipedia dataset")
        sys.exit(1)
    
    # Track stats
    start_time = time.time()
    end_time = start_time + 3600  # 1 hour
    articles_processed = 0
    successful_ingests = 0
    dream_cycles = 0
    initial_dreams = count_dreams()
    
    print(f"\n[START] Initial dreams: {initial_dreams}")
    print(f"[START] Running until: {datetime.fromtimestamp(end_time)}")
    print("-" * 60)
    
    batch = []
    
    for article in dataset:
        # Check time limit
        if time.time() >= end_time:
            print("\n[TIME] 1 hour elapsed - stopping")
            break
            
        if articles_processed >= MAX_ARTICLES:
            print("\n[LIMIT] Max articles reached - stopping")
            break
        
        # Extract and process
        article_data = extract_facts(article)
        
        # Skip very short articles
        if len(article_data['content']) < 200:
            continue
        
        success, result = ingest_article(article_data)
        articles_processed += 1
        
        if success:
            successful_ingests += 1
            batch.append(article_data['title'])
        
        # Progress update every 10 articles
        if articles_processed % 10 == 0:
            elapsed = time.time() - start_time
            rate = articles_processed / (elapsed / 60)  # per minute
            dreams_new = count_dreams() - initial_dreams
            print(f"[PROGRESS] {articles_processed}/{MAX_ARTICLES} articles | "
                  f"{successful_ingests} ingested | {dreams_new} new dreams | "
                  f"{rate:.1f}/min")
        
        # Dream cycle after each batch
        if len(batch) >= BATCH_SIZE:
            print(f"\n[DREAM] Batch complete ({len(batch)} articles)")
            print(f"[DREAM] Triggering {DREAM_CYCLES_PER_BATCH} dream cycles...")
            
            for i in range(DREAM_CYCLES_PER_BATCH):
                success, method = force_dream_generation()
                if success:
                    dream_cycles += 1
                    print(f"  Dream cycle {i+1}/{DREAM_CYCLES_PER_BATCH} via {method}")
                time.sleep(2)  # Brief pause between cycles
            
            batch = []
            print()
    
    # Final dream burst
    print("\n[FINAL] Running final dream consolidation...")
    for i in range(5):
        force_dream_generation()
        time.sleep(3)
    
    # Final stats
    elapsed = time.time() - start_time
    final_dreams = count_dreams()
    new_dreams = final_dreams - initial_dreams
    
    print("\n" + "=" * 60)
    print("DREAM RUN COMPLETE")
    print("=" * 60)
    print(f"Duration: {elapsed/60:.1f} minutes")
    print(f"Articles processed: {articles_processed}")
    print(f"Successfully ingested: {successful_ingests}")
    print(f"Dream cycles triggered: {dream_cycles}")
    print(f"Initial dreams: {initial_dreams}")
    print(f"Final dreams: {final_dreams}")
    print(f"NEW DREAMS GENERATED: {new_dreams}")
    print(f"Dream rate: {new_dreams/(elapsed/3600):.1f} dreams/hour")
    print("=" * 60)
    
    # Save run log
    log = {
        "timestamp": datetime.now().isoformat(),
        "duration_minutes": elapsed/60,
        "articles_processed": articles_processed,
        "successful_ingests": successful_ingests,
        "dream_cycles": dream_cycles,
        "initial_dreams": initial_dreams,
        "final_dreams": final_dreams,
        "new_dreams": new_dreams,
        "dreams_per_hour": new_dreams/(elapsed/3600)
    }
    
    log_path = f"/mnt/GitGraph/corpus/dream_run_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    with open(log_path, 'w') as f:
        json.dump(log, f, indent=2)
    print(f"\nLog saved to: {log_path}")

if __name__ == "__main__":
    main()
