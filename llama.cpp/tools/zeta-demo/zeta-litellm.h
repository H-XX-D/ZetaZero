// Z.E.T.A. LiteLLM Integration
// ============================================================================
// HTTP client for routing requests to the LiteLLM proxy server.
// This enables hybrid local/cloud model routing.
//
// Usage:
//   1. Start the proxy: python scripts/litellm_proxy.py --port 8081
//   2. Configure in zeta.conf: LITELLM_ENABLED="true"
//   3. Requests routed via ZetaLiteLLM::generate()
//
// The proxy handles all LiteLLM complexity (auth, retries, model mapping).
// This header just makes HTTP calls to it.
// ============================================================================

#ifndef ZETA_LITELLM_H
#define ZETA_LITELLM_H

#include <string>
#include <functional>
#include <atomic>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

// ============================================================================
// Configuration
// ============================================================================

struct ZetaLiteLLMConfig {
    bool enabled = false;
    std::string proxy_host = "127.0.0.1";
    int proxy_port = 8081;
    int timeout_sec = 120;
    bool fallback_to_local = true;
    std::string default_model = "gpt-4o-mini";

    // Model routing overrides
    std::string model_14b = "";      // Cloud model for 14B tasks (empty = use local)
    std::string model_7b = "";       // Cloud model for 7B tasks
    std::string model_reasoning = ""; // Cloud model for complex reasoning
    std::string model_code = "";     // Cloud model for code tasks
};

// Global config (set from zeta.conf)
static ZetaLiteLLMConfig g_litellm_config;

// ============================================================================
// LiteLLM Client
// ============================================================================

class ZetaLiteLLM {
public:
    // Check if proxy is available
    static bool is_available() {
        if (!g_litellm_config.enabled) return false;
        return check_health();
    }

    // Generate text via LiteLLM proxy
    // Returns empty string on failure (caller should fallback to local)
    static std::string generate(
        const std::string& prompt,
        int max_tokens = 512,
        float temperature = 0.7f,
        const std::string& model = ""
    ) {
        if (!g_litellm_config.enabled) {
            return "";
        }

        std::string use_model = model.empty() ? g_litellm_config.default_model : model;

        // Build JSON request
        std::string json_body = "{";
        json_body += "\"prompt\":\"" + escape_json(prompt) + "\",";
        json_body += "\"max_tokens\":" + std::to_string(max_tokens) + ",";
        json_body += "\"temperature\":" + std::to_string(temperature) + ",";
        json_body += "\"model\":\"" + use_model + "\"";
        json_body += "}";

        // Make HTTP request
        std::string response = http_post("/generate", json_body);
        if (response.empty()) {
            return "";
        }

        // Extract "output" field from JSON response
        return extract_json_field(response, "output");
    }

    // Health check
    static bool check_health() {
        std::string response = http_get("/health");
        return response.find("\"status\":\"ok\"") != std::string::npos;
    }

private:
    // Simple HTTP GET
    static std::string http_get(const std::string& path) {
        // Use platform socket API for minimal dependencies
        return http_request("GET", path, "");
    }

    // Simple HTTP POST
    static std::string http_post(const std::string& path, const std::string& body) {
        return http_request("POST", path, body);
    }

    // Core HTTP request function using raw sockets
    static std::string http_request(
        const std::string& method,
        const std::string& path,
        const std::string& body
    ) {
#ifdef _WIN32
        // Windows implementation
        return http_request_win(method, path, body);
#else
        // POSIX implementation
        return http_request_posix(method, path, body);
#endif
    }

#ifndef _WIN32
    // POSIX socket implementation
    static std::string http_request_posix(
        const std::string& method,
        const std::string& path,
        const std::string& body
    ) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(g_litellm_config.proxy_port);

        if (inet_pton(AF_INET, g_litellm_config.proxy_host.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);
            return "";
        }

        // Set timeout
        struct timeval tv;
        tv.tv_sec = g_litellm_config.timeout_sec;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "";
        }

        // Build HTTP request
        std::string request = method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + g_litellm_config.proxy_host + "\r\n";
        request += "Content-Type: application/json\r\n";
        if (!body.empty()) {
            request += "Content-Length: " + std::to_string(body.length()) + "\r\n";
        }
        request += "Connection: close\r\n\r\n";
        request += body;

        // Send request
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            close(sock);
            return "";
        }

        // Read response
        std::string response;
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_read] = '\0';
            response += buffer;
        }

        close(sock);

        // Extract body from HTTP response
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            return response.substr(body_start + 4);
        }
        return "";
    }
#else
    // Windows socket implementation
    static std::string http_request_win(
        const std::string& method,
        const std::string& path,
        const std::string& body
    ) {
        // Minimal Windows implementation - just return empty for now
        // Full implementation would use WinSock2
        return "";
    }
#endif

    // JSON string escape
    static std::string escape_json(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 10);
        for (char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    // Extract field from simple JSON
    static std::string extract_json_field(const std::string& json, const std::string& field) {
        std::string search = "\"" + field + "\":\"";
        size_t start = json.find(search);
        if (start == std::string::npos) return "";

        start += search.length();
        size_t end = start;

        // Find closing quote (handle escaped quotes)
        while (end < json.length()) {
            if (json[end] == '"' && json[end-1] != '\\') break;
            end++;
        }

        if (end >= json.length()) return "";

        std::string value = json.substr(start, end - start);

        // Unescape
        std::string result;
        for (size_t i = 0; i < value.length(); i++) {
            if (value[i] == '\\' && i + 1 < value.length()) {
                switch (value[i+1]) {
                    case 'n': result += '\n'; i++; break;
                    case 'r': result += '\r'; i++; break;
                    case 't': result += '\t'; i++; break;
                    case '"': result += '"'; i++; break;
                    case '\\': result += '\\'; i++; break;
                    default: result += value[i]; break;
                }
            } else {
                result += value[i];
            }
        }
        return result;
    }
};

// ============================================================================
// Configuration Helpers
// ============================================================================

// Parse LITELLM config from key-value pairs (called from zeta-config.h)
static inline void zeta_litellm_parse_config(
    const std::string& key,
    const std::string& value
) {
    if (key == "LITELLM_ENABLED") {
        g_litellm_config.enabled = (value == "true" || value == "1");
    } else if (key == "LITELLM_HOST") {
        g_litellm_config.proxy_host = value;
    } else if (key == "LITELLM_PORT") {
        g_litellm_config.proxy_port = std::stoi(value);
    } else if (key == "LITELLM_TIMEOUT") {
        g_litellm_config.timeout_sec = std::stoi(value);
    } else if (key == "LITELLM_FALLBACK_LOCAL") {
        g_litellm_config.fallback_to_local = (value == "true" || value == "1");
    } else if (key == "LITELLM_MODEL") {
        g_litellm_config.default_model = value;
    } else if (key == "LITELLM_14B_MODEL") {
        g_litellm_config.model_14b = value;
    } else if (key == "LITELLM_7B_MODEL") {
        g_litellm_config.model_7b = value;
    } else if (key == "LITELLM_REASONING_MODEL") {
        g_litellm_config.model_reasoning = value;
    } else if (key == "LITELLM_CODE_MODEL") {
        g_litellm_config.model_code = value;
    }
}

// Get model for task type
static inline std::string zeta_litellm_get_model(const std::string& task_type) {
    if (task_type == "reasoning" && !g_litellm_config.model_reasoning.empty()) {
        return g_litellm_config.model_reasoning;
    }
    if (task_type == "code" && !g_litellm_config.model_code.empty()) {
        return g_litellm_config.model_code;
    }
    if (task_type == "14B" && !g_litellm_config.model_14b.empty()) {
        return g_litellm_config.model_14b;
    }
    if (task_type == "7B" && !g_litellm_config.model_7b.empty()) {
        return g_litellm_config.model_7b;
    }
    return g_litellm_config.default_model;
}

// Convenience function matching generate_fn signature
static inline std::string zeta_litellm_generate(
    const std::string& prompt,
    int max_tokens,
    float temperature,
    float /* penalty - not used by cloud APIs */
) {
    return ZetaLiteLLM::generate(prompt, max_tokens, temperature);
}

#endif // ZETA_LITELLM_H
