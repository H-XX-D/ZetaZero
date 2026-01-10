#!/bin/bash
set -e

echo "=========================================="
echo "  Z.E.T.A. Node Starting..."
echo "=========================================="

# Check for zeta.conf - if present, server reads it automatically
if [ -f "/app/zeta.conf" ]; then
    echo "Config: /app/zeta.conf (mounted)"
    # Source it to set environment variables
    set -a  # Export all variables
    source /app/zeta.conf 2>/dev/null || true
    set +a
elif [ -f "./zeta.conf" ]; then
    echo "Config: ./zeta.conf"
    set -a  # Export all variables
    source ./zeta.conf 2>/dev/null || true
    set +a
else
    echo "Config: Environment variables (no zeta.conf found)"
fi

# Show active configuration
echo "Host: ${ZETA_HOST:-0.0.0.0}"
echo "Port: ${ZETA_PORT:-8080}"
echo "Model 14B: ${MODEL_14B:-$MODEL_MAIN}"
echo "Model 7B: ${MODEL_7B_CODER:-$MODEL_CODER}"
echo "Model Embed: ${MODEL_EMBED}"
echo "Storage: ${ZETA_STORAGE:-/storage}"
echo "=========================================="

# Auto-index codebase if INDEX_PATH is set
if [ -n "$INDEX_PATH" ]; then
    (
        sleep 10  # Wait for server to be ready
        echo "Indexing codebase: $INDEX_PATH"
        python3 /app/scripts/index_codebase.py --path "$INDEX_PATH" --server "http://localhost:${ZETA_PORT:-8080}" || {
            echo "WARNING: Indexing failed"
        }
        echo "Indexing complete."
    ) &
fi

# If zeta.conf exists, let server read it directly (no CLI overrides needed)
# Otherwise fall back to environment variables via CLI flags
if [ -f "/app/zeta.conf" ] || [ -f "./zeta.conf" ]; then
    echo "Starting with zeta.conf..."
    exec /app/zeta-server "$@"
else
    echo "Starting with environment variables..."
    exec /app/zeta-server \
        --host "${ZETA_HOST:-0.0.0.0}" \
        --port "${ZETA_PORT:-8080}" \
        -m "${MODEL_MAIN:-/models/qwen2.5-14b-instruct-q4.gguf}" \
        --model-7b-coder "${MODEL_CODER:-/models/qwen2.5-7b-instruct-q4_k_m-00002-of-00002.gguf}" \
        --embed-model "${MODEL_EMBED:-/models/nomic-embed-text-v1.5-q4_k_m.gguf}" \
        -ngl "${GPU_LAYERS_MAIN:-49}" \
        --zeta-storage "${ZETA_STORAGE:-/storage}" \
        "$@"
fi
