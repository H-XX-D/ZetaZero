// ============================================================================
// ZETA CONTINUOUS TERNARY EDGE LOGIC
// ============================================================================
//
// Single float belief value: -1.0 to +1.0
//
//   -1.0        -0.5         0.0         +0.5        +1.0
//     |-----------|-----------|-----------|-----------|
//   STRONGLY    WEAKLY     UNKNOWN/    WEAKLY     STRONGLY
//   DISBELIEVE  DISBELIEVE  NEUTRAL    BELIEVE    BELIEVE
//
// Examples:
//   +0.95 = "I'm very confident this is TRUE"
//   +0.60 = "I think this is probably true"
//    0.00 = "I have no information / neutral"
//   -0.40 = "I suspect this might be wrong"
//   -0.90 = "I'm very confident this is FALSE"
//
// ============================================================================

#ifndef ZETA_TERNARY_EDGE_H
#define ZETA_TERNARY_EDGE_H

#include <cmath>
#include <cstdint>
#include <algorithm>

// ============================================================================
// CORE TYPES
// ============================================================================

// Continuous ternary belief value
typedef float zeta_belief_t;  // Range: [-1.0, +1.0]

// Enhanced edge with continuous ternary logic
typedef struct {
    int64_t src;              // Source node ID
    int64_t tgt;              // Target node ID
    uint8_t type;             // Edge type (IS_A, HAS, CAUSES, etc.)

    // === CONTINUOUS TERNARY ===
    zeta_belief_t belief;     // -1.0 to +1.0 (core ternary value)

    // === METADATA ===
    uint32_t created_at;      // Unix timestamp
    uint32_t last_activated;  // For decay calculation
    uint16_t activation_count;// Usage frequency
    uint8_t source_mode;      // CHAT, CODE, RESEARCH, etc.
    uint8_t flags;            // BIDIRECTIONAL, TRANSITIVE, etc.
} zeta_edge_ternary_t;

// ============================================================================
// BELIEF INTERPRETATION
// ============================================================================

// Get the polarity (direction) of belief
static inline int zeta_belief_polarity(zeta_belief_t b) {
    if (b > 0.05f) return +1;   // Positive belief
    if (b < -0.05f) return -1;  // Negative belief
    return 0;                    // Neutral/unknown (deadzone)
}

// Get confidence (magnitude) regardless of direction
static inline float zeta_belief_confidence(zeta_belief_t b) {
    return fabsf(b);
}

// Is this a strong belief? (>0.7 magnitude)
static inline bool zeta_belief_is_strong(zeta_belief_t b) {
    return fabsf(b) > 0.7f;
}

// Is this uncertain? (in the neutral zone)
static inline bool zeta_belief_is_uncertain(zeta_belief_t b) {
    return fabsf(b) < 0.3f;
}

// Human-readable interpretation
static inline const char* zeta_belief_label(zeta_belief_t b) {
    if (b > 0.8f)  return "strongly believe";
    if (b > 0.5f)  return "believe";
    if (b > 0.2f)  return "lean toward";
    if (b > -0.2f) return "uncertain";
    if (b > -0.5f) return "lean against";
    if (b > -0.8f) return "disbelieve";
    return "strongly disbelieve";
}

// ============================================================================
// BELIEF CONSTRUCTION
// ============================================================================

// Create belief from separate polarity and confidence
// polarity: -1, 0, +1
// confidence: 0.0 to 1.0
static inline zeta_belief_t zeta_make_belief(int polarity, float confidence) {
    confidence = std::clamp(confidence, 0.0f, 1.0f);
    if (polarity == 0) return 0.0f;
    return polarity > 0 ? confidence : -confidence;
}

// Create belief from evidence type
typedef enum {
    EVIDENCE_STATED = 0,      // User explicitly said it → high confidence
    EVIDENCE_INFERRED = 1,    // LLM inferred → medium confidence
    EVIDENCE_RETRIEVED = 2,   // From external source → medium-high
    EVIDENCE_ASSUMED = 3,     // Default assumption → low confidence
    EVIDENCE_CONTESTED = 4,   // Contradictory evidence → uncertain
} zeta_evidence_type_t;

static inline zeta_belief_t zeta_belief_from_evidence(
    int polarity,
    zeta_evidence_type_t evidence
) {
    float conf;
    switch (evidence) {
        case EVIDENCE_STATED:    conf = 0.95f; break;
        case EVIDENCE_INFERRED:  conf = 0.65f; break;
        case EVIDENCE_RETRIEVED: conf = 0.80f; break;
        case EVIDENCE_ASSUMED:   conf = 0.40f; break;
        case EVIDENCE_CONTESTED: conf = 0.15f; break;
        default:                 conf = 0.50f; break;
    }
    return zeta_make_belief(polarity, conf);
}

// ============================================================================
// BELIEF COMBINATION (CONSENSUS)
// ============================================================================

// Combine two beliefs about the same relationship
// Uses weighted averaging with conflict detection
static inline zeta_belief_t zeta_belief_combine(
    zeta_belief_t a,
    zeta_belief_t b
) {
    // If one is neutral, use the other
    if (fabsf(a) < 0.05f) return b;
    if (fabsf(b) < 0.05f) return a;

    // Same direction: reinforce (average toward stronger)
    if ((a > 0) == (b > 0)) {
        // Weighted by confidence - stronger belief pulls more
        float weight_a = fabsf(a);
        float weight_b = fabsf(b);
        float total = weight_a + weight_b;
        return (a * weight_a + b * weight_b) / total;
    }

    // Opposite directions: conflict resolution
    // Stronger belief wins, but confidence is reduced
    float diff = a + b;  // Net direction
    float conflict_penalty = 0.3f * std::min(fabsf(a), fabsf(b));

    if (fabsf(diff) < conflict_penalty) {
        return 0.0f;  // Too close to call → uncertain
    }

    // Winner takes reduced confidence
    return diff > 0
        ? std::max(0.0f, diff - conflict_penalty)
        : std::min(0.0f, diff + conflict_penalty);
}

// Combine multiple beliefs (e.g., from multiple sources)
static inline zeta_belief_t zeta_belief_consensus(
    const zeta_belief_t* beliefs,
    int count
) {
    if (count == 0) return 0.0f;
    if (count == 1) return beliefs[0];

    zeta_belief_t result = beliefs[0];
    for (int i = 1; i < count; i++) {
        result = zeta_belief_combine(result, beliefs[i]);
    }
    return result;
}

// ============================================================================
// BELIEF PROPAGATION
// ============================================================================

// Propagate belief through a chain (A→B→C)
// Belief degrades with each hop
static inline zeta_belief_t zeta_belief_propagate(
    zeta_belief_t belief,
    float edge_weight,    // How strong is the connection? 0.0-1.0
    int hops              // Distance from source
) {
    // Decay factor per hop (belief loses ~20% per hop)
    float decay = powf(0.8f, (float)hops);

    // Further attenuate by edge weight
    return belief * edge_weight * decay;
}

// ============================================================================
// BELIEF UPDATE (LEARNING)
// ============================================================================

// Update belief based on new evidence
// learning_rate: how much to trust new evidence (0.0-1.0)
static inline zeta_belief_t zeta_belief_update(
    zeta_belief_t current,
    zeta_belief_t new_evidence,
    float learning_rate
) {
    learning_rate = std::clamp(learning_rate, 0.0f, 1.0f);
    return current * (1.0f - learning_rate) + new_evidence * learning_rate;
}

// Bayesian-style update: new evidence shifts belief
static inline zeta_belief_t zeta_belief_bayesian_update(
    zeta_belief_t prior,          // Current belief
    zeta_belief_t likelihood,     // P(evidence | hypothesis)
    float evidence_strength       // How strong is this evidence? 0.0-1.0
) {
    // Convert to probability-like [0,1] space
    float p_prior = (prior + 1.0f) / 2.0f;
    float p_likelihood = (likelihood + 1.0f) / 2.0f;

    // Weighted shift toward evidence
    float shift = (p_likelihood - 0.5f) * evidence_strength;
    float p_posterior = std::clamp(p_prior + shift * p_prior * (1.0f - p_prior) * 4.0f, 0.0f, 1.0f);

    // Convert back to [-1, +1]
    return p_posterior * 2.0f - 1.0f;
}

// ============================================================================
// TEMPORAL DECAY
// ============================================================================

// Belief decays toward neutral over time
static inline zeta_belief_t zeta_belief_decay(
    zeta_belief_t belief,
    float days_elapsed,
    float half_life_days   // Days for belief to decay to half strength
) {
    if (half_life_days <= 0.0f) return belief;
    float decay = powf(0.5f, days_elapsed / half_life_days);
    return belief * decay;
}

// ============================================================================
// CONTRADICTION DETECTION
// ============================================================================

// Check if two beliefs are contradictory
static inline bool zeta_beliefs_contradict(
    zeta_belief_t a,
    zeta_belief_t b,
    float threshold   // How opposed must they be? (default 0.5)
) {
    // Both must be confident and opposite
    return (a > threshold && b < -threshold) ||
           (a < -threshold && b > threshold);
}

// Contradiction strength: how strongly do they oppose?
static inline float zeta_contradiction_strength(
    zeta_belief_t a,
    zeta_belief_t b
) {
    if ((a > 0) == (b > 0)) return 0.0f;  // Same direction = no contradiction
    return fabsf(a) * fabsf(b);  // Product of confidences
}

// ============================================================================
// EDGE OPERATIONS
// ============================================================================

// Create a new ternary edge
static inline zeta_edge_ternary_t zeta_create_ternary_edge(
    int64_t src,
    int64_t tgt,
    uint8_t type,
    zeta_belief_t belief,
    uint8_t source_mode
) {
    zeta_edge_ternary_t e;
    e.src = src;
    e.tgt = tgt;
    e.type = type;
    e.belief = std::clamp(belief, -1.0f, 1.0f);
    e.created_at = (uint32_t)time(NULL);
    e.last_activated = e.created_at;
    e.activation_count = 1;
    e.source_mode = source_mode;
    e.flags = 0;
    return e;
}

// Activate edge (used in retrieval)
static inline void zeta_edge_activate(zeta_edge_ternary_t* e) {
    e->last_activated = (uint32_t)time(NULL);
    if (e->activation_count < 65535) e->activation_count++;
}

// Get effective belief (with decay)
static inline zeta_belief_t zeta_edge_effective_belief(
    const zeta_edge_ternary_t* e,
    float half_life_days
) {
    uint32_t now = (uint32_t)time(NULL);
    float days = (float)(now - e->last_activated) / 86400.0f;

    // Activation count slows decay (frequently used edges persist)
    float adjusted_half_life = half_life_days * (1.0f + logf(1.0f + e->activation_count) / 5.0f);

    return zeta_belief_decay(e->belief, days, adjusted_half_life);
}

// ============================================================================
// QUERY HELPERS
// ============================================================================

// Should this edge be included in retrieval?
static inline bool zeta_edge_should_retrieve(
    const zeta_edge_ternary_t* e,
    float min_confidence,     // Minimum |belief| to include
    bool include_negative     // Include disbelieved edges?
) {
    float conf = fabsf(e->belief);
    if (conf < min_confidence) return false;
    if (!include_negative && e->belief < 0) return false;
    return true;
}

// Score edge for ranking (higher = more relevant)
static inline float zeta_edge_retrieval_score(
    const zeta_edge_ternary_t* e,
    float semantic_similarity,  // Query-edge similarity
    float half_life_days
) {
    float belief = zeta_edge_effective_belief(e, half_life_days);
    float confidence = fabsf(belief);

    // Positive beliefs score higher than negative
    float polarity_boost = (belief > 0) ? 1.0f : 0.5f;

    // Combine factors
    return confidence * semantic_similarity * polarity_boost;
}

// ============================================================================
// SERIALIZATION
// ============================================================================

// Serialize edge to bytes (28 bytes)
static inline void zeta_edge_serialize(
    const zeta_edge_ternary_t* e,
    uint8_t* out
) {
    memcpy(out + 0, &e->src, 8);
    memcpy(out + 8, &e->tgt, 8);
    out[16] = e->type;
    memcpy(out + 17, &e->belief, 4);
    memcpy(out + 21, &e->created_at, 4);
    memcpy(out + 25, &e->activation_count, 2);
    out[27] = e->source_mode;
}

// Deserialize edge from bytes
static inline zeta_edge_ternary_t zeta_edge_deserialize(const uint8_t* in) {
    zeta_edge_ternary_t e;
    memcpy(&e.src, in + 0, 8);
    memcpy(&e.tgt, in + 8, 8);
    e.type = in[16];
    memcpy(&e.belief, in + 17, 4);
    memcpy(&e.created_at, in + 21, 4);
    memcpy(&e.activation_count, in + 25, 2);
    e.source_mode = in[27];
    e.last_activated = e.created_at;
    e.flags = 0;
    return e;
}

#endif // ZETA_TERNARY_EDGE_H
