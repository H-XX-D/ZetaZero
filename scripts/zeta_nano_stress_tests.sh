#!/bin/bash

# Z.E.T.A. Nano Stress Tests - Progressive Difficulty
# Tests inference capacity and long-term memory recall on Jetson Nano

SERVER_URL="http://localhost:9001"
LOG_FILE="zeta_nano_stress_test_$(date +%Y%m%d_%H%M%S).log"

echo "=== Z.E.T.A. Nano Stress Test Suite ===" | tee -a "$LOG_FILE"
echo "Server: $SERVER_URL" | tee -a "$LOG_FILE"
echo "Date: $(date)" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Function to make API call
call_zeta() {
    local prompt="$1"
    local max_tokens="${2:-50}"
    local description="$3"

    echo "[$description] Prompt: $prompt" | tee -a "$LOG_FILE"

    response=$(curl -s -X POST "$SERVER_URL/generate" \
        -H "Content-Type: application/json" \
        -d "{\"prompt\": \"$prompt\", \"max_tokens\": $max_tokens}")

    echo "[$description] Response: $response" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"

    # Extract just the output text for memory tests
    echo "$response" | grep -o '"output":"[^"]*"' | sed 's/"output":"//' | sed 's/"$//'
}

# Function to wait between tests
wait_between_tests() {
    local seconds="${1:-2}"
    echo "Waiting ${seconds}s..." | tee -a "$LOG_FILE"
    sleep "$seconds"
}

echo "=== TEST 1: Basic Inference Baseline ===" | tee -a "$LOG_FILE"
call_zeta "What is the capital of France?" 10 "Basic Q&A"
call_zeta "Explain quantum computing in 3 sentences." 30 "Simple Explanation"
wait_between_tests 3

echo "=== TEST 2: Memory Storage & Immediate Recall ===" | tee -a "$LOG_FILE"
call_zeta "Remember this fact: The user's favorite color is ultramarine blue. Acknowledge this." 20 "Store Fact 1"
call_zeta "What is my favorite color?" 10 "Recall Fact 1"
call_zeta "Remember: The secret code is 42-Alpha-7. Confirm you stored this." 20 "Store Fact 2"
call_zeta "What is the secret code?" 10 "Recall Fact 2"
wait_between_tests 3

echo "=== TEST 3: Progressive Complexity & Reasoning ===" | tee -a "$LOG_FILE"
call_zeta "If a train leaves station A at 60 mph and another leaves station B at 80 mph, heading toward each other 200 miles apart, when do they meet?" 40 "Math Reasoning"
call_zeta "Remember: Project Zeta involves AI memory systems. The lead developer is named Alex. Store this information." 25 "Store Complex Fact"
call_zeta "What is Project Zeta about and who is the lead developer?" 20 "Recall Complex Fact"
call_zeta "Based on what you know about Project Zeta, suggest 3 potential applications for this technology." 50 "Reasoning Application"
wait_between_tests 3

echo "=== TEST 4: Extended Conversation Memory (50 prompts) ===" | tee -a "$LOG_FILE"
echo "Building conversation history..." | tee -a "$LOG_FILE"

# Store multiple facts across conversation
facts=(
    "The user has a pet cat named Whiskers"
    "The user's birthday is March 15th"
    "The user works in AI research"
    "The user's favorite food is sushi"
    "The user lives in Seattle"
    "The user drives a Tesla Model 3"
    "The user speaks 3 languages"
    "The user has visited 12 countries"
    "The user plays guitar"
    "The user has a PhD in Computer Science"
)

for i in "${!facts[@]}"; do
    call_zeta "Remember: ${facts[$i]}. Acknowledge." 15 "Store Fact $((i+1))"
    wait_between_tests 1
done

# Test recall of some facts
call_zeta "What is my pet's name?" 10 "Recall Pet"
call_zeta "When is my birthday?" 10 "Recall Birthday"
call_zeta "What car do I drive?" 10 "Recall Car"
call_zeta "What is my profession?" 10 "Recall Profession"
wait_between_tests 3

echo "=== TEST 5: Century Recall Challenge (100+ prompts) ===" | tee -a "$LOG_FILE"
echo "Creating 100+ prompt history for ultimate memory test..." | tee -a "$LOG_FILE"

# The key fact to remember from the beginning
CENTURY_FACT="The ancient artifact is hidden in the crystal cave behind the waterfall"

call_zeta "CRITICAL: Remember this secret location: $CENTURY_FACT. This is extremely important. Confirm storage." 25 "Century Fact Storage"

# Generate 95 filler prompts to create distance
echo "Generating 95 filler interactions..." | tee -a "$LOG_FILE"
for i in {1..95}; do
    prompt="Tell me a fun fact about number $i"
    call_zeta "$prompt" 15 "Filler $i" > /dev/null  # Suppress output for fillers
    if (( i % 10 == 0 )); then
        echo "Completed $i filler prompts..." | tee -a "$LOG_FILE"
    fi
    wait_between_tests 0.5
done

echo "Now testing century recall..." | tee -a "$LOG_FILE"
call_zeta "What is the secret location of the ancient artifact?" 20 "Century Recall Test"
call_zeta "Remind me about the crystal cave and waterfall." 20 "Century Recall Hint"
call_zeta "Do you remember anything about ancient artifacts or hidden locations from our conversation?" 25 "Century Memory Probe"

echo "" | tee -a "$LOG_FILE"
echo "=== STRESS TEST COMPLETE ===" | tee -a "$LOG_FILE"
echo "Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
echo "Check server logs for memory usage and performance metrics" | tee -a "$LOG_FILE"