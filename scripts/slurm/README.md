# Z.E.T.A. Slurm Integration

Deploy Z.E.T.A. across a GPU farm using Slurm workload manager.

## Architecture

```
                    +-----------------+
                    |   Coordinator   |
                    |   (port 8000)   |
                    +--------+--------+
                             |
            +----------------+----------------+
            |                |                |
    +-------v------+ +-------v------+ +-------v------+
    | GPU Node 0   | | GPU Node 1   | | GPU Node 2   |
    | (port 8080)  | | (port 8081)  | | (port 8082)  |
    +--------------+ +--------------+ +--------------+
            \                |                /
             \               |               /
              +------------------------------+
              |     Shared Storage (NFS)     |
              |     /scratch/user/zeta       |
              +------------------------------+
```

## Quick Start

### 1. Build and Start Coordinator (on head node or login node)

```bash
cd scripts/slurm
g++ -std=c++17 -O2 -pthread coordinator.cpp -o coordinator
./coordinator --port 8000 &
```

### 2. Submit GPU Nodes

Single node:
```bash
sbatch zeta-node.sbatch
```

Multi-node farm (4 GPUs):
```bash
sbatch --array=0-3 zeta-array.sbatch
```

### 3. Point Clients at Coordinator

```bash
# OpenCode
export ZETA_API_URL="http://headnode:8000/v1"

# curl
curl http://headnode:8000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model":"zeta-cognitive","messages":[{"role":"user","content":"hello"}]}'
```

## Configuration

Add to `zeta.conf`:

```bash
# Slurm settings
ZETA_COORDINATOR="http://headnode:8000"
ZETA_BASE_PORT="8080"
ZETA_SHARED_STORAGE="/scratch/${USER}/zeta-storage"
```

## Monitoring

```bash
# Coordinator status (nodes, health)
curl http://headnode:8000/coord/health
curl http://headnode:8000/nodes

# Node health (proxied to a node)
curl http://headnode:8000/health

# Graph stats (proxied)
curl http://headnode:8000/gkv/stats

# Check Slurm jobs
squeue -u $USER
```

## Proxied Endpoints

All zeta-server endpoints are proxied through the coordinator:
- `/v1/chat/completions`, `/v1/models` - OpenAI-compatible API
- `/generate`, `/code` - Direct generation
- `/embedding`, `/embeddings` - Vector embeddings
- `/memory/query` - Memory graph queries
- `/graph`, `/gkv/stats` - Graph statistics
- `/project/*`, `/session/*` - Project management
- `/tools`, `/tool/execute` - Tool execution
- `/git/*` - Git operations
- `/health` - Node health check

## Shared Graph Storage

For Graph-KV consistency across nodes, use shared filesystem:

```bash
# NFS mount
ZETA_SHARED_STORAGE="/nfs/home/${USER}/zeta-storage"

# Lustre (HPC)
ZETA_SHARED_STORAGE="/lustre/scratch/${USER}/zeta-storage"
```

Each node reads/writes to the same graph, enabling:
- Consistent memory across all nodes
- Any node can serve any query with full context
- Dreams and consolidation can run on any node

## Load Balancing

Coordinator supports two strategies:

- `round_robin` (default) - rotate through nodes
- `least_loaded` - prefer nodes with fewer active requests

The coordinator tracks:
- Active request count per node
- Health status (30s checks)
- Auto-removes unresponsive nodes after 5 minutes

## Scaling

```bash
# Scale up - add more nodes
sbatch --array=4-7 zeta-array.sbatch

# Scale down - cancel specific jobs
scancel <job_id>
```

Nodes auto-register with coordinator on startup and auto-deregister when jobs end.
