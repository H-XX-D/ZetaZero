#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TIMEOUT="${ZETA_TIMEOUT:-30}"

# Only pass --base-url if explicitly provided via env.
# Otherwise let the Python test resolve base URL from zeta.conf.
ARGS=("--timeout" "$TIMEOUT")
if [[ -n "${ZETA_BASE_URL:-}" ]]; then
  ARGS+=("--base-url" "$ZETA_BASE_URL")
elif [[ -n "${ZETA_URL:-}" ]]; then
  ARGS+=("--base-url" "$ZETA_URL")
fi

exec python3 "$SCRIPT_DIR/zeta_system_smoke_test.py" "${ARGS[@]}" "$@"
