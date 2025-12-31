// Z.E.T.A. Story Coherence Integration
// Graph-backed fact consistency for long-form narrative generation
//
// Problem: LLMs generating 8K+ word stories lose track of:
//   - Character names (Dr. Evelyn Carter vs Dr. Elara Myles)
//   - Established facts (location, relationships, plot points)
//   - Temporal consistency (what happened before what)
//
// Solution: Extract story elements during planning, store in graph,
// surface relevant facts before each generation chunk.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_STORY_INTEGRATION_H
#define ZETA_STORY_INTEGRATION_H

#include "zeta-dual-process.h"
#include "zeta-graph-git.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// STORY ELEMENT TYPES (using graph node labels)
// =============================================================================

// Story node labels (stored in node->label)
#define STORY_LABEL_CHARACTER    "story_character"
#define STORY_LABEL_LOCATION     "story_location"
#define STORY_LABEL_OBJECT       "story_object"
#define STORY_LABEL_EVENT        "story_event"
#define STORY_LABEL_PLOT_POINT   "story_plot"
#define STORY_LABEL_CHAPTER      "story_chapter"
#define STORY_LABEL_RELATIONSHIP "story_relationship"

// Story edge types (reuse existing + new semantic meanings)
// EDGE_RELATED    = general relationship
// EDGE_CAUSES     = plot causation
// EDGE_TEMPORAL   = time sequence
// EDGE_HAS        = character has attribute/object
// EDGE_IS_A       = type hierarchy

// =============================================================================
// STORY CONTEXT STATE
// =============================================================================

typedef struct {
    // Currently active story
    char story_title[256];
    char story_genre[64];
    int64_t story_branch_id;        // Git branch for this story

    // Chapter tracking
    int current_chapter;
    int total_chapters;
    char chapter_titles[32][128];   // Up to 32 chapters
    int64_t chapter_node_ids[32];   // Node IDs for chapter markers

    // Character registry (for quick lookup)
    struct {
        char name[128];
        char role[64];              // "protagonist", "antagonist", "supporting"
        char traits[256];           // Key traits for consistency
        int64_t node_id;
        int introduced_in_chapter;
    } characters[64];
    int num_characters;

    // Location registry
    struct {
        char name[128];
        char description[256];
        int64_t node_id;
    } locations[32];
    int num_locations;

    // Active context for current generation
    char active_context[8192];      // Facts to inject before generation
    int context_token_count;

    // Statistics
    int facts_surfaced;
    int contradictions_prevented;
} zeta_story_ctx_t;

// Global story context
static zeta_story_ctx_t* g_story_ctx = NULL;

// =============================================================================
// INITIALIZATION
// =============================================================================

static inline zeta_story_ctx_t* zeta_story_init(zeta_git_ctx_t* git_ctx, const char* title) {
    if (g_story_ctx) {
        fprintf(stderr, "[STORY] Warning: Reinitializing story context\n");
        free(g_story_ctx);
    }

    g_story_ctx = (zeta_story_ctx_t*)calloc(1, sizeof(zeta_story_ctx_t));
    if (!g_story_ctx) return NULL;

    if (title) {
        strncpy(g_story_ctx->story_title, title, sizeof(g_story_ctx->story_title) - 1);
    }

    // Create a git branch for this story
    if (git_ctx) {
        char branch_name[256];
        snprintf(branch_name, sizeof(branch_name), "story/%s_%lld",
                 title ? title : "untitled", (long long)time(NULL));

        int branch_idx = zeta_git_branch(git_ctx, branch_name);
        if (branch_idx >= 0) {
            g_story_ctx->story_branch_id = branch_idx;
            zeta_git_checkout(git_ctx, branch_name);
            fprintf(stderr, "[STORY] Created branch: %s\n", branch_name);
        }
    }

    g_story_ctx->current_chapter = 0;
    fprintf(stderr, "[STORY] Initialized story context: %s\n", title ? title : "untitled");

    return g_story_ctx;
}

static inline void zeta_story_free(void) {
    if (g_story_ctx) {
        free(g_story_ctx);
        g_story_ctx = NULL;
    }
}

// =============================================================================
// CHARACTER EXTRACTION
// =============================================================================

// Extract character mentions from text
// Patterns: "Dr. X", "Professor Y", "named Z", character names in quotes
static inline int zeta_story_extract_characters(
    zeta_dual_ctx_t* graph,
    const char* text,
    int chapter_num
) {
    if (!graph || !text || !g_story_ctx) return 0;

    int extracted = 0;
    const char* p = text;

    // Title patterns that indicate character names
    const char* title_patterns[] = {
        "Dr. ", "Doctor ", "Professor ", "Prof. ",
        "Captain ", "Commander ", "General ", "Admiral ",
        "King ", "Queen ", "Prince ", "Princess ",
        "Mr. ", "Mrs. ", "Ms. ", "Miss ",
        "Lord ", "Lady ", "Sir ", "Dame ",
        NULL
    };

    // Check for titled characters
    for (int i = 0; title_patterns[i]; i++) {
        const char* match = text;
        while ((match = strstr(match, title_patterns[i])) != NULL) {
            const char* name_start = match + strlen(title_patterns[i]);
            char full_name[128] = {0};
            int ni = 0;

            // Copy title
            strncpy(full_name, match, strlen(title_patterns[i]));
            ni = strlen(title_patterns[i]);

            // Extract name (capitalized words)
            while (*name_start && ni < 127) {
                if (*name_start >= 'A' && *name_start <= 'Z') {
                    // Start of name word
                    while (*name_start && ni < 127 &&
                           ((*name_start >= 'a' && *name_start <= 'z') ||
                            (*name_start >= 'A' && *name_start <= 'Z') ||
                            *name_start == '-' || *name_start == '\'')) {
                        full_name[ni++] = *name_start++;
                    }
                    if (*name_start == ' ' &&
                        *(name_start+1) >= 'A' && *(name_start+1) <= 'Z') {
                        full_name[ni++] = ' ';
                        name_start++;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            full_name[ni] = '\0';

            if (ni > strlen(title_patterns[i]) + 2) {
                // Check if already registered
                bool exists = false;
                for (int c = 0; c < g_story_ctx->num_characters; c++) {
                    if (strcasecmp(g_story_ctx->characters[c].name, full_name) == 0) {
                        exists = true;
                        break;
                    }
                }

                if (!exists && g_story_ctx->num_characters < 64) {
                    // Store in registry
                    int idx = g_story_ctx->num_characters;
                    strncpy(g_story_ctx->characters[idx].name, full_name, 127);
                    g_story_ctx->characters[idx].introduced_in_chapter = chapter_num;

                    // Determine role based on first mention context
                    if (chapter_num == 1) {
                        strcpy(g_story_ctx->characters[idx].role, "protagonist");
                    } else {
                        strcpy(g_story_ctx->characters[idx].role, "supporting");
                    }

                    // Store in graph
                    int64_t node_id = zeta_commit_fact(graph, NODE_ENTITY,
                        STORY_LABEL_CHARACTER, full_name, 0.95f, SOURCE_MODEL);
                    g_story_ctx->characters[idx].node_id = node_id;

                    g_story_ctx->num_characters++;
                    extracted++;

                    fprintf(stderr, "[STORY] Character: %s (ch%d)\n", full_name, chapter_num);
                }
            }

            match++;
        }
    }

    // Check for "named X" pattern
    const char* named = text;
    while ((named = strstr(named, " named ")) != NULL) {
        named += 7;  // Skip " named "

        char name[128] = {0};
        int ni = 0;

        // Extract name (capitalized)
        while (*named && ni < 127 &&
               ((*named >= 'a' && *named <= 'z') ||
                (*named >= 'A' && *named <= 'Z') ||
                *named == ' ' || *named == '-')) {
            name[ni++] = *named++;
            if (*named == ' ' &&
                !(*(named+1) >= 'A' && *(named+1) <= 'Z')) {
                break;
            }
        }
        while (ni > 0 && name[ni-1] == ' ') ni--;
        name[ni] = '\0';

        if (ni > 2) {
            bool exists = false;
            for (int c = 0; c < g_story_ctx->num_characters; c++) {
                if (strcasecmp(g_story_ctx->characters[c].name, name) == 0) {
                    exists = true;
                    break;
                }
            }

            if (!exists && g_story_ctx->num_characters < 64) {
                int idx = g_story_ctx->num_characters;
                strncpy(g_story_ctx->characters[idx].name, name, 127);
                g_story_ctx->characters[idx].introduced_in_chapter = chapter_num;
                strcpy(g_story_ctx->characters[idx].role, "supporting");

                int64_t node_id = zeta_commit_fact(graph, NODE_ENTITY,
                    STORY_LABEL_CHARACTER, name, 0.9f, SOURCE_MODEL);
                g_story_ctx->characters[idx].node_id = node_id;

                g_story_ctx->num_characters++;
                extracted++;

                fprintf(stderr, "[STORY] Character (named): %s (ch%d)\n", name, chapter_num);
            }
        }
    }

    return extracted;
}

// =============================================================================
// LOCATION EXTRACTION
// =============================================================================

static inline int zeta_story_extract_locations(
    zeta_dual_ctx_t* graph,
    const char* text
) {
    if (!graph || !text || !g_story_ctx) return 0;

    int extracted = 0;

    // Location indicator patterns
    const char* location_patterns[] = {
        " in the ", " at the ", " on the ", " inside the ",
        " within the ", " aboard the ", " near the ",
        " called the ", " known as the ",
        NULL
    };

    // Check for location mentions
    for (int i = 0; location_patterns[i]; i++) {
        const char* match = text;
        while ((match = strstr(match, location_patterns[i])) != NULL) {
            const char* loc_start = match + strlen(location_patterns[i]);
            char location[128] = {0};
            int li = 0;

            // Extract location (capitalized words or quoted)
            if (*loc_start == '"') {
                loc_start++;
                while (*loc_start && *loc_start != '"' && li < 127) {
                    location[li++] = *loc_start++;
                }
            } else if (*loc_start >= 'A' && *loc_start <= 'Z') {
                // Capitalized location name
                while (*loc_start && li < 127 &&
                       ((*loc_start >= 'a' && *loc_start <= 'z') ||
                        (*loc_start >= 'A' && *loc_start <= 'Z') ||
                        *loc_start == ' ' || *loc_start == '-')) {
                    location[li++] = *loc_start++;
                    if (*loc_start == ' ' &&
                        !(*(loc_start+1) >= 'A' && *(loc_start+1) <= 'Z') &&
                        !(*(loc_start+1) >= 'a' && *(loc_start+1) <= 'z')) {
                        break;
                    }
                }
            }
            while (li > 0 && location[li-1] == ' ') li--;
            location[li] = '\0';

            if (li > 3) {
                // Check if already registered
                bool exists = false;
                for (int l = 0; l < g_story_ctx->num_locations; l++) {
                    if (strcasecmp(g_story_ctx->locations[l].name, location) == 0) {
                        exists = true;
                        break;
                    }
                }

                if (!exists && g_story_ctx->num_locations < 32) {
                    int idx = g_story_ctx->num_locations;
                    strncpy(g_story_ctx->locations[idx].name, location, 127);

                    int64_t node_id = zeta_commit_fact(graph, NODE_ENTITY,
                        STORY_LABEL_LOCATION, location, 0.85f, SOURCE_MODEL);
                    g_story_ctx->locations[idx].node_id = node_id;

                    g_story_ctx->num_locations++;
                    extracted++;

                    fprintf(stderr, "[STORY] Location: %s\n", location);
                }
            }

            match++;
        }
    }

    return extracted;
}

// =============================================================================
// PLOT POINT EXTRACTION
// =============================================================================

static inline int zeta_story_extract_plot_points(
    zeta_dual_ctx_t* graph,
    const char* text,
    int chapter_num
) {
    if (!graph || !text || !g_story_ctx) return 0;

    int extracted = 0;

    // Key plot indicators
    const char* plot_patterns[] = {
        " discovered ", " realized ", " learned ",
        " died", " was killed", " was destroyed",
        " married ", " fell in love",
        " betrayed ", " revealed ",
        " won ", " defeated ", " lost ",
        " transformed ", " became ",
        " escaped ", " captured ",
        " created ", " built ",
        NULL
    };

    for (int i = 0; plot_patterns[i]; i++) {
        const char* match = strstr(text, plot_patterns[i]);
        if (match) {
            // Extract context around the plot point
            char context[256] = {0};
            const char* ctx_start = match - 50;
            if (ctx_start < text) ctx_start = text;

            // Find sentence start
            while (ctx_start > text && *ctx_start != '.' && *ctx_start != '\n') {
                ctx_start--;
            }
            if (*ctx_start == '.' || *ctx_start == '\n') ctx_start++;
            while (*ctx_start == ' ') ctx_start++;

            // Copy up to sentence end
            int ci = 0;
            while (*ctx_start && ci < 255 && *ctx_start != '.' && *ctx_start != '\n') {
                context[ci++] = *ctx_start++;
            }
            if (*ctx_start == '.') context[ci++] = '.';
            context[ci] = '\0';

            if (ci > 10) {
                char label[64];
                snprintf(label, sizeof(label), "plot_ch%d", chapter_num);

                int64_t node_id = zeta_commit_fact(graph, NODE_EVENT,
                    STORY_LABEL_PLOT_POINT, context, 0.9f, SOURCE_MODEL);

                // Create temporal edge to chapter
                if (chapter_num > 0 && chapter_num <= 32 &&
                    g_story_ctx->chapter_node_ids[chapter_num-1] > 0) {
                    zeta_commit_edge(graph, node_id,
                        g_story_ctx->chapter_node_ids[chapter_num-1],
                        EDGE_TEMPORAL, 1.0f);
                }

                extracted++;
                fprintf(stderr, "[STORY] Plot: %.60s... (ch%d)\n", context, chapter_num);
            }
        }
    }

    return extracted;
}

// =============================================================================
// RELATIONSHIP EXTRACTION
// =============================================================================

// Extract relationships between characters
static inline int zeta_story_extract_relationships(
    zeta_dual_ctx_t* graph,
    const char* text
) {
    if (!graph || !text || !g_story_ctx) return 0;

    int extracted = 0;

    // Relationship patterns
    const struct {
        const char* pattern;
        zeta_edge_type_t edge_type;
        const char* relation_name;
    } relationships[] = {
        {" loves ", EDGE_RELATED, "loves"},
        {" married ", EDGE_RELATED, "married_to"},
        {" is the father of ", EDGE_RELATED, "father_of"},
        {" is the mother of ", EDGE_RELATED, "mother_of"},
        {" is the sister of ", EDGE_RELATED, "sister_of"},
        {" is the brother of ", EDGE_RELATED, "brother_of"},
        {" created ", EDGE_CREATED, "created"},
        {" destroyed ", EDGE_CAUSES, "destroyed"},
        {" killed ", EDGE_PREVENTS, "killed"},
        {" saved ", EDGE_PREVENTS, "saved"},
        {" works with ", EDGE_RELATED, "colleague"},
        {" works for ", EDGE_RELATED, "works_for"},
        {" betrayed ", EDGE_CAUSES, "betrayed"},
        {NULL, EDGE_RELATED, NULL}
    };

    for (int i = 0; relationships[i].pattern; i++) {
        const char* match = text;
        while ((match = strstr(match, relationships[i].pattern)) != NULL) {
            // Find subject (before pattern)
            char subject[128] = {0};
            const char* subj_end = match;
            const char* subj_start = match - 1;

            while (subj_start > text && *subj_start == ' ') subj_start--;
            const char* word_end = subj_start + 1;

            // Walk back to find name start
            int words = 0;
            while (subj_start > text && words < 3) {
                if (*subj_start == ' ') words++;
                subj_start--;
            }
            if (subj_start > text) subj_start++;
            if (*subj_start == ' ') subj_start++;

            int si = 0;
            while (subj_start < word_end && si < 127) {
                subject[si++] = *subj_start++;
            }
            subject[si] = '\0';

            // Find object (after pattern)
            char object[128] = {0};
            const char* obj_start = match + strlen(relationships[i].pattern);
            int oi = 0;

            // Extract up to 3 words
            words = 0;
            while (*obj_start && oi < 127 && words < 3) {
                object[oi++] = *obj_start++;
                if (*obj_start == ' ') words++;
                if (*obj_start == '.' || *obj_start == ',' ||
                    *obj_start == '!' || *obj_start == '?') break;
            }
            while (oi > 0 && object[oi-1] == ' ') oi--;
            object[oi] = '\0';

            if (si > 2 && oi > 2) {
                // Find or create nodes for subject and object
                int64_t subj_id = -1, obj_id = -1;

                for (int c = 0; c < g_story_ctx->num_characters; c++) {
                    if (strstr(g_story_ctx->characters[c].name, subject) ||
                        strstr(subject, g_story_ctx->characters[c].name)) {
                        subj_id = g_story_ctx->characters[c].node_id;
                    }
                    if (strstr(g_story_ctx->characters[c].name, object) ||
                        strstr(object, g_story_ctx->characters[c].name)) {
                        obj_id = g_story_ctx->characters[c].node_id;
                    }
                }

                if (subj_id > 0 && obj_id > 0) {
                    // Create relationship edge
                    zeta_commit_edge(graph, subj_id, obj_id,
                                    relationships[i].edge_type, 0.95f);

                    // Store relationship as fact for surfacing
                    char rel_fact[256];
                    snprintf(rel_fact, sizeof(rel_fact), "%s %s %s",
                             subject, relationships[i].relation_name, object);
                    zeta_commit_fact(graph, NODE_RELATION,
                        STORY_LABEL_RELATIONSHIP, rel_fact, 0.9f, SOURCE_MODEL);

                    extracted++;
                    fprintf(stderr, "[STORY] Relationship: %s --%s--> %s\n",
                            subject, relationships[i].relation_name, object);
                }
            }

            match++;
        }
    }

    return extracted;
}

// =============================================================================
// CHAPTER MARKING
// =============================================================================

static inline int64_t zeta_story_mark_chapter(
    zeta_dual_ctx_t* graph,
    int chapter_num,
    const char* chapter_title
) {
    if (!graph || !g_story_ctx || chapter_num < 1 || chapter_num > 32) return -1;

    char chapter_value[256];
    snprintf(chapter_value, sizeof(chapter_value), "Chapter %d: %s",
             chapter_num, chapter_title ? chapter_title : "");

    int64_t node_id = zeta_commit_fact(graph, NODE_EVENT,
        STORY_LABEL_CHAPTER, chapter_value, 0.95f, SOURCE_MODEL);

    if (node_id > 0) {
        g_story_ctx->chapter_node_ids[chapter_num - 1] = node_id;
        if (chapter_title) {
            strncpy(g_story_ctx->chapter_titles[chapter_num - 1],
                    chapter_title, 127);
        }
        g_story_ctx->current_chapter = chapter_num;

        // Create temporal edge to previous chapter
        if (chapter_num > 1 && g_story_ctx->chapter_node_ids[chapter_num - 2] > 0) {
            zeta_commit_edge(graph,
                g_story_ctx->chapter_node_ids[chapter_num - 2], node_id,
                EDGE_TEMPORAL, 1.0f);
        }

        fprintf(stderr, "[STORY] Marked Chapter %d: %s\n",
                chapter_num, chapter_title ? chapter_title : "(untitled)");
    }

    return node_id;
}

// =============================================================================
// STORY CONTEXT SURFACING
// =============================================================================

// Surface relevant story facts for current generation
// Returns formatted context string to prepend to prompt
static inline const char* zeta_story_surface_context(
    zeta_dual_ctx_t* graph,
    int current_chapter
) {
    if (!graph || !g_story_ctx) return "";

    g_story_ctx->active_context[0] = '\0';
    char* p = g_story_ctx->active_context;
    int remaining = sizeof(g_story_ctx->active_context) - 1;

    int n = snprintf(p, remaining,
        "[STORY FACTS - DO NOT CONTRADICT]\n");
    p += n; remaining -= n;

    // Surface all characters with their established names
    if (g_story_ctx->num_characters > 0) {
        n = snprintf(p, remaining, "CHARACTERS:\n");
        p += n; remaining -= n;

        for (int i = 0; i < g_story_ctx->num_characters && remaining > 100; i++) {
            n = snprintf(p, remaining, "- %s (%s, introduced ch%d)\n",
                         g_story_ctx->characters[i].name,
                         g_story_ctx->characters[i].role,
                         g_story_ctx->characters[i].introduced_in_chapter);
            p += n; remaining -= n;
        }
    }

    // Surface locations
    if (g_story_ctx->num_locations > 0) {
        n = snprintf(p, remaining, "LOCATIONS:\n");
        p += n; remaining -= n;

        for (int i = 0; i < g_story_ctx->num_locations && remaining > 100; i++) {
            n = snprintf(p, remaining, "- %s\n",
                         g_story_ctx->locations[i].name);
            p += n; remaining -= n;
        }
    }

    // Surface key plot points from previous chapters
    if (current_chapter > 1) {
        n = snprintf(p, remaining, "ESTABLISHED PLOT POINTS:\n");
        p += n; remaining -= n;

        // Query graph for plot points
        zeta_surfaced_context_t ctx_out;
        char query[64];
        snprintf(query, sizeof(query), "plot chapter %d", current_chapter - 1);
        zeta_surface_context(graph, query, &ctx_out);

        for (int i = 0; i < ctx_out.num_nodes && remaining > 100; i++) {
            if (strcmp(ctx_out.nodes[i]->label, STORY_LABEL_PLOT_POINT) == 0) {
                n = snprintf(p, remaining, "- %s\n", ctx_out.nodes[i]->value);
                p += n; remaining -= n;
                g_story_ctx->facts_surfaced++;
            }
        }
    }

    // Surface relationships
    zeta_surfaced_context_t rel_ctx;
    zeta_surface_context(graph, "relationship", &rel_ctx);

    bool has_relationships = false;
    for (int i = 0; i < rel_ctx.num_nodes && remaining > 100; i++) {
        if (strcmp(rel_ctx.nodes[i]->label, STORY_LABEL_RELATIONSHIP) == 0) {
            if (!has_relationships) {
                n = snprintf(p, remaining, "RELATIONSHIPS:\n");
                p += n; remaining -= n;
                has_relationships = true;
            }
            n = snprintf(p, remaining, "- %s\n", rel_ctx.nodes[i]->value);
            p += n; remaining -= n;
            g_story_ctx->facts_surfaced++;
        }
    }

    n = snprintf(p, remaining, "[END STORY FACTS]\n\n");
    p += n; remaining -= n;

    fprintf(stderr, "[STORY] Surfaced %d facts for ch%d (%d chars)\n",
            g_story_ctx->facts_surfaced, current_chapter,
            (int)(sizeof(g_story_ctx->active_context) - remaining));

    return g_story_ctx->active_context;
}

// =============================================================================
// FULL EXTRACTION (call during planning phase)
// =============================================================================

// Extract all story elements from planning output
static inline int zeta_story_extract_all(
    zeta_dual_ctx_t* graph,
    const char* planning_text,
    int chapter_num
) {
    if (!graph || !planning_text) return 0;

    int total = 0;

    // Initialize story context if needed
    if (!g_story_ctx) {
        zeta_story_init(NULL, "untitled");
    }

    // Mark chapter if provided
    if (chapter_num > 0) {
        // Try to extract chapter title
        char title[128] = {0};
        const char* ch_match = strstr(planning_text, "Chapter");
        if (!ch_match) ch_match = strstr(planning_text, "CHAPTER");
        if (ch_match) {
            const char* title_start = strchr(ch_match, ':');
            if (title_start) {
                title_start++;
                while (*title_start == ' ') title_start++;
                int ti = 0;
                while (*title_start && ti < 127 &&
                       *title_start != '\n' && *title_start != '.') {
                    title[ti++] = *title_start++;
                }
                title[ti] = '\0';
            }
        }

        zeta_story_mark_chapter(graph, chapter_num, title[0] ? title : NULL);
    }

    // Extract all elements
    total += zeta_story_extract_characters(graph, planning_text, chapter_num);
    total += zeta_story_extract_locations(graph, planning_text);
    total += zeta_story_extract_plot_points(graph, planning_text, chapter_num);
    total += zeta_story_extract_relationships(graph, planning_text);

    fprintf(stderr, "[STORY] Extracted %d elements from ch%d planning\n",
            total, chapter_num);

    return total;
}

// =============================================================================
// COHERENCE CHECK
// =============================================================================

// Check if text contradicts established facts
// Returns number of contradictions found (0 = coherent)
static inline int zeta_story_check_coherence(
    zeta_dual_ctx_t* graph,
    const char* generated_text
) {
    if (!graph || !generated_text || !g_story_ctx) return 0;

    int contradictions = 0;

    // Check for character name variations that might be errors
    for (int i = 0; i < g_story_ctx->num_characters; i++) {
        const char* correct_name = g_story_ctx->characters[i].name;

        // Look for similar but different names
        const char* p = generated_text;
        while (*p) {
            // Find capitalized words (potential names)
            if (*p >= 'A' && *p <= 'Z') {
                char potential[128] = {0};
                int pi = 0;

                while (*p && pi < 127 &&
                       ((*p >= 'a' && *p <= 'z') ||
                        (*p >= 'A' && *p <= 'Z') ||
                        *p == ' ' || *p == '.')) {
                    potential[pi++] = *p++;
                    if (*p == ' ' &&
                        !(*(p+1) >= 'A' && *(p+1) <= 'Z')) {
                        break;
                    }
                }
                potential[pi] = '\0';

                // Check if similar but not exact
                if (strlen(potential) > 5 &&
                    strcasecmp(potential, correct_name) != 0) {
                    // Simple similarity: shared prefix
                    int match_len = 0;
                    while (potential[match_len] && correct_name[match_len] &&
                           tolower(potential[match_len]) == tolower(correct_name[match_len])) {
                        match_len++;
                    }

                    // If more than half matches but not exact, likely a drift
                    if (match_len > strlen(correct_name) / 2 &&
                        match_len < strlen(correct_name)) {
                        contradictions++;
                        fprintf(stderr, "[STORY] WARNING: Name drift? '%s' vs '%s'\n",
                                potential, correct_name);
                    }
                }
            } else {
                p++;
            }
        }
    }

    if (contradictions > 0) {
        g_story_ctx->contradictions_prevented += contradictions;
    }

    return contradictions;
}

// =============================================================================
// HTTP ENDPOINT HELPERS
// =============================================================================

// Format story stats as JSON
static inline void zeta_story_stats_json(char* buf, size_t buf_size) {
    if (!g_story_ctx) {
        snprintf(buf, buf_size,
            "{\"initialized\": false}");
        return;
    }

    snprintf(buf, buf_size,
        "{"
        "\"title\": \"%s\","
        "\"genre\": \"%s\","
        "\"current_chapter\": %d,"
        "\"num_characters\": %d,"
        "\"num_locations\": %d,"
        "\"facts_surfaced\": %d,"
        "\"contradictions_prevented\": %d"
        "}",
        g_story_ctx->story_title,
        g_story_ctx->story_genre[0] ? g_story_ctx->story_genre : "unknown",
        g_story_ctx->current_chapter,
        g_story_ctx->num_characters,
        g_story_ctx->num_locations,
        g_story_ctx->facts_surfaced,
        g_story_ctx->contradictions_prevented);
}

// Format characters as JSON
static inline void zeta_story_characters_json(char* buf, size_t buf_size) {
    if (!g_story_ctx || g_story_ctx->num_characters == 0) {
        snprintf(buf, buf_size, "[]");
        return;
    }

    char* p = buf;
    int remaining = buf_size - 1;

    *p++ = '['; remaining--;

    for (int i = 0; i < g_story_ctx->num_characters && remaining > 200; i++) {
        int n = snprintf(p, remaining,
            "%s{\"name\": \"%s\", \"role\": \"%s\", \"introduced_ch\": %d}",
            i > 0 ? ", " : "",
            g_story_ctx->characters[i].name,
            g_story_ctx->characters[i].role,
            g_story_ctx->characters[i].introduced_in_chapter);
        p += n; remaining -= n;
    }

    *p++ = ']'; remaining--;
    *p = '\0';
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_STORY_INTEGRATION_H
