#!/usr/bin/env python3
"""Fix credential leak - credentials must NEVER surface to non-credential queries"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# The buggy code allows high-salience credentials to leak
old_code = '''        // Domain filtering: skip unrelated domains unless very high salience
        zeta_semantic_domain_t node_domain = zeta_classify_domain(node->value);
        if (!zeta_domains_related(query_domain, node_domain) && node->salience < 0.9f) {
            continue;  // Skip unrelated domain
        }'''

# Fixed code: credentials NEVER surface unless query is credential-related
new_code = '''        // Domain filtering: skip unrelated domains unless very high salience
        zeta_semantic_domain_t node_domain = zeta_classify_domain(node->value);
        
        // SECURITY: Credentials NEVER surface to non-credential queries
        if (node_domain == DOMAIN_CREDENTIALS && query_domain != DOMAIN_CREDENTIALS) {
            fprintf(stderr, "[STREAM:SECURITY] Blocked credential from non-credential query\\n");
            continue;  // HARD BLOCK
        }
        
        if (!zeta_domains_related(query_domain, node_domain) && node->salience < 0.9f) {
            continue;  // Skip unrelated domain
        }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Fixed credential leak vulnerability")
else:
    print("Pattern not found - checking if already patched")
    if "SECURITY" in content and "Blocked credential" in content:
        print("Already patched!")
        sys.exit(0)
    else:
        print("Could not find pattern to fix")
        sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
