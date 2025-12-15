#!/usr/bin/env python3
"""Upgrade 3B context size and improve extraction intelligence"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h"

with open(filepath, 'r') as f:
    content = f.read()

# 1. Increase 3B context from 2048 to 8192
old_ctx = 'cparams.n_ctx = 2048;  // Smaller context for 3B'
new_ctx = 'cparams.n_ctx = 8192;  // Larger context for smarter 3B semantic understanding'

if old_ctx in content:
    content = content.replace(old_ctx, new_ctx)
    print("Increased 3B context from 2048 to 8192")
else:
    # Try alternate
    if 'n_ctx = 2048' in content:
        content = content.replace('n_ctx = 2048', 'n_ctx = 8192')
        print("Increased 3B context to 8192 (alternate)")
    else:
        print("3B context already modified or not found")

# 2. Also increase batch size
old_batch = 'cparams.n_batch = 1024;'
new_batch = 'cparams.n_batch = 2048;  // Larger batch for better throughput'

if old_batch in content:
    content = content.replace(old_batch, new_batch)
    print("Increased 3B batch size")

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
