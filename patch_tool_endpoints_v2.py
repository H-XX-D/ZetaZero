#!/usr/bin/env python3
"""Add tool endpoints to zeta-server.cpp (v2 - line-based insertion)"""

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'r') as f:
    lines = f.readlines()

# Tool endpoints code
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
        json response;
        response["tools"] = desc;
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/tool/execute", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);

            std::string tool_name = body.value("tool", "");
            std::map<std::string, std::string> params;

            if (body.contains("params") && body["params"].is_object()) {
                for (auto& [key, val] : body["params"].items()) {
                    if (val.is_string()) {
                        params[key] = val.get<std::string>();
                    } else {
                        params[key] = val.dump();
                    }
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
        } catch (const std::exception& e) {
            json response;
            response["error"] = e.what();
            response["blocked"] = true;
            res.set_content(response.dump(), "application/json");
        }
    });

'''

# Check if already added
content = ''.join(lines)
if '/tools' in content and 'svr.Get("/tools"' in content:
    print("/tools endpoint already exists")
else:
    # Find line after /health endpoint closes
    insert_line = None
    in_health = False
    brace_count = 0

    for i, line in enumerate(lines):
        if 'svr.Get("/health"' in line:
            in_health = True
        if in_health:
            brace_count += line.count('{')
            brace_count -= line.count('}')
            if '});' in line and brace_count <= 0:
                insert_line = i + 1
                break

    if insert_line:
        lines.insert(insert_line, tool_endpoints)
        print(f"Added tool endpoints at line {insert_line}")
    else:
        # Fallback: insert before svr.Get("/cache
        for i, line in enumerate(lines):
            if 'svr.Get("/cache' in line:
                lines.insert(i, tool_endpoints)
                print(f"Added tool endpoints at line {i} (before /cache)")
                break
        else:
            print("ERROR: Could not find insertion point")
            exit(1)

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'w') as f:
    f.writelines(lines)

print("Done!")
