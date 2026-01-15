#!/bin/bash
# Test script for Zeta running on Runpod

POD_URL="https://7zc4t8qzwz8y3k-8080.proxy.runpod.net"

echo "=== Zeta Runpod Test Suite ==="
echo "Pod URL: $POD_URL"
echo ""

# Test 1: Health check
echo "1. Health Check..."
health=$(curl -s --max-time 10 "$POD_URL/health")
if [ -n "$health" ]; then
    echo "   ✓ Server is UP"
    echo "   $health" | head -c 200
    echo ""
else
    echo "   ✗ Server not ready yet (still downloading models?)"
    echo "   Try again in a minute..."
    exit 1
fi

# Test 2: List models
echo ""
echo "2. Available Models..."
curl -s "$POD_URL/v1/models" | python3 -m json.tool 2>/dev/null || echo "   (no response)"

# Test 3: Simple chat completion
echo ""
echo "3. Chat Completion Test..."
response=$(curl -s --max-time 60 "$POD_URL/v1/chat/completions" \
    -H "Content-Type: application/json" \
    -d '{
        "model": "qwen2.5-14b",
        "messages": [{"role": "user", "content": "Hello! What is 2+2? Reply briefly."}],
        "max_tokens": 50,
        "temperature": 0.7
    }')

if [ -n "$response" ]; then
    echo "   Response:"
    echo "$response" | python3 -c "import sys,json; r=json.load(sys.stdin); print('   ' + r.get('choices',[{}])[0].get('message',{}).get('content','No content')[:200])" 2>/dev/null || echo "$response" | head -c 300
else
    echo "   ✗ No response from chat endpoint"
fi

# Test 4: View graph (memory)
echo ""
echo "4. Memory Graph Status..."
graph=$(curl -s --max-time 10 "$POD_URL/graph")
if [ -n "$graph" ]; then
    nodes=$(echo "$graph" | python3 -c "import sys,json; g=json.load(sys.stdin); print(len(g.get('nodes',[])))" 2>/dev/null || echo "?")
    echo "   Graph nodes: $nodes"
else
    echo "   (empty graph - no memories yet)"
fi

echo ""
echo "=== Tests Complete ==="
