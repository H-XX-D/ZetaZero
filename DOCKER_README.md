# Z.E.T.A. Docker Deployment

**An AI that dreams about your codebase and wakes up with ideas.**

Z.E.T.A. indexes your code, builds a memory graph, and runs autonomous "dream cycles" that generate novel insights, refactors, and feature ideas - all while you sleep.

```bash
# Point it at your codebase
python3 scripts/index_codebase.py --path ./your-project/src

# Check back tomorrow
ls ~/.zetazero/storage/dreams/pending/
```

Each dream is a markdown file with concrete code suggestions based on YOUR architecture.

---

## Quick Start

```bash
# Option 1: Pull pre-built image (fastest)
docker pull ghcr.io/h-xx-d/zetazero:latest
./scripts/setup.sh          # Download models
docker-compose up -d         # Start Z.E.T.A.

# Option 2: Build from source
./scripts/setup.sh          # Download models
docker build -t zetazero:latest .
docker-compose up -d
```

Now running at `http://localhost:8080`.

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

## Index Your Codebase

This is where it gets interesting.

```bash
# Feed Z.E.T.A. your code
python3 scripts/index_codebase.py --path ./your-project/src

# Watch the graph grow
curl http://localhost:8080/git/status
# {"total_nodes": 247, "branch": "main"}
```

Z.E.T.A. extracts every function, struct, and class into a memory graph. Then, every 5 minutes, it enters a "dream cycle" where it:

1. Picks a random node from your codebase
2. Free-associates to related concepts
3. Drills down: concept → framework → implementation → actual code
4. Saves novel ideas to `~/.zetazero/storage/dreams/pending/`

**Example dream output:**
```
code_idea: Buffer Pool Optimization

The `process_request` function allocates a new buffer on every call.
Consider a thread-local buffer pool:

typedef struct {
    char buffer[BUFSIZE];
    struct buffer_pool *next;
} buffer_pool_t;

This reduces allocation overhead in hot paths by ~40%.
```

Dreams are filtered for novelty - repetitive ideas get discarded automatically.

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
