#!/bin/bash
#
# Z.E.T.A. Complete Benchmark Suite
# Runs all benchmarks on Z6 workstation with CUDA backend
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results"
LLAMA_CPP_DIR="${LLAMA_CPP_DIR:-$HOME/llama.cpp/build/bin}"

# Model paths (adjust for your setup)
MODEL_3B="${MODEL_3B:-$HOME/models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf}"
MODEL_14B="${MODEL_14B:-$HOME/models/qwen2.5-14b-instruct-q4_k_m.gguf}"

# Ports
PORT_3B=9001
PORT_14B=9000

echo "=============================================="
echo "Z.E.T.A. Benchmark Suite"
echo "=============================================="
echo "Date: $(date)"
echo "Host: $(hostname)"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Function to check if server is running
check_server() {
    local port=$1
    curl -s "http://localhost:$port/health" > /dev/null 2>&1
}

# Function to start server
start_server() {
    local model=$1
    local port=$2
    local name=$3

    if check_server $port; then
        echo "[OK] $name server already running on port $port"
        return 0
    fi

    if [[ ! -f "$model" ]]; then
        echo "[ERROR] Model not found: $model"
        return 1
    fi

    echo "[START] Starting $name server on port $port..."
    "$LLAMA_CPP_DIR/llama-server" \
        -m "$model" \
        --port $port \
        -ngl 99 \
        -c 8192 \
        > "$RESULTS_DIR/${name}_server.log" 2>&1 &

    # Wait for server to be ready
    for i in {1..30}; do
        if check_server $port; then
            echo "[OK] $name server ready"
            return 0
        fi
        sleep 1
    done

    echo "[ERROR] $name server failed to start"
    return 1
}

# Function to stop servers
stop_servers() {
    echo "[STOP] Stopping servers..."
    pkill -f "llama-server" 2>/dev/null || true
    sleep 2
}

# Parse arguments
RUN_ALL=true
RUN_RETRIEVAL=false
RUN_LATENCY=false
RUN_PERSISTENCE=false
RUN_ABLATION=false
RUN_RESOURCES=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --retrieval) RUN_ALL=false; RUN_RETRIEVAL=true ;;
        --latency) RUN_ALL=false; RUN_LATENCY=true ;;
        --persistence) RUN_ALL=false; RUN_PERSISTENCE=true ;;
        --ablation) RUN_ALL=false; RUN_ABLATION=true ;;
        --resources) RUN_ALL=false; RUN_RESOURCES=true ;;
        --stop) stop_servers; exit 0 ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --retrieval    Run retrieval benchmark only"
            echo "  --latency      Run latency benchmark only"
            echo "  --persistence  Run persistence benchmark only"
            echo "  --ablation     Run ablation benchmark only"
            echo "  --resources    Run resources benchmark only"
            echo "  --stop         Stop all servers"
            echo ""
            echo "Environment variables:"
            echo "  LLAMA_CPP_DIR  Path to llama.cpp binaries"
            echo "  MODEL_3B       Path to 3B model file"
            echo "  MODEL_14B      Path to 14B model file"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

# Hardware info
echo "=============================================="
echo "Hardware Info"
echo "=============================================="
nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader 2>/dev/null || echo "nvidia-smi not available"
echo ""

# Start servers
echo "=============================================="
echo "Starting Servers"
echo "=============================================="

# 14B is always needed
if ! start_server "$MODEL_14B" $PORT_14B "14B"; then
    echo "[WARN] 14B server not available - some tests will be skipped"
fi

# 3B needed for SubZero/Zero
if ! start_server "$MODEL_3B" $PORT_3B "3B"; then
    echo "[WARN] 3B server not available - SubZero/Zero tests will be skipped"
fi

echo ""

# Run benchmarks
cd "$SCRIPT_DIR"

if $RUN_ALL || $RUN_RETRIEVAL; then
    echo "=============================================="
    echo "Running Retrieval Benchmark (Section 6.3)"
    echo "=============================================="
    python3 benchmark_retrieval.py -o "$RESULTS_DIR/retrieval.json"
    echo ""
fi

if $RUN_ALL || $RUN_LATENCY; then
    echo "=============================================="
    echo "Running Latency Benchmark (Section 6.5)"
    echo "=============================================="
    python3 benchmark_latency.py -o "$RESULTS_DIR/latency.json" --runs 50
    echo ""
fi

if $RUN_ALL || $RUN_PERSISTENCE; then
    echo "=============================================="
    echo "Running Persistence Benchmark (Section 6.6)"
    echo "=============================================="
    python3 benchmark_persistence.py -o "$RESULTS_DIR/persistence.json"
    echo ""
fi

if $RUN_ALL || $RUN_ABLATION; then
    echo "=============================================="
    echo "Running Ablation Study (Section 7.1)"
    echo "=============================================="
    python3 benchmark_ablation.py -o "$RESULTS_DIR/ablation.json"
    echo ""
fi

if $RUN_ALL || $RUN_RESOURCES; then
    echo "=============================================="
    echo "Running Resources Benchmark (Section 6.4)"
    echo "=============================================="
    python3 benchmark_resources.py -o "$RESULTS_DIR/resources.json"
    echo ""
fi

# Generate paper tables
echo "=============================================="
echo "Generating Paper Tables"
echo "=============================================="
python3 generate_paper_tables.py -d "$RESULTS_DIR"

echo ""
echo "=============================================="
echo "Benchmark Complete!"
echo "=============================================="
echo "Results saved to: $RESULTS_DIR/"
echo ""
ls -la "$RESULTS_DIR/"
