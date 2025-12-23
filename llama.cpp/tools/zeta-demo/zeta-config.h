// =============================================================================
// Z.E.T.A. Configuration Parser
// =============================================================================
// Reads zeta.conf shell-style configuration file
//
// Config file locations (searched in order):
//   1. ./zeta.conf (current directory)
//   2. ~/ZetaZero/zeta.conf (user home)
//   3. /etc/zeta/zeta.conf (system-wide)
//
// Format: KEY="value" or KEY=value (shell-compatible)
// Lines starting with # are comments
// =============================================================================

#ifndef ZETA_CONFIG_H
#define ZETA_CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstring>

// Configuration container
struct zeta_config_t {
    // Model paths
    std::string model_14b;
    std::string model_7b_coder;
    std::string model_embed;
    std::string model_3b_instruct;
    std::string model_3b_coder;

    // Server settings
    std::string host;
    int port;
    int gpu_layers;
    int ctx_14b;
    int ctx_7b;
    int ctx_embed;
    int batch_size;

    // Paths
    std::string storage_dir;
    std::string log_file;

    // Auth
    std::string password;

    // Loaded flag
    bool loaded;
};

// Global config instance
static zeta_config_t g_config = {
    .model_14b = "",
    .model_7b_coder = "",
    .model_embed = "",
    .model_3b_instruct = "",
    .model_3b_coder = "",
    .host = "0.0.0.0",
    .port = 8080,
    .gpu_layers = 999,
    .ctx_14b = 4096,
    .ctx_7b = 2048,    // Reduced from 8192 to fit extraction context in VRAM
    .ctx_embed = 512,
    .batch_size = 2048,
    .storage_dir = "/mnt/HoloGit/blocks",
    .log_file = "/tmp/zeta.log",
    .password = "zeta1234",
    .loaded = false
};

// Trim whitespace and quotes from value
static inline std::string trim_value(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r\"'");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r\"'");
    return s.substr(start, end - start + 1);
}

// Parse a single config line
static inline void parse_config_line(const std::string& line, std::map<std::string, std::string>& config) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') return;

    // Find = sign
    size_t eq = line.find('=');
    if (eq == std::string::npos) return;

    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);

    // Trim key
    size_t ks = key.find_first_not_of(" \t");
    size_t ke = key.find_last_not_of(" \t");
    if (ks != std::string::npos && ke != std::string::npos) {
        key = key.substr(ks, ke - ks + 1);
    }

    // Strip inline comments from value (after #)
    size_t comment_pos = value.find('#');
    if (comment_pos != std::string::npos) {
        value = value.substr(0, comment_pos);
    }

    value = trim_value(value);

    // Skip variable expansions for now (e.g., ${ZETA_HOST})
    if (value.find("${") != std::string::npos) return;

    config[key] = value;
}

// Load config from file
static inline bool zeta_load_config_file(const char* path, std::map<std::string, std::string>& config) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        parse_config_line(line, config);
    }
    return true;
}

// Find and load config file
static inline bool zeta_load_config() {
    std::map<std::string, std::string> config;
    bool found = false;

    // Search paths
    const char* search_paths[] = {
        "./zeta.conf",
        NULL,  // Will be filled with ~/ZetaZero/zeta.conf
        "/etc/zeta/zeta.conf"
    };

    // Build home path
    char home_path[512] = {0};
    const char* home = getenv("HOME");
    if (home) {
        snprintf(home_path, sizeof(home_path), "%s/ZetaZero/zeta.conf", home);
        search_paths[1] = home_path;
    }

    // Try each path
    for (int i = 0; i < 3; i++) {
        if (search_paths[i] && zeta_load_config_file(search_paths[i], config)) {
            fprintf(stderr, "[CONFIG] Loaded: %s\n", search_paths[i]);
            found = true;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "[CONFIG] No config file found, using defaults\n");
        return false;
    }

    // Apply config values
    if (config.count("MODEL_14B")) g_config.model_14b = config["MODEL_14B"];
    if (config.count("MODEL_7B_CODER")) g_config.model_7b_coder = config["MODEL_7B_CODER"];
    if (config.count("MODEL_EMBED")) g_config.model_embed = config["MODEL_EMBED"];
    if (config.count("MODEL_3B_INSTRUCT")) g_config.model_3b_instruct = config["MODEL_3B_INSTRUCT"];
    if (config.count("MODEL_3B_CODER")) g_config.model_3b_coder = config["MODEL_3B_CODER"];

    if (config.count("ZETA_HOST")) g_config.host = config["ZETA_HOST"];
    if (config.count("ZETA_PORT")) g_config.port = atoi(config["ZETA_PORT"].c_str());
    if (config.count("GPU_LAYERS")) g_config.gpu_layers = atoi(config["GPU_LAYERS"].c_str());
    if (config.count("CTX_14B")) g_config.ctx_14b = atoi(config["CTX_14B"].c_str());
    if (config.count("CTX_7B")) {
        g_config.ctx_7b = atoi(config["CTX_7B"].c_str());
        fprintf(stderr, "[CONFIG] CTX_7B parsed as: %d\n", g_config.ctx_7b);
    }
    if (config.count("CTX_EMBED")) g_config.ctx_embed = atoi(config["CTX_EMBED"].c_str());
    if (config.count("BATCH_SIZE")) g_config.batch_size = atoi(config["BATCH_SIZE"].c_str());

    if (config.count("ZETA_STORAGE")) g_config.storage_dir = config["ZETA_STORAGE"];
    if (config.count("ZETA_LOG")) g_config.log_file = config["ZETA_LOG"];
    if (config.count("ZETA_PASSWORD")) g_config.password = config["ZETA_PASSWORD"];

    g_config.loaded = true;
    return true;
}

// Print current config
static inline void zeta_print_config() {
    fprintf(stderr, "\n=== Z.E.T.A. Configuration ===\n");
    fprintf(stderr, "Models:\n");
    fprintf(stderr, "  14B:     %s\n", g_config.model_14b.empty() ? "(default)" : g_config.model_14b.c_str());
    fprintf(stderr, "  7B:      %s\n", g_config.model_7b_coder.empty() ? "(default)" : g_config.model_7b_coder.c_str());
    fprintf(stderr, "  Embed:   %s\n", g_config.model_embed.empty() ? "(default)" : g_config.model_embed.c_str());
    fprintf(stderr, "Server:\n");
    fprintf(stderr, "  Port:    %d\n", g_config.port);
    fprintf(stderr, "  GPU:     %d layers\n", g_config.gpu_layers);
    fprintf(stderr, "  Context: 14B=%d, 7B=%d, Embed=%d\n", g_config.ctx_14b, g_config.ctx_7b, g_config.ctx_embed);
    fprintf(stderr, "Storage:   %s\n", g_config.storage_dir.c_str());
    fprintf(stderr, "==============================\n\n");
}

// =============================================================================
// DREAM SUGGESTION: ContextChecker Class for Unified Context Validation
// =============================================================================
// Provides centralized context validation to ensure consistency across modules

class ZetaContextChecker {
public:
    // Validation result structure
    struct ValidationResult {
        bool is_valid;
        std::string error_message;
        std::string sanitized_value;
    };

    // Validate that a context string is non-empty and well-formed
    static ValidationResult validate_context(const std::string& context) {
        ValidationResult result;
        result.is_valid = true;
        result.error_message = "";
        result.sanitized_value = context;

        // Rule 1: Context must not be empty
        if (context.empty()) {
            result.is_valid = false;
            result.error_message = "Context cannot be empty";
            return result;
        }

        // Rule 2: Context must not exceed max length
        const size_t MAX_CONTEXT_LEN = 4096;
        if (context.length() > MAX_CONTEXT_LEN) {
            result.is_valid = false;
            result.error_message = "Context exceeds maximum length";
            return result;
        }

        // Rule 3: Context must not contain null bytes
        if (context.find('\0') != std::string::npos) {
            result.is_valid = false;
            result.error_message = "Context contains null bytes";
            return result;
        }

        // Sanitize: trim leading/trailing whitespace
        size_t start = context.find_first_not_of(" \t\n\r");
        size_t end = context.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            result.sanitized_value = context.substr(start, end - start + 1);
        }

        return result;
    }

    // Validate a context type (must be one of known types)
    static bool validate_context_type(const std::string& type) {
        static const std::vector<std::string> valid_types = {
            "emotional", "task", "domain", "temporal", "causal",
            "system", "user", "memory", "cognitive"
        };

        for (const auto& valid : valid_types) {
            if (type == valid) return true;
        }
        return false;
    }

    // Validate intensity is in valid range [0.0, 1.0]
    static bool validate_intensity(float intensity) {
        return intensity >= 0.0f && intensity <= 1.0f;
    }

    // Validate a causal relationship (cause -> effect)
    static ValidationResult validate_causal_relation(
            const std::string& cause,
            const std::string& effect) {
        ValidationResult result;
        result.is_valid = true;

        auto cause_check = validate_context(cause);
        if (!cause_check.is_valid) {
            result.is_valid = false;
            result.error_message = "Invalid cause: " + cause_check.error_message;
            return result;
        }

        auto effect_check = validate_context(effect);
        if (!effect_check.is_valid) {
            result.is_valid = false;
            result.error_message = "Invalid effect: " + effect_check.error_message;
            return result;
        }

        // Check for self-referential causation
        if (cause == effect) {
            result.is_valid = false;
            result.error_message = "Self-referential causation detected";
            return result;
        }

        result.sanitized_value = cause_check.sanitized_value + " -> " + effect_check.sanitized_value;
        return result;
    }

    // Validate a lambda value (must be positive and reasonable)
    static bool validate_lambda(float lambda) {
        const float MIN_LAMBDA = 0.0001f;
        const float MAX_LAMBDA = 1.0f;
        return lambda >= MIN_LAMBDA && lambda <= MAX_LAMBDA;
    }

    // Validate recursion depth
    static bool validate_recursion_depth(int depth) {
        const int MIN_DEPTH = 1;
        const int MAX_DEPTH = 20;
        return depth >= MIN_DEPTH && depth <= MAX_DEPTH;
    }

    // Log validation results
    static void log_validation(const std::string& context_name,
                                const ValidationResult& result) {
        if (result.is_valid) {
            fprintf(stderr, "[CONTEXT-CHECK] %s: VALID\n", context_name.c_str());
        } else {
            fprintf(stderr, "[CONTEXT-CHECK] %s: INVALID - %s\n",
                    context_name.c_str(), result.error_message.c_str());
        }
    }
};

// Convenience function for quick context validation
static inline bool zeta_check_context(const std::string& context) {
    return ZetaContextChecker::validate_context(context).is_valid;
}

// Convenience function for quick context validation with logging
static inline bool zeta_check_context_log(const std::string& name, const std::string& context) {
    auto result = ZetaContextChecker::validate_context(context);
    ZetaContextChecker::log_validation(name, result);
    return result.is_valid;
}

#endif // ZETA_CONFIG_H
