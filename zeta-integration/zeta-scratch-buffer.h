// Z.E.T.A. Scratch Buffer: Working Memory for Staged Generation
//
// Enables generation beyond context window limits by buffering output.
// Model generates to scratch buffer, can revise/expand, then flushes to user.
//
// Key capabilities:
// - Build responses larger than context window
// - Self-revision without user seeing drafts
// - Graph operation injection mid-generation
// - Structured multi-section output assembly
//
// Memory hierarchy:
//   Parameters (14B) = Reasoning patterns
//   Graph (GitGraph) = Long-term knowledge
//   Scratch Buffer   = Working memory (this file)
//   Context Window   = Immediate attention
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_SCRATCH_BUFFER_H
#define ZETA_SCRATCH_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define ZETA_SCRATCH_DEFAULT_CAPACITY   (64 * 1024 * 1024)  // 64MB default
#define ZETA_SCRATCH_MAX_CAPACITY       (512 * 1024 * 1024) // 512MB max
#define ZETA_SCRATCH_WINDOW_SIZE        (8 * 1024)          // 8K tokens visible to model
#define ZETA_SCRATCH_MAX_CHECKPOINTS    64
#define ZETA_SCRATCH_MAX_SECTIONS       128
#define ZETA_SCRATCH_MAX_PENDING_OPS    64

// ============================================================================
// Control Tokens (added to tokenizer)
// ============================================================================

#define SCRATCH_TOK_START       "<|scratch_start|>"    // Begin hidden reasoning
#define SCRATCH_TOK_END         "<|scratch_end|>"      // End hidden, resume visible
#define SCRATCH_TOK_CHECKPOINT  "<|checkpoint|>"       // Mark revision point
#define SCRATCH_TOK_REVISE      "<|revise_from|>"      // Revert to last checkpoint
#define SCRATCH_TOK_SECTION     "<|section|>"          // Mark section boundary
#define SCRATCH_TOK_FLUSH       "<|flush|>"            // Send current buffer to user
#define SCRATCH_TOK_CLEAR       "<|clear|>"            // Clear buffer, start fresh
#define SCRATCH_TOK_INJECT      "<|inject|>"           // Injection point for graph results

// ============================================================================
// Buffer State
// ============================================================================

typedef enum {
    SCRATCH_MODE_VISIBLE,       // Output goes to user immediately
    SCRATCH_MODE_HIDDEN,        // Output buffered, hidden from user
    SCRATCH_MODE_SECTION,       // Building a section (flush on section end)
} zeta_scratch_mode_t;

typedef enum {
    SCRATCH_STATE_IDLE,         // Not generating
    SCRATCH_STATE_GENERATING,   // Actively receiving tokens
    SCRATCH_STATE_REVISING,     // In revision mode
    SCRATCH_STATE_FLUSHING,     // Sending to user
} zeta_scratch_state_t;

// ============================================================================
// Checkpoint for Revision
// ============================================================================

typedef struct {
    size_t buffer_pos;          // Position in buffer
    size_t token_count;         // Token count at checkpoint
    int64_t timestamp;          // When checkpoint was created
    char label[64];             // Optional label for debugging
    bool is_valid;              // Can we revise to this?
} zeta_scratch_checkpoint_t;

// ============================================================================
// Section for Structured Output
// ============================================================================

typedef struct {
    size_t start_pos;           // Start position in buffer
    size_t end_pos;             // End position (0 if still open)
    char name[64];              // Section name (e.g., "introduction", "code")
    int order;                  // Display order
    bool is_complete;           // Section finished?
    bool is_visible;            // Should this section go to user?
} zeta_scratch_section_t;

// ============================================================================
// Pending Graph Operation
// ============================================================================

typedef struct {
    int op_type;                // From git_op_type_t
    size_t inject_pos;          // Where to inject result
    char query[256];            // Query/key for the operation
    bool is_resolved;           // Has the op been executed?
    char result[1024];          // Result to inject
} zeta_scratch_pending_op_t;

// ============================================================================
// Main Scratch Buffer Structure
// ============================================================================

typedef struct {
    // Raw buffer
    char* data;
    size_t capacity;
    size_t length;
    
    // Cursors
    size_t output_cursor;       // What's been sent to user
    size_t model_cursor;        // What model has processed
    size_t write_cursor;        // Where next token goes
    
    // Token tracking
    int32_t* tokens;            // Token IDs in buffer
    size_t token_capacity;
    size_t token_count;
    
    // State
    zeta_scratch_mode_t mode;
    zeta_scratch_state_t state;
    
    // Checkpoints for revision
    zeta_scratch_checkpoint_t checkpoints[ZETA_SCRATCH_MAX_CHECKPOINTS];
    int num_checkpoints;
    int current_checkpoint;     // -1 if none
    
    // Sections for structured output
    zeta_scratch_section_t sections[ZETA_SCRATCH_MAX_SECTIONS];
    int num_sections;
    int current_section;        // -1 if none
    
    // Pending graph operations
    zeta_scratch_pending_op_t pending_ops[ZETA_SCRATCH_MAX_PENDING_OPS];
    int num_pending_ops;
    
    // Statistics
    size_t total_tokens_generated;
    size_t tokens_revised;
    size_t tokens_flushed;
    int revision_count;
    int64_t start_time;
    int64_t last_activity;
    
    // Configuration
    size_t window_size;         // How much context model sees
    bool auto_checkpoint;       // Create checkpoints automatically
    int auto_checkpoint_interval; // Tokens between auto checkpoints
    
} zeta_scratch_buffer_t;

// ============================================================================
// Initialization & Cleanup
// ============================================================================

static inline zeta_scratch_buffer_t* zeta_scratch_create(size_t capacity) {
    if (capacity == 0) capacity = ZETA_SCRATCH_DEFAULT_CAPACITY;
    if (capacity > ZETA_SCRATCH_MAX_CAPACITY) capacity = ZETA_SCRATCH_MAX_CAPACITY;
    
    zeta_scratch_buffer_t* buf = (zeta_scratch_buffer_t*)calloc(1, sizeof(zeta_scratch_buffer_t));
    if (!buf) return NULL;
    
    buf->data = (char*)malloc(capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    // Estimate ~4 chars per token average
    buf->token_capacity = capacity / 4;
    buf->tokens = (int32_t*)malloc(buf->token_capacity * sizeof(int32_t));
    if (!buf->tokens) {
        free(buf->data);
        free(buf);
        return NULL;
    }
    
    buf->capacity = capacity;
    buf->length = 0;
    buf->output_cursor = 0;
    buf->model_cursor = 0;
    buf->write_cursor = 0;
    buf->token_count = 0;
    
    buf->mode = SCRATCH_MODE_VISIBLE;
    buf->state = SCRATCH_STATE_IDLE;
    
    buf->num_checkpoints = 0;
    buf->current_checkpoint = -1;
    buf->num_sections = 0;
    buf->current_section = -1;
    buf->num_pending_ops = 0;
    
    buf->window_size = ZETA_SCRATCH_WINDOW_SIZE;
    buf->auto_checkpoint = true;
    buf->auto_checkpoint_interval = 256;
    
    buf->start_time = time(NULL);
    buf->last_activity = buf->start_time;
    
    return buf;
}

static inline void zeta_scratch_destroy(zeta_scratch_buffer_t* buf) {
    if (!buf) return;
    if (buf->data) free(buf->data);
    if (buf->tokens) free(buf->tokens);
    free(buf);
}

static inline void zeta_scratch_reset(zeta_scratch_buffer_t* buf) {
    if (!buf) return;
    
    buf->length = 0;
    buf->output_cursor = 0;
    buf->model_cursor = 0;
    buf->write_cursor = 0;
    buf->token_count = 0;
    
    buf->mode = SCRATCH_MODE_VISIBLE;
    buf->state = SCRATCH_STATE_IDLE;
    
    buf->num_checkpoints = 0;
    buf->current_checkpoint = -1;
    buf->num_sections = 0;
    buf->current_section = -1;
    buf->num_pending_ops = 0;
    
    buf->last_activity = time(NULL);
}

// ============================================================================
// Buffer Operations
// ============================================================================

// Append text to buffer
static inline bool zeta_scratch_append(zeta_scratch_buffer_t* buf, const char* text, size_t len) {
    if (!buf || !text || len == 0) return false;
    
    // Grow if needed
    if (buf->length + len >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap > ZETA_SCRATCH_MAX_CAPACITY) {
            // Can't grow more - need to flush or fail
            return false;
        }
        char* new_data = (char*)realloc(buf->data, new_cap);
        if (!new_data) return false;
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    
    memcpy(buf->data + buf->length, text, len);
    buf->length += len;
    buf->write_cursor = buf->length;
    buf->last_activity = time(NULL);
    
    return true;
}

// Append a single token (ID + text)
static inline bool zeta_scratch_append_token(zeta_scratch_buffer_t* buf, int32_t token_id, const char* text, size_t len) {
    if (!buf) return false;
    
    // Store token ID
    if (buf->token_count < buf->token_capacity) {
        buf->tokens[buf->token_count++] = token_id;
    }
    
    // Auto checkpoint
    if (buf->auto_checkpoint && 
        buf->token_count % buf->auto_checkpoint_interval == 0) {
        // Create checkpoint (implemented below)
    }
    
    buf->total_tokens_generated++;
    
    // Append text
    return zeta_scratch_append(buf, text, len);
}

// ============================================================================
// Checkpoint Operations
// ============================================================================

static inline int zeta_scratch_checkpoint(zeta_scratch_buffer_t* buf, const char* label) {
    if (!buf) return -1;
    if (buf->num_checkpoints >= ZETA_SCRATCH_MAX_CHECKPOINTS) {
        // Shift checkpoints (drop oldest)
        memmove(&buf->checkpoints[0], &buf->checkpoints[1], 
                (ZETA_SCRATCH_MAX_CHECKPOINTS - 1) * sizeof(zeta_scratch_checkpoint_t));
        buf->num_checkpoints--;
    }
    
    int idx = buf->num_checkpoints++;
    zeta_scratch_checkpoint_t* cp = &buf->checkpoints[idx];
    
    cp->buffer_pos = buf->length;
    cp->token_count = buf->token_count;
    cp->timestamp = time(NULL);
    cp->is_valid = true;
    
    if (label) {
        strncpy(cp->label, label, sizeof(cp->label) - 1);
    } else {
        snprintf(cp->label, sizeof(cp->label), "cp_%d", idx);
    }
    
    buf->current_checkpoint = idx;
    return idx;
}

// Revert to checkpoint
static inline bool zeta_scratch_revert(zeta_scratch_buffer_t* buf, int checkpoint_idx) {
    if (!buf) return false;
    if (checkpoint_idx < 0 || checkpoint_idx >= buf->num_checkpoints) return false;
    
    zeta_scratch_checkpoint_t* cp = &buf->checkpoints[checkpoint_idx];
    if (!cp->is_valid) return false;
    
    // Track revised tokens
    buf->tokens_revised += (buf->token_count - cp->token_count);
    buf->revision_count++;
    
    // Revert buffer state
    buf->length = cp->buffer_pos;
    buf->write_cursor = cp->buffer_pos;
    buf->token_count = cp->token_count;
    
    // Invalidate checkpoints after this one
    for (int i = checkpoint_idx + 1; i < buf->num_checkpoints; i++) {
        buf->checkpoints[i].is_valid = false;
    }
    
    buf->state = SCRATCH_STATE_REVISING;
    buf->last_activity = time(NULL);
    
    return true;
}

// Revert to last checkpoint
static inline bool zeta_scratch_revert_last(zeta_scratch_buffer_t* buf) {
    if (!buf || buf->current_checkpoint < 0) return false;
    return zeta_scratch_revert(buf, buf->current_checkpoint);
}

// ============================================================================
// Section Operations
// ============================================================================

static inline int zeta_scratch_begin_section(zeta_scratch_buffer_t* buf, const char* name, bool visible) {
    if (!buf) return -1;
    if (buf->num_sections >= ZETA_SCRATCH_MAX_SECTIONS) return -1;
    
    // Close current section if open
    if (buf->current_section >= 0) {
        buf->sections[buf->current_section].end_pos = buf->length;
        buf->sections[buf->current_section].is_complete = true;
    }
    
    int idx = buf->num_sections++;
    zeta_scratch_section_t* sec = &buf->sections[idx];
    
    sec->start_pos = buf->length;
    sec->end_pos = 0;
    sec->order = idx;
    sec->is_complete = false;
    sec->is_visible = visible;
    
    if (name) {
        strncpy(sec->name, name, sizeof(sec->name) - 1);
    } else {
        snprintf(sec->name, sizeof(sec->name), "section_%d", idx);
    }
    
    buf->current_section = idx;
    buf->mode = visible ? SCRATCH_MODE_VISIBLE : SCRATCH_MODE_HIDDEN;
    
    return idx;
}

static inline bool zeta_scratch_end_section(zeta_scratch_buffer_t* buf) {
    if (!buf || buf->current_section < 0) return false;
    
    zeta_scratch_section_t* sec = &buf->sections[buf->current_section];
    sec->end_pos = buf->length;
    sec->is_complete = true;
    
    buf->current_section = -1;
    buf->mode = SCRATCH_MODE_VISIBLE;
    
    return true;
}

// Get section content
static inline const char* zeta_scratch_get_section(zeta_scratch_buffer_t* buf, int section_idx, size_t* out_len) {
    if (!buf || section_idx < 0 || section_idx >= buf->num_sections) return NULL;
    
    zeta_scratch_section_t* sec = &buf->sections[section_idx];
    size_t end = sec->is_complete ? sec->end_pos : buf->length;
    
    if (out_len) *out_len = end - sec->start_pos;
    return buf->data + sec->start_pos;
}

// ============================================================================
// Pending Graph Operations
// ============================================================================

static inline int zeta_scratch_add_pending_op(zeta_scratch_buffer_t* buf, int op_type, const char* query) {
    if (!buf) return -1;
    if (buf->num_pending_ops >= ZETA_SCRATCH_MAX_PENDING_OPS) return -1;
    
    int idx = buf->num_pending_ops++;
    zeta_scratch_pending_op_t* op = &buf->pending_ops[idx];
    
    op->op_type = op_type;
    op->inject_pos = buf->length;  // Will inject result here
    op->is_resolved = false;
    
    if (query) {
        strncpy(op->query, query, sizeof(op->query) - 1);
    }
    
    // Add placeholder
    zeta_scratch_append(buf, SCRATCH_TOK_INJECT, strlen(SCRATCH_TOK_INJECT));
    
    return idx;
}

static inline bool zeta_scratch_resolve_op(zeta_scratch_buffer_t* buf, int op_idx, const char* result) {
    if (!buf || op_idx < 0 || op_idx >= buf->num_pending_ops) return false;
    
    zeta_scratch_pending_op_t* op = &buf->pending_ops[op_idx];
    if (op->is_resolved) return false;
    
    if (result) {
        strncpy(op->result, result, sizeof(op->result) - 1);
    }
    op->is_resolved = true;
    
    return true;
}

// ============================================================================
// Output Operations
// ============================================================================

// Get content ready for user (respects visibility)
static inline size_t zeta_scratch_get_output(zeta_scratch_buffer_t* buf, char* out, size_t out_size) {
    if (!buf || !out || out_size == 0) return 0;
    
    size_t written = 0;
    
    // Walk through sections, only include visible ones
    for (int i = 0; i < buf->num_sections && written < out_size - 1; i++) {
        zeta_scratch_section_t* sec = &buf->sections[i];
        if (!sec->is_visible || !sec->is_complete) continue;
        
        size_t sec_len = sec->end_pos - sec->start_pos;
        size_t to_copy = (sec_len < out_size - written - 1) ? sec_len : out_size - written - 1;
        
        memcpy(out + written, buf->data + sec->start_pos, to_copy);
        written += to_copy;
    }
    
    // If no sections, return all visible content
    if (buf->num_sections == 0 && buf->mode == SCRATCH_MODE_VISIBLE) {
        size_t avail = buf->length - buf->output_cursor;
        size_t to_copy = (avail < out_size - 1) ? avail : out_size - 1;
        memcpy(out, buf->data + buf->output_cursor, to_copy);
        written = to_copy;
    }
    
    out[written] = '\0';
    return written;
}

// Flush buffer to user (call this to send output)
static inline size_t zeta_scratch_flush(zeta_scratch_buffer_t* buf, char* out, size_t out_size) {
    size_t written = zeta_scratch_get_output(buf, out, out_size);
    
    if (written > 0) {
        buf->output_cursor = buf->length;
        buf->tokens_flushed += buf->token_count;
        buf->state = SCRATCH_STATE_FLUSHING;
    }
    
    return written;
}

// ============================================================================
// Context Window for Model
// ============================================================================

// Get the window of text model should see as context
static inline const char* zeta_scratch_get_model_context(zeta_scratch_buffer_t* buf, size_t* out_len) {
    if (!buf) return NULL;
    
    // Model sees last window_size worth of buffer
    size_t window_bytes = buf->window_size * 4;  // ~4 bytes per token estimate
    
    size_t start = 0;
    if (buf->length > window_bytes) {
        start = buf->length - window_bytes;
    }
    
    if (out_len) *out_len = buf->length - start;
    return buf->data + start;
}

// ============================================================================
// Mode Control
// ============================================================================

static inline void zeta_scratch_enter_hidden(zeta_scratch_buffer_t* buf) {
    if (buf) {
        buf->mode = SCRATCH_MODE_HIDDEN;
        zeta_scratch_checkpoint(buf, "hidden_start");
    }
}

static inline void zeta_scratch_exit_hidden(zeta_scratch_buffer_t* buf) {
    if (buf) {
        buf->mode = SCRATCH_MODE_VISIBLE;
    }
}

// ============================================================================
// Token Control Interface
// ============================================================================

typedef struct {
    int32_t tok_scratch_start;
    int32_t tok_scratch_end;
    int32_t tok_checkpoint;
    int32_t tok_revise;
    int32_t tok_section;
    int32_t tok_flush;
    int32_t tok_clear;
    int32_t tok_inject;
    bool initialized;
} zeta_scratch_tokens_t;

static zeta_scratch_tokens_t g_scratch_tokens = {0};

// Check if token is a scratch control token
static inline bool zeta_scratch_is_control_token(int32_t tok_id) {
    if (!g_scratch_tokens.initialized) return false;
    
    return tok_id == g_scratch_tokens.tok_scratch_start ||
           tok_id == g_scratch_tokens.tok_scratch_end ||
           tok_id == g_scratch_tokens.tok_checkpoint ||
           tok_id == g_scratch_tokens.tok_revise ||
           tok_id == g_scratch_tokens.tok_section ||
           tok_id == g_scratch_tokens.tok_flush ||
           tok_id == g_scratch_tokens.tok_clear ||
           tok_id == g_scratch_tokens.tok_inject;
}

// Handle control token (returns true if token was handled)
static inline bool zeta_scratch_handle_token(zeta_scratch_buffer_t* buf, int32_t tok_id) {
    if (!buf || !g_scratch_tokens.initialized) return false;
    
    if (tok_id == g_scratch_tokens.tok_scratch_start) {
        zeta_scratch_enter_hidden(buf);
        return true;
    }
    
    if (tok_id == g_scratch_tokens.tok_scratch_end) {
        zeta_scratch_exit_hidden(buf);
        return true;
    }
    
    if (tok_id == g_scratch_tokens.tok_checkpoint) {
        zeta_scratch_checkpoint(buf, NULL);
        return true;
    }
    
    if (tok_id == g_scratch_tokens.tok_revise) {
        zeta_scratch_revert_last(buf);
        return true;
    }
    
    if (tok_id == g_scratch_tokens.tok_clear) {
        zeta_scratch_reset(buf);
        return true;
    }
    
    // Section and flush need additional handling by caller
    return false;
}

// ============================================================================
// Statistics
// ============================================================================

typedef struct {
    size_t total_generated;
    size_t total_revised;
    size_t total_flushed;
    size_t current_length;
    int revision_count;
    int checkpoint_count;
    int section_count;
    int pending_ops;
    float revision_ratio;       // revised / generated
    int64_t duration_sec;
} zeta_scratch_stats_t;

static inline zeta_scratch_stats_t zeta_scratch_get_stats(zeta_scratch_buffer_t* buf) {
    zeta_scratch_stats_t stats = {0};
    if (!buf) return stats;
    
    stats.total_generated = buf->total_tokens_generated;
    stats.total_revised = buf->tokens_revised;
    stats.total_flushed = buf->tokens_flushed;
    stats.current_length = buf->length;
    stats.revision_count = buf->revision_count;
    stats.checkpoint_count = buf->num_checkpoints;
    stats.section_count = buf->num_sections;
    stats.pending_ops = buf->num_pending_ops;
    
    if (stats.total_generated > 0) {
        stats.revision_ratio = (float)stats.total_revised / stats.total_generated;
    }
    
    stats.duration_sec = time(NULL) - buf->start_time;
    
    return stats;
}

// ============================================================================
// Debug
// ============================================================================

static inline void zeta_scratch_debug_print(zeta_scratch_buffer_t* buf) {
    if (!buf) return;
    
    zeta_scratch_stats_t stats = zeta_scratch_get_stats(buf);
    
    fprintf(stderr, "\n=== Scratch Buffer Debug ===\n");
    fprintf(stderr, "Length: %zu / %zu bytes\n", buf->length, buf->capacity);
    fprintf(stderr, "Tokens: %zu generated, %zu revised, %zu flushed\n",
            stats.total_generated, stats.total_revised, stats.total_flushed);
    fprintf(stderr, "Revision ratio: %.2f%%\n", stats.revision_ratio * 100);
    fprintf(stderr, "Checkpoints: %d, Sections: %d\n", 
            stats.checkpoint_count, stats.section_count);
    fprintf(stderr, "Mode: %s, State: %s\n",
            buf->mode == SCRATCH_MODE_VISIBLE ? "visible" : 
            buf->mode == SCRATCH_MODE_HIDDEN ? "hidden" : "section",
            buf->state == SCRATCH_STATE_IDLE ? "idle" :
            buf->state == SCRATCH_STATE_GENERATING ? "generating" :
            buf->state == SCRATCH_STATE_REVISING ? "revising" : "flushing");
    fprintf(stderr, "============================\n\n");
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_SCRATCH_BUFFER_H
