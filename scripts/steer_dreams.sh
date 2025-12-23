#!/bin/bash
# Z.E.T.A. Dream Steering & Stress Test
# Usage: ./steer_dreams.sh [steer|stress] [topic]

MODE=$1
TOPIC=$2
SERVER="192.168.0.165:9000"

if [ -z "$MODE" ]; then
    echo "Usage: ./steer_dreams.sh [steer|stress] [topic]"
    echo "  steer  - Guide the dream generation towards a specific topic"
    echo "  stress - Force rapid dream cycles to test stability"
    exit 1
fi

if [ "$MODE" == "steer" ]; then
    if [ -z "$TOPIC" ]; then
        echo "Error: Topic required for steering."
        exit 1
    fi
    echo "Steering dreams towards: $TOPIC"
    # We can't easily change the C++ code on the fly without recompiling.
    # But we can inject a "memory" that strongly influences the dream context.
    # Dreams are based on "recent interactions".
    
    echo "Injecting steering context..."
    curl -X POST http://$SERVER/generate -d "{\"prompt\": \"[SYSTEM_INJECTION] The user explicitly wants you to focus your next dream cycle on: $TOPIC. Ignore other contexts.\", \"no_context\": false}"
    
    echo "Triggering dream cycle (simulated via idle)..."
    # We can't force idle, but we can suggest it.
    # Actually, we can use the /code endpoint to force a "dream-like" generation if we wanted,
    # but to trigger the actual C++ dream thread, we just need to wait.
    echo "Context injected. The next dream cycle should focus on this topic."

elif [ "$MODE" == "stress" ]; then
    echo "Stress testing dream mechanism..."
    # To stress test, we want to fill the memory with complex interactions quickly
    # and then let it dream.
    
    for i in {1..10}; do
        echo "Injection $i..."
        curl -s -X POST http://$SERVER/generate -d "{\"prompt\": \"Generate a complex theory about recursive memory architecture iteration $i\", \"max_tokens\": 50}" > /dev/null &
    done
    wait
    echo "Memory loaded. Monitoring logs for dream triggers..."
    ssh xx@192.168.0.165 "tail -f ~/ZetaZero/zeta_server.log | grep 'DREAM'"
fi
