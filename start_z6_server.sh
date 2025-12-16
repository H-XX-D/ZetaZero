#!/bin/bash
# Start ZetaZero server on z6 remotely

echo "Starting ZetaZero server on z6..."

ssh -o BatchMode=yes -o ConnectTimeout=5 xx@192.168.0.165 << 'EOF'
cd ~/ZetaZero/llama.cpp

# Kill any existing server
pkill -f llama-zeta-server 2>/dev/null
sleep 2

# Start the server in background with nohup
nohup ./build/bin/llama-zeta-server \
  -m /mnt/HoloGit/models/qwen2.5-14b-instruct-q4_k_m.gguf \
  --model-3b /mnt/HoloGit/models/qwen2.5-3b-instruct-q4_k_m.gguf \
  --embed-model /mnt/HoloGit/models/qwen3-embedding-4b-q4_k_m.gguf \
  --stream-tokens 1500 \
  --stream-nodes 10 \
  --port 9000 \
  > ~/zeta_server.log 2>&1 &

echo "Server starting... PID: $!"
sleep 3
ps aux | grep llama-zeta-server | grep -v grep
EOF

echo ""
echo "Waiting for server to initialize..."
sleep 10

# Check if server is responding
curl -s --connect-timeout 5 http://192.168.0.165:9000/health && echo "Server is UP!" || echo "Server not responding yet"
