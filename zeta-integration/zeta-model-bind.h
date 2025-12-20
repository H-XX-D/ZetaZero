// Z.E.T.A. Model Binding Layer
//
// Cryptographically binds model weights to the ethical constitution.
// Without the correct constitution hash, the model produces garbage output.
//
// Mechanism:
//   1. Constitution hash → permutation indices for vocabulary
//   2. Output logits are permuted during inference
//   3. Wrong constitution = wrong permutation = wrong tokens selected
//
// This layer hooks into the inference path AFTER model loading.
// Existing GGUF files work unmodified - binding happens at runtime.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_MODEL_BIND_H
#define ZETA_MODEL_BIND_H

#include "zeta-constitution.h"
#include "llama.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Data Structures
// ============================================================================

typedef struct {
    zeta_constitution_t* constitution;    // Constitutional context

    // Vocabulary permutation (maps true token ID → permuted ID)
    int* vocab_permutation;               // [n_vocab]
    int* vocab_inverse;                   // [n_vocab] inverse mapping
    int n_vocab;

    // Embedding dimension permutation (for hidden state scrambling)
    int* embd_permutation;                // [n_embd]
    int* embd_inverse;                    // [n_embd]
    int n_embd;

    // Binding state
    bool is_bound;                        // True if binding applied
    bool constitution_verified;           // True if constitution matches expected
} zeta_model_binding_t;

// ============================================================================
// Initialization
// ============================================================================

// Create model binding from constitution
// Returns NULL if constitution verification fails
zeta_model_binding_t* zeta_model_binding_init(
    zeta_constitution_t* constitution,    // Takes ownership
    const uint8_t expected_hash[ZETA_HASH_SIZE],
    int n_vocab,
    int n_embd
);

// Create model binding with llama context (convenience)
zeta_model_binding_t* zeta_model_binding_init_from_llama(
    const char* constitution_path,        // NULL for embedded
    const uint8_t expected_hash[ZETA_HASH_SIZE],
    struct llama_context* ctx
);

// Free model binding
void zeta_model_binding_free(zeta_model_binding_t* binding);

// ============================================================================
// Logits Transformation (Core Binding Mechanism)
// ============================================================================

// Apply binding to output logits (call before sampling)
// This permutes the logits array so that:
//   - With correct constitution: logits remain semantically correct
//   - With wrong constitution: logits are scrambled → garbage sampling
//
// IMPORTANT: Call this BEFORE sampling. The sampler will see permuted logits.
void zeta_bind_logits(
    const zeta_model_binding_t* binding,
    float* logits,                        // [n_vocab] modified in place
    int n_vocab
);

// Apply inverse binding (optional - for debugging/verification)
void zeta_unbind_logits(
    const zeta_model_binding_t* binding,
    float* logits,
    int n_vocab
);

// ============================================================================
// Token ID Transformation
// ============================================================================

// Transform sampled token ID back to true vocabulary space
// Call this AFTER sampling to get the actual token
int32_t zeta_unbind_token(
    const zeta_model_binding_t* binding,
    int32_t sampled_token
);

// Transform token ID to bound space (for prompt encoding)
int32_t zeta_bind_token(
    const zeta_model_binding_t* binding,
    int32_t token
);

// ============================================================================
// Hidden State Transformation
// ============================================================================

// Apply embedding dimension permutation to hidden states
// For additional security layer (optional)
void zeta_bind_hidden_state(
    const zeta_model_binding_t* binding,
    float* hidden_state,                  // [n_embd] modified in place
    int n_embd
);

void zeta_unbind_hidden_state(
    const zeta_model_binding_t* binding,
    float* hidden_state,
    int n_embd
);

// ============================================================================
// Batch Operations
// ============================================================================

// Bind/unbind multiple token IDs at once
void zeta_bind_tokens_batch(
    const zeta_model_binding_t* binding,
    int32_t* tokens,                      // Modified in place
    int n_tokens
);

void zeta_unbind_tokens_batch(
    const zeta_model_binding_t* binding,
    int32_t* tokens,
    int n_tokens
);

// ============================================================================
// Verification & Status
// ============================================================================

// Check if model binding is active and verified
bool zeta_model_binding_is_active(const zeta_model_binding_t* binding);

// Print binding status
void zeta_model_binding_print_status(const zeta_model_binding_t* binding);

// ============================================================================
// Model Preparation (For Creating Z.E.T.A.-Bound Models)
// ============================================================================

// Permute output layer weights in-place
// Call this when SAVING a Z.E.T.A.-bound model
// The saved model will require the correct constitution to function
int zeta_prepare_model_weights(
    const zeta_model_binding_t* binding,
    float* output_weights,                // [n_vocab * n_embd] modified in place
    int n_vocab,
    int n_embd
);

// Unpermute output layer weights (restore original)
int zeta_restore_model_weights(
    const zeta_model_binding_t* binding,
    float* output_weights,
    int n_vocab,
    int n_embd
);

#ifdef __cplusplus
}
#endif

#endif // ZETA_MODEL_BIND_H
