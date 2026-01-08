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
    std::string gkv_dir;      // Graph-KV cache directory
    std::string dream_dir;    // Dream output directory
    std::string log_file;

    // Auth - unified sudo password for all admin operations
    std::string sudo_password;
    bool sudo_enabled;  // If false, sudo protection is disabled (power user mode)

    // Constitutional Lock
    int lock_level;     // 1=embedded, 2=header, 3=remote, 4=model

    // =========================================================================
    // ADVANCED RUNTIME PARAMETERS
    // =========================================================================

    // Memory Tier Thresholds
    float cold_momentum;        // Below this = cold storage candidate
    float eviction_threshold;   // Below this = immediate evict

    // Knowledge Graph Limits
    int max_graph_nodes;
    int max_edges;
    int max_hop_depth;
    int max_edges_per_node;

    // Edge Management
    int edge_soft_cap;
    int edge_hard_cap;
    int edge_target;
    float correlation_boost;
    float correlation_decay;

    // Deduplication
    float dedup_threshold;
    float merge_threshold;

    // Output Control
    int base_output_chars;
    int base_output_words;
    float max_complexity_scale;

    // Conversation History
    int conv_history_size;
    int conv_turn_len;

    // Tunnel Search
    float tunnel_threshold;
    int tunnel_beam_width;
    int tunnel_max_hops;
    int tunnel_max_results;
    float tunnel_min_momentum;

    // Streaming Memory (these are the [RUNTIME] ones from zeta-server.cpp)
    int stream_token_budget;
    int stream_max_nodes;
    int code_token_budget;
    int code_max_nodes;

    // Prefetch
    int prefetch_lookahead;
    int prefetch_max_nodes;

    // Graph-KV
    int gkv_max_tokens;
    int gkv_max_layers;

    // Loaded flag
    bool loaded;
};

// Global config instance (C++17 compatible - no designated initializers)
static zeta_config_t g_config = {
    "",             // model_14b
    "",             // model_7b_coder
    "",             // model_embed
    "",             // model_3b_instruct
    "",             // model_3b_coder
    "0.0.0.0",      // host
    8080,           // port
    999,            // gpu_layers
    4096,           // ctx_14b
    2048,           // ctx_7b (reduced from 8192 to fit extraction context in VRAM)
    512,            // ctx_embed
    2048,           // batch_size
    "/storage",     // storage_dir
    "/storage/graph_kv",    // gkv_dir
    "/storage/dreams",      // dream_dir
    "/tmp/zeta.log",// log_file
    "zeta1234",     // sudo_password (default - CHANGE IN PRODUCTION)
    true,           // sudo_enabled (true = require password, false = power user mode)
    3,              // lock_level (default: remote verification)

    // Advanced runtime parameters - defaults match #define values
    0.30f,          // cold_momentum
    0.1f,           // eviction_threshold
    10000,          // max_graph_nodes
    50000,          // max_edges
    5,              // max_hop_depth
    8,              // max_edges_per_node
    8000,           // edge_soft_cap
    12000,          // edge_hard_cap
    6000,           // edge_target
    0.1f,           // correlation_boost
    0.95f,          // correlation_decay
    0.85f,          // dedup_threshold
    0.90f,          // merge_threshold
    1000,           // base_output_chars
    150,            // base_output_words
    6.0f,           // max_complexity_scale
    8,              // conv_history_size
    512,            // conv_turn_len
    0.3f,           // tunnel_threshold
    8,              // tunnel_beam_width
    6,              // tunnel_max_hops
    16,             // tunnel_max_results
    0.1f,           // tunnel_min_momentum
    600,            // stream_token_budget
    6,              // stream_max_nodes
    900,            // code_token_budget
    10,             // code_max_nodes
    3,              // prefetch_lookahead
    8,              // prefetch_max_nodes
    512,            // gkv_max_tokens
    64,             // gkv_max_layers
    false           // loaded
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

static inline std::string zeta_expand_user_path(const std::string &path) {
    if (path.size() >= 2 && path[0] == '~' && path[1] == '/') {
        const char *home = getenv("HOME");
        if (home && home[0] != '\0') {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
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
    if (config.count("MODEL_14B")) g_config.model_14b = zeta_expand_user_path(config["MODEL_14B"]);
    if (config.count("MODEL_7B_CODER")) g_config.model_7b_coder = zeta_expand_user_path(config["MODEL_7B_CODER"]);
    if (config.count("MODEL_EMBED")) g_config.model_embed = zeta_expand_user_path(config["MODEL_EMBED"]);
    if (config.count("MODEL_3B_INSTRUCT")) g_config.model_3b_instruct = zeta_expand_user_path(config["MODEL_3B_INSTRUCT"]);
    if (config.count("MODEL_3B_CODER")) g_config.model_3b_coder = zeta_expand_user_path(config["MODEL_3B_CODER"]);

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
    if (config.count("ZETA_GKV_DIR")) g_config.gkv_dir = config["ZETA_GKV_DIR"];
    if (config.count("ZETA_DREAM_DIR")) g_config.dream_dir = config["ZETA_DREAM_DIR"];
    if (config.count("ZETA_LOG")) g_config.log_file = config["ZETA_LOG"];
    // ZETA_SUDO_PASSWORD is the canonical name, ZETA_PASSWORD kept for backwards compatibility
    if (config.count("ZETA_PASSWORD")) g_config.sudo_password = config["ZETA_PASSWORD"];  // legacy
    if (config.count("ZETA_SUDO_PASSWORD")) g_config.sudo_password = config["ZETA_SUDO_PASSWORD"];  // preferred
    if (config.count("ZETA_SUDO_ENABLED")) {
        std::string val = config["ZETA_SUDO_ENABLED"];
        g_config.sudo_enabled = (val == "true" || val == "1" || val == "yes");
        fprintf(stderr, "[CONFIG] Sudo protection: %s\n", g_config.sudo_enabled ? "ENABLED" : "DISABLED (power user mode)");
    }
    if (config.count("ZETA_LOCK_LEVEL")) {
        g_config.lock_level = atoi(config["ZETA_LOCK_LEVEL"].c_str());
        if (g_config.lock_level < 1) g_config.lock_level = 1;
        if (g_config.lock_level > 4) g_config.lock_level = 4;
        fprintf(stderr, "[CONFIG] Constitutional Lock Level: %d\n", g_config.lock_level);
    }

    // =========================================================================
    // ADVANCED RUNTIME PARAMETERS
    // These correspond to #define values that can now be overridden at runtime
    // =========================================================================

    // Memory Tier Thresholds
    if (config.count("ZETA_COLD_MOMENTUM")) g_config.cold_momentum = atof(config["ZETA_COLD_MOMENTUM"].c_str());
    if (config.count("ZETA_EVICTION_THRESHOLD")) g_config.eviction_threshold = atof(config["ZETA_EVICTION_THRESHOLD"].c_str());

    // Knowledge Graph Limits
    if (config.count("ZETA_MAX_GRAPH_NODES")) g_config.max_graph_nodes = atoi(config["ZETA_MAX_GRAPH_NODES"].c_str());
    if (config.count("ZETA_MAX_EDGES")) g_config.max_edges = atoi(config["ZETA_MAX_EDGES"].c_str());
    if (config.count("ZETA_MAX_HOP_DEPTH")) g_config.max_hop_depth = atoi(config["ZETA_MAX_HOP_DEPTH"].c_str());
    if (config.count("ZETA_MAX_EDGES_PER_NODE")) g_config.max_edges_per_node = atoi(config["ZETA_MAX_EDGES_PER_NODE"].c_str());

    // Edge Management & Pruning
    if (config.count("ZETA_EDGE_SOFT_CAP")) g_config.edge_soft_cap = atoi(config["ZETA_EDGE_SOFT_CAP"].c_str());
    if (config.count("ZETA_EDGE_HARD_CAP")) g_config.edge_hard_cap = atoi(config["ZETA_EDGE_HARD_CAP"].c_str());
    if (config.count("ZETA_EDGE_TARGET")) g_config.edge_target = atoi(config["ZETA_EDGE_TARGET"].c_str());
    if (config.count("ZETA_CORRELATION_BOOST")) g_config.correlation_boost = atof(config["ZETA_CORRELATION_BOOST"].c_str());
    if (config.count("ZETA_CORRELATION_DECAY")) g_config.correlation_decay = atof(config["ZETA_CORRELATION_DECAY"].c_str());

    // Deduplication System
    if (config.count("ZETA_DEDUP_THRESHOLD")) g_config.dedup_threshold = atof(config["ZETA_DEDUP_THRESHOLD"].c_str());
    if (config.count("ZETA_MERGE_THRESHOLD")) g_config.merge_threshold = atof(config["ZETA_MERGE_THRESHOLD"].c_str());

    // Output Control & Verbosity
    if (config.count("ZETA_BASE_OUTPUT_CHARS")) g_config.base_output_chars = atoi(config["ZETA_BASE_OUTPUT_CHARS"].c_str());
    if (config.count("ZETA_BASE_OUTPUT_WORDS")) g_config.base_output_words = atoi(config["ZETA_BASE_OUTPUT_WORDS"].c_str());
    if (config.count("ZETA_MAX_COMPLEXITY_SCALE")) g_config.max_complexity_scale = atof(config["ZETA_MAX_COMPLEXITY_SCALE"].c_str());

    // Conversation History
    if (config.count("ZETA_CONV_HISTORY_SIZE")) g_config.conv_history_size = atoi(config["ZETA_CONV_HISTORY_SIZE"].c_str());
    if (config.count("ZETA_CONV_TURN_LEN")) g_config.conv_turn_len = atoi(config["ZETA_CONV_TURN_LEN"].c_str());

    // Tunnel Search
    if (config.count("ZETA_TUNNEL_THRESHOLD")) g_config.tunnel_threshold = atof(config["ZETA_TUNNEL_THRESHOLD"].c_str());
    if (config.count("ZETA_TUNNEL_BEAM_WIDTH")) g_config.tunnel_beam_width = atoi(config["ZETA_TUNNEL_BEAM_WIDTH"].c_str());
    if (config.count("ZETA_TUNNEL_MAX_HOPS")) g_config.tunnel_max_hops = atoi(config["ZETA_TUNNEL_MAX_HOPS"].c_str());
    if (config.count("ZETA_TUNNEL_MAX_RESULTS")) g_config.tunnel_max_results = atoi(config["ZETA_TUNNEL_MAX_RESULTS"].c_str());
    if (config.count("ZETA_TUNNEL_MIN_MOMENTUM")) g_config.tunnel_min_momentum = atof(config["ZETA_TUNNEL_MIN_MOMENTUM"].c_str());

    // Streaming Memory
    if (config.count("ZETA_STREAM_TOKEN_BUDGET")) g_config.stream_token_budget = atoi(config["ZETA_STREAM_TOKEN_BUDGET"].c_str());
    if (config.count("ZETA_STREAM_MAX_NODES")) g_config.stream_max_nodes = atoi(config["ZETA_STREAM_MAX_NODES"].c_str());
    if (config.count("ZETA_CODE_TOKEN_BUDGET")) g_config.code_token_budget = atoi(config["ZETA_CODE_TOKEN_BUDGET"].c_str());
    if (config.count("ZETA_CODE_MAX_NODES")) g_config.code_max_nodes = atoi(config["ZETA_CODE_MAX_NODES"].c_str());

    // Proactive Prefetch
    if (config.count("ZETA_PREFETCH_LOOKAHEAD")) g_config.prefetch_lookahead = atoi(config["ZETA_PREFETCH_LOOKAHEAD"].c_str());
    if (config.count("ZETA_PREFETCH_MAX_NODES")) g_config.prefetch_max_nodes = atoi(config["ZETA_PREFETCH_MAX_NODES"].c_str());

    // Graph-KV Cache
    if (config.count("ZETA_GKV_MAX_TOKENS")) g_config.gkv_max_tokens = atoi(config["ZETA_GKV_MAX_TOKENS"].c_str());
    if (config.count("ZETA_GKV_MAX_LAYERS")) g_config.gkv_max_layers = atoi(config["ZETA_GKV_MAX_LAYERS"].c_str());

    // Log advanced config if any were set
    bool advanced_loaded = false;
    if (config.count("ZETA_COLD_MOMENTUM") || config.count("ZETA_MAX_GRAPH_NODES") ||
        config.count("ZETA_TUNNEL_THRESHOLD") || config.count("ZETA_STREAM_TOKEN_BUDGET")) {
        advanced_loaded = true;
        fprintf(stderr, "[CONFIG] Advanced parameters loaded from config file\n");
    }

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
    fprintf(stderr, "Paths:\n");
    fprintf(stderr, "  Storage: %s\n", g_config.storage_dir.c_str());
    fprintf(stderr, "  GKV:     %s\n", g_config.gkv_dir.c_str());
    fprintf(stderr, "  Dreams:  %s\n", g_config.dream_dir.c_str());
    fprintf(stderr, "Advanced (runtime):\n");
    fprintf(stderr, "  Graph:   nodes=%d, edges=%d, hop_depth=%d\n",
            g_config.max_graph_nodes, g_config.max_edges, g_config.max_hop_depth);
    fprintf(stderr, "  Edge:    soft=%d, hard=%d, target=%d\n",
            g_config.edge_soft_cap, g_config.edge_hard_cap, g_config.edge_target);
    fprintf(stderr, "  Memory:  cold_mom=%.2f, evict=%.2f\n",
            g_config.cold_momentum, g_config.eviction_threshold);
    fprintf(stderr, "  Tunnel:  thresh=%.2f, beam=%d, hops=%d\n",
            g_config.tunnel_threshold, g_config.tunnel_beam_width, g_config.tunnel_max_hops);
    fprintf(stderr, "  Stream:  tokens=%d, nodes=%d\n",
            g_config.stream_token_budget, g_config.stream_max_nodes);
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
