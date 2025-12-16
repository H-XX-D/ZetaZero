#!/bin/bash
cd ~/ZetaZero/llama.cpp

# Try to find models
MODEL_14B=$(find ~/ZetaZero -name "*Qwen*14B*.gguf" -o -name "*14b*.gguf" | head -n 1)
MODEL_3B=$(find ~/ZetaZero -name "*Qwen*2.5*3B*.gguf" -o -name "*3b*.gguf" | head -n 1)

if [ -z "$MODEL_14B" ]; then
    echo "Error: Could not find a 14B model (looked for *Qwen*14B*.gguf or *14b*.gguf)"
    # Fallback to any gguf if specific ones fail, but risky
    MODEL_14B=$(find ~/ZetaZero -name "*.gguf" | grep -v "3b" | head -n 1)
fi

if [ -z "$MODEL_14B" ]; then
    echo "Critical: No models found."
    exit 1
fi

echo "Starting Zeta Server..."
echo "14B Model: $MODEL_14B"
echo "3B Model:  $MODEL_3B"

./build/bin/llama-zeta-server \
    -m "$MODEL_14B" \
    ${MODEL_3B:+--model-3b "$MODEL_3B"} \
    --stream-tokens 1500 \
    --stream-nodes 10 \
    --code-tokens 1800 \
    --code-nodes 15 \
    --port 9000
