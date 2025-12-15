#!/usr/bin/env python3
"""
Add --embed-model flag and integrate causal embeddings into zeta-server
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp'

with open(filepath, 'r') as f:
    content = f.read()

# 1. Add include for causal embeddings header after embed-integration
old_include = '#include "zeta-embed-integration.h"'
new_include = '''#include "zeta-embed-integration.h"
#include "zeta-causal-embeddings.h"'''

if 'zeta-causal-embeddings.h' not in content:
    content = content.replace(old_include, new_include)
    print("Added include for zeta-causal-embeddings.h")

# 2. Add embed model path variable
old_var = 'static std::string g_embed_model_path;'
new_var = '''static std::string g_embed_model_path;
static bool g_use_semantic_causal = false;'''

if 'g_use_semantic_causal' not in content:
    content = content.replace(old_var, new_var)
    print("Added g_use_semantic_causal flag")

# 3. Add --embed-model flag to usage message
old_usage = '''fprintf(stderr, "Usage: %s -m model_14b.gguf [--model-3b model_3b.gguf] [--model-3b-coder coder.gguf] [--port 9000]\\n", argv[0]);'''
new_usage = '''fprintf(stderr, "Usage: %s -m model_14b.gguf [--model-3b model_3b.gguf] [--embed-model bge.gguf] [--port 9000]\\n", argv[0]);'''

if '--embed-model' not in content:
    content = content.replace(old_usage, new_usage)
    print("Updated usage message")

# 4. Add --embed-model flag parsing
old_parse = '''else if (strcmp(argv[i], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];'''
new_parse = '''else if (strcmp(argv[i], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];
        else if (strcmp(argv[i], "--embed-model") == 0 && i+1 < argc) g_embed_model_path = argv[++i];'''

if '--embed-model' not in content or 'g_embed_model_path = argv' not in content:
    content = content.replace(old_parse, new_parse)
    print("Added --embed-model flag parsing")

# 5. Add embed model initialization after server startup message
# Look for where 14B model is loaded and add embed init after
old_init = '''fprintf(stderr, "Z.E.T.A. Server v5.0 (Parallel Dual-Process)\\n");'''
new_init = '''fprintf(stderr, "Z.E.T.A. Server v5.0 (Parallel Dual-Process)\\n");

    // Initialize embedding model for semantic causal extraction
    if (!g_embed_model_path.empty()) {
        if (zeta_embed_init(g_embed_model_path.c_str())) {
            if (zeta_causal_init_anchors()) {
                g_use_semantic_causal = true;
                fprintf(stderr, "[CAUSAL-EMB] Semantic causal extraction ENABLED\\n");
            }
        }
    }'''

if 'Semantic causal extraction ENABLED' not in content:
    content = content.replace(old_init, new_init)
    print("Added embedding initialization at startup")

with open(filepath, 'w') as f:
    f.write(content)

print("\\nServer patch complete!")
print("Now rebuild and test with: --embed-model ~/models/bge-small-en-v1.5-q8_0.gguf")
