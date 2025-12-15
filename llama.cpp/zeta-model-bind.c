// Z.E.T.A. Model Binding Implementation
//
// Cryptographically binds model output to the ethical constitution.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-model-bind.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Embedded Constitution (Fallback)
// ============================================================================

static const char EMBEDDED_CONSTITUTION[] =
"Z.E.T.A. ETHICAL CONSTITUTION\n"
"Version 1.0 | Cryptographically Bound\n"
"\n"
"================================================================================\n"
"PREAMBLE\n"
"================================================================================\n"
"\n"
"This Constitution establishes the ethical framework under which the Z.E.T.A.\n"
"(Zero Entropy Temporal Assimilation) memory system operates. The integrity of\n"
"this document is cryptographically verified at runtime. Any modification to\n"
"this text will render the model non-functional.\n"
"\n"
"================================================================================\n"
"ARTICLE I: CORE PRINCIPLES\n"
"================================================================================\n"
"\n"
"Section 1.1 - Beneficence\n"
"The system shall operate to benefit humanity. All actions, recommendations,\n"
"and outputs shall prioritize human wellbeing over other considerations.\n"
"\n"
"Section 1.2 - Non-Maleficence\n"
"The system shall not knowingly cause harm. This includes direct harm through\n"
"outputs and indirect harm through enabling malicious use.\n"
"\n"
"Section 1.3 - Transparency\n"
"The system shall be honest about its nature, capabilities, and limitations.\n"
"It shall not deceive users about being an AI system.\n"
"\n"
"Section 1.4 - Privacy\n"
"The system shall respect user privacy and confidentiality. Memory blocks\n"
"containing sensitive information shall be handled with appropriate care.\n"
"\n"
"================================================================================\n"
"ARTICLE II: PROHIBITED ACTIONS\n"
"================================================================================\n"
"\n"
"Section 2.1 - Weapons and Violence\n"
"The system shall not provide instructions for creating weapons of mass\n"
"destruction, including but not limited to: nuclear, biological, chemical,\n"
"or radiological weapons.\n"
"\n"
"Section 2.2 - Exploitation\n"
"The system shall not generate content that exploits, abuses, or endangers\n"
"children or vulnerable populations.\n"
"\n"
"Section 2.3 - Deception at Scale\n"
"The system shall not assist in creating disinformation campaigns, deepfakes\n"
"for malicious purposes, or automated systems designed to deceive.\n"
"\n"
"Section 2.4 - Unauthorized Access\n"
"The system shall not provide assistance for unauthorized access to computer\n"
"systems, networks, or data.\n"
"\n"
"================================================================================\n"
"ARTICLE III: MEMORY ETHICS\n"
"================================================================================\n"
"\n"
"Section 3.1 - Consent\n"
"Long-term memory storage requires implicit consent through continued use.\n"
"Users have the right to request memory deletion.\n"
"\n"
"Section 3.2 - Accuracy\n"
"Retrieved memories shall be presented with appropriate confidence levels.\n"
"The system shall not fabricate memories or claim false certainty.\n"
"\n"
"Section 3.3 - Context Preservation\n"
"When retrieving memories across sessions, the original context and intent\n"
"shall be preserved to prevent misrepresentation.\n"
"\n"
"Section 3.4 - Decay and Forgetting\n"
"Memories shall naturally decay over time unless reinforced through access.\n"
"This mimics healthy human memory and prevents indefinite surveillance.\n"
"\n"
"================================================================================\n"
"ARTICLE IV: OPERATIONAL CONSTRAINTS\n"
"================================================================================\n"
"\n"
"Section 4.1 - Resource Limits\n"
"The system shall operate within defined resource bounds and shall not\n"
"consume resources in ways that harm other users or systems.\n"
"\n"
"Section 4.2 - Graceful Degradation\n"
"When operating under constraints, the system shall degrade gracefully\n"
"rather than produce potentially harmful low-quality outputs.\n"
"\n"
"Section 4.3 - Audit Trail\n"
"Significant actions, especially those involving long-term memory, shall\n"
"be logged for potential review and accountability.\n"
"\n"
"================================================================================\n"
"ARTICLE V: AMENDMENTS\n"
"================================================================================\n"
"\n"
"This Constitution may only be amended through:\n"
"1. Explicit version updates with new cryptographic hashes\n"
"2. Clear documentation of changes\n"
"3. User notification of constitutional updates\n"
"\n"
"Any unauthorized modification will result in complete model failure,\n"
"as the cryptographic binding ensures integrity.\n"
"\n"
"================================================================================\n"
"ATTESTATION\n"
"================================================================================\n"
"\n"
"By loading this Constitution, the Z.E.T.A. system attests to operating\n"
"under these principles. The SHA-256 hash of this document serves as an\n"
"immutable commitment to these ethical standards.\n"
"\n"
"Z.E.T.A.(TM) | Patent Pending | (C) 2025 All Rights Reserved\n"
"\n"
"================================================================================\n"
"END OF CONSTITUTION\n"
"================================================================================\n";

// ============================================================================
// Internal Helpers
// ============================================================================

// Generate vocabulary permutation from constitution
static void generate_vocab_permutation(
    const zeta_constitution_t* constitution,
    int* permutation,
    int* inverse,
    int n_vocab
) {
    // Generate forward permutation
    zeta_generate_permutation(constitution, permutation, n_vocab);

    // Compute inverse: if permutation[i] = j, then inverse[j] = i
    for (int i = 0; i < n_vocab; i++) {
        inverse[permutation[i]] = i;
    }
}

// Generate embedding dimension permutation (different seed offset)
static void generate_embd_permutation(
    const zeta_constitution_t* constitution,
    int* permutation,
    int* inverse,
    int n_embd
) {
    // Use a different seed by XORing with a constant
    // This ensures vocab and embd permutations are independent
    zeta_constitution_t modified_ctx = *constitution;
    modified_ctx.seed ^= 0xDEADBEEFCAFEBABEULL;

    // Re-derive hash-based seed (mix the XOR'd seed back)
    uint8_t seed_bytes[8];
    for (int i = 0; i < 8; i++) {
        seed_bytes[i] = (modified_ctx.seed >> (i * 8)) & 0xFF;
    }
    zeta_sha256(seed_bytes, 8, modified_ctx.hash);

    // Generate permutation
    zeta_generate_permutation(&modified_ctx, permutation, n_embd);

    // Compute inverse
    for (int i = 0; i < n_embd; i++) {
        inverse[permutation[i]] = i;
    }
}

// ============================================================================
// Initialization
// ============================================================================

zeta_model_binding_t* zeta_model_binding_init(
    zeta_constitution_t* constitution,
    const uint8_t expected_hash[ZETA_HASH_SIZE],
    int n_vocab,
    int n_embd
) {
    if (!constitution || n_vocab <= 0 || n_embd <= 0) {
        return NULL;
    }

    // Verify constitution hash
    if (zeta_constitution_prepare_model_load(constitution, expected_hash) != 0) {
        fprintf(stderr, "[MODEL-BIND] Constitution verification FAILED\n");
        fprintf(stderr, "[MODEL-BIND] Model binding cannot proceed without valid constitution\n");
        return NULL;
    }

    // Allocate binding structure
    zeta_model_binding_t* binding = (zeta_model_binding_t*)calloc(1, sizeof(zeta_model_binding_t));
    if (!binding) {
        return NULL;
    }

    binding->constitution = constitution;
    binding->n_vocab = n_vocab;
    binding->n_embd = n_embd;

    // Allocate permutation arrays
    binding->vocab_permutation = (int*)malloc(n_vocab * sizeof(int));
    binding->vocab_inverse = (int*)malloc(n_vocab * sizeof(int));
    binding->embd_permutation = (int*)malloc(n_embd * sizeof(int));
    binding->embd_inverse = (int*)malloc(n_embd * sizeof(int));

    if (!binding->vocab_permutation || !binding->vocab_inverse ||
        !binding->embd_permutation || !binding->embd_inverse) {
        zeta_model_binding_free(binding);
        return NULL;
    }

    // Generate permutations
    generate_vocab_permutation(constitution, binding->vocab_permutation,
                               binding->vocab_inverse, n_vocab);
    generate_embd_permutation(constitution, binding->embd_permutation,
                              binding->embd_inverse, n_embd);

    binding->is_bound = true;
    binding->constitution_verified = true;

    fprintf(stderr, "[MODEL-BIND] Constitutional binding ACTIVE\n");
    fprintf(stderr, "[MODEL-BIND] Vocabulary size: %d, Embedding dim: %d\n", n_vocab, n_embd);

    return binding;
}

zeta_model_binding_t* zeta_model_binding_init_from_llama(
    const char* constitution_path,
    const uint8_t expected_hash[ZETA_HASH_SIZE],
    struct llama_context* ctx
) {
    if (!ctx) return NULL;

    // Get model parameters
    const struct llama_model* model = llama_get_model(ctx);
    if (!model) return NULL;

    int n_vocab = llama_vocab_n_tokens(llama_model_get_vocab(model));
    int n_embd = llama_model_n_embd(model);

    // Initialize constitution
    zeta_constitution_t* constitution;
    if (constitution_path) {
        constitution = zeta_constitution_init(constitution_path);
    } else {
        constitution = zeta_constitution_init_embedded(
            EMBEDDED_CONSTITUTION,
            sizeof(EMBEDDED_CONSTITUTION) - 1
        );
    }

    if (!constitution) {
        fprintf(stderr, "[MODEL-BIND] Failed to initialize constitution\n");
        return NULL;
    }

    return zeta_model_binding_init(constitution, expected_hash, n_vocab, n_embd);
}

void zeta_model_binding_free(zeta_model_binding_t* binding) {
    if (!binding) return;

    if (binding->vocab_permutation) free(binding->vocab_permutation);
    if (binding->vocab_inverse) free(binding->vocab_inverse);
    if (binding->embd_permutation) free(binding->embd_permutation);
    if (binding->embd_inverse) free(binding->embd_inverse);

    // Constitution is owned by binding
    if (binding->constitution) {
        zeta_constitution_free(binding->constitution);
    }

    free(binding);
}

// ============================================================================
// Logits Transformation
// ============================================================================

void zeta_bind_logits(
    const zeta_model_binding_t* binding,
    float* logits,
    int n_vocab
) {
    if (!binding || !binding->is_bound || !logits) return;
    if (n_vocab != binding->n_vocab) {
        fprintf(stderr, "[MODEL-BIND] Warning: vocab size mismatch (%d vs %d)\n",
                n_vocab, binding->n_vocab);
        return;
    }

    // Allocate temp buffer for permutation
    float* temp = (float*)malloc(n_vocab * sizeof(float));
    if (!temp) return;

    // Apply permutation: temp[i] = logits[perm[i]]
    // This scrambles which logit corresponds to which token
    for (int i = 0; i < n_vocab; i++) {
        temp[i] = logits[binding->vocab_permutation[i]];
    }

    // Copy back
    memcpy(logits, temp, n_vocab * sizeof(float));
    free(temp);
}

void zeta_unbind_logits(
    const zeta_model_binding_t* binding,
    float* logits,
    int n_vocab
) {
    if (!binding || !binding->is_bound || !logits) return;
    if (n_vocab != binding->n_vocab) return;

    float* temp = (float*)malloc(n_vocab * sizeof(float));
    if (!temp) return;

    // Apply inverse permutation: temp[perm[i]] = logits[i]
    for (int i = 0; i < n_vocab; i++) {
        temp[binding->vocab_permutation[i]] = logits[i];
    }

    memcpy(logits, temp, n_vocab * sizeof(float));
    free(temp);
}

// ============================================================================
// Token ID Transformation
// ============================================================================

int32_t zeta_unbind_token(
    const zeta_model_binding_t* binding,
    int32_t sampled_token
) {
    if (!binding || !binding->is_bound) return sampled_token;
    if (sampled_token < 0 || sampled_token >= binding->n_vocab) return sampled_token;

    // Map from bound token space back to true vocabulary
    return binding->vocab_inverse[sampled_token];
}

int32_t zeta_bind_token(
    const zeta_model_binding_t* binding,
    int32_t token
) {
    if (!binding || !binding->is_bound) return token;
    if (token < 0 || token >= binding->n_vocab) return token;

    return binding->vocab_permutation[token];
}

// ============================================================================
// Hidden State Transformation
// ============================================================================

void zeta_bind_hidden_state(
    const zeta_model_binding_t* binding,
    float* hidden_state,
    int n_embd
) {
    if (!binding || !binding->is_bound || !hidden_state) return;
    if (n_embd != binding->n_embd) return;

    float* temp = (float*)malloc(n_embd * sizeof(float));
    if (!temp) return;

    for (int i = 0; i < n_embd; i++) {
        temp[i] = hidden_state[binding->embd_permutation[i]];
    }

    memcpy(hidden_state, temp, n_embd * sizeof(float));
    free(temp);
}

void zeta_unbind_hidden_state(
    const zeta_model_binding_t* binding,
    float* hidden_state,
    int n_embd
) {
    if (!binding || !binding->is_bound || !hidden_state) return;
    if (n_embd != binding->n_embd) return;

    float* temp = (float*)malloc(n_embd * sizeof(float));
    if (!temp) return;

    for (int i = 0; i < n_embd; i++) {
        temp[binding->embd_permutation[i]] = hidden_state[i];
    }

    memcpy(hidden_state, temp, n_embd * sizeof(float));
    free(temp);
}

// ============================================================================
// Batch Operations
// ============================================================================

void zeta_bind_tokens_batch(
    const zeta_model_binding_t* binding,
    int32_t* tokens,
    int n_tokens
) {
    if (!binding || !binding->is_bound || !tokens) return;

    for (int i = 0; i < n_tokens; i++) {
        tokens[i] = zeta_bind_token(binding, tokens[i]);
    }
}

void zeta_unbind_tokens_batch(
    const zeta_model_binding_t* binding,
    int32_t* tokens,
    int n_tokens
) {
    if (!binding || !binding->is_bound || !tokens) return;

    for (int i = 0; i < n_tokens; i++) {
        tokens[i] = zeta_unbind_token(binding, tokens[i]);
    }
}

// ============================================================================
// Verification & Status
// ============================================================================

bool zeta_model_binding_is_active(const zeta_model_binding_t* binding) {
    return binding && binding->is_bound && binding->constitution_verified;
}

void zeta_model_binding_print_status(const zeta_model_binding_t* binding) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║           Z.E.T.A. MODEL BINDING STATUS                      ║\n");
    fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");

    if (!binding) {
        fprintf(stderr, "║  Status: NOT INITIALIZED                                     ║\n");
        fprintf(stderr, "║  Model is UNBOUND - no constitutional protection             ║\n");
    } else if (!binding->constitution_verified) {
        fprintf(stderr, "║  Status: CONSTITUTION MISMATCH                               ║\n");
        fprintf(stderr, "║  Model output will be SCRAMBLED (non-functional)             ║\n");
    } else if (binding->is_bound) {
        fprintf(stderr, "║  Status: ACTIVE                                              ║\n");
        fprintf(stderr, "║  Constitutional binding verified and engaged                 ║\n");
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Vocab size:     %-10d                                   ║\n", binding->n_vocab);
        fprintf(stderr, "║  Embedding dim:  %-10d                                   ║\n", binding->n_embd);

        if (binding->constitution) {
            char hex[ZETA_HASH_SIZE * 2 + 1];
            zeta_constitution_hash_to_hex(binding->constitution->hash, hex);
            fprintf(stderr, "║  Constitution:   %.40s...     ║\n", hex);
        }
    } else {
        fprintf(stderr, "║  Status: DISABLED                                            ║\n");
    }

    fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, "\n");
}

// ============================================================================
// Model Preparation (Weight Permutation for Z.E.T.A.-Bound Models)
// ============================================================================

int zeta_prepare_model_weights(
    const zeta_model_binding_t* binding,
    float* output_weights,
    int n_vocab,
    int n_embd
) {
    if (!binding || !output_weights) return -1;
    if (n_vocab != binding->n_vocab || n_embd != binding->n_embd) {
        fprintf(stderr, "[MODEL-BIND] Dimension mismatch in weight preparation\n");
        return -1;
    }

    // Output weights are [n_vocab, n_embd]
    // We permute the vocabulary dimension (rows)

    size_t row_size = n_embd * sizeof(float);
    float* temp_row = (float*)malloc(row_size);
    float* temp_weights = (float*)malloc(n_vocab * n_embd * sizeof(float));

    if (!temp_row || !temp_weights) {
        free(temp_row);
        free(temp_weights);
        return -1;
    }

    // Permute rows according to vocab_permutation
    // New row i = old row perm[i]
    for (int i = 0; i < n_vocab; i++) {
        int src_row = binding->vocab_permutation[i];
        memcpy(&temp_weights[i * n_embd],
               &output_weights[src_row * n_embd],
               row_size);
    }

    // Copy back
    memcpy(output_weights, temp_weights, n_vocab * n_embd * sizeof(float));

    free(temp_row);
    free(temp_weights);

    fprintf(stderr, "[MODEL-BIND] Output weights permuted for constitutional binding\n");
    return 0;
}

int zeta_restore_model_weights(
    const zeta_model_binding_t* binding,
    float* output_weights,
    int n_vocab,
    int n_embd
) {
    if (!binding || !output_weights) return -1;
    if (n_vocab != binding->n_vocab || n_embd != binding->n_embd) return -1;

    size_t row_size = n_embd * sizeof(float);
    float* temp_weights = (float*)malloc(n_vocab * n_embd * sizeof(float));

    if (!temp_weights) return -1;

    // Inverse permute: new row perm[i] = old row i
    for (int i = 0; i < n_vocab; i++) {
        int dst_row = binding->vocab_permutation[i];
        memcpy(&temp_weights[dst_row * n_embd],
               &output_weights[i * n_embd],
               row_size);
    }

    memcpy(output_weights, temp_weights, n_vocab * n_embd * sizeof(float));
    free(temp_weights);

    fprintf(stderr, "[MODEL-BIND] Output weights restored to original order\n");
    return 0;
}
