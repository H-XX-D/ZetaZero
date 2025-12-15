#!/usr/bin/env bash
#
# Z.E.T.A. Hippocampus Deployment
# Installs the embedding model integration on Z6
#
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ZETA_DIR="${ZETA_DIR:-$HOME/ZetaZero/llama.cpp}"
MODELS_DIR="${MODELS_DIR:-$HOME/models}"
TOOLS_DIR="$ZETA_DIR/tools/zeta-demo"

# Embedding model
EMBED_URL="https://huggingface.co/CompendiumLabs/bge-small-en-v1.5-gguf/resolve/main/bge-small-en-v1.5-q8_0.gguf"
EMBED_FILE="bge-small-en-v1.5-q8_0.gguf"

log() { echo "[$(date +%H:%M:%S)] $1"; }

log "=========================================="
log "Z.E.T.A. Hippocampus Deployment"
log "=========================================="

# 1. Download embedding model
log ""
log "[1/5] Checking embedding model..."
if [ -f "$MODELS_DIR/$EMBED_FILE" ]; then
    log "  Already exists: $MODELS_DIR/$EMBED_FILE ($(ls -lh "$MODELS_DIR/$EMBED_FILE" | awk '{print $5}'))"
else
    log "  Downloading $EMBED_FILE..."
    mkdir -p "$MODELS_DIR"
    curl -L -o "$MODELS_DIR/$EMBED_FILE" "$EMBED_URL"
    log "  Downloaded: $(ls -lh "$MODELS_DIR/$EMBED_FILE" | awk '{print $5}')"
fi

# 2. Copy integration header
log ""
log "[2/5] Installing zeta-embed-integration.h..."
cp -v "$SCRIPT_DIR/zeta-embed-integration.h" "$TOOLS_DIR/"

# 3. Backup originals
log ""
log "[3/5] Backing up original files..."
[ -f "$TOOLS_DIR/zeta-server.cpp.orig" ] || cp "$TOOLS_DIR/zeta-server.cpp" "$TOOLS_DIR/zeta-server.cpp.orig"
[ -f "$TOOLS_DIR/zeta-dual-process.h.orig" ] || cp "$TOOLS_DIR/zeta-dual-process.h" "$TOOLS_DIR/zeta-dual-process.h.orig"
log "  Backups saved as .orig files"

# 4. Patch zeta-server.cpp
log ""
log "[4/5] Patching zeta-server.cpp..."

# Check if already patched
if grep -q "zeta-embed-integration.h" "$TOOLS_DIR/zeta-server.cpp"; then
    log "  Already patched!"
else
    # Create patched version
    cat > /tmp/embed_patch.py << 'PYPATCH'
import sys

content = open(sys.argv[1]).read()

# 1. Add include after zeta-dual-process.h
if '#include "zeta-embed-integration.h"' not in content:
    content = content.replace(
        '#include "zeta-dual-process.h"',
        '#include "zeta-dual-process.h"\n#include "zeta-embed-integration.h"'
    )

# 2. Add global variable for embed model path
if 'g_embed_model_path' not in content:
    content = content.replace(
        'static std::string g_storage_dir',
        'static std::string g_embed_model_path;\nstatic std::string g_storage_dir'
    )

# 3. Add argument parsing (find the arg parsing section)
if '--model-embed' not in content:
    # Find where --storage is parsed and add after
    storage_parse = '} else if (strcmp(argv[i], "--storage") == 0 && i + 1 < argc) {\n            g_storage_dir = argv[++i];'
    embed_parse = storage_parse + '\n        } else if (strcmp(argv[i], "--model-embed") == 0 && i + 1 < argc) {\n            g_embed_model_path = argv[++i];'
    content = content.replace(storage_parse, embed_parse)

# 4. Add embedding model initialization (after zeta_dual_init)
init_marker = 'fprintf(stderr, "Z.E.T.A. Dual-Process system initialized\\n");'
if init_marker in content and 'zeta_embed_init' not in content:
    embed_init = '''fprintf(stderr, "Z.E.T.A. Dual-Process system initialized\\n");

    // Initialize embedding model (The Hippocampus)
    if (!g_embed_model_path.empty()) {
        if (zeta_embed_init(g_embed_model_path.c_str())) {
            fprintf(stderr, "Hippocampus loaded: %d-dim semantic embeddings\\n", zeta_embed_dim());
        } else {
            fprintf(stderr, "WARNING: Embedding model failed, using hash fallback\\n");
        }
    } else {
        fprintf(stderr, "NOTE: No --model-embed specified, using hash embeddings\\n");
    }'''
    content = content.replace(init_marker, embed_init)

# 5. Add cleanup (find signal handler)
if 'zeta_embed_free()' not in content:
    cleanup_marker = 'zeta_dual_free(g_dual);'
    if cleanup_marker in content:
        content = content.replace(cleanup_marker, 'zeta_embed_free();\n    ' + cleanup_marker)

print(content)
PYPATCH

    python3 /tmp/embed_patch.py "$TOOLS_DIR/zeta-server.cpp" > /tmp/zeta-server-patched.cpp
    cp /tmp/zeta-server-patched.cpp "$TOOLS_DIR/zeta-server.cpp"
    log "  Patched successfully"
fi

# 5. Patch zeta-dual-process.h - update zeta_3b_embed to use embedding model
log ""
log "[5/5] Patching zeta-dual-process.h..."

if grep -q "zeta_embed_ready" "$TOOLS_DIR/zeta-dual-process.h"; then
    log "  Already patched!"
else
    cat > /tmp/embed_patch2.py << 'PYPATCH2'
import sys
import re

content = open(sys.argv[1]).read()

# Find and replace zeta_3b_embed function
old_func = r'static void zeta_3b_embed\(zeta_dual_ctx_t\* ctx, const char\* text, float\* embedding, int embed_dim\) \{[^}]+// TODO: Run 3B inference[^}]+\}'

new_func = '''static void zeta_3b_embed(zeta_dual_ctx_t* ctx, const char* text, float* embedding, int embed_dim) {
    (void)ctx;

    // Use embedding model if available (The Hippocampus)
    if (zeta_embed_ready()) {
        int result = zeta_embed_text(text, embedding, embed_dim);
        if (result > 0) {
            // Pad with zeros if needed
            if (result < embed_dim) {
                memset(embedding + result, 0, (embed_dim - result) * sizeof(float));
            }
            return;
        }
        fprintf(stderr, "[EMBED] Warning: Failed, falling back to hash\\n");
    }

    // Fallback: Hash-based embedding (NOT semantic)
    size_t len = strlen(text);
    memset(embedding, 0, embed_dim * sizeof(float));
    for (size_t i = 0; i < len; i++) {
        uint32_t h = (uint32_t)text[i] * 2654435761u;
        embedding[h % embed_dim] += 1.0f;
    }

    // Normalize
    float norm = 0.0f;
    for (int i = 0; i < embed_dim; i++) {
        norm += embedding[i] * embedding[i];
    }
    norm = sqrtf(norm);
    if (norm > 1e-8f) {
        for (int i = 0; i < embed_dim; i++) {
            embedding[i] /= norm;
        }
    }
}'''

# Try to replace - if regex doesn't match, do simpler replacement
if re.search(old_func, content, re.DOTALL):
    content = re.sub(old_func, new_func, content, flags=re.DOTALL)
else:
    # Simpler approach: find function by signature and replace body
    start = content.find('static void zeta_3b_embed(zeta_dual_ctx_t* ctx, const char* text, float* embedding, int embed_dim)')
    if start != -1:
        # Find matching closing brace
        brace_count = 0
        in_func = False
        end = start
        for i in range(start, len(content)):
            if content[i] == '{':
                brace_count += 1
                in_func = True
            elif content[i] == '}':
                brace_count -= 1
                if in_func and brace_count == 0:
                    end = i + 1
                    break
        if end > start:
            content = content[:start] + new_func + content[end:]

print(content)
PYPATCH2

    python3 /tmp/embed_patch2.py "$TOOLS_DIR/zeta-dual-process.h" > /tmp/zeta-dual-patched.h
    cp /tmp/zeta-dual-patched.h "$TOOLS_DIR/zeta-dual-process.h"
    log "  Patched successfully"
fi

# 6. Rebuild
log ""
log "=========================================="
log "Rebuilding llama-zeta-server..."
log "=========================================="
cd "$ZETA_DIR"
cmake --build build --target llama-zeta-server -j$(nproc)

log ""
log "=========================================="
log "DEPLOYMENT COMPLETE"
log "=========================================="
log ""
log "Launch command:"
log ""
echo "  $ZETA_DIR/build/bin/llama-zeta-server \\"
echo "    -m $MODELS_DIR/qwen2.5-14b-instruct-q4.gguf \\"
echo "    --model-3b $MODELS_DIR/qwen2.5-3b-instruct-q4_k_m.gguf \\"
echo "    --model-embed $MODELS_DIR/$EMBED_FILE \\"
echo "    --port 9000 \\"
echo "    --storage /mnt/HoloGit/blocks"
log ""
