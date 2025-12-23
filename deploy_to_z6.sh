#!/bin/bash
# Deploy ZetaZero to Z6 (192.168.0.165)

SERVER="xx@192.168.0.165"
REMOTE_DIR="~/ZetaZero/llama.cpp/tools/zeta-demo"

echo "Deploying to $SERVER..."

# Ensure remote directory exists
ssh $SERVER "mkdir -p $REMOTE_DIR"

# Copy all header and cpp files from tools/zeta-demo
scp llama.cpp/tools/zeta-demo/*.h $SERVER:$REMOTE_DIR/
scp llama.cpp/tools/zeta-demo/*.cpp $SERVER:$REMOTE_DIR/

# Copy the new scripts
scp scripts/steer_dreams.sh $SERVER:~/ZetaZero/scripts/

echo "Files transferred."

# Trigger remote build
echo "Triggering remote build..."
ssh $SERVER "cd ~/ZetaZero/llama.cpp && cmake -B build && cmake --build build --config Release --target zeta-server -j4"

echo "Deployment complete."
