#!/bin/bash
# Z.E.T.A. Setup Script
# Downloads models and prepares the environment

set -e

MODELS_DIR="${MODELS_DIR:-$HOME/models}"
STORAGE_DIR="${STORAGE_DIR:-$HOME/.zetazero/storage}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "  Z.E.T.A. Setup"
echo "=========================================="

# Create directories
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$MODELS_DIR"
mkdir -p "$STORAGE_DIR/dreams/pending"
mkdir -p "$STORAGE_DIR/dreams/archive"
mkdir -p "$STORAGE_DIR/blocks"

# Check for huggingface-cli
if ! command -v huggingface-cli &> /dev/null; then
    echo -e "${YELLOW}Installing huggingface-cli...${NC}"
    pip3 install -U huggingface_hub
fi

# Model selection
echo ""
echo "Select model configuration:"
echo "  1) Production (14B + 7B) - Requires 16GB+ VRAM"
echo "  2) Lite (7B + 3B) - Requires 8GB+ VRAM"
echo "  3) Minimal (3B + 3B) - Requires 6GB+ VRAM"
echo ""
read -p "Choice [1]: " MODEL_CHOICE
MODEL_CHOICE=${MODEL_CHOICE:-1}

cd "$MODELS_DIR"

# Download embedding model (always needed)
echo -e "${GREEN}Downloading embedding model...${NC}"
if [ ! -f "nomic-embed-text-v1.5.f16.gguf" ]; then
    huggingface-cli download nomic-ai/nomic-embed-text-v1.5-GGUF nomic-embed-text-v1.5.f16.gguf --local-dir .
else
    echo "  Already exists, skipping."
fi

case $MODEL_CHOICE in
    1)
        echo -e "${GREEN}Downloading 14B main model...${NC}"
        if [ ! -f "qwen2.5-14b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-14B-Instruct-GGUF qwen2.5-14b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        echo -e "${GREEN}Downloading 7B coder model...${NC}"
        if [ ! -f "qwen2.5-coder-7b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-Coder-7B-Instruct-GGUF qwen2.5-coder-7b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        export MODEL_MAIN="$MODELS_DIR/qwen2.5-14b-instruct-q4_k_m.gguf"
        export MODEL_CODER="$MODELS_DIR/qwen2.5-coder-7b-instruct-q4_k_m.gguf"
        ;;
    2)
        echo -e "${GREEN}Downloading 7B main model...${NC}"
        if [ ! -f "qwen2.5-7b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-7B-Instruct-GGUF qwen2.5-7b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        echo -e "${GREEN}Downloading 3B coder model...${NC}"
        if [ ! -f "qwen2.5-coder-3b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-Coder-3B-Instruct-GGUF qwen2.5-coder-3b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        export MODEL_MAIN="$MODELS_DIR/qwen2.5-7b-instruct-q4_k_m.gguf"
        export MODEL_CODER="$MODELS_DIR/qwen2.5-coder-3b-instruct-q4_k_m.gguf"
        ;;
    3)
        echo -e "${GREEN}Downloading 3B main model...${NC}"
        if [ ! -f "qwen2.5-3b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-3B-Instruct-GGUF qwen2.5-3b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        echo -e "${GREEN}Downloading 3B coder model...${NC}"
        if [ ! -f "qwen2.5-coder-3b-instruct-q4_k_m.gguf" ]; then
            huggingface-cli download Qwen/Qwen2.5-Coder-3B-Instruct-GGUF qwen2.5-coder-3b-instruct-q4_k_m.gguf --local-dir .
        else
            echo "  Already exists, skipping."
        fi

        export MODEL_MAIN="$MODELS_DIR/qwen2.5-3b-instruct-q4_k_m.gguf"
        export MODEL_CODER="$MODELS_DIR/qwen2.5-coder-3b-instruct-q4_k_m.gguf"
        ;;
esac

export MODEL_EMBED="$MODELS_DIR/nomic-embed-text-v1.5.f16.gguf"

echo ""
echo -e "${GREEN}=========================================="
echo "  Setup Complete!"
echo "==========================================${NC}"
echo ""
echo "Models downloaded to: $MODELS_DIR"
echo "Storage directory: $STORAGE_DIR"
echo ""
echo "To run with Docker:"
echo "  docker-compose up -d"
echo ""
echo "To index your codebase (optional):"
echo "  python3 scripts/index_codebase.py --path ./your-code"
echo ""
