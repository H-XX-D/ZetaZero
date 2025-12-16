#!/bin/bash
echo "Checking server status and capturing logs..."

# Check if server is running
if curl -s http://192.168.0.165:9000/health > /dev/null; then
    echo "Server is running. Attempting to capture current logs..."
else
    echo "Server is not running. Starting it with logging..."
    ssh xx@192.168.0.165 "cd ~/ZetaZero/llama.cpp && ./build/bin/llama-zeta-server -m /mnt/HoloGit/models/qwen2.5-14b-instruct-q4_k_m.gguf --model-3b /mnt/HoloGit/models/qwen2.5-3b-instruct-q4_k_m.gguf --embed-model /mnt/HoloGit/models/qwen3-embedding-4b-q4_k_m.gguf --stream-tokens 1500 --stream-nodes 10 --code-tokens 1800 --code-nodes 15 --port 9000 > zeta_server.log 2>&1 &"
    sleep 15
fi

# Try to pull any existing logs
echo "Pulling logs from server..."
ssh xx@192.168.0.165 "find ~/ZetaZero -name '*.log' -exec cat {} \;" > zeta_server_combined.log 2>/dev/null || echo "No log files found"

# Also try to get current process output if possible
ssh xx@192.168.0.165 "ps aux | grep llama-zeta-server | grep -v grep" >> zeta_server_combined.log

echo "Logs saved to: zeta_server_combined.log"
echo "First 20 lines:"
head -20 zeta_server_combined.log
