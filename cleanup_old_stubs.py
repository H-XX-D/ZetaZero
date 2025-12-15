#!/usr/bin/env python3
"""Remove old tool stubs from zeta-server.cpp"""

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'r') as f:
    content = f.read()

# Find and remove the GRAPH VALIDATION STUBS section
start_marker = '// ============================================================================\n// GRAPH VALIDATION STUBS FOR TOOL SYSTEM'
end_marker = '// Tool confirmation callback'

start = content.find(start_marker)
if start != -1:
    # Find the end - look for next big section or the tool_confirmation_callback
    end = content.find(end_marker, start)
    if end == -1:
        # Try to find by looking for the next function that's not part of stubs
        end = content.find('\nstatic ', start + 100)
    if end != -1:
        # Remove everything from start to end
        content = content[:start] + content[end:]
        print("Removed GRAPH VALIDATION STUBS section")
    else:
        print("Could not find end of stubs section")
else:
    print("No GRAPH VALIDATION STUBS found")

# Also remove tool_confirmation_callback if still there
if 'tool_confirmation_callback' in content:
    lines = content.split('\n')
    new_lines = []
    skip_until_brace = False
    brace_count = 0

    for line in lines:
        if 'bool tool_confirmation_callback' in line:
            skip_until_brace = True
            brace_count = 0
            continue
        if skip_until_brace:
            brace_count += line.count('{')
            brace_count -= line.count('}')
            if brace_count <= 0 and '}' in line:
                skip_until_brace = False
            continue
        new_lines.append(line)

    content = '\n'.join(new_lines)
    print("Removed tool_confirmation_callback")

# Remove any remaining extern declarations that would conflict
content = content.replace('extern bool zeta_graph_contains_value(zeta_ctx_t*, const char*);', '')
content = content.replace('extern bool zeta_graph_has_typed_node(zeta_ctx_t*, const char*, const char*);', '')
content = content.replace('extern int zeta_graph_get_values_by_type(zeta_ctx_t*, const char*, char**, int);', '')
content = content.replace('extern bool zeta_graph_delete_node(zeta_ctx_t*, const char*);', '')
content = content.replace('extern bool zeta_graph_create_node(zeta_ctx_t*, const char*, const char*, float);', '')

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'w') as f:
    f.write(content)

print("Done!")
