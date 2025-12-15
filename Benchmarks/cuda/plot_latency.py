import matplotlib.pyplot as plt
import json
import numpy as np

# Load fixed latency data
with open('/home/xx/ZetaZero/Benchmarks/cuda/results/latency_fixed.json') as f:
    data = json.load(f)

fig, axes = plt.subplots(1, 2, figsize=(14, 5))

# Plot 1: Pipeline Breakdown
ax1 = axes[0]
sz = data['profiles']['subzero']
components = ['Entity\nExtract', 'Graph\nWalk', 'Fact\nRetrieval', '3B\nGeneration']
values = [
    sz['entity_extraction']['mean_ms'],
    sz['graph_walk']['mean_ms'],
    sz['fact_retrieval']['mean_ms'],
    sz['generation_3b']['mean_ms']
]
stds = [
    sz['entity_extraction']['std_ms'],
    sz['graph_walk']['std_ms'],
    sz['fact_retrieval']['std_ms'],
    sz['generation_3b']['std_ms']
]

colors = ['#5DADE2', '#48C9B0', '#F7DC6F', '#E74C3C']
bars = ax1.bar(components, values, color=colors, yerr=stds, capsize=5)
ax1.set_ylabel('Time (ms)')
ax1.set_title('SubZero Pipeline Breakdown')

for bar, val in zip(bars, values):
    ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 20, 
             f'{val:.1f}', ha='center', va='bottom')

# Plot 2: Total Latency Comparison  
ax2 = axes[1]
configs = ['SubZero\n(3B)', 'Zero\n(14B)', 'Baseline\n(14B only)']
totals = [
    data['profiles']['subzero']['total_subzero']['mean_ms'],
    data['profiles']['zero']['total_zero']['mean_ms'],
    data['profiles']['baseline']['generation_14b']['mean_ms']
]
total_stds = [
    data['profiles']['subzero']['total_subzero']['std_ms'],
    data['profiles']['zero']['total_zero']['std_ms'],
    data['profiles']['baseline']['generation_14b']['std_ms']
]

colors2 = ['#E74C3C', '#9B59B6', '#7F8C8D']
bars2 = ax2.bar(configs, totals, color=colors2, yerr=total_stds, capsize=5)
ax2.set_ylabel('Time (ms)')
ax2.set_title('Total Latency Comparison')

for bar, val in zip(bars2, totals):
    ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50,
             f'{val:.0f}ms', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('/home/xx/ZetaZero/Benchmarks/cuda/results/plots/latency_breakdown.png', dpi=150, bbox_inches='tight')
plt.savefig('/home/xx/ZetaZero/Benchmarks/cuda/results/plots/latency_breakdown.pdf', bbox_inches='tight')
print('Saved latency plots!')
