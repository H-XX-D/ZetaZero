#!/bin/bash
#
# Start llama.cpp servers for Z.E.T.A. benchmarking
# Run this on the Z6 workstation before running benchmarks
#

set -e

LLAMA_CPP_DIR="${LLAMA_CPP_DIR:-$HOME/llama.cpp/build/bin}"
MODELS_DIR="${MODELS_DIR:-$HOME/models}"

# Model files (adjust paths as needed)
MODEL_3B="${MODEL_3B:-$MODELS_DIR/tinyllama-1.1b-chat-v1.0.Q4_0.gguf}"
MODEL_14B="${MODEL_14B:-$MODELS_DIR/qwen2.5-14b-instruct-q4_k_m.gguf}"

# Server ports
PORT_3B=9001
PORT_14B=9000

# GPU settings
GPU_LAYERS=99
CONTEXT_SIZE_3B=4096
CONTEXT_SIZE_14B=8192

echo "=============================================="
echo "Z.E.T.A. llama.cpp Server Launcher"
echo "=============================================="
echo ""

# Check for llama-server
if [[ ! -f "$LLAMA_CPP_DIR/llama-server" ]]; then
    echo "[ERROR] llama-server not found at: $LLAMA_CPP_DIR/llama-server"
    echo ""
    echo "Build llama.cpp with CUDA support:"
    echo "  cd ~/llama.cpp"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DGGML_CUDA=ON"
    echo "  cmake --build . --config Release -j"
    exit 1
fi

# Check models
check_model() {
    local model=$1
    local name=$2
    if [[ ! -f "$model" ]]; then
        echo "[WARN] $name model not found: $model"
        return 1
    fi
    echo "[OK] $name model: $model"
    return 0
}

echo "Checking models..."
HAS_3B=false
HAS_14B=false

if check_model "$MODEL_3B" "3B"; then
    HAS_3B=true
fi

if check_model "$MODEL_14B" "14B"; then
    HAS_14B=true
fi

if ! $HAS_3B && ! $HAS_14B; then
    echo ""
    echo "[ERROR] No models found. Please download models to $MODELS_DIR"
    exit 1
fi

echo ""

# Start 14B server (main model)
if $HAS_14B; then
    echo "[START] Starting 14B server on port $PORT_14B..."
    "$LLAMA_CPP_DIR/llama-server" \
        -m "$MODEL_14B" \
        --port $PORT_14B \
        -ngl $GPU_LAYERS \
        -c $CONTEXT_SIZE_14B \
        --host 0.0.0.0 \
        > /tmp/llama_14b.log 2>&1 &

    echo "         PID: $!"
    echo "         Log: /tmp/llama_14b.log"
fi

# Start 3B server (subconscious)
if $HAS_3B; then
    echo "[START] Starting 3B server on port $PORT_3B..."
    "$LLAMA_CPP_DIR/llama-server" \
        -m "$MODEL_3B" \
        --port $PORT_3B \
        -ngl $GPU_LAYERS \
        -c $CONTEXT_SIZE_3B \
        --host 0.0.0.0 \
        > /tmp/llama_3b.log 2>&1 &

    echo "         PID: $!"
    echo "         Log: /tmp/llama_3b.log"
fi

# Wait for servers to start
echo ""
echo "Waiting for servers to start..."
sleep 5

# Check health
check_health() {
    local port=$1
    local name=$2
    if curl -s "http://localhost:$port/health" > /dev/null 2>&1; then
        echo "[OK] $name server ready on port $port"
        return 0
    else
        echo "[FAIL] $name server not responding on port $port"
        return 1
    fi
}

echo ""
if $HAS_14B; then
    check_health $PORT_14B "14B"
fi
if $HAS_3B; then
    check_health $PORT_3B "3B"
fi

echo ""
echo "=============================================="
echo "Servers running. To stop: pkill -f llama-server"
echo "=============================================="
