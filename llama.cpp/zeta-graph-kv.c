// Z.E.T.A. Graph-KV Implementation
//
// Pre-computed KV cache storage for graph-based memory retrieval.
// Skip prefill by loading cached KV states directly.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-graph-kv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

// ============================================================================
// FP16 Conversion Helpers
// ============================================================================

static inline uint16_t float_to_fp16(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));

    uint32_t sign = (bits >> 31) & 1;
    int32_t exp = ((bits >> 23) & 0xFF) - 127;
    uint32_t mant = bits & 0x7FFFFF;

    if (exp < -24) {
        // Underflow to zero
        return (uint16_t)(sign << 15);
    } else if (exp < -14) {
        // Denormalized
        mant = (mant | 0x800000) >> (-exp - 14 + 1);
        return (uint16_t)((sign << 15) | (mant >> 13));
    } else if (exp > 15) {
        // Overflow to infinity
        return (uint16_t)((sign << 15) | 0x7C00);
    } else {
        // Normalized
        return (uint16_t)((sign << 15) | ((exp + 15) << 10) | (mant >> 13));
    }
}

static inline float fp16_to_float(uint16_t h) {
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

    float result;
    memcpy(&result, &f, sizeof(result));
    return result;
}

// ============================================================================
// Q8_0 Quantization
// ============================================================================

int zeta_gkv_quantize_q8(
    const float* src,
    zeta_gkv_q8_block_t* dst,
    int n_elements
) {
    int n_blocks = (n_elements + ZETA_GKV_Q8_BLOCK_SIZE - 1) / ZETA_GKV_Q8_BLOCK_SIZE;

    for (int b = 0; b < n_blocks; b++) {
        const float* block_src = src + b * ZETA_GKV_Q8_BLOCK_SIZE;
        zeta_gkv_q8_block_t* block_dst = &dst[b];

        // Find max absolute value for scale
        float amax = 0.0f;
        int block_size = ZETA_GKV_Q8_BLOCK_SIZE;
        if ((b + 1) * ZETA_GKV_Q8_BLOCK_SIZE > n_elements) {
            block_size = n_elements - b * ZETA_GKV_Q8_BLOCK_SIZE;
        }

        for (int i = 0; i < block_size; i++) {
            float abs_val = fabsf(block_src[i]);
            if (abs_val > amax) amax = abs_val;
        }

        // Compute scale (d = max / 127)
        float d = amax / 127.0f;
        float id = (d != 0.0f) ? 127.0f / amax : 0.0f;

        block_dst->scale_fp16 = float_to_fp16(d);

        // Quantize to int8
        for (int i = 0; i < block_size; i++) {
            float v = block_src[i] * id;
            int8_t q = (int8_t)roundf(fminf(127.0f, fmaxf(-128.0f, v)));
            block_dst->qs[i] = q;
        }

        // Zero-pad remainder
        for (int i = block_size; i < ZETA_GKV_Q8_BLOCK_SIZE; i++) {
            block_dst->qs[i] = 0;
        }
    }

    return n_blocks;
}

void zeta_gkv_dequantize_q8(
    const zeta_gkv_q8_block_t* src,
    float* dst,
    int n_blocks
) {
    for (int b = 0; b < n_blocks; b++) {
        const zeta_gkv_q8_block_t* block = &src[b];
        float* block_dst = dst + b * ZETA_GKV_Q8_BLOCK_SIZE;

        float d = fp16_to_float(block->scale_fp16);

        for (int i = 0; i < ZETA_GKV_Q8_BLOCK_SIZE; i++) {
            block_dst[i] = (float)block->qs[i] * d;
        }
    }
}

// ============================================================================
// Initialization / Cleanup
// ============================================================================

zeta_gkv_ctx_t* zeta_gkv_init(
    const struct llama_model* model,
    const char* storage_dir,
    int max_segments
) {
    if (!model || max_segments <= 0) return NULL;

    zeta_gkv_ctx_t* ctx = (zeta_gkv_ctx_t*)calloc(1, sizeof(zeta_gkv_ctx_t));
    if (!ctx) return NULL;

    // Get model dimensions
    ctx->n_layer = llama_model_n_layer(model);
    ctx->n_embd_k = llama_model_n_embd(model);  // Simplified
    ctx->n_embd_v = ctx->n_embd_k;
    ctx->n_head_kv = llama_model_n_head_kv(model);

    // Allocate segment array
    ctx->max_segments = max_segments;
    ctx->segments = (zeta_gkv_segment_t**)calloc(max_segments, sizeof(zeta_gkv_segment_t*));
    if (!ctx->segments) {
        free(ctx);
        return NULL;
    }

    // Storage directory
    if (storage_dir) {
        strncpy(ctx->storage_dir, storage_dir, sizeof(ctx->storage_dir) - 1);
        mkdir(storage_dir, 0755);
    }

    fprintf(stderr, "[GKV] Initialized: %d layers, %d embd, %d kv_heads, max %d segments\n",
            ctx->n_layer, ctx->n_embd_k, ctx->n_head_kv, max_segments);

    return ctx;
}

void zeta_gkv_free(zeta_gkv_ctx_t* ctx) {
    if (!ctx) return;

    // Flush dirty segments
    zeta_gkv_flush(ctx);

    // Free all segments
    for (int i = 0; i < ctx->num_segments; i++) {
        zeta_gkv_segment_free(ctx->segments[i]);
    }
    free(ctx->segments);
    free(ctx);
}

void zeta_gkv_segment_free(zeta_gkv_segment_t* segment) {
    if (!segment) return;

    free(segment->k_blocks);
    free(segment->v_blocks);
    free(segment->rel_positions);
    free(segment->disk_path);
    free(segment);
}

// ============================================================================
// KV Capture (extract from llama context, quantize, store)
// ============================================================================

zeta_gkv_segment_t* zeta_gkv_capture(
    zeta_gkv_ctx_t* gkv_ctx,
    struct llama_context* llama_ctx,
    llama_seq_id seq_id,
    int32_t pos_start,
    int32_t pos_end,
    int64_t node_id
) {
    if (!gkv_ctx || !llama_ctx || pos_end <= pos_start) return NULL;

    int n_tokens = pos_end - pos_start;
    if (n_tokens > ZETA_GKV_MAX_TOKENS) {
        fprintf(stderr, "[GKV] Token count %d exceeds max %d\n", n_tokens, ZETA_GKV_MAX_TOKENS);
        return NULL;
    }

    // Get state size for this sequence
    size_t state_size = llama_state_seq_get_size(llama_ctx, seq_id);
    if (state_size == 0) {
        fprintf(stderr, "[GKV] No state data for seq_id %d\n", seq_id);
        return NULL;
    }

    // Allocate and get state data
    uint8_t* state_data = (uint8_t*)malloc(state_size);
    if (!state_data) return NULL;

    size_t copied = llama_state_seq_get_data(llama_ctx, state_data, state_size, seq_id);
    if (copied == 0) {
        fprintf(stderr, "[GKV] Failed to get state data\n");
        free(state_data);
        return NULL;
    }

    // Create segment
    zeta_gkv_segment_t* segment = (zeta_gkv_segment_t*)calloc(1, sizeof(zeta_gkv_segment_t));
    if (!segment) {
        free(state_data);
        return NULL;
    }

    // Fill header
    segment->header.magic = ZETA_GKV_MAGIC;
    segment->header.version = 1;
    segment->header.n_tokens = n_tokens;
    segment->header.n_layer = gkv_ctx->n_layer;
    segment->header.n_embd_k = gkv_ctx->n_embd_k;
    segment->header.n_embd_v = gkv_ctx->n_embd_v;
    segment->header.n_head_kv = gkv_ctx->n_head_kv;
    segment->header.pos_base = pos_start;

    // Calculate Q8 blocks needed
    int elements_per_layer_k = n_tokens * gkv_ctx->n_embd_k;
    int elements_per_layer_v = n_tokens * gkv_ctx->n_embd_v;
    int k_blocks = (elements_per_layer_k + ZETA_GKV_Q8_BLOCK_SIZE - 1) / ZETA_GKV_Q8_BLOCK_SIZE;
    int v_blocks = (elements_per_layer_v + ZETA_GKV_Q8_BLOCK_SIZE - 1) / ZETA_GKV_Q8_BLOCK_SIZE;

    segment->header.k_blocks_per_layer = k_blocks;
    segment->header.v_blocks_per_layer = v_blocks;

    // Allocate quantized storage
    int total_k_blocks = k_blocks * gkv_ctx->n_layer;
    int total_v_blocks = v_blocks * gkv_ctx->n_layer;

    segment->k_blocks = (zeta_gkv_q8_block_t*)calloc(total_k_blocks, sizeof(zeta_gkv_q8_block_t));
    segment->v_blocks = (zeta_gkv_q8_block_t*)calloc(total_v_blocks, sizeof(zeta_gkv_q8_block_t));
    segment->rel_positions = (int32_t*)malloc(n_tokens * sizeof(int32_t));

    if (!segment->k_blocks || !segment->v_blocks || !segment->rel_positions) {
        zeta_gkv_segment_free(segment);
        free(state_data);
        return NULL;
    }

    // Store relative positions
    for (int i = 0; i < n_tokens; i++) {
        segment->rel_positions[i] = i;  // 0, 1, 2, ... relative to segment start
    }

    // Parse state data and extract K/V
    // Note: This is simplified - actual parsing would match llama-kv-cache.cpp format
    const uint8_t* ptr = state_data;
    const uint8_t* end = state_data + copied;

    // Skip header fields
    uint32_t n_stream;
    memcpy(&n_stream, ptr, sizeof(n_stream));
    ptr += sizeof(n_stream);

    if (n_stream == 0) {
        fprintf(stderr, "[GKV] No streams in state data\n");
        zeta_gkv_segment_free(segment);
        free(state_data);
        return NULL;
    }

    // Read cell count
    uint32_t cell_count;
    memcpy(&cell_count, ptr, sizeof(cell_count));
    ptr += sizeof(cell_count);

    // Skip meta section (positions and seq_ids)
    for (uint32_t i = 0; i < cell_count && ptr < end; i++) {
        ptr += sizeof(int32_t);  // pos
        uint32_t n_seq_id;
        memcpy(&n_seq_id, ptr, sizeof(n_seq_id));
        ptr += sizeof(n_seq_id);
        ptr += n_seq_id * sizeof(int32_t);  // seq_ids
    }

    // Read data section header
    if (ptr + 8 > end) {
        fprintf(stderr, "[GKV] Truncated state data\n");
        zeta_gkv_segment_free(segment);
        free(state_data);
        return NULL;
    }

    uint32_t v_trans, n_layer;
    memcpy(&v_trans, ptr, sizeof(v_trans));
    ptr += sizeof(v_trans);
    memcpy(&n_layer, ptr, sizeof(n_layer));
    ptr += sizeof(n_layer);

    // Allocate temporary float buffer for dequantization
    float* temp_buffer = (float*)malloc(elements_per_layer_k * sizeof(float));
    if (!temp_buffer) {
        zeta_gkv_segment_free(segment);
        free(state_data);
        return NULL;
    }

    // Extract and quantize keys for each layer
    for (uint32_t l = 0; l < n_layer && l < (uint32_t)gkv_ctx->n_layer; l++) {
        if (ptr + 12 > end) break;

        int32_t k_type;
        uint64_t k_size_row;
        memcpy(&k_type, ptr, sizeof(k_type));
        ptr += sizeof(k_type);
        memcpy(&k_size_row, ptr, sizeof(k_size_row));
        ptr += sizeof(k_size_row);

        size_t k_data_size = cell_count * k_size_row;
        if (ptr + k_data_size > end) break;

        // Dequantize source data to float (simplified - assumes FP32/FP16)
        if (k_type == 0) {  // F32
            memcpy(temp_buffer, ptr, elements_per_layer_k * sizeof(float));
        } else if (k_type == 1) {  // F16
            const uint16_t* src16 = (const uint16_t*)ptr;
            for (int i = 0; i < elements_per_layer_k; i++) {
                temp_buffer[i] = fp16_to_float(src16[i]);
            }
        } else {
            // For quantized types, fill with zeros (simplified)
            memset(temp_buffer, 0, elements_per_layer_k * sizeof(float));
        }

        // Quantize to Q8 and store
        zeta_gkv_quantize_q8(temp_buffer, &segment->k_blocks[l * k_blocks], elements_per_layer_k);

        ptr += k_data_size;
    }

    // Extract and quantize values for each layer
    if (!v_trans) {
        for (uint32_t l = 0; l < n_layer && l < (uint32_t)gkv_ctx->n_layer; l++) {
            if (ptr + 12 > end) break;

            int32_t v_type;
            uint64_t v_size_row;
            memcpy(&v_type, ptr, sizeof(v_type));
            ptr += sizeof(v_type);
            memcpy(&v_size_row, ptr, sizeof(v_size_row));
            ptr += sizeof(v_size_row);

            size_t v_data_size = cell_count * v_size_row;
            if (ptr + v_data_size > end) break;

            if (v_type == 0) {  // F32
                memcpy(temp_buffer, ptr, elements_per_layer_v * sizeof(float));
            } else if (v_type == 1) {  // F16
                const uint16_t* src16 = (const uint16_t*)ptr;
                for (int i = 0; i < elements_per_layer_v; i++) {
                    temp_buffer[i] = fp16_to_float(src16[i]);
                }
            } else {
                memset(temp_buffer, 0, elements_per_layer_v * sizeof(float));
            }

            zeta_gkv_quantize_q8(temp_buffer, &segment->v_blocks[l * v_blocks], elements_per_layer_v);

            ptr += v_data_size;
        }
    }

    free(temp_buffer);
    free(state_data);

    // Set metadata
    segment->node_id = node_id;
    segment->created_at = (int64_t)time(NULL);
    segment->last_used = segment->created_at;
    segment->use_count = 0;
    segment->is_dirty = true;

    // Calculate data size for header
    segment->header.data_size = (total_k_blocks + total_v_blocks) * sizeof(zeta_gkv_q8_block_t)
                              + n_tokens * sizeof(int32_t);

    // Add to context
    if (gkv_ctx->num_segments >= gkv_ctx->max_segments) {
        // Evict LRU
        zeta_gkv_evict(gkv_ctx, 1);
    }

    gkv_ctx->segments[gkv_ctx->num_segments++] = segment;
    gkv_ctx->total_saves++;

    fprintf(stderr, "[GKV] Captured segment for node %lld: %d tokens, %d layers, %zu KB\n",
            (long long)node_id, n_tokens, gkv_ctx->n_layer,
            segment->header.data_size / 1024);

    return segment;
}

// ============================================================================
// KV Injection (load cached KV into llama context)
// ============================================================================

int zeta_gkv_inject(
    zeta_gkv_ctx_t* gkv_ctx,
    struct llama_context* llama_ctx,
    zeta_gkv_segment_t* segment,
    llama_seq_id seq_id,
    int32_t injection_pos
) {
    if (!gkv_ctx || !llama_ctx || !segment) return 0;

    // Update usage stats
    segment->last_used = (int64_t)time(NULL);
    segment->use_count++;
    gkv_ctx->total_injections++;

    int n_tokens = segment->header.n_tokens;

    // Allocate temporary buffers for dequantized K/V
    int elements_per_layer = n_tokens * gkv_ctx->n_embd_k;
    int k_blocks = segment->header.k_blocks_per_layer;
    int v_blocks = segment->header.v_blocks_per_layer;

    float* k_buffer = (float*)malloc(elements_per_layer * sizeof(float));
    float* v_buffer = (float*)malloc(elements_per_layer * sizeof(float));
    if (!k_buffer || !v_buffer) {
        free(k_buffer);
        free(v_buffer);
        return 0;
    }

    // For now, we use llama_state_seq_set_data to inject the state
    // This requires constructing a compatible state blob

    // Calculate size needed
    size_t meta_size = 8;  // n_stream + cell_count
    for (int i = 0; i < n_tokens; i++) {
        meta_size += sizeof(int32_t);   // pos
        meta_size += sizeof(uint32_t);  // n_seq_id
        meta_size += sizeof(int32_t);   // seq_id
    }

    size_t data_header_size = 8;  // v_trans + n_layer
    size_t k_data_size = 0;
    size_t v_data_size = 0;

    // We'll use FP16 for the injected data
    size_t row_size = gkv_ctx->n_embd_k * sizeof(uint16_t);
    k_data_size = gkv_ctx->n_layer * (12 + n_tokens * row_size);  // type + size_row + data
    v_data_size = gkv_ctx->n_layer * (12 + n_tokens * row_size);

    size_t total_size = meta_size + data_header_size + k_data_size + v_data_size;
    uint8_t* state_blob = (uint8_t*)calloc(1, total_size);
    if (!state_blob) {
        free(k_buffer);
        free(v_buffer);
        return 0;
    }

    uint8_t* ptr = state_blob;

    // Write meta section
    uint32_t n_stream = 1;
    uint32_t cell_count = n_tokens;
    memcpy(ptr, &n_stream, sizeof(n_stream)); ptr += sizeof(n_stream);
    memcpy(ptr, &cell_count, sizeof(cell_count)); ptr += sizeof(cell_count);

    // Write positions and seq_ids
    for (int i = 0; i < n_tokens; i++) {
        int32_t pos = injection_pos + segment->rel_positions[i];
        uint32_t n_seq_id = 1;
        memcpy(ptr, &pos, sizeof(pos)); ptr += sizeof(pos);
        memcpy(ptr, &n_seq_id, sizeof(n_seq_id)); ptr += sizeof(n_seq_id);
        memcpy(ptr, &seq_id, sizeof(seq_id)); ptr += sizeof(seq_id);
    }

    // Write data header
    uint32_t v_trans = 0;
    uint32_t n_layer = gkv_ctx->n_layer;
    memcpy(ptr, &v_trans, sizeof(v_trans)); ptr += sizeof(v_trans);
    memcpy(ptr, &n_layer, sizeof(n_layer)); ptr += sizeof(n_layer);

    // Write keys per layer
    for (int l = 0; l < gkv_ctx->n_layer; l++) {
        // Dequantize Q8 keys
        zeta_gkv_dequantize_q8(&segment->k_blocks[l * k_blocks], k_buffer, k_blocks);

        // Write layer header
        int32_t k_type = 1;  // F16
        uint64_t k_size_row = row_size;
        memcpy(ptr, &k_type, sizeof(k_type)); ptr += sizeof(k_type);
        memcpy(ptr, &k_size_row, sizeof(k_size_row)); ptr += sizeof(k_size_row);

        // Convert to FP16 and write
        uint16_t* k_fp16 = (uint16_t*)ptr;
        for (int i = 0; i < elements_per_layer; i++) {
            k_fp16[i] = float_to_fp16(k_buffer[i]);
        }
        ptr += n_tokens * row_size;
    }

    // Write values per layer
    for (int l = 0; l < gkv_ctx->n_layer; l++) {
        zeta_gkv_dequantize_q8(&segment->v_blocks[l * v_blocks], v_buffer, v_blocks);

        int32_t v_type = 1;  // F16
        uint64_t v_size_row = row_size;
        memcpy(ptr, &v_type, sizeof(v_type)); ptr += sizeof(v_type);
        memcpy(ptr, &v_size_row, sizeof(v_size_row)); ptr += sizeof(v_size_row);

        uint16_t* v_fp16 = (uint16_t*)ptr;
        for (int i = 0; i < elements_per_layer; i++) {
            v_fp16[i] = float_to_fp16(v_buffer[i]);
        }
        ptr += n_tokens * row_size;
    }

    // Inject state
    size_t actual_size = ptr - state_blob;
    size_t injected = llama_state_seq_set_data(llama_ctx, state_blob, actual_size, seq_id);

    free(state_blob);
    free(k_buffer);
    free(v_buffer);

    if (injected == 0) {
        fprintf(stderr, "[GKV] Failed to inject state for node %lld\n", (long long)segment->node_id);
        return 0;
    }

    // Estimate time saved (assume ~50ms prefill per 100 tokens)
    int64_t saved_ms = (n_tokens * 50) / 100;
    gkv_ctx->prefill_skipped_ms += saved_ms;

    fprintf(stderr, "[GKV] Injected %d tokens for node %lld at pos %d (saved ~%lld ms)\n",
            n_tokens, (long long)segment->node_id, injection_pos, (long long)saved_ms);

    return n_tokens;
}

// ============================================================================
// Segment Lookup
// ============================================================================

zeta_gkv_segment_t* zeta_gkv_find(
    zeta_gkv_ctx_t* ctx,
    int64_t node_id
) {
    if (!ctx) return NULL;

    for (int i = 0; i < ctx->num_segments; i++) {
        if (ctx->segments[i] && ctx->segments[i]->node_id == node_id) {
            return ctx->segments[i];
        }
    }
    return NULL;
}

zeta_gkv_segment_t* zeta_gkv_load(
    zeta_gkv_ctx_t* ctx,
    int64_t node_id
) {
    if (!ctx) return NULL;

    // Check if already in memory
    zeta_gkv_segment_t* segment = zeta_gkv_find(ctx, node_id);
    if (segment) return segment;

    // Try to load from disk
    char path[1024];
    snprintf(path, sizeof(path), "%s/gkv_%lld.bin", ctx->storage_dir, (long long)node_id);

    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;

    // Read header
    zeta_gkv_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    if (header.magic != ZETA_GKV_MAGIC) {
        fprintf(stderr, "[GKV] Invalid magic in %s\n", path);
        fclose(fp);
        return NULL;
    }

    // Allocate segment
    segment = (zeta_gkv_segment_t*)calloc(1, sizeof(zeta_gkv_segment_t));
    if (!segment) {
        fclose(fp);
        return NULL;
    }

    segment->header = header;
    segment->node_id = node_id;

    int total_k_blocks = header.k_blocks_per_layer * header.n_layer;
    int total_v_blocks = header.v_blocks_per_layer * header.n_layer;

    segment->k_blocks = (zeta_gkv_q8_block_t*)malloc(total_k_blocks * sizeof(zeta_gkv_q8_block_t));
    segment->v_blocks = (zeta_gkv_q8_block_t*)malloc(total_v_blocks * sizeof(zeta_gkv_q8_block_t));
    segment->rel_positions = (int32_t*)malloc(header.n_tokens * sizeof(int32_t));

    if (!segment->k_blocks || !segment->v_blocks || !segment->rel_positions) {
        zeta_gkv_segment_free(segment);
        fclose(fp);
        return NULL;
    }

    // Read data
    if (fread(segment->k_blocks, sizeof(zeta_gkv_q8_block_t), total_k_blocks, fp) != (size_t)total_k_blocks ||
        fread(segment->v_blocks, sizeof(zeta_gkv_q8_block_t), total_v_blocks, fp) != (size_t)total_v_blocks ||
        fread(segment->rel_positions, sizeof(int32_t), header.n_tokens, fp) != header.n_tokens) {
        zeta_gkv_segment_free(segment);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    segment->disk_path = strdup(path);
    segment->last_used = (int64_t)time(NULL);
    segment->is_dirty = false;

    // Add to context (evict if needed)
    if (ctx->num_segments >= ctx->max_segments) {
        zeta_gkv_evict(ctx, 1);
    }
    ctx->segments[ctx->num_segments++] = segment;
    ctx->total_loads++;

    fprintf(stderr, "[GKV] Loaded segment for node %lld from disk\n", (long long)node_id);

    return segment;
}

// ============================================================================
// Persistence
// ============================================================================

int zeta_gkv_save(
    zeta_gkv_ctx_t* ctx,
    zeta_gkv_segment_t* segment
) {
    if (!ctx || !segment || !ctx->storage_dir[0]) return -1;

    char path[1024];
    snprintf(path, sizeof(path), "%s/gkv_%lld.bin", ctx->storage_dir, (long long)segment->node_id);

    FILE* fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[GKV] Failed to open %s for writing\n", path);
        return -1;
    }

    int total_k_blocks = segment->header.k_blocks_per_layer * segment->header.n_layer;
    int total_v_blocks = segment->header.v_blocks_per_layer * segment->header.n_layer;

    // Write header
    fwrite(&segment->header, sizeof(segment->header), 1, fp);

    // Write data
    fwrite(segment->k_blocks, sizeof(zeta_gkv_q8_block_t), total_k_blocks, fp);
    fwrite(segment->v_blocks, sizeof(zeta_gkv_q8_block_t), total_v_blocks, fp);
    fwrite(segment->rel_positions, sizeof(int32_t), segment->header.n_tokens, fp);

    fclose(fp);

    free(segment->disk_path);
    segment->disk_path = strdup(path);
    segment->is_dirty = false;

    return 0;
}

int zeta_gkv_flush(zeta_gkv_ctx_t* ctx) {
    if (!ctx) return 0;

    int saved = 0;
    for (int i = 0; i < ctx->num_segments; i++) {
        if (ctx->segments[i] && ctx->segments[i]->is_dirty) {
            if (zeta_gkv_save(ctx, ctx->segments[i]) == 0) {
                saved++;
            }
        }
    }
    return saved;
}

int zeta_gkv_load_all(zeta_gkv_ctx_t* ctx) {
    // Would scan storage_dir for gkv_*.bin files
    // Simplified: just return 0 for now
    (void)ctx;
    return 0;
}

// ============================================================================
// Memory Management
// ============================================================================

int zeta_gkv_evict(zeta_gkv_ctx_t* ctx, int count) {
    if (!ctx || count <= 0) return 0;

    int evicted = 0;

    while (evicted < count && ctx->num_segments > 0) {
        // Find LRU segment
        int lru_idx = 0;
        int64_t oldest = ctx->segments[0] ? ctx->segments[0]->last_used : 0;

        for (int i = 1; i < ctx->num_segments; i++) {
            if (ctx->segments[i] && ctx->segments[i]->last_used < oldest) {
                oldest = ctx->segments[i]->last_used;
                lru_idx = i;
            }
        }

        // Save if dirty
        if (ctx->segments[lru_idx] && ctx->segments[lru_idx]->is_dirty) {
            zeta_gkv_save(ctx, ctx->segments[lru_idx]);
        }

        // Free and remove
        zeta_gkv_segment_free(ctx->segments[lru_idx]);

        // Shift array
        for (int i = lru_idx; i < ctx->num_segments - 1; i++) {
            ctx->segments[i] = ctx->segments[i + 1];
        }
        ctx->num_segments--;

        evicted++;
    }

    return evicted;
}

// ============================================================================
// Statistics
// ============================================================================

void zeta_gkv_get_stats(
    const zeta_gkv_ctx_t* ctx,
    zeta_gkv_stats_t* stats
) {
    if (!ctx || !stats) return;

    memset(stats, 0, sizeof(*stats));
    stats->num_segments = ctx->num_segments;
    stats->total_saves = ctx->total_saves;
    stats->total_loads = ctx->total_loads;
    stats->total_injections = ctx->total_injections;
    stats->prefill_skipped_ms = ctx->prefill_skipped_ms;

    // Calculate memory usage
    for (int i = 0; i < ctx->num_segments; i++) {
        if (ctx->segments[i]) {
            stats->total_bytes += sizeof(zeta_gkv_segment_t);
            stats->total_bytes += ctx->segments[i]->header.data_size;
        }
    }
}

void zeta_gkv_debug_segment(const zeta_gkv_segment_t* segment) {
    if (!segment) {
        fprintf(stderr, "[GKV] NULL segment\n");
        return;
    }

    fprintf(stderr, "[GKV] Segment node=%lld tokens=%d layers=%d embd=%d\n",
            (long long)segment->node_id,
            segment->header.n_tokens,
            segment->header.n_layer,
            segment->header.n_embd_k);
    fprintf(stderr, "      k_blocks=%d v_blocks=%d pos_base=%d\n",
            segment->header.k_blocks_per_layer,
            segment->header.v_blocks_per_layer,
            segment->header.pos_base);
    fprintf(stderr, "      data_size=%d KB uses=%d dirty=%d\n",
            segment->header.data_size / 1024,
            segment->use_count,
            segment->is_dirty);
}
