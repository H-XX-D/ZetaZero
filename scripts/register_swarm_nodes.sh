#!/bin/bash
# Register Orin Super Nano swarm nodes with Z.E.T.A.
# 7 Jetson Orin Super Nanos for distributed inference

SERVER_URL="${ZETA_URL:-http://192.168.0.165:8080}"

register_node() {
    local id=$1
    local host=$2
    local port=$3
    local role=$4
    
    echo "Registering $id ($host:$port)..."
    curl -s -X POST "$SERVER_URL/swarm/register" \
        -H "Content-Type: application/json" \
        -d "{\"id\": \"$id\", \"host\": \"$host\", \"port\": $port, \"role\": \"$role\"}"
    echo ""
}

echo "Registering Swarm Nodes..."

# GIN
register_node "GIN" "192.168.0.242" 11434 "researcher"

# TIF
register_node "TIF" "192.168.0.213" 11434 "researcher"

# ORIN
register_node "ORIN" "192.168.0.160" 11434 "researcher"

# ARCHI
register_node "ARCHI" "192.168.0.232" 11434 "researcher"

# TORI
register_node "TORI" "192.168.0.235" 11434 "researcher"

# NOVEL
register_node "NOVEL" "192.168.0.146" 11434 "researcher"

# THEO
register_node "THEO" "192.168.0.168" 11434 "researcher"

echo "Registration complete."
echo ""

# Store swarm topology in Z.E.T.A.'s memory graph
echo "=== Storing Swarm in Memory Graph ==="
curl -s -X POST "$SERVER_URL/generate" \
    -H "Content-Type: application/json" \
    -d '{
        "prompt": "Remember: My compute swarm has 7 NVIDIA Jetson Orin Super Nano nodes: GIN, TIF, ORIN, ARCHI, TORI, NOVEL, THEO. Each has 8GB unified memory, 1024 CUDA cores, 67 Tensor cores. They run Ollama on port 11434 for distributed inference, dreaming, and ternary consensus voting.",
        "max_tokens": 50
    }' > /dev/null && echo "Swarm topology stored."

# Store capabilities
curl -s -X POST "$SERVER_URL/generate" \
    -H "Content-Type: application/json" \
    -d '{
        "prompt": "Remember: The Orin swarm can: 1) Offload dream generation to idle nodes, 2) Perform distributed ternary voting (True/False/Uncertain) for consensus, 3) Run small models (1-3B params) in parallel. Total swarm: 56GB RAM, 7168 CUDA cores.",
        "max_tokens": 50
    }' > /dev/null && echo "Swarm capabilities stored."

echo ""
echo "=== Active Swarm Nodes ==="
curl -s "$SERVER_URL/swarm/nodes" | python3 -m json.tool 2>/dev/null || echo "No response"

echo ""
echo "=== Graph Query: swarm ==="
curl -s -X POST "$SERVER_URL/memory/query" \
    -H "Content-Type: application/json" \
    -d '{"query": "orin swarm nodes", "top_k": 3}' 2>/dev/null | python3 -m json.tool 2>/dev/null || echo "Query complete"
