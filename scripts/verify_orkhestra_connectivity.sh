#!/usr/bin/env bash
set -euo pipefail

# Verifies Mac mini connectivity to OrKheStrA workstation + nano nodes.
# Safe: no passwords/keys are read or stored.

WORKSTATION_IP="${WORKSTATION_IP:-192.168.0.165}"
WORKSTATION_SSH_PORT="${WORKSTATION_SSH_PORT:-22}"
SSH_CONNECT_TIMEOUT="${SSH_CONNECT_TIMEOUT:-3}"
UI_PORT="${UI_PORT:-5001}"
UI_PATH="${UI_PATH:-/api/heartbeat}"
UI_URL="http://${WORKSTATION_IP}:${UI_PORT}${UI_PATH}"
SSH_HOST_ALIAS="orka-workstation"
SSH_USER_DEFAULT="xx"
REPO_PATH_ON_WORKSTATION="/home/xx/AI-Safety-System-Design"

# Researcher nodes (Ollama)
NANO_NODES=(
  "tif:192.168.0.213"
  "gin:192.168.0.242"
  "orin:192.168.0.160"
  "tori:192.168.0.235"
  "archi:192.168.0.232"
  "theo:192.168.0.168"
  "novel:192.168.0.146"
)

SSH_USER="${SSH_USER:-$SSH_USER_DEFAULT}"
# Default to direct user@ip so this works even without ~/.ssh/config.
# You can override to use an alias with: SSH_TARGET=orka-workstation
SSH_TARGET="${SSH_TARGET:-${SSH_USER}@${WORKSTATION_IP}}"

# SSH behavior:
# - In non-interactive runs, this script will not prompt for passwords.
# - In interactive runs (TTY), it will try key-based auth first and then fall
#   back to password auth to validate basic access.
SSH_BATCHMODE="${SSH_BATCHMODE:-auto}" # yes | no | auto
SSH_PASS="${SSH_PASS:-}"             # optional; if set and sshpass exists, enables non-interactive password SSH

# Nano checks (your nanos may be SSH-controlled even if Ollama isn't reachable)
# Default: skip Ollama checks (nodes may be SSH-only / gated).
# Enable explicitly with: CHECK_NANO_OLLAMA=1
CHECK_NANO_OLLAMA="${CHECK_NANO_OLLAMA:-0}"
CHECK_NANO_SSH="${CHECK_NANO_SSH:-0}"
NANO_SSH_USER="${NANO_SSH_USER:-$SSH_USER}"
NANO_SSH_PORT="${NANO_SSH_PORT:-22}"
NANO_VIA_WORKSTATION="${NANO_VIA_WORKSTATION:-0}" # 1 = run nano checks from workstation (uses its SSH keys / network)

fail=0
errors=()

print_step() {
  echo ""
  echo "== $1 =="
}

ssh_opts_common=(
  -o ConnectTimeout="$SSH_CONNECT_TIMEOUT"
  -o ServerAliveInterval=5
  -o ServerAliveCountMax=1
  -o StrictHostKeyChecking=accept-new
)

ssh_run() {
  # Usage: ssh_run <batchmode:yes|no> <port> <target> <command>
  local batchmode="$1"; shift
  local port="$1"; shift
  local target="$1"; shift
  local cmd="$1"
  ssh "${ssh_opts_common[@]}" -o BatchMode="$batchmode" -p "$port" "$target" "$cmd"
}

ssh_run_password() {
  # Usage: ssh_run_password <port> <target> <command>
  local port="$1"; shift
  local target="$1"; shift
  local cmd="$1"

  if [[ -z "$SSH_PASS" ]]; then
    return 2
  fi
  if ! command -v sshpass >/dev/null 2>&1; then
    return 3
  fi

  # Use password auth non-interactively. This is only for reachability checks.
  sshpass -p "$SSH_PASS" ssh "${ssh_opts_common[@]}" \
    -o BatchMode=no \
    -o PreferredAuthentications=password \
    -o PubkeyAuthentication=no \
    -p "$port" "$target" "$cmd"
}

print_step "UI heartbeat"
if command -v curl >/dev/null 2>&1; then
  if curl -fsS "$UI_URL" | head -n 40; then
    echo "OK: UI reachable at $UI_URL"
  else
    echo "ERROR: UI heartbeat failed: $UI_URL" >&2
    errors+=("UI heartbeat failed: $UI_URL")
    fail=1
  fi
else
  echo "ERROR: curl not found" >&2
  exit 1
fi

print_step "SSH reachability"
# Quick TCP probe (helps distinguish network vs service/auth issues)
if command -v nc >/dev/null 2>&1; then
  echo "TCP probe: ${WORKSTATION_IP}:${WORKSTATION_SSH_PORT}"
  if ! nc -vz -G 2 "$WORKSTATION_IP" "$WORKSTATION_SSH_PORT" >/dev/null 2>&1; then
    echo "WARN: TCP probe failed (service may be down or blocked)" >&2
  fi
fi

ssh_ok=0
ssh_mode_used=""
workstation_batchmode="yes"

try_ssh_key_only() {
  ssh_run yes "$WORKSTATION_SSH_PORT" "$SSH_TARGET" 'hostname && uptime'
}

try_ssh_interactive() {
  ssh_run no "$WORKSTATION_SSH_PORT" "$SSH_TARGET" 'hostname && uptime'
}

try_ssh_password_noninteractive() {
  ssh_run_password "$WORKSTATION_SSH_PORT" "$SSH_TARGET" 'hostname && uptime'
}

if [[ "$SSH_BATCHMODE" == "yes" ]]; then
  if try_ssh_key_only; then
    ssh_ok=1
    ssh_mode_used="key"
  fi
elif [[ "$SSH_BATCHMODE" == "no" ]]; then
  if try_ssh_password_noninteractive; then
    ssh_ok=1
    ssh_mode_used="password-noninteractive"
  elif try_ssh_interactive; then
    ssh_ok=1
    ssh_mode_used="interactive"
  fi
else
  # auto
  if try_ssh_key_only; then
    ssh_ok=1
    ssh_mode_used="key"
  elif try_ssh_password_noninteractive; then
    ssh_ok=1
    ssh_mode_used="password-noninteractive"
  elif [[ -t 0 ]]; then
    echo "NOTE: key auth failed; attempting interactive SSH (may prompt for password)…" >&2
    if try_ssh_interactive; then
      ssh_ok=1
      ssh_mode_used="interactive"
    fi
  else
    echo "NOTE: key auth failed; non-interactive session so password auth not attempted." >&2
  fi
fi

if [[ $ssh_ok -eq 1 ]]; then
  if [[ "$ssh_mode_used" == "password-noninteractive" ]]; then
    echo "OK: SSH works (password via sshpass) to $SSH_TARGET"
    echo "NOTE: For best security and automation, switch to key-based auth." >&2
  elif [[ "$ssh_mode_used" == "interactive" ]]; then
    echo "OK: SSH works (interactive/password auth) to $SSH_TARGET"
    echo "NOTE: For automation, set up an SSH key and re-run with SSH_BATCHMODE=yes." >&2
  else
    echo "OK: SSH works (key auth) to $SSH_TARGET"
  fi

  if [[ "$ssh_mode_used" == "interactive" ]]; then
    workstation_batchmode="no"
  fi
else
  echo "ERROR: SSH failed (target: $SSH_TARGET)." >&2
  echo "If password login works but this script fails, re-run in a terminal with: SSH_BATCHMODE=no" >&2
  echo "If you want non-interactive password checks and have sshpass installed:" >&2
  echo "  SSH_PASS='<your-password>' SSH_BATCHMODE=no ./scripts/verify_orkhestra_connectivity.sh" >&2
  echo "If you want key-based login:" >&2
  echo "  ssh-keygen -t ed25519 -C \"mac-mini\"" >&2
  echo "  ssh-copy-id ${SSH_USER}@${WORKSTATION_IP}" >&2
  echo "And ensure ~/.ssh/config has Host $SSH_HOST_ALIAS pointing at ${WORKSTATION_IP}." >&2
  errors+=("SSH failed: $SSH_TARGET")
  fail=1
fi

run_on_workstation() {
  # Usage: run_on_workstation <command>
  # Runs a command on the workstation (uses whatever auth succeeded above).
  local cmd="$1"
  if [[ $ssh_ok -ne 1 ]]; then
    return 4
  fi
  ssh_run "$workstation_batchmode" "$WORKSTATION_SSH_PORT" "$SSH_TARGET" "$cmd"
}

print_step "Repo path exists (on workstation)"
if [[ $ssh_ok -eq 1 ]]; then
  repo_check_batchmode="yes"
  if [[ "$ssh_mode_used" == "interactive" ]]; then
    repo_check_batchmode="no"
  fi
  if ssh_run "$repo_check_batchmode" "$WORKSTATION_SSH_PORT" "$SSH_TARGET" "test -d '$REPO_PATH_ON_WORKSTATION' && echo OK || (echo MISSING; exit 2)"; then
    echo "OK: repo path exists: $REPO_PATH_ON_WORKSTATION"
  else
    echo "ERROR: Repo path missing: $REPO_PATH_ON_WORKSTATION" >&2
    errors+=("Repo path missing: $REPO_PATH_ON_WORKSTATION")
    fail=1
  fi
else
  echo "SKIP: repo path check (SSH unavailable)"
fi

if [[ "$CHECK_NANO_OLLAMA" == "1" ]]; then
  print_step "Nano nodes (Ollama): /api/tags"
else
  print_step "Nano nodes"
  echo "NOTE: Ollama checks are disabled (CHECK_NANO_OLLAMA=0)"
fi
for entry in "${NANO_NODES[@]}"; do
  name="${entry%%:*}"
  ip="${entry##*:}"
  url="http://${ip}:11434/api/tags"
  echo "-- ${name} (${ip}) --"

  if [[ "$CHECK_NANO_OLLAMA" == "1" ]]; then
    if [[ "$NANO_VIA_WORKSTATION" == "1" ]]; then
      if run_on_workstation "curl -fsS http://${ip}:11434/api/tags | head -n 25"; then
        :
      else
        echo "ERROR: Ollama tags failed (via workstation): $url" >&2
        errors+=("Ollama tags failed (via workstation): ${name} (${ip})")
        fail=1
      fi
    else
      if curl -fsS "$url" | head -n 25; then
        :
      else
        echo "ERROR: Ollama tags failed: $url" >&2
        errors+=("Ollama tags failed: ${name} (${ip})")
        fail=1

        if command -v nc >/dev/null 2>&1; then
          echo "TCP probe: ${ip}:11434"
          nc -vz -G 2 "$ip" 11434 >/dev/null 2>&1 || true
        fi
      fi
    fi
  else
    echo "SKIP: Ollama HTTP check disabled (CHECK_NANO_OLLAMA=0)"
  fi

  if [[ "$CHECK_NANO_SSH" == "1" ]]; then
    nano_target="${NANO_SSH_USER}@${ip}"
    echo "SSH probe: ${nano_target}:${NANO_SSH_PORT}"
    if [[ "$NANO_VIA_WORKSTATION" == "1" ]]; then
      if run_on_workstation "ssh -o BatchMode=yes -o ConnectTimeout=${SSH_CONNECT_TIMEOUT} -p ${NANO_SSH_PORT} ${nano_target} 'hostname && uptime'"; then
        echo "OK: nano SSH works (via workstation): ${nano_target}"
      else
        echo "WARN: nano SSH failed (via workstation): ${nano_target}" >&2
      fi
    else
      if ssh_run yes "$NANO_SSH_PORT" "$nano_target" 'hostname && uptime'; then
        echo "OK: nano SSH works: ${nano_target}"
      else
        echo "WARN: nano SSH failed: ${nano_target}" >&2
      fi
    fi
  fi
  echo ""
done

if [[ $fail -eq 0 ]]; then
  echo "OK: all checks passed."
  exit 0
else
  if ((${#errors[@]} > 0)); then
    echo "" >&2
    echo "Failures:" >&2
    for e in "${errors[@]}"; do
      echo "- $e" >&2
    done
  fi
  echo "ERROR: one or more checks failed." >&2
  exit 1
fi