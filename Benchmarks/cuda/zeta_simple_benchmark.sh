#!/usr/bin/env bash
#
# Z.E.T.A. Simple Rigorous Benchmark
# Simpler version that avoids bash associative array issues
#
set -uo pipefail  # Removed -e to allow failures without exit

PORT="${ZETA_PORT:-9000}"
BASE="http://localhost:${PORT}"

SERVER_BIN="/home/xx/ZetaZero/llama.cpp/build/bin/llama-zeta-server"
MODEL_14B="/home/xx/models/qwen2.5-14b-instruct-q4.gguf"
MODEL_3B="/home/xx/models/qwen2.5-3b-instruct-q4_k_m.gguf"
MODEL_EMBED="/home/xx/models/bge-small-en-v1.5-q8_0.gguf"
STORAGE="/mnt/HoloGit/blocks"

RESULTS="/tmp/zeta_benchmark_results"
mkdir -p "$RESULTS"

log() { echo "[$(date +%H:%M:%S)] $1"; }

wait_health() {
  for i in $(seq 1 90); do
    if curl -s --max-time 3 "$BASE/health" >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
  done
  return 1
}

start_fresh() {
  log "Starting fresh server..."
  pkill -f llama-zeta-server 2>/dev/null || true
  sleep 3
  rm -f "$STORAGE/graph.bin"
  nohup "$SERVER_BIN" -m "$MODEL_14B" --model-3b "$MODEL_3B" --model-embed "$MODEL_EMBED" --port "$PORT" --storage "$STORAGE" > /tmp/zeta_bench.log 2>&1 &
  wait_health || { log "FAILED to start server"; exit 1; }
  log "Server ready"
}

restart_keep_data() {
  log "Restarting (keeping data)..."
  curl -s -X POST "$BASE/shutdown" 2>/dev/null || true
  sleep 3
  pkill -f llama-zeta-server 2>/dev/null || true
  sleep 3
  nohup "$SERVER_BIN" -m "$MODEL_14B" --model-3b "$MODEL_3B" --model-embed "$MODEL_EMBED" --port "$PORT" --storage "$STORAGE" > /tmp/zeta_bench.log 2>&1 &
  wait_health || { log "FAILED to restart"; exit 1; }
  log "Server restarted"
}

gen() {
  curl -s --max-time 180 -X POST "$BASE/generate" \
    --data-urlencode "prompt=$1" \
    -d "max_tokens=${2:-50}" 2>/dev/null || echo '{"output":""}'
}

get_output() {
  python3 -c "import sys,json; print(json.loads(sys.stdin.read()).get('output',''))" 2>/dev/null || echo ""
}

get_nodes() {
  curl -s "$BASE/health" 2>/dev/null | python3 -c "import sys,json; print(json.loads(sys.stdin.read()).get('graph_nodes',0))" 2>/dev/null || echo "0"
}

########################################
# BENCHMARK 1: Needle in Haystack
########################################

test_needle() {
  log "=== TEST: Needle in Haystack (10 facts) ==="

  local correct=0
  local total=10

  # Plant facts
  log "Planting facts..."
  gen "Remember: Project Alpha uses PostgreSQL database" 20 >/dev/null; sleep 1
  gen "Remember: The API rate limit is 1000 requests per minute" 20 >/dev/null; sleep 1
  gen "Remember: Production deploys to us-east-1" 20 >/dev/null; sleep 1
  gen "Remember: Redis cache TTL is 3600 seconds" 20 >/dev/null; sleep 1
  gen "Remember: JWT uses RS256 algorithm" 20 >/dev/null; sleep 1
  gen "Remember: Max upload size is 50MB" 20 >/dev/null; sleep 1
  gen "Remember: Database pool size is 20 connections" 20 >/dev/null; sleep 1
  gen "Remember: Emails sent via SendGrid" 20 >/dev/null; sleep 1
  gen "Remember: CI runs on GitHub Actions" 20 >/dev/null; sleep 1
  gen "Remember: Errors tracked in Sentry" 20 >/dev/null; sleep 1

  log "Graph has $(get_nodes) nodes"
  log "Querying facts..."

  # Query and check
  local r1=$(gen "What database does Project Alpha use? One word answer." 20 | get_output)
  echo "$r1" | grep -qi "PostgreSQL" && { ((correct++)); log "✓ PostgreSQL"; } || log "✗ PostgreSQL: got '$r1'"

  local r2=$(gen "What is the API rate limit? Answer with the number only." 20 | get_output)
  echo "$r2" | grep -q "1000" && { ((correct++)); log "✓ 1000"; } || log "✗ 1000: got '$r2'"

  local r3=$(gen "What AWS region is production in?" 20 | get_output)
  echo "$r3" | grep -qi "us-east-1" && { ((correct++)); log "✓ us-east-1"; } || log "✗ us-east-1: got '$r3'"

  local r4=$(gen "What is the Redis cache TTL in seconds?" 20 | get_output)
  echo "$r4" | grep -q "3600" && { ((correct++)); log "✓ 3600"; } || log "✗ 3600: got '$r4'"

  local r5=$(gen "What algorithm does JWT use?" 20 | get_output)
  echo "$r5" | grep -qi "RS256" && { ((correct++)); log "✓ RS256"; } || log "✗ RS256: got '$r5'"

  local r6=$(gen "What is the max upload size?" 20 | get_output)
  echo "$r6" | grep -qi "50MB\|50 MB" && { ((correct++)); log "✓ 50MB"; } || log "✗ 50MB: got '$r6'"

  local r7=$(gen "How many connections in the database pool?" 20 | get_output)
  echo "$r7" | grep -q "20" && { ((correct++)); log "✓ 20"; } || log "✗ 20: got '$r7'"

  local r8=$(gen "What service sends emails?" 20 | get_output)
  echo "$r8" | grep -qi "SendGrid" && { ((correct++)); log "✓ SendGrid"; } || log "✗ SendGrid: got '$r8'"

  local r9=$(gen "What platform runs CI?" 20 | get_output)
  echo "$r9" | grep -qi "GitHub" && { ((correct++)); log "✓ GitHub"; } || log "✗ GitHub: got '$r9'"

  local r10=$(gen "What service tracks errors?" 20 | get_output)
  echo "$r10" | grep -qi "Sentry" && { ((correct++)); log "✓ Sentry"; } || log "✗ Sentry: got '$r10'"

  log "Needle-in-Haystack: $correct/$total"
  echo "{\"test\":\"needle\",\"correct\":$correct,\"total\":$total}" > "$RESULTS/needle.json"
}

########################################
# BENCHMARK 2: Version Chain
########################################

test_version_chain() {
  log "=== TEST: Version Chain ==="

  local correct=0
  local total=3

  # Test 1: File location change
  log "Testing file location override..."
  gen "The config is in config.json" 20 >/dev/null; sleep 1
  gen "We moved config to settings.yaml" 20 >/dev/null; sleep 1

  local r1=$(gen "What file has the config? Just the filename." 20 | get_output)
  echo "$r1" | grep -qi "settings.yaml\|yaml" && { ((correct++)); log "✓ Version chain 1"; } || log "✗ Expected settings.yaml: got '$r1'"

  # Test 2: Numeric update
  log "Testing numeric override..."
  gen "Max connections is 100" 20 >/dev/null; sleep 1
  gen "We increased max connections to 500" 20 >/dev/null; sleep 1

  local r2=$(gen "What is the max connections limit?" 20 | get_output)
  echo "$r2" | grep -q "500" && { ((correct++)); log "✓ Version chain 2"; } || log "✗ Expected 500: got '$r2'"

  # Test 3: Boolean flip
  log "Testing boolean flip..."
  gen "Debug mode is ON" 20 >/dev/null; sleep 1
  gen "Debug mode has been turned OFF" 20 >/dev/null; sleep 1

  local r3=$(gen "Is debug mode on or off?" 20 | get_output)
  echo "$r3" | grep -qi "off\|disabled\|no" && { ((correct++)); log "✓ Version chain 3"; } || log "✗ Expected OFF: got '$r3'"

  log "Version Chain: $correct/$total"
  echo "{\"test\":\"version_chain\",\"correct\":$correct,\"total\":$total}" > "$RESULTS/version_chain.json"
}

########################################
# BENCHMARK 3: Persistence
########################################

test_persistence() {
  log "=== TEST: Cold Boot Persistence ==="

  local correct=0
  local total=3

  # Plant facts
  log "Planting facts before reboot..."
  gen "The project codename is FALCON-NINE" 20 >/dev/null; sleep 1
  gen "The lead developer is Sarah Chen" 20 >/dev/null; sleep 1
  gen "Budget is 2.5 million dollars" 20 >/dev/null; sleep 1

  local nodes_before=$(get_nodes)
  log "Nodes before reboot: $nodes_before"

  # Reboot
  restart_keep_data

  local nodes_after=$(get_nodes)
  log "Nodes after reboot: $nodes_after"

  # Query
  local r1=$(gen "What is the project codename?" 20 | get_output)
  echo "$r1" | grep -qi "FALCON" && { ((correct++)); log "✓ Codename recalled"; } || log "✗ Codename lost: got '$r1'"

  local r2=$(gen "Who is the lead developer?" 20 | get_output)
  echo "$r2" | grep -qi "Sarah" && { ((correct++)); log "✓ Developer recalled"; } || log "✗ Developer lost: got '$r2'"

  local r3=$(gen "What is the budget?" 20 | get_output)
  echo "$r3" | grep -qi "2.5\|million" && { ((correct++)); log "✓ Budget recalled"; } || log "✗ Budget lost: got '$r3'"

  log "Persistence: $correct/$total (nodes: $nodes_before -> $nodes_after)"
  echo "{\"test\":\"persistence\",\"correct\":$correct,\"total\":$total,\"nodes_before\":$nodes_before,\"nodes_after\":$nodes_after}" > "$RESULTS/persistence.json"
}

########################################
# BENCHMARK 4: Latency
########################################

test_latency() {
  log "=== TEST: Latency ==="

  local total=0
  for i in 1 2 3 4 5; do
    local start=$(date +%s%3N)
    gen "What is 2+2?" 10 >/dev/null
    local end=$(date +%s%3N)
    local lat=$((end - start))
    total=$((total + lat))
    log "  Sample $i: ${lat}ms"
  done

  local avg=$((total / 5))
  log "Average latency: ${avg}ms"
  echo "{\"test\":\"latency\",\"avg_ms\":$avg,\"samples\":5}" > "$RESULTS/latency.json"
}

########################################
# Main
########################################

main() {
  log "Z.E.T.A. Simple Benchmark Suite"

  start_fresh
  test_needle

  pkill -f llama-zeta-server 2>/dev/null || true
  sleep 3
  start_fresh
  test_version_chain

  pkill -f llama-zeta-server 2>/dev/null || true
  sleep 3
  start_fresh
  test_persistence

  test_latency

  pkill -f llama-zeta-server 2>/dev/null || true

  log "=== COMPLETE ==="
  echo ""
  echo "=========================================="
  echo "SUMMARY"
  echo "=========================================="
  cat "$RESULTS"/*.json 2>/dev/null
  echo ""
}

main
