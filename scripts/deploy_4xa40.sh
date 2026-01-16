#!/bin/bash
# Deploy 4x A40 pod to Runpod using REST API
# Uses existing network volume 6lcp45vi17

# Get API key from environment or prompt
RUNPOD_API_KEY="${RUNPOD_API_KEY:-}"
if [ -z "$RUNPOD_API_KEY" ]; then
    echo "Enter your Runpod API key:"
    read -s RUNPOD_API_KEY
fi

echo "Deploying 4x A40 pod with ZetaZero..."

curl -s -X POST https://rest.runpod.io/v1/pods \
  -H "Authorization: Bearer $RUNPOD_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "ZetaZero-4xA40-GKV",
    "imageName": "runpod/pytorch:2.1.0-py3.10-cuda11.8.0-devel-ubuntu22.04",
    "gpuTypeIds": ["NVIDIA A40"],
    "gpuCount": 4,
    "cloudType": "SECURE",
    "containerDiskInGb": 50,
    "volumeInGb": 0,
    "networkVolumeId": "6lcp45vi17",
    "volumeMountPath": "/runpod-volume",
    "ports": ["8080/http", "9000/http", "22/tcp"],
    "env": {
      "NVIDIA_VISIBLE_DEVICES": "all",
      "CUDA_VISIBLE_DEVICES": "0,1,2,3"
    },
    "dockerStartCmd": [
      "bash", "-c", 
      "cd /runpod-volume/ZetaZero && git pull origin master && cd llama.cpp && cmake -B build -DGGML_CUDA=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8 --target zeta-zero-server && echo READY"
    ]
  }' | tee /tmp/runpod_deploy_result.json

echo ""
echo "Deployment result saved to /tmp/runpod_deploy_result.json"

# Extract pod ID if successful
POD_ID=$(cat /tmp/runpod_deploy_result.json | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('id',''))" 2>/dev/null)
if [ -n "$POD_ID" ]; then
    echo ""
    echo "✓ Pod deployed: $POD_ID"
    echo "  SSH: ssh ${POD_ID}@ssh.runpod.io"
    echo "  Cost: ~\$1.60/hr (4x A40)"
    echo "  Network Volume: 6lcp45vi17 mounted at /runpod-volume"
fi
