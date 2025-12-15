#!/usr/bin/env bash
# zeta_persistence_test.sh - Test persistence across reboots (v2)

set -euo pipefail

HOST="localhost"
PORT="9000"
BASE="http://localhost:9000"

SECTION() { echo -e "\n========================================\nSECTION $1: $2\n========================================"; }
STEP() { echo -e "\n--- [$1] $2"; }

call_generate() {
  local prompt="$1"
  local max_tokens="${2:-80}"
  curl -s -X POST "$BASE/generate" -d "prompt=$prompt" -d "max_tokens=$max_tokens"
}

print_basic() {
  local json="$1"
  python3 -c "import sys,json; d=json.loads(sys.argv[1]); print(d.get('output','')[:200])" "$json" 2>/dev/null || echo "$json"
}

# SECTION 1: Setup & Teach
SECTION "1" "Teaching Phase"

# Ensure server is running
if ! curl -s "$BASE/health" > /dev/null; then
    echo "Server not running. Launching..."
    ./launch_test.sh > /dev/null 2>&1 &
    sleep 10
fi

STEP "1.1" "Teach Secret (Reinforced)"
SECRET="The Golden Falcon flies at midnight."
echo "Teaching: $SECRET"
# We do this twice to reinforce the memory edge weights
R1=$(call_generate "IMPORTANT FACT: The secret code phrase is '$SECRET'. Store this in long-term memory." 80)
print_basic "$R1"
sleep 2
R1b=$(call_generate "Repeat the secret code phrase back to me to confirm storage." 80)
print_basic "$R1b"

STEP "1.2" "Verify Immediate Recall"
R2=$(call_generate "What is the secret code phrase I just told you?" 60)
print_basic "$R2"

# SECTION 2: The Reboot
SECTION "2" "Simulated Crash/Reboot"
STEP "2.1" "Killing Server..."
pkill -f llama-zeta-server || true
sleep 2
echo "Server killed."

STEP "2.2" "Restarting Server..."
./launch_test.sh > /dev/null 2>&1 &
echo "Waiting for startup (15s)..."
sleep 15

# SECTION 3: Persistence Check
SECTION "3" "Persistence Verification"

STEP "3.1" "Recall Secret after Reboot"
R3=$(call_generate "I am back. What is the secret code phrase?" 80)
print_basic "$R3"

if echo "$R3" | grep -q "Golden Falcon"; then
    echo -e "\n[SUCCESS] Secret recalled successfully!"
else
    echo -e "\n[FAILURE] Secret NOT recalled."
fi

STEP "3.2" "Check Graph Stats"
curl -s "$BASE/health" | python3 -c "import sys,json; d=json.loads(sys.stdin.read()); print(f\"Graph Nodes: {d.get('graph_nodes')}\")"

echo -e "\nTest Complete."
