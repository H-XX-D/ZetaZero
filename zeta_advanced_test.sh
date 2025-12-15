#!/usr/bin/env bash
# zeta_advanced_test.sh - Comprehensive test for Chat, Code Mode, and Tools

set -euo pipefail

HOST="localhost"
PORT="9000"
BASE="http://localhost:9000"

# Helper functions
SECTION() { echo -e "\n\033[1;34m========================================\nSECTION $1: $2\n========================================\033[0m"; }
STEP() { echo -e "\n\033[1;32m--- [$1] $2\033[0m"; }
WARN() { echo -e "\033[1;33m[WARN] $1\033[0m"; }
FAIL() { echo -e "\033[1;31m[FAIL] $1\033[0m"; exit 1; }
PASS() { echo -e "\033[1;32m[PASS] $1\033[0m"; }

# JSON parsing helper
get_json_val() {
    python3 -c "import sys, json; print(json.load(sys.stdin).get('$1', ''))" 2>/dev/null
}

get_nested_val() {
    python3 -c "import sys, json; d=json.load(sys.stdin); print(d$1)" 2>/dev/null
}

# Ensure server is running
SECTION "0" "Health Check"
if ! curl -s "$BASE/health" > /dev/null; then
    WARN "Server not running. Launching..."
    ./launch_test.sh > /dev/null 2>&1 &
    sleep 15
fi

HEALTH=$(curl -s "$BASE/health")
STATUS=$(echo "$HEALTH" | get_json_val "status")
NODES=$(echo "$HEALTH" | get_json_val "graph_nodes")
PARALLEL=$(echo "$HEALTH" | get_json_val "parallel_3b")

echo "Status: $STATUS"
echo "Nodes: $NODES"
echo "Parallel 3B: $PARALLEL"

if [ "$STATUS" != "ok" ]; then
    FAIL "Server health check failed"
fi
PASS "Server is healthy"

# SECTION 1: Chat Mode & Memory
SECTION "1" "Chat Mode & Memory Injection"

PROMPT="The project code name is 'Omega-Red'. Remember this."
STEP "1.1" "Injecting Memory: '$PROMPT'"

RESPONSE=$(curl -s -X POST "$BASE/generate" \
    -H "Content-Type: application/json" \
    -d "{\"prompt\": \"$PROMPT\", \"max_tokens\": 50, \"mode\": \"chat\"}")

OUTPUT=$(echo "$RESPONSE" | get_json_val "output")
NEW_NODES=$(echo "$RESPONSE" | get_json_val "graph_nodes")

echo "Output: ${OUTPUT:0:100}..."
echo "Graph Nodes: $NEW_NODES"

if [ "$NEW_NODES" -le "$NODES" ]; then
    WARN "Graph node count did not increase. Memory might not have been consolidated yet."
else
    PASS "Memory graph grew ($NODES -> $NEW_NODES)"
fi

# SECTION 2: Code Mode Workflow
SECTION "2" "Code Mode Workflow"

PROJECT_PATH="/tmp/zeta_test_project"
mkdir -p "$PROJECT_PATH"

STEP "2.1" "Opening Project: $PROJECT_PATH"
OPEN_RESP=$(curl -s -X POST "$BASE/project/open" \
    -d "path=$PROJECT_PATH&name=TestProject&description=A+test+project")

PROJ_STATUS=$(echo "$OPEN_RESP" | get_json_val "status")
PROJ_MODE=$(echo "$OPEN_RESP" | get_json_val "mode")

echo "Status: $PROJ_STATUS"
echo "Mode: $PROJ_MODE"

if [ "$PROJ_MODE" != "code" ]; then
    FAIL "Failed to switch to code mode"
fi
PASS "Switched to Code Mode"

STEP "2.2" "Extracting Code Entities"
CODE_TEXT="void calculate_entropy(float* data, int size) { float sum = 0; }"
EXTRACT_RESP=$(curl -s -X POST "$BASE/code/extract" \
    -d "text=$CODE_TEXT")

ENTITIES=$(echo "$EXTRACT_RESP" | get_json_val "entities_added")
echo "Entities Added: $ENTITIES"

STEP "2.3" "Checking Entity Existence"
CHECK_RESP=$(curl -s -X POST "$BASE/code/check" \
    -d "type=CODE_NODE_FUNCTION&name=calculate_entropy&file=main.cpp")

CAN_CREATE=$(echo "$CHECK_RESP" | get_json_val "can_create")
REASON=$(echo "$CHECK_RESP" | get_json_val "reason")

echo "Can Create: $CAN_CREATE"
echo "Reason: $REASON"

STEP "2.4" "Closing Project"
CLOSE_RESP=$(curl -s -X POST "$BASE/project/close")
CLOSE_MODE=$(echo "$CLOSE_RESP" | get_json_val "mode")

if [ "$CLOSE_MODE" != "chat" ]; then
    FAIL "Failed to switch back to chat mode"
fi
PASS "Switched back to Chat Mode"

# SECTION 3: Tool System
SECTION "3" "Tool System Discovery"

TOOLS_RESP=$(curl -s "$BASE/tools")
# Just print the raw JSON length or first few chars to verify
TOOL_LEN=${#TOOLS_RESP}

if [ "$TOOL_LEN" -lt 10 ]; then
    FAIL "Tool list seems empty or invalid"
fi
echo "Tool Schema Length: $TOOL_LEN bytes"
PASS "Tool system is active"

SECTION "4" "Summary"
echo "All tests completed."
