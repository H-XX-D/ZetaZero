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
    .ctx_7b = 8192,
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
    if (config.count("CTX_7B")) g_config.ctx_7b = atoi(config["CTX_7B"].c_str());
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

#endif // ZETA_CONFIG_H
