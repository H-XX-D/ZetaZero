#!/bin/bash
#
# Z.E.T.A. Quickstart
#
# Usage:
#   ./quickstart.sh             # Start Z.E.T.A. (safe mode)
#   ./quickstart.sh --unlock    # Disable sudo protection (edit config freely)
#   ./quickstart.sh --lite      # Force lite profile (7B + 3B)
#   ./quickstart.sh --full      # Force full profile (14B + 7B)
#
# After running, you can:
#   - Edit zeta.conf for basic settings
#   - Run with --unlock to remove password requirement
#   - Check docker-compose.yml for hardware profiles
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

CONFIG_DIR=".zetazero"
CONFIG_FILE="$CONFIG_DIR/zeta.conf"

prompt_yes_no() {
    # prompt_yes_no <prompt> <default>
    # default: y|n
    local prompt="$1"
    local default="$2"
    local reply

    while true; do
        if [ "$default" = "y" ]; then
            read -r -p "$prompt [Y/n]: " reply
            reply=${reply:-Y}
        else
            read -r -p "$prompt [y/N]: " reply
            reply=${reply:-N}
        fi

        case "${reply}" in
            Y|y) return 0 ;;
            N|n) return 1 ;;
            *) echo "Please answer y or n." ;;
        esac
    done
}

prompt_path() {
    # prompt_path <prompt> <default>
    local prompt="$1"
    local default="$2"
    local reply
    read -r -p "$prompt [$default]: " reply
    echo "${reply:-$default}"
}

abspath() {
    # abspath <path>
    # macOS: prefer python3 if available
    local p="$1"
    if command -v python3 >/dev/null 2>&1; then
        python3 - <<PY
import os,sys
print(os.path.abspath(os.path.expanduser(sys.argv[1])))
PY
"$p"
    else
        # Fallback: naive (no ~ expansion)
        if [[ "$p" = /* ]]; then
            echo "$p"
        else
            echo "$PWD/$p"
        fi
    fi
}

usage() {
    cat << 'EOF'
Usage:
  ./quickstart.sh [--unlock|-u] [--lite|--full]

Modes:
  --lite  Starts the lite profile (zeta-lite)
  --full  Starts the full profile (zeta)
  (default is auto: full if models are present, otherwise lite)
EOF
}

download_file() {
    # download_file <hf_repo> <hf_filename> <output_path>
    local hf_repo="$1"
    local hf_filename="$2"
    local out_path="$3"

    if command -v huggingface-cli &>/dev/null; then
        huggingface-cli download "$hf_repo" "$hf_filename" --local-dir "$(dirname "$out_path")" --local-dir-use-symlinks False
        # huggingface-cli uses the original filename; move/symlink handled by caller
        return 0
    fi

    local url="https://huggingface.co/${hf_repo}/resolve/main/${hf_filename}"
    if command -v wget &>/dev/null; then
        wget -O "$out_path" "$url"
    elif command -v curl &>/dev/null; then
        curl -L -o "$out_path" "$url"
    else
        echo "ERROR: No download tool found (huggingface-cli, wget, or curl)"
        exit 1
    fi
}

ensure_model() {
    # ensure_model <expected_path> <hf_repo> <hf_filename> [alt_existing_path]
    local expected="$1"
    local hf_repo="$2"
    local hf_filename="$3"
    local alt_existing="${4:-}"

    if [ -f "$expected" ]; then
        return 0
    fi

    if [ -n "$alt_existing" ] && [ -f "$alt_existing" ]; then
        ln -sf "$alt_existing" "$expected"
        return 0
    fi

    echo "Downloading model: $(basename "$expected")"

    # If using huggingface-cli, it will land at ~/models/<hf_filename>
    local tmp_out="$expected"
    if command -v huggingface-cli &>/dev/null; then
        tmp_out="$(dirname "$expected")/$hf_filename"
    fi

    download_file "$hf_repo" "$hf_filename" "$tmp_out"

    if [ "$tmp_out" != "$expected" ]; then
        ln -sf "$tmp_out" "$expected"
    fi
}


UNLOCK=false
MODE="auto"

# ==============================================================================
# PREREQUISITE CHECKS
# ==============================================================================

check_docker() {
    echo "Checking Docker..."
    if ! command -v docker &> /dev/null; then
        echo "❌ Docker not found. Please install Docker first."
        echo "   https://docs.docker.com/get-docker/"
        exit 1
    fi
    if ! docker info &> /dev/null 2>&1; then
        echo "❌ Docker daemon not running. Please start Docker."
        exit 1
    fi
    echo "✓ Docker is ready"
}

# ==============================================================================
# HARDWARE AUTO-DETECTION
# ==============================================================================

detect_hardware() {
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                     🔍 Hardware Auto-Detection                               ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""

    OS_TYPE="$(uname -s)"
    echo "  OS: $OS_TYPE"

    if [ "$OS_TYPE" = "Darwin" ]; then
        TOTAL_RAM_GB=$(( $(sysctl -n hw.memsize) / 1024 / 1024 / 1024 ))
    else
        TOTAL_RAM_GB=$(( $(grep MemTotal /proc/meminfo | awk '{print $2}') / 1024 / 1024 ))
    fi
    echo "  RAM: ${TOTAL_RAM_GB}GB"

    GPU_VRAM_GB=0
    GPU_TYPE="CPU-only"

    if [ "$OS_TYPE" = "Darwin" ]; then
        if sysctl -n machdep.cpu.brand_string 2>/dev/null | grep -q "Apple"; then
            GPU_TYPE="Apple Silicon (Unified Memory)"
            GPU_VRAM_GB=$TOTAL_RAM_GB
        fi
    elif command -v nvidia-smi &>/dev/null; then
        GPU_VRAM_MB=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null | head -1)
        if [ -n "$GPU_VRAM_MB" ]; then
            GPU_VRAM_GB=$(( GPU_VRAM_MB / 1024 ))
            GPU_TYPE="NVIDIA ($(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1))"
        fi
    fi
    echo "  GPU: $GPU_TYPE"
    [ "$GPU_VRAM_GB" -gt 0 ] && echo "  VRAM: ${GPU_VRAM_GB}GB"

    AVAILABLE_MEM=$GPU_VRAM_GB
    [ "$AVAILABLE_MEM" -eq 0 ] && AVAILABLE_MEM=$TOTAL_RAM_GB

    echo ""
    if [ "$AVAILABLE_MEM" -ge 24 ]; then
        RECOMMENDED_PROFILE="full"
        echo "  ✓ Recommended: FULL profile (14B models) - You have plenty of memory"
    elif [ "$AVAILABLE_MEM" -ge 12 ]; then
        RECOMMENDED_PROFILE="full"
        echo "  ✓ Recommended: FULL profile (14B models) - Should fit comfortably"
    elif [ "$AVAILABLE_MEM" -ge 8 ]; then
        RECOMMENDED_PROFILE="lite"
        echo "  ✓ Recommended: LITE profile (7B models) - Best for your hardware"
    else
        RECOMMENDED_PROFILE="lite"
        echo "  ⚠ Warning: Low memory detected. LITE profile recommended, may be slow."
    fi
    echo ""

    # Set hardware variables
    HAS_GPU=false
    [ "$GPU_VRAM_GB" -gt 0 ] && HAS_GPU=true
    
    if [ "$HAS_GPU" = true ]; then
        GPU_LAYERS=99
        THREADS=$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) / 2 ))
    else
        GPU_LAYERS=0
        THREADS=$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) - 2 ))
    fi
    [ "$THREADS" -lt 2 ] && THREADS=2

    HW_DETECTED=true
}
# ==============================================================================
# MODEL FAMILY SELECTION
# ==============================================================================

# Model family definitions with HuggingFace repos
declare -A FAMILY_INFO
declare -A FAMILY_MAIN_14B
declare -A FAMILY_MAIN_7B
declare -A FAMILY_CODER_7B
declare -A FAMILY_CODER_3B

# QWEN Family (RECOMMENDED - best balance of quality and efficiency)
FAMILY_INFO[qwen]="Alibaba's Qwen2.5 series. Excellent reasoning, coding, and multilingual support."
FAMILY_MAIN_14B[qwen]="Qwen/Qwen2.5-14B-Instruct-GGUF|qwen2.5-14b-instruct-q4_k_m.gguf"
FAMILY_MAIN_7B[qwen]="Qwen/Qwen2.5-7B-Instruct-GGUF|qwen2.5-7b-instruct-q4_k_m.gguf"
FAMILY_CODER_7B[qwen]="Qwen/Qwen2.5-Coder-7B-Instruct-GGUF|qwen2.5-coder-7b-instruct-q4_k_m.gguf"
FAMILY_CODER_3B[qwen]="bartowski/Qwen2.5-Coder-3B-Instruct-GGUF|Qwen2.5-Coder-3B-Instruct-Q4_K_M.gguf"

# LLAMA Family (Meta's open models)
FAMILY_INFO[llama]="Meta's LLaMA 3.x series. Strong general performance, widely supported."
FAMILY_MAIN_14B[llama]="bartowski/Meta-Llama-3.1-8B-Instruct-GGUF|Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf"
FAMILY_MAIN_7B[llama]="bartowski/Meta-Llama-3.1-8B-Instruct-GGUF|Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf"
FAMILY_CODER_7B[llama]="bartowski/CodeLlama-7b-Instruct-hf-GGUF|CodeLlama-7b-Instruct-hf-Q4_K_M.gguf"
FAMILY_CODER_3B[llama]="bartowski/CodeLlama-7b-Instruct-hf-GGUF|CodeLlama-7b-Instruct-hf-Q4_K_M.gguf"

# MISTRAL Family (European efficiency)
FAMILY_INFO[mistral]="Mistral AI's models. Fast inference, good for constrained hardware."
FAMILY_MAIN_14B[mistral]="bartowski/Mistral-Nemo-Instruct-2407-GGUF|Mistral-Nemo-Instruct-2407-Q4_K_M.gguf"
FAMILY_MAIN_7B[mistral]="TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf"
FAMILY_CODER_7B[mistral]="TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf"
FAMILY_CODER_3B[mistral]="TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf"

# PHI Family (Microsoft's small but mighty)
FAMILY_INFO[phi]="Microsoft's Phi-3/4 series. Compact but capable, great for low memory."
FAMILY_MAIN_14B[phi]="bartowski/Phi-3-medium-128k-instruct-GGUF|Phi-3-medium-128k-instruct-Q4_K_M.gguf"
FAMILY_MAIN_7B[phi]="microsoft/Phi-3-mini-4k-instruct-gguf|Phi-3-mini-4k-instruct-q4.gguf"
FAMILY_CODER_7B[phi]="microsoft/Phi-3-mini-4k-instruct-gguf|Phi-3-mini-4k-instruct-q4.gguf"
FAMILY_CODER_3B[phi]="microsoft/Phi-3-mini-4k-instruct-gguf|Phi-3-mini-4k-instruct-q4.gguf"

# DEEPSEEK Family (Code-focused, R1 reasoning)
FAMILY_INFO[deepseek]="DeepSeek's models. Exceptional at code and technical reasoning."
FAMILY_MAIN_14B[deepseek]="bartowski/DeepSeek-R1-Distill-Qwen-14B-GGUF|DeepSeek-R1-Distill-Qwen-14B-Q4_K_M.gguf"
FAMILY_MAIN_7B[deepseek]="bartowski/DeepSeek-R1-Distill-Qwen-7B-GGUF|DeepSeek-R1-Distill-Qwen-7B-Q4_K_M.gguf"
FAMILY_CODER_7B[deepseek]="bartowski/deepseek-coder-6.7b-instruct-GGUF|deepseek-coder-6.7b-instruct-Q4_K_M.gguf"
FAMILY_CODER_3B[deepseek]="bartowski/deepseek-coder-1.3b-instruct-GGUF|deepseek-coder-1.3b-instruct-Q4_K_M.gguf"

# GEMMA Family (Google's open models)
FAMILY_INFO[gemma]="Google's Gemma 2 series. Strong reasoning, efficient architecture."
FAMILY_MAIN_14B[gemma]="bartowski/gemma-2-9b-it-GGUF|gemma-2-9b-it-Q4_K_M.gguf"
FAMILY_MAIN_7B[gemma]="bartowski/gemma-2-9b-it-GGUF|gemma-2-9b-it-Q4_K_M.gguf"
FAMILY_CODER_7B[gemma]="bartowski/codegemma-7b-it-GGUF|codegemma-7b-it-Q4_K_M.gguf"
FAMILY_CODER_3B[gemma]="bartowski/codegemma-2b-GGUF|codegemma-2b-Q4_K_M.gguf"

# YI Family (01.AI - Chinese/English bilingual)
FAMILY_INFO[yi]="01.AI's Yi series. Excellent bilingual (Chinese/English) performance."
FAMILY_MAIN_14B[yi]="bartowski/Yi-1.5-34B-Chat-GGUF|Yi-1.5-34B-Chat-Q4_K_M.gguf"
FAMILY_MAIN_7B[yi]="bartowski/Yi-1.5-9B-Chat-GGUF|Yi-1.5-9B-Chat-Q4_K_M.gguf"
FAMILY_CODER_7B[yi]="bartowski/Yi-Coder-9B-Chat-GGUF|Yi-Coder-9B-Chat-Q4_K_M.gguf"
FAMILY_CODER_3B[yi]="bartowski/Yi-Coder-1.5B-Chat-GGUF|Yi-Coder-1.5B-Chat-Q4_K_M.gguf"

# STARCODER Family (BigCode - pure code generation)
FAMILY_INFO[starcoder]="BigCode's StarCoder2. Pure code-focused, 600+ languages."
FAMILY_MAIN_14B[starcoder]="bartowski/starcoder2-15b-instruct-v0.1-GGUF|starcoder2-15b-instruct-v0.1-Q4_K_M.gguf"
FAMILY_MAIN_7B[starcoder]="bartowski/starcoder2-7b-GGUF|starcoder2-7b-Q4_K_M.gguf"
FAMILY_CODER_7B[starcoder]="bartowski/starcoder2-7b-GGUF|starcoder2-7b-Q4_K_M.gguf"
FAMILY_CODER_3B[starcoder]="bartowski/starcoder2-3b-GGUF|starcoder2-3b-Q4_K_M.gguf"

# INTERNLM Family (Shanghai AI Lab)
FAMILY_INFO[internlm]="InternLM2.5 series. Strong reasoning, long context, code-aware."
FAMILY_MAIN_14B[internlm]="bartowski/internlm2_5-20b-chat-GGUF|internlm2_5-20b-chat-Q4_K_M.gguf"
FAMILY_MAIN_7B[internlm]="bartowski/internlm2_5-7b-chat-GGUF|internlm2_5-7b-chat-Q4_K_M.gguf"
FAMILY_CODER_7B[internlm]="bartowski/internlm2_5-7b-chat-GGUF|internlm2_5-7b-chat-Q4_K_M.gguf"
FAMILY_CODER_3B[internlm]="bartowski/internlm2-chat-1_8b-GGUF|internlm2-chat-1_8b-Q4_K_M.gguf"

# CODESTRAL Family (Mistral's code specialist)
FAMILY_INFO[codestral]="Mistral's Codestral. Purpose-built for code, 80+ languages."
FAMILY_MAIN_14B[codestral]="bartowski/Codestral-22B-v0.1-GGUF|Codestral-22B-v0.1-Q4_K_M.gguf"
FAMILY_MAIN_7B[codestral]="bartowski/Codestral-22B-v0.1-GGUF|Codestral-22B-v0.1-Q4_K_M.gguf"
FAMILY_CODER_7B[codestral]="bartowski/Codestral-22B-v0.1-GGUF|Codestral-22B-v0.1-Q4_K_M.gguf"
FAMILY_CODER_3B[codestral]="bartowski/Codestral-22B-v0.1-GGUF|Codestral-22B-v0.1-Q4_K_M.gguf"

# COMMAND-R Family (Cohere - retrieval optimized)
FAMILY_INFO[command]="Cohere's Command-R. Optimized for RAG and retrieval tasks."
FAMILY_MAIN_14B[command]="bartowski/c4ai-command-r-v01-GGUF|c4ai-command-r-v01-Q4_K_M.gguf"
FAMILY_MAIN_7B[command]="bartowski/c4ai-command-r-v01-GGUF|c4ai-command-r-v01-Q4_K_M.gguf"
FAMILY_CODER_7B[command]="bartowski/c4ai-command-r-v01-GGUF|c4ai-command-r-v01-Q4_K_M.gguf"
FAMILY_CODER_3B[command]="bartowski/c4ai-command-r-v01-GGUF|c4ai-command-r-v01-Q4_K_M.gguf"

# WIZARDCODER Family (Fine-tuned for code)
FAMILY_INFO[wizard]="WizardCoder. Fine-tuned specifically for programming tasks."
FAMILY_MAIN_14B[wizard]="TheBloke/WizardCoder-15B-1.0-GGUF|wizardcoder-15b-1.0.Q4_K_M.gguf"
FAMILY_MAIN_7B[wizard]="TheBloke/WizardCoder-Python-7B-V1.0-GGUF|wizardcoder-python-7b-v1.0.Q4_K_M.gguf"
FAMILY_CODER_7B[wizard]="TheBloke/WizardCoder-Python-7B-V1.0-GGUF|wizardcoder-python-7b-v1.0.Q4_K_M.gguf"
FAMILY_CODER_3B[wizard]="TheBloke/WizardCoder-3B-V1.0-GGUF|wizardcoder-3b-v1.0.Q4_K_M.gguf"

# OPENCHAT Family (Community fine-tunes)
FAMILY_INFO[openchat]="OpenChat 3.5. Strong instruction-following, efficient."
FAMILY_MAIN_14B[openchat]="TheBloke/openchat-3.5-0106-GGUF|openchat-3.5-0106.Q4_K_M.gguf"
FAMILY_MAIN_7B[openchat]="TheBloke/openchat-3.5-0106-GGUF|openchat-3.5-0106.Q4_K_M.gguf"
FAMILY_CODER_7B[openchat]="TheBloke/openchat-3.5-0106-GGUF|openchat-3.5-0106.Q4_K_M.gguf"
FAMILY_CODER_3B[openchat]="TheBloke/openchat-3.5-0106-GGUF|openchat-3.5-0106.Q4_K_M.gguf"
# ==============================================================================
# EMBEDDING MODEL FAMILIES
# ==============================================================================

declare -A EMBED_INFO
declare -A EMBED_REPO
declare -A EMBED_FILE

# Nomic (default - works with all families)
EMBED_INFO[nomic]="Nomic AI's embedding model. Universal, works well with any LLM family."
EMBED_REPO[nomic]="nomic-ai/nomic-embed-text-v1.5-GGUF"
EMBED_FILE[nomic]="nomic-embed-text-v1.5.f16.gguf"

# BGE (good for multilingual)
EMBED_INFO[bge]="BAAI's BGE embeddings. Strong multilingual support, pairs well with Qwen."
EMBED_REPO[bge]="ChristianAzinn/bge-base-en-v1.5-gguf"
EMBED_FILE[bge]="bge-base-en-v1.5-f16.gguf"

# E5 (good general purpose)
EMBED_INFO[e5]="Microsoft's E5 embeddings. Excellent retrieval performance."
EMBED_REPO[e5]="ChristianAzinn/e5-base-v2-gguf"
EMBED_FILE[e5]="e5-base-v2-f16.gguf"

# Recommended pairings (family -> embed)
declare -A FAMILY_EMBED_RECOMMEND
FAMILY_EMBED_RECOMMEND[qwen]="bge"        # Both strong multilingual
FAMILY_EMBED_RECOMMEND[llama]="nomic"     # General purpose
FAMILY_EMBED_RECOMMEND[mistral]="e5"      # Both optimized for efficiency
FAMILY_EMBED_RECOMMEND[phi]="nomic"       # Universal fallback
FAMILY_EMBED_RECOMMEND[deepseek]="bge"    # Both strong at code/technical
FAMILY_EMBED_RECOMMEND[gemma]="e5"        # Google + Microsoft pair well
FAMILY_EMBED_RECOMMEND[yi]="bge"          # Both strong multilingual
FAMILY_EMBED_RECOMMEND[starcoder]="nomic" # Code + general retrieval
FAMILY_EMBED_RECOMMEND[internlm]="bge"    # Both Chinese lab origins
FAMILY_EMBED_RECOMMEND[codestral]="e5"    # Both efficiency-focused
FAMILY_EMBED_RECOMMEND[command]="nomic"   # Command-R + general embed
FAMILY_EMBED_RECOMMEND[wizard]="nomic"    # Code + general retrieval
FAMILY_EMBED_RECOMMEND[openchat]="nomic"  # General purpose

# ═══════════════════════════════════════════════════════════════════════════════
# EMBEDDING SELECTION
# ═══════════════════════════════════════════════════════════════════════════════

show_embed_menu() {
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                         EMBEDDING MODEL SELECTION                            ║"
    echo "╠══════════════════════════════════════════════════════════════════════════════╣"
    echo "║  Embeddings power semantic search and memory retrieval.                      ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    
    local recommended="${FAMILY_EMBED_RECOMMEND[$MODEL_FAMILY]:-nomic}"
    local idx=1
    
    for embed in "${EMBED_MODELS[@]}"; do
        local marker=""
        [ "$embed" = "$recommended" ] && marker=" (⭐ RECOMMENDED for ${MODEL_FAMILY^^})"
        echo "  [$idx] ${embed^^}${marker}"
        echo "      ${EMBED_INFO[$embed]}"
        echo ""
        ((idx++))
    done
}

select_embed_model() {
    show_embed_menu
    
    local recommended="${FAMILY_EMBED_RECOMMEND[$MODEL_FAMILY]:-nomic}"
    local default_idx=1
    for i in "${!EMBED_MODELS[@]}"; do
        [ "${EMBED_MODELS[$i]}" = "$recommended" ] && default_idx=$((i+1))
    done
    
    echo ""
    read -p "Select embedding [1-${#EMBED_MODELS[@]}] (default: $default_idx for ${recommended^^}): " embed_choice
    embed_choice=${embed_choice:-$default_idx}
    
    if [[ "$embed_choice" =~ ^[0-9]+$ ]] && [ "$embed_choice" -ge 1 ] && [ "$embed_choice" -le ${#EMBED_MODELS[@]} ]; then
        EMBED_MODEL="${EMBED_MODELS[$((embed_choice-1))]}"
    else
        echo "Invalid choice, using recommended: ${recommended^^}"
        EMBED_MODEL="$recommended"
    fi
    
    local repo_key="${EMBED_MODEL^^}_REPO"
    local file_key="${EMBED_MODEL^^}_FILE"
    EMBED_REPO="${!repo_key}"
    EMBED_FILE="${!file_key}"
    
    echo ""
    echo "✓ Selected embedding: ${EMBED_MODEL^^}"
    sleep 1
}

show_family_menu() {
    cat << 'FAMILYDOCS'

╔══════════════════════════════════════════════════════════════════════════════╗
║                        🤖 Model Family Selection                             ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  Z.E.T.A. supports 12 model families. Main + Coder stay in same family.     ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ═══════════════════ GENERAL PURPOSE (Recommended) ═══════════════════════  ║
║  [1] QWEN ⭐      Alibaba's Qwen2.5. Best all-around, multilingual           ║
║  [2] LLAMA        Meta's LLaMA 3.x. Strong general, huge community           ║
║  [3] MISTRAL      Mistral AI. Fast inference, memory efficient               ║
║  [4] PHI          Microsoft Phi-3/4. Compact but powerful                    ║
║  [5] GEMMA        Google Gemma 2. Strong reasoning, efficient                ║
║  [6] YI           01.AI Yi. Excellent Chinese/English bilingual              ║
║                                                                              ║
║  ═══════════════════ CODE SPECIALISTS ════════════════════════════════════  ║
║  [7] DEEPSEEK     DeepSeek R1. Chain-of-thought, technical reasoning         ║
║  [8] STARCODER    BigCode StarCoder2. Pure code, 600+ languages              ║
║  [9] CODESTRAL    Mistral Codestral. Purpose-built coder, 80+ langs          ║
║  [10] WIZARD      WizardCoder. Fine-tuned for programming                    ║
║                                                                              ║
║  ═══════════════════ SPECIALIZED ════════════════════════════════════════   ║
║  [11] INTERNLM    Shanghai AI InternLM2.5. Long context, code-aware          ║
║  [12] COMMAND     Cohere Command-R. Optimized for RAG/retrieval              ║
║  [13] OPENCHAT    OpenChat 3.5. Efficient instruction-following              ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝

FAMILYDOCS
}

select_model_family() {
    show_family_menu

    local choice
    read -r -p "Select model family [1-13] (default: 1 - Qwen): " choice
    choice=${choice:-1}

    case "$choice" in
        1)  MODEL_FAMILY="qwen" ;;
        2)  MODEL_FAMILY="llama" ;;
        3)  MODEL_FAMILY="mistral" ;;
        4)  MODEL_FAMILY="phi" ;;
        5)  MODEL_FAMILY="gemma" ;;
        6)  MODEL_FAMILY="yi" ;;
        7)  MODEL_FAMILY="deepseek" ;;
        8)  MODEL_FAMILY="starcoder" ;;
        9)  MODEL_FAMILY="codestral" ;;
        10) MODEL_FAMILY="wizard" ;;
        11) MODEL_FAMILY="internlm" ;;
        12) MODEL_FAMILY="command" ;;
        13) MODEL_FAMILY="openchat" ;;
        *)
            echo "Invalid choice, using Qwen (recommended)"
            MODEL_FAMILY="qwen"
            ;;
    esac

    echo ""
    echo "✓ Selected family: ${MODEL_FAMILY^^}"
    echo "  ${FAMILY_INFO[$MODEL_FAMILY]}"
    echo ""
}

# ═══════════════════════════════════════════════════════════════════════════════
# MODEL PATH RESOLUTION
# ═══════════════════════════════════════════════════════════════════════════════

select_model_size() {
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          MODEL SIZE SELECTION                                ║"
    echo "╠══════════════════════════════════════════════════════════════════════════════╣"
    echo "║  Both main and coder will use the same family: ${MODEL_FAMILY^^}                        "
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    
    if [ "$HAS_GPU" = true ]; then
        echo "  GPU detected! Larger models recommended."
        echo ""
        echo "  [1] FULL SIZE (14B main + 7B coder)"
        echo "      Best quality, requires ~24GB VRAM"
        echo ""
        echo "  [2] BALANCED (7B main + 7B coder)"
        echo "      Good quality, requires ~16GB VRAM"
        echo ""
        echo "  [3] COMPACT (7B main + 3B coder)"
        echo "      Fast inference, requires ~12GB VRAM"
        echo ""
        read -p "Select size [1-3] (default: 1): " size_choice
        size_choice=${size_choice:-1}
    else
        echo "  CPU-only detected. Smaller models recommended."
        echo ""
        echo "  [1] BALANCED (7B main + 3B coder)"
        echo "      Reasonable speed on CPU"
        echo ""
        echo "  [2] COMPACT (3B main + 3B coder)"
        echo "      Fastest on CPU"
        echo ""
        read -p "Select size [1-2] (default: 1): " size_choice
        size_choice=${size_choice:-1}
    fi
    
    case "$size_choice" in
        1)
            if [ "$HAS_GPU" = true ]; then
                MAIN_SIZE="14b"; CODER_SIZE="7b"
            else
                MAIN_SIZE="7b"; CODER_SIZE="3b"
            fi
            ;;
        2)
            if [ "$HAS_GPU" = true ]; then
                MAIN_SIZE="7b"; CODER_SIZE="7b"
            else
                MAIN_SIZE="3b"; CODER_SIZE="3b"
            fi
            ;;
        3)
            MAIN_SIZE="7b"; CODER_SIZE="3b"
            ;;
        *)
            MAIN_SIZE="7b"; CODER_SIZE="3b"
            ;;
    esac
    
    # Get model info from family arrays
    local main_info=$(get_model_info "$MODEL_FAMILY" "$MAIN_SIZE" "main")
    local coder_info=$(get_model_info "$MODEL_FAMILY" "$CODER_SIZE" "coder")
    
    MAIN_REPO="${main_info%%|*}"
    MAIN_FILE="${main_info##*|}"
    CODER_REPO="${coder_info%%|*}"
    CODER_FILE="${coder_info##*|}"
    
    echo ""
    echo "✓ Main:  $MAIN_FILE"
    echo "✓ Coder: $CODER_FILE"
    sleep 1
}

get_model_info() {
    local family="$1"
    local size="$2"
    local type="$3"
    
    local info=""
    
    case "${type}_${size}" in
        main_14b) info="${FAMILY_MAIN_14B[$family]}" ;;
        main_7b)  info="${FAMILY_MAIN_7B[$family]}" ;;
        main_3b)  info="${FAMILY_MAIN_3B[$family]:-${FAMILY_MAIN_7B[$family]}}" ;;
        coder_7b) info="${FAMILY_CODER_7B[$family]}" ;;
        coder_3b) info="${FAMILY_CODER_3B[$family]}" ;;
    esac
    
    echo "$info"
}

select_auto_install() {
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          INSTALLATION METHOD                                 ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  [1] AUTO-INSTALL (Recommended)"
    echo "      Downloads models automatically via huggingface-cli"
    echo ""
    echo "  [2] MANUAL"
    echo "      You provide paths to existing model files"
    echo ""
    
    read -p "Select method [1-2] (default: 1): " install_choice
    install_choice=${install_choice:-1}
    
    case "$install_choice" in
        1)
            AUTO_INSTALL=true
            echo ""
            echo "✓ Auto-install selected. Models will be downloaded."
            ;;
        2)
            AUTO_INSTALL=false
            echo ""
            echo "✓ Manual paths selected. You'll provide model file locations."
            ;;
        *)
            AUTO_INSTALL=true
            echo ""
            echo "Using auto-install (default)"
            ;;
    esac
}
generate_zeta_config() {
    local main_file="$MAIN_HF_FILE"
    local coder_file="$CODER_HF_FILE"
    local embed_file="$EMBED_HF_FILE"
    
    echo "Generating config for ${MODEL_FAMILY^^} family with ${EMBED_FAMILY^^} embeddings..."
    mkdir -p "$CONFIG_DIR"
    
    local main_ctx=4096
    local coder_ctx=4096
    
    # Context sizes per family
    case "$MODEL_FAMILY" in
        qwen)      main_ctx=32768; coder_ctx=32768 ;;
        llama)     main_ctx=8192;  coder_ctx=16384 ;;
        mistral)   main_ctx=8192;  coder_ctx=8192 ;;
        phi)       main_ctx=4096;  coder_ctx=4096 ;;
        deepseek)  main_ctx=16384; coder_ctx=16384 ;;
        gemma)     main_ctx=8192;  coder_ctx=8192 ;;
        yi)        main_ctx=16384; coder_ctx=16384 ;;
        starcoder) main_ctx=16384; coder_ctx=16384 ;;
        internlm)  main_ctx=32768; coder_ctx=32768 ;;
        codestral) main_ctx=32768; coder_ctx=32768 ;;
        command)   main_ctx=128000; coder_ctx=128000 ;;
        wizard)    main_ctx=8192;  coder_ctx=8192 ;;
        openchat)  main_ctx=8192;  coder_ctx=8192 ;;
        *)         main_ctx=4096;  coder_ctx=4096 ;;
    esac
    
    # Lite mode caps context
    [ "$MODE" = "lite" ] && main_ctx=$((main_ctx > 8192 ? 8192 : main_ctx)) && coder_ctx=$((coder_ctx > 8192 ? 8192 : coder_ctx))
    
    local sudo_block=""
    if [ "$UNLOCK" = true ]; then
        sudo_block="ZETA_SUDO_ENABLED=false"
    else
        sudo_block="ZETA_SUDO_ENABLED=true
ZETA_SUDO_PASSWORD=\"zeta1234\""
    fi
    
    cat > "$CONFIG_FILE" << CONFEOF
# Z.E.T.A. Configuration
# Zero Entropy Temporal Assimilation
# Generated: $(date)
# Model Family: ${MODEL_FAMILY^^}
# Embedding: ${EMBED_FAMILY^^}
# Profile: ${MODE^^}

# === Server Settings ===
ZETA_HOST="0.0.0.0"
ZETA_PORT=8080
${sudo_block}

# === Model Paths (container) ===
MODEL_MAIN="/models/main.gguf"
MODEL_CODER="/models/sub.gguf"
MODEL_EMBED="/models/embed.gguf"

# === Model Family: ${MODEL_FAMILY^^} ===
# Main:  ${main_file}
# Coder: ${coder_file}
MODEL_FAMILY="${MODEL_FAMILY}"
MODEL_MAIN_CTX=${main_ctx}
MODEL_CODER_CTX=${coder_ctx}

# === Embedding: ${EMBED_FAMILY^^} ===
# File: ${embed_file}
EMBED_FAMILY="${EMBED_FAMILY}"
EMBED_DIM=768

# === Storage Paths ===
ZETA_STORAGE="/storage"
ZETA_GKV_DIR="/storage/graph_kv"
ZETA_DREAM_DIR="/storage/dreams"

# === Host Paths (reference) ===
# MODEL_MAIN_HOST="${MODEL_MAIN_HOST}"
# MODEL_CODER_HOST="${MODEL_SUB_HOST}"
# MODEL_EMBED_HOST="${MODEL_EMBED_HOST}"
# STORAGE_HOST="${STORAGE_HOST}"
CONFEOF
    
    echo "✓ Config written to $CONFIG_FILE"
}

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN EXECUTION
# ═══════════════════════════════════════════════════════════════════════════════

download_models() {
    local storage_path="$1"
    local models_dir="$storage_path/models"
    
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          DOWNLOADING MODELS                                  ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    
    if ! command -v huggingface-cli &> /dev/null; then
        echo "Installing huggingface-cli..."
        pip install -q huggingface_hub
    fi
    
    echo "Downloading main model: $MAIN_FILE"
    huggingface-cli download "$MAIN_REPO" "$MAIN_FILE" --local-dir "$models_dir" --local-dir-use-symlinks False
    
    echo ""
    echo "Downloading coder model: $CODER_FILE"
    huggingface-cli download "$CODER_REPO" "$CODER_FILE" --local-dir "$models_dir" --local-dir-use-symlinks False
    
    echo ""
    echo "Downloading embedding model: $EMBED_FILE"
    huggingface-cli download "$EMBED_REPO" "$EMBED_FILE" --local-dir "$models_dir" --local-dir-use-symlinks False
    
    echo ""
    echo "✓ All models downloaded!"
    sleep 2
}

main() {
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                                                                              ║"
    echo "║   ███████╗███████╗████████╗ █████╗     ███████╗███████╗██████╗  ██████╗      ║"
    echo "║   ╚══███╔╝██╔════╝╚══██╔══╝██╔══██╗    ╚══███╔╝██╔════╝██╔══██╗██╔═══██╗     ║"
    echo "║     ███╔╝ █████╗     ██║   ███████║      ███╔╝ █████╗  ██████╔╝██║   ██║     ║"
    echo "║    ███╔╝  ██╔══╝     ██║   ██╔══██║     ███╔╝  ██╔══╝  ██╔══██╗██║   ██║     ║"
    echo "║   ███████╗███████╗   ██║   ██║  ██║    ███████╗███████╗██║  ██║╚██████╔╝     ║"
    echo "║   ╚══════╝╚══════╝   ╚═╝   ╚═╝  ╚═╝    ╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝      ║"
    echo "║                                                                              ║"
    echo "║                 Zero Entropy Temporal Assimilation                           ║"
    echo "║                      First-Time Setup Wizard                                 ║"
    echo "║                                                                              ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    
    check_docker
    detect_hardware
    
    echo ""
    echo "Press ENTER to begin setup..."
    read -r
    
    # Family selection (includes size selection)
    select_model_family
    
    # Model size selection (sets MAIN_FILE, CODER_FILE)
    select_model_size
    
    # Embedding selection
    select_embed_model
    
    # Storage path
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          STORAGE CONFIGURATION                               ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Models and data will be stored in this directory."
    echo "  Recommended: SSD with at least 50GB free space."
    echo ""
    
    local default_storage="$HOME/.zetazero"
    read -p "Storage path [$default_storage]: " STORAGE_PATH
    STORAGE_PATH="${STORAGE_PATH:-$default_storage}"
    STORAGE_PATH="${STORAGE_PATH/#\~/$HOME}"
    
    mkdir -p "$STORAGE_PATH/models" "$STORAGE_PATH/graph" "$STORAGE_PATH/logs"
    
    echo ""
    echo "✓ Storage: $STORAGE_PATH"
    sleep 1
    
    # Auto-install choice
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          MODEL INSTALLATION                                  ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Would you like to download the selected models automatically?"
    echo ""
    echo "  Models:"
    echo "    • Main:   $MAIN_FILE"
    echo "    • Coder:  $CODER_FILE"
    echo "    • Embed:  $EMBED_FILE"
    echo ""
    echo "  Total size: ~20-40GB depending on selection"
    echo ""
    
    read -p "Auto-download models? [Y/n]: " auto_install
    auto_install="${auto_install:-Y}"
    
    if [[ "${auto_install^^}" == "Y" ]]; then
        AUTO_INSTALL=true
    else
        AUTO_INSTALL=false
    fi
    
    # Generate config
    generate_zeta_config "$STORAGE_PATH"
    
    # Download if requested
    if [ "$AUTO_INSTALL" = true ]; then
        download_models "$STORAGE_PATH"
    fi
    
    # Final summary
    clear
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          SETUP COMPLETE! 🎉                                  ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Configuration: $STORAGE_PATH/zeta.conf"
    echo ""
    echo "  Selected Models (${MODEL_FAMILY^^} Family):"
    echo "    • Main:      $MAIN_FILE"
    echo "    • Coder:     $CODER_FILE"
    echo "    • Embedding: ${EMBED_MODEL^^}"
    echo ""
    echo "  Hardware:"
    echo "    • GPU Layers: $GPU_LAYERS"
    echo "    • Threads:    $THREADS"
    echo ""
    
    if [ "$AUTO_INSTALL" = true ]; then
        echo "  To start ZetaZero:"
        echo "    docker compose up -d"
    else
        echo "  Next steps:"
        echo "    1. Download models to $STORAGE_PATH/models/"
        echo "    2. Run: docker compose up -d"
    fi
    echo ""
}

main "$@"
