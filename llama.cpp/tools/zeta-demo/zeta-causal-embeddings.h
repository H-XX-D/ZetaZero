// Z.E.T.A. Semantic Causal Extraction
// Uses BGE embeddings to detect CAUSES/PREVENTS relationships
// Replaces brittle verb patterns with learned semantic understanding

#ifndef ZETA_CAUSAL_EMBEDDINGS_H
#define ZETA_CAUSAL_EMBEDDINGS_H

#include "zeta-embed-integration.h"
#include <vector>
#include <string>

#define ZETA_CAUSAL_EMBED_DIM 1536  // BGE-small dimension
#define ZETA_MAX_ANCHORS 64

// Anchor embeddings for CAUSES and PREVENTS detection
struct zeta_causal_anchors {
    float causes_anchors[ZETA_MAX_ANCHORS][ZETA_CAUSAL_EMBED_DIM];
    int num_causes;
    float prevents_anchors[ZETA_MAX_ANCHORS][ZETA_CAUSAL_EMBED_DIM];
    int num_prevents;
    float causes_threshold;
    float prevents_threshold;
    bool initialized;
};

static zeta_causal_anchors g_causal_anchors = {0};

// Initialize causal anchors by embedding canonical phrases
static bool zeta_causal_init_anchors() {
    if (g_causal_anchors.initialized) return true;
    if (!g_embed_ctx || !g_embed_ctx->initialized) {
        fprintf(stderr, "[CAUSAL-EMB] ERROR: Embedding model not initialized\n");
        return false;
    }

    // CAUSES anchor phrases - expanded from ATOMIC/CausalBank style
    const char* causes_phrases[] = {
        "causes", "triggers", "leads to", "results in", "produces",
        "wakes", "awakens", "activates", "initiates", "starts",
        "eats", "consumes", "devours", "destroys", "kills", "killed",
        "slays", "slayed", "slew", "murdered", "assassinated",
        "poisoned", "attacked", "harmed", "injured", "defeated",
        "creates", "generates", "enables", "infects", "ignites",
        "motivates", "transforms", "unlocks", "summons", "breaks",
        "A causes B to happen", "A leads to B", "A triggers B",
        "A killed B", "A poisoned B", "A attacked B",
        NULL
    };

    // PREVENTS anchor phrases (pure prevention semantics, no killing verbs)
    const char* prevents_phrases[] = {
        "prevents", "stops", "blocks", "inhibits", "halts",
        "neutralizes", "nullifies", "negates", "interrupts",
        "shields from", "protects from", "guards against",
        "cures", "heals", "saves from", "rescues from",
        "gave antidote", "provided cure", "stopped the effect",
        "before it could happen", "prevented from happening",
        "A prevents B", "A stops B from happening", "A blocks B",
        "A saved B from", "A protected B from",
        NULL
    };

    // Embed CAUSES anchors
    g_causal_anchors.num_causes = 0;
    for (int i = 0; causes_phrases[i] && g_causal_anchors.num_causes < ZETA_MAX_ANCHORS; i++) {
        int dim = zeta_embed_text(causes_phrases[i],
                                   g_causal_anchors.causes_anchors[g_causal_anchors.num_causes],
                                   ZETA_CAUSAL_EMBED_DIM);
        if (dim > 0) {
            g_causal_anchors.num_causes++;
        }
    }
    fprintf(stderr, "[CAUSAL-EMB] Embedded %d CAUSES anchors\n", g_causal_anchors.num_causes);

    // Embed PREVENTS anchors
    g_causal_anchors.num_prevents = 0;
    for (int i = 0; prevents_phrases[i] && g_causal_anchors.num_prevents < ZETA_MAX_ANCHORS; i++) {
        int dim = zeta_embed_text(prevents_phrases[i],
                                   g_causal_anchors.prevents_anchors[g_causal_anchors.num_prevents],
                                   ZETA_CAUSAL_EMBED_DIM);
        if (dim > 0) {
            g_causal_anchors.num_prevents++;
        }
    }
    fprintf(stderr, "[CAUSAL-EMB] Embedded %d PREVENTS anchors\n", g_causal_anchors.num_prevents);

    // Set thresholds (tunable)
    g_causal_anchors.causes_threshold = 0.55f;
    g_causal_anchors.prevents_threshold = 0.60f;
    g_causal_anchors.initialized = true;

    return true;
}

// Classify a sentence as CAUSES, PREVENTS, or NEITHER
// Returns: 1 = CAUSES, 2 = PREVENTS, 0 = NEITHER
// confidence is set to the max similarity score
static int zeta_causal_classify(const char* sentence, float* confidence) {
    if (!g_causal_anchors.initialized || !sentence) {
        if (confidence) *confidence = 0.0f;
        return 0;
    }

    // Embed the candidate sentence
    float sentence_emb[ZETA_CAUSAL_EMBED_DIM];
    int dim = zeta_embed_text(sentence, sentence_emb, ZETA_CAUSAL_EMBED_DIM);
    if (dim <= 0) {
        if (confidence) *confidence = 0.0f;
        return 0;
    }

    // Find max similarity to CAUSES anchors
    float max_causes = 0.0f;
    for (int i = 0; i < g_causal_anchors.num_causes; i++) {
        float sim = zeta_embed_similarity(sentence_emb,
                                           g_causal_anchors.causes_anchors[i],
                                           ZETA_CAUSAL_EMBED_DIM);
        if (sim > max_causes) max_causes = sim;
    }

    // Find max similarity to PREVENTS anchors
    float max_prevents = 0.0f;
    for (int i = 0; i < g_causal_anchors.num_prevents; i++) {
        float sim = zeta_embed_similarity(sentence_emb,
                                           g_causal_anchors.prevents_anchors[i],
                                           ZETA_CAUSAL_EMBED_DIM);
        if (sim > max_prevents) max_prevents = sim;
    }

    // Debug: log similarity scores
    fprintf(stderr, "[CAUSAL-DBG] Sentence: %.40s... | CAUSES=%.3f PREVENTS=%.3f\n",
            sentence, max_causes, max_prevents);
    
    // Decide based on thresholds
    bool is_causes = max_causes > g_causal_anchors.causes_threshold;
    bool is_prevents = max_prevents > g_causal_anchors.prevents_threshold;

    // PREVENTS takes priority (prevention overrides causation)
    if (is_prevents && max_prevents > max_causes) {
        if (confidence) *confidence = max_prevents;
        fprintf(stderr, "[CAUSAL-EMB] PREVENTS detected (sim=%.3f): %.50s...\n", max_prevents, sentence);
        return 2;
    }
    if (is_causes) {
        if (confidence) *confidence = max_causes;
        fprintf(stderr, "[CAUSAL-EMB] CAUSES detected (sim=%.3f): %.50s...\n", max_causes, sentence);
        return 1;
    }

    if (confidence) *confidence = fmaxf(max_causes, max_prevents);
    return 0;
}

// Extract causal edges from text using embeddings
// Returns number of edges created
int zeta_causal_extract_edges(
    zeta_dual_ctx_t* ctx,
    const char* text
) {
    if (!ctx || !text) return 0;
    if (!g_causal_anchors.initialized) {
        zeta_causal_init_anchors();
    }

    int edges_created = 0;

    // Split text into sentences (simple split on . ! ?)
    char text_copy[2048];
    strncpy(text_copy, text, 2047);
    text_copy[2047] = '\0';

    char* sentence = strtok(text_copy, ".!?");
    while (sentence) {
        // Skip whitespace
        while (*sentence == ' ' || *sentence == '\n') sentence++;
        if (strlen(sentence) < 5) {
            sentence = strtok(NULL, ".!?");
            continue;
        }

        float confidence = 0.0f;
        int causal_type = zeta_causal_classify(sentence, &confidence);

        fprintf(stderr, "[CAUSAL-DEBUG] Sentence: %s -> type=%d, conf=%.2f\n", sentence, causal_type, confidence);

        if (causal_type > 0) {
            // Extract subject (first noun phrase) and object (after verb)
            // Simple heuristic: split on the verb phrase
            char lower[512];
            int slen = strlen(sentence);
            if (slen > 511) slen = 511;
            for (int i = 0; i < slen; i++) lower[i] = tolower(sentence[i]);
            lower[slen] = '\0';

            // Find subject (before first space-verb-space pattern)
            char subject[128] = {0};
            char object[128] = {0};

            // Look for common verb patterns to split subject/object
            const char* verb_markers[] = {
                // CAUSES verbs
                " wakes ", " awakens ", " triggers ", " activates ", " initiates ",
                " eats ", " devours ", " consumes ", " destroys ", " decimates ", " shatters ", " crushes ", " annihilates ",
                " causes ", " leads to ", " results in ", " produces ", " creates ",
                " kills ", " slays ", " slayed ", " killed ", " eliminated ",
                // PREVENTS verbs
                " prevents ", " stops ", " blocks ", " inhibits ", " halts ",
                " neutralizes ", " neutralized ", " negates ", " counters ",
                " quells ", " quenches ", " extinguishes ", " suppresses ",
                " defeats ", " vanquishes ", " thwarts ", " foils ",
                // Temporal markers
                " before ", " prevented ", NULL
            };

            for (int v = 0; verb_markers[v]; v++) {
                const char* split = strstr(lower, verb_markers[v]);
                if (split) {
                    // Subject is before split
                    int subj_len = split - lower;
                    if (subj_len > 127) subj_len = 127;
                    strncpy(subject, lower, subj_len);
                    subject[subj_len] = '\0';

                    // Trim "the " prefix if present
                    char* subj_start = subject;
                    if (strncmp(subj_start, "the ", 4) == 0) subj_start += 4;
                    memmove(subject, subj_start, strlen(subj_start) + 1);

                    // Object is after verb
                    const char* obj_start = split + strlen(verb_markers[v]);
                    strncpy(object, obj_start, 127);
                    object[127] = '\0';

                    // Trim trailing whitespace
                    int olen = strlen(object);
                    while (olen > 0 && (object[olen-1] == ' ' || object[olen-1] == '\n')) {
                        object[--olen] = '\0';
                    }

                    // Trim "the " prefix
                    if (strncmp(object, "the ", 4) == 0) {
                        memmove(object, object + 4, strlen(object + 4) + 1);
                    }

                    break;
                }
            }

            if (strlen(subject) > 1 && strlen(object) > 1) {
                // Create nodes and edge
                zeta_edge_type_t edge_type = (causal_type == 1) ? EDGE_CAUSES : EDGE_PREVENTS;
                const char* subj_label = (causal_type == 1) ? "causal_agent" : "preventer";
                const char* obj_label = (causal_type == 1) ? "causal_target" : "prevented";

                int64_t subj_id = zeta_create_node(ctx, NODE_ENTITY, subj_label, subject, 0.9f);
                int64_t obj_id = zeta_create_node(ctx, NODE_ENTITY, obj_label, object, 0.9f);
                zeta_create_edge(ctx, subj_id, obj_id, edge_type, confidence);

                // Store full sentence as NODE_FACT for surfacing (no extra edges needed)
                // The CAUSES/PREVENTS edge already links subject->object
                char relation_label[64];
                snprintf(relation_label, sizeof(relation_label), "%s_relation",
                         (causal_type == 1) ? "causes" : "prevents");
                zeta_create_node(ctx, NODE_FACT, relation_label, sentence, 0.95f);

                edges_created++;
                fprintf(stderr, "[CAUSAL-EMB] Edge: %s --%s--> %s (conf=%.2f)\n",
                        subject, (causal_type == 1) ? "CAUSES" : "PREVENTS", object, confidence);
                fprintf(stderr, "[CAUSAL-EMB] Stored sentence: %.60s...\n", sentence);
            }
        }

        sentence = strtok(NULL, ".!?");
    }

    return edges_created;
}

// Semantic similarity surfacing boost
// Returns boost factor based on embedding similarity to query
static float zeta_causal_semantic_boost(
    const char* query,
    const char* fact_value,
    float base_momentum
) {
    if (!g_embed_ctx || !g_embed_ctx->initialized) return 1.0f;
    if (!query || !fact_value) return 1.0f;

    // Embed query and fact
    float query_emb[ZETA_CAUSAL_EMBED_DIM];
    float fact_emb[ZETA_CAUSAL_EMBED_DIM];

    if (zeta_embed_text(query, query_emb, ZETA_CAUSAL_EMBED_DIM) <= 0) return 1.0f;
    if (zeta_embed_text(fact_value, fact_emb, ZETA_CAUSAL_EMBED_DIM) <= 0) return 1.0f;

    // Get sharpened similarity
    float sim = zeta_embed_similarity_sharp(query_emb, fact_emb, ZETA_CAUSAL_EMBED_DIM, 3.0f);

    // Apply tunneling: high momentum = narrow threshold
    float tunnel_threshold = 0.3f + (base_momentum * 0.5f);  // 0.3 to 0.8

    if (sim > tunnel_threshold) {
        // Boost proportional to how much it exceeds threshold
        float boost = 1.0f + (sim - tunnel_threshold) * 3.0f;
        return boost;
    }

    return 1.0f;
}

#endif // ZETA_CAUSAL_EMBEDDINGS_H
