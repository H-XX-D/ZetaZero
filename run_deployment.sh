#!/bin/bash
set -e

echo "=== Step 1: Deploying Code and Building on z6 ==="
chmod +x deploy_to_z6.sh
./deploy_to_z6.sh

echo "=== Step 2: Launching Zeta Server on z6 ==="
chmod +x launch_remote.sh
./launch_remote.sh
