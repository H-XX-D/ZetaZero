#!/bin/bash
SERVER="xx@192.168.0.165"
REMOTE_BUILD_DIR="~/ZetaZero/llama.cpp/build/bin"
LOG_FILE="~/ZetaZero/zeta_server.log"

echo "Stopping existing zeta-server..."
ssh $SERVER "pkill -f zeta-server"

echo "Waiting for process to terminate..."
sleep 2

echo "Starting new zeta-server..."
# Start in background, redirect output to log file
ssh $SERVER "cd $REMOTE_BUILD_DIR && nohup ./zeta-server > $LOG_FILE 2>&1 &"

echo "Server restarted. Logs are being written to $LOG_FILE"
