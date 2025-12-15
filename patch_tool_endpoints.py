#!/usr/bin/env python3
"""Add tool endpoints to zeta-server.cpp"""

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'r') as f:
    content = f.read()

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

# Find the /health endpoint and add after it
if '/tools' not in content:
    # Find: svr.Get("/health"...});
    import re
    pattern = r'(svr\.Get\("/health"[^}]+\}\);)'
    match = re.search(pattern, content, re.DOTALL)
    if match:
        insert_pos = match.end()
        content = content[:insert_pos] + '\n' + tool_endpoints + content[insert_pos:]
        print("Added tool endpoints after /health")
    else:
        # Try finding svr.Get("/graph" instead
        pattern2 = r'(svr\.Get\("/graph"[^}]+\}\);)'
        match2 = re.search(pattern2, content, re.DOTALL)
        if match2:
            insert_pos = match2.start()
            content = content[:insert_pos] + tool_endpoints + '\n' + content[insert_pos:]
            print("Added tool endpoints before /graph")
        else:
            print("ERROR: Could not find insertion point")
            exit(1)
else:
    print("/tools endpoint already exists")

with open('/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-server.cpp', 'w') as f:
    f.write(content)

print("Done!")
