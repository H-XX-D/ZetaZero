// Z.E.T.A. Code Entity Extraction - 7B + 4B Parallel Pipeline
// 4B Embed (CPU): Semantic embeddings for search
// 7B Coder: KV extraction (parallel process)
// Creates typed graph nodes WITHOUT calling 14B

#ifndef ZETA_CODE_EXTRACT_H
#define ZETA_CODE_EXTRACT_H

#include "zeta-dual-process.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// CODE ENTITY TYPES (extend base graph node types)
// ============================================================================

#define CODE_ENTITY_FILE      100
#define CODE_ENTITY_FUNCTION  101
#define CODE_ENTITY_STRUCT    102
#define CODE_ENTITY_CLASS     103
#define CODE_ENTITY_ENUM      104
#define CODE_ENTITY_TYPEDEF   105
#define CODE_ENTITY_MACRO     106
#define CODE_ENTITY_GLOBAL    107

// Code relationship types
#define CODE_REL_CONTAINS     50
#define CODE_REL_CALLS        51
#define CODE_REL_INHERITS     52
#define CODE_REL_IMPLEMENTS   53
#define CODE_REL_USES         54
#define CODE_REL_RETURNS      55
#define CODE_REL_DECLARES     56
#define CODE_REL_INCLUDES     57
#define CODE_REL_MEMBER_OF    58

// Code entity structure
typedef struct zeta_code_entity {
    int64_t id;
    int type;                     // CODE_ENTITY_*
    char name[256];
    char filepath[512];
    int line_start;
    int line_end;
    char signature[1024];
    char return_type[128];
    char parent_name[256];
    char scope[64];
    float complexity;
    int is_static;
    int is_inline;
    int is_virtual;
    float embedding[768];         // 4B embed output
    int has_embedding;
} zeta_code_entity;

// Code relationship structure
typedef struct zeta_code_rel {
    int64_t src_id;
    int64_t tgt_id;
    int type;                     // CODE_REL_*
    float confidence;
    int line_ref;
    char context[256];
} zeta_code_rel;

// Extraction result
typedef struct zeta_extract_result {
    zeta_code_entity* entities;
    int num_entities;
    int max_entities;
    zeta_code_rel* relationships;
    int num_relationships;
    int max_relationships;
    int files_processed;
    int64_t extraction_time_ms;
} zeta_extract_result;

// ============================================================================
// C API
// ============================================================================

static inline zeta_extract_result* zeta_extract_result_new(int max_entities, int max_rels) {
    zeta_extract_result* r = (zeta_extract_result*)calloc(1, sizeof(zeta_extract_result));
    if (!r) return NULL;
    r->entities = (zeta_code_entity*)calloc(max_entities, sizeof(zeta_code_entity));
    r->relationships = (zeta_code_rel*)calloc(max_rels, sizeof(zeta_code_rel));
    r->max_entities = max_entities;
    r->max_relationships = max_rels;
    return r;
}

static inline void zeta_extract_result_free(zeta_extract_result* r) {
    if (r) {
        if (r->entities) free(r->entities);
        if (r->relationships) free(r->relationships);
        free(r);
    }
}

static inline const char* zeta_entity_type_str(int type) {
    switch (type) {
        case CODE_ENTITY_FILE: return "file";
        case CODE_ENTITY_FUNCTION: return "function";
        case CODE_ENTITY_STRUCT: return "struct";
        case CODE_ENTITY_CLASS: return "class";
        case CODE_ENTITY_ENUM: return "enum";
        case CODE_ENTITY_TYPEDEF: return "typedef";
        case CODE_ENTITY_MACRO: return "macro";
        case CODE_ENTITY_GLOBAL: return "global";
        default: return "unknown";
    }
}

static inline const char* zeta_rel_type_str(int type) {
    switch (type) {
        case CODE_REL_CONTAINS: return "contains";
        case CODE_REL_CALLS: return "calls";
        case CODE_REL_INHERITS: return "inherits";
        case CODE_REL_IMPLEMENTS: return "implements";
        case CODE_REL_USES: return "uses";
        case CODE_REL_RETURNS: return "returns";
        case CODE_REL_DECLARES: return "declares";
        case CODE_REL_INCLUDES: return "includes";
        case CODE_REL_MEMBER_OF: return "member_of";
        default: return "related";
    }
}

// ============================================================================
// REGEX EXTRACTION (Fast, no model needed)
// ============================================================================

static inline int zeta_extract_functions_regex(
    const char* code,
    const char* filename,
    zeta_extract_result* result
) {
    if (!code || !result) return 0;
    int added = 0;

    // Simple function pattern: type name(...)
    const char* p = code;
    int line = 1;

    while (*p) {
        // Track line numbers
        if (*p == '\n') { line++; p++; continue; }

        // Skip whitespace
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (!*p) break;

        // Look for function pattern: word word(

        // Skip modifiers
        if (strncmp(p, "static ", 7) == 0) p += 7;
        else if (strncmp(p, "inline ", 7) == 0) p += 7;
        else if (strncmp(p, "virtual ", 8) == 0) p += 8;
        else if (strncmp(p, "extern ", 7) == 0) p += 7;

        while (*p && (*p == ' ' || *p == '\t')) p++;

        // Get return type
        char ret_type[128] = {0};
        int ri = 0;
        while (*p && *p != '(' && *p != '\n' && *p != ';' && ri < 120) {
            if (*p == ' ' && ri > 0 && p[1] != '*' && p[1] != '&') {
                // Space might separate type from name
                const char* next = p + 1;
                while (*next == ' ') next++;

                // Check if next word is followed by (
                const char* paren = next;
                while (*paren && *paren != '(' && *paren != ' ' && *paren != '\n') paren++;
                while (*paren == ' ') paren++;

                if (*paren == '(') {
                    // Found function name
                    ret_type[ri] = '\0';

                    // Extract function name
                    char name[128] = {0};
                    int ni = 0;
                    p = next;
                    while (*p && *p != '(' && *p != ' ' && ni < 120) {
                        name[ni++] = *p++;
                    }
                    name[ni] = '\0';

                    // Skip common keywords
                    if (strcmp(name, "if") == 0 || strcmp(name, "while") == 0 ||
                        strcmp(name, "for") == 0 || strcmp(name, "switch") == 0 ||
                        strcmp(name, "return") == 0 || strcmp(name, "sizeof") == 0 ||
                        strcmp(name, "typedef") == 0 || strcmp(name, "struct") == 0 ||
                        strcmp(name, "class") == 0 || strcmp(name, "enum") == 0 ||
                        name[0] == '\0') {
                        break;
                    }

                    // Valid function found
                    if (result->num_entities < result->max_entities) {
                        zeta_code_entity* e = &result->entities[result->num_entities++];
                        memset(e, 0, sizeof(*e));
                        e->id = result->num_entities;
                        e->type = CODE_ENTITY_FUNCTION;
                        snprintf(e->name, sizeof(e->name), "%s", name);
                        snprintf(e->filepath, sizeof(e->filepath), "%s", filename ? filename : "");
                        snprintf(e->return_type, sizeof(e->return_type), "%s", ret_type);
                        e->line_start = line;
                        e->line_end = line;
                        added++;
                    }
                    break;
                }
            }
            ret_type[ri++] = *p++;
        }

        // Skip to end of line
        while (*p && *p != '\n') p++;
    }

    return added;
}

static inline int zeta_extract_structs_regex(
    const char* code,
    const char* filename,
    zeta_extract_result* result
) {
    if (!code || !result) return 0;
    int added = 0;

    const char* p = code;
    int line = 1;

    while (*p) {
        if (*p == '\n') { line++; p++; continue; }

        // Look for "struct Name {"
        if (strncmp(p, "struct ", 7) == 0) {
            p += 7;
            while (*p == ' ') p++;

            char name[128] = {0};
            int ni = 0;
            while (*p && *p != ' ' && *p != '{' && *p != ':' && *p != ';' && ni < 120) {
                name[ni++] = *p++;
            }
            name[ni] = '\0';

            // Check for { to confirm it's a definition
            const char* check = p;
            while (*check == ' ' || *check == ':') {
                if (*check == ':') {
                    while (*check && *check != '{' && *check != ';') check++;
                }
                check++;
            }

            if (*check == '{' && name[0] && result->num_entities < result->max_entities) {
                zeta_code_entity* e = &result->entities[result->num_entities++];
                memset(e, 0, sizeof(*e));
                e->id = result->num_entities;
                e->type = CODE_ENTITY_STRUCT;
                strncpy(e->name, name, sizeof(e->name) - 1);
                strncpy(e->filepath, filename, sizeof(e->filepath) - 1);
                e->line_start = line;
                e->line_end = line;
                added++;
            }
        }
        // Look for "class Name"
        else if (strncmp(p, "class ", 6) == 0) {
            p += 6;
            while (*p == ' ') p++;

            char name[128] = {0};
            int ni = 0;
            while (*p && *p != ' ' && *p != '{' && *p != ':' && *p != ';' && ni < 120) {
                name[ni++] = *p++;
            }
            name[ni] = '\0';

            if (name[0] && result->num_entities < result->max_entities) {
                zeta_code_entity* e = &result->entities[result->num_entities++];
                memset(e, 0, sizeof(*e));
                e->id = result->num_entities;
                e->type = CODE_ENTITY_CLASS;
                strncpy(e->name, name, sizeof(e->name) - 1);
                strncpy(e->filepath, filename, sizeof(e->filepath) - 1);
                e->line_start = line;
                e->line_end = line;
                added++;
            }
        }
        // Look for "enum Name"
        else if (strncmp(p, "enum ", 5) == 0) {
            p += 5;
            if (strncmp(p, "class ", 6) == 0) p += 6;  // enum class
            while (*p == ' ') p++;

            char name[128] = {0};
            int ni = 0;
            while (*p && *p != ' ' && *p != '{' && *p != ':' && ni < 120) {
                name[ni++] = *p++;
            }
            name[ni] = '\0';

            if (name[0] && result->num_entities < result->max_entities) {
                zeta_code_entity* e = &result->entities[result->num_entities++];
                memset(e, 0, sizeof(*e));
                e->id = result->num_entities;
                e->type = CODE_ENTITY_ENUM;
                strncpy(e->name, name, sizeof(e->name) - 1);
                strncpy(e->filepath, filename, sizeof(e->filepath) - 1);
                e->line_start = line;
                e->line_end = line;
                added++;
            }
        }

        p++;
    }

    return added;
}

// ============================================================================
// COMMIT TO GRAPH (Direct, no 14B)
// Uses 4B embed for semantic embeddings
// ============================================================================

static inline int zeta_commit_code_to_graph(
    zeta_dual_ctx_t* ctx,
    zeta_extract_result* result,
    int64_t file_node_id  // Parent file node
) {
    if (!ctx || !result) return 0;
    int committed = 0;

    // We need a stable mapping from extraction-local entity IDs (used by relationships)
    // to committed graph node IDs. We also need the *graph* file node ID to create
    // file -> entity edges; the incoming file_node_id is the extraction ID.
    int n = result->num_entities;
    if (n <= 0) return 0;
    if (n > result->max_entities) n = result->max_entities;
    if (n <= 0) return 0;
    size_t nn = (size_t)n;
    int64_t* orig_ids = (int64_t*)calloc(nn, sizeof(int64_t));
    int64_t* graph_ids = (int64_t*)calloc(nn, sizeof(int64_t));
    if (!orig_ids || !graph_ids) {
        if (orig_ids) free(orig_ids);
        if (graph_ids) free(graph_ids);
        return 0;
    }

    // Pass 1: create nodes and record mapping
    for (int i = 0; i < n; i++) {
        zeta_code_entity* e = &result->entities[i];
        orig_ids[i] = e->id;

        // Build value string with metadata
        char value[1024];
        switch (e->type) {
            case CODE_ENTITY_FILE:
                snprintf(value, sizeof(value),
                         "file:%s|lines:%d-%d",
                         e->filepath, e->line_start, e->line_end);
                break;
            case CODE_ENTITY_FUNCTION:
                snprintf(value, sizeof(value),
                         "func:%s|ret:%s|file:%s|L%d-%d",
                         e->name, e->return_type, e->filepath, e->line_start, e->line_end);
                break;
            case CODE_ENTITY_STRUCT:
                snprintf(value, sizeof(value),
                         "struct:%s|file:%s|L%d-%d",
                         e->name, e->filepath, e->line_start, e->line_end);
                break;
            case CODE_ENTITY_CLASS:
                snprintf(value, sizeof(value),
                         "class:%s|parent:%s|file:%s|L%d-%d",
                         e->name, e->parent_name, e->filepath, e->line_start, e->line_end);
                break;
            case CODE_ENTITY_ENUM:
                snprintf(value, sizeof(value),
                         "enum:%s|file:%s|L%d-%d",
                         e->name, e->filepath, e->line_start, e->line_end);
                break;
            default:
                snprintf(value, sizeof(value), "%s:%s|file:%s|L%d-%d",
                         zeta_entity_type_str(e->type), e->name, e->filepath, e->line_start, e->line_end);
        }

        // Create node directly (SOURCE_MODEL, no 14B involvement)
        // The 4B embed function will be called automatically by zeta_create_node_with_source
        int64_t node_id = zeta_create_node_with_source(
            ctx,
            NODE_ENTITY,
            zeta_entity_type_str(e->type),  // Label: "function", "struct", etc.
            value,
            0.7f,
            SOURCE_MODEL
        );

        graph_ids[i] = node_id;
        if (node_id > 0) {
            e->id = node_id;  // Update with actual graph ID
            committed++;
        }
    }

    // Resolve file graph ID from extraction-local ID
    int64_t file_graph_id = 0;
    if (file_node_id > 0) {
        for (int i = 0; i < n; i++) {
            if (orig_ids[i] == file_node_id) {
                file_graph_id = graph_ids[i];
                break;
            }
        }
    }

    // Pass 2: create file -> entity edges (now using committed graph IDs)
    if (file_graph_id > 0) {
        for (int i = 0; i < n; i++) {
            zeta_code_entity* e = &result->entities[i];
            if (e->type == CODE_ENTITY_FILE) continue;
            if (graph_ids[i] > 0) {
                zeta_create_edge(ctx, file_graph_id, graph_ids[i], EDGE_HAS, 1.0f);
            }
        }
    }

    // Pass 3: map relationship IDs and create relationship edges
    for (int i = 0; i < result->num_relationships; i++) {
        zeta_code_rel* r = &result->relationships[i];

        int64_t src_graph_id = 0;
        int64_t tgt_graph_id = 0;
        for (int j = 0; j < n; j++) {
            if (orig_ids[j] == r->src_id) src_graph_id = graph_ids[j];
            if (orig_ids[j] == r->tgt_id) tgt_graph_id = graph_ids[j];
        }

        if (src_graph_id > 0 && tgt_graph_id > 0) {
            // Update relationship IDs so the caller can rely on graph IDs post-commit.
            r->src_id = src_graph_id;
            r->tgt_id = tgt_graph_id;

            zeta_edge_type_t edge_type = EDGE_RELATED;
            switch (r->type) {
                case CODE_REL_CONTAINS:
                case CODE_REL_MEMBER_OF:
                    edge_type = EDGE_HAS;
                    break;
                case CODE_REL_INHERITS:
                case CODE_REL_IMPLEMENTS:
                    edge_type = EDGE_IS_A;
                    break;
                default:
                    edge_type = EDGE_RELATED;
            }
            zeta_create_edge(ctx, src_graph_id, tgt_graph_id, edge_type, r->confidence);
        }
    }

    free(orig_ids);
    free(graph_ids);

    fprintf(stderr, "[CODE-EXTRACT] Committed %d entities to graph\n", committed);
    return committed;
}

// ============================================================================
// FULL EXTRACTION PIPELINE
// ============================================================================

static inline zeta_extract_result* zeta_extract_code(
    const char* code,
    const char* filename
) {
    zeta_extract_result* result = zeta_extract_result_new(10000, 50000);
    if (!result) return NULL;

    int64_t start_ms = (int64_t)(clock() * 1000 / CLOCKS_PER_SEC);

    // Create file node first
    if (result->num_entities < result->max_entities) {
        zeta_code_entity* file_ent = &result->entities[result->num_entities++];
        memset(file_ent, 0, sizeof(*file_ent));
        file_ent->id = result->num_entities;
        file_ent->type = CODE_ENTITY_FILE;
        strncpy(file_ent->name, filename, sizeof(file_ent->name) - 1);
        strncpy(file_ent->filepath, filename, sizeof(file_ent->filepath) - 1);
        file_ent->line_start = 1;

        // Count lines
        int lines = 1;
        for (const char* p = code; *p; p++) if (*p == '\n') lines++;
        file_ent->line_end = lines;
    }

    // Extract entities
    int funcs = zeta_extract_functions_regex(code, filename, result);
    int types = zeta_extract_structs_regex(code, filename, result);

    int64_t end_ms = (int64_t)(clock() * 1000 / CLOCKS_PER_SEC);
    result->extraction_time_ms = end_ms - start_ms;
    result->files_processed = 1;

    fprintf(stderr, "[CODE-EXTRACT] %s: %d functions, %d types in %lldms\n",
            filename, funcs, types, (long long)result->extraction_time_ms);

    return result;
}

#endif // ZETA_CODE_EXTRACT_H
