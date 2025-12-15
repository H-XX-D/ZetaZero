#!/usr/bin/env python3
"""Proper patch for embedding model support in zeta-server.cpp"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if '--model-embed' in content:
    print("Already patched!")
    sys.exit(0)

# 1. Add argument parsing after --zeta-storage line
old_arg = 'else if (strcmp(argv[i], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];'
new_arg = '''else if (strcmp(argv[i], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];
        else if (strcmp(argv[i], "--model-embed") == 0 && i+1 < argc) g_embed_model_path = argv[++i];
        else if (strcmp(argv[i], "--storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];'''

if old_arg in content:
    content = content.replace(old_arg, new_arg)
    print("Added --model-embed argument parsing")
else:
    print("WARNING: Could not find argument parsing location")

# 2. Add embedding init AFTER "3B parallel worker started" message
# Find the fprintf for worker started and add after
old_worker = 'fprintf(stderr, "3B parallel worker started\\n");'
new_worker = '''fprintf(stderr, "3B parallel worker started\\n");

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

if old_worker in content:
    content = content.replace(old_worker, new_worker)
    print("Added embedding init code")
else:
    print("WARNING: Could not find worker message location")

# Write back
with open(filepath, 'w') as f:
    f.write(content)

print("Patch complete!")
