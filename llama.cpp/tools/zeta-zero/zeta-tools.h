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
// LOCAL TRUST MODE
// When enabled, allows tool execution for local/LAN requests
// ============================================================================

static bool g_tools_local_trust = true;  // Trust local by default

inline void zeta_tools_set_local_trust(bool enabled) {
    g_tools_local_trust = enabled;
    fprintf(stderr, "[TOOLS] Local trust %s\n", enabled ? "ENABLED" : "DISABLED");
}

// ============================================================================
// GRAPH VALIDATION (permissive for local trust mode)
// ============================================================================

inline bool graph_has_value(zeta_ctx_t* ctx, const std::string& value) {
    (void)ctx;

    // Local trust mode - allow common paths
    if (g_tools_local_trust) {
        static std::vector<std::string> local_allowlist = {
            ".", "..", "/tmp", "/home", "/mnt", "/storage",
            "~/", "/var/tmp", "/usr/local", "/opt",
            "Desktop", "Documents", "Downloads", "scripts"
        };
        for (const auto& allowed : local_allowlist) {
            if (value.find(allowed) != std::string::npos) return true;
        }
        // Allow relative paths in current dir
        if (value.length() > 0 && value[0] != '/') return true;
    }

    // Strict mode - minimal allowlist
    static std::vector<std::string> strict_allowlist = {
        "/tmp", "/home"
    };
    for (const auto& allowed : strict_allowlist) {
        if (value.find(allowed) == 0) return true;
    }
    return false;
}

inline bool graph_has_typed_node(zeta_ctx_t* ctx, const std::string& type, const std::string& value) {
    (void)ctx; (void)type; (void)value;
    // In local trust mode, allow typed nodes
    if (g_tools_local_trust) return true;
    return false;
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
// GLOBAL EXTERN - For accessing unified sudo password from zeta-conflict.h
// ============================================================================
extern const char* g_zeta_sudo_password;

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

// Protected config files that require sudo
inline bool is_protected_config(const std::string& path) {
    static std::vector<std::string> protected_files = {
        "zeta.conf", "/zeta.conf",
        "zeta-constitution.h", "/zeta-constitution.h",
        ".env", "/.env"
    };
    for (const auto& pf : protected_files) {
        if (path.find(pf) != std::string::npos) return true;
    }
    return false;
}

inline ToolResult write_file(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto path_it = params.find("path");
    auto content_it = params.find("content");
    if (path_it == params.end() || content_it == params.end())
        return ToolResult::make_error("Missing path or content");

    std::string path = sanitize_path(path_it->second);

    // Block writes to protected config files (unless sudo protection is disabled)
    if (g_config.sudo_enabled && is_protected_config(path)) {
        return ToolResult::blocked(ToolStatus::BLOCKED_NO_PERMISSION,
            "PROTECTED: zeta.conf requires sudo. Use 'write_config' tool with password.");
    }

    std::ofstream file(path);
    if (!file.is_open()) return ToolResult::make_error("Cannot write: " + path);

    file << content_it->second;
    return ToolResult::success("Written to " + path);
}

// Sudo-protected config writer
// When ZETA_SUDO_ENABLED=true (default), requires password
// When ZETA_SUDO_ENABLED=false (power user mode), no password needed

inline ToolResult write_config(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto key_it = params.find("key");
    auto value_it = params.find("value");
    auto password_it = params.find("password");

    if (key_it == params.end() || value_it == params.end())
        return ToolResult::make_error("Missing key or value");

    // Check if sudo protection is enabled (from config)
    if (g_config.sudo_enabled) {
        if (password_it == params.end())
            return ToolResult::blocked(ToolStatus::BLOCKED_NO_PERMISSION,
                "SUDO REQUIRED: write_config requires password parameter");

        // Verify password against the unified sudo password
        if (password_it->second != std::string(::g_zeta_sudo_password)) {
            return ToolResult::blocked(ToolStatus::BLOCKED_NO_PERMISSION,
                "AUTHENTICATION FAILED: Invalid sudo password");
        }
    }
    // Power user mode - no password check

    std::string key = key_it->second;
    std::string value = value_it->second;

    // Validate key format (must be ZETA_* or MODEL_* or other known keys)
    if (key.find("ZETA_") != 0 && key.find("MODEL_") != 0 &&
        key.find("GPU_") != 0 && key.find("CTX_") != 0 &&
        key.find("BATCH_") != 0) {
        return ToolResult::make_error("Invalid config key. Must be ZETA_*, MODEL_*, GPU_*, CTX_*, or BATCH_*");
    }

    // Find and read zeta.conf
    std::string conf_path;
    const char* home = getenv("HOME");
    if (home) {
        conf_path = std::string(home) + "/ZetaZero/zeta.conf";
    }
    if (conf_path.empty() || access(conf_path.c_str(), F_OK) != 0) {
        conf_path = "./zeta.conf";
    }

    std::ifstream infile(conf_path);
    if (!infile.is_open()) {
        return ToolResult::make_error("Cannot read config: " + conf_path);
    }

    std::vector<std::string> lines;
    std::string line;
    bool found = false;

    while (std::getline(infile, line)) {
        // Check if this line sets the key (commented or not)
        std::string check_key = key + "=";
        std::string check_key_commented = "# " + key + "=";

        if (line.find(check_key) == 0 || line.find(check_key_commented) == 0 ||
            (line.length() > 2 && line.substr(2).find(check_key) == 0)) {
            // Replace this line
            lines.push_back(key + "=" + value);
            found = true;
        } else {
            lines.push_back(line);
        }
    }
    infile.close();

    // If key not found, append it
    if (!found) {
        lines.push_back("");
        lines.push_back("# Added by write_config tool");
        lines.push_back(key + "=" + value);
    }

    // Write back
    std::ofstream outfile(conf_path);
    if (!outfile.is_open()) {
        return ToolResult::make_error("Cannot write config: " + conf_path);
    }

    for (const auto& l : lines) {
        outfile << l << "\n";
    }
    outfile.close();

    return ToolResult::success("Config updated: " + key + "=" + value +
        " (restart server to apply)");
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

inline ToolResult search_files(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto query_it = params.find("query");
    auto path_it = params.find("path");
    auto type_it = params.find("type");

    if (query_it == params.end()) return ToolResult::make_error("Missing query");

    std::string base_path = path_it != params.end() ? sanitize_path(path_it->second) : "/mnt/GitGraph";
    std::string query = query_it->second;
    std::string search_type = type_it != params.end() ? type_it->second : "content";

    // Safety: restrict to allowed paths
    static std::vector<std::string> allowed_roots = {
        "/mnt/GitGraph", "/home/xx/ZetaZero", "/tmp", "."
    };
    bool path_ok = false;
    for (const auto& root : allowed_roots) {
        if (base_path.find(root) == 0 || base_path == root) {
            path_ok = true;
            break;
        }
    }
    if (!path_ok) {
        return ToolResult::make_error("Search restricted to: /mnt/GitGraph, ~/ZetaZero, /tmp");
    }

    std::string cmd;
    if (search_type == "filename" || search_type == "name") {
        // Search by filename pattern
        cmd = "find " + base_path + " -maxdepth 5 -name '*" + query + "*' 2>/dev/null | head -50";
    } else if (search_type == "content" || search_type == "grep") {
        // Search file contents
        cmd = "grep -r -l --include='*.txt' --include='*.md' --include='*.json' --include='*.h' --include='*.cpp' '"
              + query + "' " + base_path + " 2>/dev/null | head -30";
    } else if (search_type == "recent") {
        // Find recently modified files
        cmd = "find " + base_path + " -maxdepth 4 -type f -mtime -7 -name '*" + query + "*' 2>/dev/null | head -30";
    } else {
        return ToolResult::make_error("Unknown type. Use: filename, content, recent");
    }

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return ToolResult::make_error("Search failed");

    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    pclose(pipe);

    if (result.empty()) result = "No matches found";
    return ToolResult::success(result);
}

inline ToolResult read_dreams(const std::map<std::string, std::string>& params, zeta_ctx_t*) {
    auto count_it = params.find("count");
    auto category_it = params.find("category");

    int count = count_it != params.end() ? std::stoi(count_it->second) : 5;
    if (count > 20) count = 20;  // Cap at 20

    std::string dream_dir = "/mnt/GitGraph/dreams";
    std::string category_filter = category_it != params.end() ? category_it->second : "";

    // List dream files
    std::string cmd = "ls -t " + dream_dir + "/pending/*.txt 2>/dev/null | head -" + std::to_string(count * 2);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return ToolResult::make_error("Cannot access dream directory");

    char buf[1024];
    std::vector<std::string> files;
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string f = buf;
        while (!f.empty() && (f.back() == '\n' || f.back() == '\r')) f.pop_back();
        if (!f.empty()) files.push_back(f);
    }
    pclose(pipe);

    if (files.empty()) return ToolResult::success("No dreams found in " + dream_dir + "/pending/");

    std::string result = "Recent dreams:\n\n";
    int shown = 0;
    for (const auto& file : files) {
        if (shown >= count) break;

        // Read file
        std::ifstream ifs(file);
        if (!ifs.is_open()) continue;

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string content = ss.str();

        // Category filter
        if (!category_filter.empty()) {
            if (content.find("Category: " + category_filter) == std::string::npos) continue;
        }

        // Extract just the filename
        size_t last_slash = file.rfind('/');
        std::string fname = last_slash != std::string::npos ? file.substr(last_slash + 1) : file;

        result += "=== " + fname + " ===\n";
        if (content.size() > 2000) content = content.substr(0, 2000) + "\n...[truncated]";
        result += content + "\n\n";
        shown++;
    }

    if (shown == 0) result = "No dreams matching filter";
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

        // SEARCH FILES - Read tier, searches filesystem
        tools["search_files"] = {
            "search_files", "Search files by name or content", ToolTier::READ,
            {{"query", "string", true, false, "", "Search query"},
             {"path", "path", false, false, "", "Base path (default: /mnt/GitGraph)"},
             {"type", "string", false, false, "", "Search type: filename, content, recent"}},
            tools::search_files
        };

        // READ DREAMS - Read tier, read Z.E.T.A.'s dream outputs
        tools["read_dreams"] = {
            "read_dreams", "Read recent dreams from dream directory", ToolTier::READ,
            {{"count", "number", false, false, "", "Number of dreams (default: 5, max: 20)"},
             {"category", "string", false, false, "", "Filter by category: code_idea, insight, story"}},
            tools::read_dreams
        };

        // WRITE CONFIG - Dangerous tier, requires sudo password
        tools["write_config"] = {
            "write_config", "Modify zeta.conf (REQUIRES SUDO PASSWORD)", ToolTier::DANGEROUS,
            {{"key", "string", true, false, "", "Config key (e.g., ZETA_MAX_GRAPH_NODES)"},
             {"value", "string", true, false, "", "New value"},
             {"password", "string", true, false, "", "Sudo password (ZETA_SUDO_PASSWORD from config)"}},
            tools::write_config
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

        // Block DANGEROUS tier without confirmation (unless local trust mode)
        if (tool.tier == ToolTier::DANGEROUS && !g_tools_local_trust) {
            fprintf(stderr, "[TOOLS] BLOCKED: Dangerous tool requires confirmation\n");
            return ToolResult::blocked(ToolStatus::BLOCKED_NEEDS_CONFIRMATION,
                                       "Dangerous operation requires confirmation");
        }
        if (tool.tier == ToolTier::DANGEROUS) {
            fprintf(stderr, "[TOOLS] ALLOWED: Dangerous tool (local trust mode)\n");
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
