#!/bin/bash
# Z.E.T.A. Server Startup Script
# Default model paths for z6 (RTX 5060 Ti 16GB)

# Model paths
MODEL_14B="/home/xx/models/qwen2.5-14b-instruct-q4.gguf"
MODEL_7B_CODER="/home/xx/models/qwen2.5-7b-coder-q4_k_m.gguf"
MODEL_EMBED="/home/xx/models/qwen3-embedding-4b-q4_k_m.gguf"

# Optional specialist models
MODEL_3B_INSTRUCT="/home/xx/models/qwen2.5-3b-instruct-q4_k_m.gguf"
MODEL_3B_CODER="/home/xx/models/qwen2.5-3b-coder-q4_k_m.gguf"
MODEL_CRITIC="/home/xx/models/qwen2.5-1.5b-critic-q6_k.gguf"
MODEL_ROUTER="/home/xx/models/qwen2.5-0.5b-router-q6_k.gguf"
MODEL_IMMUNE="/home/xx/models/qwen2.5-0.5b-immune-q6_k.gguf"
MODEL_TOOLS="/home/xx/models/qwen2.5-0.5b-tools-q6_k.gguf"

# Storage
STORAGE_DIR="/mnt/HoloGit/blocks"

# Server settings
PORT=8080
GPU_LAYERS=99
CTX_14B=4096
CTX_3B=1024

# Build directory
BUILD_DIR="/home/xx/ZetaZero/llama.cpp/build"

cd "$BUILD_DIR" || exit 1

# Check if server already running
if pgrep -x zeta-server > /dev/null; then
    echo "Server already running. Kill it first:"
    echo "  pkill zeta-server"
    exit 1
fi

# Start server
echo "Starting Z.E.T.A. Server..."
echo "  14B: $MODEL_14B"
echo "  7B Coder: $MODEL_7B_CODER"
echo "  Embed: $MODEL_EMBED"
echo "  Port: $PORT"

nohup ./bin/zeta-server \
    --host 0.0.0.0 \
    --port "$PORT" \
    -m "$MODEL_14B" \
    --model-7b-coder "$MODEL_7B_CODER" \
    --embed-model "$MODEL_EMBED" \
    --gpu-layers "$GPU_LAYERS" \
    --ctx-size "$CTX_14B" \
    --zeta-storage "$STORAGE_DIR" \
    > /tmp/zeta.log 2>&1 &

echo "Server starting... check /tmp/zeta.log"
sleep 5

# Health check
if curl -s "http://localhost:$PORT/health" | grep -q "ok"; then
    echo "Server is UP"
    curl -s "http://localhost:$PORT/health" | python3 -m json.tool 2>/dev/null || cat
else
    echo "Server may still be loading. Check:"
    echo "  tail -f /tmp/zeta.log"
fi
