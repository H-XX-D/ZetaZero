#!/usr/bin/env python3
"""Patch zeta-dual-process.h to use embedding model"""

import sys
import re

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'zeta_embed_ready' in content:
    print("Already patched!")
    sys.exit(0)

# The new zeta_3b_embed function that uses embedding model
new_func = '''static inline void zeta_3b_embed(
    zeta_dual_ctx_t* ctx,
    const char* text,
    float* embedding,
    int embed_dim
) {
    (void)ctx;  // May not need ctx when using embedding model

    // Use embedding model if available (The Hippocampus)
    if (zeta_embed_ready()) {
        int result = zeta_embed_text(text, embedding, embed_dim);
        if (result > 0) {
            // Pad with zeros if embedding model dim < embed_dim
            if (result < embed_dim) {
                memset(embedding + result, 0, (embed_dim - result) * sizeof(float));
            }
            return;
        }
        fprintf(stderr, "[EMBED] Warning: Failed, falling back to hash\\n");
    }

    // Fallback to hash embedding if embedding model not available
    memset(embedding, 0, embed_dim * sizeof(float));
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        uint32_t h = (uint32_t)text[i] * 2654435761u;
        embedding[h % embed_dim] += 1.0f;
    }
    // Normalize
    float norm = 0;
    for (int i = 0; i < embed_dim; i++) norm += embedding[i] * embedding[i];
    norm = sqrtf(norm + 1e-8f);
    for (int i = 0; i < embed_dim; i++) embedding[i] /= norm;
}'''

# Find and replace the old zeta_3b_embed function
# Match from "static inline void zeta_3b_embed(" to the closing brace
pattern = r'static inline void zeta_3b_embed\(\s*zeta_dual_ctx_t\* ctx,\s*const char\* text,\s*float\* embedding,\s*int embed_dim\s*\)\s*\{[^}]+?// Normalize[^}]+?\}'

match = re.search(pattern, content, re.DOTALL)
if match:
    content = content[:match.start()] + new_func + content[match.end():]
    print("Replaced zeta_3b_embed function")
else:
    print("WARNING: Could not find zeta_3b_embed function pattern")
    # Try simpler approach
    old_start = "static inline void zeta_3b_embed("
    if old_start in content:
        start_idx = content.find(old_start)
        # Find the matching closing brace by counting
        brace_count = 0
        in_func = False
        end_idx = start_idx
        for i in range(start_idx, len(content)):
            if content[i] == '{':
                brace_count += 1
                in_func = True
            elif content[i] == '}':
                brace_count -= 1
                if in_func and brace_count == 0:
                    end_idx = i + 1
                    break
        if end_idx > start_idx:
            content = content[:start_idx] + new_func + content[end_idx:]
            print("Replaced zeta_3b_embed function (simple method)")
        else:
            print("ERROR: Could not find function end")
    else:
        print("ERROR: Could not find zeta_3b_embed")

# Write back
with open(filepath, 'w') as f:
    f.write(content)

print("Patch complete!")
