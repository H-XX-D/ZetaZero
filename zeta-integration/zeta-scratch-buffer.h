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

// Planning Buffer (hidden reasoning)
#define SCRATCH_TOK_START       "<|scratch_start|>"    // Begin hidden reasoning
#define SCRATCH_TOK_END         "<|scratch_end|>"      // End hidden, resume visible
#define SCRATCH_TOK_CHECKPOINT  "<|checkpoint|>"       // Mark revision point
#define SCRATCH_TOK_REVISE      "<|revise_from|>"      // Revert to last checkpoint
#define SCRATCH_TOK_SECTION     "<|section|>"          // Mark section boundary
#define SCRATCH_TOK_FLUSH       "<|flush|>"            // Send current buffer to user
#define SCRATCH_TOK_CLEAR       "<|clear|>"            // Clear buffer, start fresh
#define SCRATCH_TOK_INJECT      "<|inject|>"           // Injection point for graph results

// Output Buffer (formatted deliverable)
#define OUTPUT_TOK_START        "<|output_start|>"     // Begin writing to output buffer
#define OUTPUT_TOK_END          "<|output_end|>"       // End output, finalize deliverable
#define OUTPUT_TOK_FORMAT       "<|format|>"           // Format specification marker

// Format Discovery (self-configuration)
#define FORMAT_TOK_DISCOVER     "<|format_discover|>"  // 14B determines output format
#define FORMAT_TOK_APPLY        "<|format_apply|>"     // Apply discovered format to output

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
// Output Buffer: Formatted Deliverable (Dual-Buffer Architecture)
// ============================================================================
// The Planning Buffer (scratch) holds hidden reasoning.
// The Output Buffer holds the formatted deliverable.
// 14B reasons in Planning, writes deliverable to Output.
// Format is self-discovered via FORMAT_TOK_DISCOVER.

#define ZETA_OUTPUT_DEFAULT_CAPACITY   (16 * 1024 * 1024)  // 16MB default
#define ZETA_OUTPUT_MAX_CAPACITY       (64 * 1024 * 1024)  // 64MB max
#define ZETA_FORMAT_SPEC_SIZE          4096                // Max format spec size

typedef struct {
    // Raw buffer
    char* data;
    size_t capacity;
    size_t length;

    // Write cursor
    size_t write_cursor;

    // Format specification (self-discovered by 14B)
    char format_spec[ZETA_FORMAT_SPEC_SIZE];
    size_t format_spec_len;
    bool format_locked;          // Once format is set, it's locked

    // Output state
    bool is_active;              // Currently writing to output buffer
    bool is_finalized;           // Output complete, ready to deliver

    // Statistics
    size_t total_bytes_written;
    int64_t start_time;
    int64_t finalize_time;

} zeta_output_buffer_t;

// Global output buffer instance
static zeta_output_buffer_t* g_output_buffer = NULL;

static inline zeta_output_buffer_t* zeta_output_create(size_t capacity) {
    if (capacity == 0) capacity = ZETA_OUTPUT_DEFAULT_CAPACITY;
    if (capacity > ZETA_OUTPUT_MAX_CAPACITY) capacity = ZETA_OUTPUT_MAX_CAPACITY;

    zeta_output_buffer_t* buf = (zeta_output_buffer_t*)calloc(1, sizeof(zeta_output_buffer_t));
    if (!buf) return NULL;

    buf->data = (char*)malloc(capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }

    buf->capacity = capacity;
    buf->length = 0;
    buf->write_cursor = 0;
    buf->format_spec_len = 0;
    buf->format_locked = false;
    buf->is_active = false;
    buf->is_finalized = false;
    buf->total_bytes_written = 0;
    buf->start_time = 0;
    buf->finalize_time = 0;

    return buf;
}

static inline void zeta_output_destroy(zeta_output_buffer_t* buf) {
    if (!buf) return;
    if (buf->data) free(buf->data);
    free(buf);
}

static inline void zeta_output_reset(zeta_output_buffer_t* buf) {
    if (!buf) return;
    buf->length = 0;
    buf->write_cursor = 0;
    buf->format_spec_len = 0;
    buf->format_locked = false;
    buf->is_active = false;
    buf->is_finalized = false;
    buf->start_time = 0;
    buf->finalize_time = 0;
}

// Set format specification (called after FORMAT_TOK_DISCOVER)
static inline bool zeta_output_set_format(zeta_output_buffer_t* buf, const char* format_spec, size_t len) {
    if (!buf || buf->format_locked) return false;

    if (len >= ZETA_FORMAT_SPEC_SIZE) len = ZETA_FORMAT_SPEC_SIZE - 1;
    memcpy(buf->format_spec, format_spec, len);
    buf->format_spec[len] = '\0';
    buf->format_spec_len = len;
    buf->format_locked = true;

    fprintf(stderr, "[OUTPUT] Format spec set (%zu bytes): %.100s%s\n",
            len, buf->format_spec, len > 100 ? "..." : "");
    return true;
}

// Begin writing to output buffer
static inline void zeta_output_start(zeta_output_buffer_t* buf) {
    if (!buf) return;
    buf->is_active = true;
    buf->is_finalized = false;
    buf->start_time = time(NULL);
    fprintf(stderr, "[OUTPUT] Output buffer activated\n");
}

// Append to output buffer
static inline bool zeta_output_append(zeta_output_buffer_t* buf, const char* text, size_t len) {
    if (!buf || !buf->is_active || buf->is_finalized) return false;
    if (!text || len == 0) return true;

    // Grow if needed
    if (buf->length + len >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap > ZETA_OUTPUT_MAX_CAPACITY) return false;
        char* new_data = (char*)realloc(buf->data, new_cap);
        if (!new_data) return false;
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->length, text, len);
    buf->length += len;
    buf->write_cursor = buf->length;
    buf->total_bytes_written += len;

    return true;
}

// Finalize output buffer (mark as ready for delivery)
static inline void zeta_output_finalize(zeta_output_buffer_t* buf) {
    if (!buf) return;
    buf->is_active = false;
    buf->is_finalized = true;
    buf->finalize_time = time(NULL);
    fprintf(stderr, "[OUTPUT] Output buffer finalized (%zu bytes)\n", buf->length);
}

// Get output content
static inline const char* zeta_output_get_content(zeta_output_buffer_t* buf, size_t* out_len) {
    if (!buf) return NULL;
    if (out_len) *out_len = buf->length;
    return buf->data;
}

// Check if output is ready
static inline bool zeta_output_is_ready(zeta_output_buffer_t* buf) {
    return buf && buf->is_finalized && buf->length > 0;
}

// JSON serialization for output buffer
static inline size_t zeta_output_to_json(zeta_output_buffer_t* buf, char* json, size_t json_size) {
    if (!buf || !json || json_size < 256) return 0;

    return snprintf(json, json_size,
        "{"
        "\"length\":%zu,"
        "\"capacity\":%zu,"
        "\"format_spec_len\":%zu,"
        "\"format_locked\":%s,"
        "\"is_active\":%s,"
        "\"is_finalized\":%s,"
        "\"total_bytes_written\":%zu"
        "}",
        buf->length,
        buf->capacity,
        buf->format_spec_len,
        buf->format_locked ? "true" : "false",
        buf->is_active ? "true" : "false",
        buf->is_finalized ? "true" : "false",
        buf->total_bytes_written
    );
}

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
    if (!text || len == 0) return true;  // Nothing to append, but not an error
    
    // Store token ID (if array exists)
    if (buf->tokens && buf->token_count < buf->token_capacity) {
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

// ============================================================================
// PART 2: Streaming Output Manager
// ============================================================================

// Callback for streaming tokens to user
typedef void (*zeta_stream_callback_t)(const char* text, size_t len, void* user_data);

typedef struct {
    zeta_stream_callback_t on_token;     // Called for each visible token
    zeta_stream_callback_t on_section;   // Called when section completes
    zeta_stream_callback_t on_complete;  // Called when generation done
    zeta_stream_callback_t on_revision;  // Called when revision happens (optional debug)
    void* user_data;
} zeta_scratch_stream_t;

static inline void zeta_scratch_set_stream(zeta_scratch_buffer_t* buf, zeta_scratch_stream_t* stream) {
    // Store stream callbacks - we'd need to add this field to the struct
    // For now, this is the interface
    (void)buf;
    (void)stream;
}

// Stream-aware token append - calls callback if visible
static inline bool zeta_scratch_stream_token(
    zeta_scratch_buffer_t* buf,
    int32_t token_id,
    const char* text,
    size_t len,
    zeta_scratch_stream_t* stream
) {
    if (!buf) return false;
    
    // Append to buffer
    bool ok = zeta_scratch_append_token(buf, token_id, text, len);
    if (!ok) return false;
    
    // Stream to user if visible mode
    if (stream && stream->on_token && buf->mode == SCRATCH_MODE_VISIBLE) {
        stream->on_token(text, len, stream->user_data);
    }
    
    return true;
}

// ============================================================================
// PART 3: Chunked Generation for Long Output
// ============================================================================

// When buffer exceeds context, we need to "chunk" - summarize old content
// and continue with fresh context

#define ZETA_CHUNK_SUMMARY_SIZE 2048    // Summary of previous chunk

typedef struct {
    char summary[ZETA_CHUNK_SUMMARY_SIZE];
    size_t summary_len;
    int chunk_number;
    size_t tokens_in_chunk;
    size_t total_tokens;
} zeta_chunk_state_t;

typedef struct {
    zeta_scratch_buffer_t* buffer;
    zeta_chunk_state_t chunks[32];      // Track up to 32 chunks
    int num_chunks;
    int current_chunk;
    
    // Threshold for when to create new chunk
    size_t chunk_threshold;             // Tokens before chunking
    
    // Summarizer callback (could be 3B model)
    const char* (*summarize)(const char* content, size_t len, void* ctx);
    void* summarize_ctx;
} zeta_chunked_buffer_t;

static inline zeta_chunked_buffer_t* zeta_chunked_create(size_t capacity) {
    zeta_chunked_buffer_t* cb = (zeta_chunked_buffer_t*)calloc(1, sizeof(zeta_chunked_buffer_t));
    if (!cb) return NULL;
    
    cb->buffer = zeta_scratch_create(capacity);
    if (!cb->buffer) {
        free(cb);
        return NULL;
    }
    
    cb->num_chunks = 0;
    cb->current_chunk = 0;
    cb->chunk_threshold = 4096;  // Default: chunk after 4K tokens
    cb->summarize = NULL;
    cb->summarize_ctx = NULL;
    
    return cb;
}

static inline void zeta_chunked_destroy(zeta_chunked_buffer_t* cb) {
    if (!cb) return;
    if (cb->buffer) zeta_scratch_destroy(cb->buffer);
    free(cb);
}

// Check if we need to chunk (context getting full)
static inline bool zeta_chunked_needs_chunk(zeta_chunked_buffer_t* cb) {
    if (!cb || !cb->buffer) return false;
    return cb->buffer->token_count >= cb->chunk_threshold;
}

// Create a new chunk (summarize current, start fresh)
static inline bool zeta_chunked_create_chunk(zeta_chunked_buffer_t* cb) {
    if (!cb || !cb->buffer) return false;
    if (cb->num_chunks >= 32) return false;
    
    // Get current buffer content
    size_t content_len = cb->buffer->length;
    
    // Summarize if we have a summarizer
    zeta_chunk_state_t* chunk = &cb->chunks[cb->num_chunks];
    chunk->chunk_number = cb->num_chunks;
    chunk->tokens_in_chunk = cb->buffer->token_count;
    chunk->total_tokens = cb->buffer->total_tokens_generated;
    
    if (cb->summarize) {
        const char* summary = cb->summarize(cb->buffer->data, content_len, cb->summarize_ctx);
        if (summary) {
            size_t slen = strlen(summary);
            if (slen >= ZETA_CHUNK_SUMMARY_SIZE) slen = ZETA_CHUNK_SUMMARY_SIZE - 1;
            memcpy(chunk->summary, summary, slen);
            chunk->summary[slen] = '\0';
            chunk->summary_len = slen;
        }
    } else {
        // No summarizer - just keep last N chars as "summary"
        size_t keep = ZETA_CHUNK_SUMMARY_SIZE - 64;
        if (content_len > keep) {
            memcpy(chunk->summary, cb->buffer->data + content_len - keep, keep);
            chunk->summary[keep] = '\0';
            chunk->summary_len = keep;
        }
    }
    
    cb->num_chunks++;
    cb->current_chunk = cb->num_chunks;
    
    // Reset buffer but keep stats
    size_t total_gen = cb->buffer->total_tokens_generated;
    zeta_scratch_reset(cb->buffer);
    cb->buffer->total_tokens_generated = total_gen;
    
    // Inject summary as context
    if (chunk->summary_len > 0) {
        zeta_scratch_append(cb->buffer, "[Previous context summary: ", 27);
        zeta_scratch_append(cb->buffer, chunk->summary, chunk->summary_len);
        zeta_scratch_append(cb->buffer, "]\n\n", 3);
    }
    
    return true;
}

// Get total content across all chunks (for final assembly)
static inline size_t zeta_chunked_get_total_length(zeta_chunked_buffer_t* cb) {
    if (!cb) return 0;
    
    size_t total = cb->buffer->length;
    for (int i = 0; i < cb->num_chunks; i++) {
        total += cb->chunks[i].tokens_in_chunk * 4;  // Estimate
    }
    return total;
}

// ============================================================================
// PART 4: Revision Strategies
// ============================================================================

typedef enum {
    REVISION_NONE,              // No revision needed
    REVISION_MINOR,             // Small fix, revert 1 checkpoint
    REVISION_MAJOR,             // Big change, revert multiple
    REVISION_RESTART,           // Start over from beginning
} zeta_revision_level_t;

typedef struct {
    float confidence_threshold;  // Below this, consider revision
    int max_revisions;           // Max revisions before giving up
    bool allow_restart;          // Can we restart from scratch?
    int lookback_checkpoints;    // How many checkpoints to consider
} zeta_revision_config_t;

static inline zeta_revision_config_t zeta_revision_default_config(void) {
    return (zeta_revision_config_t){
        .confidence_threshold = 0.3f,
        .max_revisions = 5,
        .allow_restart = true,
        .lookback_checkpoints = 3
    };
}

// Determine revision level based on confidence score
static inline zeta_revision_level_t zeta_revision_evaluate(
    zeta_scratch_buffer_t* buf,
    float confidence,
    zeta_revision_config_t* config
) {
    if (!buf || !config) return REVISION_NONE;
    
    // Already revised too many times?
    if (buf->revision_count >= config->max_revisions) {
        return REVISION_NONE;  // Give up on revising
    }
    
    // Confidence ok?
    if (confidence >= config->confidence_threshold) {
        return REVISION_NONE;
    }
    
    // How bad is it?
    if (confidence < 0.1f && config->allow_restart) {
        return REVISION_RESTART;
    }
    
    if (confidence < 0.2f && buf->num_checkpoints > 1) {
        return REVISION_MAJOR;
    }
    
    return REVISION_MINOR;
}

// Execute revision based on level
static inline bool zeta_revision_execute(
    zeta_scratch_buffer_t* buf,
    zeta_revision_level_t level,
    zeta_revision_config_t* config
) {
    if (!buf || !config) return false;
    
    switch (level) {
        case REVISION_NONE:
            return true;  // Nothing to do
            
        case REVISION_MINOR:
            return zeta_scratch_revert_last(buf);
            
        case REVISION_MAJOR: {
            // Go back multiple checkpoints
            int target = buf->current_checkpoint - config->lookback_checkpoints;
            if (target < 0) target = 0;
            return zeta_scratch_revert(buf, target);
        }
        
        case REVISION_RESTART:
            zeta_scratch_reset(buf);
            return true;
    }
    
    return false;
}

// ============================================================================
// PART 5: Section Templates for Structured Output
// ============================================================================

typedef struct {
    const char* name;
    bool required;
    bool visible;
    int min_tokens;
    int max_tokens;
    const char* prompt_hint;    // Hint to model for this section
} zeta_section_template_t;

// Common templates
static const zeta_section_template_t SECTION_THINKING = {
    .name = "thinking",
    .required = false,
    .visible = false,           // Hidden from user
    .min_tokens = 0,
    .max_tokens = 2048,
    .prompt_hint = "Think through the problem step by step."
};

static const zeta_section_template_t SECTION_PLANNING = {
    .name = "planning",
    .required = false,
    .visible = false,
    .min_tokens = 0,
    .max_tokens = 1024,
    .prompt_hint = "Outline your approach."
};

static const zeta_section_template_t SECTION_RESPONSE = {
    .name = "response",
    .required = true,
    .visible = true,            // This goes to user
    .min_tokens = 10,
    .max_tokens = 8192,
    .prompt_hint = NULL
};

static const zeta_section_template_t SECTION_CODE = {
    .name = "code",
    .required = false,
    .visible = true,
    .min_tokens = 0,
    .max_tokens = 16384,
    .prompt_hint = "Write the code implementation."
};

static const zeta_section_template_t SECTION_REVIEW = {
    .name = "review",
    .required = false,
    .visible = false,
    .min_tokens = 0,
    .max_tokens = 1024,
    .prompt_hint = "Review your response for errors."
};

// Template-based section creation
static inline int zeta_scratch_begin_template_section(
    zeta_scratch_buffer_t* buf,
    const zeta_section_template_t* tmpl
) {
    if (!buf || !tmpl) return -1;
    return zeta_scratch_begin_section(buf, tmpl->name, tmpl->visible);
}

// ============================================================================
// PART 6: Generation Loop Integration
// ============================================================================

// Forward declaration - would be implemented with llama.cpp types
struct llama_context;
struct llama_model;

typedef struct {
    zeta_scratch_buffer_t* scratch;
    zeta_chunked_buffer_t* chunked;     // Optional chunked mode
    zeta_scratch_stream_t* stream;
    zeta_revision_config_t revision_cfg;
    
    // Generation state
    bool is_generating;
    bool use_chunking;
    int current_section_template;
    
    // Token tracking
    int32_t last_token;
    float last_confidence;
    
    // GitGraph integration (would link to actual GitGraph)
    void* gitgraph_ctx;
    
} zeta_generation_ctx_t;

static inline zeta_generation_ctx_t* zeta_generation_create(bool use_chunking) {
    zeta_generation_ctx_t* ctx = (zeta_generation_ctx_t*)calloc(1, sizeof(zeta_generation_ctx_t));
    if (!ctx) return NULL;
    
    if (use_chunking) {
        ctx->chunked = zeta_chunked_create(0);
        ctx->scratch = ctx->chunked ? ctx->chunked->buffer : NULL;
    } else {
        ctx->scratch = zeta_scratch_create(0);
    }
    
    if (!ctx->scratch) {
        free(ctx);
        return NULL;
    }
    
    ctx->revision_cfg = zeta_revision_default_config();
    ctx->use_chunking = use_chunking;
    ctx->is_generating = false;
    
    return ctx;
}

static inline void zeta_generation_destroy(zeta_generation_ctx_t* ctx) {
    if (!ctx) return;
    
    if (ctx->use_chunking && ctx->chunked) {
        zeta_chunked_destroy(ctx->chunked);
    } else if (ctx->scratch) {
        zeta_scratch_destroy(ctx->scratch);
    }
    
    free(ctx);
}

// Main token processing function - called for each generated token
typedef enum {
    TOKEN_RESULT_CONTINUE,      // Keep generating
    TOKEN_RESULT_REVISE,        // Trigger revision
    TOKEN_RESULT_CHUNK,         // Create new chunk
    TOKEN_RESULT_DONE,          // Generation complete
    TOKEN_RESULT_ERROR,         // Something went wrong
} zeta_token_result_t;

static inline zeta_token_result_t zeta_generation_process_token(
    zeta_generation_ctx_t* ctx,
    int32_t token_id,
    const char* token_text,
    size_t token_len,
    float confidence         // Model's confidence in this token
) {
    if (!ctx || !ctx->scratch) return TOKEN_RESULT_ERROR;
    
    ctx->last_token = token_id;
    ctx->last_confidence = confidence;
    
    // Check for control tokens
    if (zeta_scratch_is_control_token(token_id)) {
        zeta_scratch_handle_token(ctx->scratch, token_id);
        return TOKEN_RESULT_CONTINUE;
    }
    
    // Check for EOS
    // (Would check against actual EOS token ID)
    
    // Append token
    if (ctx->stream) {
        zeta_scratch_stream_token(ctx->scratch, token_id, token_text, token_len, ctx->stream);
    } else {
        zeta_scratch_append_token(ctx->scratch, token_id, token_text, token_len);
    }
    
    // Check if revision needed
    zeta_revision_level_t rev_level = zeta_revision_evaluate(
        ctx->scratch, confidence, &ctx->revision_cfg
    );
    
    if (rev_level != REVISION_NONE) {
        zeta_revision_execute(ctx->scratch, rev_level, &ctx->revision_cfg);
        return TOKEN_RESULT_REVISE;
    }
    
    // Check if chunking needed
    if (ctx->use_chunking && ctx->chunked) {
        if (zeta_chunked_needs_chunk(ctx->chunked)) {
            zeta_chunked_create_chunk(ctx->chunked);
            return TOKEN_RESULT_CHUNK;
        }
    }
    
    return TOKEN_RESULT_CONTINUE;
}

// Start generation with sections
static inline void zeta_generation_start(
    zeta_generation_ctx_t* ctx,
    const zeta_section_template_t* sections[],
    int num_sections
) {
    if (!ctx || !ctx->scratch) return;
    
    ctx->is_generating = true;
    ctx->scratch->state = SCRATCH_STATE_GENERATING;
    
    // Start first section if provided
    if (sections && num_sections > 0) {
        zeta_scratch_begin_template_section(ctx->scratch, sections[0]);
        ctx->current_section_template = 0;
    }
}

// End generation and get final output
static inline size_t zeta_generation_finish(
    zeta_generation_ctx_t* ctx,
    char* output,
    size_t output_size
) {
    if (!ctx || !ctx->scratch) return 0;
    
    ctx->is_generating = false;
    
    // Close any open section
    if (ctx->scratch->current_section >= 0) {
        zeta_scratch_end_section(ctx->scratch);
    }
    
    // Flush to output
    return zeta_scratch_flush(ctx->scratch, output, output_size);
}

// ============================================================================
// PART 7: Parallel Buffer (for speculative generation)
// ============================================================================

// Run two generation paths in parallel, pick better one
typedef struct {
    zeta_scratch_buffer_t* primary;
    zeta_scratch_buffer_t* speculative;
    bool spec_active;
    float spec_threshold;       // Confidence below this triggers speculation
    int spec_tokens;            // How many tokens to speculate
} zeta_parallel_buffer_t;

static inline zeta_parallel_buffer_t* zeta_parallel_create(void) {
    zeta_parallel_buffer_t* pb = (zeta_parallel_buffer_t*)calloc(1, sizeof(zeta_parallel_buffer_t));
    if (!pb) return NULL;
    
    pb->primary = zeta_scratch_create(0);
    pb->speculative = zeta_scratch_create(ZETA_SCRATCH_DEFAULT_CAPACITY / 4);  // Smaller
    
    if (!pb->primary || !pb->speculative) {
        if (pb->primary) zeta_scratch_destroy(pb->primary);
        if (pb->speculative) zeta_scratch_destroy(pb->speculative);
        free(pb);
        return NULL;
    }
    
    pb->spec_active = false;
    pb->spec_threshold = 0.5f;
    pb->spec_tokens = 32;
    
    return pb;
}

static inline void zeta_parallel_destroy(zeta_parallel_buffer_t* pb) {
    if (!pb) return;
    if (pb->primary) zeta_scratch_destroy(pb->primary);
    if (pb->speculative) zeta_scratch_destroy(pb->speculative);
    free(pb);
}

// Start speculative path
static inline void zeta_parallel_start_speculation(zeta_parallel_buffer_t* pb) {
    if (!pb) return;
    
    // Copy current primary state to speculative
    if (pb->primary->length > 0) {
        zeta_scratch_reset(pb->speculative);
        zeta_scratch_append(pb->speculative, pb->primary->data, pb->primary->length);
    }
    
    pb->spec_active = true;
}

// Accept speculative path (it was better)
static inline void zeta_parallel_accept_speculation(zeta_parallel_buffer_t* pb) {
    if (!pb || !pb->spec_active) return;
    
    // Swap buffers
    zeta_scratch_buffer_t* tmp = pb->primary;
    pb->primary = pb->speculative;
    pb->speculative = tmp;
    
    pb->spec_active = false;
}

// Reject speculative path (primary was better)
static inline void zeta_parallel_reject_speculation(zeta_parallel_buffer_t* pb) {
    if (!pb) return;
    
    zeta_scratch_reset(pb->speculative);
    pb->spec_active = false;
}

// ============================================================================
// PART 8: JSON Serialization for Persistence
// ============================================================================

// Serialize buffer state to JSON (for saving/restoring sessions)
static inline size_t zeta_scratch_to_json(zeta_scratch_buffer_t* buf, char* json, size_t json_size) {
    if (!buf || !json || json_size < 256) return 0;
    
    zeta_scratch_stats_t stats = zeta_scratch_get_stats(buf);
    
    int written = snprintf(json, json_size,
        "{"
        "\"length\":%zu,"
        "\"capacity\":%zu,"
        "\"tokens_generated\":%zu,"
        "\"tokens_revised\":%zu,"
        "\"tokens_flushed\":%zu,"
        "\"revision_count\":%d,"
        "\"checkpoints\":%d,"
        "\"sections\":%d,"
        "\"revision_ratio\":%.4f,"
        "\"duration\":%lld,"
        "\"mode\":\"%s\","
        "\"state\":\"%s\""
        "}",
        buf->length,
        buf->capacity,
        stats.total_generated,
        stats.total_revised,
        stats.total_flushed,
        stats.revision_count,
        stats.checkpoint_count,
        stats.section_count,
        stats.revision_ratio,
        (long long)stats.duration_sec,
        buf->mode == SCRATCH_MODE_VISIBLE ? "visible" : 
        buf->mode == SCRATCH_MODE_HIDDEN ? "hidden" : "section",
        buf->state == SCRATCH_STATE_IDLE ? "idle" :
        buf->state == SCRATCH_STATE_GENERATING ? "generating" :
        buf->state == SCRATCH_STATE_REVISING ? "revising" : "flushing"
    );
    
    return (size_t)written;
}

// ============================================================================
// PART 9: Dual-Buffer Context (combines Planning + Output)
// ============================================================================
// This section must be AFTER all scratch buffer functions are defined.
// Planning Buffer (scratch) = hidden reasoning
// Output Buffer = formatted deliverable for test/benchmark

typedef struct {
    zeta_scratch_buffer_t* planning;   // Hidden reasoning (existing scratch)
    zeta_output_buffer_t* output;      // Formatted deliverable

    // Current mode
    bool in_planning;                  // Currently writing to planning buffer
    bool in_output;                    // Currently writing to output buffer
    bool format_discovered;            // 14B has discovered the format

    // Generation state
    bool is_generating;
    int64_t start_time;

} zeta_dual_buffer_ctx_t;

static inline zeta_dual_buffer_ctx_t* zeta_dual_buffer_create(void) {
    zeta_dual_buffer_ctx_t* ctx = (zeta_dual_buffer_ctx_t*)calloc(1, sizeof(zeta_dual_buffer_ctx_t));
    if (!ctx) return NULL;

    ctx->planning = zeta_scratch_create(0);
    ctx->output = zeta_output_create(0);

    if (!ctx->planning || !ctx->output) {
        if (ctx->planning) zeta_scratch_destroy(ctx->planning);
        if (ctx->output) zeta_output_destroy(ctx->output);
        free(ctx);
        return NULL;
    }

    ctx->in_planning = false;
    ctx->in_output = false;
    ctx->format_discovered = false;
    ctx->is_generating = false;

    return ctx;
}

static inline void zeta_dual_buffer_destroy(zeta_dual_buffer_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->planning) zeta_scratch_destroy(ctx->planning);
    if (ctx->output) zeta_output_destroy(ctx->output);
    free(ctx);
}

static inline void zeta_dual_buffer_reset(zeta_dual_buffer_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->planning) zeta_scratch_reset(ctx->planning);
    if (ctx->output) zeta_output_reset(ctx->output);
    ctx->in_planning = false;
    ctx->in_output = false;
    ctx->format_discovered = false;
    ctx->is_generating = false;
}

// Enter planning mode (hidden reasoning)
static inline void zeta_dual_buffer_enter_planning(zeta_dual_buffer_ctx_t* ctx) {
    if (!ctx) return;
    ctx->in_planning = true;
    ctx->in_output = false;
    if (ctx->planning) zeta_scratch_enter_hidden(ctx->planning);
}

// Exit planning, enter output mode
static inline void zeta_dual_buffer_enter_output(zeta_dual_buffer_ctx_t* ctx) {
    if (!ctx) return;
    ctx->in_planning = false;
    ctx->in_output = true;
    if (ctx->planning) zeta_scratch_exit_hidden(ctx->planning);
    if (ctx->output) zeta_output_start(ctx->output);
}

// Append to current buffer
static inline bool zeta_dual_buffer_append(zeta_dual_buffer_ctx_t* ctx, const char* text, size_t len) {
    if (!ctx) return false;

    if (ctx->in_output && ctx->output) {
        return zeta_output_append(ctx->output, text, len);
    } else if (ctx->planning) {
        return zeta_scratch_append(ctx->planning, text, len);
    }
    return false;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_SCRATCH_BUFFER_H
