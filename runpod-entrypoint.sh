#!/bin/bash
# Runpod/Cloud entrypoint - download models from HuggingFace if missing
# This script is designed to work with the default Docker paths (/models)
# and automatically download missing models on first boot.

set -e

# Use /runpod-volume/models if network volume exists, else /models
if [ -d "/runpod-volume" ]; then
    MODELS_DIR="/runpod-volume/models"
    STORAGE_DIR="/runpod-volume/zetazero"
elif [ -d "/workspace" ]; then
    MODELS_DIR="/workspace/models"
    STORAGE_DIR="/workspace/zetazero"
else
    MODELS_DIR="/models"
    STORAGE_DIR="/storage"
fi

echo "[RUNPOD] Models directory: $MODELS_DIR"
echo "[RUNPOD] Storage directory: $STORAGE_DIR"
mkdir -p "$MODELS_DIR" "$STORAGE_DIR"

# Check if all models exist
NEED_14B=0
NEED_7B=0
NEED_EMBED=0

[ ! -f "$MODELS_DIR/qwen2.5-14b-instruct-q4.gguf" ] && NEED_14B=1
[ ! -f "$MODELS_DIR/qwen2.5-7b-coder-q4_k_m.gguf" ] && NEED_7B=1
[ ! -f "$MODELS_DIR/qwen3-embedding-4b-q4_k_m.gguf" ] && NEED_EMBED=1

if [ "$NEED_14B" -eq 1 ] || [ "$NEED_7B" -eq 1 ] || [ "$NEED_EMBED" -eq 1 ]; then
    echo "[RUNPOD] Installing huggingface-cli..."
    pip install -U huggingface_hub --quiet
    
    cd "$MODELS_DIR"
    
    if [ "$NEED_14B" -eq 1 ]; then
        echo "[RUNPOD] Downloading Qwen2.5-14B-Instruct Q4_K_M from HuggingFace..."
        huggingface-cli download Qwen/Qwen2.5-14B-Instruct-GGUF \
            --include "qwen2.5-14b-instruct-q4_k_m*.gguf" \
            --local-dir . --local-dir-use-symlinks False
        # Merge if split and rename
        if [ -f "qwen2.5-14b-instruct-q4_k_m-00001-of-00002.gguf" ]; then
            llama-gguf-split --merge qwen2.5-14b-instruct-q4_k_m-00001-of-00002.gguf qwen2.5-14b-instruct-q4.gguf
            rm -f qwen2.5-14b-instruct-q4_k_m-*.gguf
        else
            mv qwen2.5-14b-instruct-q4_k_m.gguf qwen2.5-14b-instruct-q4.gguf 2>/dev/null || true
        fi
    fi
    
    if [ "$NEED_7B" -eq 1 ]; then
        echo "[RUNPOD] Downloading Qwen2.5-Coder-7B-Instruct Q4_K_M from HuggingFace..."
        huggingface-cli download Qwen/Qwen2.5-Coder-7B-Instruct-GGUF \
            --include "qwen2.5-coder-7b-instruct-q4_k_m*.gguf" \
            --local-dir . --local-dir-use-symlinks False
        # Merge if split and rename
        if [ -f "qwen2.5-coder-7b-instruct-q4_k_m-00001-of-00002.gguf" ]; then
            llama-gguf-split --merge qwen2.5-coder-7b-instruct-q4_k_m-00001-of-00002.gguf qwen2.5-7b-coder-q4_k_m.gguf
            rm -f qwen2.5-coder-7b-instruct-q4_k_m-*.gguf
        else
            mv qwen2.5-coder-7b-instruct-q4_k_m.gguf qwen2.5-7b-coder-q4_k_m.gguf 2>/dev/null || true
        fi
    fi
    
    if [ "$NEED_EMBED" -eq 1 ]; then
        echo "[RUNPOD] Downloading Qwen3-Embedding-4B Q4_K_M from HuggingFace..."
        huggingface-cli download Qwen/Qwen3-Embedding-4B-GGUF \
            --include "qwen3-embedding-4b-q4_k_m.gguf" \
            --local-dir . --local-dir-use-symlinks False
    fi
    
    echo "[RUNPOD] Download complete. Models:"
    ls -lh "$MODELS_DIR/"*.gguf
else
    echo "[RUNPOD] All models present:"
    ls -lh "$MODELS_DIR/"*.gguf
fi

# Ensure config is in a location the server will find
# Server search order: ./zeta.conf, ~/ZetaZero/zeta.conf, /etc/zeta/zeta.conf
mkdir -p /etc/zeta

# Generate config based on detected paths
cat > /etc/zeta/zeta.conf << EOF
# Auto-generated Runpod config
ZETA_HOST="0.0.0.0"
ZETA_PORT="8080"

# Models
MODEL_14B="$MODELS_DIR/qwen2.5-14b-instruct-q4.gguf"
MODEL_7B_CODER="$MODELS_DIR/qwen2.5-7b-coder-q4_k_m.gguf"
MODEL_EMBED="$MODELS_DIR/qwen3-embedding-4b-q4_k_m.gguf"

# Storage
ZETA_STORAGE="$STORAGE_DIR"
ZETA_GKV_DIR="$STORAGE_DIR/graph_kv"
ZETA_DREAM_DIR="$STORAGE_DIR/dreams"
ZETA_LOG="$STORAGE_DIR/logs/zeta.log"

# GPU settings
GPU_LAYERS="999"
CTX_14B="4096"
CTX_7B="4096"
CTX_EMBED="512"
BATCH_SIZE="4096"
EOF

echo "[RUNPOD] Config written to /etc/zeta/zeta.conf"
cat /etc/zeta/zeta.conf

# Start the main entrypoint from /app directory
echo "[RUNPOD] Starting Z.E.T.A. server..."
cd /app
exec /app/docker-entrypoint.sh "$@"
