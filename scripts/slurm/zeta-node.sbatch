#!/bin/bash
# =============================================================================
# Z.E.T.A. Slurm Job - Single GPU Node
# =============================================================================
# Submit: sbatch zeta-node.sbatch
# =============================================================================

#SBATCH --job-name=zeta-node
#SBATCH --partition=gpu
#SBATCH --gres=gpu:1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=24:00:00
#SBATCH --output=logs/zeta-%j.out
#SBATCH --error=logs/zeta-%j.err

set -e

# Load modules (adjust for your cluster)
# module load cuda/12.0
# module load gcc/11

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

# Calculate port based on job ID (avoid collisions)
NODE_PORT=${ZETA_PORT:-8080}
if [[ -n "$SLURM_JOB_ID" ]]; then
    NODE_PORT=$((8080 + (SLURM_JOB_ID % 1000)))
fi

# Get node hostname for registration
NODE_HOST=$(hostname -f)

echo "=============================================="
echo "  Z.E.T.A. Slurm Node Starting"
echo "=============================================="
echo "  Job ID:    ${SLURM_JOB_ID}"
echo "  Node:      ${NODE_HOST}"
echo "  Port:      ${NODE_PORT}"
echo "  GPU:       ${CUDA_VISIBLE_DEVICES:-auto}"
echo "  Storage:   ${ZETA_STORAGE:-./storage}"
echo "=============================================="

# Create logs directory
mkdir -p logs

# Register with coordinator if configured
if [[ -n "$ZETA_COORDINATOR" ]]; then
    echo "Registering with coordinator: $ZETA_COORDINATOR"
    curl -s -X POST "${ZETA_COORDINATOR}/register" \
        -H "Content-Type: application/json" \
        -d "{\"host\":\"${NODE_HOST}\",\"port\":${NODE_PORT},\"job_id\":\"${SLURM_JOB_ID}\"}" \
        || echo "Warning: Could not register with coordinator"
fi

# Build server path
ZETA_SERVER="${ZETA_BUILD_DIR:-build}/bin/zeta-server"
if [[ ! -f "$ZETA_SERVER" ]]; then
    ZETA_SERVER="./zeta-server"
fi

# Start server
exec "$ZETA_SERVER" \
    --host 0.0.0.0 \
    --port "$NODE_PORT" \
    -m "${MODEL_14B}" \
    --model-7b-coder "${MODEL_7B_CODER}" \
    --embed-model "${MODEL_EMBED}" \
    -ngl "${GPU_LAYERS:-999}" \
    --ctx-size "${CTX_14B:-4096}" \
    --zeta-storage "${ZETA_STORAGE:-./storage}"
