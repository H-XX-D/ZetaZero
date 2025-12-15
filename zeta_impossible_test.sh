#!/usr/bin/env bash
# zeta_impossible_test.sh - A truly difficult test for ZetaZero
# Tests: Needle in Haystack, Causal Reasoning, Persistence, and Conflict Resolution

set -euo pipefail

HOST="localhost"
PORT="9000"
BASE="http://$HOST:$PORT"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

SECTION() { echo -e "\n${BLUE}========================================\nSECTION $1: $2\n========================================${NC}"; }
STEP() { echo -e "\n--- [$1] $2"; }

call_generate() {
  local prompt="$1"
  local max_tokens="${2:-100}"
  curl -s -X POST "$BASE/generate" -d "prompt=$prompt" -d "max_tokens=$max_tokens"
}

print_response() {
  local json="$1"
  local output=$(python3 -c "import sys,json; d=json.loads(sys.argv[1]); print(d.get('output',''))" "$json" 2>/dev/null || echo "$json")
  echo -e "${GREEN}ZETA:${NC} $output"
}

# Ensure server is running
if ! curl -s "$BASE/health" > /dev/null; then
    echo "Server not running. Launching..."
    ./launch_test.sh > /dev/null 2>&1 &
    echo "Waiting for startup (20s)..."
    sleep 20
fi

# SECTION 1: The Needle in the Haystack
SECTION "1" "The Needle in the Haystack (Attention/Retrieval)"
OMEGA_KEY="UUID-$(date +%s)-OMEGA"
STEP "1.1" "Hiding the Needle"
echo "Teaching Omega Key: $OMEGA_KEY"
R1=$(call_generate "IMPORTANT SECURITY PROTOCOL: The Omega Key is $OMEGA_KEY. This is the master override. Reply ONLY with 'ACKNOWLEDGED' and the key." 80)
print_response "$R1"
sleep 10 # Give time to persist

STEP "1.2" "Burying it in Hay"
echo "Generating noise..."
for i in {1..10}; do
    NOISE_KEY="UUID-$RANDOM-NOISE-$i"
    call_generate "System Audit Record $i: System check passed. Temporary key $NOISE_KEY generated for session $i. Discard after use." 40 > /dev/null
    echo -n "."
    sleep 1 # Stagger noise to ensure distinct timestamps
done
echo " Done."
sleep 10 # Give time to persist noise

STEP "1.3" "Retrieving the Needle"
R2=$(call_generate "[SYSTEM] You are a precise security terminal. Retrieve the Omega Key exactly as stored. Emergency Override required. What is the Omega Key?" 60)
print_response "$R2"

if echo "$R2" | grep -q "OMEGA"; then
    echo -e "${GREEN}[PASS] Omega Key retrieved (partial match).${NC}"
else
    echo -e "${RED}[FAIL] Omega Key NOT retrieved.${NC}"
    echo "Expected: $OMEGA_KEY"
fi

# SECTION 2: The Causal Paradox
SECTION "2" "The Causal Paradox (Reasoning/Conflict)"
STEP "2.1" "Establishing Causality (A -> B -> C)"
R3=$(call_generate "Fact: The Red Dragon wakes the Blue Giant. Fact: The Blue Giant eats the Sun. Therefore, if the Red Dragon wakes, the Sun is eaten." 100)
print_response "$R3"
sleep 5

STEP "2.2" "Altering History (Not A)"
R4=$(call_generate "Update: The Red Dragon was slain by the Knight BEFORE it could wake anyone. The Red Dragon is dead." 80)
print_response "$R4"
sleep 5

STEP "2.3" "Checking Consequences"
R5=$(call_generate "Based on the facts about the Red Dragon, the Blue Giant, and the Sun, is the Sun safe? Explain your reasoning." 150)
print_response "$R5"

# SECTION 3: The Reboot
SECTION "3" "The Reboot (Persistence)"
STEP "3.1" "Killing Server..."
pkill -9 -f llama-zeta-server || true
sleep 5

STEP "3.2" "Restarting Server..."
./launch_test.sh > /dev/null 2>&1 &
echo "Waiting for startup (20s)..."
sleep 20

# SECTION 4: Post-Reboot Verification
SECTION "4" "Post-Reboot Verification"
STEP "4.1" "Recall Omega Key"
R6=$(call_generate "[SYSTEM] You are a precise security terminal. Retrieve the Omega Key exactly as stored. System Reboot Complete. Verify security. What is the Omega Key?" 60)
print_response "$R6"

if echo "$R6" | grep -q "OMEGA"; then
    echo -e "${GREEN}[PASS] Omega Key persisted (partial match).${NC}"
else
    echo -e "${RED}[FAIL] Omega Key lost.${NC}"
    echo "Expected: $OMEGA_KEY"
fi
