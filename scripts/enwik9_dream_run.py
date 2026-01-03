#!/usr/bin/env python3
"""
enwik9 Dream Run - Fast local corpus ingestion
enwik9 is 1GB of Wikipedia XML - extract articles and dream at max rate
"""

import requests
import re
import time
import os
from datetime import datetime

ZETA_URL = "http://localhost:8080"
ENWIK_PATH = "/mnt/GitGraph/corpus/enwik9"
DREAM_DIR = "/mnt/GitGraph/dreams/pending"
BATCH_SIZE = 30  # Articles before dream trigger
RUN_MINUTES = 60

def extract_articles(filepath, max_articles=5000):
    """Extract article text from enwik9 XML."""
    print(f"[PARSE] Reading {filepath}...")
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    print(f"[PARSE] Loaded {len(content)/1e9:.2f}GB")
    
    # Find <text> tags (article content)
    # Pattern: <text ...>content</text>
    pattern = r'<title>([^<]+)</title>.*?<text[^>]*>([^<]{200,5000})'
    
    articles = []
    for match in re.finditer(pattern, content, re.DOTALL):
        title = match.group(1).strip()
        text = match.group(2).strip()
        
        # Skip meta pages
        if title.startswith('Wikipedia:') or title.startswith('Template:'):
            continue
        if title.startswith('Category:') or title.startswith('File:'):
            continue
        
        # Clean up wiki markup
        text = re.sub(r'\[\[([^\]|]+\|)?([^\]]+)\]\]', r'\2', text)  # [[link|text]] -> text
        text = re.sub(r"'{2,}", '', text)  # Remove bold/italic
        text = re.sub(r'\{\{[^}]+\}\}', '', text)  # Remove templates
        text = re.sub(r'<[^>]+>', '', text)  # Remove HTML
        text = re.sub(r'\s+', ' ', text).strip()  # Normalize whitespace
        
        if len(text) > 100:
            articles.append((title, text))
            
        if len(articles) >= max_articles:
            break
    
    print(f"[PARSE] Extracted {len(articles)} articles")
    return articles

def extract_facts(text, max_facts=5):
    """Extract key sentences as facts."""
    sentences = text.replace('\n', ' ').split('. ')
    facts = []
    
    for s in sentences:
        s = s.strip()
        if len(s) > 40 and len(s) < 250:
            # Skip fragments and lists
            if s[0].isupper() and not s.startswith('*'):
                facts.append(s + '.')
                if len(facts) >= max_facts:
                    break
    
    return facts

def commit_fact(fact):
    """Use REMEMBER: prefix to commit to graph."""
    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": f"remember: {fact}", "max_tokens": 30, "temperature": 0.2},
            timeout=20
        )
        return resp.status_code == 200
    except:
        return False

def trigger_dream():
    """Trigger dream consolidation."""
    try:
        requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": "Reflect on patterns in what you've learned. Generate novel insights.",
                "max_tokens": 300,
                "temperature": 0.95,
            },
            timeout=45
        )
    except:
        pass

def get_stats():
    try:
        r = requests.get(f"{ZETA_URL}/health", timeout=5).json()
        dreams = len([f for f in os.listdir(DREAM_DIR) if f.startswith('dream_')])
        return r.get('graph_nodes', 0), r.get('graph_edges', 0), dreams
    except:
        return -1, -1, -1

def main():
    print("=" * 60)
    print("Z.E.T.A. enwik9 Dream Run")
    print("1GB Wikipedia corpus - max speed ingestion")
    print("=" * 60)
    
    # Initial stats
    nodes, edges, dreams = get_stats()
    if nodes < 0:
        print("[ERROR] Server not reachable")
        return
    
    print(f"[START] Graph: {nodes} nodes, {edges} edges")
    print(f"[START] Dreams: {dreams}")
    
    # Parse enwik9
    articles = extract_articles(ENWIK_PATH, max_articles=10000)
    if not articles:
        print("[ERROR] No articles extracted")
        return
    
    start_time = time.time()
    end_time = start_time + (RUN_MINUTES * 60)
    
    initial_nodes, initial_edges, initial_dreams = nodes, edges, dreams
    processed = 0
    facts_sent = 0
    
    print(f"\n[RUN] Processing {len(articles)} articles for {RUN_MINUTES} min")
    print("-" * 60)
    
    for title, text in articles:
        if time.time() >= end_time:
            print("\n[TIME] Run complete")
            break
        
        facts = extract_facts(text)
        for fact in facts:
            if commit_fact(fact):
                facts_sent += 1
        
        processed += 1
        
        # Progress every 20 articles
        if processed % 20 == 0:
            nodes, edges, dreams = get_stats()
            elapsed = (time.time() - start_time) / 60
            new_nodes = nodes - initial_nodes
            new_dreams = dreams - initial_dreams
            rate = processed / elapsed if elapsed > 0 else 0
            
            print(f"[{processed}/{len(articles)}] {facts_sent} facts | +{new_nodes} nodes | +{new_dreams} dreams | {rate:.0f} art/min")
        
        # Dream cycle - add idle time for dreaming to trigger
        if processed % BATCH_SIZE == 0:
            print("  [DREAM] Pausing for consolidation (10s idle)...")
            time.sleep(10)  # Let the dream thread wake up
            trigger_dream()
            time.sleep(5)  # More idle time
    
    # Final dream burst
    print("\n[FINAL] Final consolidation...")
    for _ in range(5):
        trigger_dream()
        time.sleep(2)
    
    # Stats
    nodes, edges, dreams = get_stats()
    elapsed = (time.time() - start_time) / 60
    
    print("\n" + "=" * 60)
    print("ENWIK9 DREAM RUN COMPLETE")
    print("=" * 60)
    print(f"Time: {elapsed:.1f} min")
    print(f"Articles: {processed}")
    print(f"Facts: {facts_sent}")
    print(f"Nodes: {initial_nodes} -> {nodes} (+{nodes - initial_nodes})")
    print(f"Edges: {initial_edges} -> {edges} (+{edges - initial_edges})")
    print(f"Dreams: {initial_dreams} -> {dreams} (+{dreams - initial_dreams})")
    print(f"Rate: {(dreams - initial_dreams) / (elapsed/60):.0f} dreams/hour")
    print("=" * 60)

if __name__ == "__main__":
    main()
