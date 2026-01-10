#!/bin/bash
# =============================================================================
# Z.E.T.A. Server Startup Script
# =============================================================================
# Starts the server *from zeta.conf*.
#
# The C++ server reads zeta.conf automatically (./zeta.conf in repo root is
# highest priority). This script intentionally avoids passing CLI flags that
# would override config values.
# =============================================================================

set -euo pipefail

# Find config file
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="${REPO_ROOT}/zeta.conf"

# Load config (export all variables so child processes inherit them).
# zeta.conf is intentionally shell-compatible (KEY="value"), so `source` is OK.
if [[ -f "$CONFIG_FILE" ]]; then
    echo "Loading config from: $CONFIG_FILE"
    set -a  # Export all variables
    source "$CONFIG_FILE"
    set +a
else
    echo "ERROR: Config file not found: $CONFIG_FILE"
    echo "Create zeta.conf in repository root with your settings."
    exit 1
fi

# Ensure we run from repo root so ./zeta.conf wins in the server's search order.
cd "$REPO_ROOT"

expand_path() {
    local p="$1"
    if [[ "$p" == "~" ]]; then
        echo "$HOME"
    elif [[ "$p" == ~/* ]]; then
        echo "$HOME/${p:2}"
    else
        echo "$p"
    fi
}

# Determine server binary path (prefer zeta-zero-server).
ZETA_SERVER_BIN="${ZETA_SERVER_BIN:-${ZETA_BUILD_DIR:-./llama.cpp/build}/bin/zeta-zero-server}"
ZETA_SERVER_BIN="$(expand_path "$ZETA_SERVER_BIN")"

if [[ ! -x "$ZETA_SERVER_BIN" ]]; then
    # Back-compat fallback
    local_fallback="${ZETA_BUILD_DIR:-./llama.cpp/build}/bin/zeta-server"
    local_fallback="$(expand_path "$local_fallback")"
    if [[ -x "$local_fallback" ]]; then
        ZETA_SERVER_BIN="$local_fallback"
    else
        echo "ERROR: Server binary not found or not executable."
        echo "Tried: $ZETA_SERVER_BIN"
        echo "Also tried: $local_fallback"
        exit 1
    fi
fi

# Check if running locally or remotely
# If ZETA_SSH_* is set, assume remote deployment unless we're already on an SSH session.
IS_REMOTE=false
if [[ -n "${ZETA_SSH_USER:-}" ]] && [[ -n "${ZETA_SSH_HOST:-}" ]]; then
    if [[ -z "${SSH_CONNECTION:-}" ]] && [[ "$ZETA_SSH_HOST" != "localhost" ]]; then
        IS_REMOTE=true
    fi
fi

start_server() {
    echo "=============================================="
    echo "Starting Z.E.T.A. Server"
    echo "=============================================="
    echo "  Host: ${ZETA_HOST:-0.0.0.0}:${ZETA_PORT:-8080}"
    echo "  Config: $CONFIG_FILE"
    echo "  Binary: $ZETA_SERVER_BIN"
    echo "=============================================="

    # Best-effort stop of any prior instance.
    pkill -x zeta-zero-server 2>/dev/null || true
    pkill -x zeta-server 2>/dev/null || true
    pkill -x llama-zeta-zero 2>/dev/null || true
    pkill -x llama-zeta-server 2>/dev/null || true
    sleep 2

    # Ensure default directories exist (relative to repo root by default).
    ZETA_STORAGE_DIR="$(expand_path "${ZETA_STORAGE:-./storage/graph}")"
    ZETA_LOG_FILE="$(expand_path "${ZETA_LOG:-./logs/zeta.log}")"
    mkdir -p "$ZETA_STORAGE_DIR"
    mkdir -p "$(dirname "$ZETA_LOG_FILE")"
    mkdir -p "$(expand_path "${ZETA_GKV_DIR:-./storage/graph_kv}")" || true
    mkdir -p "$(expand_path "${ZETA_DREAM_DIR:-./storage/dreams}")" || true

    echo "Command: $ZETA_SERVER_BIN $*"
    echo ""

    # Start server
    nohup "$ZETA_SERVER_BIN" "$@" > "$ZETA_LOG_FILE" 2>&1 &
    SERVER_PID=$!
    echo "Server PID: $SERVER_PID"
    
    # Wait for startup
    echo "Waiting for server to start..."
    for i in {1..30}; do
        sleep 1
        if curl -s "http://localhost:${ZETA_PORT:-8080}/health" > /dev/null 2>&1; then
            echo "✓ Server is UP!"
            curl -s "http://localhost:${ZETA_PORT:-8080}/health" | head -c 200
            echo ""
            exit 0
        fi
        echo -n "."
    done

    echo ""
    echo "ERROR: Server failed to start. Check $ZETA_LOG_FILE"
    tail -50 "$ZETA_LOG_FILE"
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
