#!/usr/bin/env bash
# Launch Z.E.T.A. server with embedding model (Hippocampus)

pkill -f llama-zeta-server 2>/dev/null || true
sleep 2

rm -f /mnt/HoloGit/blocks/graph.bin

SERVER="$HOME/ZetaZero/llama.cpp/build/bin/llama-zeta-server"
MODEL_14B="$HOME/models/qwen2.5-14b-instruct-q4.gguf"
MODEL_3B="$HOME/models/qwen2.5-3b-instruct-q4_k_m.gguf"
MODEL_EMBED="$HOME/models/bge-small-en-v1.5-q8_0.gguf"
STORAGE="/mnt/HoloGit/blocks"

echo "Starting Z.E.T.A. with Hippocampus..."
nohup "$SERVER" \
    -m "$MODEL_14B" \
    --model-3b "$MODEL_3B" \
    --model-embed "$MODEL_EMBED" \
    --port 9000 \
    --storage "$STORAGE" \
    > /tmp/zeta_embed.log 2>&1 &

sleep 10
echo "--- Server Log ---"
tail -100 /tmp/zeta_embed.log
