#!/usr/bin/env python3
"""Add numeric fact extraction rules to 3B semantic prompt"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h"

with open(filepath, 'r') as f:
    content = f.read()

# Find and replace the conversational extraction prompt with enhanced version
old_rules = '''"RULE 7: 'my favorite X is Y' -> favorite_X|Y\\n"
                "NEVER output code or explanations. Output ONLY TYPE|VALUE lines.\\n"'''

new_rules = '''"RULE 7: 'my favorite X is Y' -> favorite_X|Y\\n"
                "RULE 8: 'I was born in YYYY' or 'born in YYYY' -> birth_year|YYYY\\n"
                "RULE 9: 'I am N years old' or 'my age is N' -> age|N\\n"
                "RULE 10: 'I make $N' or 'salary is $N' or 'earn $N' -> salary|N\\n"
                "RULE 11: 'my sister/brother is X' or 'sibling named X' -> sibling|X\\n"
                "RULE 12: 'I work at X' or 'employed at X' or 'my job is at X' -> workplace|X\\n"
                "RULE 13: 'my pet X is named Y' or 'X named Y' (dog/cat/pet) -> pet_name|Y\\n"
                "RULE 14: 'my password is X' or 'code is X' or 'PIN is X' -> secret_code|X (SENSITIVE)\\n"
                "CRITICAL: Extract ALL numeric facts (years, ages, amounts) - they are important!\\n"
                "NEVER output code or explanations. Output ONLY TYPE|VALUE lines.\\n"'''

if old_rules in content:
    content = content.replace(old_rules, new_rules)
    print("Added numeric extraction rules to 3B prompt")
else:
    print("Pattern not found - checking alternate...")
    if "RULE 7:" in content and "favorite_X|Y" in content:
        # Try to add after RULE 7
        old_alt = '"RULE 7: \'my favorite X is Y\' -> favorite_X|Y\\n"'
        new_alt = '''"RULE 7: 'my favorite X is Y' -> favorite_X|Y\\n"
                "RULE 8: 'I was born in YYYY' or 'born in YYYY' -> birth_year|YYYY\\n"
                "RULE 9: 'I am N years old' or 'my age is N' -> age|N\\n"
                "RULE 10: 'I make $N' or 'salary is $N' -> salary|N\\n"
                "RULE 11: 'my sister/brother is X' -> sibling|X\\n"
                "RULE 12: 'I work at X' -> workplace|X\\n"
                "RULE 13: 'my pet is named Y' -> pet_name|Y\\n"'''
        if old_alt in content:
            content = content.replace(old_alt, new_alt)
            print("Added rules after RULE 7 (alternate)")
        else:
            print("Could not find insertion point")
            sys.exit(1)
    else:
        print("Could not find rules section")
        sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
