#!/bin/bash
set -e

echo "=========================================="
echo "  Z.E.T.A. Node Starting..."
echo "=========================================="
echo "Main Model (14B): $MODEL_MAIN"
echo "Coder Model (7B): $MODEL_CODER"
echo "Embed Model: $MODEL_EMBED"
echo "Storage: $ZETA_STORAGE"
echo "Dream Interval: ${ZETA_DREAM_INTERVAL}s"
echo "GPU Layers Main: $GPU_LAYERS_MAIN"
echo "GPU Layers Coder: $GPU_LAYERS_CODER"
echo "=========================================="

# Check if models exist
for model in "$MODEL_MAIN" "$MODEL_CODER" "$MODEL_EMBED"; do
    if [ ! -f "$model" ]; then
        echo "WARNING: Model not found: $model"
    fi
done

# Execute the server with full configuration
exec /app/zeta-server \
    --host "$ZETA_HOST" \
    --port "$ZETA_PORT" \
    --model "$MODEL_MAIN" \
    --model-7b-coder "$MODEL_CODER" \
    --model-embed "$MODEL_EMBED" \
    --n-gpu-layers "$GPU_LAYERS_MAIN" \
    --n-gpu-layers-7b "$GPU_LAYERS_CODER" \
    --ctx-size 16384 \
    --parallel 4 \
    --dream-interval "$ZETA_DREAM_INTERVAL" \
    --dream-dir "$ZETA_DREAM_DIR" \
    --graph-path "$ZETA_GRAPH_PATH" \
    --flash-attn \
    "$@"
