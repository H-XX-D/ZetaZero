#!/usr/bin/env bash
# Z.E.T.A. One-shot installer for new users
# - Optionally clones the repo
# - Checks Docker + Docker Compose
# - Checks NVIDIA container runtime (Linux, if GPU present)
# - Runs ./quickstart.sh to generate per-user config and start the container

set -euo pipefail

REPO_URL_DEFAULT="https://github.com/H-XX-D/ZetaZero.git"
TARGET_DIR_DEFAULT="ZetaZero"

MODE=""        # "--lite" or "--full" (empty -> quickstart auto)
UNLOCK=""      # "--unlock" or empty
NO_GPU_CHECK=false
SKIP_CLONE=false
REPO_URL="$REPO_URL_DEFAULT"
TARGET_DIR="$TARGET_DIR_DEFAULT"

usage() {
  cat <<EOF
Usage:
  ./install.sh [options]

Options:
  --repo <url>        Repo URL to clone (default: $REPO_URL_DEFAULT)
  --dir <path>        Target directory (default: $TARGET_DIR_DEFAULT)
  --skip-clone        Assume you're already inside the repo; do not clone
  --lite              Run quickstart in lite mode
  --full              Run quickstart in full mode
  --unlock            Run quickstart unlocked
  --no-gpu-check       Skip NVIDIA container runtime check
  -h, --help          Show help

Examples:
  ./install.sh
  ./install.sh --lite
  ./install.sh --repo https://github.com/H-XX-D/ZetaZero.git --dir ./ZetaZero
EOF
}

err() { echo "ERROR: $*" >&2; }
info() { echo "[install] $*"; }

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { err "Missing dependency: $1"; return 1; }
}

os_name() {
  uname -s | tr '[:upper:]' '[:lower:]'
}

check_docker() {
  need_cmd docker || return 1

  if ! docker info >/dev/null 2>&1; then
    err "Docker daemon not running or permission denied."
    case "$(os_name)" in
      darwin)
        echo "Install Docker Desktop and start it: https://www.docker.com/products/docker-desktop/" >&2
        ;;
      linux)
        echo "Start Docker (systemd): sudo systemctl enable --now docker" >&2
        echo "If permissions: sudo usermod -aG docker \"$USER\" && newgrp docker" >&2
        ;;
      *)
        echo "Start Docker and ensure you can run 'docker info'." >&2
        ;;
    esac
    return 1
  fi

  if docker compose version >/dev/null 2>&1; then
    :
  else
    err "Docker Compose v2 not found (expected: 'docker compose ...')."
    case "$(os_name)" in
      darwin)
        echo "Docker Desktop includes Compose v2." >&2
        ;;
      linux)
        echo "Install Compose plugin: https://docs.docker.com/compose/install/linux/" >&2
        ;;
    esac
    return 1
  fi
}

check_gpu_runtime_linux() {
  # Only meaningful on Linux.
  if [ "$(os_name)" != "linux" ]; then
    return 0
  fi

  if $NO_GPU_CHECK; then
    info "Skipping GPU runtime check (--no-gpu-check)."
    return 0
  fi

  # If no nvidia-smi, user might still be CPU-only; do not hard fail.
  if ! command -v nvidia-smi >/dev/null 2>&1; then
    info "nvidia-smi not found; assuming CPU-only or drivers not installed."
    info "You can still run, but GPU mode will not work without NVIDIA drivers + nvidia-container-toolkit."
    return 0
  fi

  info "Detected NVIDIA drivers (nvidia-smi present). Checking NVIDIA container runtime..."

  # This is the best practical runtime test.
  if docker run --rm --gpus all nvidia/cuda:12.2.0-base-ubuntu22.04 nvidia-smi >/dev/null 2>&1; then
    info "NVIDIA container runtime OK."
    return 0
  fi

  err "GPU detected, but Docker can't access it (nvidia-container-toolkit likely missing)."
  echo "Install NVIDIA container toolkit, then restart Docker:" >&2
  echo "  https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html" >&2
  echo "Then verify:" >&2
  echo "  docker run --rm --gpus all nvidia/cuda:12.2.0-base-ubuntu22.04 nvidia-smi" >&2
  return 1
}

clone_or_enter_repo() {
  if $SKIP_CLONE; then
    return 0
  fi

  need_cmd git || return 1

  if [ -d "$TARGET_DIR/.git" ]; then
    info "Repo already exists at $TARGET_DIR (skipping clone)."
    return 0
  fi

  info "Cloning $REPO_URL -> $TARGET_DIR"
  git clone "$REPO_URL" "$TARGET_DIR"
}

run_quickstart() {
  local dir="$1"
  cd "$dir"

  if [ -x "./scripts/doctor.sh" ]; then
    info "Running dependency checker (doctor)..."
    if $NO_GPU_CHECK; then
      ./scripts/doctor.sh --no-gpu-check
    else
      ./scripts/doctor.sh
    fi
  fi

  if [ ! -x "./quickstart.sh" ]; then
    err "./quickstart.sh not found or not executable in $dir"
    return 1
  fi

  info "Starting quickstart (this will prompt for paths and generate per-user config in ./.zetazero/)"
  # shellcheck disable=SC2086
  ./quickstart.sh ${UNLOCK} ${MODE}
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo)
      REPO_URL="$2"; shift 2 ;;
    --dir)
      TARGET_DIR="$2"; shift 2 ;;
    --skip-clone)
      SKIP_CLONE=true; shift 1 ;;
    --lite)
      MODE="--lite"; shift 1 ;;
    --full)
      MODE="--full"; shift 1 ;;
    --unlock)
      UNLOCK="--unlock"; shift 1 ;;
    --no-gpu-check)
      NO_GPU_CHECK=true; shift 1 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      err "Unknown arg: $1"; usage; exit 2 ;;
  esac
done

info "Checking Docker + Compose..."
check_docker
check_gpu_runtime_linux

clone_or_enter_repo

if $SKIP_CLONE; then
  run_quickstart "$(pwd)"
else
  run_quickstart "$TARGET_DIR"
fi
