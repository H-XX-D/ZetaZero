// Z.E.T.A. Server v5.0 - Parallel Dual-Process Engine
// 3B runs PARALLEL to 14B with cyclic correlation feedback

// ============================================================================
// 16GB GPU Config (14B + 7B + 1.5B Embed)
// Context size tuned for VRAM efficiency - lower = more headroom
// ============================================================================
#ifndef ZETA_CTX_SIZE
#define ZETA_CTX_SIZE 2048  // 2K context for 16GB GPU (saves ~768MB vs 8K)
#endif
#ifndef ZETA_BATCH_SIZE
#define ZETA_BATCH_SIZE 1024  // Batch size for inference
#endif

#include "httplib.h"
#include "arg.h"
#include "common.h"
#include "llama.h"
#include "sampling.h"
#include <cstdio>
#include <string>
#include <mutex>
#include <vector>
#include <signal.h>
#include <atomic>
#include "log.h"

extern "C" {
#include "zeta-memory.h"
#include "zeta-integration.h"
#include "zeta-constitution.h"
#include "zeta-dual-process.h"
#include "zeta-embed-integration.h"
#include "zeta-cyclic.h"
#include "zeta-code-mode.h"
#include "zeta-code-conflict.h"
#include "zeta-code-streaming.h"
#include "zeta-streaming.h"
#include "zeta-conflict.h"
//MOVED: zeta-tools.h
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

// 14B Conscious model
static llama_model* g_model_14b = nullptr;
static llama_context* g_ctx_14b = nullptr;

// 3B Subconscious model
static llama_model* g_model_3b = nullptr;

// ZETA contexts
static zeta_context_t* g_zeta = nullptr;
static zeta_dual_ctx_t* g_dual = nullptr;
static zeta_code_ctx_t* g_code = nullptr;  // Code mode context
static llama_model* g_model_3b_coder = nullptr;  // 3B Coder model

static const llama_vocab* g_vocab = nullptr;
static common_params g_params;
static std::mutex g_mutex;
static std::string g_embed_model_path, g_embed_model_code_path;
static std::string g_storage_dir = "/mnt/HoloGit/blocks";
static int g_n_embd = 0;

// 3B worker thread
static pthread_t g_3b_worker_tid;
static bool g_3b_worker_running = false;

// Streaming memory state - reactive context management
static zeta_stream_state_t g_stream_state;

// Streaming configuration defaults
int g_stream_token_budget = 600;
int g_stream_max_nodes    = 6;
int g_code_token_budget   = 900;
int g_code_max_nodes      = 10;

static std::atomic<bool> g_shutdown_requested{false};
static std::atomic<time_t> g_last_activity{0};
static std::thread g_idle_watchdog;

// Tier based on RECENCY (importance affects retrieval, not storage)
static void zeta_apply_temporal_decay(zeta_dual_ctx_t* ctx) {
    if (!ctx) return;
    time_t now = time(NULL);
    for (int i = 0; i < ctx->num_nodes; i++) {
        auto& n = ctx->nodes[i];
        if (!n.is_active) continue;
        float age_secs = (float)(now - n.last_accessed);
        // Tier by recency only - importance is for retrieval ranking
        if (age_secs < 300)        n.current_tier = ZETA_TIER_VRAM;   // < 5 min
        else if (age_secs < 1800)  n.current_tier = ZETA_TIER_RAM;    // < 30 min
        else                       n.current_tier = ZETA_TIER_NVME;   // > 30 min
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"



// Idle decay function
// Smart idle decay using Z.E.T.A. functions
static void idle_decay() {
    if (!g_dual) return;
    // Apply temporal decay to all nodes
    zeta_apply_temporal_decay(g_dual);
    // Restage based on decayed salience × current momentum
    // Tier restaging happens automatically during retrieval
    fprintf(stderr, "[IDLE] Applied temporal decay, restaged %d nodes\n", g_dual->num_nodes);
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"


// Watchdog thread
static void idle_watchdog_thread() {
    while (!g_shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        time_t now = time(NULL);
        time_t idle_secs = now - g_last_activity.load();
        if (idle_secs > 300) {  // 5 min idle
            idle_decay();
        }
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static httplib::Server* g_server = nullptr;

static void consolidate_memory();
static void signal_handler(int sig);

// Helper: Qwen chat template wrapper
static std::string make_qwen_prompt(const std::string& user)
{
    return std::string("<|im_start|>system\nYou are a senior software architect assistant.<|im_end|>\n<|im_start|>user\n") +
           user +
           "<|im_end|>\n<|im_start|>assistant\n";
}

// Compute momentum from 14B logits (entropy-based)
static float compute_momentum_from_logits(float* logits, int n_vocab) {
    if (!logits || n_vocab <= 0) return 0.5f;

    float max_logit = logits[0];
    for (int i = 1; i < n_vocab; i++) {
        if (logits[i] > max_logit) max_logit = logits[i];
    }

    float sum_exp = 0;
    for (int i = 0; i < n_vocab; i++) {
        sum_exp += expf(logits[i] - max_logit);
    }

    float entropy = 0;
    for (int i = 0; i < n_vocab; i++) {
        float p = expf(logits[i] - max_logit) / sum_exp;
        if (p > 1e-8f) entropy -= p * logf(p);
    }

    float momentum = 1.0f - (entropy / 10.0f);
    if (momentum < 0) momentum = 0;
    if (momentum > 1) momentum = 1;

    return momentum;
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static std::string generate(const std::string& prompt, int max_tokens) {
    std::lock_guard<std::mutex> lock(g_mutex);

    fprintf(stderr, "[GENERATE] Received prompt (len=%zu): %.60s...\n", prompt.size(), prompt.c_str());


    // === PUSH INPUT TO 3B QUEUE (non-blocking) ===
    zeta_cyclic_push(prompt.c_str(), true, 0.5f);

    // === 3B SUBCONSCIOUS: Stream relevant context on-demand ===
    zeta_stream_evict(&g_stream_state, 0.5f);  // Evict served/low-priority first

    char stream_context[2048];
    stream_context[0] = '\0';

    if (g_dual) {
        // Surface nodes one at a time until budget exhausted or no more quality nodes
        int surfaced_count = 0;
        while (surfaced_count < g_stream_max_nodes) {
            zeta_graph_node_t* node = zeta_stream_surface_one(
                g_dual, &g_stream_state, prompt.c_str(), 0.5f
            );
            if (!node) break;  // No more suitable nodes
            surfaced_count++;
        }

        if (surfaced_count > 0) {
            zeta_stream_format(g_dual, &g_stream_state, stream_context, sizeof(stream_context));
            fprintf(stderr, "[STREAM] %d nodes (%d tokens) surfaced for 14B\n",
                    g_stream_state.num_active, g_stream_state.total_tokens);
            fprintf(stderr, "[STREAM] Context: %.200s...\n", stream_context);
        }
    }

    // Check for numeric conflicts BEFORE generation
    char conflict_warning[512];
    conflict_warning[0] = '\0';
    if (g_dual) {
        int conflicts = zeta_check_numeric_conflicts(g_dual, prompt.c_str(),
                                                     conflict_warning, sizeof(conflict_warning));
        if (conflicts > 0) {
            fprintf(stderr, "[SERVER] Numeric conflicts detected: %d\n", conflicts);
        }
    }

    // Augment prompt with streamed memory AND any conflict warnings
    // Apply Qwen template
    std::string wrapped = make_qwen_prompt(prompt);
    std::string augmented_prompt = std::string(stream_context) + std::string(conflict_warning) + wrapped;

    // Tokenize
    std::vector<llama_token> tokens(4096);
    int n_tokens = llama_tokenize(g_vocab, augmented_prompt.c_str(),
                                   augmented_prompt.length(),
                                   tokens.data(), tokens.size(), true, true);
    if (n_tokens < 0) {
        return "{\"error\": \"tokenization failed\"}";
    }
    tokens.resize(n_tokens);

    // Clear KV cache
    llama_memory_t mem = llama_get_memory(g_ctx_14b);
    llama_memory_clear(mem, true);

    // Decode prompt
    llama_batch batch = llama_batch_init(4096, 0, 1);
    // Safety: truncate if prompt too long
    if (n_tokens > 3800) {
        fprintf(stderr, "[WARN] Truncating prompt from %d to 3800 tokens\n", n_tokens);
        n_tokens = 3800;
    }
    for (int i = 0; i < n_tokens; i++) {
        common_batch_add(batch, tokens[i], i, {0}, false);
    }
    batch.logits[batch.n_tokens - 1] = true;

    if (llama_decode(g_ctx_14b, batch) != 0) {
        llama_batch_free(batch);
        return "{\"error\": \"decode failed\"}";
    }

    // Generate with momentum tracking
    std::string output;
    float avg_momentum = 0.0f;
    int n_generated = 0;
    int n_vocab = llama_vocab_n_tokens(g_vocab);

    auto* sampler = common_sampler_init(g_model_14b, g_params.sampling);

    for (int i = 0; i < max_tokens; i++) {
        float* logits = llama_get_logits_ith(g_ctx_14b, -1);

        // Compute momentum from 14B logits
        float momentum = compute_momentum_from_logits(logits, n_vocab);
        avg_momentum += momentum;
        n_generated++;

        // Update dual-process momentum
        if (g_dual) {
            zeta_update_momentum(g_dual, momentum);
        }

        llama_token tok = common_sampler_sample(sampler, g_ctx_14b, -1);
        common_sampler_accept(sampler, tok, true);

        // Convert token to piece first
        char piece[64] = {0};
        llama_token_to_piece(g_vocab, tok, piece, sizeof(piece), 0, true);
        // Skip stray leading <|im_start|>
        if (output.empty() && strcmp(piece, "<|im_start|>") == 0) {
            continue;
        }
        if (strcmp(piece, "<|im_end|>") == 0) break;
        if (llama_vocab_is_eog(g_vocab, tok)) break;

        output += piece;
        // Stop on chat template tokens (prevents repetition)
        if (strstr(piece, "<|im_start") || strstr(piece, "<|im_end")) break;

        // Prepare next
        common_batch_clear(batch);
        common_batch_add(batch, tok, n_tokens + i, {0}, true);
        if (llama_decode(g_ctx_14b, batch) != 0) break;
    }

    common_sampler_free(sampler);
    llama_batch_free(batch);

    avg_momentum = (n_generated > 0) ? (avg_momentum / n_generated) : 0.5f;

    // === PUSH OUTPUT TO 3B QUEUE (cyclic feedback) ===
    zeta_cyclic_push(output.c_str(), false, avg_momentum);

    // Mark served nodes - they've been used in this turn
    for (int i = 0; i < g_stream_state.num_active; i++) {
        if (!g_stream_state.active[i].served) {
            zeta_stream_ack_served(g_dual, &g_stream_state,
                                   g_stream_state.active[i].node_id);
        }
    }

    // Apply conflict detection guardrail
    char safe_output_buf[8192];
    const char* final_output = output.c_str();
    if (g_dual) {
        final_output = zeta_apply_conflict_guardrail(
            g_dual, output.c_str(), safe_output_buf, sizeof(safe_output_buf)
        );
    }

    // Escape quotes in output for JSON
    std::string escaped_output;
    for (const char* p = final_output; *p; p++) {
        if (*p == '"') escaped_output += "\\\"";

        else if (*p == '\\') escaped_output += "\\\\";
        else if (*p == '\n') escaped_output += "\\n";
        else if (*p == '\r') escaped_output += "\\r";
        else if (*p == '\t') escaped_output += "\\t";
        else escaped_output += *p;
    }

    // Build response
    char json[8192];
    snprintf(json, sizeof(json),
        "{\"output\": \"%s\", \"tokens\": %d, \"momentum\": %.3f, "
        "\"graph_nodes\": %d, \"graph_edges\": %d}",
        escaped_output.c_str(), n_generated, avg_momentum,
        g_dual ? g_dual->num_nodes : 0,
        g_dual ? g_dual->num_edges : 0);

    return std::string(json);
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static void consolidate_memory() {
    if (!g_dual || g_dual->num_nodes == 0) return;

    fprintf(stderr, "[CONSOLIDATE] Saving %d nodes, %d edges...\n",
            g_dual->num_nodes, g_dual->num_edges);

    char path[1024];
    snprintf(path, sizeof(path), "%s/graph.bin", g_storage_dir.c_str());
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(&g_dual->num_nodes, sizeof(int), 1, f);
        fwrite(&g_dual->num_edges, sizeof(int), 1, f);
        fwrite(g_dual->nodes, sizeof(zeta_graph_node_t), g_dual->num_nodes, f);
        fwrite(g_dual->edges, sizeof(zeta_graph_edge_t), g_dual->num_edges, f);
        fclose(f);
        fprintf(stderr, "[CONSOLIDATE] Saved to %s\n", path);
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static void save_graph() {
    if (!g_dual || g_dual->num_nodes == 0) return;

    char path[1024];
    snprintf(path, sizeof(path), "%s/graph.bin", g_storage_dir.c_str());
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(&g_dual->num_nodes, sizeof(int), 1, f);
        fwrite(&g_dual->num_edges, sizeof(int), 1, f);
        fwrite(g_dual->nodes, sizeof(zeta_graph_node_t), g_dual->num_nodes, f);
        fwrite(g_dual->edges, sizeof(zeta_graph_edge_t), g_dual->num_edges, f);
        fclose(f);
        fprintf(stderr, "[SAVE] Persisted %d nodes, %d edges to %s\n",
                g_dual->num_nodes, g_dual->num_edges, path);
    } else {
        fprintf(stderr, "[SAVE] ERROR: Could not open %s for writing\n", path);
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static void load_graph() {
    if (!g_dual) return;

    char path[1024];
    snprintf(path, sizeof(path), "%s/graph.bin", g_storage_dir.c_str());
    FILE* f = fopen(path, "rb");
    if (f) {
        size_t r = fread(&g_dual->num_nodes, sizeof(int), 1, f);
        r += fread(&g_dual->num_edges, sizeof(int), 1, f);
        r += fread(g_dual->nodes, sizeof(zeta_graph_node_t), g_dual->num_nodes, f);
        r += fread(g_dual->edges, sizeof(zeta_graph_edge_t), g_dual->num_edges, f);
        fclose(f);
        (void)r;  // Suppress unused warning
        // Update next IDs to avoid conflicts with loaded data
        int64_t max_node_id = 0;
        int64_t max_edge_id = 0;
        for (int i = 0; i < g_dual->num_nodes; i++) {
            if (g_dual->nodes[i].node_id > max_node_id) {
                max_node_id = g_dual->nodes[i].node_id;
            }
        }
        for (int i = 0; i < g_dual->num_edges; i++) {
            if (g_dual->edges[i].edge_id > max_edge_id) {
                max_edge_id = g_dual->edges[i].edge_id;
            }
        }
        g_dual->next_node_id = max_node_id + 1;
        g_dual->next_edge_id = max_edge_id + 1;

        fprintf(stderr, "[LOAD] Restored %d nodes, %d edges from %s (next_id=%lld)\n",
                g_dual->num_nodes, g_dual->num_edges, path, (long long)g_dual->next_node_id);
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

static void signal_handler(int sig) {
    const char* sig_name = (sig == SIGTERM) ? "SIGTERM" :
                           (sig == SIGINT)  ? "SIGINT"  : "SIGNAL";
    fprintf(stderr, "\n[SHUTDOWN] Received %s...\n", sig_name);
    save_graph();
    g_shutdown_requested = true;
    if (g_server) g_server->stop();
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"
// Quiet log callback - filter tensor spam
static void quiet_log_callback(enum ggml_log_level level, const char* text, void* user_data) {
    (void)user_data;
    if (level == GGML_LOG_LEVEL_ERROR || level == GGML_LOG_LEVEL_WARN) {
        fprintf(stderr, "%s", text);
    } else if (level == GGML_LOG_LEVEL_INFO) {
        if (strstr(text, "loading tensor") || strstr(text, "create_tensor") ||
            strstr(text, "llama_kv_cache: layer") || strstr(text, "kv  ")) return;
        fprintf(stderr, "%s", text);
    }
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"



// Tool confirmation callback - sends request to client



int main(int argc, char** argv) {
    // Suppress tensor loading spam
    llama_log_set(quiet_log_callback, NULL);
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -m model_14b.gguf [--model-3b model_3b.gguf] [--model-3b-coder coder.gguf] [--port 9000]\n", argv[0]);
        return 1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    std::string model_14b_path, model_3b_path, model_3b_coder_path, model_7b_coder_path;
    int port = 9000;

    g_params.sampling.temp = 0.7f;
    g_params.sampling.top_p = 0.9f;
    g_params.sampling.top_k = 40;
    g_params.sampling.penalty_repeat = 1.15f;
    g_params.sampling.penalty_last_n = 64;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i+1 < argc) model_14b_path = argv[++i];
        else if (strcmp(argv[i], "--model-3b") == 0 && i+1 < argc) model_3b_path = argv[++i];
        else if (strcmp(argv[i], "--model-3b-coder") == 0 && i+1 < argc) model_3b_coder_path = argv[++i];
        else if (strcmp(argv[i], "--model-7b-coder") == 0 && i+1 < argc) model_7b_coder_path = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i+1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--zeta-storage") == 0 && i+1 < argc) g_storage_dir = argv[++i];
        else if (strcmp(argv[i], "--embed-model") == 0 && i+1 < argc) g_embed_model_path = argv[++i];
        else if (strcmp(argv[i], "--embed-model-code") == 0 && i+1 < argc) g_embed_model_code_path = argv[++i];
        else if (strcmp(argv[i], "--stream-tokens") == 0 && i+1 < argc) g_stream_token_budget = atoi(argv[++i]);
        else if (strcmp(argv[i], "--stream-nodes") == 0 && i+1 < argc) g_stream_max_nodes = atoi(argv[++i]);
        else if (strcmp(argv[i], "--code-tokens") == 0 && i+1 < argc) g_code_token_budget = atoi(argv[++i]);
        else if (strcmp(argv[i], "--code-nodes") == 0 && i+1 < argc) g_code_max_nodes = atoi(argv[++i]);
    }

    fprintf(stderr, "Z.E.T.A. Server v5.0 (Parallel Dual-Process)\n");
    fprintf(stderr, "Streaming budget: %d tokens, %d nodes\n", g_stream_token_budget, g_stream_max_nodes);
    fprintf(stderr, "Code budget:      %d tokens, %d nodes\n", g_code_token_budget, g_code_max_nodes);
    fprintf(stderr, "14B Conscious: %s\n", model_14b_path.c_str());
    fprintf(stderr, "3B Subconscious: %s\n", model_3b_path.empty() ? "(pattern-based)" : model_3b_path.c_str());
    fprintf(stderr, "Port: %d\n", port);

    // Load 14B model
    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 99;
    g_model_14b = llama_model_load_from_file(model_14b_path.c_str(), mparams);
    if (!g_model_14b) { fprintf(stderr, "Failed to load 14B model\n"); return 1; }

    // Load 3B model if specified
    if (!model_3b_path.empty()) {
        llama_model_params mparams_3b = llama_model_default_params();
        mparams_3b.n_gpu_layers = 99;
        g_model_3b = llama_model_load_from_file(model_3b_path.c_str(), mparams_3b);
        if (g_model_3b) fprintf(stderr, "3B Subconscious model loaded\n");
    }

    // Initialize embedding model for semantic retrieval
    if (!g_embed_model_path.empty()) {
        if (zeta_embed_init(g_embed_model_path.c_str())) {
            fprintf(stderr, "Embedding model loaded: %s\n", g_embed_model_path.c_str());
        } else {
            fprintf(stderr, "WARNING: Failed to load embedding model\n");
        }
    }

    // Skip 3B Coder at startup - load dynamically on mode switch
    if (false && !model_3b_coder_path.empty()) { // Disabled - dynamic loading
        llama_model_params mparams_coder = llama_model_default_params();
        mparams_coder.n_gpu_layers = 99;
        g_model_3b_coder = llama_model_load_from_file(model_3b_coder_path.c_str(), mparams_coder);
        if (g_model_3b_coder) fprintf(stderr, "3B Coder model loaded (for code mode)\n");
    }

    // Init 14B context
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = ZETA_CTX_SIZE;  // Configurable for GPU VRAM
    cparams.n_batch = ZETA_BATCH_SIZE;
    g_ctx_14b = llama_init_from_model(g_model_14b, cparams);
    if (!g_ctx_14b) { fprintf(stderr, "Failed to create 14B context\n"); return 1; }

    g_vocab = llama_model_get_vocab(g_model_14b);
    zeta_set_vocab(g_vocab);  // Enable tokenization at storage
    g_n_embd = llama_model_n_embd(g_model_14b);

    // Init ZETA memory
    // Increased retrieval threshold to 0.35f to filter noise in high-load scenarios
    g_zeta = zeta_context_init(g_ctx_14b, g_storage_dir.c_str(), nullptr, 0.1f, 0.15f, 0.35f, 0.2f);

    // Init dual-process engine
    g_dual = zeta_dual_init(g_model_3b ? g_model_3b : g_model_14b, g_storage_dir.c_str());

    // Initialize streaming memory state
    memset(&g_stream_state, 0, sizeof(g_stream_state));
    // Initialize code mode context (3B Coder not loaded yet - will use 3B Instruct)
    g_code = zeta_code_init(g_dual, g_model_3b, NULL, g_model_14b,
        (g_storage_dir + "/code").c_str());
    if (g_code) fprintf(stderr, "[INIT] Code mode context initialized\n");
    // Set model paths for dynamic swapping
    if (g_code) zeta_set_model_paths(g_code, model_3b_path.c_str(), model_3b_coder_path.c_str(), model_14b_path.c_str(), model_7b_coder_path.c_str(), g_embed_model_path.c_str(), g_embed_model_code_path.c_str());
    if (g_dual) {
        load_graph();  // Restore previous graph
        g_dual->current_session_id = (int64_t)time(NULL);
        fprintf(stderr, "[SESSION] Started session %lld\n", (long long)g_dual->current_session_id);
        fprintf(stderr, "Dual-process engine initialized (nodes=%d, edges=%d)\n",
                g_dual->num_nodes, g_dual->num_edges);

        // START 3B PARALLEL WORKER
        g_3b_worker_tid = zeta_3b_start_worker(g_dual);
        g_3b_worker_running = true;
        fprintf(stderr, "3B parallel worker started\n");
    }

    httplib::Server svr;
    g_server = &svr;

    svr.Post("/generate", [](const httplib::Request& req, httplib::Response& res) {
        g_last_activity = time(NULL);  // Track activity

        // Parse JSON body
        std::string prompt;
        std::string mode = "chat";
        std::string project_id;
        int max_tokens = 2048;  // Increased default from 100

        // Try JSON body first
            // Parse mode
            size_t mode_pos = req.body.find("\"mode\":");
            if (mode_pos != std::string::npos) {
                size_t ms = req.body.find('"', mode_pos + 7);
                if (ms != std::string::npos) {
                    size_t me = req.body.find('"', ms + 1);
                    if (me != std::string::npos) mode = req.body.substr(ms + 1, me - ms - 1);
                }
            }
            // Parse project_id
            size_t proj_pos = req.body.find("\"project_id\":");
            if (proj_pos != std::string::npos) {
                size_t ps = req.body.find('"', proj_pos + 13);
                if (ps != std::string::npos) {
                    size_t pe = req.body.find('"', ps + 1);
                    if (pe != std::string::npos) project_id = req.body.substr(ps + 1, pe - ps - 1);
                }
            }
        if (!req.body.empty()) {
            // Simple JSON parsing for {"prompt": "...", "max_tokens": N}
            size_t prompt_pos = req.body.find("\"prompt\":");
            if (prompt_pos != std::string::npos) {
                size_t start = req.body.find('\"', prompt_pos + 9);
                if (start != std::string::npos) {
                    size_t end = req.body.find('\"', start + 1);
                    if (end != std::string::npos) {
                        prompt = req.body.substr(start + 1, end - start - 1);
                    }
                }
            }
            size_t tokens_pos = req.body.find("\"max_tokens\":");
            if (tokens_pos != std::string::npos) {
                size_t num_start = tokens_pos + 13;
                while (num_start < req.body.size() && !isdigit(req.body[num_start])) num_start++;
                if (num_start < req.body.size()) {
                    max_tokens = std::stoi(req.body.substr(num_start));
                }
            }
        }

        fprintf(stderr, "[GENERATE] Mode: %s, Project: %s\\n", mode.c_str(), project_id.c_str());
        // Fallback to URL params
        if (prompt.empty()) {
            prompt = req.get_param_value("prompt");
            if (req.has_param("max_tokens")) max_tokens = std::stoi(req.get_param_value("max_tokens"));
        }
        std::string result = generate(prompt, max_tokens);
        // Save graph after each generate (resilience against crash)
        if (g_dual && g_dual->num_nodes > 0) consolidate_memory();
        res.set_content(result, "application/json");
    });

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        char json[512];
        snprintf(json, sizeof(json),
            "{\"status\": \"ok\", \"version\": \"5.0\", "
            "\"parallel_3b\": %s, \"graph_nodes\": %d, \"graph_edges\": %d}",
            g_3b_worker_running ? "true" : "false",
            g_dual ? g_dual->num_nodes : 0,
            g_dual ? g_dual->num_edges : 0);
        res.set_content(json, "application/json");
    });


    // ========================================================================
    // TOOL SYSTEM ENDPOINTS
    // ========================================================================

    svr.Get("/tools", [](const httplib::Request&, httplib::Response& res) {
        std::string schema = zeta_tools::get_tool_schema();
        res.set_content(schema, "application/json");
    });

    svr.Get("/tools/describe", [](const httplib::Request&, httplib::Response& res) {
        std::string desc = zeta_tools::get_tool_prompt();
        char json[16384];
        // Escape special chars in desc for JSON
        std::string escaped_desc;
        for (char c : desc) {
            if (c == '\n') escaped_desc += "\\n";
            else if (c == '\t') escaped_desc += "\\t";
            else if (c == '"') escaped_desc += "\\\"";
            else if (c == '\\') escaped_desc += "\\\\";
            else escaped_desc += c;
        }
        snprintf(json, sizeof(json), "{\"tools\": \"%s\"}", escaped_desc.c_str());
        res.set_content(json, "application/json");
    });

    svr.Post("/tool/execute", [](const httplib::Request& req, httplib::Response& res) {
        char json_out[8192];

        // Simple JSON parsing for tool name and params
        std::string body = req.body;
        std::string tool_name;
        std::map<std::string, std::string> params;

        // Extract tool name
        size_t tool_pos = body.find("\"tool\"");
        if (tool_pos != std::string::npos) {
            size_t start = body.find("\"", tool_pos + 7);
            if (start != std::string::npos) {
                size_t end = body.find("\"", start + 1);
                if (end != std::string::npos) {
                    tool_name = body.substr(start + 1, end - start - 1);
                }
            }
        }

        // Extract params (simple key-value parsing)
        size_t params_pos = body.find("\"params\"");
        if (params_pos != std::string::npos) {
            size_t brace_start = body.find("{", params_pos);
            size_t brace_end = body.rfind("}");
            if (brace_start != std::string::npos && brace_end > brace_start) {
                std::string params_str = body.substr(brace_start + 1, brace_end - brace_start - 1);
                // Parse key-value pairs
                size_t pos = 0;
                while (pos < params_str.size()) {
                    size_t key_start = params_str.find("\"", pos);
                    if (key_start == std::string::npos) break;
                    size_t key_end = params_str.find("\"", key_start + 1);
                    if (key_end == std::string::npos) break;
                    std::string key = params_str.substr(key_start + 1, key_end - key_start - 1);

                    size_t val_start = params_str.find("\"", key_end + 1);
                    if (val_start == std::string::npos) break;
                    size_t val_end = params_str.find("\"", val_start + 1);
                    if (val_end == std::string::npos) break;
                    std::string val = params_str.substr(val_start + 1, val_end - val_start - 1);

                    params[key] = val;
                    pos = val_end + 1;
                }
            }
        }

        if (tool_name.empty()) {
            snprintf(json_out, sizeof(json_out),
                "{\"error\": \"Missing tool name\", \"blocked\": true}");
            res.set_content(json_out, "application/json");
            return;
        }

        // Execute tool (pass g_dual as context for graph validation)
        auto result = zeta_tools::g_tool_registry.execute(tool_name, params,
            reinterpret_cast<zeta_ctx_t*>(g_dual));

        // Build response
        snprintf(json_out, sizeof(json_out),
            "{\"tool\": \"%s\", \"status\": %d, \"output\": \"%.4000s\", "
            "\"error\": \"%s\", \"blocked\": %s}",
            tool_name.c_str(),
            static_cast<int>(result.status),
            result.output.substr(0, 4000).c_str(),
            result.error_msg.c_str(),
            result.status != zeta_tools::ToolStatus::SUCCESS ? "true" : "false");

        res.set_content(json_out, "application/json");
    });


    // Cache clear endpoint
    svr.Get("/cache/clear", [](const httplib::Request&, httplib::Response& res) {
        if (g_ctx_14b) {
            auto mem = llama_get_memory(g_ctx_14b);
            if (mem) llama_memory_clear(mem, true);
        }
        // Decay based on salience and age - remove lowest 10%
        int removed = 0;
        if (g_dual && g_dual->num_nodes > 10) {
            time_t now = time(NULL);
            for (int i = g_dual->num_nodes - 1; i >= 0 && removed < g_dual->num_nodes / 10; i--) {
                auto& n = g_dual->nodes[i];
                int64_t age = now - n.last_accessed;
                if (n.salience < 0.3f && age > 3600) {  // Low salience + 1hr old
                    n.is_active = false;
                    removed++;
                }
            }
        }
        char resp[128];
        snprintf(resp, sizeof(resp), "{\"status\": \"ok\", \"decayed\": %d}", removed);
        res.set_content(resp, "application/json");
    });

    // Unload 3B to free VRAM
    svr.Get("/system/unload-3b", [](const httplib::Request&, httplib::Response& res) {
        if (g_code && g_code->models.ctx_3b) {
            llama_free(g_code->models.ctx_3b);
            g_code->models.ctx_3b = NULL;
        }
        if (g_code && g_code->models.model_3b_instruct) {
            llama_model_free(g_code->models.model_3b_instruct);
            g_code->models.model_3b_instruct = NULL;
        }
        if (g_code && g_code->models.model_3b_coder) {
            llama_model_free(g_code->models.model_3b_coder);
            g_code->models.model_3b_coder = NULL;
        }
        res.set_content("{\"status\": \"ok\", \"freed\": \"3b_models\"}", "application/json");
    });


    svr.Get("/graph", [](const httplib::Request&, httplib::Response& res) {
        if (!g_dual || g_dual->num_nodes == 0) {
            res.set_content("{\"nodes\": [], \"edges\": []}", "application/json");
            return;
        }
        std::string json = "{\"nodes\": [";
        int dumped = 0;
        for (int i = 0; i < g_dual->num_nodes && dumped < 50; i++) {
            auto& n = g_dual->nodes[i];
            if (!n.is_active) continue;  // Only dump active nodes
            if (dumped > 0) json += ",";
            dumped++;
            char buf[512];
            // Sanitize concept_key for JSON output
            char safe_ck[64] = {0};
            for (int j = 0; j < 63 && n.concept_key[j]; j++) {
                if (n.concept_key[j] >= 32 && n.concept_key[j] < 127) safe_ck[j] = n.concept_key[j];
            }
            snprintf(buf, sizeof(buf),
                "{\"id\": %lld, \"label\": \"%s\", \"value\": \"%s\", \"salience\": %.2f, \"concept_key\": \"%s\", \"superseded_by\": %lld}",
                (long long)n.node_id, n.label, n.value, n.salience, safe_ck, (long long)n.superseded_by);
            json += buf;
        }
        json += "], \"edges\": [";
        for (int i = 0; i < g_dual->num_edges && i < 100; i++) {
            if (i > 0) json += ",";
            auto& e = g_dual->edges[i];
            char buf[128];
            snprintf(buf, sizeof(buf),
                "{\"src\": %lld, \"tgt\": %lld, \"type\": %d, \"w\": %.2f}",
                (long long)e.source_id, (long long)e.target_id, e.type, e.weight);
            json += buf;
        }
        json += "]}";
        res.set_content(json, "application/json");
    });


    // =========================================================================
    // Project/Code Mode API
    // =========================================================================

    svr.Post("/project/open", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        std::string path = req.get_param_value("path");
        std::string name = req.get_param_value("name");
        std::string desc = req.get_param_value("description");

        if (path.empty()) {
            res.set_content("{\"error\": \"path required\"}", "application/json");
            return;
        }

        if (!g_code) {
            res.set_content("{\"error\": \"code mode not initialized\"}", "application/json");
            return;
        }

        zeta_project_t* proj = zeta_project_open(g_code, path.c_str(),
            name.empty() ? nullptr : name.c_str(),
            desc.empty() ? nullptr : desc.c_str());

        if (!proj) {
            res.set_content("{\"error\": \"failed to open project\"}", "application/json");
            return;
        }

        // Switch to code mode - swap 3B Instruct for 3B Coder
        zeta_switch_to_code_mode(g_code);
        if (g_ctx_14b) { llama_free(g_ctx_14b); g_ctx_14b = nullptr; }
        if (g_code->models.active_main) {
            llama_context_params cp = llama_context_default_params();
            cp.n_ctx = ZETA_CTX_SIZE; cp.n_batch = ZETA_BATCH_SIZE;
            g_ctx_14b = llama_init_from_model(g_code->models.active_main, cp);
            g_model_14b = g_code->models.active_main; // Update model pointer for sampler
            g_vocab = llama_model_get_vocab(g_model_14b); // Update vocab for tokenizer
        }
        // Sync dual-process context with new 3B model (7B coder in code mode)
        if (g_dual) {
            if (g_dual->ctx_3b) { llama_free(g_dual->ctx_3b); g_dual->ctx_3b = nullptr; }
            g_dual->model_3b = g_code->models.model_3b_coder;
            if (g_dual->model_3b) {
                llama_context_params dp = llama_context_default_params();
                dp.n_ctx = ZETA_CTX_SIZE; dp.n_batch = ZETA_BATCH_SIZE;
                g_dual->ctx_3b = llama_init_from_model(g_dual->model_3b, dp);
                fprintf(stderr, "[MODE] Synced dual-process to 7B Coder\n");
            }
        }
        fprintf(stderr, "[MODE] Switched to CODE mode\n");

        char json[2048];
        snprintf(json, sizeof(json),
            "{\"status\": \"ok\", \"project_id\": \"%s\", \"name\": \"%s\", \"mode\": \"code\"}",
            proj->project_id, proj->project_name);
        res.set_content(json, "application/json");
    });

    svr.Post("/project/close", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!g_code || !g_code->active_project) {
            res.set_content("{\"error\": \"no active project\"}", "application/json");
            return;
        }

        // Switch back to chat mode - swap 3B Coder for 3B Instruct
        zeta_switch_to_chat_mode(g_code);
        if (g_ctx_14b) { llama_free(g_ctx_14b); g_ctx_14b = nullptr; }
        if (g_code->models.active_main) {
            llama_context_params cp = llama_context_default_params();
            cp.n_ctx = ZETA_CTX_SIZE; cp.n_batch = ZETA_BATCH_SIZE;
            g_ctx_14b = llama_init_from_model(g_code->models.active_main, cp);
            g_model_14b = g_code->models.active_main; // Update model pointer for sampler
            g_vocab = llama_model_get_vocab(g_model_14b); // Update vocab for tokenizer
        }
        // Sync dual-process context with new 3B model (3B Instruct in chat mode)
        if (g_dual) {
            if (g_dual->ctx_3b) { llama_free(g_dual->ctx_3b); g_dual->ctx_3b = nullptr; }
            g_dual->model_3b = g_code->models.model_3b_instruct;
            if (g_dual->model_3b) {
                llama_context_params dp = llama_context_default_params();
                dp.n_ctx = ZETA_CTX_SIZE; dp.n_batch = ZETA_BATCH_SIZE;
                g_dual->ctx_3b = llama_init_from_model(g_dual->model_3b, dp);
                fprintf(stderr, "[MODE] Synced dual-process to 3B Instruct\n");
            }
        }
        fprintf(stderr, "[MODE] Switched to CHAT mode\n");
        zeta_project_close(g_code);
        res.set_content("{\"status\": \"ok\", \"mode\": \"chat\"}", "application/json");
    });

    svr.Get("/project/current", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!g_code) {
            res.set_content("{\"mode\": \"chat\", \"project\": null}", "application/json");
            return;
        }

        zeta_project_t* proj = zeta_project_current(g_code);
        if (!proj) {
            res.set_content("{\"mode\": \"chat\", \"project\": null}", "application/json");
            return;
        }

        char json[4096];
        snprintf(json, sizeof(json),
            "{\"mode\": \"code\", \"project\": {"
            "\"id\": \"%s\", \"name\": \"%s\", \"path\": \"%s\", "
            "\"description\": \"%s\", \"languages\": \"%s\", "
            "\"tags\": \"%s\", \"status\": \"%s\", "
            "\"file_count\": %d, \"function_count\": %d, \"todo_count\": %d}}",
            proj->project_id, proj->project_name, proj->root_path,
            proj->description, proj->languages, proj->tags, proj->status,
            proj->file_count, proj->function_count, proj->todo_count);
        res.set_content(json, "application/json");
    });

    svr.Get("/projects/list", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!g_code) {
            res.set_content("{\"projects\": []}", "application/json");
            return;
        }

        std::string json = "{\"projects\": [";
        for (int i = 0; i < g_code->project_count; i++) {
            if (i > 0) json += ",";
            char buf[1024];
            snprintf(buf, sizeof(buf),
                "{\"id\": \"%s\", \"name\": \"%s\", \"status\": \"%s\", \"is_open\": %s}",
                g_code->projects[i].project_id,
                g_code->projects[i].project_name,
                g_code->projects[i].status,
                g_code->projects[i].is_open ? "true" : "false");
            json += buf;
        }
        json += "]}";
        res.set_content(json, "application/json");
    });

    svr.Post("/code/check", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        std::string entity_type = req.get_param_value("type");
        std::string entity_name = req.get_param_value("name");
        std::string file_path = req.get_param_value("file");

        if (!g_code || !g_code->active_project) {
            res.set_content("{\"error\": \"no active project\"}", "application/json");
            return;
        }

        char reason[512] = {0};
        bool can_create = zeta_can_create(g_code, entity_type.c_str(),
            entity_name.c_str(), file_path.c_str(), reason, sizeof(reason));

        char json[1024];
        snprintf(json, sizeof(json),
            "{\"can_create\": %s, \"reason\": \"%s\"}",
            can_create ? "true" : "false", reason);
        res.set_content(json, "application/json");
    });

    svr.Get("/code/recent", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!g_code || !g_code->active_project) {
            res.set_content("{\"error\": \"no active project\"}", "application/json");
            return;
        }

        char buffer[4096] = {0};
        zeta_surface_recent_work(g_code, buffer, sizeof(buffer));

        // Escape for JSON
        std::string escaped;
        for (char c : std::string(buffer)) {
            if (c == '\n') escaped += "\\n";
            else if (c == '\"') escaped += "\\\"";
            else escaped += c;
        }

        char json[8192];
        snprintf(json, sizeof(json), "{\"recent_work\": \"%s\"}", escaped.c_str());
        res.set_content(json, "application/json");
    });

    // POST /code/extract - Extract code entities from text
    svr.Post("/code/extract", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_mutex);

        std::string text = req.get_param_value("text");
        if (text.empty()) {
            res.set_content("{\"error\": \"text required\"}", "application/json");
            return;
        }

        if (!g_code || !g_code->active_project) {
            res.set_content("{\"error\": \"no project open\"}", "application/json");
            return;
        }

        int added = zeta_code_extract_entities(g_code, text.c_str());
        char json[512];
        snprintf(json, sizeof(json), "{\"status\": \"ok\", \"entities_added\": %d}", added);
        res.set_content(json, "application/json");
    });

    svr.Post("/shutdown", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\": \"shutting_down\"}", "application/json");

        save_graph();
    g_shutdown_requested = true;
        if (g_server) g_server->stop();
    });

    fprintf(stderr, "\nZ.E.T.A. Server v5.0 listening on port %d\n", port);
    fprintf(stderr, "  POST /generate - Generate with parallel 3B memory\n");
    fprintf(stderr, "  GET  /health   - Health check\n");
    fprintf(stderr, "  GET  /graph    - View memory graph\n");
    fprintf(stderr, "  POST /shutdown - Graceful shutdown\n");

    // Start new session (versions old data)
    svr.Post("/session/new", [](const httplib::Request& req, httplib::Response& res) {
        int64_t old_session = g_dual->current_session_id;
        g_dual->current_session_id = (int64_t)time(NULL);
        char buf[256];
        snprintf(buf, sizeof(buf), "{\"status\": \"new_session\", \"old_session\": %lld, \"new_session\": %lld}",
                 (long long)old_session, (long long)g_dual->current_session_id);
        res.set_content(buf, "application/json");
        fprintf(stderr, "[SESSION] New session %lld (old: %lld)\n",
                (long long)g_dual->current_session_id, (long long)old_session);
    });
    fprintf(stderr, "  POST /project/open  - Open project (code mode)\n");
    fprintf(stderr, "  POST /project/close - Close project (chat mode)\n");
    fprintf(stderr, "  GET  /project/current - Current project info\n");
    fprintf(stderr, "  GET  /projects/list - List all projects\n");
    fprintf(stderr, "  POST /code/check    - Check if can create entity\n");
    fprintf(stderr, "  GET  /code/recent   - Recent work in project\n\n");
    fprintf(stderr, "  POST /code/extract  - Extract code entities from text\n");
    g_last_activity = time(NULL);
    g_idle_watchdog = std::thread(idle_watchdog_thread);
    fprintf(stderr, "[IDLE] Watchdog started (decay@5m, 3B always loaded)\n");


    // Initialize tool system
    fprintf(stderr, "[TOOLS] Tool system initialized with %zu tools\n",
            zeta_tools::g_tool_registry.tools.size());

    svr.listen("0.0.0.0", port);

    fprintf(stderr, "\n[SHUTDOWN] Stopping 3B worker...\n");
    if (g_3b_worker_running) {
        zeta_3b_stop_worker(g_3b_worker_tid);
        g_3b_worker_running = false;
    }

    fprintf(stderr, "[SHUTDOWN] Consolidating memory...\n");
    consolidate_memory();

    if (g_dual) free(g_dual);
    if (g_zeta) zeta_context_free(g_zeta);
    llama_free(g_ctx_14b);
    llama_model_free(g_model_14b);
    if (g_model_3b) llama_model_free(g_model_3b);
    if (g_model_3b_coder) llama_model_free(g_model_3b_coder);

    fprintf(stderr, "[SHUTDOWN] Complete.\n");
    return 0;
}

// C++ tool system (must be outside extern C)
#include "zeta-tools.h"

