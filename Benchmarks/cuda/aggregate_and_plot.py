#!/usr/bin/env python3
"""
Aggregate multiple benchmark runs and generate publication-quality plots.
"""

import json
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from collections import defaultdict

plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['axes.titlesize'] = 16
plt.rcParams['figure.figsize'] = (10, 6)

RESULTS_DIR = Path('results/runs')
OUTPUT_DIR = Path('results/plots')
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

def load_all_runs(prefix: str, n_runs: int = 5) -> list:
    """Load all runs of a benchmark."""
    runs = []
    for i in range(1, n_runs + 1):
        path = RESULTS_DIR / f'{prefix}_{i}.json'
        if path.exists():
            with open(path) as f:
                runs.append(json.load(f))
    return runs

def aggregate_retrieval(runs: list) -> dict:
    """Aggregate retrieval results across runs."""
    configs = defaultdict(lambda: defaultdict(list))
    
    for run in runs:
        for config, metrics in run.get('configs', {}).items():
            for metric, value in metrics.items():
                configs[config][metric].append(value)
    
    aggregated = {}
    for config, metrics in configs.items():
        aggregated[config] = {
            metric: {'mean': np.mean(values), 'std': np.std(values)}
            for metric, values in metrics.items()
        }
    return aggregated

def aggregate_ablation(runs: list) -> dict:
    """Aggregate ablation results across runs."""
    configs = defaultdict(lambda: defaultdict(list))
    
    for run in runs:
        for cfg in run.get('configs', []):
            name = cfg['config']
            configs[name]['recall_at_5'].append(cfg['recall_at_5'])
            configs[name]['mrr'].append(cfg['mrr'])
            for ttype, metrics in cfg.get('by_type', {}).items():
                configs[name][f'{ttype}_recall'].append(metrics['recall_at_5'])
    
    aggregated = {}
    for config, metrics in configs.items():
        aggregated[config] = {
            metric: {'mean': np.mean(values), 'std': np.std(values)}
            for metric, values in metrics.items()
        }
    return aggregated

def aggregate_latency(runs: list) -> dict:
    """Aggregate latency results across runs."""
    profiles = defaultdict(lambda: defaultdict(list))
    
    for run in runs:
        for profile_name, ops in run.get('profiles', {}).items():
            for op_name, stats in ops.items():
                if isinstance(stats, dict) and 'mean_ms' in stats:
                    profiles[profile_name][op_name].append(stats['mean_ms'])
    
    aggregated = {}
    for profile, ops in profiles.items():
        aggregated[profile] = {
            op: {'mean': np.mean(values), 'std': np.std(values)}
            for op, values in ops.items()
        }
    return aggregated

def aggregate_persistence(runs: list) -> dict:
    """Aggregate persistence results across runs."""
    hours_data = defaultdict(lambda: defaultdict(list))
    
    for run in runs:
        for result in run.get('results', []):
            h = result['hours']
            hours_data[h]['recall'].append(result['recall_rate'])
            hours_data[h]['score'].append(result['avg_score'])
            hours_data[h]['expected'].append(result['expected_decay'])
    
    aggregated = {}
    for hours, metrics in sorted(hours_data.items()):
        aggregated[hours] = {
            'recall': {'mean': np.mean(metrics['recall']), 'std': np.std(metrics['recall'])},
            'score': {'mean': np.mean(metrics['score']), 'std': np.std(metrics['score'])},
            'expected': metrics['expected'][0]
        }
    return aggregated

def plot_retrieval(data: dict):
    """Plot retrieval accuracy comparison."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    configs = ['baseline', 'subzero', 'zero']
    metrics = ['recall_at_1', 'recall_at_5', 'mrr']
    x = np.arange(len(configs))
    width = 0.25
    
    colors = ['#2ecc71', '#3498db', '#9b59b6']
    
    for i, metric in enumerate(metrics):
        means = [data.get(c, {}).get(metric, {}).get('mean', 0) * 100 for c in configs]
        stds = [data.get(c, {}).get(metric, {}).get('std', 0) * 100 for c in configs]
        bars = ax.bar(x + i*width, means, width, yerr=stds, label=metric.replace('_', '@').upper(),
                     color=colors[i], capsize=5, alpha=0.8)
    
    ax.set_xlabel('Configuration')
    ax.set_ylabel('Score (%)')
    ax.set_title('Z.E.T.A. Retrieval Accuracy (5 runs)')
    ax.set_xticks(x + width)
    ax.set_xticklabels(['Baseline', 'SubZero', 'Zero'])
    ax.legend()
    ax.set_ylim(0, 110)
    
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / 'retrieval_accuracy.png', dpi=150)
    plt.savefig(OUTPUT_DIR / 'retrieval_accuracy.pdf')
    plt.close()
    print('Saved: retrieval_accuracy.png/pdf')

def plot_ablation(data: dict):
    """Plot ablation study progression."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # Order of configs
    config_order = ['raw_cosine', 'plus_cubic', 'plus_tunnel', 'plus_decay', 'plus_graph', 'full_subzero']
    labels = ['Cosine', '+Cubic³', '+Tunnel', '+Decay', '+Graph', 'Full']
    
    # Recall@5 progression
    means = [data.get(c, {}).get('recall_at_5', {}).get('mean', 0) * 100 for c in config_order]
    stds = [data.get(c, {}).get('recall_at_5', {}).get('std', 0) * 100 for c in config_order]
    
    ax1.errorbar(range(len(config_order)), means, yerr=stds, marker='o', markersize=10,
                linewidth=2, capsize=5, color='#e74c3c')
    ax1.fill_between(range(len(config_order)), 
                    [m-s for m,s in zip(means, stds)],
                    [m+s for m,s in zip(means, stds)],
                    alpha=0.2, color='#e74c3c')
    ax1.set_xticks(range(len(config_order)))
    ax1.set_xticklabels(labels, rotation=45, ha='right')
    ax1.set_ylabel('Recall@5 (%)')
    ax1.set_title('Component Contribution (Cumulative)')
    ax1.set_ylim(0, 110)
    
    # By test type
    test_types = ['direct', 'multi_hop', 'temporal', 'noisy']
    type_labels = ['Direct', 'Multi-hop', 'Temporal', 'Noisy']
    x = np.arange(len(test_types))
    width = 0.35
    
    baseline_means = [data.get('raw_cosine', {}).get(f'{t}_recall', {}).get('mean', 0) * 100 for t in test_types]
    full_means = [data.get('full_subzero', {}).get(f'{t}_recall', {}).get('mean', 0) * 100 for t in test_types]
    
    ax2.bar(x - width/2, baseline_means, width, label='Cosine Only', color='#95a5a6')
    ax2.bar(x + width/2, full_means, width, label='Full SubZero', color='#27ae60')
    ax2.set_xticks(x)
    ax2.set_xticklabels(type_labels)
    ax2.set_ylabel('Recall@5 (%)')
    ax2.set_title('Performance by Test Type')
    ax2.legend()
    ax2.set_ylim(0, 110)
    
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / 'ablation_study.png', dpi=150)
    plt.savefig(OUTPUT_DIR / 'ablation_study.pdf')
    plt.close()
    print('Saved: ablation_study.png/pdf')

def plot_latency(data: dict):
    """Plot latency breakdown."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # SubZero breakdown
    if 'subzero' in data:
        ops = ['entity_extraction', 'graph_walk', 'fact_retrieval', 'generation_3b']
        labels = ['Entity Extract', 'Graph Walk', 'Fact Retrieval', '3B Generation']
        means = [data['subzero'].get(op, {}).get('mean', 0) for op in ops]
        stds = [data['subzero'].get(op, {}).get('std', 0) for op in ops]
        
        colors = ['#3498db', '#2ecc71', '#f39c12', '#e74c3c']
        bars = ax1.bar(labels, means, yerr=stds, capsize=5, color=colors, alpha=0.8)
        ax1.set_ylabel('Time (ms)')
        ax1.set_title('SubZero Pipeline Breakdown')
        ax1.tick_params(axis='x', rotation=45)
        
        # Add value labels
        for bar, mean in zip(bars, means):
            ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 5,
                    f'{mean:.1f}', ha='center', va='bottom', fontsize=10)
    
    # Comparison
    configs = []
    totals = []
    stds_list = []
    
    if 'subzero' in data and 'total_subzero' in data['subzero']:
        configs.append('SubZero')
        totals.append(data['subzero']['total_subzero']['mean'])
        stds_list.append(data['subzero']['total_subzero']['std'])
    
    if 'zero' in data and 'total_zero' in data['zero']:
        configs.append('Zero\n(overhead)')
        totals.append(data['zero']['total_zero']['mean'])
        stds_list.append(data['zero']['total_zero']['std'])
    
    if configs:
        colors = ['#e74c3c', '#9b59b6'][:len(configs)]
        bars = ax2.bar(configs, totals, yerr=stds_list, capsize=5, color=colors, alpha=0.8)
        ax2.set_ylabel('Time (ms)')
        ax2.set_title('Total Latency Comparison')
        
        for bar, total in zip(bars, totals):
            ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 10,
                    f'{total:.0f}ms', ha='center', va='bottom', fontsize=12, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / 'latency_breakdown.png', dpi=150)
    plt.savefig(OUTPUT_DIR / 'latency_breakdown.pdf')
    plt.close()
    print('Saved: latency_breakdown.png/pdf')

def plot_persistence(data: dict):
    """Plot memory persistence decay curve."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    hours = sorted(data.keys())
    measured_scores = [data[h]['score']['mean'] for h in hours]
    measured_stds = [data[h]['score']['std'] for h in hours]
    expected = [data[h]['expected'] for h in hours]
    recall = [data[h]['recall']['mean'] * 100 for h in hours]
    
    # Plot measured vs expected
    ax.errorbar(hours, measured_scores, yerr=measured_stds, marker='o', markersize=10,
               linewidth=2, capsize=5, color='#e74c3c', label='Measured Score')
    ax.plot(hours, expected, '--', linewidth=2, color='#3498db', label='Expected D(t)=λ^t')
    
    ax.set_xlabel('Hours Since Learning')
    ax.set_ylabel('Score')
    ax.set_title('Memory Persistence: Measured vs Theoretical Decay')
    ax.legend()
    ax.set_xlim(-2, max(hours) + 2)
    ax.set_ylim(0, 1.1)
    
    # Add recall annotations
    for h, r, s in zip(hours, recall, measured_scores):
        ax.annotate(f'{r:.0f}%', (h, s + 0.08), ha='center', fontsize=9, color='#27ae60')
    
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / 'persistence_decay.png', dpi=150)
    plt.savefig(OUTPUT_DIR / 'persistence_decay.pdf')
    plt.close()
    print('Saved: persistence_decay.png/pdf')

def generate_summary_table(retrieval, ablation, latency, persistence):
    """Generate summary markdown table."""
    lines = [
        '# Z.E.T.A. Benchmark Results (5 runs, mean ± std)',
        '',
        '## Retrieval Accuracy',
        '| Config | Recall@1 | Recall@5 | MRR |',
        '|--------|----------|----------|-----|'
    ]
    
    for cfg in ['baseline', 'subzero', 'zero']:
        if cfg in retrieval:
            r1 = retrieval[cfg].get('recall_at_1', {})
            r5 = retrieval[cfg].get('recall_at_5', {})
            mrr = retrieval[cfg].get('mrr', {})
            lines.append(f"| {cfg.title()} | {r1.get('mean',0)*100:.1f}±{r1.get('std',0)*100:.1f}% | "
                        f"{r5.get('mean',0)*100:.1f}±{r5.get('std',0)*100:.1f}% | "
                        f"{mrr.get('mean',0):.3f}±{mrr.get('std',0):.3f} |")
    
    lines.extend(['', '## Latency', '| Pipeline | Total (ms) |', '|----------|------------|'])
    
    if 'subzero' in latency and 'total_subzero' in latency['subzero']:
        t = latency['subzero']['total_subzero']
        lines.append(f"| SubZero | {t['mean']:.1f}±{t['std']:.1f} |")
    if 'zero' in latency and 'total_zero' in latency['zero']:
        t = latency['zero']['total_zero']
        lines.append(f"| Zero | {t['mean']:.1f}±{t['std']:.1f} |")
    
    lines.extend(['', '## Ablation (Recall@5)', '| Config | Score |', '|--------|-------|'])
    
    for cfg in ['raw_cosine', 'plus_cubic', 'plus_tunnel', 'plus_decay', 'plus_graph', 'full_subzero']:
        if cfg in ablation:
            r = ablation[cfg].get('recall_at_5', {})
            lines.append(f"| {cfg} | {r.get('mean',0)*100:.1f}±{r.get('std',0)*100:.1f}% |")
    
    with open(OUTPUT_DIR / 'SUMMARY.md', 'w') as f:
        f.write('\n'.join(lines))
    print('Saved: SUMMARY.md')

def main():
    print('Loading benchmark runs...')
    
    retrieval_runs = load_all_runs('retrieval')
    ablation_runs = load_all_runs('ablation')
    latency_runs = load_all_runs('latency')
    persistence_runs = load_all_runs('persistence')
    
    print(f'  Retrieval: {len(retrieval_runs)} runs')
    print(f'  Ablation: {len(ablation_runs)} runs')
    print(f'  Latency: {len(latency_runs)} runs')
    print(f'  Persistence: {len(persistence_runs)} runs')
    
    print('\nAggregating results...')
    retrieval = aggregate_retrieval(retrieval_runs) if retrieval_runs else {}
    ablation = aggregate_ablation(ablation_runs) if ablation_runs else {}
    latency = aggregate_latency(latency_runs) if latency_runs else {}
    persistence = aggregate_persistence(persistence_runs) if persistence_runs else {}
    
    print('\nGenerating plots...')
    if retrieval:
        plot_retrieval(retrieval)
    if ablation:
        plot_ablation(ablation)
    if latency:
        plot_latency(latency)
    if persistence:
        plot_persistence(persistence)
    
    print('\nGenerating summary table...')
    generate_summary_table(retrieval, ablation, latency, persistence)
    
    # Save aggregated JSON
    with open(OUTPUT_DIR / 'aggregated.json', 'w') as f:
        json.dump({
            'retrieval': {k: {m: dict(v) for m, v in metrics.items()} for k, metrics in retrieval.items()},
            'ablation': {k: {m: dict(v) for m, v in metrics.items()} for k, metrics in ablation.items()},
            'latency': {k: {m: dict(v) for m, v in ops.items()} for k, ops in latency.items()},
            'persistence': {str(k): v for k, v in persistence.items()}
        }, f, indent=2, default=float)
    print('Saved: aggregated.json')
    
    print('\n✓ All plots and summaries generated!')

if __name__ == '__main__':
    main()
