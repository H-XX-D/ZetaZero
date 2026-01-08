#!/usr/bin/env bash
set -euo pipefail

# Bootstrap a *fresh* user on Z6 to run Docker + clone/pull a (private) repo.
# This script is designed to be run from your local machine.
#
# IMPORTANT:
# - Creating users / adding to docker group requires sudo on the remote host.
# - Private repo cloning requires credentials (deploy key or HTTPS token).
# - This script does NOT embed secrets.
#
# Usage:
#   ./scripts/z6_docker_fresh_user_setup.sh xx@192.168.0.165 zetauser /home/<user>/ZetaZero
#
# Args:
#   1) SSH target (default: xx@192.168.0.165)
#   2) New username (default: zetauser)
#   3) Repo clone dir (default: /home/<user>/ZetaZero)

SERVER="${1:-xx@192.168.0.165}"
NEW_USER="${2:-zetauser}"
REPO_DIR="${3:-/home/${NEW_USER}/ZetaZero}"

cat <<EOF
[INFO] This will (on the remote host):
  - create user: ${NEW_USER}
  - add to docker group (so they can run docker without sudo)
  - prepare ${REPO_DIR}

You will still need to provide repo access:
  Option A (recommended): add a deploy key for the repo and place it at /home/${NEW_USER}/.ssh/id_ed25519
  Option B: use HTTPS + token (GITHUB_TOKEN) and set the remote URL accordingly
EOF

echo "[STEP] Creating user and enabling docker group..."
ssh "$SERVER" "set -euo pipefail
  if ! id -u '${NEW_USER}' >/dev/null 2>&1; then
    sudo useradd -m -s /bin/bash '${NEW_USER}'
  fi
  sudo usermod -aG docker '${NEW_USER}'
  sudo mkdir -p /home/'${NEW_USER}'/.ssh
  sudo chown -R '${NEW_USER}':'${NEW_USER}' /home/'${NEW_USER}'/.ssh
  sudo chmod 700 /home/'${NEW_USER}'/.ssh
  echo '[OK] user + docker group configured'
"

echo "[STEP] Checking docker works for the new user..."
ssh "$SERVER" "set -euo pipefail
  sudo -u '${NEW_USER}' -H bash -lc 'docker version >/dev/null 2>&1 && echo "[OK] docker version works" || (echo "[ERR] docker not usable for user yet"; exit 1)'
"

echo "[NEXT] Clone or pull the repo as the new user. Example commands (run on Z6):"
cat <<EOF
ssh ${SERVER} "sudo -u ${NEW_USER} -H bash -lc 'git clone <YOUR_PRIVATE_REPO_URL> ${REPO_DIR}'"
# or if already cloned:
ssh ${SERVER} "sudo -u ${NEW_USER} -H bash -lc 'cd ${REPO_DIR} && git pull --ff-only'"

# Then run docker compose (example):
ssh ${SERVER} "sudo -u ${NEW_USER} -H bash -lc 'cd ${REPO_DIR} && docker compose up -d --build'"
EOF
