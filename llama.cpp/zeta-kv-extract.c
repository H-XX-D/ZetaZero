// Z.E.T.A. KV Cache Extraction Implementation
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-kv-extract.h"
#include "zeta-integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// Internal Helpers
// ============================================================================

// Dequantize various ggml types to float
// Note: This is a simplified version - full implementation would use ggml's
// dequantization functions directly
static void dequantize_row(const void* src, float* dst, int n, int type) {
    switch (type) {
        case 0:  // GGML_TYPE_F32
            memcpy(dst, src, n * sizeof(float));
            break;

        case 1: {  // GGML_TYPE_F16
            const uint16_t* src16 = (const uint16_t*)src;
            for (int i = 0; i < n; i++) {
                // IEEE 754 half-precision to float conversion
                uint16_t h = src16[i];
                uint32_t sign = (h >> 15) & 1;
                uint32_t exp = (h >> 10) & 0x1F;
                uint32_t mant = h & 0x3FF;

                uint32_t f;
                if (exp == 0) {
                    if (mant == 0) {
                        f = sign << 31;
                    } else {
                        // Denormalized
                        exp = 1;
                        while (!(mant & 0x400)) {
                            mant <<= 1;
                            exp--;
                        }
                        mant &= 0x3FF;
                        f = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13);
                    }
                } else if (exp == 31) {
                    f = (sign << 31) | 0x7F800000 | (mant << 13);
                } else {
                    f = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13);
                }
                memcpy(&dst[i], &f, sizeof(float));
            }
            break;
        }

        case 2: {  // GGML_TYPE_Q4_0 (simplified)
            // Q4_0 format: blocks of 32 elements
            // Each block: 1 float16 scale + 16 bytes of 4-bit values
            const int block_size = 32;
            const int n_blocks = (n + block_size - 1) / block_size;
            const uint8_t* ptr = (const uint8_t*)src;

            for (int b = 0; b < n_blocks; b++) {
                // Read scale (f16)
                uint16_t scale_h;
                memcpy(&scale_h, ptr, 2);
                ptr += 2;

                // Convert scale to float
                float scale;
                uint32_t sign = (scale_h >> 15) & 1;
                uint32_t exp = (scale_h >> 10) & 0x1F;
                uint32_t mant = scale_h & 0x3FF;
                if (exp == 0) {
                    scale = sign ? -0.0f : 0.0f;
                } else if (exp == 31) {
                    scale = sign ? -INFINITY : INFINITY;
                } else {
                    uint32_t f = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13);
                    memcpy(&scale, &f, sizeof(float));
                }

                // Read and dequantize 32 4-bit values
                for (int j = 0; j < 16 && (b * block_size + j * 2) < n; j++) {
                    uint8_t qval = ptr[j];
                    int idx = b * block_size + j * 2;

                    // Low nibble
                    if (idx < n) {
                        int q0 = (qval & 0x0F) - 8;
                        dst[idx] = q0 * scale;
                    }
                    // High nibble
                    if (idx + 1 < n) {
                        int q1 = ((qval >> 4) & 0x0F) - 8;
                        dst[idx + 1] = q1 * scale;
                    }
                }
                ptr += 16;
            }
            break;
        }

        case 8: {  // GGML_TYPE_Q8_0 (simplified)
            // Q8_0 format: blocks of 32 elements
            // Each block: 1 float16 scale + 32 int8 values
            const int block_size = 32;
            const int n_blocks = (n + block_size - 1) / block_size;
            const uint8_t* ptr = (const uint8_t*)src;

            for (int b = 0; b < n_blocks; b++) {
                // Read scale (f16)
                uint16_t scale_h;
                memcpy(&scale_h, ptr, 2);
                ptr += 2;

                // Convert scale to float
                float scale;
                uint32_t sign = (scale_h >> 15) & 1;
                uint32_t exp = (scale_h >> 10) & 0x1F;
                uint32_t mant = scale_h & 0x3FF;
                if (exp == 0) {
                    scale = sign ? -0.0f : 0.0f;
                } else if (exp == 31) {
                    scale = sign ? -INFINITY : INFINITY;
                } else {
                    uint32_t f = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13);
                    memcpy(&scale, &f, sizeof(float));
                }

                // Read and dequantize 32 int8 values
                const int8_t* qvals = (const int8_t*)ptr;
                for (int j = 0; j < 32 && (b * block_size + j) < n; j++) {
                    int idx = b * block_size + j;
                    dst[idx] = qvals[j] * scale;
                }
                ptr += 32;
            }
            break;
        }

        default:
            // For unsupported types, fill with zeros
            fprintf(stderr, "zeta_kv: unsupported ggml type %d, using zeros\n", type);
            memset(dst, 0, n * sizeof(float));
            break;
    }
}

// ============================================================================
// State Parsing
// ============================================================================

// Parse the serialized state format from llama_state_seq_get_data
// This matches the format in llama-kv-cache.cpp::state_write
static zeta_kv_data_t* parse_state_data(
    const uint8_t* data,
    size_t data_size,
    const struct llama_model* model
) {
    if (!data || data_size < 8) return NULL;

    const uint8_t* ptr = data;
    const uint8_t* end = data + data_size;

    // Read n_stream
    uint32_t n_stream;
    memcpy(&n_stream, ptr, sizeof(n_stream));
    ptr += sizeof(n_stream);

    if (n_stream == 0) return NULL;

    // For simplicity, we only handle stream 0
    // Read cell_count for stream 0
    uint32_t cell_count;
    memcpy(&cell_count, ptr, sizeof(cell_count));
    ptr += sizeof(cell_count);

    if (cell_count == 0) return NULL;

    // Allocate result
    zeta_kv_data_t* result = (zeta_kv_data_t*)calloc(1, sizeof(zeta_kv_data_t));
    if (!result) return NULL;

    result->n_tokens = cell_count;
    result->positions = (int32_t*)malloc(cell_count * sizeof(int32_t));

    // Parse meta section (positions and seq_ids)
    for (uint32_t i = 0; i < cell_count; i++) {
        if (ptr + 8 > end) goto error;

        int32_t pos;
        uint32_t n_seq_id;
        memcpy(&pos, ptr, sizeof(pos));
        ptr += sizeof(pos);
        memcpy(&n_seq_id, ptr, sizeof(n_seq_id));
        ptr += sizeof(n_seq_id);

        result->positions[i] = pos;

        // Skip seq_ids
        ptr += n_seq_id * sizeof(int32_t);
    }

    // Parse data section
    if (ptr + 8 > end) goto error;

    uint32_t v_trans, n_layer;
    memcpy(&v_trans, ptr, sizeof(v_trans));
    ptr += sizeof(v_trans);
    memcpy(&n_layer, ptr, sizeof(n_layer));
    ptr += sizeof(n_layer);

    result->n_layer = n_layer;

    // Get dimensions from model
    int n_embd_k = llama_model_n_embd(model);  // Simplified - actually per-layer
    int n_embd_v = n_embd_k;  // Usually same

    result->n_embd_k = n_embd_k;
    result->n_embd_v = n_embd_v;

    // Allocate layer arrays
    result->keys = (float**)calloc(n_layer, sizeof(float*));
    result->values = (float**)calloc(n_layer, sizeof(float*));

    // Parse keys for each layer
    for (uint32_t l = 0; l < n_layer; l++) {
        if (ptr + 12 > end) goto error;

        int32_t k_type;
        uint64_t k_size_row;
        memcpy(&k_type, ptr, sizeof(k_type));
        ptr += sizeof(k_type);
        memcpy(&k_size_row, ptr, sizeof(k_size_row));
        ptr += sizeof(k_size_row);

        // Allocate and dequantize
        size_t k_data_size = cell_count * k_size_row;
        if (ptr + k_data_size > end) goto error;

        result->keys[l] = (float*)malloc(cell_count * n_embd_k * sizeof(float));
        if (!result->keys[l]) goto error;

        // Dequantize each row
        for (uint32_t t = 0; t < cell_count; t++) {
            dequantize_row(ptr + t * k_size_row, result->keys[l] + t * n_embd_k, n_embd_k, k_type);
        }
        ptr += k_data_size;
    }

    // Parse values for each layer
    if (!v_trans) {
        // Non-transposed values
        for (uint32_t l = 0; l < n_layer; l++) {
            if (ptr + 12 > end) goto error;

            int32_t v_type;
            uint64_t v_size_row;
            memcpy(&v_type, ptr, sizeof(v_type));
            ptr += sizeof(v_type);
            memcpy(&v_size_row, ptr, sizeof(v_size_row));
            ptr += sizeof(v_size_row);

            size_t v_data_size = cell_count * v_size_row;
            if (ptr + v_data_size > end) goto error;

            result->values[l] = (float*)malloc(cell_count * n_embd_v * sizeof(float));
            if (!result->values[l]) goto error;

            for (uint32_t t = 0; t < cell_count; t++) {
                dequantize_row(ptr + t * v_size_row, result->values[l] + t * n_embd_v, n_embd_v, v_type);
            }
            ptr += v_data_size;
        }
    } else {
        // Transposed values - more complex parsing
        // For POC, we'll use a simplified approach
        for (uint32_t l = 0; l < n_layer; l++) {
            if (ptr + 12 > end) goto error;

            int32_t v_type;
            uint32_t v_size_el, n_embd_v_gqa;
            memcpy(&v_type, ptr, sizeof(v_type));
            ptr += sizeof(v_type);
            memcpy(&v_size_el, ptr, sizeof(v_size_el));
            ptr += sizeof(v_size_el);
            memcpy(&n_embd_v_gqa, ptr, sizeof(n_embd_v_gqa));
            ptr += sizeof(n_embd_v_gqa);

            result->values[l] = (float*)calloc(cell_count * n_embd_v, sizeof(float));
            if (!result->values[l]) goto error;

            // Parse transposed data (row by row, then cell by cell)
            for (uint32_t j = 0; j < n_embd_v_gqa && j < (uint32_t)n_embd_v; j++) {
                size_t row_size = cell_count * v_size_el;
                if (ptr + row_size > end) goto error;

                // Dequantize this row
                float* temp = (float*)malloc(cell_count * sizeof(float));
                if (!temp) goto error;

                dequantize_row(ptr, temp, cell_count, v_type);

                // Transpose into result
                for (uint32_t t = 0; t < cell_count; t++) {
                    result->values[l][t * n_embd_v + j] = temp[t];
                }
                free(temp);
                ptr += row_size;
            }
        }
    }

    return result;

error:
    zeta_kv_data_free(result);
    return NULL;
}

// ============================================================================
// Public API
// ============================================================================

zeta_kv_data_t* zeta_extract_kv_cache(
    struct llama_context* ctx,
    llama_seq_id seq_id
) {
    if (!ctx) return NULL;

    const struct llama_model* model = llama_get_model(ctx);

    // Get state size
    size_t state_size = llama_state_seq_get_size(ctx, seq_id);
    if (state_size == 0) {
        fprintf(stderr, "zeta_kv: no state data for seq_id %d\n", seq_id);
        return NULL;
    }

    // Allocate and get state data
    uint8_t* state_data = (uint8_t*)malloc(state_size);
    if (!state_data) return NULL;

    size_t copied = llama_state_seq_get_data(ctx, state_data, state_size, seq_id);
    if (copied == 0) {
        fprintf(stderr, "zeta_kv: failed to get state data\n");
        free(state_data);
        return NULL;
    }

    // Parse state data
    zeta_kv_data_t* result = parse_state_data(state_data, copied, model);
    free(state_data);

    return result;
}

zeta_kv_data_t* zeta_extract_kv_range(
    struct llama_context* ctx,
    llama_seq_id seq_id,
    int32_t pos_start,
    int32_t pos_end
) {
    // Extract full cache first
    zeta_kv_data_t* full = zeta_extract_kv_cache(ctx, seq_id);
    if (!full) return NULL;

    // Count tokens in range
    int count = 0;
    for (int i = 0; i < full->n_tokens; i++) {
        if (full->positions[i] >= pos_start && full->positions[i] < pos_end) {
            count++;
        }
    }

    if (count == 0) {
        zeta_kv_data_free(full);
        return NULL;
    }

    // Allocate result
    zeta_kv_data_t* result = (zeta_kv_data_t*)calloc(1, sizeof(zeta_kv_data_t));
    if (!result) {
        zeta_kv_data_free(full);
        return NULL;
    }

    result->n_tokens = count;
    result->n_layer = full->n_layer;
    result->n_embd_k = full->n_embd_k;
    result->n_embd_v = full->n_embd_v;
    result->positions = (int32_t*)malloc(count * sizeof(int32_t));
    result->keys = (float**)calloc(full->n_layer, sizeof(float*));
    result->values = (float**)calloc(full->n_layer, sizeof(float*));

    for (int l = 0; l < full->n_layer; l++) {
        result->keys[l] = (float*)malloc(count * full->n_embd_k * sizeof(float));
        result->values[l] = (float*)malloc(count * full->n_embd_v * sizeof(float));
    }

    // Copy filtered data
    int idx = 0;
    for (int i = 0; i < full->n_tokens; i++) {
        if (full->positions[i] >= pos_start && full->positions[i] < pos_end) {
            result->positions[idx] = full->positions[i];

            for (int l = 0; l < full->n_layer; l++) {
                memcpy(result->keys[l] + idx * full->n_embd_k,
                       full->keys[l] + i * full->n_embd_k,
                       full->n_embd_k * sizeof(float));
                memcpy(result->values[l] + idx * full->n_embd_v,
                       full->values[l] + i * full->n_embd_v,
                       full->n_embd_v * sizeof(float));
            }
            idx++;
        }
    }

    zeta_kv_data_free(full);
    return result;
}

void zeta_kv_data_free(zeta_kv_data_t* data) {
    if (!data) return;

    if (data->keys) {
        for (int l = 0; l < data->n_layer; l++) {
            free(data->keys[l]);
        }
        free(data->keys);
    }

    if (data->values) {
        for (int l = 0; l < data->n_layer; l++) {
            free(data->values[l]);
        }
        free(data->values);
    }

    free(data->positions);
    free(data);
}

void zeta_get_kv_dimensions(
    const struct llama_model* model,
    int* n_layer_out,
    int* n_embd_k_out,
    int* n_embd_v_out,
    int* n_head_kv_out
) {
    if (n_layer_out) *n_layer_out = llama_model_n_layer(model);
    if (n_embd_k_out) *n_embd_k_out = llama_model_n_embd(model);
    if (n_embd_v_out) *n_embd_v_out = llama_model_n_embd(model);
    if (n_head_kv_out) *n_head_kv_out = llama_model_n_head_kv(model);
}

void zeta_compute_mean_kv(
    const zeta_kv_data_t* data,
    float* mean_k_out,
    float* mean_v_out
) {
    if (!data || data->n_layer == 0) return;

    int total_k = data->n_tokens * data->n_embd_k;
    int total_v = data->n_tokens * data->n_embd_v;

    if (mean_k_out) {
        memset(mean_k_out, 0, total_k * sizeof(float));
        for (int l = 0; l < data->n_layer; l++) {
            for (int i = 0; i < total_k; i++) {
                mean_k_out[i] += data->keys[l][i];
            }
        }
        float scale = 1.0f / data->n_layer;
        for (int i = 0; i < total_k; i++) {
            mean_k_out[i] *= scale;
        }
    }

    if (mean_v_out) {
        memset(mean_v_out, 0, total_v * sizeof(float));
        for (int l = 0; l < data->n_layer; l++) {
            for (int i = 0; i < total_v; i++) {
                mean_v_out[i] += data->values[l][i];
            }
        }
        float scale = 1.0f / data->n_layer;
        for (int i = 0; i < total_v; i++) {
            mean_v_out[i] *= scale;
        }
    }
}

int64_t zeta_sublimate_kv_cache(
    void* zeta_ptr,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int layer_idx,
    int32_t pos_start,
    int32_t pos_end
) {
    if (!zeta_ptr || !llama_ctx) return -1;

    zeta_context_t* zeta = (zeta_context_t*)zeta_ptr;

    // Extract KV cache
    zeta_kv_data_t* kv_data;
    if (pos_start >= 0 && pos_end > pos_start) {
        kv_data = zeta_extract_kv_range(llama_ctx, seq_id, pos_start, pos_end);
    } else {
        kv_data = zeta_extract_kv_cache(llama_ctx, seq_id);
    }

    if (!kv_data) {
        fprintf(stderr, "zeta_sublimate_kv: failed to extract KV cache\n");
        return -1;
    }

    // Determine which keys/values to use
    float* keys_to_use;
    float* values_to_use;
    bool allocated = false;

    if (layer_idx >= 0 && layer_idx < kv_data->n_layer) {
        // Use specific layer
        keys_to_use = kv_data->keys[layer_idx];
        values_to_use = kv_data->values[layer_idx];
    } else {
        // Use mean across all layers
        int total_k = kv_data->n_tokens * kv_data->n_embd_k;
        int total_v = kv_data->n_tokens * kv_data->n_embd_v;

        keys_to_use = (float*)malloc(total_k * sizeof(float));
        values_to_use = (float*)malloc(total_v * sizeof(float));
        if (!keys_to_use || !values_to_use) {
            free(keys_to_use);
            free(values_to_use);
            zeta_kv_data_free(kv_data);
            return -1;
        }

        zeta_compute_mean_kv(kv_data, keys_to_use, values_to_use);
        allocated = true;
    }

    // Get embeddings to use as summary (same space as query vectors)
    const float* embeddings = llama_get_embeddings(llama_ctx);
    float* summary = NULL;

    if (embeddings) {
        // Use embeddings directly as summary vector
        // This ensures query and summary are in the same representation space
        int n_embd = kv_data->n_embd_k;
        summary = (float*)malloc(n_embd * sizeof(float));
        if (summary) {
            memcpy(summary, embeddings, n_embd * sizeof(float));
        }
    }

    // Sublimate to memory block (with embedding-based summary)
    int64_t block_id = zeta_sublimate_block_ext(
        zeta->memory,
        keys_to_use,
        values_to_use,
        summary,  // Use embeddings as summary
        kv_data->n_tokens,
        kv_data->positions[0]
    );

    free(summary);

    // Cleanup
    if (allocated) {
        free(keys_to_use);
        free(values_to_use);
    }
    zeta_kv_data_free(kv_data);

    return block_id;
}

int64_t zeta_sublimate_kv_cache_with_summary(
    void* zeta_ptr,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int layer_idx,
    int32_t pos_start,
    int32_t pos_end,
    const float* summary_in,
    int summary_dim
) {
    if (!zeta_ptr || !llama_ctx) return -1;
    (void)summary_dim;  // Validated by caller

    zeta_context_t* zeta = (zeta_context_t*)zeta_ptr;

    // Extract KV cache
    zeta_kv_data_t* kv_data;
    if (pos_start >= 0 && pos_end > pos_start) {
        kv_data = zeta_extract_kv_range(llama_ctx, seq_id, pos_start, pos_end);
    } else {
        kv_data = zeta_extract_kv_cache(llama_ctx, seq_id);
    }

    if (!kv_data) {
        fprintf(stderr, "zeta_sublimate_kv_with_summary: failed to extract KV cache\n");
        return -1;
    }

    // Determine which keys/values to use
    float* keys_to_use;
    float* values_to_use;
    bool allocated = false;

    if (layer_idx >= 0 && layer_idx < kv_data->n_layer) {
        // Use specific layer
        keys_to_use = kv_data->keys[layer_idx];
        values_to_use = kv_data->values[layer_idx];
    } else {
        // Use mean across all layers
        int total_k = kv_data->n_tokens * kv_data->n_embd_k;
        int total_v = kv_data->n_tokens * kv_data->n_embd_v;

        keys_to_use = (float*)malloc(total_k * sizeof(float));
        values_to_use = (float*)malloc(total_v * sizeof(float));
        if (!keys_to_use || !values_to_use) {
            free(keys_to_use);
            free(values_to_use);
            zeta_kv_data_free(kv_data);
            return -1;
        }

        zeta_compute_mean_kv(kv_data, keys_to_use, values_to_use);
        allocated = true;
    }

    // Sublimate to memory block with provided summary
    // This ensures query and summary are in the same representation space
    int64_t block_id = zeta_sublimate_block_ext(
        zeta->memory,
        keys_to_use,
        values_to_use,
        summary_in,  // Use caller-provided summary (e.g., logits-derived)
        kv_data->n_tokens,
        kv_data->positions[0]
    );

    // Cleanup
    if (allocated) {
        free(keys_to_use);
        free(values_to_use);
    }
    zeta_kv_data_free(kv_data);

    return block_id;
}
