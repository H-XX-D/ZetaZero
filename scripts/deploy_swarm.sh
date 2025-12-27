#!/bin/bash
set -e
# Deploy Zeta Architecture to Orin Swarm
# This script runs on Z6 (192.168.0.165)

# Configuration
BUILD_NODE_IP="192.168.0.242"
BUILD_NODE_USER="gin"
BUILD_NODE_HOST="$BUILD_NODE_USER@$BUILD_NODE_IP"

# Models on Z6
MODEL_INSTRUCT="/home/xx/models/qwen2.5-3b-instruct-q4_k_m.gguf"
MODEL_CODER="/home/xx/models/qwen2.5-3b-coder-q4_k_m.gguf"
MODEL_EMBED="/home/xx/ZetaZero/llama.cpp/models/nomic-embed-text-v1.5-q4_k_m.gguf"

# Nodes List "User@IP"
NODES=(
    "gin@192.168.0.242"
    "tif@192.168.0.213"
    "orin@192.168.0.160"
    "archi@192.168.0.232"
    "tori@192.168.0.235"
    "novel@192.168.0.146"
    "page@192.168.0.168"
)

echo "=================================================="
echo "Zeta Swarm Deployment"
echo "=================================================="

# 1. Prepare Build Node (GIN)
if false; then
echo "[1/4] Preparing Build Node ($BUILD_NODE_HOST)..."
ssh $BUILD_NODE_HOST "rm -rf ~/ZetaZero && mkdir -p ~/ZetaZero/llama.cpp"

echo "Syncing source code..."
# Sync llama.cpp excluding build artifacts and git history to save time
rsync -av --exclude 'build' --exclude '.git' --exclude 'models/*.gguf' \
    ~/ZetaZero/llama.cpp/ $BUILD_NODE_HOST:~/ZetaZero/llama.cpp/

# Sync zeta-integration folder
rsync -av ~/ZetaZero/zeta-integration/ $BUILD_NODE_HOST:~/ZetaZero/zeta-integration/

echo "Compiling on Build Node (this may take a while)..."
ssh $BUILD_NODE_HOST "cd ~/ZetaZero/llama.cpp && cmake -B build -DLLAMA_CURL=OFF -DBUILD_SHARED_LIBS=OFF && cmake --build build --config Release --target zeta-server -j1"

echo "Retrieving compiled binary..."
mkdir -p ~/ZetaZero/dist
scp $BUILD_NODE_HOST:~/ZetaZero/llama.cpp/build/bin/zeta-server ~/ZetaZero/dist/zeta-server-aarch64
fi

# Check if binary exists
if [ ! -f ~/ZetaZero/dist/zeta-server-aarch64 ]; then
    echo "ERROR: Compilation failed or binary not found!"
    exit 1
fi

# 2. Deploy to All Nodes
echo "[2/4] Deploying to Swarm Nodes..."

for NODE in "${NODES[@]}"; do
    echo "--------------------------------------------------"
    echo "Deploying to $NODE..."
    
    # Wipe and Prepare
    ssh $NODE "pkill -x zeta-server || true; rm -rf ~/ZetaZero; mkdir -p ~/ZetaZero/bin ~/models"
    
    # Copy Binary
    scp ~/ZetaZero/dist/zeta-server-aarch64 $NODE:~/ZetaZero/bin/zeta-server
    ssh $NODE "chmod +x ~/ZetaZero/bin/zeta-server"
    
    # Copy Models (Check if exists first to save bandwidth)
    echo "Syncing models..."
    # We use rsync with --ignore-existing to avoid re-copying large files
    rsync -av --progress --ignore-existing $MODEL_INSTRUCT $NODE:~/models/
    rsync -av --progress --ignore-existing $MODEL_CODER $NODE:~/models/
    rsync -av --progress --ignore-existing $MODEL_EMBED $NODE:~/models/
    
    # Create Config
    echo "Creating configuration..."
    ssh $NODE "cat > ~/ZetaZero/zeta.conf" <<EOF
# Zeta Swarm Node Configuration
ZETA_HOST="0.0.0.0"
ZETA_PORT="11434"

# Models
MODEL_MAIN="/home/${NODE%%@*}/models/$(basename $MODEL_INSTRUCT)"
MODEL_CODER="/home/${NODE%%@*}/models/$(basename $MODEL_CODER)"
MODEL_EMBED="/home/${NODE%%@*}/models/$(basename $MODEL_EMBED)"

# Storage
ZETA_STORAGE="/home/${NODE%%@*}/.zetazero/storage"
EOF

    # Create Startup Script
    ssh $NODE "cat > ~/ZetaZero/start_node.sh" <<EOF
#!/bin/bash
# Start Zeta Node
cd ~/ZetaZero
source zeta.conf

mkdir -p \$ZETA_STORAGE

echo "Starting Zeta Node..."
nohup ./bin/zeta-server \\
    -m \$MODEL_MAIN \\
    --model-7b-coder \$MODEL_CODER \\
    --embed-model \$MODEL_EMBED \\
    --zeta-storage \$ZETA_STORAGE \\
    --port \$ZETA_PORT \\
    --host \$ZETA_HOST \\
    > zeta_node.log 2>&1 &

echo "Node started on port \$ZETA_PORT"
EOF
    ssh $NODE "chmod +x ~/ZetaZero/start_node.sh"
    
    # Start Service
    echo "Starting service..."
    ssh $NODE "~/ZetaZero/start_node.sh"
    
    echo "Done with $NODE"
done

echo "=================================================="
echo "Deployment Complete!"
echo "=================================================="
