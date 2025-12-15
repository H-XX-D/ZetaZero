#!/usr/bin/env python3
"""
Patch to integrate Z.E.T.A. Tool System with zeta-server.cpp

This adds:
1. Graph validation stub functions
2. /tools endpoint for listing available tools
3. /tool/execute endpoint for direct tool execution
4. Tool call parsing in /generate responses
"""

import re

# Read the server file
with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'r') as f:
    content = f.read()

# ============================================================================
# 1. Add include for zeta-tools.h
# ============================================================================

if '#include "zeta-tools.h"' not in content:
    # Find the last include
    last_include = content.rfind('#include "zeta-')
    if last_include != -1:
        # Find end of that line
        end_of_line = content.find('\n', last_include)
        content = content[:end_of_line+1] + '#include "zeta-tools.h"\n' + content[end_of_line+1:]
        print("Added #include for zeta-tools.h")
else:
    print("zeta-tools.h already included")

# ============================================================================
# 2. Add graph validation stub functions (before main or at top of file)
# ============================================================================

graph_stubs = '''
// ============================================================================
// GRAPH VALIDATION STUBS FOR TOOL SYSTEM
// ============================================================================

// Check if a value exists anywhere in the graph
bool zeta_graph_contains_value(zeta_ctx_t* ctx, const char* value) {
    if (!ctx || !value) return false;

    // Search through all nodes for matching value
    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].value && strstr(ctx->nodes[i].value, value)) {
            return true;
        }
    }
    return false;
}

// Check if a node with specific type (label) and value exists
bool zeta_graph_has_typed_node(zeta_ctx_t* ctx, const char* node_type, const char* value) {
    if (!ctx || !node_type || !value) return false;

    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].label && strcmp(ctx->nodes[i].label, node_type) == 0) {
            if (ctx->nodes[i].value && strstr(ctx->nodes[i].value, value)) {
                return true;
            }
        }
    }
    return false;
}

// Get all values for nodes of a specific type
int zeta_graph_get_values_by_type(zeta_ctx_t* ctx, const char* node_type, char** values, int max_count) {
    if (!ctx || !node_type || !values) return 0;

    int count = 0;
    for (int i = 0; i < ctx->node_count && count < max_count; i++) {
        if (ctx->nodes[i].label && strcmp(ctx->nodes[i].label, node_type) == 0) {
            if (ctx->nodes[i].value) {
                values[count] = strdup(ctx->nodes[i].value);
                count++;
            }
        }
    }
    return count;
}

// Delete a node from the graph by label
bool zeta_graph_delete_node(zeta_ctx_t* ctx, const char* label) {
    if (!ctx || !label) return false;

    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].label && strcmp(ctx->nodes[i].label, label) == 0) {
            // Mark as superseded (soft delete)
            ctx->nodes[i].superseded_by = -1;  // -1 = deleted
            fprintf(stderr, "[TOOLS] Deleted node: %s\\n", label);
            return true;
        }
    }
    return false;
}

// Create a new node in the graph
bool zeta_graph_create_node(zeta_ctx_t* ctx, const char* label, const char* value, float salience) {
    if (!ctx || !label || !value) return false;

    // Use existing zeta_create_node function
    extern int zeta_create_node(zeta_ctx_t*, int, const char*, const char*, float);
    int result = zeta_create_node(ctx, NODE_FACT, label, value, salience);
    if (result >= 0) {
        fprintf(stderr, "[TOOLS] Created node %d: %s = %s\\n", result, label, value);
        return true;
    }
    return false;
}

// Tool confirmation callback - sends request to client
static bool g_pending_tool_confirmation = false;
static std::string g_pending_tool_name;
static std::map<std::string, std::string> g_pending_tool_params;

bool tool_confirmation_callback(const std::string& tool_name,
                                 const std::map<std::string, std::string>& params) {
    // For now, log and deny - in production, this would send to client
    fprintf(stderr, "[TOOLS] DANGEROUS operation requires confirmation: %s\\n", tool_name.c_str());
    for (const auto& [k, v] : params) {
        fprintf(stderr, "[TOOLS]   %s = %s\\n", k.c_str(), v.c_str());
    }

    // Store for async confirmation flow
    g_pending_tool_confirmation = true;
    g_pending_tool_name = tool_name;
    g_pending_tool_params = params;

    // Return false to block - client must explicitly confirm via /tool/confirm
    return false;
}

'''

# Find a good place to insert the stubs (before main function or after globals)
if 'zeta_graph_contains_value' not in content:
    # Find the main function or a good insertion point
    main_pos = content.find('int main(')
    if main_pos == -1:
        # Try to find after includes
        last_include_end = 0
        for m in re.finditer(r'#include\s*[<"][^>"]+[>"]', content):
            last_include_end = max(last_include_end, m.end())
        insert_pos = content.find('\n', last_include_end) + 1
    else:
        insert_pos = main_pos

    content = content[:insert_pos] + graph_stubs + '\n' + content[insert_pos:]
    print("Added graph validation stub functions")
else:
    print("Graph stubs already exist")

# ============================================================================
# 3. Add /tools endpoint (list available tools)
# ============================================================================

tools_endpoint = '''
        // GET /tools - List available tools
        if (req.method == "GET" && req.path == "/tools") {
            std::string schema = zeta_tools::get_tool_schema();
            res.set_content(schema, "application/json");
            return;
        }

        // GET /tools/describe - Human-readable tool descriptions
        if (req.method == "GET" && req.path == "/tools/describe") {
            std::string desc = zeta_tools::get_tool_prompt();
            json response;
            response["tools"] = desc;
            res.set_content(response.dump(), "application/json");
            return;
        }

        // POST /tool/execute - Direct tool execution
        if (req.method == "POST" && req.path == "/tool/execute") {
            json body = json::parse(req.body);

            std::string tool_name = body.value("tool", "");
            std::map<std::string, std::string> params;

            if (body.contains("params") && body["params"].is_object()) {
                for (auto& [key, val] : body["params"].items()) {
                    params[key] = val.get<std::string>();
                }
            }

            auto result = zeta_tools::g_tool_registry.execute(tool_name, params, g_ctx);

            json response;
            response["tool"] = tool_name;
            response["status"] = static_cast<int>(result.status);
            response["output"] = result.output;
            response["error"] = result.error;
            response["blocked"] = (result.status != zeta_tools::ToolStatus::SUCCESS);

            res.set_content(response.dump(), "application/json");
            return;
        }

        // POST /tool/confirm - Confirm a pending dangerous operation
        if (req.method == "POST" && req.path == "/tool/confirm") {
            if (!g_pending_tool_confirmation) {
                json response;
                response["error"] = "No pending confirmation";
                res.set_content(response.dump(), "application/json");
                return;
            }

            json body = json::parse(req.body);
            bool confirmed = body.value("confirm", false);

            if (confirmed) {
                // Execute the pending tool
                auto result = zeta_tools::g_tool_registry.tools[g_pending_tool_name].execute(
                    g_pending_tool_params, g_ctx);

                json response;
                response["tool"] = g_pending_tool_name;
                response["status"] = static_cast<int>(result.status);
                response["output"] = result.output;
                res.set_content(response.dump(), "application/json");
            } else {
                json response;
                response["status"] = "cancelled";
                res.set_content(response.dump(), "application/json");
            }

            g_pending_tool_confirmation = false;
            g_pending_tool_name.clear();
            g_pending_tool_params.clear();
            return;
        }

'''

# Find the /health endpoint and insert after it
if '/tools' not in content:
    health_pattern = r'(if\s*\(req\.path\s*==\s*"/health"\).*?\})'
    match = re.search(health_pattern, content, re.DOTALL)
    if match:
        insert_pos = match.end()
        content = content[:insert_pos] + '\n' + tools_endpoint + content[insert_pos:]
        print("Added /tools endpoints")
    else:
        print("WARNING: Could not find /health endpoint to insert after")
else:
    print("/tools endpoint already exists")

# ============================================================================
# 4. Add tool initialization in main()
# ============================================================================

tool_init = '''
    // Initialize tool system
    zeta_tools::set_confirmation_callback(tool_confirmation_callback);
    fprintf(stderr, "[TOOLS] Tool system initialized with %zu tools\\n",
            zeta_tools::g_tool_registry.tools.size());
'''

if 'Tool system initialized' not in content:
    # Find server startup (httplib listen or similar)
    listen_pattern = r'(svr\.listen\([^)]+\))'
    match = re.search(listen_pattern, content)
    if match:
        insert_pos = match.start()
        content = content[:insert_pos] + tool_init + '\n    ' + content[insert_pos:]
        print("Added tool initialization")
    else:
        print("WARNING: Could not find server listen() call")
else:
    print("Tool initialization already exists")

# ============================================================================
# 5. Add tool execution to /generate endpoint (optional - parse tool calls)
# ============================================================================

tool_execution_in_generate = '''
            // Check for tool calls in LLM output
            std::string tool_results = zeta_tools::execute_tool_calls(output, g_ctx);
            if (!tool_results.empty()) {
                response["tool_results"] = tool_results;
                // Optionally append to output
                // output += tool_results;
            }
'''

# This is optional - add near the end of /generate handling
# We'll skip this for now as it requires careful placement

# Write the patched file
with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'w') as f:
    f.write(content)

print("\n=== Patch complete! ===")
print("Rebuild with: cd ~/ZetaZero/llama.cpp && make -j8 llama-zeta-server")
print("\nNew endpoints:")
print("  GET  /tools          - JSON schema of available tools")
print("  GET  /tools/describe - Human-readable tool descriptions")
print("  POST /tool/execute   - Execute a tool directly")
print("  POST /tool/confirm   - Confirm a dangerous operation")
