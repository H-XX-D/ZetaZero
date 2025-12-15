#!/usr/bin/env python3
"""Add 'Remember:' pattern to fact extraction"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'remember:' in content.lower() and 'raw_memory' in content:
    print("Already has remember pattern!")
    sys.exit(0)

# Add remember pattern handling before the final return
# Find the update patterns section (near end of function)
old_code = '''    // Update patterns (for versioning)
    if (strstr(lower, "changed to ") || strstr(lower, "actually ") ||
        strstr(lower, "now it's ") || strstr(lower, "updated to ")) {
        // TODO: Extract old and new values for versioning
        fprintf(stderr, "[3B] Version update detected (TODO: implement)\\n");
    }

    return facts_created;'''

new_code = '''    // Update patterns (for versioning)
    if (strstr(lower, "changed to ") || strstr(lower, "actually ") ||
        strstr(lower, "now it's ") || strstr(lower, "updated to ")) {
        // TODO: Extract old and new values for versioning
        fprintf(stderr, "[3B] Version update detected (TODO: implement)\\n");
    }

    // REMEMBER PATTERN: Store anything prefixed with "Remember:"
    if (strncasecmp(text, "remember:", 9) == 0) {
        const char* val_start = text + 9;
        while (*val_start == ' ') val_start++;  // Skip leading spaces

        if (strlen(val_start) > 5) {
            char clean_val[512];
            strncpy(clean_val, val_start, sizeof(clean_val) - 1);
            clean_val[sizeof(clean_val) - 1] = '\\0';

            zeta_create_node(ctx, NODE_FACT, "raw_memory", clean_val, 0.9f);
            facts_created++;
            fprintf(stderr, "[3B:REMEMBER] Stored: %.60s...\\n", clean_val);
        }
    }
    // Also store any non-question text as context if no facts extracted
    else if (facts_created == 0 && strlen(text) > 30 && strchr(text, '?') == NULL) {
        // Skip common non-memorable phrases
        if (strncasecmp(text, "ok", 2) != 0 &&
            strncasecmp(text, "yes", 3) != 0 &&
            strncasecmp(text, "no", 2) != 0 &&
            strncasecmp(text, "hi", 2) != 0 &&
            strncasecmp(text, "hello", 5) != 0) {

            zeta_create_node(ctx, NODE_FACT, "context", text, 0.7f);
            facts_created++;
            fprintf(stderr, "[3B:CONTEXT] Stored: %.60s...\\n", text);
        }
    }

    return facts_created;'''

if old_code in content:
    content = content.replace(old_code, new_code)
    print("Added remember pattern handling")
else:
    print("Pattern not found - trying simpler match...")
    # Try to find and replace
    if "return facts_created;" in content and "Version update detected" in content:
        print("Found markers but not exact pattern")
    else:
        print("Could not find insertion point")

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
