// Z.E.T.A. Cloud Routing - Optional cloud API integration
//
// Enables smart routing to OpenAI/Anthropic for complex queries while
// keeping memory operations, dreams, and simple queries local.
//
// DISABLED BY DEFAULT - requires explicit opt-in via zeta.conf
//
// Z.E.T.A.(TM) | (C) 2025 All rights reserved.

#ifndef ZETA_CLOUD_H
#define ZETA_CLOUD_H

#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// Simple HTTP client using system curl (no additional dependencies)
// This keeps the build simple and Docker-compatible

namespace zeta_cloud {

// Cloud routing configuration
struct CloudConfig {
    bool enabled = false;
    float complexity_threshold = 0.7f;
    std::string provider = "openai";      // openai, anthropic, both
    std::string model = "gpt-4o";
    std::string openai_key;
    std::string anthropic_key;
};

static CloudConfig g_cloud_config;

// Initialize cloud config from environment
inline void init_from_env() {
    const char* routing = getenv("CLOUD_ROUTING");
    if (routing && strcmp(routing, "true") == 0) {
        g_cloud_config.enabled = true;
    }

    const char* threshold = getenv("CLOUD_COMPLEXITY_THRESHOLD");
    if (threshold) {
        g_cloud_config.complexity_threshold = std::stof(threshold);
    }

    const char* provider = getenv("CLOUD_PROVIDER");
    if (provider) {
        g_cloud_config.provider = provider;
    }

    const char* model = getenv("CLOUD_MODEL");
    if (model) {
        g_cloud_config.model = model;
    }

    const char* openai_key = getenv("OPENAI_API_KEY");
    if (openai_key) {
        g_cloud_config.openai_key = openai_key;
    }

    const char* anthropic_key = getenv("ANTHROPIC_API_KEY");
    if (anthropic_key) {
        g_cloud_config.anthropic_key = anthropic_key;
    }

    // Disable if no keys provided
    if (g_cloud_config.enabled &&
        g_cloud_config.openai_key.empty() &&
        g_cloud_config.anthropic_key.empty()) {
        fprintf(stderr, "[CLOUD] Warning: CLOUD_ROUTING=true but no API keys provided. Disabling.\n");
        g_cloud_config.enabled = false;
    }

    if (g_cloud_config.enabled) {
        fprintf(stderr, "[CLOUD] Smart routing ENABLED (threshold=%.2f, provider=%s, model=%s)\n",
                g_cloud_config.complexity_threshold,
                g_cloud_config.provider.c_str(),
                g_cloud_config.model.c_str());
    }
}

// Estimate query complexity (0-1)
// Uses simple heuristics - can be enhanced with embedding similarity later
inline float estimate_complexity(const std::string& prompt) {
    float score = 0.0f;

    // Length factor
    if (prompt.length() > 500) score += 0.1f;
    if (prompt.length() > 1000) score += 0.1f;
    if (prompt.length() > 2000) score += 0.1f;

    // Multi-step indicators
    if (prompt.find("step by step") != std::string::npos) score += 0.15f;
    if (prompt.find("explain") != std::string::npos && prompt.find("detail") != std::string::npos) score += 0.1f;

    // Code complexity
    if (prompt.find("refactor") != std::string::npos) score += 0.15f;
    if (prompt.find("implement") != std::string::npos) score += 0.1f;
    if (prompt.find("debug") != std::string::npos) score += 0.1f;
    if (prompt.find("optimize") != std::string::npos) score += 0.1f;

    // Reasoning indicators
    if (prompt.find("why") != std::string::npos) score += 0.05f;
    if (prompt.find("compare") != std::string::npos) score += 0.1f;
    if (prompt.find("analyze") != std::string::npos) score += 0.1f;
    if (prompt.find("pros and cons") != std::string::npos) score += 0.15f;

    // Creative/complex tasks
    if (prompt.find("write a") != std::string::npos && prompt.length() > 200) score += 0.2f;
    if (prompt.find("design") != std::string::npos) score += 0.15f;
    if (prompt.find("architect") != std::string::npos) score += 0.2f;

    // Cap at 1.0
    return score > 1.0f ? 1.0f : score;
}

// Check if query should be routed to cloud
inline bool should_route_to_cloud(const std::string& prompt) {
    if (!g_cloud_config.enabled) return false;

    float complexity = estimate_complexity(prompt);
    bool route = complexity >= g_cloud_config.complexity_threshold;

    if (route) {
        fprintf(stderr, "[CLOUD] Routing to cloud (complexity=%.2f >= threshold=%.2f)\n",
                complexity, g_cloud_config.complexity_threshold);
    }

    return route;
}

// Escape string for JSON
inline std::string json_escape(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

// Call OpenAI API via curl
inline std::string call_openai(const std::string& prompt, const std::string& context, int max_tokens) {
    if (g_cloud_config.openai_key.empty()) {
        return "{\"error\": \"No OpenAI API key configured\"}";
    }

    // Build the enhanced prompt with Z.E.T.A. context
    std::string enhanced_prompt;
    if (!context.empty()) {
        enhanced_prompt = "[ZETA_CONTEXT]\n" + context + "\n[/ZETA_CONTEXT]\n\n" + prompt;
    } else {
        enhanced_prompt = prompt;
    }

    // Build JSON payload
    std::string payload = "{\"model\":\"" + g_cloud_config.model + "\","
                         "\"messages\":[{\"role\":\"user\",\"content\":\"" + json_escape(enhanced_prompt) + "\"}],"
                         "\"max_tokens\":" + std::to_string(max_tokens) + "}";

    // Write payload to temp file (avoids shell escaping issues)
    FILE* tmp = fopen("/tmp/zeta_cloud_payload.json", "w");
    if (!tmp) return "{\"error\": \"Failed to create temp file\"}";
    fprintf(tmp, "%s", payload.c_str());
    fclose(tmp);

    // Call curl
    std::string cmd = "curl -s https://api.openai.com/v1/chat/completions "
                     "-H 'Content-Type: application/json' "
                     "-H 'Authorization: Bearer " + g_cloud_config.openai_key + "' "
                     "-d @/tmp/zeta_cloud_payload.json 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "{\"error\": \"Failed to call curl\"}";

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);

    // Clean up
    remove("/tmp/zeta_cloud_payload.json");

    return result;
}

// Call Anthropic API via curl
inline std::string call_anthropic(const std::string& prompt, const std::string& context, int max_tokens) {
    if (g_cloud_config.anthropic_key.empty()) {
        return "{\"error\": \"No Anthropic API key configured\"}";
    }

    // Build the enhanced prompt with Z.E.T.A. context
    std::string enhanced_prompt;
    if (!context.empty()) {
        enhanced_prompt = "[ZETA_CONTEXT]\n" + context + "\n[/ZETA_CONTEXT]\n\n" + prompt;
    } else {
        enhanced_prompt = prompt;
    }

    // Determine model
    std::string model = g_cloud_config.model;
    if (model.find("gpt") != std::string::npos) {
        model = "claude-sonnet-4-20250514";  // Default Anthropic model
    }

    // Build JSON payload
    std::string payload = "{\"model\":\"" + model + "\","
                         "\"max_tokens\":" + std::to_string(max_tokens) + ","
                         "\"messages\":[{\"role\":\"user\",\"content\":\"" + json_escape(enhanced_prompt) + "\"}]}";

    // Write payload to temp file
    FILE* tmp = fopen("/tmp/zeta_cloud_payload.json", "w");
    if (!tmp) return "{\"error\": \"Failed to create temp file\"}";
    fprintf(tmp, "%s", payload.c_str());
    fclose(tmp);

    // Call curl
    std::string cmd = "curl -s https://api.anthropic.com/v1/messages "
                     "-H 'Content-Type: application/json' "
                     "-H 'x-api-key: " + g_cloud_config.anthropic_key + "' "
                     "-H 'anthropic-version: 2023-06-01' "
                     "-d @/tmp/zeta_cloud_payload.json 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "{\"error\": \"Failed to call curl\"}";

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);

    // Clean up
    remove("/tmp/zeta_cloud_payload.json");

    return result;
}

// Parse OpenAI response to extract content
inline std::string parse_openai_response(const std::string& response) {
    // Find "content": "..."
    size_t pos = response.find("\"content\":");
    if (pos == std::string::npos) {
        // Check for error
        if (response.find("\"error\"") != std::string::npos) {
            return "[Cloud Error] " + response;
        }
        return "[Cloud Error] Failed to parse response";
    }

    pos = response.find('"', pos + 10);
    if (pos == std::string::npos) return "[Cloud Error] Malformed response";

    size_t end = pos + 1;
    while (end < response.size()) {
        if (response[end] == '"' && response[end-1] != '\\') break;
        end++;
    }

    std::string content = response.substr(pos + 1, end - pos - 1);

    // Unescape
    std::string result;
    for (size_t i = 0; i < content.size(); i++) {
        if (content[i] == '\\' && i + 1 < content.size()) {
            char next = content[i + 1];
            if (next == 'n') { result += '\n'; i++; }
            else if (next == 't') { result += '\t'; i++; }
            else if (next == '"') { result += '"'; i++; }
            else if (next == '\\') { result += '\\'; i++; }
            else result += content[i];
        } else {
            result += content[i];
        }
    }

    return result;
}

// Parse Anthropic response to extract content
inline std::string parse_anthropic_response(const std::string& response) {
    // Find "text": "..."
    size_t pos = response.find("\"text\":");
    if (pos == std::string::npos) {
        // Check for error
        if (response.find("\"error\"") != std::string::npos) {
            return "[Cloud Error] " + response;
        }
        return "[Cloud Error] Failed to parse response";
    }

    pos = response.find('"', pos + 7);
    if (pos == std::string::npos) return "[Cloud Error] Malformed response";

    size_t end = pos + 1;
    while (end < response.size()) {
        if (response[end] == '"' && response[end-1] != '\\') break;
        end++;
    }

    std::string content = response.substr(pos + 1, end - pos - 1);

    // Unescape
    std::string result;
    for (size_t i = 0; i < content.size(); i++) {
        if (content[i] == '\\' && i + 1 < content.size()) {
            char next = content[i + 1];
            if (next == 'n') { result += '\n'; i++; }
            else if (next == 't') { result += '\t'; i++; }
            else if (next == '"') { result += '"'; i++; }
            else if (next == '\\') { result += '\\'; i++; }
            else result += content[i];
        } else {
            result += content[i];
        }
    }

    return result;
}

// Main routing function - call cloud with Z.E.T.A. context
inline std::string route_to_cloud(const std::string& prompt, const std::string& zeta_context, int max_tokens) {
    std::string response;
    std::string content;

    if (g_cloud_config.provider == "anthropic" ||
        (g_cloud_config.provider == "both" && !g_cloud_config.anthropic_key.empty())) {
        response = call_anthropic(prompt, zeta_context, max_tokens);
        content = parse_anthropic_response(response);
    } else {
        response = call_openai(prompt, zeta_context, max_tokens);
        content = parse_openai_response(response);
    }

    fprintf(stderr, "[CLOUD] Response received (%zu chars)\n", content.length());
    return content;
}

} // namespace zeta_cloud

#endif // ZETA_CLOUD_H
