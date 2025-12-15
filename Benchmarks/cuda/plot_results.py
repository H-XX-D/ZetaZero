#!/usr/bin/env python3
"""
Plot Z.E.T.A. Benchmark Results

Generates publication-quality figures for the paper.
"""

import json
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# Use a clean style
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['axes.titlesize'] = 16

def load_results(results_dir: Path) -> dict:
    results = {}
    for name in ['retrieval', 'latency', 'persistence', 'ablation', 'resources']:
        f = results_dir / f'{name}.json'
        if f.exists():
            with open(f) as fp:
                results[name] = json.load(fp)
    return results


def plot_ablation(data: dict, output_dir: Path):
    """Plot ablation study results - stacked bar chart by test type."""
    if not data:
        return
    
    configs = [c['config'] for c in data['configs']]
    test_types = ['direct', 'multi_hop', 'temporal', 'momentum', 'noisy']
    
    # Get recall@5 for each config and test type
    fig, ax = plt.subplots(figsize=(12, 6))
    
    x = np.arange(len(configs))
    width = 0.15
    colors = ['#2ecc71', '#3498db', '#e74c3c', '#9b59b6', '#f39c12']
    
    for i, tt in enumerate(test_types):
        values = [c['by_type'][tt]['recall_at_5'] * 100 for c in data['configs']]
        bars = ax.bar(x + i * width, values, width, label=tt.replace('_', ' ').title(), color=colors[i])
    
    ax.set_xlabel('Configuration')
    ax.set_ylabel('Recall@5 (%)')
    ax.set_title('Z.E.T.A. Ablation Study: Component Contribution by Test Type')
    ax.set_xticks(x + width * 2)
    ax.set_xticklabels([c.replace('_', '\n') for c in configs], rotation=0)
    ax.legend(loc='upper left', ncol=3)
    ax.set_ylim(0, 110)
    ax.axhline(y=100, color='gray', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'ablation_by_type.png', dpi=150, bbox_inches='tight')
    plt.savefig(output_dir / 'ablation_by_type.pdf', bbox_inches='tight')
    print(f'Saved: ablation_by_type.png/pdf')
    plt.close()
    
    # Overall progression line chart
    fig, ax = plt.subplots(figsize=(10, 5))
    overall = [c['recall_at_5'] * 100 for c in data['configs']]
    mrr = [c['mrr'] * 100 for c in data['configs']]
    
    ax.plot(configs, overall, 'o-', linewidth=2, markersize=10, label='Recall@5', color='#3498db')
    ax.plot(configs, mrr, 's--', linewidth=2, markersize=8, label='MRR x100', color='#e74c3c')
    
    ax.set_xlabel('Configuration (cumulative features)')
    ax.set_ylabel('Score (%)')
    ax.set_title('Z.E.T.A. Component Progression')
    ax.set_xticklabels([c.replace('_', '\n') for c in configs], rotation=45, ha='right')
    ax.legend()
    ax.set_ylim(0, 110)
    ax.grid(True, alpha=0.3)
    
    # Annotate gains
    for i in range(1, len(overall)):
        gain = overall[i] - overall[i-1]
        if gain > 0:
            ax.annotate(f'+{gain:.0f}%', (i, overall[i] + 3), ha='center', fontsize=10, color='green')
    
    plt.tight_layout()
    plt.savefig(output_dir / 'ablation_progression.png', dpi=150, bbox_inches='tight')
    plt.savefig(output_dir / 'ablation_progression.pdf', bbox_inches='tight')
    print(f'Saved: ablation_progression.png/pdf')
    plt.close()


def plot_persistence(data: dict, output_dir: Path):
    """Plot decay curve validation."""
    if not data:
        return
    
    fig, ax = plt.subplots(figsize=(8, 5))
    
    hours = [r['hours'] for r in data['results']]
    actual = [r['avg_score'] for r in data['results']]
    expected = [r['expected_decay'] for r in data['results']]
    recall = [r['recall_rate'] * 100 for r in data['results']]
    
    ax.plot(hours, actual, 'o-', linewidth=2, markersize=10, label='Actual Score', color='#3498db')
    ax.plot(hours, expected, 's--', linewidth=2, markersize=8, label='Expected D(t)=λ^t', color='#e74c3c')
    
    ax2 = ax.twinx()
    ax2.bar(hours, recall, alpha=0.3, width=2, label='Recall Rate', color='#2ecc71')
    ax2.set_ylabel('Recall Rate (%)', color='#2ecc71')
    ax2.set_ylim(0, 120)
    
    ax.set_xlabel('Hours Since Learning')
    ax.set_ylabel('Decay Score')
    ax.set_title('Z.E.T.A. Memory Persistence: D(t) = λ^t Validation')
    ax.legend(loc='upper right')
    ax.set_ylim(0, 1.1)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'persistence_decay.png', dpi=150, bbox_inches='tight')
    plt.savefig(output_dir / 'persistence_decay.pdf', bbox_inches='tight')
    print(f'Saved: persistence_decay.png/pdf')
    plt.close()


def plot_latency(data: dict, output_dir: Path):
    """Plot latency breakdown."""
    if not data:
        return
    
    profiles = data.get('profiles', {})
    
    # SubZero breakdown
    if 'subzero' in profiles:
        fig, ax = plt.subplots(figsize=(8, 5))
        
        sz = profiles['subzero']
        components = ['entity_extraction', 'graph_walk', 'fact_retrieval', 'generation_3b']
        labels = ['Entity\nExtraction', 'Graph\nWalk', 'Fact\nRetrieval', '3B\nGeneration']
        values = [sz.get(c, {}).get('mean_ms', 0) for c in components]
        colors = ['#3498db', '#2ecc71', '#e74c3c', '#9b59b6']
        
        bars = ax.bar(labels, values, color=colors)
        ax.set_ylabel('Latency (ms)')
        ax.set_title('SubZero Pipeline Latency Breakdown')
        
        for bar, val in zip(bars, values):
            ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 5, 
                   f'{val:.1f}ms', ha='center', fontsize=10)
        
        plt.tight_layout()
        plt.savefig(output_dir / 'latency_subzero.png', dpi=150, bbox_inches='tight')
        print(f'Saved: latency_subzero.png')
        plt.close()


def plot_retrieval(data: dict, output_dir: Path):
    """Plot retrieval comparison."""
    if not data:
        return
    
    configs = data.get('configs', {})
    
    fig, ax = plt.subplots(figsize=(8, 5))
    
    names = list(configs.keys())
    r1 = [configs[n].get('recall_at_1', 0) * 100 for n in names]
    r5 = [configs[n].get('recall_at_5', 0) * 100 for n in names]
    
    x = np.arange(len(names))
    width = 0.35
    
    bars1 = ax.bar(x - width/2, r1, width, label='Recall@1', color='#3498db')
    bars2 = ax.bar(x + width/2, r5, width, label='Recall@5', color='#2ecc71')
    
    ax.set_xlabel('Configuration')
    ax.set_ylabel('Recall (%)')
    ax.set_title('Z.E.T.A. Retrieval Accuracy: Baseline vs SubZero vs Zero')
    ax.set_xticks(x)
    ax.set_xticklabels(names)
    ax.legend()
    ax.set_ylim(0, 110)
    
    for bars in [bars1, bars2]:
        for bar in bars:
            h = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2, h + 2, f'{h:.0f}%', ha='center', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'retrieval_comparison.png', dpi=150, bbox_inches='tight')
    plt.savefig(output_dir / 'retrieval_comparison.pdf', bbox_inches='tight')
    print(f'Saved: retrieval_comparison.png/pdf')
    plt.close()


def main():
    results_dir = Path('results')
    output_dir = Path('plots')
    output_dir.mkdir(exist_ok=True)
    
    print('Loading benchmark results...')
    results = load_results(results_dir)
    
    print(f'Found: {list(results.keys())}')
    
    if 'ablation' in results:
        plot_ablation(results['ablation'], output_dir)
    
    if 'persistence' in results:
        plot_persistence(results['persistence'], output_dir)
    
    if 'latency' in results:
        plot_latency(results['latency'], output_dir)
    
    if 'retrieval' in results:
        plot_retrieval(results['retrieval'], output_dir)
    
    print(f'\nAll plots saved to {output_dir}/')


if __name__ == '__main__':
    main()
