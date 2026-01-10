#!/usr/bin/env bash
set -euo pipefail

# End-to-end system test:
# 1) Start Z.E.T.A. server from zeta.conf (no CLI overrides)
# 2) Wait for /health
# 3) Run the HTTP smoke test suite

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$REPO_ROOT"

# Start server (best-effort). This leaves the server running after the test.
"$SCRIPT_DIR/start_zeta_server.sh"

# Run smoke tests against the configured URL.
# The smoke test resolves base URL from: ZETA_BASE_URL -> ZETA_URL -> zeta.conf -> http://localhost:8080
"$SCRIPT_DIR/run_system_smoke_test.sh" "$@"
