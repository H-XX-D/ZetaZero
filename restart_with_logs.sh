#!/bin/bash
# Restart Zeta server with logging
echo "Restarting Zeta server with logging..."

# Kill existing server
ssh xx@192.168.0.165 "pkill -f llama-zeta-server" 2>/dev/null || true
sleep 2

# Start server with logging
ssh xx@192.168.0.165 "cd ~/ZetaZero/llama.cpp && nohup ./build/bin/llama-zeta-server -m /mnt/HoloGit/models/qwen2.5-14b-instruct-q4_k_m.gguf --model-3b /mnt/HoloGit/models/qwen2.5-3b-instruct-q4_k_m.gguf --embed-model /mnt/HoloGit/models/qwen3-embedding-4b-q4_k_m.gguf --stream-tokens 1500 --stream-nodes 10 --code-tokens 1800 --code-nodes 15 --port 9000 > ~/ZetaZero/zeta_server.log 2>&1 &"

echo "Server restarted. Waiting for startup..."
sleep 10

# Check if it's running
curl -s http://192.168.0.165:9000/health > /dev/null
if [ $? -eq 0 ]; then
    echo "Server is running. Pulling logs..."
    scp xx@192.168.0.165:~/ZetaZero/zeta_server.log ./zeta_server_remote.log
    echo "Logs pulled to: zeta_server_remote.log"
    head -50 zeta_server_remote.log
else
    echo "Server failed to start"
fi
