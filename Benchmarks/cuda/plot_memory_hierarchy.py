import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

fig, ax = plt.subplots(figsize=(10, 8))

# Three layer hierarchy
layers = [
    ('L0: VRAM (16GB)', 'Model Weights + Active KV', '~0.01ms', '#E74C3C', 0.8),
    ('L1: RAM (32GB)', 'Entity Graph + Recent Blocks', '~0.1ms', '#3498DB', 0.6),
    ('L2: NVMe (4TB)', 'Archived Blocks + Facts', '~1ms', '#2ECC71', 0.4),
]

y_positions = [0.7, 0.45, 0.2]
widths = [0.5, 0.6, 0.7]

for i, (name, content, latency, color, width) in enumerate(layers):
    y = y_positions[i]
    w = widths[i]
    rect = mpatches.FancyBboxPatch(
        (0.5 - w/2, y - 0.08), w, 0.16,
        boxstyle=round,pad=0.02,rounding_size=0.02,
        facecolor=color, edgecolor='black', linewidth=2, alpha=0.8
    )
    ax.add_patch(rect)
    ax.text(0.5, y + 0.02, name, ha='center', va='center', fontsize=14, fontweight='bold', color='white')
    ax.text(0.5, y - 0.03, content, ha='center', va='center', fontsize=10, color='white')
    ax.text(0.9, y, latency, ha='left', va='center', fontsize=12, fontweight='bold')

# Arrows
for i in range(len(layers)-1):
    ax.annotate('', xy=(0.5, y_positions[i+1] + 0.08), xytext=(0.5, y_positions[i] - 0.08),
                arrowprops=dict(arrowstyle='->', color='gray', lw=2))

# Labels
ax.text(0.02, 0.7, 'Fastest\n(ns)', ha='left', va='center', fontsize=10, color='gray')
ax.text(0.02, 0.2, 'Largest\n(TB)', ha='left', va='center', fontsize=10, color='gray')

ax.text(0.5, 0.92, 'Z.E.T.A. Memory Hierarchy', ha='center', va='center', fontsize=16, fontweight='bold')

ax.set_xlim(0, 1)
ax.set_ylim(0, 1)
ax.axis('off')

plt.tight_layout()
plt.savefig('/home/xx/ZetaZero/Benchmarks/cuda/results/plots/memory_hierarchy.png', dpi=150, bbox_inches='tight', facecolor='white')
plt.savefig('/home/xx/ZetaZero/Benchmarks/cuda/results/plots/memory_hierarchy.pdf', bbox_inches='tight')
print('Saved memory hierarchy diagram!')
