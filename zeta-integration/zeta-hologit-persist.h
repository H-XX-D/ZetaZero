// Z.E.T.A. HoloGit NVMe Persistence Layer
#ifndef ZETA_HOLOGIT_PERSIST_H
#define ZETA_HOLOGIT_PERSIST_H

#include "zeta-hologit.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define HOLOGIT_STORAGE_ROOT "/mnt/HoloGit"

// Save a single block to disk
static inline int hologit_save_block(int64_t block_id, const float* summary, int dim) {
    char path[256];
    snprintf(path, sizeof(path), "%s/blocks/%lld.bin", HOLOGIT_STORAGE_ROOT, (long long)block_id);
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    fwrite(&block_id, sizeof(int64_t), 1, f);
    fwrite(&dim, sizeof(int), 1, f);
    fwrite(summary, sizeof(float), dim, f);
    fclose(f);
    return 0;
}

// Save prompt text for a block
static inline int hologit_save_text(int64_t block_id, const char* text) {
    char path[256];
    snprintf(path, sizeof(path), "%s/texts/%lld.txt", HOLOGIT_STORAGE_ROOT, (long long)block_id);
    
    FILE* f = fopen(path, "w");
    if (!f) return -1;
    
    fprintf(f, "%s", text);
    fclose(f);
    return 0;
}

// Load a block from disk
static inline float* hologit_load_block(int64_t block_id, int* dim_out) {
    char path[256];
    snprintf(path, sizeof(path), "%s/blocks/%lld.bin", HOLOGIT_STORAGE_ROOT, (long long)block_id);
    
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    int64_t stored_id;
    int dim;
    fread(&stored_id, sizeof(int64_t), 1, f);
    fread(&dim, sizeof(int), 1, f);
    
    float* summary = (float*)malloc(dim * sizeof(float));
    fread(summary, sizeof(float), dim, f);
    fclose(f);
    
    if (dim_out) *dim_out = dim;
    return summary;
}

// Save edge to disk
static inline int hologit_save_edge(const zeta_edge_t* edge) {
    char path[256];
    snprintf(path, sizeof(path), "%s/edges/%lld_%lld.bin", 
             HOLOGIT_STORAGE_ROOT, (long long)edge->block_a, (long long)edge->block_b);
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    fwrite(edge, sizeof(zeta_edge_t), 1, f);
    fclose(f);
    return 0;
}

// Save version snapshot
static inline int hologit_save_version(int64_t block_id, const zeta_version_t* ver, int dim) {
    char path[256];
    snprintf(path, sizeof(path), "%s/versions/%lld_v%lld.bin", 
             HOLOGIT_STORAGE_ROOT, (long long)block_id, (long long)ver->version_id);
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    fwrite(ver, sizeof(zeta_version_t) - sizeof(float*), 1, f);  // Exclude pointer
    fwrite(&dim, sizeof(int), 1, f);
    fwrite(ver->summary_snapshot, sizeof(float), dim, f);
    fclose(f);
    return 0;
}

// Flush all hologit state to disk
static inline void hologit_sync(zeta_hologit_t* hg, int summary_dim) {
    fprintf(stderr, "[HoloGit] Syncing to %s...\n", HOLOGIT_STORAGE_ROOT);
    
    // Save all blocks
    for (int i = 0; i < hg->num_blocks; i++) {
        // Assuming we have access to summaries - would need to store them
        // This is a placeholder - full implementation would track summaries
    }
    
    // Save all edges
    for (int i = 0; i < hg->num_edges; i++) {
        hologit_save_edge(&hg->edges[i]);
    }
    
    fprintf(stderr, "[HoloGit] Synced %d edges\n", hg->num_edges);
}

#endif // ZETA_HOLOGIT_PERSIST_H
