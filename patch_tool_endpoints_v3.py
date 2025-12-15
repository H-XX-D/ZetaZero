#!/usr/bin/env python3
"""Add tool endpoints to zeta-server.cpp (v3 - C-style JSON, correct globals)"""

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'r') as f:
    content = f.read()

# First, remove the broken v2 endpoints
if 'TOOL SYSTEM ENDPOINTS' in content:
    # Find and remove the broken section
    start = content.find('// ========================================================================\n    // TOOL SYSTEM ENDPOINTS')
    if start != -1:
        # Find the end - look for the next svr.Get("/cache
        end = content.find('    // Cache clear endpoint', start)
        if end == -1:
            end = content.find('svr.Get("/cache', start)
        if end != -1:
            content = content[:start] + content[end:]
            print("Removed broken v2 endpoints")

# Tool endpoints code - using C-style JSON and g_dual context
tool_endpoints = '''
    // ========================================================================
    // TOOL SYSTEM ENDPOINTS
    // ========================================================================

    svr.Get("/tools", [](const httplib::Request&, httplib::Response& res) {
        std::string schema = zeta_tools::get_tool_schema();
        res.set_content(schema, "application/json");
    });

    svr.Get("/tools/describe", [](const httplib::Request&, httplib::Response& res) {
        std::string desc = zeta_tools::get_tool_prompt();
        char json[16384];
        // Escape special chars in desc for JSON
        std::string escaped_desc;
        for (char c : desc) {
            if (c == '\\n') escaped_desc += "\\\\n";
            else if (c == '\\t') escaped_desc += "\\\\t";
            else if (c == '"') escaped_desc += "\\\\\\"";
            else if (c == '\\\\') escaped_desc += "\\\\\\\\";
            else escaped_desc += c;
        }
        snprintf(json, sizeof(json), "{\\"tools\\": \\"%s\\"}", escaped_desc.c_str());
        res.set_content(json, "application/json");
    });

    svr.Post("/tool/execute", [](const httplib::Request& req, httplib::Response& res) {
        char json_out[8192];

        // Simple JSON parsing for tool name and params
        std::string body = req.body;
        std::string tool_name;
        std::map<std::string, std::string> params;

        // Extract tool name
        size_t tool_pos = body.find("\\"tool\\"");
        if (tool_pos != std::string::npos) {
            size_t start = body.find("\\"", tool_pos + 7);
            if (start != std::string::npos) {
                size_t end = body.find("\\"", start + 1);
                if (end != std::string::npos) {
                    tool_name = body.substr(start + 1, end - start - 1);
                }
            }
        }

        // Extract params (simple key-value parsing)
        size_t params_pos = body.find("\\"params\\"");
        if (params_pos != std::string::npos) {
            size_t brace_start = body.find("{", params_pos);
            size_t brace_end = body.rfind("}");
            if (brace_start != std::string::npos && brace_end > brace_start) {
                std::string params_str = body.substr(brace_start + 1, brace_end - brace_start - 1);
                // Parse key-value pairs
                size_t pos = 0;
                while (pos < params_str.size()) {
                    size_t key_start = params_str.find("\\"", pos);
                    if (key_start == std::string::npos) break;
                    size_t key_end = params_str.find("\\"", key_start + 1);
                    if (key_end == std::string::npos) break;
                    std::string key = params_str.substr(key_start + 1, key_end - key_start - 1);

                    size_t val_start = params_str.find("\\"", key_end + 1);
                    if (val_start == std::string::npos) break;
                    size_t val_end = params_str.find("\\"", val_start + 1);
                    if (val_end == std::string::npos) break;
                    std::string val = params_str.substr(val_start + 1, val_end - val_start - 1);

                    params[key] = val;
                    pos = val_end + 1;
                }
            }
        }

        if (tool_name.empty()) {
            snprintf(json_out, sizeof(json_out),
                "{\\"error\\": \\"Missing tool name\\", \\"blocked\\": true}");
            res.set_content(json_out, "application/json");
            return;
        }

        // Execute tool (pass g_dual as context for graph validation)
        auto result = zeta_tools::g_tool_registry.execute(tool_name, params,
            reinterpret_cast<zeta_ctx_t*>(g_dual));

        // Build response
        snprintf(json_out, sizeof(json_out),
            "{\\"tool\\": \\"%s\\", \\"status\\": %d, \\"output\\": \\"%.4000s\\", "
            "\\"error\\": \\"%s\\", \\"blocked\\": %s}",
            tool_name.c_str(),
            static_cast<int>(result.status),
            result.output.substr(0, 4000).c_str(),
            result.error.c_str(),
            result.status != zeta_tools::ToolStatus::SUCCESS ? "true" : "false");

        res.set_content(json_out, "application/json");
    });

'''

# Check if already added (clean version)
if 'TOOL SYSTEM ENDPOINTS' not in content:
    # Find line after /health endpoint closes
    import re

    # Find: svr.Get("/health"...});
    # Look for the closing }); after /health
    health_pattern = r'svr\.Get\("/health"[^;]+;\s*\}\);'
    match = re.search(health_pattern, content, re.DOTALL)

    if match:
        insert_pos = match.end()
        content = content[:insert_pos] + '\n' + tool_endpoints + content[insert_pos:]
        print("Added tool endpoints after /health")
    else:
        # Fallback: insert before cache clear
        cache_pos = content.find('// Cache clear endpoint')
        if cache_pos != -1:
            content = content[:cache_pos] + tool_endpoints + '\n    ' + content[cache_pos:]
            print("Added tool endpoints before cache clear")
        else:
            print("ERROR: Could not find insertion point")
            exit(1)
else:
    print("/tools endpoint section exists but may need cleanup")

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'w') as f:
    f.write(content)

print("Done!")
