#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/ubuntu_build_llama_cpp.sh [--clean] [--cuda] [--build-dir DIR] [--jobs N]

Builds the repo's llama.cpp fork on Ubuntu (CPU by default).

Options:
  --clean           Remove build dir before configuring
  --cuda            Build with CUDA backend enabled (requires CUDA toolkit installed)
  --build-dir DIR   Build directory (default: llama.cpp/build-ubuntu)
  --jobs N          Parallel jobs for build
EOF
}

build_dir="llama.cpp/build-ubuntu"
clean=0
cuda=0
jobs=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      build_dir="$2"
      shift 2
      ;;
    --clean)
      clean=1
      shift
      ;;
    --cuda)
      cuda=1
      shift
      ;;
    --jobs)
      jobs="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown arg: $1" >&2
      usage
      exit 2
      ;;
  esac
done

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ ! -d "llama.cpp" ]]; then
  echo "ERROR: expected llama.cpp/ under repo root: $repo_root" >&2
  exit 1
fi

if [[ $clean -eq 1 ]]; then
  rm -rf "$build_dir"
fi

cmake_args=(
  -S llama.cpp
  -B "$build_dir"
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DLLAMA_BUILD_TESTS=OFF
  -DLLAMA_BUILD_EXAMPLES=ON
  -DLLAMA_BUILD_TOOLS=ON
  -DGGML_METAL=OFF
)

if [[ $cuda -eq 1 ]]; then
  if ! command -v nvcc >/dev/null 2>&1; then
    echo "ERROR: --cuda requested but nvcc not found." >&2
    echo "Install the CUDA toolkit (not just the NVIDIA driver), then re-run." >&2
    echo "Quick check: nvcc --version" >&2
    exit 3
  fi
  cmake_args+=(
    -DGGML_CUDA=ON
  )
else
  cmake_args+=(
    -DGGML_CUDA=OFF
  )
fi

echo "[1/2] Configuring: $build_dir"
cmake "${cmake_args[@]}"

if [[ $cuda -eq 1 ]]; then
  cache_file="$build_dir/CMakeCache.txt"
  if [[ ! -f "$cache_file" ]]; then
    echo "ERROR: CMakeCache.txt not found at $cache_file" >&2
    exit 4
  fi
  if ! grep -q '^GGML_CUDA:BOOL=ON$' "$cache_file"; then
    echo "ERROR: --cuda was requested but GGML_CUDA was not enabled by CMake." >&2
    echo "Common fixes:" >&2
    echo "  - Ensure CUDA toolkit is installed and on PATH (nvcc --version)" >&2
    echo "  - Ensure CMake can find CUDA (check CMake output for CUDA)" >&2
    echo "  - Try a clean rebuild: ./scripts/ubuntu_build_llama_cpp.sh --clean --cuda" >&2
    exit 5
  fi
fi

echo "[2/2] Building"
if [[ -n "$jobs" ]]; then
  cmake --build "$build_dir" -- -j"$jobs"
else
  cmake --build "$build_dir"
fi

echo ""
echo "OK: build complete"
echo "Binaries:"
echo "  $repo_root/$build_dir/bin/llama-zeta-demo"
echo "  $repo_root/$build_dir/bin/llama-cli"
