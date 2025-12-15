#!/usr/bin/env bash
set -euo pipefail

# Minimal smoke check for an Ubuntu build directory.

build_dir="llama.cpp/build-ubuntu"

if [[ $# -gt 0 ]]; then
  build_dir="$1"
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

zeta_demo="$repo_root/$build_dir/bin/llama-zeta-demo"
llama_cli="$repo_root/$build_dir/bin/llama-cli"

if [[ ! -x "$zeta_demo" ]]; then
  echo "ERROR: not found/executable: $zeta_demo" >&2
  exit 1
fi

if [[ ! -x "$llama_cli" ]]; then
  echo "ERROR: not found/executable: $llama_cli" >&2
  exit 1
fi

echo "== llama-zeta-demo --help =="
"$zeta_demo" --help | head -n 40

echo ""
echo "== llama-cli --help =="
"$llama_cli" --help | head -n 40

echo ""
echo "OK: smoke check passed."