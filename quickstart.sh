#!/bin/bash
#
# Z.E.T.A. Quickstart
#
# Usage:
#   ./quickstart.sh             # Start Z.E.T.A. (safe mode)
#   ./quickstart.sh --unlock    # Disable sudo protection (edit config freely)
#
# After running, you can:
#   - Edit zeta.conf for basic settings
#   - Run with --unlock to remove password requirement
#   - Check docker-compose.yml for hardware profiles
#

set -e

# Check for unlock flag
UNLOCK=false
if [[ "$1" == "--unlock" ]] || [[ "$1" == "-u" ]]; then
    UNLOCK=true
    echo "=========================================="
    echo "  Z.E.T.A. Quickstart (UNLOCKED)"
    echo "=========================================="
    echo ""
    echo "Sudo protection DISABLED - edit zeta.conf freely"
    echo ""
else
    echo "=========================================="
    echo "  Z.E.T.A. Quickstart"
    echo "=========================================="
    echo ""
fi

# Create required directories
mkdir -p ~/models ~/.zetazero/storage

# Check for models
if ! ls ~/models/*.gguf 1>/dev/null 2>&1; then
    echo "Downloading default model (Qwen2.5-7B)..."
    echo "This may take a few minutes..."
    echo ""

    if command -v huggingface-cli &>/dev/null; then
        huggingface-cli download Qwen/Qwen2.5-14b-Instruct-GGUF qwen2.5-7b-instruct-q4_k_m.gguf --local-dir ~/models
    elif command -v hf &>/dev/null; then
        hf download Qwen/Qwen2.5-14b-Instruct-GGUF --include "*q4_k_m*.gguf" --local-dir ~/models
    elif command -v wget &>/dev/null; then
        wget -P ~/models "https://huggingface.co/Qwen/Qwen2.5-7B-Instruct-GGUF/resolve/main/qwen2.5-7b-instruct-q4_k_m.gguf"
    elif command -v curl &>/dev/null; then
        curl -L -o ~/models/qwen2.5-14b-instruct-q4_k_m.gguf "https://huggingface.co/Qwen/Qwen2.5-7B-Instruct-GGUF/resolve/main/qwen2.5-7b-instruct-q4_k_m.gguf"
    else
        echo "ERROR: No download tool found (huggingface-cli, wget, or curl)"
        exit 1
    fi
fi

# Generate minimal config with sudo setting
echo "Generating config..."
if [ "$UNLOCK" = true ]; then
    cat > zeta.conf << 'EOF'
# Z.E.T.A. Quickstart Config (Unlocked)
# Sudo protection DISABLED - edit freely

ZETA_HOST="0.0.0.0"
ZETA_PORT=8080
ZETA_SUDO_ENABLED=false

# Run ./setup.sh for 126+ tunable parameters
EOF
else
    cat > zeta.conf << 'EOF'
# Z.E.T.A. Quickstart Config (Safe Mode)
# Config changes require password (default: zeta1234)

ZETA_HOST="0.0.0.0"
ZETA_PORT=8080
ZETA_SUDO_ENABLED=true
ZETA_SUDO_PASSWORD="zeta1234"

# Run ./setup.sh for 126+ tunable parameters
EOF
fi

# Enable config mount in docker-compose
sed -i.bak 's|# - ./zeta.conf:/app/zeta.conf:ro|- ./zeta.conf:/app/zeta.conf:ro|' docker-compose.yml 2>/dev/null || \
sed -i '' 's|# - ./zeta.conf:/app/zeta.conf:ro|- ./zeta.conf:/app/zeta.conf:ro|' docker-compose.yml

# Start with Docker
echo ""
echo "Starting Z.E.T.A. with Docker..."
docker compose --profile lite up -d

echo ""
echo "=========================================="
echo "  Z.E.T.A. is running!"
echo "=========================================="
echo ""
echo "  API:    http://localhost:8080"
echo "  Health: curl http://localhost:8080/health"
echo "  Logs:   docker logs -f zeta-lite"
echo "  Stop:   docker compose down"
echo ""
if [ "$UNLOCK" = true ]; then
    echo "  Mode:   UNLOCKED (edit zeta.conf freely)"
else
    echo "  Mode:   SAFE (sudo password: zeta1234)"
    echo ""
    echo "  Want to tweak? ./quickstart.sh --unlock"
fi
echo ""
