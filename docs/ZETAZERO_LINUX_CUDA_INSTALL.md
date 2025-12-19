# ZetaZero local install (Linux + CUDA)

This is the minimal path to build the ZETA-enabled `llama.cpp` binary (`llama-zeta-demo`) on an Ubuntu workstation with NVIDIA CUDA.

## What you need on the workstation

- Ubuntu (or Debian-based)
- NVIDIA driver working (`nvidia-smi` succeeds)
- CUDA toolkit installed (CMake must find it)

## Fast path

From the repo root:

```bash
./scripts/ubuntu_setup.sh
./scripts/ubuntu_build_llama_cpp.sh --clean --cuda
./scripts/ubuntu_smoke_llama_cpp.sh
```

Binaries:

- `llama.cpp/build-ubuntu/bin/llama-zeta-demo`
- `llama.cpp/build-ubuntu/bin/llama-cli`

## If CMake can’t find CUDA

Make sure the CUDA toolkit is installed and on PATH. Common checks:

```bash
nvidia-smi
nvcc --version
```

If `nvcc` is missing, you likely only have the driver installed (not the toolkit).

## Verify you built CUDA (on the workstation)

After building, confirm CMake enabled CUDA:

```bash
grep '^GGML_CUDA:BOOL=' llama.cpp/build-ubuntu/CMakeCache.txt
```

Expected:

```text
GGML_CUDA:BOOL=ON
```

## Transfer only what’s needed (from this Mac → workstation)

If you don’t want to copy the whole repo, transfer just:

- `llama.cpp/` (this contains the ZETA integration + tools)
- `scripts/ubuntu_setup.sh`
- `scripts/ubuntu_build_llama_cpp.sh`
- `scripts/ubuntu_smoke_llama_cpp.sh`

Example (run on the Mac, adjust destination path):

```bash
rsync -av --delete \
  ./llama.cpp \
  ./scripts/ubuntu_setup.sh \
  ./scripts/ubuntu_build_llama_cpp.sh \
  ./scripts/ubuntu_smoke_llama_cpp.sh \
  xx@192.168.0.165:~/ZetaZero/
```

Then on the workstation:

```bash
cd ~/ZetaZero
./scripts/ubuntu_setup.sh
./scripts/ubuntu_build_llama_cpp.sh --cuda
./scripts/ubuntu_smoke_llama_cpp.sh
```
