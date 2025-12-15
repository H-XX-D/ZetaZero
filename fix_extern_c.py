#!/usr/bin/env python3
"""Fix extern C block and duplicate includes"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp"

with open(filepath, 'r') as f:
    content = f.read()

# Remove duplicate include
content = content.replace(
    '#include "zeta-embed-integration.h"\n#include "zeta-embed-integration.h"',
    '#include "zeta-embed-integration.h"'
)

# Move zeta-embed-integration.h outside of extern C block
# Since it's a C++ header
old_block = '''extern "C" {
#include "zeta-memory.h"
#include "zeta-integration.h"
#include "zeta-constitution.h"
#include "zeta-embed-integration.h"
#include "zeta-dual-process.h"
#include "zeta-cyclic.h"
#include "zeta-code-mode.h"
}'''

new_block = '''#include "zeta-embed-integration.h"

extern "C" {
#include "zeta-memory.h"
#include "zeta-integration.h"
#include "zeta-constitution.h"
}

#include "zeta-dual-process.h"
#include "zeta-cyclic.h"
#include "zeta-code-mode.h"'''

if old_block in content:
    content = content.replace(old_block, new_block)
    print("Fixed extern C block")
else:
    # Try simpler fix - just move embed before the block
    if 'extern "C" {' in content and '#include "zeta-embed-integration.h"' in content:
        # Remove embed from inside extern C
        content = content.replace('#include "zeta-embed-integration.h"\n', '', 1)
        # Add it before extern C
        content = content.replace('extern "C" {', '#include "zeta-embed-integration.h"\n\nextern "C" {')
        print("Moved embed include out of extern C")
    else:
        print("Could not find pattern to fix")

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
