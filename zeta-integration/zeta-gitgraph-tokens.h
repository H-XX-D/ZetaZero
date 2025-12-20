// Z.E.T.A. GitGraph Native Token Definitions
//
// Special tokens that enable the model to emit graph operations directly.
// These tokens are intercepted during generation and executed on the graph.
//
// Design principle: The model learns to "think in graphs" rather than
// having graph operations extracted externally via regex.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_GITGRAPH_TOKENS_H
#define ZETA_GITGRAPH_TOKENS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Token String Definitions
// ============================================================================

// Block delimiters
#define GIT_TOK_START           "<|git_start|>"
#define GIT_TOK_END             "<|git_end|>"

// Node operations
#define GIT_TOK_NODE            "<|git_node|>"
#define GIT_TOK_WRITE_ENTITY    "<|git_write:entity|>"
#define GIT_TOK_WRITE_FACT      "<|git_write:fact|>"
#define GIT_TOK_WRITE_PREF      "<|git_write:preference|>"
#define GIT_TOK_WRITE_HYPO      "<|git_write:hypothesis|>"

// Edge operations  
#define GIT_TOK_EDGE            "<|git_edge|>"
#define GIT_TOK_LINK_CAUSES     "<|git_link:causes|>"
#define GIT_TOK_LINK_SUPPORTS   "<|git_link:supports|>"
#define GIT_TOK_LINK_CONTRA     "<|git_link:contradicts|>"
#define GIT_TOK_LINK_TEMPORAL   "<|git_link:temporal|>"
#define GIT_TOK_LINK_BELONGS    "<|git_link:belongs_to|>"

// Query operations
#define GIT_TOK_QUERY           "<|git_query|>"
#define GIT_TOK_READ_ENTITY     "<|git_read:entity|>"
#define GIT_TOK_READ_FACT       "<|git_read:fact|>"
#define GIT_TOK_READ_RELATION   "<|git_read:relation|>"
#define GIT_TOK_READ_SESSION    "<|git_read:session|>"
#define GIT_TOK_RESULT          "<|git_result|>"

// State modification
#define GIT_TOK_CONTRADICT      "<|git_contradict|>"
#define GIT_TOK_DECAY_SLOW      "<|git_decay:slow|>"
#define GIT_TOK_DECAY_MEDIUM    "<|git_decay:medium|>"
#define GIT_TOK_DECAY_FAST      "<|git_decay:fast|>"
#define GIT_TOK_DECAY_INSTANT   "<|git_decay:instant|>"

// Hypothetical reasoning
#define GIT_TOK_HYPOTHETICAL    "<|git_hypothetical|>"
#define GIT_TOK_GROUND_CONFIRM  "<|git_ground:confirmed|>"
#define GIT_TOK_GROUND_REJECT   "<|git_ground:rejected|>"
#define GIT_TOK_GROUND_UPDATE   "<|git_ground:updated|>"

// ============================================================================
// Token ID Mapping (filled in by tokenizer extension)
// ============================================================================

typedef struct {
    int32_t tok_start;
    int32_t tok_end;
    int32_t tok_node;
    int32_t tok_edge;
    int32_t tok_query;
    int32_t tok_result;
    
    // Write operations
    int32_t tok_write_entity;
    int32_t tok_write_fact;
    int32_t tok_write_pref;
    int32_t tok_write_hypo;
    
    // Link operations
    int32_t tok_link_causes;
    int32_t tok_link_supports;
    int32_t tok_link_contra;
    int32_t tok_link_temporal;
    int32_t tok_link_belongs;
    
    // Read operations
    int32_t tok_read_entity;
    int32_t tok_read_fact;
    int32_t tok_read_relation;
    int32_t tok_read_session;
    
    // State modification
    int32_t tok_contradict;
    int32_t tok_decay_slow;
    int32_t tok_decay_medium;
    int32_t tok_decay_fast;
    int32_t tok_decay_instant;
    
    // Hypothetical
    int32_t tok_hypothetical;
    int32_t tok_ground_confirm;
    int32_t tok_ground_reject;
    int32_t tok_ground_update;
    
    bool initialized;
} git_token_ids_t;

// Global token ID cache (filled once at init)
static git_token_ids_t g_git_tokens = {0};

// ============================================================================
// Token Categories
// ============================================================================

typedef enum {
    GIT_CAT_DELIMITER,      // start, end, node, edge
    GIT_CAT_WRITE,          // write:entity, write:fact, etc.
    GIT_CAT_LINK,           // link:causes, link:supports, etc.
    GIT_CAT_READ,           // read:entity, read:fact, etc.
    GIT_CAT_DECAY,          // decay:slow, decay:fast, etc.
    GIT_CAT_HYPOTHETICAL,   // hypothetical, ground:*
    GIT_CAT_RESULT,         // result injection
    GIT_CAT_UNKNOWN
} git_token_category_t;

// ============================================================================
// Operation Types
// ============================================================================

typedef enum {
    GIT_OP_NONE = 0,
    
    // Write operations (create nodes)
    GIT_OP_WRITE_ENTITY,
    GIT_OP_WRITE_FACT,
    GIT_OP_WRITE_PREFERENCE,
    GIT_OP_WRITE_HYPOTHESIS,
    
    // Link operations (create edges)
    GIT_OP_LINK_CAUSES,
    GIT_OP_LINK_SUPPORTS,
    GIT_OP_LINK_CONTRADICTS,
    GIT_OP_LINK_TEMPORAL,
    GIT_OP_LINK_BELONGS_TO,
    
    // Read operations (query graph)
    GIT_OP_READ_ENTITY,
    GIT_OP_READ_FACT,
    GIT_OP_READ_RELATION,
    GIT_OP_READ_SESSION,
    
    // Decay operations
    GIT_OP_DECAY_SLOW,
    GIT_OP_DECAY_MEDIUM,
    GIT_OP_DECAY_FAST,
    GIT_OP_DECAY_INSTANT,
    
    // Contradiction handling
    GIT_OP_CONTRADICT,
    
    // Hypothetical reasoning
    GIT_OP_HYPOTHETICAL_START,
    GIT_OP_GROUND_CONFIRM,
    GIT_OP_GROUND_REJECT,
    GIT_OP_GROUND_UPDATE,
} git_op_type_t;

// ============================================================================
// Token Utilities
// ============================================================================

// All special token strings for tokenizer extension
static const char* GIT_ALL_TOKENS[] = {
    GIT_TOK_START,
    GIT_TOK_END,
    GIT_TOK_NODE,
    GIT_TOK_EDGE,
    GIT_TOK_QUERY,
    GIT_TOK_RESULT,
    GIT_TOK_WRITE_ENTITY,
    GIT_TOK_WRITE_FACT,
    GIT_TOK_WRITE_PREF,
    GIT_TOK_WRITE_HYPO,
    GIT_TOK_LINK_CAUSES,
    GIT_TOK_LINK_SUPPORTS,
    GIT_TOK_LINK_CONTRA,
    GIT_TOK_LINK_TEMPORAL,
    GIT_TOK_LINK_BELONGS,
    GIT_TOK_READ_ENTITY,
    GIT_TOK_READ_FACT,
    GIT_TOK_READ_RELATION,
    GIT_TOK_READ_SESSION,
    GIT_TOK_CONTRADICT,
    GIT_TOK_DECAY_SLOW,
    GIT_TOK_DECAY_MEDIUM,
    GIT_TOK_DECAY_FAST,
    GIT_TOK_DECAY_INSTANT,
    GIT_TOK_HYPOTHETICAL,
    GIT_TOK_GROUND_CONFIRM,
    GIT_TOK_GROUND_REJECT,
    GIT_TOK_GROUND_UPDATE,
    NULL  // Sentinel
};

#define GIT_NUM_TOKENS 28

// Check if a token ID is a GitGraph special token
static inline bool git_is_special_token(int32_t tok_id) {
    if (!g_git_tokens.initialized) return false;
    
    return tok_id == g_git_tokens.tok_start ||
           tok_id == g_git_tokens.tok_end ||
           tok_id == g_git_tokens.tok_node ||
           tok_id == g_git_tokens.tok_edge ||
           tok_id == g_git_tokens.tok_query ||
           tok_id == g_git_tokens.tok_result ||
           tok_id == g_git_tokens.tok_write_entity ||
           tok_id == g_git_tokens.tok_write_fact ||
           tok_id == g_git_tokens.tok_write_pref ||
           tok_id == g_git_tokens.tok_write_hypo ||
           tok_id == g_git_tokens.tok_link_causes ||
           tok_id == g_git_tokens.tok_link_supports ||
           tok_id == g_git_tokens.tok_link_contra ||
           tok_id == g_git_tokens.tok_link_temporal ||
           tok_id == g_git_tokens.tok_link_belongs ||
           tok_id == g_git_tokens.tok_read_entity ||
           tok_id == g_git_tokens.tok_read_fact ||
           tok_id == g_git_tokens.tok_read_relation ||
           tok_id == g_git_tokens.tok_read_session ||
           tok_id == g_git_tokens.tok_contradict ||
           tok_id == g_git_tokens.tok_decay_slow ||
           tok_id == g_git_tokens.tok_decay_medium ||
           tok_id == g_git_tokens.tok_decay_fast ||
           tok_id == g_git_tokens.tok_decay_instant ||
           tok_id == g_git_tokens.tok_hypothetical ||
           tok_id == g_git_tokens.tok_ground_confirm ||
           tok_id == g_git_tokens.tok_ground_reject ||
           tok_id == g_git_tokens.tok_ground_update;
}

// Get operation type from token ID
static inline git_op_type_t git_token_to_op(int32_t tok_id) {
    if (!g_git_tokens.initialized) return GIT_OP_NONE;
    
    if (tok_id == g_git_tokens.tok_write_entity) return GIT_OP_WRITE_ENTITY;
    if (tok_id == g_git_tokens.tok_write_fact) return GIT_OP_WRITE_FACT;
    if (tok_id == g_git_tokens.tok_write_pref) return GIT_OP_WRITE_PREFERENCE;
    if (tok_id == g_git_tokens.tok_write_hypo) return GIT_OP_WRITE_HYPOTHESIS;
    
    if (tok_id == g_git_tokens.tok_link_causes) return GIT_OP_LINK_CAUSES;
    if (tok_id == g_git_tokens.tok_link_supports) return GIT_OP_LINK_SUPPORTS;
    if (tok_id == g_git_tokens.tok_link_contra) return GIT_OP_LINK_CONTRADICTS;
    if (tok_id == g_git_tokens.tok_link_temporal) return GIT_OP_LINK_TEMPORAL;
    if (tok_id == g_git_tokens.tok_link_belongs) return GIT_OP_LINK_BELONGS_TO;
    
    if (tok_id == g_git_tokens.tok_read_entity) return GIT_OP_READ_ENTITY;
    if (tok_id == g_git_tokens.tok_read_fact) return GIT_OP_READ_FACT;
    if (tok_id == g_git_tokens.tok_read_relation) return GIT_OP_READ_RELATION;
    if (tok_id == g_git_tokens.tok_read_session) return GIT_OP_READ_SESSION;
    
    if (tok_id == g_git_tokens.tok_decay_slow) return GIT_OP_DECAY_SLOW;
    if (tok_id == g_git_tokens.tok_decay_medium) return GIT_OP_DECAY_MEDIUM;
    if (tok_id == g_git_tokens.tok_decay_fast) return GIT_OP_DECAY_FAST;
    if (tok_id == g_git_tokens.tok_decay_instant) return GIT_OP_DECAY_INSTANT;
    
    if (tok_id == g_git_tokens.tok_contradict) return GIT_OP_CONTRADICT;
    
    if (tok_id == g_git_tokens.tok_hypothetical) return GIT_OP_HYPOTHETICAL_START;
    if (tok_id == g_git_tokens.tok_ground_confirm) return GIT_OP_GROUND_CONFIRM;
    if (tok_id == g_git_tokens.tok_ground_reject) return GIT_OP_GROUND_REJECT;
    if (tok_id == g_git_tokens.tok_ground_update) return GIT_OP_GROUND_UPDATE;
    
    return GIT_OP_NONE;
}

// Get category from token ID
static inline git_token_category_t git_token_category(int32_t tok_id) {
    if (!g_git_tokens.initialized) return GIT_CAT_UNKNOWN;
    
    if (tok_id == g_git_tokens.tok_start ||
        tok_id == g_git_tokens.tok_end ||
        tok_id == g_git_tokens.tok_node ||
        tok_id == g_git_tokens.tok_edge ||
        tok_id == g_git_tokens.tok_query) {
        return GIT_CAT_DELIMITER;
    }
    
    if (tok_id == g_git_tokens.tok_write_entity ||
        tok_id == g_git_tokens.tok_write_fact ||
        tok_id == g_git_tokens.tok_write_pref ||
        tok_id == g_git_tokens.tok_write_hypo) {
        return GIT_CAT_WRITE;
    }
    
    if (tok_id == g_git_tokens.tok_link_causes ||
        tok_id == g_git_tokens.tok_link_supports ||
        tok_id == g_git_tokens.tok_link_contra ||
        tok_id == g_git_tokens.tok_link_temporal ||
        tok_id == g_git_tokens.tok_link_belongs) {
        return GIT_CAT_LINK;
    }
    
    if (tok_id == g_git_tokens.tok_read_entity ||
        tok_id == g_git_tokens.tok_read_fact ||
        tok_id == g_git_tokens.tok_read_relation ||
        tok_id == g_git_tokens.tok_read_session) {
        return GIT_CAT_READ;
    }
    
    if (tok_id == g_git_tokens.tok_decay_slow ||
        tok_id == g_git_tokens.tok_decay_medium ||
        tok_id == g_git_tokens.tok_decay_fast ||
        tok_id == g_git_tokens.tok_decay_instant) {
        return GIT_CAT_DECAY;
    }
    
    if (tok_id == g_git_tokens.tok_hypothetical ||
        tok_id == g_git_tokens.tok_ground_confirm ||
        tok_id == g_git_tokens.tok_ground_reject ||
        tok_id == g_git_tokens.tok_ground_update) {
        return GIT_CAT_HYPOTHETICAL;
    }
    
    if (tok_id == g_git_tokens.tok_result) {
        return GIT_CAT_RESULT;
    }
    
    return GIT_CAT_UNKNOWN;
}

// Check if token signals end of content (delimiter that needs content before it)
static inline bool git_is_content_end(int32_t tok_id) {
    return tok_id == g_git_tokens.tok_node ||
           tok_id == g_git_tokens.tok_edge ||
           tok_id == g_git_tokens.tok_query;
}

// Check if token starts a block
static inline bool git_is_block_start(int32_t tok_id) {
    return tok_id == g_git_tokens.tok_start;
}

// Check if token ends a block
static inline bool git_is_block_end(int32_t tok_id) {
    return tok_id == g_git_tokens.tok_end;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_GITGRAPH_TOKENS_H
