#!/usr/bin/env bash
set -euo pipefail

# Clears GPU usage on Z6 by:
# - stopping docker containers using the GPU (best-effort)
# - killing any remaining compute PIDs reported by nvidia-smi (best-effort)
#
# Usage:
#   ./scripts/z6_clear_gpu.sh xx@192.168.0.165

SERVER="${1:-xx@192.168.0.165}"

ssh "$SERVER" 'set -euo pipefail

if command -v docker >/dev/null 2>&1; then
  echo "[GPU-CLEAR] Stopping docker containers (best-effort)..."
  # Stop any running containers that may hold GPU memory.
  # We do not try to be clever about GPU detection here; we stop likely candidates.
  docker ps --format "{{.ID}} {{.Image}} {{.Names}}" | sed -n "1,30p" || true
  # Try stopping all running containers (safe on a dedicated box; comment out if undesired)
  docker ps -q | xargs -r docker stop -t 10 || true
fi

echo "[GPU-CLEAR] Killing remaining compute PIDs from nvidia-smi (best-effort)..."
if command -v nvidia-smi >/dev/null 2>&1; then
  pids=$(nvidia-smi --query-compute-apps=pid --format=csv,noheader 2>/dev/null | tr -d " " | grep -E "^[0-9]+$" || true)
  if [ -n "$pids" ]; then
    echo "$pids" | xargs -r kill || true
    sleep 2
    # If still present, SIGKILL
    pids2=$(nvidia-smi --query-compute-apps=pid --format=csv,noheader 2>/dev/null | tr -d " " | grep -E "^[0-9]+$" || true)
    if [ -n "$pids2" ]; then
      echo "$pids2" | xargs -r kill -9 || true
    fi
  else
    echo "[GPU-CLEAR] No compute PIDs reported."
  fi

  echo "[GPU-CLEAR] Current GPU memory usage:"
  nvidia-smi --query-gpu=name,memory.used,memory.free --format=csv,noheader || true
else
  echo "[GPU-CLEAR] nvidia-smi not found."
fi
'
