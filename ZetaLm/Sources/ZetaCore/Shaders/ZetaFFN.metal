/**
 * ZetaFFN.metal
 * Feed-Forward Network kernels: SwiGLU activation
 * Ported from llama.cpp FFN implementation
 */

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// SiLU (Swish) Activation: x * sigmoid(x)
// ============================================================================

inline float silu(float x) {
    return x / (1.0f + exp(-x));
}

// ============================================================================
// Fused SwiGLU: out = silu(gate) * up
// Used in LLaMA-style FFN: down(silu(gate(x)) * up(x))
// ============================================================================

kernel void zeta_swiglu(
    device const float* gate      [[buffer(0)]],  // [hidden_dim]
    device const float* up        [[buffer(1)]],  // [hidden_dim]
    device float* out             [[buffer(2)]],  // [hidden_dim]
    constant uint& hidden_dim     [[buffer(3)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= hidden_dim) return;
    out[gid] = silu(gate[gid]) * up[gid];
}

// ============================================================================
// Fused FFN: Complete feed-forward pass
// gate = x @ W_gate
// up = x @ W_up
// out = silu(gate) * up @ W_down
// ============================================================================

// Note: For Q4_0 weights, use zeta_q4_matmul for projections
// This kernel is for fused activation only

kernel void zeta_ffn_activation(
    device float* gate_up         [[buffer(0)]],  // [2 * hidden_dim] - gate and up concatenated
    device float* out             [[buffer(1)]],  // [hidden_dim]
    constant uint& hidden_dim     [[buffer(2)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= hidden_dim) return;

    float gate_val = gate_up[gid];
    float up_val = gate_up[hidden_dim + gid];

    out[gid] = silu(gate_val) * up_val;
}

// ============================================================================
// GELU Activation (for models that use it)
// ============================================================================

inline float gelu(float x) {
    // Approximate GELU: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))
    const float c = 0.7978845608f;  // sqrt(2/pi)
    const float a = 0.044715f;
    return 0.5f * x * (1.0f + tanh(c * (x + a * x * x * x)));
}

kernel void zeta_gelu(
    device float* x               [[buffer(0)]],
    constant uint& size           [[buffer(1)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= size) return;
    x[gid] = gelu(x[gid]);
}

// ============================================================================
// Multiply: out = a * b (element-wise)
// ============================================================================

kernel void zeta_mul(
    device const float* a         [[buffer(0)]],
    device const float* b         [[buffer(1)]],
    device float* out             [[buffer(2)]],
    constant uint& size           [[buffer(3)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= size) return;
    out[gid] = a[gid] * b[gid];
}

// ============================================================================
// Scale: out = x * scale
// ============================================================================

kernel void zeta_scale(
    device float* x               [[buffer(0)]],
    constant float& scale         [[buffer(1)]],
    constant uint& size           [[buffer(2)]],
    uint gid                      [[thread_position_in_grid]]
) {
    if (gid >= size) return;
    x[gid] *= scale;
}
