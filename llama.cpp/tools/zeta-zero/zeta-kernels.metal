// Z.E.T.A. Metal Kernels
//
// Temporal Decay and Sparse Gating for Extended Context
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// Constants
// ============================================================================

constant float ZETA_EPSILON = 1e-10f;

// ============================================================================
// Temporal Decay Kernel
// ============================================================================
// Applies exponential decay: Z(t) = Z₀ · e^(-λt)
// where t is the age of each token (current_pos - token_pos)
//
// This biases attention toward recent tokens while maintaining
// access to older context through the memory system.

kernel void zeta_temporal_decay(
    device float* attention_scores [[buffer(0)]],     // [seq_len, kv_len] - modified in place
    constant int& seq_len [[buffer(1)]],
    constant int& kv_len [[buffer(2)]],
    constant int& current_pos [[buffer(3)]],          // Current generation position
    constant float& lambda [[buffer(4)]],             // Decay rate (0.01-0.1 typical)
    uint2 gid [[thread_position_in_grid]]
) {
    const int q_idx = gid.y;  // Query index
    const int k_idx = gid.x;  // Key index (position in KV cache)

    if (q_idx >= seq_len || k_idx >= kv_len) return;

    // Compute token age
    const int token_age = current_pos - k_idx;

    // Only apply decay to past tokens
    if (token_age > 0 && lambda > 0.0f) {
        // Exponential decay: e^(-λt)
        const float decay = exp(-lambda * float(token_age));

        // Apply to attention score
        const int idx = q_idx * kv_len + k_idx;
        attention_scores[idx] *= decay;
    }
}

// ============================================================================
// Sparse Gating Kernel (Tunneling)
// ============================================================================
// Zeros out attention weights below threshold τ
// This creates sparse attention patterns, focusing compute on relevant tokens

kernel void zeta_sparse_gate(
    device float* attention_scores [[buffer(0)]],     // [seq_len, kv_len] - modified in place
    constant int& seq_len [[buffer(1)]],
    constant int& kv_len [[buffer(2)]],
    constant float& threshold [[buffer(3)]],          // Gating threshold (0.01-0.1 typical)
    uint2 gid [[thread_position_in_grid]]
) {
    const int q_idx = gid.y;
    const int k_idx = gid.x;

    if (q_idx >= seq_len || k_idx >= kv_len) return;

    const int idx = q_idx * kv_len + k_idx;

    // Use -INFINITY for sub-threshold attention (pre-softmax)
    // This ensures exp(-inf) = 0 without causing NaN in softmax
    if (attention_scores[idx] < threshold) {
        attention_scores[idx] = -INFINITY;
    }
}

// ============================================================================
// Combined Z.E.T.A. Attention Modifier
// ============================================================================
// Applies both temporal decay AND sparse gating in one pass

// Note: This kernel applies decay and gating per-element.
// For the all-masked safeguard, use zeta_attention_modifier_safe which does a row-wise check.
kernel void zeta_attention_modifier(
    device float* attention_scores [[buffer(0)]],     // [seq_len, kv_len] - modified in place
    constant int& seq_len [[buffer(1)]],
    constant int& kv_len [[buffer(2)]],
    constant int& current_pos [[buffer(3)]],
    constant float& lambda [[buffer(4)]],             // Temporal decay rate
    constant float& threshold [[buffer(5)]],          // Sparse gating threshold
    uint2 gid [[thread_position_in_grid]]
) {
    const int q_idx = gid.y;
    const int k_idx = gid.x;

    if (q_idx >= seq_len || k_idx >= kv_len) return;

    const int idx = q_idx * kv_len + k_idx;
    float score = attention_scores[idx];

    // Skip already-masked positions
    if (score <= -INFINITY) return;

    // 1. Temporal Decay
    if (lambda > 0.0f) {
        const int token_age = current_pos - k_idx;
        if (token_age > 0) {
            score *= exp(-lambda * float(token_age));
        }
    }

    // 2. Sparse Gating (use -INFINITY to prevent NaN in softmax)
    // Note: The most recent token (k_idx == current_pos) is never masked
    // to prevent all-masked rows that cause NaN
    if (score < threshold && k_idx != current_pos) {
        score = -INFINITY;
    }

    attention_scores[idx] = score;
}

// ============================================================================
// Attention Mask Generator with Z.E.T.A. Features
// ============================================================================
// Generates a causal mask with temporal decay baked in
// Can be used as input to standard attention kernels

kernel void zeta_generate_mask(
    device float* mask [[buffer(0)]],                 // [seq_len, kv_len] output
    constant int& seq_len [[buffer(1)]],
    constant int& kv_len [[buffer(2)]],
    constant int& current_pos [[buffer(3)]],
    constant float& lambda [[buffer(4)]],
    constant float& threshold [[buffer(5)]],
    constant int& causal [[buffer(6)]],               // 1 for causal, 0 for bidirectional
    uint2 gid [[thread_position_in_grid]]
) {
    const int q_idx = gid.y;
    const int k_idx = gid.x;

    if (q_idx >= seq_len || k_idx >= kv_len) return;

    const int idx = q_idx * kv_len + k_idx;

    // Start with causal mask
    float mask_val = 0.0f;

    if (causal && k_idx > q_idx + current_pos) {
        // Future token - mask completely
        mask_val = -INFINITY;
    } else {
        // Apply temporal decay as additive bias
        const int token_age = (q_idx + current_pos) - k_idx;
        if (token_age > 0 && lambda > 0.0f) {
            // Convert decay to additive log-space bias
            // e^(-λt) in multiplicative = -λt in log space (pre-softmax)
            mask_val = -lambda * float(token_age);
        }
    }

    mask[idx] = mask_val;
}

// ============================================================================
// Memory Injection Kernel
// ============================================================================
// Injects retrieved memory attention into the output
// O_final = O_context + α · O_memory

kernel void zeta_memory_injection(
    device float* output [[buffer(0)]],               // [seq_len, dim] - context output
    device const float* memory_output [[buffer(1)]],  // [seq_len, dim] - memory attention output
    constant int& seq_len [[buffer(2)]],
    constant int& dim [[buffer(3)]],
    constant float& alpha [[buffer(4)]],              // Injection weight (0.0-1.0)
    uint2 gid [[thread_position_in_grid]]
) {
    const int seq_idx = gid.y;
    const int dim_idx = gid.x;

    if (seq_idx >= seq_len || dim_idx >= dim) return;

    const int idx = seq_idx * dim + dim_idx;

    // Superposition: O = O_ctx + α · O_mem
    output[idx] = output[idx] + alpha * memory_output[idx];
}

// ============================================================================
// Softmax with Sparse Cleanup
// ============================================================================
// Applies softmax and then zeros out very small values

kernel void zeta_sparse_softmax(
    device float* scores [[buffer(0)]],               // [seq_len, kv_len] - in/out
    constant int& seq_len [[buffer(1)]],
    constant int& kv_len [[buffer(2)]],
    constant float& min_attention [[buffer(3)]],      // Minimum attention to keep
    constant int& num_threads [[buffer(4)]],          // Threadgroup size
    threadgroup float* shared [[threadgroup(0)]],
    uint gid [[threadgroup_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]]
) {
    const int q_idx = gid;

    if (q_idx >= seq_len) return;

    // Find max for numerical stability
    float max_score = -INFINITY;
    for (int k = tid; k < kv_len; k += num_threads) {
        const int idx = q_idx * kv_len + k;
        max_score = max(max_score, scores[idx]);
    }

    // Reduce to find global max
    shared[tid] = max_score;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint s = num_threads / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] = max(shared[tid], shared[tid + s]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    max_score = shared[0];

    // Compute exp sum
    float sum_exp = 0.0f;
    for (int k = tid; k < kv_len; k += num_threads) {
        const int idx = q_idx * kv_len + k;
        const float exp_val = exp(scores[idx] - max_score);
        scores[idx] = exp_val;  // Store temporarily
        sum_exp += exp_val;
    }

    // Reduce to find global sum
    shared[tid] = sum_exp;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint s = num_threads / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] += shared[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    sum_exp = shared[0] + ZETA_EPSILON;

    // Normalize and apply sparse cleanup
    for (int k = tid; k < kv_len; k += num_threads) {
        const int idx = q_idx * kv_len + k;
        float attention = scores[idx] / sum_exp;

        // Zero out very small attention values (sparse cleanup)
        if (attention < min_attention) {
            attention = 0.0f;
        }

        scores[idx] = attention;
    }
}

// ============================================================================
// Cosine Similarity for Memory Retrieval
// ============================================================================
// Computes similarity between query and memory block summaries

kernel void zeta_cosine_similarity(
    device const float* query [[buffer(0)]],          // [dim]
    device const float* summaries [[buffer(1)]],      // [n_blocks, dim]
    device float* similarities [[buffer(2)]],         // [n_blocks] output
    constant int& n_blocks [[buffer(3)]],
    constant int& dim [[buffer(4)]],
    constant int& num_threads [[buffer(5)]],          // Threadgroup size
    threadgroup float* shared [[threadgroup(0)]],
    uint gid [[threadgroup_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]]
) {
    const int block_idx = gid;

    if (block_idx >= n_blocks) return;
    device const float* summary = summaries + block_idx * dim;

    // Compute dot product and norms
    float dot_product = 0.0f;
    float query_norm_sq = 0.0f;
    float summary_norm_sq = 0.0f;

    for (int d = tid; d < dim; d += num_threads) {
        const float q = query[d];
        const float s = summary[d];
        dot_product += q * s;
        query_norm_sq += q * q;
        summary_norm_sq += s * s;
    }

    // Reduce within threadgroup
    shared[tid] = dot_product;
    shared[tid + num_threads] = query_norm_sq;
    shared[tid + 2 * num_threads] = summary_norm_sq;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint s = num_threads / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared[tid] += shared[tid + s];
            shared[tid + num_threads] += shared[tid + s + num_threads];
            shared[tid + 2 * num_threads] += shared[tid + s + 2 * num_threads];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (tid == 0) {
        const float dot = shared[0];
        const float q_norm = sqrt(shared[num_threads] + ZETA_EPSILON);
        const float s_norm = sqrt(shared[2 * num_threads] + ZETA_EPSILON);

        similarities[block_idx] = dot / (q_norm * s_norm);
    }
}
