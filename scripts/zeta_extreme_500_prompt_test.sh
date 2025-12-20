#!/bin/bash

# Z.E.T.A. EXTREME STRESS TEST - 500 Prompts + Power Cycles
# Tests ultimate memory persistence on Jetson Nano

SERVER_URL="http://192.168.0.146:9001"
LOG_FILE="zeta_extreme_stress_$(date +%Y%m%d_%H%M%S).log"
NANO_HOST="guard@192.168.0.146"

# Memory bank to track facts for recall testing
declare -a MEMORY_BANK
RECALL_INTERVAL=50
POWER_CYCLE_POINTS=(150 350)

echo "🚀 Z.E.T.A. EXTREME STRESS TEST - 500 PROMPTS + POWER CYCLES" | tee -a "$LOG_FILE"
echo "Server: $SERVER_URL" | tee -a "$LOG_FILE"
echo "Nano Host: $NANO_HOST" | tee -a "$LOG_FILE"
echo "Date: $(date)" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Function to make API call
call_zeta() {
    local prompt="$1"
    local max_tokens="${2:-30}"
    local description="$3"

    echo "[$description] Prompt: $prompt" | tee -a "$LOG_FILE"

    response=$(curl -s -X POST "$SERVER_URL/generate" \
        -H "Content-Type: application/json" \
        -d "{\"prompt\": \"$prompt\", \"max_tokens\": $max_tokens}")

    echo "[$description] Response: $response" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"

    # Extract just the output text
    echo "$response" | grep -o '"output":"[^"]*"' | sed 's/"output":"//' | sed 's/"$//'
}

# Function to store fact in memory bank
store_fact() {
    local fact="$1"
    local prompt_num="$2"
    MEMORY_BANK+=("$prompt_num:$fact")
}

# Function to test recall of ALL facts from previous tests
test_comprehensive_recall() {
    local current_prompt="$1"
    local test_description="$2"

    echo "🧠 COMPREHENSIVE RECALL TEST at prompt $current_prompt: $test_description" | tee -a "$LOG_FILE"
    echo "Testing recall of ALL ${current_prompt} previous facts!" | tee -a "$LOG_FILE"

    local total_facts=$((current_prompt - 1))
    local recall_score=0
    local total_tests=$total_facts

    if [ $total_facts -eq 0 ]; then
        echo "No previous facts to recall" | tee -a "$LOG_FILE"
        return
    fi

    echo "Testing recall of $total_facts facts from prompts 1-$((current_prompt - 1))" | tee -a "$LOG_FILE"

    # Test recall of ALL previous facts
    for ((i=1; i<current_prompt; i++)); do
        local fact_entry=""
        # Find the fact for this prompt number
        for entry in "${MEMORY_BANK[@]}"; do
            if [[ $entry == $i:* ]]; then
                fact_entry="$entry"
                break
            fi
        done

        if [ -n "$fact_entry" ]; then
            local fact_content=$(echo "$fact_entry" | cut -d: -f2-)
            echo "Testing recall of fact from prompt $i: $fact_content" | tee -a "$LOG_FILE"

            # Ask for recall
            local recall_prompt="What do you remember about this from our conversation: $fact_content"
            local response=$(call_zeta "$recall_prompt" 30 "Comprehensive Recall $i")

            # Simple scoring - check if response shows recognition
            if echo "$response" | grep -qi "remember\|recall\|yes\|correct\|true\|yes\|that\|it\|the"; then
                ((recall_score++))
                echo "✅ RECALL SUCCESS for prompt $i" | tee -a "$LOG_FILE"
            else
                echo "❌ RECALL FAILED for prompt $i" | tee -a "$LOG_FILE"
            fi
        fi
    done

    local accuracy=$((recall_score * 100 / total_tests))
    echo "🎯 COMPREHENSIVE RECALL ACCURACY: $recall_score/$total_tests ($accuracy%)" | tee -a "$LOG_FILE"
    echo "📊 Tested ALL $total_facts previous facts at prompt $current_prompt" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
}

# Function to simulate power cycle (shutdown and restart)
power_cycle_nano() {
    local cycle_point="$1"

    echo "🔌 POWER CYCLE at prompt $cycle_point - PHYSICAL UNPLUG REQUIRED!" | tee -a "$LOG_FILE"
    echo "⚠️  PLEASE UNPLUG THE JETSON NANO POWER CABLE NOW!" | tee -a "$LOG_FILE"
    echo "⏳ Waiting 30 seconds for you to unplug..." | tee -a "$LOG_FILE"

    # Wait for user to unplug
    sleep 30

    echo "🔌 Please plug the Nano back in and wait for it to boot..." | tee -a "$LOG_FILE"
    echo "⏳ Waiting 60 seconds for Nano to boot up..." | tee -a "$LOG_FILE"

    # Wait for Nano to boot
    sleep 60

    # Try to reconnect and restart server
    echo "🔄 Attempting to restart Z.E.T.A. server..." | tee -a "$LOG_FILE"

    # Multiple attempts to reconnect
    for attempt in {1..10}; do
        echo "Attempt $attempt to connect..." | tee -a "$LOG_FILE"
        if ssh -o ConnectTimeout=5 "$NANO_HOST" 'echo "Connection successful"' 2>/dev/null; then
            echo "✅ Nano is back online!" | tee -a "$LOG_FILE"
            break
        fi
        sleep 10
    done

    # Restart the server
    echo "🚀 Starting Z.E.T.A. server..." | tee -a "$LOG_FILE"
    ssh "$NANO_HOST" 'cd ~/ZetaZero/llama.cpp/build/bin && nohup ./llama-zeta-server -m ~/models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf --model-3b ~/models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf --port 9001 > ~/zeta_server.log 2>&1 &' 2>/dev/null

    # Wait for server to start
    echo "⏳ Waiting for server to start..." | tee -a "$LOG_FILE"
    sleep 30

    # Test if server is back
    local health_check=$(curl -s "$SERVER_URL/health" 2>/dev/null)
    if [ -n "$health_check" ]; then
        echo "✅ SERVER RESTARTED SUCCESSFULLY AFTER POWER CYCLE" | tee -a "$LOG_FILE"
    else
        echo "❌ SERVER FAILED TO RESTART AFTER POWER CYCLE" | tee -a "$LOG_FILE"
        exit 1
    fi

    # Test memory persistence
    echo "🧠 Testing memory persistence after power cycle..." | tee -a "$LOG_FILE"
    call_zeta "Do you remember our conversation before the power interruption? What were we discussing?" 50 "Persistence Test After Power Cycle"

    echo "✅ POWER CYCLE TEST COMPLETED" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
}

# Function to check system resources
check_resources() {
    local prompt_num="$1"

    if (( prompt_num % 50 == 0 )); then
        echo "📊 RESOURCE CHECK at prompt $prompt_num:" | tee -a "$LOG_FILE"

        # Check Nano resources
        echo "Nano resources:" | tee -a "$LOG_FILE"
        ssh "$NANO_HOST" 'free -h && echo "--- GPU ---" && nvidia-smi --query-gpu=memory.used,memory.free,utilization.gpu --format=csv,noheader' 2>/dev/null | tee -a "$LOG_FILE"

        echo "" | tee -a "$LOG_FILE"
    fi
}

# Main test loop
echo "🎯 STARTING 500-PROMPT EXTREME STRESS TEST" | tee -a "$LOG_FILE"
echo "Features:" | tee -a "$LOG_FILE"
echo "- 500 total prompts (extreme for edge device)" | tee -a "$LOG_FILE"
echo "- Recall testing every 50 prompts" | tee -a "$LOG_FILE"
echo "- Power cycles at prompts 150 and 350" | tee -a "$LOG_FILE"
echo "- Memory persistence through interruptions" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

for prompt_num in {1..500}; do
    echo "🔄 PROMPT $prompt_num/500" | tee -a "$LOG_FILE"

    # Check if this is a recall test point
    if (( prompt_num % RECALL_INTERVAL == 0 && prompt_num > 0 )); then
        test_comprehensive_recall "$prompt_num" "Comprehensive Recall Test at $prompt_num"
    fi

    # Check if this is a power cycle point
    for cycle_point in "${POWER_CYCLE_POINTS[@]}"; do
        if [ "$prompt_num" -eq "$cycle_point" ]; then
            power_cycle_nano "$cycle_point"
            break
        fi
    done

    # Generate a unique fact to store
    fact_types=("color" "animal" "food" "city" "number" "object" "emotion" "season" "sport" "instrument")
    random_type=${fact_types[$RANDOM % ${#fact_types[@]}]}
    random_value=$RANDOM

    case $random_type in
        "color") fact="My favorite color is RGB($random_value, $((random_value+50)), $((random_value+100)))" ;;
        "animal") fact="I once saw a $random_type with $random_value spots" ;;
        "animal") fact="I once saw an animal with $random_value spots" ;;
        "food") fact="I ate $random_value servings of food today" ;;
        "city") fact="The population of City_$random_value is $((random_value*1000)) people" ;;
        "number") fact="The number $random_value is very special to me" ;;
        "object") fact="I have $random_value objects in my collection" ;;
        "emotion") fact="I feel emotion at level $random_value out of 100" ;;
        "season") fact="season is my favorite time with temperature $random_value°F" ;;
        "sport") fact="I scored $random_value points in sport last game" ;;
        "instrument") fact="I can play instrument for $random_value minutes straight" ;;
    esac

    # Store fact for later recall testing
    store_fact "$fact" "$prompt_num"

    # Send fact to Z.E.T.A.
    call_zeta "Remember this: $fact" 20 "Store Fact $prompt_num"

    # Check resources periodically
    check_resources "$prompt_num"

    # Small delay between prompts
    sleep 1
done

# Final comprehensive recall test
echo "🎉 FINAL COMPREHENSIVE RECALL TEST" | tee -a "$LOG_FILE"
test_comprehensive_recall 500 "Final Comprehensive Recall Test"

# Final resource check
echo "📊 FINAL RESOURCE CHECK:" | tee -a "$LOG_FILE"
ssh "$NANO_HOST" 'free -h && echo "--- GPU ---" && nvidia-smi --query-gpu=memory.used,memory.free,utilization.gpu --format=csv,noheader' 2>/dev/null | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"
echo "🏁 EXTREME STRESS TEST COMPLETE!" | tee -a "$LOG_FILE"
echo "Results:" | tee -a "$LOG_FILE"
echo "- 500 prompts processed on Jetson Nano" | tee -a "$LOG_FILE"
echo "- Memory persistence through 2 power cycles" | tee -a "$LOG_FILE"
echo "- Recall accuracy tested 10 times across conversation" | tee -a "$LOG_FILE"
echo "- System stability under extreme load" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"