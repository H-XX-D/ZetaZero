#!/usr/bin/env python3
"""
Hook semantic causal extraction into the Remember: path
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h'

with open(filepath, 'r') as f:
    content = f.read()

# Add include for causal embeddings at the top
if 'zeta-causal-embeddings.h' not in content:
    old_include = '#include <math.h>'
    new_include = '''#include <math.h>

// Forward declaration - actual header is in zeta-causal-embeddings.h
// If embedding model loaded, use semantic extraction
extern bool g_use_semantic_causal;
int zeta_causal_extract_edges(zeta_dual_ctx_t* ctx, const char* text);'''

    if old_include in content:
        content = content.replace(old_include, new_include)
        print("Added forward declaration for semantic extraction")
    else:
        print("Could not find math.h include")

# Find the Remember: extraction section and add semantic call
# Look for the end of the Remember block where we added verb-based extraction
old_remember_end = '''            return facts_created;  // Skip full 3B extraction but kept causal edges'''

new_remember_end = '''            // Use semantic causal extraction if available
            extern bool g_use_semantic_causal;
            if (g_use_semantic_causal) {
                extern int zeta_causal_extract_edges(zeta_dual_ctx_t*, const char*);
                int sem_edges = zeta_causal_extract_edges(ctx, content);
                facts_created += sem_edges;
                fprintf(stderr, "[SEMANTIC] Extracted %d causal edges\\n", sem_edges);
            }

            return facts_created;  // Skip full 3B extraction but kept causal edges'''

if 'Use semantic causal extraction' not in content and old_remember_end in content:
    content = content.replace(old_remember_end, new_remember_end)
    print("Added semantic extraction call to Remember: path")
elif 'Use semantic causal extraction' in content:
    print("Semantic extraction already added")
else:
    print("Could not find Remember: return statement")
    # Check if the pattern exists
    if 'Skip full 3B extraction' in content:
        print("Found 'Skip full 3B' marker")

with open(filepath, 'w') as f:
    f.write(content)

print("\\nPatch complete!")
