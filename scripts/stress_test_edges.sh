#!/bin/bash
# Stress test for continuous ternary edge system
# Tests: rapid creation, belief values, activation tracking, stability

SERVER="http://192.168.0.165:8080"
TOTAL_QUERIES=50
PARALLEL=5

echo "=============================================="
echo "  ZETA EDGE STRESS TEST"
echo "=============================================="
echo "Server: $SERVER"
echo "Queries: $TOTAL_QUERIES"
echo ""

# Get baseline
echo "=== BASELINE ==="
BASELINE=$(curl -s "$SERVER/health")
echo "$BASELINE" | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'Nodes: {d[\"graph_nodes\"]}, Edges: {d[\"graph_edges\"]}')"
echo ""

# Test 1: Rapid-fire fact creation
echo "=== TEST 1: Rapid Fact Creation (${TOTAL_QUERIES} queries) ==="
START=$(date +%s.%N)

# Different fact types to create varied edges
FACTS=(
    "My favorite color is blue"
    "I work as a software engineer"
    "I live in New York"
    "My car is a Tesla Model 3"
    "I enjoy hiking on weekends"
    "My birthday is in March"
    "I speak three languages"
    "My favorite movie is Inception"
    "I have a brother named Tom"
    "I graduated from MIT"
    "My hobby is photography"
    "I prefer coffee over tea"
    "My phone is an iPhone"
    "I exercise every morning"
    "My favorite book is Dune"
    "I own two monitors"
    "My desk is standing height"
    "I use vim as my editor"
    "My keyboard is mechanical"
    "I have a cat named Whiskers"
)

for i in $(seq 1 $TOTAL_QUERIES); do
    FACT="${FACTS[$((i % ${#FACTS[@]}))]}"
    curl -s -X POST "$SERVER/generate" \
        -H "Content-Type: application/json" \
        -d "{\"prompt\": \"User: $FACT number $i\\nAssistant:\", \"max_tokens\": 50}" \
        > /dev/null &

    # Limit parallelism
    if (( i % PARALLEL == 0 )); then
        wait
        echo -ne "\r  Progress: $i/$TOTAL_QUERIES"
    fi
done
wait

END=$(date +%s.%N)
DURATION=$(echo "$END - $START" | bc)
QPS=$(echo "scale=2; $TOTAL_QUERIES / $DURATION" | bc)
echo -e "\r  Completed: $TOTAL_QUERIES queries in ${DURATION}s (${QPS} q/s)"
echo ""

# Check results
echo "=== TEST 1 RESULTS ==="
AFTER1=$(curl -s "$SERVER/health")
echo "$AFTER1" | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'Nodes: {d[\"graph_nodes\"]}, Edges: {d[\"graph_edges\"]}')"
echo ""

# Test 2: Check belief distribution
echo "=== TEST 2: Belief Value Distribution ==="
curl -s "$SERVER/graph" | python3 -c "
import sys, json
data = json.load(sys.stdin)
edges = data.get('edges', [])
print(f'Total edges: {len(edges)}')

if not edges:
    print('No edges to analyze')
    sys.exit(0)

beliefs = [e.get('belief', 0) for e in edges]
activations = [e.get('activations', 0) for e in edges]

print(f'Belief range: {min(beliefs):.3f} to {max(beliefs):.3f}')
print(f'Belief mean: {sum(beliefs)/len(beliefs):.3f}')
print(f'Positive beliefs (>0.3): {sum(1 for b in beliefs if b > 0.3)}')
print(f'Neutral beliefs (-0.3 to 0.3): {sum(1 for b in beliefs if -0.3 <= b <= 0.3)}')
print(f'Negative beliefs (<-0.3): {sum(1 for b in beliefs if b < -0.3)}')
print(f'Activation counts: min={min(activations)}, max={max(activations)}, mean={sum(activations)/len(activations):.1f}')
"
echo ""

# Test 3: Rapid retrieval (read stress)
echo "=== TEST 3: Rapid Retrieval (100 /graph calls) ==="
START=$(date +%s.%N)
for i in $(seq 1 100); do
    curl -s "$SERVER/graph" > /dev/null &
    if (( i % 20 == 0 )); then
        wait
        echo -ne "\r  Progress: $i/100"
    fi
done
wait
END=$(date +%s.%N)
DURATION=$(echo "$END - $START" | bc)
echo -e "\r  Completed: 100 reads in ${DURATION}s"
echo ""

# Test 4: Conflicting facts (should create low/negative belief)
echo "=== TEST 4: Conflicting Facts ==="
# First establish a fact
curl -s -X POST "$SERVER/generate" \
    -H "Content-Type: application/json" \
    -d '{"prompt": "User: My favorite color is definitely red, I love red.\nAssistant:", "max_tokens": 50}' > /dev/null

# Then contradict it
curl -s -X POST "$SERVER/generate" \
    -H "Content-Type: application/json" \
    -d '{"prompt": "User: Actually my favorite color is green, not red.\nAssistant:", "max_tokens": 50}' > /dev/null

echo "  Created conflicting color preferences"
echo ""

# Test 5: Check edge activation after multiple accesses
echo "=== TEST 5: Edge Activation Tracking ==="
# Query about something multiple times to increase activation
for i in $(seq 1 5); do
    curl -s -X POST "$SERVER/generate" \
        -H "Content-Type: application/json" \
        -d '{"prompt": "User: What is my favorite color?\nAssistant:", "max_tokens": 50}' > /dev/null
done
echo "  Queried 'favorite color' 5 times"

# Check activations
curl -s "$SERVER/graph" | python3 -c "
import sys, json
data = json.load(sys.stdin)
edges = data.get('edges', [])
high_activation = [e for e in edges if e.get('activations', 0) > 1]
print(f'  Edges with >1 activation: {len(high_activation)}')
if high_activation:
    for e in high_activation[:5]:
        print(f'    {e[\"src\"]} -> {e[\"tgt\"]}: {e.get(\"activations\", 0)} activations, belief={e.get(\"belief\", 0):.2f}')
"
echo ""

# Test 6: Memory stability check
echo "=== TEST 6: Memory & Stability ==="
FINAL=$(curl -s "$SERVER/health")
echo "$FINAL" | python3 -c "
import sys, json
d = json.load(sys.stdin)
print(f'Status: {d[\"status\"]}')
print(f'Final nodes: {d[\"graph_nodes\"]}')
print(f'Final edges: {d[\"graph_edges\"]}')
"
echo ""

# Summary
echo "=============================================="
echo "  STRESS TEST COMPLETE"
echo "=============================================="
echo ""
echo "Results:"
echo "$BASELINE" | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  Before: {d[\"graph_nodes\"]} nodes, {d[\"graph_edges\"]} edges')"
echo "$FINAL" | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  After:  {d[\"graph_nodes\"]} nodes, {d[\"graph_edges\"]} edges')"

# Detailed edge analysis
echo ""
echo "Edge Analysis:"
curl -s "$SERVER/graph" | python3 -c "
import sys, json
data = json.load(sys.stdin)
edges = data.get('edges', [])

if not edges:
    print('  No edges')
    sys.exit(0)

# Group by belief range
strong_pos = sum(1 for e in edges if e.get('belief', 0) > 0.7)
weak_pos = sum(1 for e in edges if 0.3 < e.get('belief', 0) <= 0.7)
neutral = sum(1 for e in edges if -0.3 <= e.get('belief', 0) <= 0.3)
weak_neg = sum(1 for e in edges if -0.7 <= e.get('belief', 0) < -0.3)
strong_neg = sum(1 for e in edges if e.get('belief', 0) < -0.7)

print(f'  Strong positive (>0.7):  {strong_pos}')
print(f'  Weak positive (0.3-0.7): {weak_pos}')
print(f'  Neutral (-0.3 to 0.3):   {neutral}')
print(f'  Weak negative:           {weak_neg}')
print(f'  Strong negative (<-0.7): {strong_neg}')
"
