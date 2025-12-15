// Z.E.T.A. Code Mode - Extension for code structure tracking
// Dynamic 3B model swapping: Instruct <-> Coder

#ifndef ZETA_CODE_MODE_H
#define ZETA_CODE_MODE_H

#include "zeta-dual-process.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZETA_MAX_PROJECTS        64
#define ZETA_MAX_CODE_NODES      20000
#define ZETA_MAX_CREATIONS       1000
#define ZETA_MAX_CONFLICTS       64
#define ZETA_MAX_ASSETS          128
#define ZETA_MAX_OVERRIDES       256
#define ZETA_RECENT_WINDOW_SEC   1800

typedef enum {
    CODE_NODE_FILE, CODE_NODE_FUNCTION, CODE_NODE_CLASS, CODE_NODE_VARIABLE,
    CODE_NODE_IMPORT, CODE_NODE_TODO, CODE_NODE_DECISION, CODE_NODE_CONSTRAINT
} zeta_code_node_type_t;

typedef enum {
    CODE_EDGE_CONTAINS, CODE_EDGE_CALLS, CODE_EDGE_IMPORTS, CODE_EDGE_INHERITS,
    CODE_EDGE_IMPLEMENTS, CODE_EDGE_DEPENDS_ON, CODE_EDGE_SUPERSEDES
} zeta_code_edge_type_t;

typedef struct {
    char project_id[64];
    char project_name[128];
    char root_path[512];
    char description[1024];
    char languages[256];
    char tags[512];
    char status[64];
    int64_t created_at;
    int64_t last_accessed;
    int file_count;
    int function_count;
    int todo_count;
    bool is_open;
} zeta_project_t;

typedef struct {
    int64_t node_id;
    char project_id[64];
    zeta_code_node_type_t type;
    char name[128];
    char file_path[512];
    int line_start;
    int line_end;
    char signature[512];
    char scope[128];
    float salience;
    int64_t created_at;
    zeta_source_t source;
} zeta_code_node_t;

typedef struct {
    char entity_type[32];
    char entity_name[128];
    char file_path[512];
    int line_start;
    int line_end;
    int64_t created_at;
    zeta_source_t author;
    char project_id[64];
} zeta_creation_event_t;

typedef enum { SEVERITY_BLOCKER, SEVERITY_WARNING, SEVERITY_INFO } zeta_severity_t;
typedef enum { CONFLICT_NAMING, CONFLICT_CIRCULAR, CONFLICT_API_BREAK, CONFLICT_TYPE_MISMATCH } zeta_conflict_type_t;

typedef struct {
    int conflict_id;
    zeta_conflict_type_t type;
    zeta_severity_t severity;
    char description[512];
    bool user_acknowledged;
} zeta_conflict_t;

typedef struct {
    zeta_conflict_t conflicts[ZETA_MAX_CONFLICTS];
    int conflict_count;
    int blocker_count;
    bool all_resolved;
} zeta_conflict_review_t;

// Model context with dynamic swapping
typedef struct {
    llama_model* model_3b_instruct;
    llama_model* model_3b_coder;
    llama_model* model_14b;
    llama_model* active_3b;
    llama_context* ctx_3b;
    
    // Paths for dynamic loading
    char path_3b_instruct[512];
    char path_3b_coder[512];
    
    bool in_code_mode;
} zeta_model_ctx_t;

typedef struct {
    zeta_dual_ctx_t* base_ctx;
    zeta_model_ctx_t models;
    zeta_project_t projects[ZETA_MAX_PROJECTS];
    int project_count;
    zeta_project_t* active_project;
    zeta_code_node_t code_nodes[ZETA_MAX_CODE_NODES];
    int code_node_count;
    zeta_creation_event_t creations[ZETA_MAX_CREATIONS];
    int creation_count;
    zeta_conflict_review_t current_review;
    char code_storage_dir[512];
} zeta_code_ctx_t;

// Hash project ID from path
static inline void zeta_hash_project_id(const char* path, char* out_id, size_t out_len) {
    uint64_t hash = 5381;
    for (const char* p = path; *p; p++) hash = ((hash << 5) + hash) + (uint64_t)*p;
    snprintf(out_id, out_len, "proj_%016llx", (unsigned long long)hash);
}

// Initialize with model paths for dynamic loading
static inline zeta_code_ctx_t* zeta_code_init(
    zeta_dual_ctx_t* base_ctx,
    llama_model* model_3b_instruct,
    llama_model* model_3b_coder,
    llama_model* model_14b,
    const char* code_storage_dir
) {
    zeta_code_ctx_t* ctx = (zeta_code_ctx_t*)calloc(1, sizeof(zeta_code_ctx_t));
    if (!ctx) return NULL;
    ctx->base_ctx = base_ctx;
    ctx->models.model_3b_instruct = model_3b_instruct;
    ctx->models.model_3b_coder = model_3b_coder;
    ctx->models.model_14b = model_14b;
    ctx->models.active_3b = model_3b_instruct;
    ctx->models.in_code_mode = false;
    strncpy(ctx->code_storage_dir, code_storage_dir, sizeof(ctx->code_storage_dir) - 1);
    return ctx;
}

// Set model paths for dynamic swapping
static inline void zeta_set_model_paths(zeta_code_ctx_t* ctx, const char* instruct_path, const char* coder_path) {
    if (instruct_path) strncpy(ctx->models.path_3b_instruct, instruct_path, 511);
    if (coder_path) strncpy(ctx->models.path_3b_coder, coder_path, 511);
}

// Switch to code mode - UNLOAD instruct, LOAD coder
static inline void zeta_switch_to_code_mode(zeta_code_ctx_t* ctx) {
    if (!ctx) return;
    
    // Free 3B context first
    if (ctx->models.ctx_3b) {
        llama_free(ctx->models.ctx_3b);
        ctx->models.ctx_3b = NULL;
    }
    
    // Unload 3B Instruct to free VRAM
    if (ctx->models.model_3b_instruct) {
        fprintf(stderr, "[MODE] Unloading 3B Instruct...\n");
        llama_model_free(ctx->models.model_3b_instruct);
        ctx->models.model_3b_instruct = NULL;
    }
    
    // Load 3B Coder if path set and not already loaded
    if (!ctx->models.model_3b_coder && ctx->models.path_3b_coder[0]) {
        fprintf(stderr, "[MODE] Loading 3B Coder from %s...\n", ctx->models.path_3b_coder);
        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 99;
        ctx->models.model_3b_coder = llama_model_load_from_file(ctx->models.path_3b_coder, mparams);
        if (ctx->models.model_3b_coder) {
            fprintf(stderr, "[MODE] 3B Coder loaded\n");
        }
    }
    
    ctx->models.active_3b = ctx->models.model_3b_coder;
    ctx->models.in_code_mode = true;
    
    // Create context for coder
    if (ctx->models.model_3b_coder) {
        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 512;
        cparams.n_batch = 512;
        ctx->models.ctx_3b = llama_init_from_model(ctx->models.model_3b_coder, cparams);
    }
}

// Switch to chat mode - UNLOAD coder, LOAD instruct
static inline void zeta_switch_to_chat_mode(zeta_code_ctx_t* ctx) {
    if (!ctx) return;
    
    // Free 3B context first
    if (ctx->models.ctx_3b) {
        llama_free(ctx->models.ctx_3b);
        ctx->models.ctx_3b = NULL;
    }
    
    // Unload 3B Coder to free VRAM
    if (ctx->models.model_3b_coder) {
        fprintf(stderr, "[MODE] Unloading 3B Coder...\n");
        llama_model_free(ctx->models.model_3b_coder);
        ctx->models.model_3b_coder = NULL;
    }
    
    // Load 3B Instruct if path set and not already loaded
    if (!ctx->models.model_3b_instruct && ctx->models.path_3b_instruct[0]) {
        fprintf(stderr, "[MODE] Loading 3B Instruct from %s...\n", ctx->models.path_3b_instruct);
        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 99;
        ctx->models.model_3b_instruct = llama_model_load_from_file(ctx->models.path_3b_instruct, mparams);
        if (ctx->models.model_3b_instruct) {
            fprintf(stderr, "[MODE] 3B Instruct loaded\n");
        }
    }
    
    ctx->models.active_3b = ctx->models.model_3b_instruct;
    ctx->models.in_code_mode = false;
    
    // Create context for instruct
    if (ctx->models.model_3b_instruct) {
        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 256;
        cparams.n_batch = 128;
        ctx->models.ctx_3b = llama_init_from_model(ctx->models.model_3b_instruct, cparams);
    }
}

// Open project
static inline zeta_project_t* zeta_project_open(zeta_code_ctx_t* ctx, const char* root_path, const char* name, const char* desc) {
    if (!ctx || !root_path) return NULL;
    char project_id[64];
    zeta_hash_project_id(root_path, project_id, sizeof(project_id));
    
    for (int i = 0; i < ctx->project_count; i++) {
        if (strcmp(ctx->projects[i].project_id, project_id) == 0) {
            ctx->projects[i].is_open = true;
            ctx->projects[i].last_accessed = (int64_t)time(NULL);
            ctx->active_project = &ctx->projects[i];
            zeta_switch_to_code_mode(ctx);
            return ctx->active_project;
        }
    }
    
    if (ctx->project_count >= ZETA_MAX_PROJECTS) return NULL;
    zeta_project_t* proj = &ctx->projects[ctx->project_count++];
    memset(proj, 0, sizeof(zeta_project_t));
    strncpy(proj->project_id, project_id, 63);
    strncpy(proj->root_path, root_path, 511);
    if (name) strncpy(proj->project_name, name, 127);
    if (desc) strncpy(proj->description, desc, 1023);
    proj->created_at = proj->last_accessed = (int64_t)time(NULL);
    proj->is_open = true;
    strncpy(proj->status, "active", 63);
    ctx->active_project = proj;
    
    char proj_dir[1024];
    snprintf(proj_dir, sizeof(proj_dir), "%s/%s", ctx->code_storage_dir, project_id);
    mkdir(proj_dir, 0755);
    char assets_dir[1024];
    snprintf(assets_dir, sizeof(assets_dir), "%s/assets", proj_dir);
    mkdir(assets_dir, 0755);
    
    zeta_switch_to_code_mode(ctx);
    return proj;
}

// Close project
static inline void zeta_project_close(zeta_code_ctx_t* ctx) {
    if (!ctx || !ctx->active_project) return;
    ctx->active_project->is_open = false;
    ctx->active_project->last_accessed = (int64_t)time(NULL);
    ctx->active_project = NULL;
    zeta_switch_to_chat_mode(ctx);
}

static inline zeta_project_t* zeta_project_current(zeta_code_ctx_t* ctx) {
    return ctx ? ctx->active_project : NULL;
}

// Find code node
static inline zeta_code_node_t* zeta_find_code_node(zeta_code_ctx_t* ctx, const char* name, zeta_code_node_type_t type) {
    if (!ctx || !ctx->active_project || !name) return NULL;
    for (int i = 0; i < ctx->code_node_count; i++) {
        if (strcmp(ctx->code_nodes[i].project_id, ctx->active_project->project_id) != 0) continue;
        if (ctx->code_nodes[i].type == type && strcmp(ctx->code_nodes[i].name, name) == 0)
            return &ctx->code_nodes[i];
    }
    return NULL;
}

// Create code node
static inline zeta_code_node_t* zeta_create_code_node(zeta_code_ctx_t* ctx, zeta_code_node_type_t type,
    const char* name, const char* file_path, int line_start, int line_end, zeta_source_t source) {
    if (!ctx || !ctx->active_project || ctx->code_node_count >= ZETA_MAX_CODE_NODES) return NULL;
    zeta_code_node_t* node = &ctx->code_nodes[ctx->code_node_count++];
    memset(node, 0, sizeof(zeta_code_node_t));
    node->node_id = ctx->code_node_count;
    strncpy(node->project_id, ctx->active_project->project_id, 63);
    node->type = type;
    if (name) strncpy(node->name, name, 127);
    if (file_path) strncpy(node->file_path, file_path, 511);
    node->line_start = line_start;
    node->line_end = line_end;
    node->created_at = (int64_t)time(NULL);
    node->source = source;
    node->salience = 1.0f;
    if (type == CODE_NODE_FILE) ctx->active_project->file_count++;
    else if (type == CODE_NODE_FUNCTION) ctx->active_project->function_count++;
    else if (type == CODE_NODE_TODO) ctx->active_project->todo_count++;
    return node;
}

// Log creation
static inline void zeta_log_creation(zeta_code_ctx_t* ctx, const char* type, const char* name,
    const char* file, int start, int end, zeta_source_t author) {
    if (!ctx || !ctx->active_project || ctx->creation_count >= ZETA_MAX_CREATIONS) return;
    zeta_creation_event_t* evt = &ctx->creations[ctx->creation_count++];
    memset(evt, 0, sizeof(zeta_creation_event_t));
    if (type) strncpy(evt->entity_type, type, 31);
    if (name) strncpy(evt->entity_name, name, 127);
    if (file) strncpy(evt->file_path, file, 511);
    evt->line_start = start; evt->line_end = end;
    evt->created_at = (int64_t)time(NULL);
    evt->author = author;
    strncpy(evt->project_id, ctx->active_project->project_id, 63);
}

// Check if can create (duplicate prevention)
static inline bool zeta_can_create(zeta_code_ctx_t* ctx, const char* type, const char* name,
    const char* file, char* reason, size_t reason_len) {
    if (!ctx || !ctx->active_project) return true;
    int64_t now = (int64_t)time(NULL);
    for (int i = 0; i < ctx->code_node_count; i++) {
        if (strcmp(ctx->code_nodes[i].project_id, ctx->active_project->project_id) != 0) continue;
        if (strcmp(ctx->code_nodes[i].name, name) == 0) {
            snprintf(reason, reason_len, "[BLOCKED] %s '%s' exists at %s:%d",
                type, name, ctx->code_nodes[i].file_path, ctx->code_nodes[i].line_start);
            return false;
        }
    }
    for (int i = 0; i < ctx->creation_count; i++) {
        zeta_creation_event_t* evt = &ctx->creations[i];
        if (strcmp(evt->project_id, ctx->active_project->project_id) != 0) continue;
        if (strcmp(evt->entity_name, name) == 0 && (now - evt->created_at) < ZETA_RECENT_WINDOW_SEC) {
            snprintf(reason, reason_len, "[BLOCKED] '%s' created %d min ago",
                name, (int)((now - evt->created_at) / 60));
            return false;
        }
    }
    return true;
}

// Surface recent work
static inline void zeta_surface_recent_work(zeta_code_ctx_t* ctx, char* buf, size_t len) {
    if (!ctx || !ctx->active_project) return;
    int64_t now = (int64_t)time(NULL);
    int off = snprintf(buf, len, "=== RECENT WORK (last 30 min) ===\n");
    int count = 0;
    for (int i = ctx->creation_count - 1; i >= 0 && count < 10; i--) {
        zeta_creation_event_t* evt = &ctx->creations[i];
        if (strcmp(evt->project_id, ctx->active_project->project_id) != 0) continue;
        if ((now - evt->created_at) > ZETA_RECENT_WINDOW_SEC) continue;
        off += snprintf(buf + off, len - off, "[%d min ago] %s %s '%s' in %s\n",
            (int)((now - evt->created_at) / 60),
            evt->author == SOURCE_USER ? "USER" : "AI",
            evt->entity_type, evt->entity_name, evt->file_path);
        count++;
    }
    if (count == 0) snprintf(buf + off, len - off, "(no recent activity)\n");
}

// Conflict management
static inline void zeta_add_conflict(zeta_code_ctx_t* ctx, zeta_conflict_type_t type,
    zeta_severity_t sev, const char* desc) {
    if (!ctx || ctx->current_review.conflict_count >= ZETA_MAX_CONFLICTS) return;
    zeta_conflict_t* c = &ctx->current_review.conflicts[ctx->current_review.conflict_count++];
    c->conflict_id = ctx->current_review.conflict_count;
    c->type = type; c->severity = sev;
    if (desc) strncpy(c->description, desc, 511);
    if (sev == SEVERITY_BLOCKER) ctx->current_review.blocker_count++;
}

static inline bool zeta_can_proceed_to_codegen(zeta_code_ctx_t* ctx, char* reason, size_t len) {
    if (!ctx) return true;
    for (int i = 0; i < ctx->current_review.conflict_count; i++) {
        if (ctx->current_review.conflicts[i].severity == SEVERITY_BLOCKER &&
            !ctx->current_review.conflicts[i].user_acknowledged) {
            snprintf(reason, len, "[BLOCKED] %s", ctx->current_review.conflicts[i].description);
            return false;
        }
    }
    return true;
}

static inline void zeta_clear_review(zeta_code_ctx_t* ctx) {
    if (ctx) memset(&ctx->current_review, 0, sizeof(zeta_conflict_review_t));
}

// =============================================================================
// CODE EXTRACTION PIPELINE (3B Coder) - Full Implementation
// =============================================================================

#define ZETA_CODER_EXTRACTION_PROMPT "<|im_start|>system\n" "Extract code entities from the input. Output JSON only.\n" "Format: {\"entities\":[{\"type\":\"function|class|variable|import\",\"name\":\"name\",\"file\":\"path\",\"line_start\":N}]}\n" "<|im_end|>\n" "<|im_start|>user\n"

// Parse JSON output into code nodes
static inline int zeta_parse_code_json(zeta_code_ctx_t* ctx, const char* json) {
    if (!ctx || !ctx->active_project || !json) return 0;
    int added = 0;
    const char* p = json;

    while ((p = strstr(p, "\"type\":")) != NULL) {
        char type_str[32] = {0}, name[128] = {0}, file[512] = {0};
        int line_start = 0;

        // Extract type
        const char* q = strchr(p + 7, '"');
        if (q) {
            const char* end = strchr(q + 1, '"');
            if (end && (end - q - 1) < 31) strncpy(type_str, q + 1, end - q - 1);
        }

        // Extract name
        const char* np = strstr(p, "\"name\":");
        if (np && np < p + 200) {
            q = strchr(np + 7, '"');
            if (q) {
                const char* end = strchr(q + 1, '"');
                if (end && (end - q - 1) < 127) strncpy(name, q + 1, end - q - 1);
            }
        }

        // Extract file
        const char* fp = strstr(p, "\"file\":");
        if (fp && fp < p + 300) {
            q = strchr(fp + 6, '"');
            if (q) {
                const char* end = strchr(q + 1, '"');
                if (end && (end - q - 1) < 511) strncpy(file, q + 1, end - q - 1);
            }
        }

        // Extract line_start
        const char* lp = strstr(p, "\"line_start\":");
        if (lp && lp < p + 400) line_start = atoi(lp + 14);

        // Map type
        zeta_code_node_type_t node_type = CODE_NODE_FUNCTION;
        if (strcmp(type_str, "class") == 0) node_type = CODE_NODE_CLASS;
        else if (strcmp(type_str, "variable") == 0) node_type = CODE_NODE_VARIABLE;
        else if (strcmp(type_str, "import") == 0) node_type = CODE_NODE_IMPORT;

        // Add node
        if (name[0] && ctx->code_node_count < ZETA_MAX_CODE_NODES) {
            zeta_code_node_t* node = &ctx->code_nodes[ctx->code_node_count++];
            memset(node, 0, sizeof(zeta_code_node_t));
            strncpy(node->project_id, ctx->active_project->project_id, 63);
            node->type = node_type;
            strncpy(node->name, name, 127);
            if (file[0]) strncpy(node->file_path, file, 511);
            node->line_start = line_start;
            node->created_at = (int64_t)time(NULL);
            added++;
            fprintf(stderr, "[CODE] Added %s: %s\n", type_str, name);
        }
        p++;
    }
    return added;
}

// Full 3B Coder extraction with inference
static inline int zeta_code_extract_entities(zeta_code_ctx_t* ctx, const char* input) {
    if (!ctx || !ctx->active_project || !input) return 0;
    if (!ctx->models.in_code_mode || !ctx->models.model_3b_coder) {
        fprintf(stderr, "[CODE] Not in code mode or no coder model\n");
        return 0;
    }

    fprintf(stderr, "[CODE] Extracting from %zu bytes\n", strlen(input));

    // Get vocab
    const llama_vocab* vocab = llama_model_get_vocab(ctx->models.model_3b_coder);
    if (!vocab) return 0;

    // Create context if needed
    if (!ctx->models.ctx_3b) {
        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 256;
        cparams.n_batch = 128;
        ctx->models.ctx_3b = llama_init_from_model(ctx->models.model_3b_coder, cparams);
        if (!ctx->models.ctx_3b) return 0;
    }

    // Build prompt
    char prompt[4096];
    snprintf(prompt, sizeof(prompt), "%s%s\n<|im_end|>\n<|im_start|>assistant\n", 
             ZETA_CODER_EXTRACTION_PROMPT, input);

    // Tokenize
    int max_tokens = 1024;
    std::vector<llama_token> tokens(max_tokens);
    int n_tokens = llama_tokenize(vocab, prompt, strlen(prompt), 
                                   tokens.data(), max_tokens, true, true);
    if (n_tokens < 0) return 0;
    tokens.resize(n_tokens);

    // Clear memory
    llama_memory_clear(llama_get_memory(ctx->models.ctx_3b), true);

    // Decode prompt
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    for (int i = 0; i < n_tokens; i++) {
        common_batch_add(batch, tokens[i], i, {0}, false);
    }
    batch.logits[batch.n_tokens - 1] = true;

    if (llama_decode(ctx->models.ctx_3b, batch) != 0) {
        llama_batch_free(batch);
        return 0;
    }

    // Generate JSON output
    std::string output;
    int n_cur = n_tokens;
    int n_vocab = llama_vocab_n_tokens(vocab);

    for (int i = 0; i < 512; i++) {
        float* logits = llama_get_logits_ith(ctx->models.ctx_3b, -1);

        // Greedy sampling
        llama_token best = 0;
        float best_logit = logits[0];
        for (int j = 1; j < n_vocab; j++) {
            if (logits[j] > best_logit) {
                best_logit = logits[j];
                best = j;
            }
        }

        if (llama_vocab_is_eog(vocab, best)) break;

        std::string piece = common_token_to_piece(vocab, best, true);
        if (piece.find("<|im_end|>") != std::string::npos) break;

        output += piece;

        // Next token
        llama_batch_free(batch);
        batch = llama_batch_init(1, 0, 1);
        common_batch_add(batch, best, n_cur++, {0}, true);
        if (llama_decode(ctx->models.ctx_3b, batch) != 0) break;
    }

    llama_batch_free(batch);
    fprintf(stderr, "[CODE] Generated %zu chars: %.100s...\n", output.size(), output.c_str());

    // Parse JSON into nodes
    return zeta_parse_code_json(ctx, output.c_str());
}

static inline void zeta_code_free(zeta_code_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->models.ctx_3b) llama_free(ctx->models.ctx_3b);
    free(ctx);
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_CODE_MODE_H
