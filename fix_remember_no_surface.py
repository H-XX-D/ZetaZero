#!/usr/bin/env python3
"""Fix: Remember statements should not trigger fact surfacing"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'Remember: skip' in content or 'REMEMBER: skip' in content:
    print("Already patched!")
    sys.exit(0)

# Find the start of zeta_stream_surface_one function and add a check
old_code = '''// Classify query domain for filtering
    zeta_semantic_domain_t query_domain = DOMAIN_GENERAL;
    if (query && strlen(query) > 0) {
        query_domain = zeta_classify_domain(query);
        fprintf(stderr, "[STREAM] Query domain: %s\\n", zeta_domain_name(query_domain));
    }'''

new_code = '''// REMEMBER statements are storage commands, not queries - skip surfacing
    if (query && strlen(query) > 0 && strncasecmp(query, "remember:", 9) == 0) {
        fprintf(stderr, "[STREAM] Remember: skip surfacing for storage command\\n");
        return 0;  // Don't surface any facts for Remember statements
    }
    
    // Classify query domain for filtering
    zeta_semantic_domain_t query_domain = DOMAIN_GENERAL;
    if (query && strlen(query) > 0) {
        query_domain = zeta_classify_domain(query);
        fprintf(stderr, "[STREAM] Query domain: %s\\n", zeta_domain_name(query_domain));
    }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Added Remember skip check")
else:
    print("Pattern not found - checking variations...")
    # Try simpler pattern
    if 'zeta_classify_domain(query)' in content:
        print("Found domain classification but not exact pattern")
    sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
