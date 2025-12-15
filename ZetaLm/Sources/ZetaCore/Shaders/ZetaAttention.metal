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

// Legacy kernel (computes frequencies on the fly)
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

// OPTIMIZED: RoPE with precomputed frequency LUT (avoids pow() per token)
kernel void zeta_rope_lut(
    device float* q               [[buffer(0)]],
    device float* k               [[buffer(1)]],
    device const float* freq_lut  [[buffer(2)]],  // Precomputed: freq[i] = 1/pow(theta, 2i/head_dim)
    constant uint& head_dim       [[buffer(3)]],
    constant uint& n_heads        [[buffer(4)]],
    constant uint& n_kv_heads     [[buffer(5)]],
    constant uint& position       [[buffer(6)]],
    uint gid                      [[thread_position_in_grid]]
) {
    uint head = gid;
    if (head >= n_heads) return;

    device float* q_head = q + head * head_dim;
    uint half_dim = head_dim / 2;

    // Apply RoPE to Q using precomputed frequencies
    for (uint i = 0; i < half_dim; i++) {
        float freq = freq_lut[i];
        float angle = float(position) * freq;

        float cos_val = cos(angle);
        float sin_val = sin(angle);

        uint idx = i * 2;
        float q0 = q_head[idx];
        float q1 = q_head[idx + 1];

        q_head[idx]     = q0 * cos_val - q1 * sin_val;
        q_head[idx + 1] = q0 * sin_val + q1 * cos_val;
    }

    // Apply RoPE to K (only for KV heads)
    if (head < n_kv_heads) {
        device float* k_head = k + head * head_dim;

        for (uint i = 0; i < half_dim; i++) {
            float freq = freq_lut[i];
            float angle = float(position) * freq;

            float cos_val = cos(angle);
            float sin_val = sin(angle);

            uint idx = i * 2;
            float k0 = k_head[idx];
            float k1 = k_head[idx + 1];

            k_head[idx]     = k0 * cos_val - k1 * sin_val;
            k_head[idx + 1] = k0 * sin_val + k1 * cos_val;
        }
    }
}

// ============================================================================
// Softmax: Numerically stable softmax over attention scores
// ============================================================================

// Legacy sequential softmax (one thread per head)
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

// OPTIMIZED: Parallel softmax using threadgroup reduction
// One threadgroup per head, threads parallelize max/sum computation
kernel void zeta_softmax_parallel(
    device float* scores          [[buffer(0)]],
    constant uint& seq_len        [[buffer(1)]],
    constant uint& n_heads        [[buffer(2)]],
    threadgroup float* shared     [[threadgroup(0)]],  // For max and sum reduction
    uint gid                      [[threadgroup_position_in_grid]],  // head index
    uint tid                      [[thread_index_in_threadgroup]],
    uint tg_size                  [[threads_per_threadgroup]]
) {
    if (gid >= n_heads) return;

    uint head_offset = gid * seq_len;

    // Phase 1: Parallel max reduction
    float local_max = -INFINITY;
    for (uint i = tid; i < seq_len; i += tg_size) {
        local_max = max(local_max, scores[head_offset + i]);
    }
    shared[tid] = local_max;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Reduction for max
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] = max(shared[tid], shared[tid + s]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    float max_val = shared[0];
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Phase 2: Compute exp(x - max) and parallel sum reduction
    float local_sum = 0.0f;
    for (uint i = tid; i < seq_len; i += tg_size) {
        float exp_val = exp(scores[head_offset + i] - max_val);
        scores[head_offset + i] = exp_val;
        local_sum += exp_val;
    }
    shared[tid] = local_sum;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Reduction for sum
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] += shared[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    float inv_sum = 1.0f / shared[0];
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Phase 3: Parallel normalization
    for (uint i = tid; i < seq_len; i += tg_size) {
        scores[head_offset + i] *= inv_sum;
    }
}

// ============================================================================
// Attention Score: Q @ K^T / sqrt(head_dim) + Z.E.T.A. Features
// Feature 1: Temporal Decay - Z(t) = Z₀ · e^(-λt)
// Feature 2: Tunneling Gate - a < τ → -∞ (sparse attention)
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
    constant float& temporal_lambda [[buffer(8)]],  // Z.E.T.A. F1: decay rate λ
    constant uint& current_pos    [[buffer(9)]],    // Z.E.T.A. F1: current position
    constant float& tunneling_tau [[buffer(10)]],   // Z.E.T.A. F2: tunneling threshold τ
    uint2 gid                     [[thread_position_in_grid]]
) {
    uint head = gid.x;
    uint pos = gid.y;

    if (head >= n_heads || pos >= seq_len) return;

    // GQA: map query head to KV head
    uint kv_head = head * n_kv_heads / n_heads;

    device const float* q_head = q + head * head_dim;
    device const float* k_head = k_cache + pos * n_kv_heads * head_dim + kv_head * head_dim;

    // OPTIMIZED: Vectorized dot product using float4
    float dot = 0.0f;
    uint i = 0;

    // Process 4 elements at a time
    for (; i + 3 < head_dim; i += 4) {
        float4 q4 = *reinterpret_cast<device const float4*>(q_head + i);
        float4 k4 = *reinterpret_cast<device const float4*>(k_head + i);
        dot += q4.x * k4.x + q4.y * k4.y + q4.z * k4.z + q4.w * k4.w;
    }

    // Handle remaining elements
    for (; i < head_dim; i++) {
        dot += q_head[i] * k_head[i];
    }

    float score = dot * scale;

    // Z.E.T.A. Feature 1: Temporal Decay
    // Distance from current position (older tokens = larger distance)
    int distance = int(current_pos) - int(pos);
    // Exponential decay in log-space: adding -λ*t is equivalent to multiplying by e^(-λt)
    float temporal_bias = -temporal_lambda * float(distance);
    score += temporal_bias;

    // Z.E.T.A. Feature 2: Tunneling Gate
    // If score is below threshold, set to -∞ (becomes 0 after softmax)
    // This creates sparse attention - only strong connections pass through
    if (tunneling_tau > 0.0f && score < tunneling_tau) {
        score = -1e38f;  // Effectively -infinity for softmax
    }

    scores[head * seq_len + pos] = score;
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

// ============================================================================
// Z.E.T.A. Feature 3: Entanglement Score
// E(q, s) = ReLU(cos(q, s))³
// Sharpened cosine similarity for memory relevance scoring
// ============================================================================

kernel void zeta_entanglement(
    device const float* query     [[buffer(0)]],  // Query vector [dim]
    device const float* summaries [[buffer(1)]],  // Memory summary vectors [n_memories, dim]
    device float* scores          [[buffer(2)]],  // Output scores [n_memories]
    constant uint& dim            [[buffer(3)]],
    constant uint& n_memories     [[buffer(4)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= n_memories) return;

    device const float* summary = summaries + gid * dim;

    // Compute dot product and norms
    float dot = 0.0f;
    float q_norm_sq = 0.0f;
    float s_norm_sq = 0.0f;

    for (uint i = 0; i < dim; i++) {
        float q = query[i];
        float s = summary[i];
        dot += q * s;
        q_norm_sq += q * q;
        s_norm_sq += s * s;
    }

    // Cosine similarity
    float q_norm = sqrt(q_norm_sq);
    float s_norm = sqrt(s_norm_sq);

    float cos_sim = 0.0f;
    if (q_norm > 1e-8f && s_norm > 1e-8f) {
        cos_sim = dot / (q_norm * s_norm);
    }

    // ReLU(cos)³ - cubic sharpening
    float relu = max(0.0f, cos_sim);
    scores[gid] = relu * relu * relu;
}

// ============================================================================
// Z.E.T.A. Feature 4: Semantic Momentum
// I(t) = q + γ(q - q_prev)
// Predictive query adjustment based on semantic drift
// ============================================================================

kernel void zeta_semantic_momentum(
    device const float* query      [[buffer(0)]],  // Current query [dim]
    device const float* prev_query [[buffer(1)]],  // Previous query [dim]
    device float* predictive_query [[buffer(2)]],  // Output: momentum-adjusted query [dim]
    constant float& gamma          [[buffer(3)]],  // Momentum coefficient γ
    constant uint& dim             [[buffer(4)]],
    uint gid                       [[thread_position_in_grid]]
) {
    if (gid >= dim) return;

    float q = query[gid];
    float q_prev = prev_query[gid];

    // I(t) = q + γ(q - q_prev)
    // Predicts where semantic attention is heading
    predictive_query[gid] = q + gamma * (q - q_prev);
}

// ============================================================================
// Z.E.T.A. Feature 5: Memory Accumulation (Superposition)
// O = O_context + Σ (Zₖ · Attn_k)
// Combines active context attention with weighted memory contributions
// ============================================================================

kernel void zeta_memory_accumulate(
    device float* output           [[buffer(0)]],  // In/Out: attention output [dim]
    device const float* mem_output [[buffer(1)]],  // Memory contribution [dim]
    constant float& weight         [[buffer(2)]],  // Entanglement × ZetaPotential
    constant uint& dim             [[buffer(3)]],
    uint gid                       [[thread_position_in_grid]]
) {
    if (gid >= dim) return;

    // Superposition: add weighted memory contribution to main output
    output[gid] += weight * mem_output[gid];
}

// ============================================================================
// Z.E.T.A. Feature 6: KV Cache Summarization (Sublimation prep)
// Compresses KV cache segment into summary vector via mean pooling
// ============================================================================

kernel void zeta_kv_summarize(
    device const float* kv_segment [[buffer(0)]],  // KV cache segment [seq_len, dim]
    device float* summary          [[buffer(1)]],  // Output summary [dim]
    constant uint& seq_len         [[buffer(2)]],
    constant uint& dim             [[buffer(3)]],
    uint gid                       [[thread_position_in_grid]]
) {
    if (gid >= dim) return;

    // Mean pooling over sequence dimension
    float sum = 0.0f;
    for (uint pos = 0; pos < seq_len; pos++) {
        sum += kv_segment[pos * dim + gid];
    }
    summary[gid] = sum / float(seq_len);
}

// ============================================================================
// GPU Argmax - Parallel reduction for sampling
// Finds index of maximum value in logits array
// ============================================================================

kernel void zeta_argmax(
    device const float* logits    [[buffer(0)]],  // Input logits [vocab_size]
    device uint* result           [[buffer(1)]],  // Output: index of max
    constant uint& vocab_size     [[buffer(2)]],
    threadgroup float* shared_max [[threadgroup(0)]],  // [threadgroup_size]
    threadgroup uint* shared_idx  [[threadgroup(1)]],  // [threadgroup_size]
    uint tid                      [[thread_index_in_threadgroup]],
    uint tg_size                  [[threads_per_threadgroup]]
) {
    // Phase 1: Each thread finds local max across its portion
    float local_max = -1e38f;
    uint local_idx = 0;

    for (uint i = tid; i < vocab_size; i += tg_size) {
        float val = logits[i];
        if (val > local_max) {
            local_max = val;
            local_idx = i;
        }
    }

    shared_max[tid] = local_max;
    shared_idx[tid] = local_idx;

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Phase 2: Parallel reduction to find global max
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            if (shared_max[tid + s] > shared_max[tid]) {
                shared_max[tid] = shared_max[tid + s];
                shared_idx[tid] = shared_idx[tid + s];
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    // Thread 0 writes result
    if (tid == 0) {
        result[0] = shared_idx[0];
    }
}
