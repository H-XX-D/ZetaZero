#!/usr/bin/env bash
# switch_architecture.sh - Adapt ZetaZero source code for different model families
#
# Usage: ./scripts/switch_architecture.sh [qwen|llama3|mistral]

set -e

FAMILY="${1:-qwen}"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CODE_MODE_H="$SRC_DIR/llama.cpp/tools/zeta-demo/zeta-code-mode.h"
SERVER_CPP="$SRC_DIR/llama.cpp/tools/zeta-demo/zeta-server.cpp"

echo "=== ZetaZero Architecture Adapter ==="
echo "Target Family: $FAMILY"
echo "Source Dir: $SRC_DIR"
echo ""

# 1. Define Templates
# -------------------

# Qwen (ChatML) - Default
QWEN_PROMPT="<|im_start|>system\\\\n\" \"Extract code entities from the input. Output JSON only.\\\\n\" \"Format: {\\\\\"entities\\\\\":[{\\\\\"type\\\\\":\\\\\"function|class|variable|import\\\\\",\\\\\"name\\\\\":\\\\\"name\\\\\",\\\\\"file\\\\\":\\\\\"path\\\\\",\\\\\"line_start\\\\\":N}]}\\\\n\" \"<|im_end|>\\\\n\" \"<|im_start|>user\\\\n"
QWEN_CTX=32768

# Llama 3 (Official)
LLAMA3_PROMPT="<|begin_of_text|><|start_header_id|>system<|end_header_id|>\\\\n\\\\n\" \"Extract code entities from the input. Output JSON only.\\\\n\" \"Format: {\\\\\"entities\\\\\":[{\\\\\"type\\\\\":\\\\\"function|class|variable|import\\\\\",\\\\\"name\\\\\":\\\\\"name\\\\\",\\\\\"file\\\\\":\\\\\"path\\\\\",\\\\\"line_start\\\\\":N}]}\\\\n\" \"<|eot_id|><|start_header_id|>user<|end_header_id|>\\\\n\\\\n"
LLAMA3_CTX=8192

# Mistral (v1/v2/v3)
MISTRAL_PROMPT="<s>[INST] System: Extract code entities from the input. Output JSON only.\\\\n\" \"Format: {\\\\\"entities\\\\\":[{\\\\\"type\\\\\":\\\\\"function|class|variable|import\\\\\",\\\\\"name\\\\\":\\\\\"name\\\\\",\\\\\"file\\\\\":\\\\\"path\\\\\",\\\\\"line_start\\\\\":N}]}\\\\n\\\\n\" \"User: "
MISTRAL_CTX=32768

# 2. Select Configuration
# -----------------------
case "$FAMILY" in
    qwen)
        NEW_PROMPT="$QWEN_PROMPT"
        NEW_CTX="$QWEN_CTX"
        ;;
    llama3)
        NEW_PROMPT="$LLAMA3_PROMPT"
        NEW_CTX="$LLAMA3_CTX"
        ;;
    mistral)
        NEW_PROMPT="$MISTRAL_PROMPT"
        NEW_CTX="$MISTRAL_CTX"
        ;;
    *)
        echo "ERROR: Unknown family '$FAMILY'. Supported: qwen, llama3, mistral"
        exit 1
        ;;
esac

# 3. Patch Source Code
# --------------------

echo "[PATCH] Updating Prompt Template in zeta-code-mode.h..."
# We use a marker comment to find the line to replace
# Original: #define ZETA_CODER_EXTRACTION_PROMPT "..."
sed -i "s|^#define ZETA_CODER_EXTRACTION_PROMPT .*|#define ZETA_CODER_EXTRACTION_PROMPT \"$NEW_PROMPT\"|" "$CODE_MODE_H"

echo "[PATCH] Updating Default Context in zeta-server.cpp..."
# Original: cparams.n_ctx = 4096;
sed -i "s|cparams.n_ctx = [0-9]*;|cparams.n_ctx = $NEW_CTX;|" "$SERVER_CPP"

# 4. Recompile
# ------------
echo "[BUILD] Recompiling llama-zeta-server..."
cd "$SRC_DIR/llama.cpp/build"
make -j$(nproc) llama-zeta-server

echo ""
echo "=== Success ==="
echo "ZetaZero has been adapted for $FAMILY."
echo "Please ensure you update your 'launch_hippocampus.sh' to point to the correct $FAMILY GGUF models."
