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
#include "zeta-format-discovery.h"
#include "zeta-dual-process.h"
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

// Forward declaration - use void* to avoid C/C++ struct compatibility issues
typedef const char* (*zeta_graph_query_fn)(void* ctx, const char* query, void* user_data);

typedef struct {
    void* graph_ctx;
    zeta_graph_query_fn query_fn;
    void* user_data;
    
    // Pending injection
    char pending_query[256];
    bool has_pending;
} zeta_inject_ctx_t;

static zeta_inject_ctx_t g_inject_ctx = {0};

// Set up injection context
static inline void zeta_scratch_set_inject_ctx(
    void* graph_ctx,
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
    void* ctx,
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
struct zeta_decode_hook_t {
    zeta_scratch_buffer_t* scratch;
    zeta_generation_ctx_t* gen_ctx;

    // Streaming output callback (use function pointer for C compatibility)
    void (*on_visible_token)(const char*, size_t, void*);
    void* callback_user_data;

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

    // Initialization state
    bool initialized;

    // Thread safety: simple spinlock for critical sections
    volatile int lock;
};

// Use a function to get/initialize the global hook safely
static inline zeta_decode_hook_t* zeta_get_decode_hook(void) {
    static zeta_decode_hook_t hook = {
        .scratch = NULL,
        .gen_ctx = NULL,
        .on_visible_token = NULL,
        .callback_user_data = NULL,
        .control_accum = {0},
        .control_accum_len = 0,
        .in_control_sequence = false,
        .tokens_processed = 0,
        .control_tokens_handled = 0,
        .injections_performed = 0,
        .enable_hidden_thinking = true,
        .enable_revision = false,  // Disabled by default - causes revision loops
        .enable_injection = true,
        .revision_threshold = 0.3f,
        .initialized = false,
        .lock = 0
    };
    return &hook;
}

// Simple spinlock for thread safety
static inline void zeta_hook_lock(zeta_decode_hook_t* hook) {
    while (__sync_lock_test_and_set(&hook->lock, 1)) {
        // Spin
    }
}

static inline void zeta_hook_unlock(zeta_decode_hook_t* hook) {
    __sync_lock_release(&hook->lock);
}

// Initialize decode hook (C-compatible version)
static inline bool zeta_decode_hook_init_c(
    void (*on_visible)(const char*, size_t, void*),
    void* user_data
) {
    zeta_decode_hook_t* hook = zeta_get_decode_hook();
    if (!hook) return false;

    zeta_hook_lock(hook);

    // Clean up any existing context
    if (hook->gen_ctx) {
        zeta_generation_destroy(hook->gen_ctx);
        hook->gen_ctx = NULL;
        hook->scratch = NULL;
    }

    hook->gen_ctx = zeta_generation_create(false);
    if (!hook->gen_ctx) {
        zeta_hook_unlock(hook);
        return false;
    }

    hook->scratch = hook->gen_ctx->scratch;
    hook->on_visible_token = on_visible;
    hook->callback_user_data = user_data;
    hook->control_accum_len = 0;
    hook->in_control_sequence = false;
    hook->tokens_processed = 0;
    hook->control_tokens_handled = 0;
    hook->injections_performed = 0;

    hook->enable_hidden_thinking = true;
    hook->enable_revision = false;  // Disabled - causes loops
    hook->enable_injection = true;
    hook->revision_threshold = 0.3f;
    hook->initialized = true;

    zeta_hook_unlock(hook);

    fprintf(stderr, "[SCRATCH] Decode hook initialized (scratch=%p)\n", (void*)hook->scratch);
    return true;
}

// C++ wrapper for backward compatibility
static inline bool zeta_decode_hook_init(void) {
    return zeta_decode_hook_init_c(NULL, NULL);
}

// Clean up decode hook
static inline void zeta_decode_hook_free(void) {
    zeta_decode_hook_t* hook = zeta_get_decode_hook();
    if (!hook) return;

    zeta_hook_lock(hook);

    if (hook->gen_ctx) {
        zeta_generation_destroy(hook->gen_ctx);
        hook->gen_ctx = NULL;
        hook->scratch = NULL;
    }
    hook->initialized = false;

    zeta_hook_unlock(hook);
}

// Reset for new generation
static inline void zeta_decode_hook_reset(void) {
    zeta_decode_hook_t* hook = zeta_get_decode_hook();
    if (!hook || !hook->initialized) return;

    zeta_hook_lock(hook);

    if (hook->scratch) {
        zeta_scratch_reset(hook->scratch);
    }
    hook->control_accum_len = 0;
    hook->in_control_sequence = false;
    hook->tokens_processed = 0;
    hook->control_tokens_handled = 0;
    hook->injections_performed = 0;

    zeta_hook_unlock(hook);
}

// Process a single token through the scratch buffer system
// Returns: true if token should be output to user, false if consumed internally
static inline bool zeta_decode_hook_process(
    int32_t token_id,
    const char* token_text,
    size_t token_len,
    float confidence
) {
    zeta_decode_hook_t* hook = zeta_get_decode_hook();

    // Safety check - if not initialized or scratch buffer null, just pass through
    if (!hook || !hook->initialized || !hook->scratch || !hook->gen_ctx) {
        return true;
    }

    // Safety check for text pointer
    if (!token_text || token_len == 0) return true;

    // Don't lock for the whole function - only for state modifications
    hook->tokens_processed++;

    // Check for control token by ID
    if (g_scratch_tok_ids.registered && zeta_scratch_is_control_token(token_id)) {
        hook->control_tokens_handled++;

        // Handle the control token
        if (token_id == g_scratch_tok_ids.scratch_start) {
            zeta_scratch_enter_hidden(hook->scratch);
            return false;  // Don't output
        }

        if (token_id == g_scratch_tok_ids.scratch_end) {
            zeta_scratch_exit_hidden(hook->scratch);
            return false;
        }

        if (token_id == g_scratch_tok_ids.checkpoint) {
            zeta_scratch_checkpoint(hook->scratch, NULL);
            return false;
        }

        if (token_id == g_scratch_tok_ids.revise) {
            zeta_scratch_revert_last(hook->scratch);
            return false;
        }

        if (token_id == g_scratch_tok_ids.inject && hook->enable_injection) {
            // Would need to capture query from preceding tokens
            // For now, just mark injection point
            hook->injections_performed++;
            return false;
        }

        // Other control tokens...
        return false;
    }

    // Check for control sequence in text (fallback for unregistered tokens)
    int control_type = -1;
    if (zeta_scratch_detect_control_sequence(token_text, &control_type)) {
        hook->control_tokens_handled++;

        switch (control_type) {
            case 0: zeta_scratch_enter_hidden(hook->scratch); return false;
            case 1: zeta_scratch_exit_hidden(hook->scratch); return false;
            case 2: zeta_scratch_checkpoint(hook->scratch, NULL); return false;
            case 3: zeta_scratch_revert_last(hook->scratch); return false;
            case 7: hook->injections_performed++; return false;
            default: break;
        }
        return false;
    }

    // Regular token - append to buffer
    if (token_text && token_len > 0) {
        zeta_scratch_append_token(hook->scratch, token_id, token_text, token_len);
    }

    // Check revision threshold (only if enabled - disabled by default)
    if (hook->enable_revision && hook->gen_ctx &&
        confidence < hook->revision_threshold) {
        zeta_revision_level_t level = zeta_revision_evaluate(
            hook->scratch, confidence, &hook->gen_ctx->revision_cfg
        );

        if (level != REVISION_NONE) {
            zeta_revision_execute(hook->scratch, level, &hook->gen_ctx->revision_cfg);
            return false;  // Revising, don't output this token
        }
    }

    // Output if visible mode
    if (hook->scratch->mode == SCRATCH_MODE_VISIBLE) {
        if (hook->on_visible_token) {
            hook->on_visible_token(token_text, token_len, hook->callback_user_data);
        }
        return true;
    }

    return false;  // Hidden mode, don't output
}

// Get final output after generation
static inline std::string zeta_decode_hook_finalize(void) {
    zeta_decode_hook_t* hook = zeta_get_decode_hook();
    if (!hook || !hook->initialized || !hook->scratch) return "";

    char output[65536];
    size_t len = zeta_scratch_flush(hook->scratch, output, sizeof(output));

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
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }

        char json[4096];
        size_t len = zeta_scratch_to_json(hook->scratch, json, sizeof(json));

        if (len > 0) {
            res.set_content(json, "application/json");
        } else {
            res.set_content("{\"error\": \"serialization failed\"}", "application/json");
        }
    });

    // GET /scratch/content - Get current buffer content
    svr.Get("/scratch/content", [](const httplib::Request& req, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }

        bool visible_only = req.has_param("visible") && req.get_param_value("visible") == "true";

        char content[65536];
        size_t len;

        if (visible_only) {
            len = zeta_scratch_get_output(hook->scratch, content, sizeof(content));
        } else {
            len = hook->scratch->length;
            if (len >= sizeof(content)) len = sizeof(content) - 1;
            memcpy(content, hook->scratch->data, len);
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
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"checkpoints\": []}", "application/json");
            return;
        }

        std::string json = "{\"checkpoints\": [";
        for (int i = 0; i < hook->scratch->num_checkpoints; i++) {
            if (i > 0) json += ",";
            auto& cp = hook->scratch->checkpoints[i];
            char buf[256];
            snprintf(buf, sizeof(buf),
                "{\"index\": %d, \"label\": \"%s\", \"position\": %zu, "
                "\"tokens\": %zu, \"valid\": %s}",
                i, cp.label, cp.buffer_pos, cp.token_count,
                cp.is_valid ? "true" : "false");
            json += buf;
        }
        json += "], \"current\": " + std::to_string(hook->scratch->current_checkpoint) + "}";

        res.set_content(json, "application/json");
    });

    // GET /scratch/sections - List all sections
    svr.Get("/scratch/sections", [](const httplib::Request&, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"sections\": []}", "application/json");
            return;
        }

        std::string json = "{\"sections\": [";
        for (int i = 0; i < hook->scratch->num_sections; i++) {
            if (i > 0) json += ",";
            auto& sec = hook->scratch->sections[i];
            char buf[256];
            snprintf(buf, sizeof(buf),
                "{\"index\": %d, \"name\": \"%s\", \"start\": %zu, \"end\": %zu, "
                "\"visible\": %s, \"complete\": %s}",
                i, sec.name, sec.start_pos, sec.end_pos,
                sec.is_visible ? "true" : "false",
                sec.is_complete ? "true" : "false");
            json += buf;
        }
        json += "], \"current\": " + std::to_string(hook->scratch->current_section) + "}";

        res.set_content(json, "application/json");
    });

    // POST /scratch/checkpoint - Create a manual checkpoint
    svr.Post("/scratch/checkpoint", [](const httplib::Request& req, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }

        std::string label = req.has_param("label") ? req.get_param_value("label") : "";
        int idx = zeta_scratch_checkpoint(hook->scratch, label.empty() ? NULL : label.c_str());

        char json[128];
        snprintf(json, sizeof(json), "{\"checkpoint_index\": %d}", idx);
        res.set_content(json, "application/json");
    });

    // POST /scratch/revert - Revert to checkpoint
    svr.Post("/scratch/revert", [](const httplib::Request& req, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }

        int idx = -1;
        if (req.has_param("index")) {
            idx = std::stoi(req.get_param_value("index"));
        }

        bool success;
        if (idx >= 0) {
            success = zeta_scratch_revert(hook->scratch, idx);
        } else {
            success = zeta_scratch_revert_last(hook->scratch);
        }

        char json[128];
        snprintf(json, sizeof(json), "{\"success\": %s, \"index\": %d}",
            success ? "true" : "false", idx);
        res.set_content(json, "application/json");
    });

    // POST /scratch/clear - Clear the scratch buffer
    svr.Post("/scratch/clear", [](const httplib::Request&, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook || !hook->initialized || !hook->scratch) {
            res.set_content("{\"error\": \"scratch buffer not initialized\"}", "application/json");
            return;
        }

        zeta_scratch_reset(hook->scratch);
        res.set_content("{\"success\": true}", "application/json");
    });

    // GET /scratch/hook/stats - Decode hook statistics
    svr.Get("/scratch/hook/stats", [](const httplib::Request&, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook) {
            res.set_content("{\"error\": \"hook not available\"}", "application/json");
            return;
        }

        char json[512];
        snprintf(json, sizeof(json),
            "{\"tokens_processed\": %d, \"control_tokens\": %d, \"injections\": %d, "
            "\"hidden_thinking\": %s, \"revision_enabled\": %s, \"injection_enabled\": %s, "
            "\"revision_threshold\": %.2f, \"initialized\": %s}",
            hook->tokens_processed,
            hook->control_tokens_handled,
            hook->injections_performed,
            hook->enable_hidden_thinking ? "true" : "false",
            hook->enable_revision ? "true" : "false",
            hook->enable_injection ? "true" : "false",
            hook->revision_threshold,
            hook->initialized ? "true" : "false");
        res.set_content(json, "application/json");
    });

    // POST /scratch/config - Update scratch buffer configuration
    svr.Post("/scratch/config", [](const httplib::Request& req, httplib::Response& res) {
        zeta_decode_hook_t* hook = zeta_get_decode_hook();
        if (!hook) {
            res.set_content("{\"error\": \"hook not available\"}", "application/json");
            return;
        }

        if (req.has_param("hidden_thinking")) {
            hook->enable_hidden_thinking = (req.get_param_value("hidden_thinking") == "true");
        }
        if (req.has_param("revision")) {
            hook->enable_revision = (req.get_param_value("revision") == "true");
        }
        if (req.has_param("injection")) {
            hook->enable_injection = (req.get_param_value("injection") == "true");
        }
        if (req.has_param("revision_threshold")) {
            hook->revision_threshold = std::stof(req.get_param_value("revision_threshold"));
        }

        res.set_content("{\"success\": true}", "application/json");
    });

    // =========================================================================
    // OUTPUT BUFFER ENDPOINTS (Dual-Buffer Architecture)
    // =========================================================================

    // GET /output/stats - Get output buffer statistics
    svr.Get("/output/stats", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        char json[4096];
        size_t len = zeta_output_to_json(g_output_buffer, json, sizeof(json));

        if (len > 0) {
            res.set_content(json, "application/json");
        } else {
            res.set_content("{\"error\": \"serialization failed\"}", "application/json");
        }
    });

    // GET /output/content - Get output buffer content
    svr.Get("/output/content", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        size_t len = 0;
        const char* content = zeta_output_get_content(g_output_buffer, &len);

        // Escape for JSON
        std::string escaped;
        for (size_t i = 0; i < len && content; i++) {
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
            "{\"length\": %zu, \"is_finalized\": %s, \"format_locked\": %s, \"content\": \"%s\"}",
            len,
            g_output_buffer->is_finalized ? "true" : "false",
            g_output_buffer->format_locked ? "true" : "false",
            escaped.c_str());

        res.set_content(json, "application/json");
    });

    // GET /output/format - Get current format specification
    svr.Get("/output/format", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        // Escape format spec for JSON
        std::string escaped;
        for (size_t i = 0; i < g_output_buffer->format_spec_len; i++) {
            char c = g_output_buffer->format_spec[i];
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else if (c >= 32 && c < 127) escaped += c;
        }

        char json[8192];
        snprintf(json, sizeof(json),
            "{\"format_spec\": \"%s\", \"format_locked\": %s, \"length\": %zu}",
            escaped.c_str(),
            g_output_buffer->format_locked ? "true" : "false",
            g_output_buffer->format_spec_len);

        res.set_content(json, "application/json");
    });

    // POST /output/format - Set format specification (for 14B self-configuration)
    svr.Post("/output/format", [](const httplib::Request& req, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        if (g_output_buffer->format_locked) {
            res.set_content("{\"error\": \"format already locked\"}", "application/json");
            return;
        }

        // Parse format_spec from JSON body
        std::string format_spec;
        size_t start = req.body.find("\"format_spec\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start);
                if (start != std::string::npos) {
                    start++;
                    size_t end = start;
                    while (end < req.body.size()) {
                        if (req.body[end] == '\"' && req.body[end-1] != '\\') break;
                        end++;
                    }
                    format_spec = req.body.substr(start, end - start);
                }
            }
        }

        bool success = zeta_output_set_format(g_output_buffer, format_spec.c_str(), format_spec.size());

        char json[128];
        snprintf(json, sizeof(json), "{\"success\": %s}", success ? "true" : "false");
        res.set_content(json, "application/json");
    });

    // POST /output/start - Begin writing to output buffer
    svr.Post("/output/start", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        zeta_output_start(g_output_buffer);
        res.set_content("{\"success\": true}", "application/json");
    });

    // POST /output/append - Append to output buffer
    svr.Post("/output/append", [](const httplib::Request& req, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        // Parse text from JSON body
        std::string text;
        size_t start = req.body.find("\"text\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start);
                if (start != std::string::npos) {
                    start++;
                    size_t end = start;
                    while (end < req.body.size()) {
                        if (req.body[end] == '\"' && req.body[end-1] != '\\') break;
                        end++;
                    }
                    text = req.body.substr(start, end - start);
                }
            }
        }

        bool success = zeta_output_append(g_output_buffer, text.c_str(), text.size());

        char json[128];
        snprintf(json, sizeof(json), "{\"success\": %s, \"new_length\": %zu}",
            success ? "true" : "false", g_output_buffer->length);
        res.set_content(json, "application/json");
    });

    // POST /output/finalize - Mark output as complete
    svr.Post("/output/finalize", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        zeta_output_finalize(g_output_buffer);
        res.set_content("{\"success\": true}", "application/json");
    });

    // POST /output/clear - Clear the output buffer
    svr.Post("/output/clear", [](const httplib::Request&, httplib::Response& res) {
        if (!g_output_buffer) {
            res.set_content("{\"error\": \"output buffer not initialized\"}", "application/json");
            return;
        }

        zeta_output_reset(g_output_buffer);
        res.set_content("{\"success\": true}", "application/json");
    });

    // =========================================================================
    // FORMAT DISCOVERY ENDPOINTS (Benchmark-Agnostic)
    // =========================================================================

    // GET /format/current - Get current format specification
    svr.Get("/format/current", [](const httplib::Request&, httplib::Response& res) {
        char json[4096];
        size_t len = zeta_format_to_json(json, sizeof(json));
        if (len > 0) {
            res.set_content(json, "application/json");
        } else {
            res.set_content("{\"error\": \"serialization failed\"}", "application/json");
        }
    });

    // GET /format/template - Get format template for prompt injection
    svr.Get("/format/template", [](const httplib::Request&, httplib::Response& res) {
        const char* tmpl = zeta_format_get_template();
        const char* example = zeta_format_get_example();

        // Escape for JSON
        std::string tmpl_escaped, example_escaped;
        for (const char* p = tmpl; p && *p; p++) {
            if (*p == '"') tmpl_escaped += "\\\"";
            else if (*p == '\\') tmpl_escaped += "\\\\";
            else if (*p == '\n') tmpl_escaped += "\\n";
            else if (*p == '\r') tmpl_escaped += "\\r";
            else if (*p == '\t') tmpl_escaped += "\\t";
            else tmpl_escaped += *p;
        }
        for (const char* p = example; p && *p; p++) {
            if (*p == '"') example_escaped += "\\\"";
            else if (*p == '\\') example_escaped += "\\\\";
            else if (*p == '\n') example_escaped += "\\n";
            else if (*p == '\r') example_escaped += "\\r";
            else if (*p == '\t') example_escaped += "\\t";
            else example_escaped += *p;
        }

        char json[8192];
        snprintf(json, sizeof(json),
            "{\"template\": \"%s\", \"example\": \"%s\"}",
            tmpl_escaped.c_str(), example_escaped.c_str());
        res.set_content(json, "application/json");
    });

    // POST /format/detect - Auto-detect format from prompt
    svr.Post("/format/detect", [](const httplib::Request& req, httplib::Response& res) {
        // Parse prompt from JSON body
        std::string prompt;
        size_t start = req.body.find("\"prompt\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start);
                if (start != std::string::npos) {
                    start++;
                    size_t end = start;
                    while (end < req.body.size()) {
                        if (req.body[end] == '\"' && req.body[end-1] != '\\') break;
                        end++;
                    }
                    prompt = req.body.substr(start, end - start);
                }
            }
        }

        zeta_format_type_t detected = zeta_format_detect(prompt.c_str());
        zeta_format_set_type(detected);

        char json[512];
        snprintf(json, sizeof(json),
            "{\"detected_type\": %d, \"name\": \"%s\", \"success\": true}",
            detected, g_format_ctx.current_format.name);
        res.set_content(json, "application/json");
    });

    // POST /format/set - Set format type directly
    svr.Post("/format/set", [](const httplib::Request& req, httplib::Response& res) {
        // Parse type from JSON body
        int type = 0;
        size_t start = req.body.find("\"type\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                type = std::stoi(req.body.substr(start + 1));
            }
        }

        bool success = zeta_format_set_type((zeta_format_type_t)type);

        char json[256];
        snprintf(json, sizeof(json),
            "{\"success\": %s, \"type\": %d, \"name\": \"%s\"}",
            success ? "true" : "false", type, g_format_ctx.current_format.name);
        res.set_content(json, "application/json");
    });

    // POST /format/custom - Set custom format (14B self-configuration)
    svr.Post("/format/custom", [](const httplib::Request& req, httplib::Response& res) {
        // Parse fields from JSON
        std::string name, template_spec, start_marker, end_marker;

        auto extract_field = [&req](const char* field) -> std::string {
            std::string result;
            size_t start = req.body.find(std::string("\"") + field + "\"");
            if (start != std::string::npos) {
                start = req.body.find(":", start);
                if (start != std::string::npos) {
                    start = req.body.find("\"", start);
                    if (start != std::string::npos) {
                        start++;
                        size_t end = start;
                        while (end < req.body.size()) {
                            if (req.body[end] == '\"' && req.body[end-1] != '\\') break;
                            end++;
                        }
                        result = req.body.substr(start, end - start);
                    }
                }
            }
            return result;
        };

        name = extract_field("name");
        template_spec = extract_field("template");
        start_marker = extract_field("start_marker");
        end_marker = extract_field("end_marker");

        bool success = zeta_format_set_custom(
            name.empty() ? NULL : name.c_str(),
            template_spec.empty() ? NULL : template_spec.c_str(),
            start_marker.empty() ? NULL : start_marker.c_str(),
            end_marker.empty() ? NULL : end_marker.c_str()
        );

        char json[256];
        snprintf(json, sizeof(json), "{\"success\": %s}", success ? "true" : "false");
        res.set_content(json, "application/json");
    });

    // POST /format/lock - Lock format (prevents further changes)
    svr.Post("/format/lock", [](const httplib::Request&, httplib::Response& res) {
        zeta_format_lock();
        res.set_content("{\"success\": true, \"locked\": true}", "application/json");
    });

    // POST /format/reset - Reset format for new task
    svr.Post("/format/reset", [](const httplib::Request&, httplib::Response& res) {
        zeta_format_reset();
        res.set_content("{\"success\": true}", "application/json");
    });

    // POST /format/validate - Validate output against current format
    svr.Post("/format/validate", [](const httplib::Request& req, httplib::Response& res) {
        // Parse output from JSON body
        std::string output;
        size_t start = req.body.find("\"output\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start);
                if (start != std::string::npos) {
                    start++;
                    size_t end = start;
                    while (end < req.body.size()) {
                        if (req.body[end] == '\"' && req.body[end-1] != '\\') break;
                        end++;
                    }
                    output = req.body.substr(start, end - start);
                }
            }
        }

        bool valid = zeta_format_validate(output.c_str());

        char json[256];
        snprintf(json, sizeof(json),
            "{\"valid\": %s, \"format\": \"%s\"}",
            valid ? "true" : "false", g_format_ctx.current_format.name);
        res.set_content(json, "application/json");
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
    fprintf(stderr, "[OUTPUT] HTTP endpoints registered:\n");
    fprintf(stderr, "  GET  /output/stats        - Output buffer statistics\n");
    fprintf(stderr, "  GET  /output/content      - Output buffer content\n");
    fprintf(stderr, "  GET  /output/format       - Current format specification\n");
    fprintf(stderr, "  POST /output/format       - Set format spec (14B self-config)\n");
    fprintf(stderr, "  POST /output/start        - Begin writing to output\n");
    fprintf(stderr, "  POST /output/append       - Append text to output\n");
    fprintf(stderr, "  POST /output/finalize     - Mark output complete\n");
    fprintf(stderr, "  POST /output/clear        - Clear output buffer\n");
    fprintf(stderr, "[FORMAT] HTTP endpoints registered:\n");
    fprintf(stderr, "  GET  /format/current      - Current format spec (JSON)\n");
    fprintf(stderr, "  GET  /format/template     - Get template for prompt\n");
    fprintf(stderr, "  POST /format/detect       - Auto-detect from prompt\n");
    fprintf(stderr, "  POST /format/set          - Set format type\n");
    fprintf(stderr, "  POST /format/custom       - Set custom format (14B)\n");
    fprintf(stderr, "  POST /format/lock         - Lock format\n");
    fprintf(stderr, "  POST /format/reset        - Reset for new task\n");
    fprintf(stderr, "  POST /format/validate     - Validate output\n");
}

#endif // CPPHTTPLIB_HTTPLIB_H

// ============================================================================
// PART 5: Context Injection Integration (Core Flow)
// ============================================================================

// Global graph context for context injection
static zeta_dual_ctx_t* g_graph_ctx = NULL;

// Set the graph context (call at server init after graph is created)
static inline void zeta_scratch_set_graph(zeta_dual_ctx_t* ctx) {
    g_graph_ctx = ctx;
    fprintf(stderr, "[SCRATCH] Graph context set for context injection\n");
}

// Inject graph context into prompt BEFORE generation
// Returns allocated string (caller must free) or NULL if no injection
static inline char* zeta_scratch_inject_context(const char* prompt) {
    if (!g_graph_ctx || !prompt) return NULL;
    return zeta_inject_context_to_prompt(g_graph_ctx, prompt);
}

// Extract facts from generation output AFTER generation
static inline int zeta_scratch_extract_facts(const char* output, bool is_planning) {
    if (!g_graph_ctx || !output) return 0;
    return zeta_extract_from_generation(g_graph_ctx, output, is_planning);
}

// Build context injection string (non-allocating version)
static inline size_t zeta_scratch_build_context(
    const char* prompt,
    char* out_context,
    size_t max_len
) {
    if (!g_graph_ctx || !prompt) {
        if (out_context && max_len > 0) out_context[0] = '\0';
        return 0;
    }
    return zeta_build_context_injection(g_graph_ctx, prompt, out_context, max_len);
}

// ============================================================================
// PART 6: Server Integration Macros
// ============================================================================

// Call at server init (after model load)
// Now enabled with safe initialization
#define ZETA_SCRATCH_INIT(vocab) do { \
    fprintf(stderr, "[SCRATCH] Initializing scratch buffer...\n"); \
    if (zeta_decode_hook_init()) { \
        zeta_scratch_register_tokens(vocab); \
        fprintf(stderr, "[SCRATCH] Scratch buffer ENABLED\n"); \
    } else { \
        fprintf(stderr, "[SCRATCH] Failed to initialize - will pass through\n"); \
    } \
} while(0)

// Call before each generation (reset buffers)
#define ZETA_SCRATCH_START_GENERATION() do { \
    zeta_decode_hook_reset(); \
} while(0)

// Set the graph context for context injection (call at server init)
#define ZETA_SCRATCH_SET_GRAPH(graph_ctx) \
    zeta_scratch_set_graph(graph_ctx)

// Inject context into prompt BEFORE tokenization
// Returns allocated string (caller must free) or original if no injection
#define ZETA_INJECT_CONTEXT(prompt) \
    zeta_scratch_inject_context(prompt)

// Extract facts from output AFTER generation
#define ZETA_EXTRACT_FACTS(output, is_planning) \
    zeta_scratch_extract_facts(output, is_planning)

// Build context injection (non-allocating)
#define ZETA_BUILD_CONTEXT(prompt, out_buf, max_len) \
    zeta_scratch_build_context(prompt, out_buf, max_len)

// Call for each token in decode loop
// Returns true if token should be output to user
// Safe implementation with null checks
#define ZETA_SCRATCH_PROCESS_TOKEN(tok_id, tok_text, tok_len, confidence) \
    zeta_decode_hook_process((tok_id), (tok_text), (tok_len), (confidence))

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
