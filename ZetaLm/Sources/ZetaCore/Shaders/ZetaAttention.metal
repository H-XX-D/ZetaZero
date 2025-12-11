/**
 * ZetaAttention.metal
 * Attention mechanism kernels: RoPE, Softmax, KV Cache
 * Ported from llama.cpp attention implementation
 */

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// RMSNorm: x = x * rsqrt(mean(x^2) + eps) * weight
// ============================================================================

kernel void zeta_rmsnorm(
    device const float* x         [[buffer(0)]],
    device const float* weight    [[buffer(1)]],
    device float* out             [[buffer(2)]],
    constant uint& dim            [[buffer(3)]],
    constant float& eps           [[buffer(4)]],
    uint gid                      [[thread_position_in_grid]]
) {
    // Compute sum of squares (single thread for now, optimize later)
    if (gid != 0) return;

    float sum_sq = 0.0f;
    for (uint i = 0; i < dim; i++) {
        sum_sq += x[i] * x[i];
    }

    float scale = rsqrt(sum_sq / float(dim) + eps);

    for (uint i = 0; i < dim; i++) {
        out[i] = x[i] * scale * weight[i];
    }
}

// Parallel RMSNorm using threadgroup reduction
kernel void zeta_rmsnorm_parallel(
    device const float* x         [[buffer(0)]],
    device const float* weight    [[buffer(1)]],
    device float* out             [[buffer(2)]],
    constant uint& dim            [[buffer(3)]],
    constant float& eps           [[buffer(4)]],
    threadgroup float* shared     [[threadgroup(0)]],
    uint tid                      [[thread_index_in_threadgroup]],
    uint tg_size                  [[threads_per_threadgroup]]
) {
    // Phase 1: Each thread computes partial sum of squares
    float local_sum = 0.0f;
    for (uint i = tid; i < dim; i += tg_size) {
        local_sum += x[i] * x[i];
    }
    shared[tid] = local_sum;

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Phase 2: Reduction
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] += shared[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    // Phase 3: Compute scale and apply
    float scale = rsqrt(shared[0] / float(dim) + eps);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint i = tid; i < dim; i += tg_size) {
        out[i] = x[i] * scale * weight[i];
    }
}

// ============================================================================
// RoPE: Rotary Position Embedding
// Applied to Q and K vectors
// ============================================================================

kernel void zeta_rope(
    device float* q               [[buffer(0)]],
    device float* k               [[buffer(1)]],
    constant uint& head_dim       [[buffer(2)]],
    constant uint& n_heads        [[buffer(3)]],
    constant uint& n_kv_heads     [[buffer(4)]],
    constant uint& position       [[buffer(5)]],
    constant float& theta_base    [[buffer(6)]],
    uint gid                      [[thread_position_in_grid]]
) {
    // Each thread handles one head
    uint head = gid;
    if (head >= n_heads) return;

    // Apply RoPE to Q
    device float* q_head = q + head * head_dim;

    for (uint i = 0; i < head_dim; i += 2) {
        float freq = 1.0f / pow(theta_base, float(i) / float(head_dim));
        float angle = float(position) * freq;

        float cos_val = cos(angle);
        float sin_val = sin(angle);

        float q0 = q_head[i];
        float q1 = q_head[i + 1];

        q_head[i]     = q0 * cos_val - q1 * sin_val;
        q_head[i + 1] = q0 * sin_val + q1 * cos_val;
    }

    // Apply RoPE to K (only for KV heads)
    if (head < n_kv_heads) {
        device float* k_head = k + head * head_dim;

        for (uint i = 0; i < head_dim; i += 2) {
            float freq = 1.0f / pow(theta_base, float(i) / float(head_dim));
            float angle = float(position) * freq;

            float cos_val = cos(angle);
            float sin_val = sin(angle);

            float k0 = k_head[i];
            float k1 = k_head[i + 1];

            k_head[i]     = k0 * cos_val - k1 * sin_val;
            k_head[i + 1] = k0 * sin_val + k1 * cos_val;
        }
    }
}

// ============================================================================
// Softmax: Numerically stable softmax over attention scores
// ============================================================================

kernel void zeta_softmax(
    device float* scores          [[buffer(0)]],
    constant uint& seq_len        [[buffer(1)]],
    constant uint& n_heads        [[buffer(2)]],
    uint gid                      [[thread_position_in_grid]]
) {
    // One thread per head
    if (gid >= n_heads) return;

    uint head_offset = gid * seq_len;

    // Find max for numerical stability
    float max_val = scores[head_offset];
    for (uint i = 1; i < seq_len; i++) {
        max_val = max(max_val, scores[head_offset + i]);
    }

    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (uint i = 0; i < seq_len; i++) {
        scores[head_offset + i] = exp(scores[head_offset + i] - max_val);
        sum += scores[head_offset + i];
    }

    // Normalize
    float inv_sum = 1.0f / sum;
    for (uint i = 0; i < seq_len; i++) {
        scores[head_offset + i] *= inv_sum;
    }
}

// ============================================================================
// Attention Score: Q @ K^T / sqrt(head_dim)
// ============================================================================

kernel void zeta_attn_scores(
    device const float* q         [[buffer(0)]],  // [n_heads, head_dim]
    device const float* k_cache   [[buffer(1)]],  // [seq_len, n_kv_heads, head_dim]
    device float* scores          [[buffer(2)]],  // [n_heads, seq_len]
    constant uint& head_dim       [[buffer(3)]],
    constant uint& n_heads        [[buffer(4)]],
    constant uint& n_kv_heads     [[buffer(5)]],
    constant uint& seq_len        [[buffer(6)]],
    constant float& scale         [[buffer(7)]],
    uint2 gid                     [[thread_position_in_grid]]
) {
    uint head = gid.x;
    uint pos = gid.y;

    if (head >= n_heads || pos >= seq_len) return;

    // GQA: map query head to KV head
    uint kv_head = head * n_kv_heads / n_heads;

    device const float* q_head = q + head * head_dim;
    device const float* k_head = k_cache + pos * n_kv_heads * head_dim + kv_head * head_dim;

    // Dot product
    float dot = 0.0f;
    for (uint i = 0; i < head_dim; i++) {
        dot += q_head[i] * k_head[i];
    }

    scores[head * seq_len + pos] = dot * scale;
}

// ============================================================================
// Attention Output: softmax(scores) @ V
// ============================================================================

kernel void zeta_attn_output(
    device const float* scores    [[buffer(0)]],  // [n_heads, seq_len]
    device const float* v_cache   [[buffer(1)]],  // [seq_len, n_kv_heads, head_dim]
    device float* out             [[buffer(2)]],  // [n_heads, head_dim]
    constant uint& head_dim       [[buffer(3)]],
    constant uint& n_heads        [[buffer(4)]],
    constant uint& n_kv_heads     [[buffer(5)]],
    constant uint& seq_len        [[buffer(6)]],
    uint2 gid                     [[thread_position_in_grid]]
) {
    uint head = gid.x;
    uint d = gid.y;

    if (head >= n_heads || d >= head_dim) return;

    uint kv_head = head * n_kv_heads / n_heads;

    float sum = 0.0f;
    for (uint pos = 0; pos < seq_len; pos++) {
        float score = scores[head * seq_len + pos];
        float v_val = v_cache[pos * n_kv_heads * head_dim + kv_head * head_dim + d];
        sum += score * v_val;
    }

    out[head * head_dim + d] = sum;
}

// ============================================================================
// Element-wise Add: out = a + b
// ============================================================================

kernel void zeta_add(
    device const float* a         [[buffer(0)]],
    device const float* b         [[buffer(1)]],
    device float* out             [[buffer(2)]],
    constant uint& size           [[buffer(3)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= size) return;
    out[gid] = a[gid] + b[gid];
}

// ============================================================================
// Embedding Lookup
// ============================================================================

kernel void zeta_embedding(
    device const float* table     [[buffer(0)]],  // [vocab, dim]
    device float* out             [[buffer(1)]],  // [dim]
    constant uint& token          [[buffer(2)]],
    constant uint& dim            [[buffer(3)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= dim) return;
    out[gid] = table[token * dim + gid];
}

// ============================================================================
// Copy: dest = src (for KV cache storage)
// ============================================================================

kernel void zeta_copy(
    device const float* src       [[buffer(0)]],
    device float* dest            [[buffer(1)]],
    constant uint& size           [[buffer(2)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= size) return;
    dest[gid] = src[gid];
}
