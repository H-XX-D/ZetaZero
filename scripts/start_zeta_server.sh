#!/bin/bash
# =============================================================================
# Z.E.T.A. Server Startup Script
# =============================================================================
# Reads configuration from ../zeta.conf (relative to this script)
# =============================================================================

set -e

# Find config file
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="${REPO_ROOT}/zeta.conf"

# Load config
if [[ -f "$CONFIG_FILE" ]]; then
    echo "Loading config from: $CONFIG_FILE"
    source "$CONFIG_FILE"
else
    echo "ERROR: Config file not found: $CONFIG_FILE"
    echo "Create zeta.conf in repository root with your settings."
    exit 1
fi

# Validate required settings
if [[ -z "$MODEL_14B" ]] && [[ -z "$MODEL_7B_CODER" ]]; then
    echo "ERROR: No models configured. Set MODEL_14B or MODEL_7B_CODER in zeta.conf"
    exit 1
fi

if [[ -z "$ZETA_SERVER_BIN" ]]; then
    ZETA_SERVER_BIN="${ZETA_BUILD_DIR}/bin/zeta-server"
fi

# Check if running locally or remotely
IS_REMOTE=false
if [[ -n "$ZETA_SSH_USER" ]] && [[ -n "$ZETA_SSH_HOST" ]]; then
    # Check if we're on the remote host already
    CURRENT_HOST=$(hostname -I 2>/dev/null | awk '{print $1}' || echo "localhost")
    if [[ "$CURRENT_HOST" != "$ZETA_SSH_HOST" ]] && [[ "$ZETA_SSH_HOST" != "localhost" ]]; then
        IS_REMOTE=true
    fi
fi

start_server() {
    echo "=============================================="
    echo "Starting Z.E.T.A. Server"
    echo "=============================================="
    echo "  Host: ${ZETA_HOST}:${ZETA_PORT}"
    echo "  14B Model: ${MODEL_14B:-'(none)'}"
    echo "  7B Coder:  ${MODEL_7B_CODER:-'(none)'}"
    echo "  Embed:     ${MODEL_EMBED:-'(none)'}"
    echo "  GPU Layers: $GPU_LAYERS"
    echo "  Context: $CTX_14B"
    echo "=============================================="

    # Check if server already running
    if pgrep -x zeta-server > /dev/null 2>&1; then
        echo "WARNING: Server already running. Killing it..."
        pkill -9 zeta-server || true
        sleep 2
    fi

    # Build command
    CMD="$ZETA_SERVER_BIN --host 0.0.0.0 --port $ZETA_PORT"
    
    if [[ -n "$MODEL_14B" ]] && [[ -f "$MODEL_14B" ]]; then
        CMD="$CMD -m $MODEL_14B"
    fi
    
    if [[ -n "$MODEL_7B_CODER" ]] && [[ -f "$MODEL_7B_CODER" ]]; then
        CMD="$CMD --model-7b-coder $MODEL_7B_CODER"
    fi
    
    if [[ -n "$MODEL_EMBED" ]] && [[ -f "$MODEL_EMBED" ]]; then
        CMD="$CMD --embed-model $MODEL_EMBED"
    fi
    
    CMD="$CMD --gpu-layers $GPU_LAYERS"
    CMD="$CMD --ctx-size $CTX_14B"
    
    if [[ -n "$ZETA_STORAGE" ]]; then
        CMD="$CMD --zeta-storage $ZETA_STORAGE"
    fi

    echo "Command: $CMD"
    echo ""

    # Start server
    nohup $CMD > "${ZETA_LOG:-/tmp/zeta.log}" 2>&1 &
    SERVER_PID=$!
    echo "Server PID: $SERVER_PID"
    
    # Wait for startup
    echo "Waiting for server to start..."
    for i in {1..30}; do
        sleep 1
        if curl -s "http://localhost:${ZETA_PORT}/health" > /dev/null 2>&1; then
            echo "✓ Server is UP!"
            curl -s "http://localhost:${ZETA_PORT}/health" | head -c 200
            echo ""
            exit 0
        fi
        echo -n "."
    done

    echo ""
    echo "ERROR: Server failed to start. Check ${ZETA_LOG:-/tmp/zeta.log}"
    tail -50 "${ZETA_LOG:-/tmp/zeta.log}"
    exit 1
}

# Run remotely via SSH if needed
if [[ "$IS_REMOTE" == "true" ]]; then
    echo "Deploying to remote host: ${ZETA_SSH_USER}@${ZETA_SSH_HOST}"
    
    # Copy config to remote
    scp "$CONFIG_FILE" "${ZETA_SSH_USER}@${ZETA_SSH_HOST}:~/ZetaZero/zeta.conf"
    
    # Run this script on remote
    ssh "${ZETA_SSH_USER}@${ZETA_SSH_HOST}" "cd ~/ZetaZero && bash scripts/start_zeta_server.sh"
else
    start_server
fi
