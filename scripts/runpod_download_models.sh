#!/bin/bash
# Download models directly from HuggingFace to Runpod
# Run this inside the Runpod container/pod

set -e

MODELS_DIR="${1:-/workspace/models}"
mkdir -p "$MODELS_DIR"
cd "$MODELS_DIR"

echo "=== Downloading models to $MODELS_DIR ==="
echo ""

# Install huggingface-cli if needed
if ! command -v huggingface-cli &> /dev/null; then
    pip install -U huggingface_hub
fi

echo "[1/3] Downloading Qwen2.5-14B-Instruct Q4_K_M..."
huggingface-cli download Qwen/Qwen2.5-14B-Instruct-GGUF \
    --include "qwen2.5-14b-instruct-q4_k_m*.gguf" \
    --local-dir . --local-dir-use-symlinks False

# Merge if split
if [ -f "qwen2.5-14b-instruct-q4_k_m-00001-of-00002.gguf" ]; then
    echo "  Merging split files..."
    llama-gguf-split --merge qwen2.5-14b-instruct-q4_k_m-00001-of-00002.gguf qwen2.5-14b-instruct-q4_k_m.gguf
    rm -f qwen2.5-14b-instruct-q4_k_m-*.gguf
fi

# Rename to match config
mv qwen2.5-14b-instruct-q4_k_m.gguf qwen2.5-14b-instruct-q4.gguf 2>/dev/null || true

echo "[2/3] Downloading Qwen2.5-Coder-7B-Instruct Q4_K_M..."
huggingface-cli download Qwen/Qwen2.5-Coder-7B-Instruct-GGUF \
    --include "qwen2.5-coder-7b-instruct-q4_k_m*.gguf" \
    --local-dir . --local-dir-use-symlinks False

# Merge if split
if [ -f "qwen2.5-coder-7b-instruct-q4_k_m-00001-of-00002.gguf" ]; then
    echo "  Merging split files..."
    llama-gguf-split --merge qwen2.5-coder-7b-instruct-q4_k_m-00001-of-00002.gguf qwen2.5-coder-7b-instruct-q4_k_m.gguf
    rm -f qwen2.5-coder-7b-instruct-q4_k_m-*.gguf
fi

# Rename to match config
mv qwen2.5-coder-7b-instruct-q4_k_m.gguf qwen2.5-7b-coder-q4_k_m.gguf 2>/dev/null || true

echo "[3/3] Downloading Qwen3-Embedding-4B Q4_K_M..."
huggingface-cli download Qwen/Qwen3-Embedding-4B-GGUF \
    --include "qwen3-embedding-4b-q4_k_m.gguf" \
    --local-dir . --local-dir-use-symlinks False

echo ""
echo "=== Download complete! ==="
ls -lh "$MODELS_DIR"/*.gguf
