#!/usr/bin/env python3
"""
enwik9 Ingestion v2 - Domain-tagged, theme-diverse
Properly tags Wikipedia content with domain and extracts diverse themes
"""

import requests
import re
import time
import random
from collections import defaultdict

ZETA_URL = "http://localhost:8080"
ENWIK_PATH = "/mnt/GitGraph/corpus/enwik9"
BATCH_SIZE = 50
MAX_ARTICLES = 5000
RUN_MINUTES = 60

# Theme categories to track diversity
THEME_CATEGORIES = {
    'science': ['physics', 'chemistry', 'biology', 'mathematics', 'astronomy', 'geology', 'medicine'],
    'history': ['war', 'empire', 'century', 'ancient', 'medieval', 'revolution', 'dynasty'],
    'geography': ['country', 'city', 'river', 'mountain', 'island', 'ocean', 'continent'],
    'culture': ['art', 'music', 'literature', 'film', 'theatre', 'religion', 'philosophy'],
    'technology': ['computer', 'software', 'engineering', 'invention', 'machine', 'electricity'],
    'politics': ['government', 'president', 'parliament', 'election', 'democracy', 'law'],
    'sports': ['football', 'baseball', 'olympic', 'championship', 'tournament', 'athlete'],
    'nature': ['animal', 'plant', 'species', 'ecosystem', 'climate', 'forest', 'ocean'],
}

def detect_themes(title, text):
    """Detect theme categories from article content."""
    combined = (title + ' ' + text).lower()
    themes = []
    
    for category, keywords in THEME_CATEGORIES.items():
        for kw in keywords:
            if kw in combined:
                themes.append(category)
                break
    
    # Default to 'general' if no themes detected
    return themes if themes else ['general']

def extract_articles(filepath, max_articles=5000):
    """Extract diverse articles from enwik9 XML."""
    print(f"[PARSE] Reading {filepath}...")
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    print(f"[PARSE] Loaded {len(content)/1e9:.2f}GB")
    
    # Find articles
    pattern = r'<title>([^<]+)</title>.*?<text[^>]*>([^<]{200,8000})'
    
    articles = []
    theme_counts = defaultdict(int)
    
    for match in re.finditer(pattern, content, re.DOTALL):
        title = match.group(1).strip()
        text = match.group(2).strip()
        
        # Skip meta pages
        if any(title.startswith(p) for p in ['Wikipedia:', 'Template:', 'Category:', 'File:', 'MediaWiki:', 'Help:']):
            continue
        
        # Clean wiki markup
        text = re.sub(r'\[\[([^\]|]+\|)?([^\]]+)\]\]', r'\2', text)
        text = re.sub(r"'{2,}", '', text)
        text = re.sub(r'\{\{[^}]+\}\}', '', text)
        text = re.sub(r'<[^>]+>', '', text)
        text = re.sub(r'\s+', ' ', text).strip()
        
        if len(text) < 100:
            continue
        
        themes = detect_themes(title, text)
        
        # Balance theme distribution - don't oversample any category
        min_theme_count = min(theme_counts[t] for t in themes) if any(theme_counts[t] > 0 for t in themes) else 0
        if min_theme_count > max_articles // 20:  # Cap at 5% per theme
            continue
        
        articles.append((title, text, themes))
        for t in themes:
            theme_counts[t] += 1
        
        if len(articles) >= max_articles:
            break
    
    print(f"[PARSE] Extracted {len(articles)} articles")
    print(f"[THEMES] Distribution: {dict(theme_counts)}")
    return articles

def extract_facts(title, text, max_facts=3):
    """Extract key sentences as facts with title context."""
    sentences = text.replace('\n', ' ').split('. ')
    facts = []
    
    for s in sentences:
        s = s.strip()
        if len(s) > 50 and len(s) < 300:
            if s[0].isupper() and not s.startswith('*'):
                # Include title context for clarity
                facts.append(f"{s}.")
                if len(facts) >= max_facts:
                    break
    
    return facts

def commit_fact(fact, title, themes):
    """Commit fact with domain tag and themes."""
    # Format: remember: [domain:wikipedia] [theme:X,Y] Title: Fact
    theme_str = ','.join(themes[:3])
    tagged = f"remember: [domain:wikipedia] [theme:{theme_str}] {title}: {fact}"
    
    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": tagged, "max_tokens": 30, "temperature": 0.2},
            timeout=30
        )
        return resp.status_code == 200
    except Exception as e:
        return False

def get_stats():
    """Get current graph stats."""
    try:
        resp = requests.get(f"{ZETA_URL}/health", timeout=10)
        data = resp.json()
        nodes = data.get('graph_nodes', 0)
        edges = data.get('graph_edges', 0)
        return nodes, edges
    except:
        return -1, -1

def main():
    print("=" * 70)
    print("Z.E.T.A. enwik9 Ingestion v2 - Domain Tagged, Theme Diverse")
    print("=" * 70)
    
    # Check server
    nodes, edges = get_stats()
    if nodes < 0:
        print("[ERROR] Server not reachable at", ZETA_URL)
        return
    
    print(f"[START] Graph: {nodes} nodes, {edges} edges")
    
    # Parse enwik9
    articles = extract_articles(ENWIK_PATH, max_articles=MAX_ARTICLES)
    if not articles:
        print("[ERROR] No articles extracted")
        return
    
    # Shuffle for diversity
    random.shuffle(articles)
    
    start_time = time.time()
    end_time = start_time + (RUN_MINUTES * 60)
    
    initial_nodes = nodes
    processed = 0
    facts_sent = 0
    theme_stats = defaultdict(int)
    
    print(f"\n[RUN] Ingesting {len(articles)} articles for {RUN_MINUTES} min")
    print("-" * 70)
    
    for title, text, themes in articles:
        if time.time() >= end_time:
            print("\n[TIME] Run complete")
            break
        
        facts = extract_facts(title, text)
        for fact in facts:
            if commit_fact(fact, title, themes):
                facts_sent += 1
                for t in themes:
                    theme_stats[t] += 1
        
        processed += 1
        
        # Progress every 50 articles
        if processed % 50 == 0:
            nodes, edges = get_stats()
            elapsed = (time.time() - start_time) / 60
            new_nodes = nodes - initial_nodes
            rate = processed / elapsed if elapsed > 0 else 0
            
            print(f"[{processed:4d}/{len(articles)}] {facts_sent:4d} facts | +{new_nodes:3d} nodes | {rate:.0f} art/min | themes: {len(theme_stats)}")
        
        # Small delay to not overwhelm server
        time.sleep(0.1)
        
        # Longer pause every batch for dream consolidation
        if processed % BATCH_SIZE == 0:
            print(f"  [PAUSE] 5s consolidation break...")
            time.sleep(5)
    
    # Final stats
    nodes, edges = get_stats()
    elapsed = (time.time() - start_time) / 60
    
    print("\n" + "=" * 70)
    print("ENWIK9 INGESTION COMPLETE")
    print("=" * 70)
    print(f"Time: {elapsed:.1f} min")
    print(f"Articles: {processed}")
    print(f"Facts committed: {facts_sent}")
    print(f"Nodes: {initial_nodes} -> {nodes} (+{nodes - initial_nodes})")
    print(f"Themes: {dict(theme_stats)}")
    print("=" * 70)

if __name__ == "__main__":
    main()
