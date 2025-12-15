#!/bin/bash
# Z.E.T.A. Comprehensive Benchmark Suite
# Tests all features and generates comparison metrics
#
# Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

set -e

# Configuration
MODEL="${MODEL:-/Users/hendrixx./models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf}"
ZETA_DEMO="./bin/llama-zeta-demo"
LLAMA_CLI="./bin/llama-cli"
RESULTS_DIR="./benchmarks/results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_FILE="${RESULTS_DIR}/benchmark_${TIMESTAMP}.csv"

# Test parameters
TOKENS_TO_GENERATE=200
CONTEXT_SIZE=2048
PROMPT="The history of artificial intelligence began in the 1950s when Alan Turing proposed the Turing test. Over the decades, AI has evolved through multiple paradigms including symbolic AI, expert systems, and machine learning. Today, large language models represent the cutting edge of AI research. Please continue this discussion about"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   Z.E.T.A. Benchmark Suite v1.0${NC}"
echo -e "${BLUE}======================================${NC}"

# Create results directory
mkdir -p "${RESULTS_DIR}"

# Initialize CSV
echo "test_name,tokens_generated,total_time_ms,tokens_per_second,decode_time_ms,retrieval_time_ms,retrieval_overhead_pct,memory_blocks,avg_retrieval_score,metal_active" > "${RESULTS_FILE}"

# Function to run a benchmark and extract metrics
run_benchmark() {
    local test_name="$1"
    local extra_args="$2"
    local use_zeta="$3"

    echo -e "\n${YELLOW}Running: ${test_name}${NC}"
    echo "Args: ${extra_args}"

    local output_file="${RESULTS_DIR}/${test_name}_${TIMESTAMP}.log"

    if [ "$use_zeta" = "true" ]; then
        # Run Z.E.T.A. demo
        timeout 300 ${ZETA_DEMO} \
            -m "${MODEL}" \
            -c ${CONTEXT_SIZE} \
            -n ${TOKENS_TO_GENERATE} \
            -p "${PROMPT}" \
            ${extra_args} \
            2>&1 | tee "${output_file}" || true
    else
        # Run baseline llama-cli
        timeout 300 ${LLAMA_CLI} \
            -m "${MODEL}" \
            -c ${CONTEXT_SIZE} \
            -n ${TOKENS_TO_GENERATE} \
            -p "${PROMPT}" \
            ${extra_args} \
            2>&1 | tee "${output_file}" || true
    fi

    # Extract metrics from output
    local tokens_generated=$(grep -o "Total tokens generated: [0-9]*" "${output_file}" | grep -o "[0-9]*" || echo "0")
    local total_time=$(grep -o "Total generation time: *[0-9.]*" "${output_file}" | grep -o "[0-9.]*" || echo "0")
    local tokens_per_sec=$(grep -o "Tokens/second: *[0-9.]*" "${output_file}" | grep -o "[0-9.]*" || echo "0")
    local decode_time=$(grep -o "Total decode time: *[0-9.]*" "${output_file}" | grep -o "[0-9.]*" || echo "0")
    local retrieval_time=$(grep -o "Total retrieval time: *[0-9.]*" "${output_file}" | grep -o "[0-9.]*" || echo "0")
    local retrieval_pct=$(grep -o "Total retrieval time:.*([0-9.]*%)" "${output_file}" | grep -o "[0-9.]*%" | tr -d '%' || echo "0")
    local memory_blocks=$(grep -o "Archived blocks: [0-9]*" "${output_file}" | grep -o "[0-9]*" || echo "0")
    local metal_active=$(grep -q "Metal acceleration:.*ACTIVE" "${output_file}" && echo "true" || echo "false")

    # For baseline, extract from llama-cli format
    if [ "$use_zeta" = "false" ]; then
        tokens_per_sec=$(grep -o "eval time.*tokens per second" "${output_file}" | grep -o "[0-9.]*" | tail -1 || echo "0")
        tokens_generated=${TOKENS_TO_GENERATE}
    fi

    # Write to CSV
    echo "${test_name},${tokens_generated},${total_time},${tokens_per_sec},${decode_time},${retrieval_time},${retrieval_pct},${memory_blocks},0,${metal_active}" >> "${RESULTS_FILE}"

    echo -e "${GREEN}✓ ${test_name}: ${tokens_per_sec} tok/s${NC}"
}

# ============================================================================
# Run Benchmarks
# ============================================================================

echo -e "\n${BLUE}=== Phase 1: Baseline Comparison ===${NC}"

# 1. Baseline llama.cpp (no Z.E.T.A.)
run_benchmark "baseline_llama" "" "false"

# 2. Z.E.T.A. with all features disabled (overhead measurement)
run_benchmark "zeta_disabled" "--zeta-lambda 0 --zeta-tau -1000" "true"

echo -e "\n${BLUE}=== Phase 2: Individual Feature Tests ===${NC}"

# 3. Temporal Decay only
run_benchmark "temporal_decay_only" "--zeta-lambda 0.05 --zeta-tau -1000" "true"

# 4. Sparse Gating only
run_benchmark "sparse_gating_only" "--zeta-lambda 0 --zeta-tau 0.01" "true"

# 5. Memory Retrieval only (low thresholds for decay/gating)
run_benchmark "memory_retrieval_only" "--zeta-lambda 0.001 --zeta-tau -1000 --zeta-retrieve 0.3" "true"

echo -e "\n${BLUE}=== Phase 3: Combined Feature Tests ===${NC}"

# 6. Temporal Decay + Sparse Gating
run_benchmark "decay_plus_gating" "--zeta-lambda 0.05 --zeta-tau 0.01" "true"

# 7. All features (default config)
run_benchmark "all_features_default" "--zeta-lambda 0.01 --zeta-tau 0.01 --zeta-retrieve 0.3" "true"

# 8. All features (aggressive config)
run_benchmark "all_features_aggressive" "--zeta-lambda 0.1 --zeta-tau 0.05 --zeta-retrieve 0.5" "true"

# 9. All features (conservative config)
run_benchmark "all_features_conservative" "--zeta-lambda 0.005 --zeta-tau 0.005 --zeta-retrieve 0.2" "true"

echo -e "\n${BLUE}=== Phase 4: Scaling Tests ===${NC}"

# 10. Longer generation (500 tokens)
TOKENS_TO_GENERATE=500
run_benchmark "long_gen_500tok" "--zeta-lambda 0.01 --zeta-tau 0.01" "true"

# 11. Even longer (1000 tokens) - will trigger sublimation
TOKENS_TO_GENERATE=1000
run_benchmark "long_gen_1000tok" "--zeta-lambda 0.01 --zeta-tau 0.01" "true"

# Reset
TOKENS_TO_GENERATE=200

echo -e "\n${BLUE}=== Phase 5: Context Window Tests ===${NC}"

# 12. Small context (512)
CONTEXT_SIZE=512
run_benchmark "context_512" "--zeta-lambda 0.01 --zeta-tau 0.01" "true"

# 13. Large context (4096)
CONTEXT_SIZE=4096
run_benchmark "context_4096" "--zeta-lambda 0.01 --zeta-tau 0.01" "true"

# Reset
CONTEXT_SIZE=2048

echo -e "\n${GREEN}======================================${NC}"
echo -e "${GREEN}   Benchmark Complete!${NC}"
echo -e "${GREEN}======================================${NC}"
echo -e "Results saved to: ${RESULTS_FILE}"
echo ""

# Generate summary
echo -e "${BLUE}=== Summary ===${NC}"
echo ""
column -t -s',' "${RESULTS_FILE}" | head -20

# Run Python analysis if available
if command -v python3 &> /dev/null; then
    echo -e "\n${BLUE}Generating plots...${NC}"
    python3 "${RESULTS_DIR}/../analyze_benchmark.py" "${RESULTS_FILE}" "${RESULTS_DIR}/plots_${TIMESTAMP}"
fi

echo -e "\n${GREEN}Done!${NC}"
