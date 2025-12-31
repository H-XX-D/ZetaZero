// Z.E.T.A. MCP (Model Context Protocol) Integration
// JSON-RPC style tool calling interface compatible with MCP spec
//
// Wraps existing zeta-tools.h with MCP protocol layer:
// - Tool discovery (tools/list)
// - Tool calling (tools/call)
// - Resource access (resources/read)
// - Prompt templates (prompts/get)
//
// Security: All tool calls go through existing graph-gated validation
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_MCP_H
#define ZETA_MCP_H

#include "zeta-tools.h"
#include <cstring>
#include <cstdio>

namespace zeta_mcp {

// MCP Protocol Version
#define ZETA_MCP_VERSION "2024-11-05"

// ============================================================================
// MCP MESSAGE TYPES
// ============================================================================

enum class MCPMethod {
    INITIALIZE,
    TOOLS_LIST,
    TOOLS_CALL,
    RESOURCES_LIST,
    RESOURCES_READ,
    PROMPTS_LIST,
    PROMPTS_GET,
    UNKNOWN
};

struct MCPRequest {
    std::string jsonrpc;    // Should be "2.0"
    std::string id;
    MCPMethod method;
    std::string method_str;
    std::map<std::string, std::string> params;
    std::string raw_params; // For complex nested params
};

struct MCPResponse {
    std::string id;
    bool is_error;
    int error_code;
    std::string error_message;
    std::string result;     // JSON string
};

// ============================================================================
// JSON PARSING HELPERS (minimal, no external deps)
// ============================================================================

inline std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";

    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";

    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}

inline std::string extract_json_object(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find("{", pos);
    if (pos == std::string::npos) return "";

    int depth = 1;
    size_t end = pos + 1;
    while (end < json.size() && depth > 0) {
        if (json[end] == '{') depth++;
        else if (json[end] == '}') depth--;
        end++;
    }

    return json.substr(pos, end - pos);
}

inline MCPMethod parse_method(const std::string& method) {
    if (method == "initialize") return MCPMethod::INITIALIZE;
    if (method == "tools/list") return MCPMethod::TOOLS_LIST;
    if (method == "tools/call") return MCPMethod::TOOLS_CALL;
    if (method == "resources/list") return MCPMethod::RESOURCES_LIST;
    if (method == "resources/read") return MCPMethod::RESOURCES_READ;
    if (method == "prompts/list") return MCPMethod::PROMPTS_LIST;
    if (method == "prompts/get") return MCPMethod::PROMPTS_GET;
    return MCPMethod::UNKNOWN;
}

// ============================================================================
// MCP REQUEST PARSING
// ============================================================================

inline MCPRequest parse_mcp_request(const std::string& json) {
    MCPRequest req;
    req.jsonrpc = extract_json_string(json, "jsonrpc");
    req.id = extract_json_string(json, "id");
    req.method_str = extract_json_string(json, "method");
    req.method = parse_method(req.method_str);
    req.raw_params = extract_json_object(json, "params");

    // Extract common params
    if (!req.raw_params.empty()) {
        req.params["name"] = extract_json_string(req.raw_params, "name");
        req.params["uri"] = extract_json_string(req.raw_params, "uri");

        // Extract arguments for tools/call
        std::string args = extract_json_object(req.raw_params, "arguments");
        if (!args.empty()) {
            // Parse key-value pairs from arguments object
            size_t pos = 0;
            while ((pos = args.find("\"", pos)) != std::string::npos) {
                size_t key_end = args.find("\"", pos + 1);
                if (key_end == std::string::npos) break;

                std::string key = args.substr(pos + 1, key_end - pos - 1);
                if (key.empty() || key[0] == '{' || key[0] == '}') {
                    pos = key_end + 1;
                    continue;
                }

                size_t colon = args.find(":", key_end);
                if (colon == std::string::npos) break;

                size_t val_start = args.find("\"", colon);
                if (val_start == std::string::npos) break;

                size_t val_end = args.find("\"", val_start + 1);
                if (val_end == std::string::npos) break;

                std::string val = args.substr(val_start + 1, val_end - val_start - 1);
                req.params[key] = val;

                pos = val_end + 1;
            }
        }
    }

    return req;
}

// ============================================================================
// MCP RESPONSE BUILDING
// ============================================================================

inline std::string build_mcp_response(const MCPResponse& resp) {
    std::string json = "{\"jsonrpc\":\"2.0\",\"id\":\"" + resp.id + "\"";

    if (resp.is_error) {
        json += ",\"error\":{\"code\":" + std::to_string(resp.error_code);
        json += ",\"message\":\"" + zeta_tools::sanitize_for_json(resp.error_message) + "\"}}";
    } else {
        json += ",\"result\":" + resp.result + "}";
    }

    return json;
}

inline MCPResponse make_error(const std::string& id, int code, const std::string& msg) {
    return {id, true, code, msg, ""};
}

inline MCPResponse make_result(const std::string& id, const std::string& result_json) {
    return {id, false, 0, "", result_json};
}

// ============================================================================
// MCP HANDLERS
// ============================================================================

inline MCPResponse handle_initialize(const MCPRequest& req) {
    std::string result = R"({
        "protocolVersion": ")" ZETA_MCP_VERSION R"(",
        "serverInfo": {
            "name": "zeta-mcp",
            "version": "1.0.0"
        },
        "capabilities": {
            "tools": {},
            "resources": {},
            "prompts": {}
        }
    })";
    return make_result(req.id, result);
}

inline MCPResponse handle_tools_list(const MCPRequest& req) {
    std::string tools_json = "[";
    bool first = true;

    for (const auto& [name, tool] : zeta_tools::g_tool_registry.tools) {
        if (!first) tools_json += ",";
        first = false;

        tools_json += "{\"name\":\"" + name + "\"";
        tools_json += ",\"description\":\"" + zeta_tools::sanitize_for_json(tool.description) + "\"";
        tools_json += ",\"inputSchema\":{\"type\":\"object\",\"properties\":{";

        bool first_param = true;
        for (const auto& param : tool.params) {
            if (!first_param) tools_json += ",";
            first_param = false;
            tools_json += "\"" + param.name + "\":{\"type\":\"string\"";
            tools_json += ",\"description\":\"" + zeta_tools::sanitize_for_json(param.description) + "\"}";
        }
        tools_json += "}}}";
    }
    tools_json += "]";

    return make_result(req.id, "{\"tools\":" + tools_json + "}");
}

inline MCPResponse handle_tools_call(const MCPRequest& req, zeta_context_t* ctx) {
    std::string tool_name = req.params.count("name") ? req.params.at("name") : "";

    if (tool_name.empty()) {
        return make_error(req.id, -32602, "Missing tool name");
    }

    fprintf(stderr, "[MCP] tools/call: %s\n", tool_name.c_str());

    // Build params map (excluding 'name' which is the tool name)
    std::map<std::string, std::string> tool_params;
    for (const auto& [k, v] : req.params) {
        if (k != "name" && k != "uri") {
            tool_params[k] = v;
        }
    }

    // Execute via existing tool registry
    // Note: ctx is currently unused by tools, cast to expected type
    zeta_tools::ToolResult result = zeta_tools::g_tool_registry.execute(tool_name, tool_params, (zeta_ctx_t*)ctx);

    if (result.status != zeta_tools::ToolStatus::SUCCESS) {
        // Map status to MCP error
        int code = -32000;
        switch (result.status) {
            case zeta_tools::ToolStatus::BLOCKED_NO_PERMISSION:
                code = -32001; break;
            case zeta_tools::ToolStatus::BLOCKED_PARAM_NOT_IN_GRAPH:
                code = -32002; break;
            case zeta_tools::ToolStatus::BLOCKED_NEEDS_CONFIRMATION:
                code = -32003; break;
            case zeta_tools::ToolStatus::BLOCKED_INVALID_PARAMS:
                code = -32602; break;
            default:
                code = -32000;
        }
        return make_error(req.id, code, result.error_msg);
    }

    // Build success response
    std::string content = "{\"content\":[{\"type\":\"text\",\"text\":\"";
    content += zeta_tools::sanitize_for_json(result.output);
    content += "\"}]}";

    return make_result(req.id, content);
}

inline MCPResponse handle_resources_list(const MCPRequest& req) {
    // Expose graph memory as a resource
    std::string result = R"({
        "resources": [
            {
                "uri": "memory://graph",
                "name": "Z.E.T.A. Memory Graph",
                "description": "Persistent memory graph storage",
                "mimeType": "application/json"
            },
            {
                "uri": "memory://identity",
                "name": "Z.E.T.A. Identity",
                "description": "Core identity facts",
                "mimeType": "text/plain"
            }
        ]
    })";
    return make_result(req.id, result);
}

inline MCPResponse handle_resources_read(const MCPRequest& req) {
    std::string uri = req.params.count("uri") ? req.params.at("uri") : "";

    if (uri == "memory://identity") {
        std::string content = R"({
            "contents": [{
                "uri": "memory://identity",
                "mimeType": "text/plain",
                "text": "I am Z.E.T.A., created by Alex in 2025."
            }]
        })";
        return make_result(req.id, content);
    }

    if (uri == "memory://graph") {
        // Would return graph state - stub for now
        std::string content = R"({
            "contents": [{
                "uri": "memory://graph",
                "mimeType": "application/json",
                "text": "{\"nodes\": [], \"edges\": []}"
            }]
        })";
        return make_result(req.id, content);
    }

    return make_error(req.id, -32602, "Unknown resource: " + uri);
}

inline MCPResponse handle_prompts_list(const MCPRequest& req) {
    std::string result = R"({
        "prompts": [
            {
                "name": "code_review",
                "description": "Review code for issues",
                "arguments": [
                    {"name": "code", "description": "Code to review", "required": true}
                ]
            },
            {
                "name": "explain",
                "description": "Explain a concept",
                "arguments": [
                    {"name": "topic", "description": "Topic to explain", "required": true}
                ]
            }
        ]
    })";
    return make_result(req.id, result);
}

inline MCPResponse handle_prompts_get(const MCPRequest& req) {
    std::string name = req.params.count("name") ? req.params.at("name") : "";

    if (name == "code_review") {
        std::string code = req.params.count("code") ? req.params.at("code") : "";
        std::string result = R"({
            "messages": [{
                "role": "user",
                "content": {
                    "type": "text",
                    "text": "Please review this code for bugs, security issues, and improvements:\n\n)" +
            zeta_tools::sanitize_for_json(code) + R"("
                }
            }]
        })";
        return make_result(req.id, result);
    }

    if (name == "explain") {
        std::string topic = req.params.count("topic") ? req.params.at("topic") : "";
        std::string result = R"({
            "messages": [{
                "role": "user",
                "content": {
                    "type": "text",
                    "text": "Please explain: )" + zeta_tools::sanitize_for_json(topic) + R"("
                }
            }]
        })";
        return make_result(req.id, result);
    }

    return make_error(req.id, -32602, "Unknown prompt: " + name);
}

// ============================================================================
// MAIN MCP HANDLER
// ============================================================================

inline MCPResponse handle_mcp_request(const std::string& json, zeta_context_t* ctx) {
    MCPRequest req = parse_mcp_request(json);

    fprintf(stderr, "[MCP] Method: %s, ID: %s\n", req.method_str.c_str(), req.id.c_str());

    switch (req.method) {
        case MCPMethod::INITIALIZE:
            return handle_initialize(req);
        case MCPMethod::TOOLS_LIST:
            return handle_tools_list(req);
        case MCPMethod::TOOLS_CALL:
            return handle_tools_call(req, ctx);
        case MCPMethod::RESOURCES_LIST:
            return handle_resources_list(req);
        case MCPMethod::RESOURCES_READ:
            return handle_resources_read(req);
        case MCPMethod::PROMPTS_LIST:
            return handle_prompts_list(req);
        case MCPMethod::PROMPTS_GET:
            return handle_prompts_get(req);
        default:
            return make_error(req.id, -32601, "Unknown method: " + req.method_str);
    }
}

// Convenience function for string in/out
inline std::string process_mcp(const std::string& json, zeta_context_t* ctx) {
    MCPResponse resp = handle_mcp_request(json, ctx);
    return build_mcp_response(resp);
}

} // namespace zeta_mcp

#endif // ZETA_MCP_H
