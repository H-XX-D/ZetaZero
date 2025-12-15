# Ubuntu build: llama.cpp (ZetaLm fork)

This repo contains a `llama.cpp/` fork with the `llama-zeta-demo` tool.

## 1) Install dependencies (Ubuntu)

```bash
./scripts/ubuntu_setup.sh
```

## 2) Build (CPU)

```bash
./scripts/ubuntu_build_llama_cpp.sh
```

Outputs are placed under:
- `llama.cpp/build-ubuntu/bin/llama-zeta-demo`
- `llama.cpp/build-ubuntu/bin/llama-cli`

## 3) Smoke check

```bash
./scripts/ubuntu_smoke_llama_cpp.sh
```

## 4) Build with CUDA (optional)

Prereq: CUDA toolkit installed and discoverable by CMake.

```bash
./scripts/ubuntu_build_llama_cpp.sh --cuda
```
