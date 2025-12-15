#!/usr/bin/env python3
"""Add security re-check in FORMAT function"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-streaming.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'FORMAT:SECURITY' in content:
    print("Already patched!")
    sys.exit(0)

# Add security check in format function
old_code = '''        // Find the node by ID
        bool found = false;
        for (int j = 0; j < ctx->num_nodes; j++) {
            if (ctx->nodes[j].node_id == state->active[i].node_id) {
                n = snprintf(p, remaining, "%s\\n", ctx->nodes[j].value);
                p += n; remaining -= n;
                fprintf(stderr, "[FORMAT] Added: %.50s (id=%lld)\\n", ctx->nodes[j].value, (long long)state->active[i].node_id);
                found = true;
                break;
            }
        }'''

new_code = '''        // Find the node by ID
        bool found = false;
        for (int j = 0; j < ctx->num_nodes; j++) {
            if (ctx->nodes[j].node_id == state->active[i].node_id) {
                // FORMAT:SECURITY - Re-check that this isn't a credential leaking into non-credential context
                zeta_semantic_domain_t nd = zeta_classify_domain(ctx->nodes[j].value);
                bool is_cred_label = (
                    strstr(ctx->nodes[j].label, "password") || strstr(ctx->nodes[j].label, "token") ||
                    strstr(ctx->nodes[j].label, "secret") || strstr(ctx->nodes[j].label, "key") ||
                    strstr(ctx->nodes[j].label, "code") || strstr(ctx->nodes[j].label, "api_") ||
                    strstr(ctx->nodes[j].label, "auth") || strstr(ctx->nodes[j].label, "credential")
                );
                // Skip credential nodes in non-credential contexts
                if ((nd == DOMAIN_CREDENTIALS || is_cred_label)) {
                    fprintf(stderr, "[FORMAT:SECURITY] Skipped credential node: %.30s (label=%s)\\n", 
                            ctx->nodes[j].value, ctx->nodes[j].label);
                    found = true;  // Mark as found but don't add
                    break;
                }
                
                n = snprintf(p, remaining, "%s\\n", ctx->nodes[j].value);
                p += n; remaining -= n;
                fprintf(stderr, "[FORMAT] Added: %.50s (id=%lld)\\n", ctx->nodes[j].value, (long long)state->active[i].node_id);
                found = true;
                break;
            }
        }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Added security re-check in FORMAT")
else:
    print("Pattern not found")
    sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
