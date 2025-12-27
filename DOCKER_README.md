# Z.E.T.A. Docker Deployment

Run Z.E.T.A. in Docker with full CUDA support, dream system, and persistent memory.

## Quick Start (One Command)

```bash
# Download models + create directories
./scripts/setup.sh

# Build and run
docker-compose up -d
```

That's it. You now have Z.E.T.A. running at `http://localhost:8080`.

## Prerequisites

- NVIDIA GPU with CUDA 12.x
- Docker with nvidia-container-toolkit
- ~20GB disk space for models

## Setup Script

The setup script handles everything:

```bash
./scripts/setup.sh
```

It will:
1. Create required directories
2. Ask which model size (14B+7B, 7B+3B, or 3B+3B)
3. Download models from HuggingFace
4. Configure storage paths

### Model Sizes

| Config | VRAM Required | Download Size |
|--------|---------------|---------------|
| Production (14B+7B) | 16GB+ | ~15GB |
| Lite (7B+3B) | 8GB+ | ~8GB |
| Minimal (3B+3B) | 6GB+ | ~4GB |

## Build Options

### x86_64 (Desktop/Server)
```bash
docker build -t zetazero:latest .
```

### Jetson (aarch64)
```bash
# First build the binary on Jetson
cd llama.cpp && cmake -B build -DGGML_CUDA=ON && cmake --build build -t zeta-server
cp build/bin/zeta-server ../dist/zeta-server-aarch64

# Then build Docker image
docker build -t zetazero:latest -f Dockerfile.jetson .
```

## Configuration

Environment variables (set in docker-compose.yml):

| Variable | Default | Description |
|----------|---------|-------------|
| `MODEL_MAIN` | 14B model path | Main conscious model |
| `MODEL_CODER` | 7B model path | Code specialist model |
| `MODEL_EMBED` | embed model path | Embedding model |
| `GPU_LAYERS_MAIN` | 49 | GPU layers for main model |
| `GPU_LAYERS_CODER` | 35 | GPU layers for coder model |
| `ZETA_DREAM_INTERVAL` | 300 | Seconds between dream cycles |
| `ZETA_PORT` | 8080 | HTTP API port |

## Persistent Storage

Data is stored in `~/.zetazero/storage/`:
```
~/.zetazero/storage/
├── blocks/
│   └── graph.bin      # Memory graph
└── dreams/
    ├── pending/       # New dreams
    └── archive/       # Reviewed dreams
```

## Swarm Deployment

1. Build image on one node:
   ```bash
   docker build -t zetazero:latest .
   docker save zetazero:latest | gzip > zetazero.tar.gz
   ```

2. Distribute to nodes:
   ```bash
   scp zetazero.tar.gz docker-compose.yml node1:~/
   ```

3. Load and run on each node:
   ```bash
   docker load < zetazero.tar.gz
   docker-compose up -d
   ```

## Index Your Codebase (Optional)

Let Z.E.T.A. dream about your own code:

```bash
# Index a codebase into Z.E.T.A.'s memory graph
python3 scripts/index_codebase.py --path ./your-project/src

# Or from inside Docker
docker exec zeta python3 /app/scripts/index_codebase.py --path /your-mounted-code
```

This extracts functions, structs, and classes into the memory graph. Z.E.T.A. will then dream about your code and generate ideas.

## Commands

```bash
# View logs
docker logs -f zeta

# Check health
curl http://localhost:8080/health

# Check dream status
curl http://localhost:8080/git/status

# Stop
docker-compose down

# Restart
docker-compose restart
```

## Troubleshooting

**CUDA not detected:**
```bash
# Verify nvidia runtime
docker run --rm --runtime=nvidia nvidia/cuda:12.2.0-base-ubuntu22.04 nvidia-smi
```

**Out of memory:**
- Reduce `GPU_LAYERS_MAIN` and `GPU_LAYERS_CODER`
- Or use the lite profile: `docker-compose --profile lite up -d`

**Models not found:**
- Check `~/models/` contains the correct .gguf files
- Verify volume mount in docker-compose.yml
