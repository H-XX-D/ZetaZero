#!/usr/bin/env bash
# Direct patch for embedding model argument parsing and initialization

FILE="$HOME/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp"

# Backup
cp "$FILE" "$FILE.bak2"

# 1. Add --model-embed argument parsing after --zeta-storage line
sed -i 's/else if (strcmp(argv\[i\], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv\[++i\];/else if (strcmp(argv[i], "--zeta-storage") == 0 \&\& i+1 < argc) g_storage_dir = argv[++i];\n        else if (strcmp(argv[i], "--model-embed") == 0 \&\& i+1 < argc) g_embed_model_path = argv[++i];\n        else if (strcmp(argv[i], "--storage") == 0 \&\& i+1 < argc) g_storage_dir = argv[++i];/' "$FILE"

# 2. Find the "Dual-process engine initialized" line and add embed init after it
sed -i '/Dual-process engine initialized/a\    \/\/ Initialize embedding model (The Hippocampus)\n    if (!g_embed_model_path.empty()) {\n        if (zeta_embed_init(g_embed_model_path.c_str())) {\n            fprintf(stderr, "Hippocampus loaded: %d-dim semantic embeddings\\n", zeta_embed_dim());\n        } else {\n            fprintf(stderr, "WARNING: Embedding model failed, using hash fallback\\n");\n        }\n    } else {\n        fprintf(stderr, "NOTE: No --model-embed specified, using hash embeddings\\n");\n    }' "$FILE"

echo "Patch applied. Verifying..."
grep -n "model-embed\|g_embed_model_path\|zeta_embed_init" "$FILE" | head -20
