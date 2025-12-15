// Z.E.T.A. Fact Store - Cross-session entity-fact persistence
// Stores facts as entity-value pairs with embeddings for similarity search

#ifndef ZETA_FACT_STORE_H
#define ZETA_FACT_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZETA_MAX_FACTS       10000
#define ZETA_MAX_ENTITY_LEN  128
#define ZETA_MAX_FACT_LEN    512
#define ZETA_FACT_EMBED_DIM  256

typedef struct {
    int64_t fact_id;
    char entity[ZETA_MAX_ENTITY_LEN];
    char value[ZETA_MAX_FACT_LEN];
    char full_text[ZETA_MAX_FACT_LEN];
    float embedding[ZETA_FACT_EMBED_DIM];
    float embedding_norm;
    int64_t timestamp;
    float confidence;
    bool is_superseded;
    int64_t supersedes_id;
} zeta_fact_t;

typedef struct {
    zeta_fact_t facts[ZETA_MAX_FACTS];
    int num_facts;
    int64_t next_fact_id;
    char storage_path[512];
    int embed_dim;
} zeta_fact_store_t;

// Forward declarations
static inline int zeta_fact_store_load(zeta_fact_store_t* store);
static inline void zeta_compute_fact_embedding(const char* text, float* embed, int dim);

static inline zeta_fact_store_t* zeta_fact_store_init(const char* storage_dir) {
    zeta_fact_store_t* store = (zeta_fact_store_t*)calloc(1, sizeof(zeta_fact_store_t));
    if (!store) return NULL;
    
    snprintf(store->storage_path, sizeof(store->storage_path), "%s/facts", storage_dir);
    store->embed_dim = ZETA_FACT_EMBED_DIM;
    store->next_fact_id = 1;
    
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", store->storage_path);
    (void)system(cmd);
    
    zeta_fact_store_load(store);
    
    return store;
}

static inline void zeta_fact_store_free(zeta_fact_store_t* store) {
    free(store);
}

static inline void zeta_compute_fact_embedding(const char* text, float* embed, int dim) {
    memset(embed, 0, dim * sizeof(float));
    if (!text || !embed) return;
    
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        uint32_t hash = (uint32_t)text[i];
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = (hash >> 16) ^ hash;
        
        int idx = hash % dim;
        embed[idx] += 1.0f;
        
        if (i + 1 < len) {
            uint32_t bigram = ((uint32_t)text[i] << 8) | (uint32_t)text[i+1];
            bigram = ((bigram >> 16) ^ bigram) * 0x45d9f3b;
            idx = bigram % dim;
            embed[idx] += 0.5f;
        }
    }
    
    float norm = 0.0f;
    for (int i = 0; i < dim; i++) norm += embed[i] * embed[i];
    norm = sqrtf(norm + 1e-8f);
    for (int i = 0; i < dim; i++) embed[i] /= norm;
}

static inline int64_t zeta_store_fact(zeta_fact_store_t* store, const char* entity, 
                                       const char* value, const char* full_text, float confidence) {
    if (!store || !entity || !value) return -1;
    if (store->num_facts >= ZETA_MAX_FACTS) return -1;
    
    for (int i = 0; i < store->num_facts; i++) {
        if (strcmp(store->facts[i].entity, entity) == 0 &&
            strcmp(store->facts[i].value, value) == 0) {
            store->facts[i].timestamp = (int64_t)time(NULL);
            return store->facts[i].fact_id;
        }
    }
    
    zeta_fact_t* fact = &store->facts[store->num_facts];
    fact->fact_id = store->next_fact_id++;
    strncpy(fact->entity, entity, ZETA_MAX_ENTITY_LEN - 1);
    strncpy(fact->value, value, ZETA_MAX_FACT_LEN - 1);
    strncpy(fact->full_text, full_text ? full_text : value, ZETA_MAX_FACT_LEN - 1);
    fact->timestamp = (int64_t)time(NULL);
    fact->confidence = confidence;
    fact->is_superseded = false;
    fact->supersedes_id = 0;
    
    zeta_compute_fact_embedding(fact->full_text, fact->embedding, store->embed_dim);
    
    float norm = 0.0f;
    for (int i = 0; i < store->embed_dim; i++) norm += fact->embedding[i] * fact->embedding[i];
    fact->embedding_norm = sqrtf(norm + 1e-8f);
    
    store->num_facts++;
    return fact->fact_id;
}

static inline int zeta_find_facts_by_entity(zeta_fact_store_t* store, const char* entity,
                                             zeta_fact_t** results, int max_results, bool include_superseded) {
    if (!store || !entity || !results) return 0;
    
    int count = 0;
    for (int i = 0; i < store->num_facts && count < max_results; i++) {
        zeta_fact_t* fact = &store->facts[i];
        if (!include_superseded && fact->is_superseded) continue;
        
        if (strcmp(fact->entity, entity) == 0) {
            results[count++] = fact;
        }
    }
    return count;
}

static inline int zeta_find_facts_by_similarity(zeta_fact_store_t* store, const char* query_text,
                                                 zeta_fact_t** results, float* scores, 
                                                 int max_results, float threshold) {
    if (!store || !query_text || !results || !scores) return 0;
    
    float query_embed[ZETA_FACT_EMBED_DIM];
    zeta_compute_fact_embedding(query_text, query_embed, store->embed_dim);
    
    float query_norm = 0.0f;
    for (int i = 0; i < store->embed_dim; i++) query_norm += query_embed[i] * query_embed[i];
    query_norm = sqrtf(query_norm + 1e-8f);
    
    typedef struct { int idx; float score; } scored_fact;
    scored_fact scored[ZETA_MAX_FACTS];
    int n_scored = 0;
    
    for (int i = 0; i < store->num_facts; i++) {
        zeta_fact_t* fact = &store->facts[i];
        if (fact->is_superseded) continue;
        
        float dot = 0.0f;
        for (int j = 0; j < store->embed_dim; j++) {
            dot += query_embed[j] * fact->embedding[j];
        }
        float sim = dot / (query_norm * fact->embedding_norm + 1e-8f);
        
        if (sim >= threshold) {
            scored[n_scored].idx = i;
            scored[n_scored].score = sim;
            n_scored++;
        }
    }
    
    for (int i = 0; i < n_scored - 1; i++) {
        for (int j = i + 1; j < n_scored; j++) {
            if (scored[j].score > scored[i].score) {
                scored_fact tmp = scored[i];
                scored[i] = scored[j];
                scored[j] = tmp;
            }
        }
    }
    
    int count = (n_scored < max_results) ? n_scored : max_results;
    for (int i = 0; i < count; i++) {
        results[i] = &store->facts[scored[i].idx];
        scores[i] = scored[i].score;
    }
    
    return count;
}

static inline int zeta_fact_store_save(zeta_fact_store_t* store) {
    if (!store) return -1;
    
    char path[600];
    snprintf(path, sizeof(path), "%s/facts.bin", store->storage_path);
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    const char magic[] = "ZFCT";
    fwrite(magic, 1, 4, f);
    fwrite(&store->num_facts, sizeof(int), 1, f);
    fwrite(&store->next_fact_id, sizeof(int64_t), 1, f);
    fwrite(&store->embed_dim, sizeof(int), 1, f);
    
    for (int i = 0; i < store->num_facts; i++) {
        fwrite(&store->facts[i], sizeof(zeta_fact_t), 1, f);
    }
    
    fclose(f);
    
    snprintf(path, sizeof(path), "%s/facts_index.txt", store->storage_path);
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < store->num_facts; i++) {
            zeta_fact_t* fact = &store->facts[i];
            fprintf(f, "%lld|%s|%s|%.2f|%s\n",
                (long long)fact->fact_id, fact->entity, fact->value, 
                fact->confidence, fact->is_superseded ? "superseded" : "active");
        }
        fclose(f);
    }
    
    fprintf(stderr, "[FACT] Saved %d facts to %s\n", store->num_facts, store->storage_path);
    return 0;
}

static inline int zeta_fact_store_load(zeta_fact_store_t* store) {
    char path[600];
    snprintf(path, sizeof(path), "%s/facts.bin", store->storage_path);
    
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[FACT] No existing facts at %s\n", path);
        return 0;
    }
    
    char magic[5] = {0};
    fread(magic, 1, 4, f);
    if (strcmp(magic, "ZFCT") != 0) {
        fprintf(stderr, "[FACT] Invalid facts file\n");
        fclose(f);
        return -1;
    }
    
    fread(&store->num_facts, sizeof(int), 1, f);
    fread(&store->next_fact_id, sizeof(int64_t), 1, f);
    fread(&store->embed_dim, sizeof(int), 1, f);
    
    for (int i = 0; i < store->num_facts; i++) {
        fread(&store->facts[i], sizeof(zeta_fact_t), 1, f);
    }
    
    fclose(f);
    
    fprintf(stderr, "[FACT] Loaded %d facts from %s\n", store->num_facts, store->storage_path);
    return store->num_facts;
}

static inline int zeta_extract_facts_from_text(const char* text, 
                                               char entities[][ZETA_MAX_ENTITY_LEN],
                                               char values[][ZETA_MAX_FACT_LEN], 
                                               int max_facts) {
    if (!text || !entities || !values) return 0;
    int count = 0;
    
    const char* patterns[][2] = {
        {"my name is ", "user_name"},
        {"i am ", "user_identity"},
        {"i'm called ", "user_name"},
        {"call me ", "user_name"},
        {"project ", "project_name"},
        {"codenamed ", "project_codename"},
        {"favorite color is ", "favorite_color"},
        {"favorite number is ", "favorite_number"},
        {NULL, NULL}
    };
    
    char lower[2048];
    size_t len = strlen(text);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    }
    lower[len] = '\0';
    
    for (int p = 0; patterns[p][0] != NULL && count < max_facts; p++) {
        const char* match = strstr(lower, patterns[p][0]);
        if (match) {
            const char* value_start = text + (match - lower) + strlen(patterns[p][0]);
            
            char value[ZETA_MAX_FACT_LEN] = {0};
            int vi = 0;
            while (*value_start && vi < ZETA_MAX_FACT_LEN - 1) {
                if (*value_start == '.' || *value_start == ',' || 
                    *value_start == '!' || *value_start == '?' ||
                    *value_start == '\n' || *value_start == ' ' && vi > 0 && 
                    (*(value_start+1) == 'a' || *(value_start+1) == 'A' ||
                     *(value_start+1) == 'i' || *(value_start+1) == 'I' ||
                     *(value_start+1) == 't' || *(value_start+1) == 'T')) {
                    break;
                }
                value[vi++] = *value_start++;
            }
            value[vi] = '\0';
            
            while (vi > 0 && value[vi-1] == ' ') value[--vi] = '\0';
            
            if (vi > 0) {
                strncpy(entities[count], patterns[p][1], ZETA_MAX_ENTITY_LEN - 1);
                strncpy(values[count], value, ZETA_MAX_FACT_LEN - 1);
                count++;
            }
        }
    }
    
    return count;
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_FACT_STORE_H
