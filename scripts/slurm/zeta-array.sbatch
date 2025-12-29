#!/bin/bash
# =============================================================================
# Z.E.T.A. Slurm Array Job - Multi-Node GPU Farm
# =============================================================================
# Submit N nodes: sbatch --array=0-3 zeta-array.sbatch
# =============================================================================

#SBATCH --job-name=zeta-farm
#SBATCH --partition=gpu
#SBATCH --gres=gpu:1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=24:00:00
#SBATCH --output=logs/zeta-%A_%a.out
#SBATCH --error=logs/zeta-%A_%a.err

set -e

# Find repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
cd "$REPO_ROOT"

# Load config
if [[ -f "zeta.conf" ]]; then
    set -a
    source zeta.conf
    set +a
fi

# Each array task gets a unique port
BASE_PORT=${ZETA_BASE_PORT:-8080}
NODE_PORT=$((BASE_PORT + SLURM_ARRAY_TASK_ID))
NODE_HOST=$(hostname -f)

echo "=============================================="
echo "  Z.E.T.A. Farm Node ${SLURM_ARRAY_TASK_ID}"
echo "=============================================="
echo "  Array Job: ${SLURM_ARRAY_JOB_ID}"
echo "  Task ID:   ${SLURM_ARRAY_TASK_ID}"
echo "  Node:      ${NODE_HOST}"
echo "  Port:      ${NODE_PORT}"
echo "=============================================="

mkdir -p logs

# Register with coordinator
if [[ -n "$ZETA_COORDINATOR" ]]; then
    curl -s -X POST "${ZETA_COORDINATOR}/register" \
        -H "Content-Type: application/json" \
        -d "{\"host\":\"${NODE_HOST}\",\"port\":${NODE_PORT},\"task_id\":${SLURM_ARRAY_TASK_ID}}" \
        || true
fi

# Shared storage for graph (NFS/Lustre mount point)
SHARED_STORAGE="${ZETA_SHARED_STORAGE:-/scratch/${USER}/zeta-storage}"
mkdir -p "$SHARED_STORAGE"

ZETA_SERVER="${ZETA_BUILD_DIR:-build}/bin/zeta-server"
[[ ! -f "$ZETA_SERVER" ]] && ZETA_SERVER="./zeta-server"

exec "$ZETA_SERVER" \
    --host 0.0.0.0 \
    --port "$NODE_PORT" \
    -m "${MODEL_14B}" \
    --model-7b-coder "${MODEL_7B_CODER}" \
    --embed-model "${MODEL_EMBED}" \
    -ngl "${GPU_LAYERS:-999}" \
    --zeta-storage "$SHARED_STORAGE"
