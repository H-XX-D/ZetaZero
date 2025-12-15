#!/bin/bash
# Z.E.T.A. Quick Benchmark - Fast comparison test
# Runs a subset of benchmarks for quick validation
#
# Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

set -e

cd "$(dirname "$0")/.."

# Configuration
MODEL="${MODEL:-/Users/hendrixx./ZetaZero/zzv3/Models/tinyllama-zeta.gguf}"
ZETA_DEMO="./build/bin/llama-zeta-demo"
RESULTS_DIR="./benchmarks/results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
CSV_FILE="${RESULTS_DIR}/quick_${TIMESTAMP}.csv"

TOKENS=100
PROMPT="Explain the theory of relativity in simple terms:"

mkdir -p "${RESULTS_DIR}"

echo "========================================"
echo "   Z.E.T.A. Quick Benchmark"
echo "========================================"
echo "Model: ${MODEL}"
echo "Tokens: ${TOKENS}"
echo ""

# CSV header
echo "config,tokens_per_second,decode_ms,retrieval_ms,overhead_pct" > "${CSV_FILE}"

run_test() {
    local name="$1"
    local args="$2"

    echo -n "Testing ${name}... "

    local output=$(timeout 120 ${ZETA_DEMO} \
        -m "${MODEL}" \
        -c 2048 \
        -n ${TOKENS} \
        -p "${PROMPT}" \
        ${args} 2>&1)

    local tps=$(echo "$output" | grep -o "Tokens/second: *[0-9.]*" | grep -o "[0-9.]*" || echo "0")
    local decode=$(echo "$output" | grep -o "Total decode time: *[0-9.]*" | grep -o "[0-9.]*" || echo "0")
    local retrieval=$(echo "$output" | grep -o "Total retrieval time: *[0-9.]*" | grep -o "[0-9.]*" || echo "0")
    local overhead=$(echo "$output" | grep -o "retrieval time:.*([0-9.]*%)" | grep -o "[0-9.]*%" | tr -d '%' || echo "0")

    echo "${tps} tok/s (overhead: ${overhead}%)"
    echo "${name},${tps},${decode},${retrieval},${overhead}" >> "${CSV_FILE}"
}

echo ""
echo "=== Running Tests ==="

# Core feature tests
run_test "zeta_off" "--zeta-lambda 0 --zeta-tau -1000"
run_test "temporal_only" "--zeta-lambda 0.05 --zeta-tau -1000"
run_test "sparse_only" "--zeta-lambda 0 --zeta-tau 0.01"
run_test "all_default" "--zeta-lambda 0.01 --zeta-tau 0.01 --zeta-retrieve 0.3"
run_test "all_aggressive" "--zeta-lambda 0.1 --zeta-tau 0.05"

echo ""
echo "=== Results ==="
column -t -s',' "${CSV_FILE}"

echo ""
echo "=== Quick Analysis ==="

# Simple analysis
baseline=$(grep "zeta_off" "${CSV_FILE}" | cut -d',' -f2)
allfeatures=$(grep "all_default" "${CSV_FILE}" | cut -d',' -f2)

if [ -n "$baseline" ] && [ -n "$allfeatures" ]; then
    if command -v bc &> /dev/null; then
        pct=$(echo "scale=2; (${allfeatures}/${baseline})*100" | bc)
        echo "All features vs disabled: ${pct}% of baseline throughput"
    fi
fi

echo ""
echo "Results saved to: ${CSV_FILE}"

# Generate plot if matplotlib available
if command -v python3 &> /dev/null; then
    python3 - "${CSV_FILE}" "${RESULTS_DIR}" << 'PYEOF'
import sys
import pandas as pd
import matplotlib.pyplot as plt

try:
    csv_file = sys.argv[1]
    output_dir = sys.argv[2]

    df = pd.read_csv(csv_file)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Throughput comparison
    colors = ['#2196F3', '#4CAF50', '#FF9800', '#F44336', '#9C27B0']
    ax1.bar(df['config'], df['tokens_per_second'], color=colors[:len(df)], edgecolor='black')
    ax1.set_ylabel('Tokens/Second')
    ax1.set_title('Z.E.T.A. Throughput Comparison')
    ax1.tick_params(axis='x', rotation=45)

    for i, v in enumerate(df['tokens_per_second']):
        ax1.text(i, v + 0.5, f'{v:.1f}', ha='center', fontsize=9)

    # Overhead
    ax2.bar(df['config'], df['overhead_pct'], color='#E91E63', edgecolor='black')
    ax2.set_ylabel('Overhead (%)')
    ax2.set_title('Z.E.T.A. Retrieval Overhead')
    ax2.tick_params(axis='x', rotation=45)
    ax2.axhline(y=1.0, color='green', linestyle='--', alpha=0.7, label='1% threshold')
    ax2.legend()

    plt.tight_layout()
    plt.savefig(f'{output_dir}/quick_benchmark.png', dpi=150, bbox_inches='tight')
    print(f"Plot saved to: {output_dir}/quick_benchmark.png")
except Exception as e:
    print(f"Plot generation skipped: {e}")
PYEOF
fi

echo ""
echo "Done!"
