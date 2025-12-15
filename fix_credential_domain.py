#!/usr/bin/env python3
"""Fix domain classification to check label for credential detection"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'zeta_is_credential_label' in content:
    print("Already patched!")
    sys.exit(0)

# Add helper function and fix the domain check
old_code = '''        // Domain filtering: skip unrelated domains unless very high salience
        zeta_semantic_domain_t node_domain = zeta_classify_domain(node->value);
        
        // SECURITY: Credentials NEVER surface to non-credential queries
        if (node_domain == DOMAIN_CREDENTIALS && query_domain != DOMAIN_CREDENTIALS) {
            fprintf(stderr, "[STREAM:SECURITY] Blocked credential from non-credential query\\n");
            continue;  // HARD BLOCK
        }'''

new_code = '''        // Domain filtering: skip unrelated domains unless very high salience
        zeta_semantic_domain_t node_domain = zeta_classify_domain(node->value);
        
        // SECURITY: Also check label for credential detection
        // Labels like "api_token", "password", "secret" indicate credentials
        // even if value itself doesn't contain keywords
        bool is_credential_by_label = (
            strstr(node->label, "password") != NULL ||
            strstr(node->label, "token") != NULL ||
            strstr(node->label, "secret") != NULL ||
            strstr(node->label, "key") != NULL ||
            strstr(node->label, "code") != NULL ||
            strstr(node->label, "auth") != NULL ||
            strstr(node->label, "api_") != NULL ||
            strstr(node->label, "credential") != NULL
        );
        
        // SECURITY: Credentials NEVER surface to non-credential queries
        if ((node_domain == DOMAIN_CREDENTIALS || is_credential_by_label) && 
            query_domain != DOMAIN_CREDENTIALS) {
            fprintf(stderr, "[STREAM:SECURITY] Blocked credential from non-credential query (label=%s)\\n", node->label);
            continue;  // HARD BLOCK
        }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Added label-based credential detection")
else:
    print("Pattern not found")
    sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
