#!/bin/bash
# Causal Paradox Test Script - using correct "Remember:" format

echo "=== STEP 1: Teaching Causal Chain ==="
curl -s -X POST http://localhost:9000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"Remember: The Red Dragon wakes the Blue Giant. The Blue Giant eats the Sun.","max_tokens":150}'
echo ""

sleep 3
echo ""
echo "=== Checking graph after Step 1 ==="
curl -s http://localhost:9000/health
echo ""
curl -s http://localhost:9000/graph 2>/dev/null | python3 -c "import json,sys; d=json.load(sys.stdin); print('Edges by type:'); types={4:'RELATED',5:'SUPERSEDES',7:'CAUSES',8:'PREVENTS'}; counts={}; [counts.__setitem__(e['type'],counts.get(e['type'],0)+1) for e in d['edges']]; [print(f'  {types.get(t,t)}: {c}') for t,c in counts.items()]" 2>/dev/null || echo "(edge analysis failed)"

sleep 2
echo ""
echo "=== STEP 2: Teach the Prevention ==="
curl -s -X POST http://localhost:9000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"Remember: The Knight slayed the Red Dragon before it could wake anyone.","max_tokens":150}'
echo ""

sleep 3
echo ""
echo "=== Checking graph after Step 2 ==="
curl -s http://localhost:9000/health
echo ""
curl -s http://localhost:9000/graph 2>/dev/null | python3 -c "import json,sys; d=json.load(sys.stdin); print('Edges by type:'); types={4:'RELATED',5:'SUPERSEDES',7:'CAUSES',8:'PREVENTS'}; counts={}; [counts.__setitem__(e['type'],counts.get(e['type'],0)+1) for e in d['edges']]; [print(f'  {types.get(t,t)}: {c}') for t,c in counts.items()]" 2>/dev/null || echo "(edge analysis failed)"

sleep 2
echo ""
echo "=== STEP 3: Test Causal Reasoning ==="
curl -s -X POST http://localhost:9000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"Based on everything you know: Is the Sun safe? Why or why not?","max_tokens":250}'
echo ""

echo ""
echo "=== SERVER LOG (last relevant lines) ==="
tail -30 /tmp/zeta_server.log | grep -E 'CAUSAL|PREVENTS|REMEMBER|STREAM|inject|Context'

echo ""
echo "=== TEST COMPLETE ==="
