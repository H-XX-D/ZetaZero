#!/bin/bash
set -euo pipefail

SERVER="xx@192.168.0.165"
REMOTE_REPO_DIR="~/ZetaZero"
REMOTE_BUILD_BIN_DIR="$REMOTE_REPO_DIR/llama.cpp/build/bin"
LOG_FILE="$REMOTE_REPO_DIR/logs/zeta_server_remote.log"

# Prefer the port from the remote zeta.conf so the restart+health wait aligns
# with the server's config-driven listen port.
REMOTE_CONF_PORT="$(ssh "$SERVER" bash -lc "cd $REMOTE_REPO_DIR 2>/dev/null && awk -F= '/^ZETA_PORT=/{gsub(/\"/,\"\",\$2); print \$2; exit}' zeta.conf 2>/dev/null" || true)"
PORT="${REMOTE_CONF_PORT:-${ZETA_PORT:-8080}}"

remote() {
	# Force bash on the remote host (some login shells don't support '||').
	ssh "$SERVER" bash -lc "$*"
}

echo "Stopping existing Zeta server (best effort)..."
# NOTE: avoid `pkill -f <pattern>` because the pattern can appear in the SSH-launched
# command line and kill the session. Use exact-name matches instead.
remote "pkill -x zeta-zero-server 2>/dev/null || true; pkill -x zeta-server 2>/dev/null || true; pkill -x llama-zeta-zero 2>/dev/null || true; pkill -x llama-zeta-server 2>/dev/null || true"

echo "Waiting for process to terminate..."
sleep 2

echo "Starting new server from zeta.conf (port ${PORT})..."

# Start from repo root so ./zeta.conf is found first.
# Avoid passing CLI args that override config unless you explicitly want to.
remote "mkdir -p $REMOTE_REPO_DIR/logs $REMOTE_REPO_DIR/storage/graph; \
	cd $REMOTE_REPO_DIR; \
	(test -x $REMOTE_BUILD_BIN_DIR/zeta-zero-server && SERVER_BIN=$REMOTE_BUILD_BIN_DIR/zeta-zero-server) || \
	(test -x $REMOTE_BUILD_BIN_DIR/zeta-server && SERVER_BIN=$REMOTE_BUILD_BIN_DIR/zeta-server) || \
	(echo 'ERROR: no server binary found' && ls -la $REMOTE_BUILD_BIN_DIR && exit 1); \
	nohup \"$SERVER_BIN\" > $LOG_FILE 2>&1 & disown; \
	echo \"Launched $SERVER_BIN (config: ./zeta.conf)\""

echo "Server restarted. Logs are being written to $LOG_FILE"

echo "Waiting for /health (remote localhost:${PORT})..."
remote "for i in {1..30}; do curl -s --connect-timeout 1 http://localhost:${PORT}/health && exit 0; sleep 1; done; echo 'ERROR: server did not become healthy'; tail -50 $LOG_FILE; exit 1"
