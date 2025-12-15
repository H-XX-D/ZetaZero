#!/usr/bin/env python3
"""Integrate numeric conflict detection into server pipeline"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'zeta_check_numeric_conflicts' in content:
    print("Already integrated!")
    sys.exit(0)

# Find where stream_context is used and add numeric conflict check
old_code = '''    // Augment prompt with streamed memory (compact format)
    std::string augmented_prompt = std::string(stream_context) + prompt;'''

new_code = '''    // Check for numeric conflicts BEFORE generation
    char conflict_warning[512];
    conflict_warning[0] = '\\0';
    if (g_dual) {
        int conflicts = zeta_check_numeric_conflicts(g_dual, prompt.c_str(), 
                                                     conflict_warning, sizeof(conflict_warning));
        if (conflicts > 0) {
            fprintf(stderr, "[SERVER] Numeric conflicts detected: %d\\n", conflicts);
        }
    }
    
    // Augment prompt with streamed memory AND any conflict warnings
    std::string augmented_prompt = std::string(stream_context) + 
                                   std::string(conflict_warning) + prompt;'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Integrated numeric conflict check into pipeline")
else:
    print("Pattern not found")
    sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
