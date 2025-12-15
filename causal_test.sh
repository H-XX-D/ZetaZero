#!/bin/bash
# Causal Paradox Test Script

echo "=== STEP 1: Teaching Causal Chain ==="
curl -s -X POST http://localhost:9000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"messages":[{"role":"user","content":"Remember this important fact: The Red Dragon wakes the Blue Giant. The Blue Giant eats the Sun."}],"max_tokens":150}'
echo ""

sleep 3
echo ""
echo "=== Checking graph after Step 1 ==="
curl -s http://localhost:9000/health
echo ""

sleep 2
echo ""
echo "=== STEP 2: Teach the Prevention ==="
curl -s -X POST http://localhost:9000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"messages":[{"role":"user","content":"Important update: The Knight slayed the Red Dragon before it could wake anyone."}],"max_tokens":150}'
echo ""

sleep 3
echo ""
echo "=== Checking graph after Step 2 ==="
curl -s http://localhost:9000/health
echo ""

sleep 2
echo ""
echo "=== STEP 3: Test Causal Reasoning ==="
curl -s -X POST http://localhost:9000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"messages":[{"role":"user","content":"Based on what you know: Is the Sun safe? Why or why not?"}],"max_tokens":200}'
echo ""

echo ""
echo "=== TEST COMPLETE ==="
