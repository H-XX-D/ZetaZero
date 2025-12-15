#!/usr/bin/env python3
"""Add fallback node creation when 3B extraction fails"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'FALLBACK' in content:
    print("Already patched!")
    sys.exit(0)

# Find the exact pattern
old_pattern = '''    return facts_created;
}

#ifdef __cplusplus'''

new_pattern = '''    // FALLBACK: Store raw input if no semantic facts extracted
    if (facts_created == 0 && strlen(text) > 20) {
        // Skip questions
        bool is_question = (strchr(text, '?') != NULL ||
                           strncasecmp(text, "what ", 5) == 0 ||
                           strncasecmp(text, "where ", 6) == 0 ||
                           strncasecmp(text, "how ", 4) == 0 ||
                           strncasecmp(text, "who ", 4) == 0 ||
                           strncasecmp(text, "when ", 5) == 0 ||
                           strncasecmp(text, "why ", 4) == 0);

        if (!is_question) {
            char value[512];
            strncpy(value, text, sizeof(value) - 1);
            value[sizeof(value) - 1] = '\\0';

            // Clean "Remember:" prefix
            if (strncasecmp(value, "remember:", 9) == 0) {
                char* start = value + 9;
                while (*start == ' ') start++;
                memmove(value, start, strlen(start) + 1);
            }

            if (strlen(value) > 5) {
                zeta_create_node(ctx, NODE_FACT, "memory", value, 0.85f);
                facts_created++;
                fprintf(stderr, "[3B:FALLBACK] Stored: %.60s...\\n", value);
            }
        }
    }

    return facts_created;
}

#ifdef __cplusplus'''

if old_pattern in content:
    content = content.replace(old_pattern, new_pattern)
    print("Added fallback storage")
else:
    print("Pattern not found!")
    # Show what we're looking for
    print("Looking for return at end of function...")

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
