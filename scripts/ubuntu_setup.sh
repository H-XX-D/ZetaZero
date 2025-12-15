#!/usr/bin/env bash
set -euo pipefail

# Ubuntu-first dependency bootstrap for this repo's llama.cpp fork.
# Installs only build prerequisites (no CUDA toolkit).

if ! command -v apt-get >/dev/null 2>&1; then
  echo "ERROR: apt-get not found. This script targets Ubuntu/Debian." >&2
  exit 1
fi

sudo apt-get update

# Core build tooling
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  git \
  python3 \
  python3-pip \
  ca-certificates \
  curl

# llama.cpp defaults to LLAMA_CURL=ON, so install libcurl dev headers.
sudo apt-get install -y \
  libcurl4-openssl-dev

# Optional (recommended if you want HTTPS model downloads via OpenSSL directly):
# sudo apt-get install -y libssl-dev

# Optional (often beneficial for CPU perf):
# sudo apt-get install -y libopenblas-dev

echo "OK: Ubuntu build deps installed."