#!/usr/bin/env python3
"""
Wikipedia Dream Run v3 - Uses REMEMBER: prefix to commit facts
"""

import requests
import json
import time
import os
import sys
from datetime import datetime

ZETA_URL = "http://localhost:8080"
DREAM_DIR = "/mnt/GitGraph/dreams/pending"
BATCH_SIZE = 20
MAX_ARTICLES = 3000
RUN_MINUTES = 60

def load_wikipedia():
    """Load Simple Wikipedia."""
    print("[CORPUS] Loading Simple Wikipedia...")
    from datasets import load_dataset
    return load_dataset("wikipedia", "20220301.simple", split="train", streaming=True, trust_remote_code=True)

def extract_facts(text, title):
    """Extract key sentences as facts."""
    sentences = text.replace('\n', ' ').split('. ')
    facts = []
    
    # Get first 5 informative sentences
    for s in sentences[:10]:
        s = s.strip()
        if len(s) > 30 and len(s) < 300:
            # Skip meta sentences
            if s.startswith('This article') or s.startswith('See also'):
                continue
            facts.append(s)
            if len(facts) >= 5:
                break
    
    return facts

def commit_fact(fact, title):
    """Use REMEMBER: prefix to trigger graph commit."""
    
    # The system parses "remember:" to commit facts
    prompt = f"remember: {fact}"
    
    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": prompt,
                "max_tokens": 50,  # Short response
                "temperature": 0.3,
            },
            timeout=30
        )
        return resp.status_code == 200
    except:
        return False

def trigger_dreams():
    """Let the system dream."""
    try:
        requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": "Think about what you've learned. Find connections and patterns.",
                "max_tokens": 300,
                "temperature": 0.9,
            },
            timeout=60
        )
    except:
        pass

def count_dreams():
    try:
        return len([f for f in os.listdir(DREAM_DIR) if f.startswith('dream_')])
    except:
        return 0

def get_graph_nodes():
    try:
        r = requests.get(f"{ZETA_URL}/health", timeout=5)
        return r.json().get('graph_nodes', 0)
    except:
        return -1

def main():
    print("=" * 60)
    print("Z.E.T.A. Wikipedia Dream Run v3")
    print("Using REMEMBER: prefix for graph commits")
    print("=" * 60)
    
    # Health check
    nodes = get_graph_nodes()
    if nodes < 0:
        print("[ERROR] Server not reachable")
        return
    print(f"[SERVER] Initial graph: {nodes} nodes")
    
    # Load dataset
    try:
        dataset = load_wikipedia()
    except Exception as e:
        print(f"[ERROR] {e}")
        return
    
    start_time = time.time()
    end_time = start_time + (RUN_MINUTES * 60)
    
    articles = 0
    facts_sent = 0
    initial_dreams = count_dreams()
    initial_nodes = nodes
    
    print(f"\n[START] Dreams: {initial_dreams}, Nodes: {initial_nodes}")
    print(f"[START] Running for {RUN_MINUTES} minutes")
    print("-" * 60)
    
    for article in dataset:
        if time.time() >= end_time or articles >= MAX_ARTICLES:
            break
        
        title = article.get('title', '')
        text = article.get('text', '')
        
        if len(text) < 200:
            continue
        
        # Extract facts
        facts = extract_facts(text, title)
        
        for fact in facts:
            if commit_fact(fact, title):
                facts_sent += 1
        
        articles += 1
        
        # Progress
        if articles % 10 == 0:
            current_nodes = get_graph_nodes()
            current_dreams = count_dreams()
            elapsed = (time.time() - start_time) / 60
            node_delta = current_nodes - initial_nodes
            dream_delta = current_dreams - initial_dreams
            
            print(f"[{articles}] {facts_sent} facts | +{node_delta} nodes | +{dream_delta} dreams | {elapsed:.1f}min")
        
        # Dream cycle
        if articles % BATCH_SIZE == 0:
            print(f"  [DREAM] Triggering consolidation...")
            trigger_dreams()
            time.sleep(2)
    
    # Final
    print("\n[FINAL] Final dream burst...")
    for _ in range(3):
        trigger_dreams()
        time.sleep(3)
    
    elapsed = (time.time() - start_time) / 60
    final_nodes = get_graph_nodes()
    final_dreams = count_dreams()
    
    print("\n" + "=" * 60)
    print("COMPLETE")
    print("=" * 60)
    print(f"Time: {elapsed:.1f} minutes")
    print(f"Articles: {articles}")
    print(f"Facts sent: {facts_sent}")
    print(f"Nodes: {initial_nodes} -> {final_nodes} (+{final_nodes - initial_nodes})")
    print(f"Dreams: {initial_dreams} -> {final_dreams} (+{final_dreams - initial_dreams})")
    print("=" * 60)

if __name__ == "__main__":
    main()
