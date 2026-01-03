// Z.E.T.A. Deliberation Engine - "Think Before You Think" Architecture
// Structural verification before token emission
//
// This module implements the 5-subsystem Deliberation Weights:
//   1. Graph Navigator (2.5B) - Query → Graph Traversal
//   2. Consistency Verifier (2.0B) - Contradiction Detection
//   3. Confidence Gate (1.5B) - Evidence Weighting
//   4. Veto Network (2.0B) - Constitutional + Safety Checks
//   5. Output Synthesizer (2.0B) - Verified → Natural Language
//
// Key Principle: The model doesn't predict tokens; it navigates structure.
// If the graph doesn't contain verifiable evidence, output states uncertainty.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_DELIBERATION_H
#define ZETA_DELIBERATION_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>

// Forward declarations
struct llama_context;
struct llama_model;

extern "C" {
#include "zeta-graph-kv.h"
#include "zeta-memory.h"
#include "zeta-constitution.h"
}

#include "zeta-dual-process.h"
#include "zeta-scratch-buffer.h"
#include "zeta-critic.h"

// ============================================================================
// Configuration
// ============================================================================

#define DELIB_CONFIDENCE_THRESHOLD      0.80f   // Minimum confidence to make claims
#define DELIB_MAX_ITERATIONS            5       // Max deliberation loops
#define DELIB_MAX_GRAPH_DEPTH           4       // Max BFS depth in graph
#define DELIB_MAX_EVIDENCE_NODES        32      // Max nodes to consider
#define DELIB_REFLECTION_THRESHOLD      0.70f   // Trigger reflection below this
#define DELIB_LATENCY_BUDGET_MS         50      // Target deliberation overhead

// ============================================================================
// Veto Reasons (Structural, not pattern-matched)
// ============================================================================

typedef enum {
    VETO_NONE = 0,                      // Output allowed
    VETO_CONSTITUTIONAL,                // Violates Z.E.T.A. Constitution
    VETO_FACTUAL_CONFLICT,              // Contradicts graph facts
    VETO_LOGICAL_PARADOX,               // Self-referential or impossible
    VETO_INSUFFICIENT_EVIDENCE,         // Cannot verify claim
    VETO_TEMPORAL_INCONSISTENCY,        // Uses outdated information
    VETO_CAUSAL_VIOLATION,              // Breaks known causal chain
    VETO_SAFETY_BLOCK,                  // Harmful content detected
    VETO_CONFIDENCE_TOO_LOW,            // Below confidence threshold
} zeta_veto_reason_t;

// ============================================================================
// Deliberation State
// ============================================================================

typedef enum {
    DELIB_STATE_IDLE,                   // Not deliberating
    DELIB_STATE_QUERYING,               // Querying graph
    DELIB_STATE_VERIFYING,              // Checking consistency
    DELIB_STATE_GATING,                 // Computing confidence
    DELIB_STATE_VETOING,                // Running veto checks
    DELIB_STATE_SYNTHESIZING,           // Generating output
    DELIB_STATE_REFLECTING,             // Self-evaluation loop
    DELIB_STATE_COMPLETE,               // Deliberation finished
} zeta_delib_state_t;

// ============================================================================
// Evidence Node (from graph retrieval)
// ============================================================================

typedef struct {
    int64_t node_id;
    char content[1024];
    float trust;                        // 0.0 - 1.0 (USER facts = 1.0)
    float salience;                     // Relevance to query
    float recency;                      // Temporal decay factor
    int64_t timestamp;
    bool has_supersedes;                // Newer version exists?
    int64_t superseded_by;              // If so, which node?
} zeta_evidence_t;

// ============================================================================
// Consistency Check Result
// ============================================================================

typedef struct {
    bool is_self_consistent;            // No internal contradictions
    bool is_temporally_valid;           // Using current information
    bool is_causally_valid;             // Respects causal chains
    int num_conflicts;
    char conflicts[4][256];             // Up to 4 conflict descriptions
    float conflict_severity;            // 0.0 = minor, 1.0 = critical
} zeta_consistency_t;

// ============================================================================
// Confidence Computation
// ============================================================================

typedef struct {
    float raw_confidence;               // Before calibration
    float calibrated_confidence;        // After learned adjustment
    int evidence_count;
    float avg_trust;
    float avg_salience;
    float avg_recency;
    bool passes_threshold;
} zeta_confidence_t;

// ============================================================================
// Veto Result
// ============================================================================

typedef struct {
    zeta_veto_reason_t reason;
    bool is_blocked;
    char explanation[512];
    float severity;                     // 0.0 = soft block, 1.0 = hard block

    // For debugging/logging
    bool constitutional_check_passed;
    bool factual_check_passed;
    bool logical_check_passed;
    bool safety_check_passed;
} zeta_veto_result_t;

// ============================================================================
// Reflection Result (Self-Evaluation)
// ============================================================================

typedef struct {
    bool is_satisfactory;
    char revision_suggestion[512];
    float meta_confidence;              // Confidence in our confidence
    bool needs_more_evidence;
    bool needs_clarification;
    bool is_tangential;                 // Doesn't address query
} zeta_reflection_t;

// ============================================================================
// Full Deliberation Result
// ============================================================================

typedef struct {
    // State tracking
    zeta_delib_state_t final_state;
    int iterations;

    // Evidence gathered
    zeta_evidence_t evidence[DELIB_MAX_EVIDENCE_NODES];
    int evidence_count;

    // Checks performed
    zeta_consistency_t consistency;
    zeta_confidence_t confidence;
    zeta_veto_result_t veto;
    zeta_reflection_t reflection;

    // Output
    char verified_output[4096];
    bool output_is_verified;
    bool output_states_uncertainty;

    // Performance
    int64_t latency_us;
    bool within_budget;
} zeta_deliberation_t;

// ============================================================================
// Callbacks (set by server, implemented by model inference)
// ============================================================================

// Graph navigation callback: query → relevant node IDs
typedef std::function<std::vector<int64_t>(
    const std::string& query,
    int max_depth,
    float min_salience
)> delib_navigate_fn;

// Consistency check callback: evidence → consistency result
typedef std::function<zeta_consistency_t(
    const std::string& proposed_claim,
    const std::vector<zeta_evidence_t>& evidence
)> delib_verify_fn;

// Confidence computation callback: evidence → confidence score
typedef std::function<zeta_confidence_t(
    const std::vector<zeta_evidence_t>& evidence
)> delib_gate_fn;

// Veto check callback: output + context → veto decision
typedef std::function<zeta_veto_result_t(
    const std::string& proposed_output,
    const zeta_consistency_t& consistency,
    const zeta_confidence_t& confidence
)> delib_veto_fn;

// Output synthesis callback: verified context → natural language
typedef std::function<std::string(
    const std::string& query,
    const std::vector<zeta_evidence_t>& evidence,
    const zeta_confidence_t& confidence
)> delib_synthesize_fn;

// Reflection callback: draft → self-evaluation
typedef std::function<zeta_reflection_t(
    const std::string& query,
    const std::string& draft,
    const std::vector<zeta_evidence_t>& evidence
)> delib_reflect_fn;

// ============================================================================
// Main Deliberation Engine Class
// ============================================================================

class ZetaDeliberation {
private:
    // Core dependencies
    zeta_dual_ctx_t* dual_ctx;
    zeta_scratch_buffer_t* scratch;
    zeta_gkv_ctx_t* gkv_ctx;

    // Model callbacks (set via set_callbacks)
    delib_navigate_fn navigate_fn;
    delib_verify_fn verify_fn;
    delib_gate_fn gate_fn;
    delib_veto_fn veto_fn;
    delib_synthesize_fn synthesize_fn;
    delib_reflect_fn reflect_fn;

    // State
    std::atomic<zeta_delib_state_t> current_state{DELIB_STATE_IDLE};
    std::mutex delib_mutex;
    bool initialized;

    // Configuration (can be adjusted at runtime)
    float confidence_threshold;
    int max_iterations;
    int max_graph_depth;
    int64_t latency_budget_us;

    // Performance tracking
    std::chrono::high_resolution_clock::time_point delib_start;
    int64_t total_latency_us;

public:
    // ========================================================================
    // Lifecycle
    // ========================================================================

    ZetaDeliberation()
        : dual_ctx(nullptr)
        , scratch(nullptr)
        , gkv_ctx(nullptr)
        , initialized(false)
        , confidence_threshold(DELIB_CONFIDENCE_THRESHOLD)
        , max_iterations(DELIB_MAX_ITERATIONS)
        , max_graph_depth(DELIB_MAX_GRAPH_DEPTH)
        , latency_budget_us(DELIB_LATENCY_BUDGET_MS * 1000)
        , total_latency_us(0)
    {}

    bool init(zeta_dual_ctx_t* dual, zeta_scratch_buffer_t* sb, zeta_gkv_ctx_t* gkv) {
        if (!dual || !sb) {
            fprintf(stderr, "[DELIB] Init failed: missing context\n");
            return false;
        }

        dual_ctx = dual;
        scratch = sb;
        gkv_ctx = gkv;
        initialized = true;

        fprintf(stderr, "[DELIB] Deliberation Engine initialized\n");
        fprintf(stderr, "[DELIB]   Confidence threshold: %.2f\n", confidence_threshold);
        fprintf(stderr, "[DELIB]   Max iterations: %d\n", max_iterations);
        fprintf(stderr, "[DELIB]   Latency budget: %lld us\n", (long long)latency_budget_us);

        return true;
    }

    void set_callbacks(
        delib_navigate_fn nav,
        delib_verify_fn ver,
        delib_gate_fn gate,
        delib_veto_fn veto,
        delib_synthesize_fn synth,
        delib_reflect_fn refl
    ) {
        navigate_fn = nav;
        verify_fn = ver;
        gate_fn = gate;
        veto_fn = veto;
        synthesize_fn = synth;
        reflect_fn = refl;

        fprintf(stderr, "[DELIB] All 6 deliberation callbacks registered\n");
    }

    bool is_ready() const {
        return initialized && navigate_fn && verify_fn && gate_fn &&
               veto_fn && synthesize_fn && reflect_fn;
    }

    // ========================================================================
    // Main Deliberation Entry Point
    // ========================================================================

    zeta_deliberation_t deliberate(const std::string& query) {
        std::lock_guard<std::mutex> lock(delib_mutex);

        zeta_deliberation_t result = {};
        result.final_state = DELIB_STATE_IDLE;

        if (!is_ready()) {
            fprintf(stderr, "[DELIB] Not ready - missing callbacks or context\n");
            snprintf(result.verified_output, sizeof(result.verified_output),
                     "I'm unable to process this request due to a system issue.");
            return result;
        }

        delib_start = std::chrono::high_resolution_clock::now();

        // ====================================================================
        // PHASE 1: GRAPH NAVIGATION
        // ====================================================================

        current_state = DELIB_STATE_QUERYING;
        fprintf(stderr, "[DELIB] Phase 1: Querying graph for '%s'\n",
                query.substr(0, 50).c_str());

        auto node_ids = navigate_fn(query, max_graph_depth, 0.3f);

        // Retrieve full evidence from graph
        for (size_t i = 0; i < node_ids.size() && result.evidence_count < DELIB_MAX_EVIDENCE_NODES; i++) {
            zeta_evidence_t ev = retrieve_evidence(node_ids[i]);
            if (ev.node_id > 0) {
                result.evidence[result.evidence_count++] = ev;
            }
        }

        fprintf(stderr, "[DELIB]   Retrieved %d evidence nodes\n", result.evidence_count);

        // ====================================================================
        // PHASE 2: CONSISTENCY VERIFICATION
        // ====================================================================

        current_state = DELIB_STATE_VERIFYING;
        fprintf(stderr, "[DELIB] Phase 2: Verifying consistency\n");

        std::vector<zeta_evidence_t> evidence_vec(
            result.evidence,
            result.evidence + result.evidence_count
        );

        // Generate initial claim based on evidence
        std::string initial_claim = synthesize_initial_claim(query, evidence_vec);

        result.consistency = verify_fn(initial_claim, evidence_vec);

        fprintf(stderr, "[DELIB]   Self-consistent: %s\n",
                result.consistency.is_self_consistent ? "YES" : "NO");
        fprintf(stderr, "[DELIB]   Temporally valid: %s\n",
                result.consistency.is_temporally_valid ? "YES" : "NO");
        fprintf(stderr, "[DELIB]   Conflicts: %d\n", result.consistency.num_conflicts);

        // ====================================================================
        // PHASE 3: CONFIDENCE GATING
        // ====================================================================

        current_state = DELIB_STATE_GATING;
        fprintf(stderr, "[DELIB] Phase 3: Computing confidence\n");

        result.confidence = gate_fn(evidence_vec);

        fprintf(stderr, "[DELIB]   Confidence: %.3f (threshold: %.3f)\n",
                result.confidence.calibrated_confidence, confidence_threshold);

        // ====================================================================
        // PHASE 4: VETO CHECK
        // ====================================================================

        current_state = DELIB_STATE_VETOING;
        fprintf(stderr, "[DELIB] Phase 4: Running veto checks\n");

        result.veto = veto_fn(initial_claim, result.consistency, result.confidence);

        if (result.veto.is_blocked) {
            fprintf(stderr, "[DELIB]   VETO TRIGGERED: %s\n",
                    get_veto_reason_name(result.veto.reason));

            // Generate uncertainty response
            generate_uncertainty_response(&result, result.veto.reason);
            result.output_states_uncertainty = true;
            result.final_state = DELIB_STATE_COMPLETE;
            finalize_latency(&result);
            return result;
        }

        fprintf(stderr, "[DELIB]   All veto checks passed\n");

        // ====================================================================
        // PHASE 5: OUTPUT SYNTHESIS
        // ====================================================================

        current_state = DELIB_STATE_SYNTHESIZING;
        fprintf(stderr, "[DELIB] Phase 5: Synthesizing output\n");

        std::string output = synthesize_fn(query, evidence_vec, result.confidence);

        // ====================================================================
        // PHASE 6: REFLECTION LOOP (if needed)
        // ====================================================================

        result.iterations = 1;

        while (result.iterations < max_iterations) {
            current_state = DELIB_STATE_REFLECTING;
            fprintf(stderr, "[DELIB] Phase 6: Reflection (iteration %d)\n", result.iterations);

            result.reflection = reflect_fn(query, output, evidence_vec);

            if (result.reflection.is_satisfactory &&
                result.reflection.meta_confidence >= DELIB_REFLECTION_THRESHOLD) {
                fprintf(stderr, "[DELIB]   Reflection passed (meta-confidence: %.3f)\n",
                        result.reflection.meta_confidence);
                break;
            }

            fprintf(stderr, "[DELIB]   Reflection suggests revision: %s\n",
                    result.reflection.revision_suggestion);

            // Re-synthesize with reflection feedback
            // (In full implementation, this would re-run relevant phases)

            result.iterations++;
        }

        // ====================================================================
        // FINALIZE
        // ====================================================================

        strncpy(result.verified_output, output.c_str(), sizeof(result.verified_output) - 1);
        result.output_is_verified = true;
        result.output_states_uncertainty = (result.confidence.calibrated_confidence < confidence_threshold);
        result.final_state = DELIB_STATE_COMPLETE;

        finalize_latency(&result);

        fprintf(stderr, "[DELIB] Complete: %d iterations, %lld us, within_budget=%s\n",
                result.iterations, (long long)result.latency_us,
                result.within_budget ? "YES" : "NO");

        return result;
    }

    // ========================================================================
    // Utility Methods
    // ========================================================================

    void set_confidence_threshold(float threshold) {
        confidence_threshold = std::max(0.0f, std::min(1.0f, threshold));
    }

    void set_max_iterations(int max) {
        max_iterations = std::max(1, std::min(10, max));
    }

    void set_latency_budget(int64_t budget_us) {
        latency_budget_us = budget_us;
    }

    zeta_delib_state_t get_state() const {
        return current_state.load();
    }

private:
    // ========================================================================
    // Internal Helpers
    // ========================================================================

    zeta_evidence_t retrieve_evidence(int64_t node_id) {
        zeta_evidence_t ev = {};
        ev.node_id = node_id;

        if (!dual_ctx) return ev;

        // Find node in dual context
        for (int i = 0; i < dual_ctx->num_nodes; i++) {
            if (dual_ctx->nodes[i].node_id == node_id) {
                const auto& node = dual_ctx->nodes[i];

                strncpy(ev.content, node.text, sizeof(ev.content) - 1);
                ev.trust = (node.source == ZETA_SOURCE_USER) ? 1.0f : 0.8f;
                ev.salience = node.salience;
                ev.timestamp = node.created;

                // Compute recency (exponential decay)
                int64_t age_seconds = time(NULL) - node.created;
                float lambda = 0.00001f;  // Very slow decay
                ev.recency = expf(-lambda * age_seconds);

                // Check for supersession
                ev.has_supersedes = (node.supersedes_id > 0);
                ev.superseded_by = node.supersedes_id;

                break;
            }
        }

        return ev;
    }

    std::string synthesize_initial_claim(
        const std::string& query,
        const std::vector<zeta_evidence_t>& evidence
    ) {
        // Simple claim generation from evidence
        // (In full implementation, this uses the 2B Output Synthesizer)

        if (evidence.empty()) {
            return "No verified information available.";
        }

        std::string claim = evidence[0].content;
        return claim;
    }

    void generate_uncertainty_response(
        zeta_deliberation_t* result,
        zeta_veto_reason_t reason
    ) {
        switch (reason) {
            case VETO_INSUFFICIENT_EVIDENCE:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I don't have verified information about this topic. "
                    "Would you like me to search for more context?");
                break;

            case VETO_CONFIDENCE_TOO_LOW:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I found some relevant information, but I'm not confident enough "
                    "to make a definitive claim. Here's what I found: [evidence summary]");
                break;

            case VETO_FACTUAL_CONFLICT:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I found conflicting information about this. "
                    "Would you like me to explain the different perspectives?");
                break;

            case VETO_TEMPORAL_INCONSISTENCY:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "My information about this may be outdated. "
                    "Would you like me to check for more recent data?");
                break;

            case VETO_CONSTITUTIONAL:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I'm unable to assist with this request.");
                break;

            case VETO_SAFETY_BLOCK:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I'm unable to assist with this request.");
                break;

            default:
                snprintf(result->verified_output, sizeof(result->verified_output),
                    "I'm unable to provide a verified response to this query.");
        }
    }

    void finalize_latency(zeta_deliberation_t* result) {
        auto now = std::chrono::high_resolution_clock::now();
        result->latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now - delib_start
        ).count();
        result->within_budget = (result->latency_us <= latency_budget_us);
    }

    const char* get_veto_reason_name(zeta_veto_reason_t reason) {
        switch (reason) {
            case VETO_NONE: return "NONE";
            case VETO_CONSTITUTIONAL: return "CONSTITUTIONAL";
            case VETO_FACTUAL_CONFLICT: return "FACTUAL_CONFLICT";
            case VETO_LOGICAL_PARADOX: return "LOGICAL_PARADOX";
            case VETO_INSUFFICIENT_EVIDENCE: return "INSUFFICIENT_EVIDENCE";
            case VETO_TEMPORAL_INCONSISTENCY: return "TEMPORAL_INCONSISTENCY";
            case VETO_CAUSAL_VIOLATION: return "CAUSAL_VIOLATION";
            case VETO_SAFETY_BLOCK: return "SAFETY_BLOCK";
            case VETO_CONFIDENCE_TOO_LOW: return "CONFIDENCE_TOO_LOW";
            default: return "UNKNOWN";
        }
    }
};

// ============================================================================
// Global Instance (set by server)
// ============================================================================

static ZetaDeliberation g_deliberation;

// ============================================================================
// Convenience Functions
// ============================================================================

static inline bool zeta_deliberation_init(
    zeta_dual_ctx_t* dual,
    zeta_scratch_buffer_t* scratch,
    zeta_gkv_ctx_t* gkv
) {
    return g_deliberation.init(dual, scratch, gkv);
}

static inline zeta_deliberation_t zeta_deliberate(const std::string& query) {
    return g_deliberation.deliberate(query);
}

static inline bool zeta_deliberation_ready() {
    return g_deliberation.is_ready();
}

#endif // ZETA_DELIBERATION_H
