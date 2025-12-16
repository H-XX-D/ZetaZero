#!/bin/bash
set -e

REMOTE_USER="xx"
REMOTE_HOST="192.168.0.165"
REMOTE_DIR="~/ZetaZero/llama.cpp/tools/zeta-demo"

echo "Deploying modified files to $REMOTE_HOST..."

scp llama.cpp/tools/zeta-demo/zeta-streaming.h $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/
scp llama.cpp/tools/zeta-demo/zeta-code-streaming.h $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/
scp llama.cpp/tools/zeta-demo/zeta-server.cpp $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/

echo "Files transferred. Starting build on remote..."

ssh $REMOTE_USER@$REMOTE_HOST "cd ~/ZetaZero/llama.cpp && make llama-zeta-server"

echo "Build complete."
