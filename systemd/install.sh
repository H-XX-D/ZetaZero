#!/bin/bash
# Z.E.T.A. System Daemon Installer
# Run as root: sudo ./install.sh

set -e

ZETA_USER="zeta"
ZETA_HOME="/opt/zeta"
ZETA_CONFIG="/etc/zeta"
ZETA_DATA="/var/lib/zeta"
ZETA_LOG="/var/log/zeta"

echo "=== Z.E.T.A. System Daemon Installer ==="
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run as root (sudo ./install.sh)"
    exit 1
fi

# Check for NVIDIA GPU
if ! command -v nvidia-smi &> /dev/null; then
    echo "Warning: nvidia-smi not found. GPU support may not work."
fi

# Create zeta user
echo "[1/7] Creating zeta user..."
if ! id "$ZETA_USER" &>/dev/null; then
    useradd -r -s /bin/false -d "$ZETA_HOME" -G video,render "$ZETA_USER"
    echo "  Created user: $ZETA_USER"
else
    echo "  User already exists: $ZETA_USER"
fi

# Create directories
echo "[2/7] Creating directories..."
mkdir -p "$ZETA_HOME"/{models,bin}
mkdir -p "$ZETA_CONFIG"
mkdir -p "$ZETA_DATA"/{graph_kv,dreams/pending,dreams/archive,storage,blocks}
mkdir -p "$ZETA_LOG"

# Set ownership
chown -R "$ZETA_USER:$ZETA_USER" "$ZETA_HOME" "$ZETA_DATA" "$ZETA_LOG"
chmod 750 "$ZETA_HOME" "$ZETA_DATA"

# Copy binary (if exists)
echo "[3/7] Installing binary..."
if [ -f "./zeta-server" ]; then
    cp ./zeta-server "$ZETA_HOME/zeta-server"
    chmod 755 "$ZETA_HOME/zeta-server"
    echo "  Installed: $ZETA_HOME/zeta-server"
elif [ -f "../llama.cpp/build/bin/zeta-server" ]; then
    cp ../llama.cpp/build/bin/zeta-server "$ZETA_HOME/zeta-server"
    chmod 755 "$ZETA_HOME/zeta-server"
    echo "  Installed: $ZETA_HOME/zeta-server"
else
    echo "  Warning: zeta-server binary not found. Build it first."
fi

# Install config files
echo "[4/7] Installing configuration..."
if [ ! -f "$ZETA_CONFIG/zeta.conf" ]; then
    if [ -f "../zeta.conf.example" ]; then
        cp ../zeta.conf.example "$ZETA_CONFIG/zeta.conf"
    else
        cat > "$ZETA_CONFIG/zeta.conf" << 'CONF'
# Z.E.T.A. Configuration
ZETA_MODEL_14B="/opt/zeta/models/qwen2.5-14b-instruct-q4_k_m.gguf"
ZETA_MODEL_7B="/opt/zeta/models/qwen2.5-coder-7b-instruct-q4_k_m.gguf"
ZETA_MODEL_EMBED="/opt/zeta/models/nomic-embed-text-v1.5.f16.gguf"
ZETA_GKV_DIR="/var/lib/zeta/graph_kv"
ZETA_DREAM_DIR="/var/lib/zeta/dreams"
ZETA_PORT=8080
CONF
    fi
    echo "  Created: $ZETA_CONFIG/zeta.conf"
else
    echo "  Config exists: $ZETA_CONFIG/zeta.conf"
fi

cp ./zeta.environment "$ZETA_CONFIG/environment"
chmod 640 "$ZETA_CONFIG"/*
chown root:$ZETA_USER "$ZETA_CONFIG"/*

# Install systemd service
echo "[5/7] Installing systemd service..."
cp ./zeta.service /etc/systemd/system/zeta.service
chmod 644 /etc/systemd/system/zeta.service
systemctl daemon-reload
echo "  Installed: /etc/systemd/system/zeta.service"

# Create symlink for CLI access
echo "[6/7] Creating CLI symlink..."
ln -sf "$ZETA_HOME/zeta-server" /usr/local/bin/zeta-server
echo "  Symlink: /usr/local/bin/zeta-server -> $ZETA_HOME/zeta-server"

echo "[7/7] Setup complete!"
echo ""
echo "=== Next Steps ==="
echo ""
echo "1. Copy your models to: $ZETA_HOME/models/"
echo "   - qwen2.5-14b-instruct-q4_k_m.gguf"
echo "   - qwen2.5-coder-7b-instruct-q4_k_m.gguf"
echo "   - nomic-embed-text-v1.5.f16.gguf"
echo ""
echo "2. Edit configuration: $ZETA_CONFIG/zeta.conf"
echo ""
echo "3. Enable and start the service:"
echo "   sudo systemctl enable zeta"
echo "   sudo systemctl start zeta"
echo ""
echo "4. Check status:"
echo "   sudo systemctl status zeta"
echo "   journalctl -u zeta -f"
echo ""
echo "=== Z.E.T.A. Ready for System-Level Operation ==="
