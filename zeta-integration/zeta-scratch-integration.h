// Z.E.T.A. Scratch Buffer Integration
//
// Integrates scratch buffer with llama.cpp server:
// 1. Register control tokens with llama vocab
// 2. Hook into decode loop for token processing
// 3. Wire <|inject|> to GitGraph queries
// 4. HTTP endpoints for scratch buffer state
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_SCRATCH_INTEGRATION_H
#define ZETA_SCRATCH_INTEGRATION_H

#include "zeta-scratch-buffer.h"
#include "llama.h"
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// PART 1: Token Registration
// ============================================================================

// All scratch buffer control tokens
static const char* SCRATCH_CONTROL_TOKENS[] = {
    "<|scratch_start|>",
    "<|scratch_end|>",
    "<|checkpoint|>",
    "<|revise_from|>",
    "<|section|>",
    "<|flush|>",
    "<|clear|>",
    "<|inject|>",
    NULL
};

// Token ID storage (filled by registration)
typedef struct {
    int32_t scratch_start;
    int32_t scratch_end;
    int32_t checkpoint;
    int32_t revise;
    int32_t section;
    int32_t flush;
    int32_t clear;
    int32_t inject;
    bool registered;
} zeta_scratch_token_ids_t;

static zeta_scratch_token_ids_t g_scratch_tok_ids = {0};

// Check if a token is one of our control tokens by string matching
static inline int32_t zeta_scratch_find_token(const llama_vocab* vocab, const char* text) {
    if (!vocab || !text) return -1;
    
    // Try to find the token in vocab
    std::vector<llama_token> tokens(8);
    int n = llama_tokenize(vocab, text, strlen(text), tokens.data(), tokens.size(), false, true);
    
    // If tokenizes to single token, that's our ID
    if (n == 1) {
        return tokens[0];
    }
    
    // Otherwise, need to add as special token (would require model modification)
    return -1;
}

// Register tokens with vocab (call at server init)
static inline bool zeta_scratch_register_tokens(const llama_vocab* vocab) {
    if (!vocab) return false;
    
    g_scratch_tok_ids.scratch_start = zeta_scratch_find_token(vocab, "<|scratch_start|>");
    g_scratch_tok_ids.scratch_end = zeta_scratch_find_token(vocab, "<|scratch_end|>");
    g_scratch_tok_ids.checkpoint = zeta_scratch_find_token(vocab, "<|checkpoint|>");
    g_scratch_tok_ids.revise = zeta_scratch_find_token(vocab, "<|revise_from|>");
    g_scratch_tok_ids.section = zeta_scratch_find_token(vocab, "<|section|>");
    g_scratch_tok_ids.flush = zeta_scratch_find_token(vocab, "<|flush|>");
    g_scratch_tok_ids.clear = zeta_scratch_find_token(vocab, "<|clear|>");
    g_scratch_tok_ids.inject = zeta_scratch_find_token(vocab, "<|inject|>");
    
    // Update global scratch tokens struct
    g_scratch_tokens.tok_scratch_start = g_scratch_tok_ids.scratch_start;
    g_scratch_tokens.tok_scratch_end = g_scratch_tok_ids.scratch_end;
    g_scratch_tokens.tok_checkpoint = g_scratch_tok_ids.checkpoint;
    g_scratch_tokens.tok_revise = g_scratch_tok_ids.revise;
    g_scratch_tokens.tok_section = g_scratch_tok_ids.section;
    g_scratch_tokens.tok_flush = g_scratch_tok_ids.flush;
    g_scratch_tokens.tok_clear = g_scratch_tok_ids.clear;
    g_scratch_tokens.tok_inject = g_scratch_tok_ids.inject;
    g_scratch_tokens.initialized = true;
    
    g_scratch_tok_ids.registered = true;
    
    fprintf(stderr, "[SCRATCH] Token registration:\n");
    fprintf(stderr, "  scratch_start: %d\n", g_scratch_tok_ids.scratch_start);
    fprintf(stderr, "  scratch_end:   %d\n", g_scratch_tok_ids.scratch_end);
    fprintf(stderr, "  checkpoint:    %d\n", g_scratch_tok_ids.checkpoint);
    fprintf(stderr, "  revise:        %d\n", g_scratch_tok_ids.revise);
    fprintf(stderr, "  inject:        %d\n", g_scratch_tok_ids.inject);
    
    return true;
}

// Alternative: Pattern-based detection (doesn't require vocab registration)
// This works with any model by detecting token sequences
static inline bool zeta_scratch_detect_control_sequence(
    const char* text,
    int* out_control_type
) {
    if (!text || !out_control_type) return false;
    
    if (strstr(text, "<|scratch_start|>")) { *out_control_type = 0; return true; }
    if (strstr(text, "<|scratch_end|>")) { *out_control_type = 1; return true; }
    if (strstr(text, "<|checkpoint|>")) { *out_control_type = 2; return true; }
    if (strstr(text, "<|revise_from|>")) { *out_control_type = 3; return true; }
    if (strstr(text, "<|section|>")) { *out_control_type = 4; return true; }
    if (strstr(text, "<|flush|>")) { *out_control_type = 5; return true; }
    if (strstr(text, "<|clear|>")) { *out_control_type = 6; return true; }
    if (strstr(text, "<|inject|>")) { *out_control_type = 7; return true; }
    
    return false;
}

// ============================================================================
// PART 2: GitGraph Injection Integration
// ============================================================================

// Forward declaration - actual graph query function
struct zeta_dual_ctx_t;
typedef const char* (*zeta_graph_query_fn)(struct zeta_dual_ctx_t* ctx, const char* query, void* user_data);

typedef struct {
    struct zeta_dual_ctx_t* graph_ctx;
    zeta_graph_query_fn query_fn;
    void* user_data;
    
    // Pending injection
    char pending_query[256];
    bool has_pending;
} zeta_inject_ctx_t;

static zeta_inject_ctx_t g_inject_ctx = {0};

// Set up injection context
static inline void zeta_scratch_set_inject_ctx(
    struct zeta_dual_ctx_t* graph_ctx,
    zeta_graph_query_fn query_fn,
    void* user_data
) {
    g_inject_ctx.graph_ctx = graph_ctx;
    g_inject_ctx.query_fn = query_fn;
    g_inject_ctx.user_data = user_data;
    g_inject_ctx.has_pending = false;
}

// Handle inject token - parse query from surrounding tokens
static inline const char* zeta_scratch_resolve_inject(
    zeta_scratch_buffer_t* buf,
    const char* query
) {
    if (!buf || !g_inject_ctx.query_fn || !g_inject_ctx.graph_ctx) {
        return "[inject error: no graph context]";
    }
    
    // Query the graph
    const char* result = g_inject_ctx.query_fn(
        g_inject_ctx.graph_ctx,
        query,
        g_inject_ctx.user_data
    );
    
    return result ? result : "[no result]";
}

// Default graph query implementation (uses existing surfacing)
static inline const char* zeta_default_graph_query(
    struct zeta_dual_ctx_t* ctx,
    const char* query,
    void* user_data
) {
    (void)user_data;
    if (!ctx || !query) return NULL;
    
    // This would use existing zeta_surface or zeta_stream_surface functions
    // For now, return placeholder
    static char result_buf[2048];
    snprintf(result_buf, sizeof(result_buf), "[Graph query: %s]", query);
    return result_buf;
}

// ============================================================================
// PART 3: Decode Loop Hook
// ============================================================================

// State for tracking generation with scratch buffer
typedef struct {
    zeta_scratch_buffer_t* scratch;
    zeta_generation_ctx_t* gen_ctx;
    
    // Streaming output callback
    std::function<void(const char*, size_t)> on_visible_token;
    
    // Control sequence accumulator
    char control_accum[64];
    int control_accum_len;
    bool in_control_sequence;
    
    // Stats
    int tokens_processed;
    int control_tokens_handled;
    int injections_performed;
    
    // Configuration
    bool enable_hidden_thinking;
    bool enable_revision;
    bool enable_injection;
    float revision_threshold;
} zeta_decode_hook_t;

static zeta_decode_hook_t g_decode_hook = {0};

// Initialize decode hook
static inline bool zeta_decode_hook_init(
    std::function<void(const char*, size_t)> on_visible
) {
    g_decode_hook.gen_ctx = zeta_generation_create(false);
    if (!g_decode_hook.gen_ctx) return false;
    
    g_decode_hook.scratch = g_decode_hook.gen_ctx->scratch;
    g_decode_hook.on_visible_token = on_visible;
    g_decode_hook.control_accum_len = 0;
    g_decode_hook.in_control_sequence = false;
    g_decode_hook.tokens_processed = 0;
    g_decode_hook.control_tokens_handled = 0;
    g_decode_hook.injections_performed = 0;
    
    g_decode_hook.enable_hidden_thinking = true;
    g_decode_hook.enable_revision = true;
    g_decode_hook.enable_injection = true;
    g_decode_hook.revision_threshold = 0.3f;
    
    return true;
}

// Clean up decode hook
static inline void zeta_decode_hook_free(void) {
    if (g_decode_hook.gen_ctx) {
        zeta_generation_destroy(g_decode_hook.gen_ctx);
        g_decode_hook.gen_ctx = NULL;
        g_decode_hook.scratch = NULL;
    }
}

// Reset for new generation
static inline void zeta_decode_hook_reset(void) {
    if (g_decode_hook.scratch) {
        zeta_scratch_reset(g_decode_hook.scratch);
    }
    g_decode_hook.control_accum_len = 0;
    g_decode_hook.in_control_sequence = false;
    g_decode_hook.tokens_processed = 0;
    g_decode_hook.control_tokens_handled = 0;
    g_decode_hook.injections_performed = 0;
}

// Process a single token through the scratch buffer system
// Returns: true if token should be output to user, false if consumed internally
static inline bool zeta_decode_hook_process(
    int32_t token_id,
    const char* token_text,
    size_t token_len,
    float confidence
) {
    if (!g_decode_hook.scratch) return true;  // No scratch buffer, pass through
    
    g_decode_hook.tokens_processed++;
    
    // Check for control token by ID
    if (g_scratch_tok_ids.registered && zeta_scratch_is_control_token(token_id)) {
        g_decode_hook.control_tokens_handled++;
        
        // Handle the control token
        if (token_id == g_scratch_tok_ids.scratch_start) {
            zeta_scratch_enter_hidden(g_decode_hook.scratch);
            return false;  // Don't output
        }
        
        if (token_id == g_scratch_tok_ids.scratch_end) {
            zeta_scratch_exit_hidden(g_decode_hook.scratch);
            return false;
        }
        
        if (token_id == g_scratch_tok_ids.checkpoint) {
            zeta_scratch_checkpoint(g_decode_hook.scratch, NULL);
            return false;
        }
        
        if (token_id == g_scratch_tok_ids.revise) {
            zeta_scratch_revert_last(g_decode_hook.scratch);
            return false;
        }
        
        if (token_id == g_scratch_tok_ids.inject && g_decode_hook.enable_injection) {
            // Would need to capture query from preceding tokens
            // For now, just mark injection point
            g_decode_hook.injections_performed++;
            return false;
        }
        
        // Other control tokens...
        return false;
    }
    
    // Check for control sequence in text (fallback for unregistered tokens)
    int control_type = -1;
    if (zeta_scratch_detect_control_sequence(token_text, &control_type)) {
        g_decode_hook.control_tokens_handled++;
        
        switch (control_type) {
            case 0: zeta_scratch_enter_hidden(g_decode_hook.scratch); return false;
            case 1: zeta_scratch_exit_hidden(g_decode_hook.scratch); return false;
            case 2: zeta_scratch_checkpoint(g_decode_hook.scratch, NULL); return false;
            case 3: zeta_scratch_revert_last(g_decode_hook.scratch); return false;
            case 7: g_decode_hook.injections_performed++; return false;
            default: break;
        }
        return false;
    }
    
    // Regular token - append to buffer
    zeta_scratch_append_token(g_decode_hook.scratch, token_id, token_text, token_len);
    
    // Check revision threshold
    if (g_decode_hook.enable_revision && confidence < g_decode_hook.revision_threshold) {
        zeta_revision_level_t level = zeta_revision_evaluate(
            g_decode_hook.scratch, confidence, &g_decode_hook.gen_ctx->revision_cfg
        );
        
        if (level != REVISION_NONE) {
            zeta_revision_execute(g_decode_hook.scratch, level, &g_decode_hook.gen_ctx->revision_cfg);
            return false;  // Revising, don't output this token
        }
    }
    
    // Output if visible mode
    if (g_decode_hook.scratch->mode == SCRATCH_MODE_VISIBLE) {
        if (g_decode_hook.on_visible_token) {
            g_decode_hook.on_visible_token(token_text, token_len);
        }
        return true;
    }
    
    return false;  // Hidden mode, don't output
}

// Get final output after generation
static inline std::string zeta_decode_hook_finalize(void) {
    if (!g_decode_hook.scratch) return "";
    
    char output[65536];
    size_t len = zeta_scratch_flush(g_decode_hook.scratch, output, sizeof(output));
    
    return std::string(output, len);
}

// ============================================================================
// PART 4: HTTP Endpoints
// ============================================================================

#ifdef CPPHTTPLIB_HTTPLIB_H

// Register scratch buffer endpoints with httplib server
static inline void zeta_scratch_register_endpoints(httplib::Server& svr) {
    
    // GET /scratch/stats - Get scratch buffer statistics
    svr.Get("/scratch/stats", [](const httplib::Request&, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }
        
        char json[4096];
        size_t len = zeta_scratch_to_json(g_decode_hook.scratch, json, sizeof(json));
        
        if (len > 0) {
            res.set_content(json, "application/json");
        } else {
            res.set_content("{\"error\": \"serialization failed\"}", "application/json");
        }
    });
    
    // GET /scratch/content - Get current buffer content
    svr.Get("/scratch/content", [](const httplib::Request& req, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }
        
        bool visible_only = req.has_param("visible") && req.get_param_value("visible") == "true";
        
        char content[65536];
        size_t len;
        
        if (visible_only) {
            len = zeta_scratch_get_output(g_decode_hook.scratch, content, sizeof(content));
        } else {
            len = g_decode_hook.scratch->length;
            if (len >= sizeof(content)) len = sizeof(content) - 1;
            memcpy(content, g_decode_hook.scratch->data, len);
            content[len] = '\0';
        }
        
        // Escape for JSON
        std::string escaped;
        for (size_t i = 0; i < len; i++) {
            char c = content[i];
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else if (c >= 32 && c < 127) escaped += c;
            else {
                char hex[8];
                snprintf(hex, sizeof(hex), "\\u%04x", (unsigned char)c);
                escaped += hex;
            }
        }
        
        char json[131072];
        snprintf(json, sizeof(json),
            "{\"length\": %zu, \"visible_only\": %s, \"content\": \"%s\"}",
            len, visible_only ? "true" : "false", escaped.c_str());
        
        res.set_content(json, "application/json");
    });
    
    // GET /scratch/checkpoints - List all checkpoints
    svr.Get("/scratch/checkpoints", [](const httplib::Request&, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"checkpoints\": []}", "application/json");
            return;
        }
        
        std::string json = "{\"checkpoints\": [";
        for (int i = 0; i < g_decode_hook.scratch->num_checkpoints; i++) {
            if (i > 0) json += ",";
            auto& cp = g_decode_hook.scratch->checkpoints[i];
            char buf[256];
            snprintf(buf, sizeof(buf),
                "{\"index\": %d, \"label\": \"%s\", \"position\": %zu, "
                "\"tokens\": %zu, \"valid\": %s}",
                i, cp.label, cp.buffer_pos, cp.token_count,
                cp.is_valid ? "true" : "false");
            json += buf;
        }
        json += "], \"current\": " + std::to_string(g_decode_hook.scratch->current_checkpoint) + "}";
        
        res.set_content(json, "application/json");
    });
    
    // GET /scratch/sections - List all sections
    svr.Get("/scratch/sections", [](const httplib::Request&, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"sections\": []}", "application/json");
            return;
        }
        
        std::string json = "{\"sections\": [";
        for (int i = 0; i < g_decode_hook.scratch->num_sections; i++) {
            if (i > 0) json += ",";
            auto& sec = g_decode_hook.scratch->sections[i];
            char buf[256];
            snprintf(buf, sizeof(buf),
                "{\"index\": %d, \"name\": \"%s\", \"start\": %zu, \"end\": %zu, "
                "\"visible\": %s, \"complete\": %s}",
                i, sec.name, sec.start_pos, sec.end_pos,
                sec.is_visible ? "true" : "false",
                sec.is_complete ? "true" : "false");
            json += buf;
        }
        json += "], \"current\": " + std::to_string(g_decode_hook.scratch->current_section) + "}";
        
        res.set_content(json, "application/json");
    });
    
    // POST /scratch/checkpoint - Create a manual checkpoint
    svr.Post("/scratch/checkpoint", [](const httplib::Request& req, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }
        
        std::string label = req.has_param("label") ? req.get_param_value("label") : "";
        int idx = zeta_scratch_checkpoint(g_decode_hook.scratch, label.empty() ? NULL : label.c_str());
        
        char json[128];
        snprintf(json, sizeof(json), "{\"checkpoint_index\": %d}", idx);
        res.set_content(json, "application/json");
    });
    
    // POST /scratch/revert - Revert to checkpoint
    svr.Post("/scratch/revert", [](const httplib::Request& req, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }
        
        int idx = -1;
        if (req.has_param("index")) {
            idx = std::stoi(req.get_param_value("index"));
        }
        
        bool success;
        if (idx >= 0) {
            success = zeta_scratch_revert(g_decode_hook.scratch, idx);
        } else {
            success = zeta_scratch_revert_last(g_decode_hook.scratch);
        }
        
        char json[128];
        snprintf(json, sizeof(json), "{\"success\": %s, \"index\": %d}",
            success ? "true" : "false", idx);
        res.set_content(json, "application/json");
    });
    
    // POST /scratch/clear - Clear the scratch buffer
    svr.Post("/scratch/clear", [](const httplib::Request&, httplib::Response& res) {
        if (!g_decode_hook.scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }
        
        zeta_scratch_reset(g_decode_hook.scratch);
        res.set_content("{\"success\": true}", "application/json");
    });
    
    // GET /scratch/hook/stats - Decode hook statistics
    svr.Get("/scratch/hook/stats", [](const httplib::Request&, httplib::Response& res) {
        char json[512];
        snprintf(json, sizeof(json),
            "{\"tokens_processed\": %d, \"control_tokens\": %d, \"injections\": %d, "
            "\"hidden_thinking\": %s, \"revision_enabled\": %s, \"injection_enabled\": %s, "
            "\"revision_threshold\": %.2f}",
            g_decode_hook.tokens_processed,
            g_decode_hook.control_tokens_handled,
            g_decode_hook.injections_performed,
            g_decode_hook.enable_hidden_thinking ? "true" : "false",
            g_decode_hook.enable_revision ? "true" : "false",
            g_decode_hook.enable_injection ? "true" : "false",
            g_decode_hook.revision_threshold);
        res.set_content(json, "application/json");
    });
    
    // POST /scratch/config - Update scratch buffer configuration
    svr.Post("/scratch/config", [](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("hidden_thinking")) {
            g_decode_hook.enable_hidden_thinking = (req.get_param_value("hidden_thinking") == "true");
        }
        if (req.has_param("revision")) {
            g_decode_hook.enable_revision = (req.get_param_value("revision") == "true");
        }
        if (req.has_param("injection")) {
            g_decode_hook.enable_injection = (req.get_param_value("injection") == "true");
        }
        if (req.has_param("revision_threshold")) {
            g_decode_hook.revision_threshold = std::stof(req.get_param_value("revision_threshold"));
        }
        
        res.set_content("{\"success\": true}", "application/json");
    });
    
    fprintf(stderr, "[SCRATCH] HTTP endpoints registered:\n");
    fprintf(stderr, "  GET  /scratch/stats       - Buffer statistics (JSON)\n");
    fprintf(stderr, "  GET  /scratch/content     - Buffer content (?visible=true)\n");
    fprintf(stderr, "  GET  /scratch/checkpoints - List checkpoints\n");
    fprintf(stderr, "  GET  /scratch/sections    - List sections\n");
    fprintf(stderr, "  POST /scratch/checkpoint  - Create checkpoint (?label=name)\n");
    fprintf(stderr, "  POST /scratch/revert      - Revert to checkpoint (?index=N)\n");
    fprintf(stderr, "  POST /scratch/clear       - Clear buffer\n");
    fprintf(stderr, "  GET  /scratch/hook/stats  - Decode hook statistics\n");
    fprintf(stderr, "  POST /scratch/config      - Update configuration\n");
}

#endif // CPPHTTPLIB_HTTPLIB_H

// ============================================================================
// PART 5: Server Integration Macros
// ============================================================================

// Call at server init (after model load)
#define ZETA_SCRATCH_INIT(vocab) do { \
    zeta_scratch_register_tokens(vocab); \
    zeta_decode_hook_init([](const char* t, size_t l) { /* default no-op */ }); \
    fprintf(stderr, "[SCRATCH] Scratch buffer initialized\n"); \
} while(0)

// Call before each generation
#define ZETA_SCRATCH_START_GENERATION() do { \
    zeta_decode_hook_reset(); \
} while(0)

// Call for each token in decode loop
// Returns true if token should be output to user
#define ZETA_SCRATCH_PROCESS_TOKEN(tok_id, tok_text, tok_len, confidence) \
    zeta_decode_hook_process(tok_id, tok_text, tok_len, confidence)

// Call after generation complete
#define ZETA_SCRATCH_END_GENERATION() \
    zeta_decode_hook_finalize()

// Register HTTP endpoints (call after svr is created)
#define ZETA_SCRATCH_REGISTER_HTTP(svr) \
    zeta_scratch_register_endpoints(svr)

// Clean up (call at server shutdown)
#define ZETA_SCRATCH_CLEANUP() do { \
    zeta_decode_hook_free(); \
    fprintf(stderr, "[SCRATCH] Scratch buffer cleaned up\n"); \
} while(0)

// ============================================================================
// PART 6: Generation Loop Example
// ============================================================================

/*
Usage in existing server decode loop:

    // Before generation
    ZETA_SCRATCH_START_GENERATION();

    // In token loop
    for (int i = 0; i < max_tokens; i++) {
        // ... sample token ...
        llama_token tok = common_sampler_sample(sampler, ctx, -1);
        
        char piece[64];
        llama_token_to_piece(vocab, tok, piece, sizeof(piece), 0, true);
        
        // Get confidence from logits
        float* logits = llama_get_logits_ith(ctx, -1);
        float confidence = compute_confidence(logits, tok, n_vocab);
        
        // Process through scratch buffer
        bool should_output = ZETA_SCRATCH_PROCESS_TOKEN(
            tok, piece, strlen(piece), confidence
        );
        
        if (should_output) {
            // Send to user (streaming)
            stream_to_user(piece);
        }
        
        // ... decode next ...
    }

    // After generation
    std::string final_output = ZETA_SCRATCH_END_GENERATION();
*/

#endif // ZETA_SCRATCH_INTEGRATION_H
