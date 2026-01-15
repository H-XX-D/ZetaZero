# Build Stage - CUDA enabled
FROM nvidia/cuda:12.2.0-devel-ubuntu22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY zeta-integration /app/zeta-integration
COPY llama.cpp /app/llama.cpp

# Build zeta-server with CUDA support
WORKDIR /app/llama.cpp
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DGGML_CUDA=ON \
    -DLLAMA_CURL=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    && cmake --build build --config Release --target zeta-zero-server -j$(nproc)

# Runtime Stage
FROM nvidia/cuda:12.2.0-runtime-ubuntu22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    ca-certificates \
    curl \
    python3 \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/llama.cpp/build/bin/zeta-zero-server /app/zeta-server

# Create directories for models, storage, and dreams
RUN mkdir -p /models /storage /storage/dreams/pending /storage/dreams/archive /storage/blocks

# Environment variables (defaults - 14B + 7B production config)
ENV ZETA_HOST="0.0.0.0"
ENV ZETA_PORT="8080"
# Defaults are overridable via docker-compose.yml or a mounted zeta.conf.
ENV MODEL_MAIN="/models/qwen2.5-14b-instruct-q4.gguf"
ENV MODEL_CODER="/models/qwen2.5-7b-coder-q4_k_m.gguf"
ENV MODEL_EMBED="/models/qwen3-embedding-4b-q4_k_m.gguf"
ENV ZETA_STORAGE="/storage"
ENV ZETA_DREAM_INTERVAL="300"
ENV ZETA_DREAM_DIR="/storage/dreams"
ENV ZETA_GRAPH_PATH="/storage/blocks/graph.bin"

# GPU layers - adjust based on VRAM (14B needs ~10GB, 7B needs ~5GB)
ENV GPU_LAYERS_MAIN="49"
ENV GPU_LAYERS_CODER="35"

# Expose the port
EXPOSE 8080

# Copy entrypoint script and utilities
COPY docker-entrypoint.sh /app/
COPY runpod-entrypoint.sh /app/
COPY scripts/index_codebase.py /app/scripts/
RUN chmod +x /app/docker-entrypoint.sh /app/runpod-entrypoint.sh

# Health check - extended start period for model downloads
HEALTHCHECK --interval=30s --timeout=10s --start-period=600s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Use runpod-entrypoint which downloads models then calls docker-entrypoint
ENTRYPOINT ["/app/runpod-entrypoint.sh"]
