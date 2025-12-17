#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PORT_ZETA="${PORT_ZETA:-9000}"
PORT_UI="${PORT_UI:-9001}"

MODELS_DIR="${MODELS_DIR:-$HOME/models/zetazero-8gb}"
ZETA_STORAGE_DIR="${ZETA_STORAGE_DIR:-$HOME/.zetazero/HoloGit/blocks}"
UI_DIR="$ROOT/ui/for-8gb-gpus-chat"

MODEL_MAIN_NAME="${MODEL_MAIN_NAME:-qwen2.5-3b-instruct-q4_k_m.gguf}"
MODEL_3B_NAME="${MODEL_3B_NAME:-qwen2.5-3b-instruct-q4_k_m.gguf}"
MODEL_3B_CODER_NAME="${MODEL_3B_CODER_NAME:-qwen2.5-3b-coder-q4_k_m.gguf}"
EMBED_NAME="${EMBED_NAME:-gte-Qwen2-1.5B-instruct-Q4_K_M.gguf}"

MODEL_MAIN_URL="${MODEL_MAIN_URL:-}"
MODEL_3B_URL="${MODEL_3B_URL:-}"
MODEL_3B_CODER_URL="${MODEL_3B_CODER_URL:-}"
EMBED_URL="${EMBED_URL:-}"

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "ERROR: missing required command: $1" >&2; exit 2; }
}

need_cmd wget
need_cmd curl
need_cmd python3

wget_auth_args=()
if [[ -n "${HF_TOKEN:-}" ]]; then
  wget_auth_args+=("--header=Authorization: Bearer ${HF_TOKEN}")
fi

mkdir -p "$MODELS_DIR" "$ZETA_STORAGE_DIR"

fetch_model() {
  local url="$1"
  local out="$2"

  if [[ -z "$url" ]]; then
    echo "ERROR: missing URL for $out" >&2
    return 1
  fi

  if [[ -s "$out" ]]; then
    echo "OK: exists $(basename "$out")"
    return 0
  fi

  echo "Downloading $(basename "$out")"
  local tmp="${out}.partial"
  rm -f "$tmp"
  wget -O "$tmp" --progress=dot:giga "${wget_auth_args[@]}" "$url"

  if [[ ! -s "$tmp" ]]; then
    echo "ERROR: download produced empty file: $out" >&2
    rm -f "$tmp"
    return 1
  fi

  mv "$tmp" "$out"
}

fetch_model_optional() {
  local url="$1"
  local out="$2"
  if [[ -s "$out" ]]; then
    echo "OK: exists $(basename "$out")"
    return 0
  fi
  if [[ -z "$url" ]]; then
    echo "SKIP: no URL for optional $(basename "$out")"
    return 0
  fi
  fetch_model "$url" "$out"
}

cat <<EOF
== For 8GB GPUs ==
This script downloads models via wget, starts llama-zeta-server on :${PORT_ZETA},
creates a local HoloGit storage dir, and opens a minimal chat-only UI.

You must provide model URLs (Hugging Face 'resolve/main' links work).
Set env vars before running, for example:

  export MODEL_MAIN_URL='...'
  export MODEL_3B_URL='...'
  export MODEL_3B_CODER_URL='...'
  export EMBED_URL='...'

Optional for private repos:
  export HF_TOKEN='hf_...'

EOF

MODEL_MAIN_PATH="$MODELS_DIR/$MODEL_MAIN_NAME"
MODEL_3B_PATH="$MODELS_DIR/$MODEL_3B_NAME"
MODEL_3B_CODER_PATH="$MODELS_DIR/$MODEL_3B_CODER_NAME"
EMBED_PATH="$MODELS_DIR/$EMBED_NAME"

fetch_model "$MODEL_MAIN_URL" "$MODEL_MAIN_PATH"
fetch_model "$MODEL_3B_URL" "$MODEL_3B_PATH"
fetch_model_optional "$MODEL_3B_CODER_URL" "$MODEL_3B_CODER_PATH"
fetch_model_optional "$EMBED_URL" "$EMBED_PATH"

echo "Models in: $MODELS_DIR"
echo "HoloGit storage in: $ZETA_STORAGE_DIR"

# Pick the best available server binary
ZETA_BIN=""
if [[ -x "$ROOT/llama.cpp/build-cuda12/bin/llama-zeta-server" ]]; then
  ZETA_BIN="$ROOT/llama.cpp/build-cuda12/bin/llama-zeta-server"
elif [[ -x "$ROOT/llama.cpp/build/bin/llama-zeta-server" ]]; then
  ZETA_BIN="$ROOT/llama.cpp/build/bin/llama-zeta-server"
else
  echo "ERROR: llama-zeta-server not found. Build it first (see README)." >&2
  exit 3
fi

echo "Using server: $ZETA_BIN"

# Stop any existing server on the port
if command -v pkill >/dev/null 2>&1; then
  pkill -f "llama-zeta-server.*--port ${PORT_ZETA}" 2>/dev/null || true
fi

LOG_FILE="$ROOT/zeta_server_8gb.log"

# For 8GB GPUs we keep models small (3B + 3B). Coder+embedding are optional.
args=(
  -m "$MODEL_MAIN_PATH"
  --model-3b "$MODEL_3B_PATH"
  --zeta-storage "$ZETA_STORAGE_DIR"
  --port "$PORT_ZETA"
)

if [[ -s "$MODEL_3B_CODER_PATH" ]]; then
  args+=(--model-3b-coder "$MODEL_3B_CODER_PATH")
fi

if [[ -s "$EMBED_PATH" ]]; then
  args+=(--embed-model "$EMBED_PATH")
fi

set +e
nohup "$ZETA_BIN" \
  "${args[@]}" \
  >"$LOG_FILE" 2>&1 &
ZETA_PID=$!
set -e

echo "Server PID: $ZETA_PID"
echo "Logs: $LOG_FILE"

echo -n "Waiting for /health"
for _ in {1..60}; do
  if curl -sf "http://localhost:${PORT_ZETA}/health" >/dev/null 2>&1; then
    echo " OK"
    break
  fi
  echo -n "."
  sleep 0.5
done

if ! curl -sf "http://localhost:${PORT_ZETA}/health" >/dev/null 2>&1; then
  echo
  echo "ERROR: server did not become healthy. Check: $LOG_FILE" >&2
  exit 4
fi

# Serve the chat-only UI
UI_URL="http://localhost:${PORT_UI}/?wd=$(python3 -c 'import urllib.parse,sys; print(urllib.parse.quote(sys.argv[1]))' "$ROOT")"

echo "Starting UI server on :${PORT_UI}"
nohup python3 -m http.server "$PORT_UI" --directory "$UI_DIR" >"$ROOT/zeta_ui_8gb.log" 2>&1 &
UI_PID=$!

echo "UI: $UI_URL"

if command -v xdg-open >/dev/null 2>&1; then
  xdg-open "$UI_URL" >/dev/null 2>&1 || true
else
  echo "NOTE: xdg-open not found; open the URL manually." >&2
fi

echo "Done. To stop: kill $ZETA_PID $UI_PID"
