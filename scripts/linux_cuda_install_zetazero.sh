#!/usr/bin/env bash
set -euo pipefail

# Minimal installer: build ZETA-enabled llama.cpp with CUDA on Ubuntu.
# Assumes this directory contains:
#   - llama.cpp/
#   - scripts/ubuntu_setup.sh
#   - scripts/ubuntu_build_llama_cpp.sh
#   - scripts/ubuntu_smoke_llama_cpp.sh

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ ! -d llama.cpp ]]; then
  echo "ERROR: llama.cpp/ not found under $repo_root" >&2
  exit 1
fi

if ! command -v nvidia-smi >/dev/null 2>&1; then
  echo "ERROR: nvidia-smi not found. Install NVIDIA driver first." >&2
  exit 2
fi

echo "[0/3] NVIDIA driver check"
nvidia-smi || true

if ! command -v nvcc >/dev/null 2>&1; then
  echo "ERROR: nvcc not found. You likely have the driver but not the CUDA toolkit." >&2
  echo "Install the CUDA toolkit, then re-run." >&2
  exit 3
fi

echo "CUDA toolkit:" 
nvcc --version | head -n 5 || true

echo "[1/3] Installing build deps (Ubuntu)"
./scripts/ubuntu_setup.sh

echo "[2/3] Building llama.cpp with CUDA"
./scripts/ubuntu_build_llama_cpp.sh --clean --cuda

echo "[3/3] Smoke check"
./scripts/ubuntu_smoke_llama_cpp.sh

echo "OK: ZetaZero CUDA build complete"
