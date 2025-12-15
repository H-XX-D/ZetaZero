// Z.E.T.A. HoloGit Implementation
// Memory Versioning and Correlation Tracking
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-hologit.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// Internal Helpers
// ============================================================================

static int find_block_index(const zeta_hologit_t* hg, int64_t block_id) {
    for (int i = 0; i < hg->num_blocks; i++) {
        if (hg->block_meta[i].block_id == block_id) {
            return i;
        }
    }
    return -1;
}

static int find_or_create_edge(zeta_hologit_t* hg, int64_t a, int64_t b) {
    // Normalize order: smaller ID first
    if (a > b) {
        int64_t tmp = a;
        a = b;
        b = tmp;
    }

    // Search existing
    for (int i = 0; i < hg->num_edges; i++) {
        if (hg->edges[i].block_a == a && hg->edges[i].block_b == b) {
            return i;
        }
    }

    // Create new
    if (hg->num_edges >= hg->max_edges) return -1;

    int idx = hg->num_edges++;
    hg->edges[idx].block_a = a;
    hg->edges[idx].block_b = b;
    hg->edges[idx].weight = 0.0f;
    hg->edges[idx].co_retrieval_count = 0;
    hg->edges[idx].last_co_retrieval = 0;

    return idx;
}

static float compute_summary_distance(const float* a, const float* b, int dim) {
    float sum = 0.0f;
    for (int i = 0; i < dim; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

// ============================================================================
// Initialization
// ============================================================================

zeta_hologit_t* zeta_hologit_init(int max_blocks, int summary_dim) {
    zeta_hologit_t* hg = (zeta_hologit_t*)calloc(1, sizeof(zeta_hologit_t));
    if (!hg) return NULL;

    hg->max_blocks = max_blocks;
    hg->block_meta = (zeta_block_meta_t*)calloc(max_blocks, sizeof(zeta_block_meta_t));

    // Allocate edge storage (assume max edges = blocks * edges_per_block / 2)
    hg->max_edges = max_blocks * ZETA_MAX_EDGES_PER_BLOCK / 2;
    hg->edges = (zeta_edge_t*)calloc(hg->max_edges, sizeof(zeta_edge_t));

    if (!hg->block_meta || !hg->edges) {
        zeta_hologit_free(hg);
        return NULL;
    }

    // Default config
    hg->correlation_decay = ZETA_CORRELATION_DECAY;
    hg->correlation_boost = ZETA_CORRELATION_BOOST;
    hg->stability_threshold = 0.01f;  // 1% drift = stable

    return hg;
}

void zeta_hologit_free(zeta_hologit_t* hg) {
    if (!hg) return;

    // Free version snapshots
    for (int i = 0; i < hg->num_blocks; i++) {
        for (int v = 0; v < hg->block_meta[i].num_versions; v++) {
            free(hg->block_meta[i].versions[v].summary_snapshot);
        }
    }

    free(hg->block_meta);
    free(hg->edges);
    free(hg);
}

// ============================================================================
// Block Registration
// ============================================================================

int zeta_hologit_register_block(
    zeta_hologit_t* hg,
    int64_t block_id,
    const float* initial_summary,
    int summary_dim
) {
    if (hg->num_blocks >= hg->max_blocks) return -1;

    int idx = hg->num_blocks++;
    zeta_block_meta_t* meta = &hg->block_meta[idx];

    meta->block_id = block_id;
    meta->num_versions = 0;
    meta->current_version = -1;
    meta->num_edges = 0;
    meta->summary_drift = 0.0f;
    meta->is_stable = false;

    // Create initial version
    if (initial_summary) {
        zeta_version_t* v = &meta->versions[0];
        v->version_id = 0;
        v->step_created = 0;
        v->summary_snapshot = (float*)malloc(summary_dim * sizeof(float));
        memcpy(v->summary_snapshot, initial_summary, summary_dim * sizeof(float));
        strncpy(v->reason, "initial", sizeof(v->reason) - 1);

        meta->num_versions = 1;
        meta->current_version = 0;
    }

    return idx;
}

// ============================================================================
// Co-Retrieval Tracking
// ============================================================================

void zeta_hologit_record_co_retrieval(
    zeta_hologit_t* hg,
    const int64_t* block_ids,
    int num_blocks,
    int64_t current_step
) {
    if (num_blocks < 2) return;

    hg->total_co_retrievals++;

    // Create/update edges between all pairs
    for (int i = 0; i < num_blocks; i++) {
        for (int j = i + 1; j < num_blocks; j++) {
            int edge_idx = find_or_create_edge(hg, block_ids[i], block_ids[j]);
            if (edge_idx < 0) continue;

            zeta_edge_t* edge = &hg->edges[edge_idx];
            edge->co_retrieval_count++;
            edge->last_co_retrieval = current_step;

            // Boost weight (bounded to 1.0)
            edge->weight += hg->correlation_boost * (1.0f - edge->weight);
            if (edge->weight > 1.0f) edge->weight = 1.0f;

            // Update block edge lists
            int idx_a = find_block_index(hg, block_ids[i]);
            int idx_b = find_block_index(hg, block_ids[j]);

            if (idx_a >= 0 && hg->block_meta[idx_a].num_edges < ZETA_MAX_EDGES_PER_BLOCK) {
                // Check if edge already recorded
                bool found = false;
                for (int e = 0; e < hg->block_meta[idx_a].num_edges; e++) {
                    if (hg->block_meta[idx_a].edge_targets[e] == block_ids[j]) {
                        hg->block_meta[idx_a].edge_weights[e] = edge->weight;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    int e = hg->block_meta[idx_a].num_edges++;
                    hg->block_meta[idx_a].edge_targets[e] = block_ids[j];
                    hg->block_meta[idx_a].edge_weights[e] = edge->weight;
                }
            }

            if (idx_b >= 0 && hg->block_meta[idx_b].num_edges < ZETA_MAX_EDGES_PER_BLOCK) {
                bool found = false;
                for (int e = 0; e < hg->block_meta[idx_b].num_edges; e++) {
                    if (hg->block_meta[idx_b].edge_targets[e] == block_ids[i]) {
                        hg->block_meta[idx_b].edge_weights[e] = edge->weight;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    int e = hg->block_meta[idx_b].num_edges++;
                    hg->block_meta[idx_b].edge_targets[e] = block_ids[i];
                    hg->block_meta[idx_b].edge_weights[e] = edge->weight;
                }
            }
        }
    }
}

void zeta_hologit_decay_edges(zeta_hologit_t* hg) {
    for (int i = 0; i < hg->num_edges; i++) {
        hg->edges[i].weight *= hg->correlation_decay;

        // Prune very weak edges
        if (hg->edges[i].weight < 0.001f) {
            hg->edges[i].weight = 0.0f;
        }
    }

    // Update block edge weights
    for (int i = 0; i < hg->num_blocks; i++) {
        for (int e = 0; e < hg->block_meta[i].num_edges; e++) {
            hg->block_meta[i].edge_weights[e] *= hg->correlation_decay;
        }
    }
}

// ============================================================================
// Summary Evolution (Patching)
// ============================================================================

bool zeta_hologit_should_patch(
    const zeta_hologit_t* hg,
    int64_t block_id
) {
    int idx = find_block_index(hg, block_id);
    if (idx < 0) return false;

    const zeta_block_meta_t* meta = &hg->block_meta[idx];

    // Don't patch if already stable
    if (meta->is_stable) return false;

    // Check if any strongly correlated neighbors exist
    for (int e = 0; e < meta->num_edges; e++) {
        if (meta->edge_weights[e] > 0.5f) {  // Strong correlation
            return true;
        }
    }

    return false;
}

void zeta_hologit_compute_patch(
    const zeta_hologit_t* hg,
    int64_t block_id,
    const float* original_summary,
    const float** neighbor_summaries,
    int summary_dim,
    float* patched_summary_out
) {
    int idx = find_block_index(hg, block_id);
    if (idx < 0) {
        memcpy(patched_summary_out, original_summary, summary_dim * sizeof(float));
        return;
    }

    const zeta_block_meta_t* meta = &hg->block_meta[idx];

    // Weighted average: patched = (original + sum(w_i * neighbor_i)) / (1 + sum(w_i))
    float total_weight = 1.0f;  // Original has weight 1

    memcpy(patched_summary_out, original_summary, summary_dim * sizeof(float));

    for (int e = 0; e < meta->num_edges && neighbor_summaries[e] != NULL; e++) {
        float w = meta->edge_weights[e];
        if (w < 0.1f) continue;  // Skip weak edges

        total_weight += w;
        for (int d = 0; d < summary_dim; d++) {
            patched_summary_out[d] += w * neighbor_summaries[e][d];
        }
    }

    // Normalize
    float inv_weight = 1.0f / total_weight;
    for (int d = 0; d < summary_dim; d++) {
        patched_summary_out[d] *= inv_weight;
    }
}

void zeta_hologit_apply_patch(
    zeta_hologit_t* hg,
    int64_t block_id,
    const float* new_summary,
    int summary_dim,
    const char* reason
) {
    int idx = find_block_index(hg, block_id);
    if (idx < 0) return;

    zeta_block_meta_t* meta = &hg->block_meta[idx];

    // Compute drift from current version
    if (meta->current_version >= 0) {
        float* old = meta->versions[meta->current_version].summary_snapshot;
        meta->summary_drift = compute_summary_distance(old, new_summary, summary_dim);
    }

    // Shift versions if at max
    if (meta->num_versions >= ZETA_MAX_VERSION_HISTORY) {
        free(meta->versions[0].summary_snapshot);
        memmove(&meta->versions[0], &meta->versions[1],
                (ZETA_MAX_VERSION_HISTORY - 1) * sizeof(zeta_version_t));
        meta->num_versions--;
    }

    // Add new version
    int v = meta->num_versions++;
    meta->versions[v].version_id = v;
    meta->versions[v].step_created = 0;  // Should pass current step
    meta->versions[v].summary_snapshot = (float*)malloc(summary_dim * sizeof(float));
    memcpy(meta->versions[v].summary_snapshot, new_summary, summary_dim * sizeof(float));
    strncpy(meta->versions[v].reason, reason, sizeof(meta->versions[v].reason) - 1);

    meta->current_version = v;
    hg->total_patches++;
}

// ============================================================================
// Convergence Detection
// ============================================================================

bool zeta_hologit_is_converged(
    const zeta_hologit_t* hg,
    int64_t block_id
) {
    int idx = find_block_index(hg, block_id);
    if (idx < 0) return false;
    return hg->block_meta[idx].is_stable;
}

void zeta_hologit_check_convergence(zeta_hologit_t* hg, int summary_dim) {
    hg->stable_blocks = 0;

    for (int i = 0; i < hg->num_blocks; i++) {
        zeta_block_meta_t* meta = &hg->block_meta[i];

        if (meta->num_versions < 2) {
            meta->is_stable = false;
            continue;
        }

        // Compare last two versions
        float* v_curr = meta->versions[meta->current_version].summary_snapshot;
        float* v_prev = meta->versions[meta->current_version - 1].summary_snapshot;

        float drift = compute_summary_distance(v_curr, v_prev, summary_dim);
        float norm = compute_summary_distance(v_curr, NULL, summary_dim);
        if (norm < 1e-6f) norm = 1.0f;

        float relative_drift = drift / norm;
        meta->is_stable = (relative_drift < hg->stability_threshold);

        if (meta->is_stable) {
            hg->stable_blocks++;
        }
    }
}

// ============================================================================
// Query Enhancement
// ============================================================================

void zeta_hologit_expand_retrieval_set(
    const zeta_hologit_t* hg,
    const int64_t* initial_blocks,
    int num_initial,
    int64_t* expanded_blocks_out,
    int* num_expanded_out,
    int max_expanded,
    float min_correlation
) {
    // Start with initial blocks
    memcpy(expanded_blocks_out, initial_blocks, num_initial * sizeof(int64_t));
    *num_expanded_out = num_initial;

    // Add correlated neighbors
    for (int i = 0; i < num_initial && *num_expanded_out < max_expanded; i++) {
        int idx = find_block_index(hg, initial_blocks[i]);
        if (idx < 0) continue;

        const zeta_block_meta_t* meta = &hg->block_meta[idx];

        for (int e = 0; e < meta->num_edges; e++) {
            if (meta->edge_weights[e] < min_correlation) continue;
            if (*num_expanded_out >= max_expanded) break;

            int64_t neighbor = meta->edge_targets[e];

            // Check if already in set
            bool found = false;
            for (int j = 0; j < *num_expanded_out; j++) {
                if (expanded_blocks_out[j] == neighbor) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                expanded_blocks_out[(*num_expanded_out)++] = neighbor;
            }
        }
    }
}

// ============================================================================
// Serialization
// ============================================================================

int zeta_hologit_save(const zeta_hologit_t* hg, const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;

    // Header
    uint32_t magic = 0x5A455441;  // "ZETA"
    uint32_t version = 1;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    // Counts
    fwrite(&hg->num_blocks, sizeof(hg->num_blocks), 1, f);
    fwrite(&hg->num_edges, sizeof(hg->num_edges), 1, f);

    // Config
    fwrite(&hg->correlation_decay, sizeof(float), 1, f);
    fwrite(&hg->correlation_boost, sizeof(float), 1, f);
    fwrite(&hg->stability_threshold, sizeof(float), 1, f);

    // Edges (simplified - just the edge list)
    for (int i = 0; i < hg->num_edges; i++) {
        fwrite(&hg->edges[i], sizeof(zeta_edge_t), 1, f);
    }

    // Block metadata (without version snapshots for now)
    for (int i = 0; i < hg->num_blocks; i++) {
        const zeta_block_meta_t* m = &hg->block_meta[i];
        fwrite(&m->block_id, sizeof(m->block_id), 1, f);
        fwrite(&m->num_edges, sizeof(m->num_edges), 1, f);
        fwrite(m->edge_targets, sizeof(int64_t), m->num_edges, f);
        fwrite(m->edge_weights, sizeof(float), m->num_edges, f);
        fwrite(&m->is_stable, sizeof(m->is_stable), 1, f);
    }

    fclose(f);
    return 0;
}

zeta_hologit_t* zeta_hologit_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    uint32_t magic, version;
    fread(&magic, sizeof(magic), 1, f);
    fread(&version, sizeof(version), 1, f);

    if (magic != 0x5A455441 || version != 1) {
        fclose(f);
        return NULL;
    }

    int num_blocks, num_edges;
    fread(&num_blocks, sizeof(num_blocks), 1, f);
    fread(&num_edges, sizeof(num_edges), 1, f);

    zeta_hologit_t* hg = zeta_hologit_init(num_blocks * 2, 4096);  // Assume 4096 dim
    if (!hg) {
        fclose(f);
        return NULL;
    }

    fread(&hg->correlation_decay, sizeof(float), 1, f);
    fread(&hg->correlation_boost, sizeof(float), 1, f);
    fread(&hg->stability_threshold, sizeof(float), 1, f);

    // Read edges
    hg->num_edges = num_edges;
    for (int i = 0; i < num_edges; i++) {
        fread(&hg->edges[i], sizeof(zeta_edge_t), 1, f);
    }

    // Read block metadata
    hg->num_blocks = num_blocks;
    for (int i = 0; i < num_blocks; i++) {
        zeta_block_meta_t* m = &hg->block_meta[i];
        fread(&m->block_id, sizeof(m->block_id), 1, f);
        fread(&m->num_edges, sizeof(m->num_edges), 1, f);
        fread(m->edge_targets, sizeof(int64_t), m->num_edges, f);
        fread(m->edge_weights, sizeof(float), m->num_edges, f);
        fread(&m->is_stable, sizeof(m->is_stable), 1, f);
    }

    fclose(f);
    return hg;
}

// ============================================================================
// Debugging
// ============================================================================

void zeta_hologit_print_stats(const zeta_hologit_t* hg) {
    fprintf(stderr,
        "\n=== HoloGit Statistics ===\n"
        "Blocks:          %d\n"
        "Edges:           %d\n"
        "Co-retrievals:   %lld\n"
        "Patches applied: %lld\n"
        "Stable blocks:   %lld (%.1f%%)\n"
        "==========================\n",
        hg->num_blocks,
        hg->num_edges,
        (long long)hg->total_co_retrievals,
        (long long)hg->total_patches,
        (long long)hg->stable_blocks,
        hg->num_blocks > 0 ? 100.0 * hg->stable_blocks / hg->num_blocks : 0.0
    );
}

void zeta_hologit_print_block(const zeta_hologit_t* hg, int64_t block_id) {
    int idx = find_block_index(hg, block_id);
    if (idx < 0) {
        fprintf(stderr, "Block %lld not found\n", (long long)block_id);
        return;
    }

    const zeta_block_meta_t* m = &hg->block_meta[idx];
    fprintf(stderr,
        "Block %lld:\n"
        "  Versions: %d (current: %d)\n"
        "  Edges: %d\n"
        "  Stable: %s\n"
        "  Drift: %.4f\n",
        (long long)m->block_id,
        m->num_versions, m->current_version,
        m->num_edges,
        m->is_stable ? "yes" : "no",
        m->summary_drift
    );

    for (int e = 0; e < m->num_edges; e++) {
        fprintf(stderr, "    -> %lld (w=%.3f)\n",
                (long long)m->edge_targets[e], m->edge_weights[e]);
    }
}

void zeta_hologit_print_top_edges(const zeta_hologit_t* hg, int n) {
    // Simple O(n*num_edges) top-n selection
    fprintf(stderr, "\n=== Top %d Edges ===\n", n);

    bool* used = (bool*)calloc(hg->num_edges, sizeof(bool));

    for (int i = 0; i < n && i < hg->num_edges; i++) {
        float max_weight = -1.0f;
        int max_idx = -1;

        for (int j = 0; j < hg->num_edges; j++) {
            if (!used[j] && hg->edges[j].weight > max_weight) {
                max_weight = hg->edges[j].weight;
                max_idx = j;
            }
        }

        if (max_idx < 0) break;
        used[max_idx] = true;

        const zeta_edge_t* e = &hg->edges[max_idx];
        fprintf(stderr, "  %lld <-> %lld: %.3f (co-retrieved %lld times)\n",
                (long long)e->block_a, (long long)e->block_b,
                e->weight, (long long)e->co_retrieval_count);
    }

    free(used);
}
