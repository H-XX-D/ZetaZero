#!/usr/bin/env python3
"""Fix include order - embed before dual-process"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp"

with open(filepath, 'r') as f:
    content = f.read()

# Remove the duplicate embed include (if it exists after dual-process)
content = content.replace(
    '#include "zeta-dual-process.h"\n#include "zeta-embed-integration.h"',
    '#include "zeta-embed-integration.h"\n#include "zeta-dual-process.h"'
)

# Also handle case where there might be another include between
if '#include "zeta-embed-integration.h"' in content and '#include "zeta-dual-process.h"' in content:
    # Check order
    embed_pos = content.find('#include "zeta-embed-integration.h"')
    dual_pos = content.find('#include "zeta-dual-process.h"')

    if dual_pos < embed_pos:
        # Wrong order - remove embed from current location, add before dual
        content = content.replace('#include "zeta-embed-integration.h"\n', '')
        content = content.replace('#include "zeta-embed-integration.h"', '')
        content = content.replace(
            '#include "zeta-dual-process.h"',
            '#include "zeta-embed-integration.h"\n#include "zeta-dual-process.h"'
        )
        print("Fixed include order")
    else:
        print("Include order already correct")
else:
    print("Missing includes")

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
