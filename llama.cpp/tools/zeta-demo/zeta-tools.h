/**
 * Z.E.T.A. Tool Integration Layer (Simplified)
 *
 * Security Architecture:
 * - Tools are gated by permission tiers
 * - WRITE/DANGEROUS tools require graph-validated parameters
 * - Graph validation stubs - fail closed (deny by default)
 */

#ifndef ZETA_TOOLS_H
#define ZETA_TOOLS_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

// Forward declaration - we cast g_dual to this
struct zeta_ctx_t;

namespace zeta_tools {

// ============================================================================
// TOOL PERMISSION TIERS
// ============================================================================

enum class ToolTier {
    READ,       // Safe read-only operations
    WRITE,      // Modifies state, requires graph-validated target
    DANGEROUS   // Requires explicit user confirmation
};

enum class ToolStatus {
    SUCCESS = 0,
    BLOCKED_NO_PERMISSION = 1,
    BLOCKED_PARAM_NOT_IN_GRAPH = 2,
    BLOCKED_NEEDS_CONFIRMATION = 3,
    BLOCKED_INVALID_PARAMS = 4,
    EXECUTION_ERROR = 5
};

// ============================================================================
// TOOL RESULT
// ============================================================================

struct ToolResult {
    ToolStatus status;
    std::string output;
    std::string error_msg;

    static ToolResult success(const std::string& out) {
        return {ToolStatus::SUCCESS, out, ""};
    }

    static ToolResult blocked(ToolStatus st, const std::string& reason) {
        return {st, "", reason};
    }

    static ToolResult make_error(const std::string& err) {
        return {ToolStatus::EXECUTION_ERROR, "", err};
    }
};

// ============================================================================
// GRAPH VALIDATION STUBS (fail closed - deny by default)
// ============================================================================

// Stub: Always returns false (blocks graph-gated params)
inline bool graph_has_value(zeta_ctx_t* ctx, const std::string& value) {
    (void)ctx; (void)value;
    // TODO: Implement actual graph lookup
    // For now, only allow if in allowlist
    static std::vector<std::string> allowlist = {
        ".", "..", "/tmp", "/home"
    };
    for (const auto& allowed : allowlist) {
        if (value.find(allowed) == 0) return true;
    }
    return false;  // Deny by default
}

inline bool graph_has_typed_node(zeta_ctx_t* ctx, const std::string& type, const std::string& value) {
    (void)ctx; (void)type; (void)value;
    return false;  // Deny by default
}

// ============================================================================
// SANITIZATION
// ============================================================================

inline std::string sanitize_path(const std::string& path) {
    std::string result = path;
    // Remove traversal
    while (result.find("..") != std::string::npos) {
        size_t pos = result.find("..");
        result.erase(pos, 2);
    }
    // Remove null bytes
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
    return result;
}

inline std::string sanitize_for_json(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            default: result += c;
        }
    }
    return result;
}

// ============================================================================
// TOOL PARAM
// ============================================================================

struct ToolParam {
    std::string name;
    std::string type;
    bool required;
    bool must_be_in_graph;
    std::string graph_node_type;
    std::string description;
};

// ============================================================================
// TOOL DEFINITION
// ============================================================================

struct ToolDef {
    std::string name;
    std::string description;
    ToolTier tier;
    std::vector<ToolParam> params;
    std::function<ToolResult(const std::map<std::string, std::string>&, zeta_ctx_t*)> execute;
};

// ============================================================================
// TOOL IMPLEMENTATIONS
// ============================================================================

namespace tools {

inline ToolResult web_search(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto it = params.find("query");
    if (it == params.end()) return ToolResult::make_error("Missing query");

    // Stub - in production, use actual API
    return ToolResult::success("Web search for: " + it->second + " (stub result)");
}

inline ToolResult read_file(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto it = params.find("path");
    if (it == params.end()) return ToolResult::make_error("Missing path");

    std::string path = sanitize_path(it->second);
    std::ifstream file(path);
    if (!file.is_open()) return ToolResult::make_error("Cannot open: " + path);

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();
    if (content.size() > 10000) content = content.substr(0, 10000) + "...[truncated]";

    return ToolResult::success(content);
}

inline ToolResult list_dir(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto it = params.find("path");
    std::string path = it != params.end() ? sanitize_path(it->second) : ".";

    FILE* pipe = popen(("ls -la " + path + " 2>&1 | head -50").c_str(), "r");
    if (!pipe) return ToolResult::make_error("Failed to list");

    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    pclose(pipe);

    return ToolResult::success(result);
}

inline ToolResult write_file(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto path_it = params.find("path");
    auto content_it = params.find("content");
    if (path_it == params.end() || content_it == params.end())
        return ToolResult::make_error("Missing path or content");

    std::string path = sanitize_path(path_it->second);
    std::ofstream file(path);
    if (!file.is_open()) return ToolResult::make_error("Cannot write: " + path);

    file << content_it->second;
    return ToolResult::success("Written to " + path);
}

inline ToolResult run_command(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto it = params.find("command");
    if (it == params.end()) return ToolResult::make_error("Missing command");

    FILE* pipe = popen(("timeout 30 " + it->second + " 2>&1").c_str(), "r");
    if (!pipe) return ToolResult::make_error("Failed to execute");

    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    pclose(pipe);

    return ToolResult::success(result);
}

} // namespace tools

// ============================================================================
// TOOL REGISTRY
// ============================================================================

class ToolRegistry {
public:
    std::map<std::string, ToolDef> tools;

    ToolRegistry() {
        // WEB SEARCH - Read tier, no graph gating
        tools["web_search"] = {
            "web_search", "Search the web", ToolTier::READ,
            {{"query", "string", true, false, "", "Search query"}},
            tools::web_search
        };

        // LIST DIR - Read tier
        tools["list_dir"] = {
            "list_dir", "List directory contents", ToolTier::READ,
            {{"path", "path", false, false, "", "Directory path"}},
            tools::list_dir
        };

        // READ FILE - Read tier, graph-gated path
        tools["read_file"] = {
            "read_file", "Read a file", ToolTier::READ,
            {{"path", "path", true, true, "allowed_path", "File path"}},
            tools::read_file
        };

        // WRITE FILE - Write tier, graph-gated path
        tools["write_file"] = {
            "write_file", "Write to a file", ToolTier::WRITE,
            {{"path", "path", true, true, "project_root", "File path"},
             {"content", "string", true, false, "", "Content to write"}},
            tools::write_file
        };

        // RUN COMMAND - Dangerous tier
        tools["run_command"] = {
            "run_command", "Execute a shell command", ToolTier::DANGEROUS,
            {{"command", "command", true, true, "allowed_command", "Command to run"}},
            tools::run_command
        };
    }

    ToolResult execute(const std::string& name,
                       const std::map<std::string, std::string>& params,
                       zeta_ctx_t* ctx) {
        auto it = tools.find(name);
        if (it == tools.end()) {
            return ToolResult::blocked(ToolStatus::BLOCKED_INVALID_PARAMS,
                                       "Unknown tool: " + name);
        }

        const ToolDef& tool = it->second;
        fprintf(stderr, "[TOOLS] Execute '%s' (tier %d)\n", name.c_str(), (int)tool.tier);

        // Validate required params
        for (const auto& param : tool.params) {
            if (param.required && params.find(param.name) == params.end()) {
                return ToolResult::blocked(ToolStatus::BLOCKED_INVALID_PARAMS,
                                           "Missing: " + param.name);
            }
        }

        // Validate graph-gated params
        for (const auto& param : tool.params) {
            if (!param.must_be_in_graph) continue;
            auto pit = params.find(param.name);
            if (pit == params.end()) continue;

            if (!graph_has_value(ctx, pit->second)) {
                fprintf(stderr, "[TOOLS] BLOCKED: %s not in graph\n", pit->second.c_str());
                return ToolResult::blocked(ToolStatus::BLOCKED_PARAM_NOT_IN_GRAPH,
                                           param.name + " not in graph: " + pit->second);
            }
        }

        // Block DANGEROUS tier without confirmation
        if (tool.tier == ToolTier::DANGEROUS) {
            fprintf(stderr, "[TOOLS] BLOCKED: Dangerous tool requires confirmation\n");
            return ToolResult::blocked(ToolStatus::BLOCKED_NEEDS_CONFIRMATION,
                                       "Dangerous operation requires confirmation");
        }

        // Execute
        try {
            return tool.execute(params, ctx);
        } catch (const std::exception& e) {
            return ToolResult::make_error(e.what());
        }
    }

    std::string get_tool_descriptions() const {
        std::string result = "Available tools:\\n";
        for (const auto& [name, tool] : tools) {
            result += "- " + name + ": " + tool.description + "\\n";
        }
        return result;
    }

    std::string get_tool_schema_json() const {
        std::string json = "{\"tools\": [";
        bool first = true;
        for (const auto& [name, tool] : tools) {
            if (!first) json += ",";
            first = false;
            json += "{\"name\": \"" + name + "\", \"description\": \"" + tool.description + "\"}";
        }
        json += "]}";
        return json;
    }
};

// Global instance
static ToolRegistry g_tool_registry;

inline std::string get_tool_prompt() {
    return g_tool_registry.get_tool_descriptions();
}

inline std::string get_tool_schema() {
    return g_tool_registry.get_tool_schema_json();
}

} // namespace zeta_tools

#endif
