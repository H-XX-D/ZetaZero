#!/usr/bin/env bash
# Z.E.T.A. dependency checker ("doctor")
#
# Goal: frictionless first-run experience.
# This script checks host prerequisites and prints actionable fixes.

set -euo pipefail

NO_GPU_CHECK=false
QUIET=false

usage() {
  cat <<EOF
Usage:
  ./scripts/doctor.sh [--no-gpu-check] [--quiet]

Checks:
  - docker + daemon running
  - docker compose (v2)
  - git
  - download tool for models (huggingface-cli OR curl OR wget)
  - (Linux + GPU) NVIDIA container runtime (docker --gpus all)

Exit codes:
  0 = OK
  1 = Missing/failed dependency
EOF
}

os_name() { uname -s | tr '[:upper:]' '[:lower:]'; }

say() {
  $QUIET && return 0
  echo "$@"
}

fail() {
  echo "ERROR: $*" >&2
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1
}

check_docker() {
  if ! need_cmd docker; then
    fail "Docker not found."
    case "$(os_name)" in
      darwin)
        echo "Install Docker Desktop: https://www.docker.com/products/docker-desktop/" >&2
        ;;
      linux)
        echo "Install Docker Engine: https://docs.docker.com/engine/install/" >&2
        ;;
      *)
        echo "Install Docker and ensure it's on PATH." >&2
        ;;
    esac
    return 1
  fi

  if ! docker info >/dev/null 2>&1; then
    fail "Docker daemon not running or permission denied (docker info failed)."
    case "$(os_name)" in
      darwin)
        echo "Start Docker Desktop." >&2
        ;;
      linux)
        echo "Try: sudo systemctl enable --now docker" >&2
        echo "If permissions: sudo usermod -aG docker \"$USER\" && newgrp docker" >&2
        ;;
      *)
        echo "Start Docker daemon and retry." >&2
        ;;
    esac
    return 1
  fi

  if ! docker compose version >/dev/null 2>&1; then
    fail "Docker Compose v2 not found (expected: 'docker compose ...')."
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

  return 0
}

check_git() {
  if ! need_cmd git; then
    fail "git not found."
    case "$(os_name)" in
      darwin)
        echo "Install Xcode Command Line Tools: xcode-select --install" >&2
        ;;
      linux)
        echo "Install git (Debian/Ubuntu): sudo apt-get update && sudo apt-get install -y git" >&2
        ;;
    esac
    return 1
  fi
  return 0
}

check_python() {
  if need_cmd python3; then
    say "[doctor] Python 3: OK ($(python3 --version 2>&1))"
    return 0
  fi
  fail "Python 3 not found (needed for graph seeding scripts)."
  case "$(os_name)" in
    darwin)
      echo "Install Python: brew install python3" >&2
      ;;
    linux)
      echo "Install Python: sudo apt-get update && sudo apt-get install -y python3" >&2
      ;;
  esac
  return 1
}

check_huggingface_cli() {
  if need_cmd huggingface-cli; then
    say "[doctor] huggingface-cli: OK (recommended for model downloads)"
    return 0
  fi
  say "[doctor] huggingface-cli: not found (will use curl/wget instead)"
  say "         Install for faster downloads: pip install huggingface_hub[cli]"
  return 0  # Not a hard requirement
}

check_download_tools() {
  if need_cmd huggingface-cli || need_cmd curl || need_cmd wget; then
    return 0
  fi
  fail "No model download tool found (need one of: huggingface-cli, curl, wget)."
  case "$(os_name)" in
    darwin)
      echo "Install curl/wget (Homebrew): brew install curl wget" >&2
      ;;
    linux)
      echo "Install curl/wget: sudo apt-get update && sudo apt-get install -y curl wget" >&2
      ;;
  esac
  return 1
}

check_gpu_runtime_linux() {
  if [ "$(os_name)" != "linux" ]; then
    return 0
  fi
  if $NO_GPU_CHECK; then
    say "[doctor] GPU runtime check: skipped"
    return 0
  fi

  if ! need_cmd nvidia-smi; then
    say "[doctor] NVIDIA drivers: not detected (nvidia-smi missing) — CPU-only is fine"
    return 0
  fi

  say "[doctor] NVIDIA drivers detected. Testing Docker GPU access..."
  if docker run --rm --gpus all nvidia/cuda:12.2.0-base-ubuntu22.04 nvidia-smi >/dev/null 2>&1; then
    say "[doctor] GPU runtime: OK"
    return 0
  fi

  fail "GPU detected but Docker cannot access it."
  echo "Install NVIDIA Container Toolkit:" >&2
  echo "  https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html" >&2
  echo "Then verify:" >&2
  echo "  docker run --rm --gpus all nvidia/cuda:12.2.0-base-ubuntu22.04 nvidia-smi" >&2
  return 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-gpu-check)
      NO_GPU_CHECK=true; shift ;;
    --quiet)
      QUIET=true; shift ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      fail "Unknown arg: $1"; usage; exit 1 ;;
  esac
done

say "[doctor] Checking dependencies..."

ok=true
check_git || ok=false
check_docker || ok=false
check_download_tools || ok=false
check_python || ok=false
check_huggingface_cli
check_gpu_runtime_linux || ok=false

if $ok; then
  say "[doctor] OK"
  exit 0
fi

exit 1
