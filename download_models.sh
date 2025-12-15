#!/usr/bin/env bash
# download_models.sh - Download required GGUF models for ZetaZero
#
# This script downloads the specific quantized models required by launch_hippocampus.sh
# and places them in $HOME/models/

set -euo pipefail

MODEL_DIR="$HOME/models"
mkdir -p "$MODEL_DIR"

echo "=== ZetaZero Model Downloader ==="
echo "Target Directory: $MODEL_DIR"
echo ""

# 1. Qwen 2.5 14B Instruct (The Conscious Mind)
# We use Q4_K_M quantization for a balance of speed and quality.
# Note: launch_hippocampus.sh expects 'qwen2.5-14b-instruct-q4.gguf'
URL_14B="https://huggingface.co/Qwen/Qwen2.5-14B-Instruct-GGUF/resolve/main/qwen2.5-14b-instruct-q4_k_m.gguf"
FILE_14B="qwen2.5-14b-instruct-q4.gguf"

if [ -f "$MODEL_DIR/$FILE_14B" ]; then
    echo "[SKIP] 14B Model already exists: $FILE_14B"
else
    echo "[DOWNLOADING] Qwen 2.5 14B Instruct (Q4_K_M)..."
    wget -O "$MODEL_DIR/$FILE_14B" "$URL_14B" --show-progress
fi

# 2. Qwen 2.5 3B Instruct (The Subconscious / Coder)
# We use Q4_K_M quantization.
URL_3B="https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf"
FILE_3B="qwen2.5-3b-instruct-q4_k_m.gguf"

if [ -f "$MODEL_DIR/$FILE_3B" ]; then
    echo "[SKIP] 3B Model already exists: $FILE_3B"
else
    echo "[DOWNLOADING] Qwen 2.5 3B Instruct (Q4_K_M)..."
    wget -O "$MODEL_DIR/$FILE_3B" "$URL_3B" --show-progress
fi

# 3. BGE Small v1.5 (The Hippocampus / Embedding)
# We use Q8_0 for maximum precision in retrieval (it's a small model, so Q8 is cheap).
URL_EMBED="https://huggingface.co/lmstudio-community/bge-small-en-v1.5-GGUF/resolve/main/bge-small-en-v1.5-q8_0.gguf"
FILE_EMBED="bge-small-en-v1.5-q8_0.gguf"

if [ -f "$MODEL_DIR/$FILE_EMBED" ]; then
    echo "[SKIP] Embedding Model already exists: $FILE_EMBED"
else
    echo "[DOWNLOADING] BGE Small v1.5 (Q8_0)..."
    wget -O "$MODEL_DIR/$FILE_EMBED" "$URL_EMBED" --show-progress
fi

echo ""
echo "=== Download Complete ==="
echo "You can now run: ./launch_hippocampus.sh"
