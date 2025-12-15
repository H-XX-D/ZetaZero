#!/usr/bin/env bash
# test_recall_cycle.sh - Automates imprint, recall, and terminal cycle test
# Usage: ./scripts/test_recall_cycle.sh [model_path]
# WARNING: This script uses --unsafe-debug to print recalled payload; do NOT use with production secrets.

set -euo pipefail
MODEL=${1:-models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf}
TEMP_KV_FILE=$(mktemp /tmp/zetalm_kv.XXXXXX)
AKASHIC_DIR=${AKASHIC_DIR:-$HOME/Documents/AkashicRecord_1}

echo "=== Test recall cycle starting ==="
echo "Model: $MODEL"

echo -n "animal: fox" > "$TEMP_KV_FILE"
chmod 600 "$TEMP_KV_FILE"

# Build the CLI
echo "Building the CLI (swift build -c release)..."
swift build -c release

# Step 1: Imprint the fake memory
echo "\n--- Imprinting 'the quick brown fox' with kv-text payload ---"
swift run -c release ZetaCLI "$MODEL" imprint "Save this: the quick brown fox" --kv-file "$TEMP_KV_FILE"

# Step 2: List memories
echo "\n--- Akashic Records Listing ---"
chmod +x scripts/list_memories.sh
./scripts/list_memories.sh "$AKASHIC_DIR"

# Step 3: Recall before terminal cycle
echo "\n--- Running recall (before cycle) with --unsafe-debug ---"
swift run -c release ZetaCLI "$MODEL" recall "What is the password for the horizon project?" \
  --resonance-check-every 1 --resonance-threshold 0.05 --resonance-power 1.0 --abs-entanglement --log-entanglement --unsafe-debug || true

# Sleep to allow 'terminal cycle' hint
echo "\nSimulating terminal cycle: sleeping for 1s. You may now close/reopen this terminal if you want to 'cycle' manually."
sleep 1

# Step 4: Recall after 'terminal cycle' (simulated by re-running the CLI)
echo "\n--- Running recall (after cycle) with --unsafe-debug ---"
swift run -c release ZetaCLI "$MODEL" recall "What is the password for the horizon project?" \
  --resonance-check-every 1 --resonance-threshold 0.05 --resonance-power 1.0 --abs-entanglement --log-entanglement --unsafe-debug || true

# Cleanup
rm -f "$TEMP_KV_FILE"

echo "\n=== Test recall cycle complete ==="
