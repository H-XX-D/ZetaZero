#!/usr/bin/env bash
# Launch Z.E.T.A. server with embedding model (Hippocampus) - TEST VERSION

pkill -f llama-zeta-server 2>/dev/null || true
sleep 2

SERVER="$HOME/ZetaZero/llama.cpp/build/bin/llama-zeta-server"
MODEL_14B="$HOME/models/qwen2.5-14b-instruct-q4.gguf"
MODEL_3B="$HOME/models/qwen2.5-3b-instruct-q4_k_m.gguf"
MODEL_EMBED="$HOME/models/bge-small-en-v1.5-q8_0.gguf"
STORAGE="$HOME/HoloGit/blocks"

echo "Starting Z.E.T.A. with Hippocampus..."
echo "Server: $SERVER"
echo "Storage: $STORAGE"

export CUDA_VISIBLE_DEVICES=0

nohup "$SERVER" \
    -m "$MODEL_14B" \
    --model-3b "$MODEL_3B" \
    --embed-model "$MODEL_EMBED" \
    --port 9000 \
    --storage "$STORAGE" \
    --ctx-size 4096 \
    --n-gpu-layers 99 \
    > /tmp/zeta_embed.log 2>&1 &

echo "Server launched. Waiting for startup..."
sleep 5
tail -n 20 /tmp/zeta_embed.log
