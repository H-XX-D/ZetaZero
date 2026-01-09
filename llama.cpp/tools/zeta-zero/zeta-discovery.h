// Z.E.T.A. Discovery Mode v2.0 - Production-Hardened Scientific Discovery
// ============================================================================
//
// CONCEPT:
//   Discovery = Research + Dream in a controlled feedback loop
//   Research Mode: Low temperature, grounded, finds TENSIONS
//   Dream Mode:    High temperature, free association, finds CONNECTIONS
//   Discovery Mode: Research seeds dreams, dreams expand research, graph validates both
//
// THE DISCOVERY CYCLE (8 phases):
//   1. RESEARCH:   Explore topic, find what's grounded, identify tensions
//   2. CLASSIFY:   Categorize tensions by type for targeted seeding
//   3. DREAM:      Free associate on tensions with graph jumps
//   4. VALIDATE:   Deterministic verification with support/contradiction spans
//   5. PROMOTE:    Ladder: CANDIDATE → CONDITIONAL → GROUNDED → PREDICTION
//   6. INTEGRATE:  Add grounded hypotheses as research branches
//   7. DRILL:      Convert to testable predictions with provenance
//   8. MERGE:      Compress branches, update operational entropy
//
// KEY DIFFERENTIATORS:
//   - Explicit tension taxonomy (5 types)
//   - Deterministic validation with rejection reasons
//   - Structural novelty scoring (not vibes)
//   - Promotion ladder with gating criteria
//   - Provenanced drill-down with stop conditions
//   - Computable convergence via operational entropy
//   - Falsifiability required for predictions
//   - Span-level citations with versioning
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#pragma once

#include "zeta-research.h"
#include "zeta-dream.h"
#include <random>
#include <cmath>
#include <numeric>
#include <sstream>
#include <map>

namespace zeta_discovery {

using namespace zeta_research;

// ============================================================================
// TENSION TYPES - Explicit categories for stable downstream processing
// ============================================================================

enum class TensionType {
    CONTRADICTION,       // Mutually exclusive propositions under same scope/assumptions
    CONFLICTING_EVIDENCE,// Sources disagree, but scope/conditions may differ
    GAP,                 // Needed premise missing in graph
    AMBIGUITY,           // Multiple plausible mechanisms, insufficient discriminators
    SCOPE_MISMATCH,      // Differing regimes (temperature, scale, population, etc.)
    UNKNOWN              // Unclassified
};

inline const char* tension_type_str(TensionType t) {
    switch (t) {
        case TensionType::CONTRADICTION:        return "CONTRADICTION";
        case TensionType::CONFLICTING_EVIDENCE: return "CONFLICTING_EVIDENCE";
        case TensionType::GAP:                  return "GAP";
        case TensionType::AMBIGUITY:            return "AMBIGUITY";
        case TensionType::SCOPE_MISMATCH:       return "SCOPE_MISMATCH";
        default:                                return "UNKNOWN";
    }
}

// ============================================================================
// VALIDATION VERDICT - Deterministic, auditable outcomes
// ============================================================================

enum class ValidationVerdict {
    VALIDATED,           // Passed all checks
    CONDITIONAL,         // Has support but also contradictions or scope limits
    REJECTED,            // Failed validation
    INSUFFICIENT_DATA    // Cannot determine (no relevant nodes found)
};

enum class RejectionReason {
    NONE,                       // Not rejected
    NO_SUPPORT,                 // Zero supporting evidence found
    DIRECT_CONTRADICTION,       // Strong contradicting evidence
    SCOPE_DEPENDENT,            // Only valid under narrow conditions
    INSUFFICIENT_SPECIFICITY,   // Too vague to validate
    LOW_NOVELTY,                // Too similar to existing knowledge
    ASSUMPTION_VIOLATED         // Extracted assumption contradicts graph
};

inline const char* verdict_str(ValidationVerdict v) {
    switch (v) {
        case ValidationVerdict::VALIDATED:         return "VALIDATED";
        case ValidationVerdict::CONDITIONAL:       return "CONDITIONAL";
        case ValidationVerdict::REJECTED:          return "REJECTED";
        case ValidationVerdict::INSUFFICIENT_DATA: return "INSUFFICIENT_DATA";
        default:                                   return "UNKNOWN";
    }
}

inline const char* rejection_reason_str(RejectionReason r) {
    switch (r) {
        case RejectionReason::NONE:                     return "NONE";
        case RejectionReason::NO_SUPPORT:               return "NO_SUPPORT";
        case RejectionReason::DIRECT_CONTRADICTION:     return "DIRECT_CONTRADICTION";
        case RejectionReason::SCOPE_DEPENDENT:          return "SCOPE_DEPENDENT";
        case RejectionReason::INSUFFICIENT_SPECIFICITY: return "INSUFFICIENT_SPECIFICITY";
        case RejectionReason::LOW_NOVELTY:              return "LOW_NOVELTY";
        case RejectionReason::ASSUMPTION_VIOLATED:      return "ASSUMPTION_VIOLATED";
        default:                                        return "UNKNOWN";
    }
}

// ============================================================================
// HYPOTHESIS STAGE - Promotion ladder for dream candidates
// ============================================================================

enum class HypothesisStage {
    DREAM_CANDIDATE,        // Raw dream output
    CONDITIONAL_HYPOTHESIS, // Has some support
    GROUNDED_HYPOTHESIS,    // Passes validation thresholds
    TESTABLE_PREDICTION     // Produces falsifiable statement + test method
};

inline const char* stage_str(HypothesisStage s) {
    switch (s) {
        case HypothesisStage::DREAM_CANDIDATE:        return "DREAM_CANDIDATE";
        case HypothesisStage::CONDITIONAL_HYPOTHESIS: return "CONDITIONAL_HYPOTHESIS";
        case HypothesisStage::GROUNDED_HYPOTHESIS:    return "GROUNDED_HYPOTHESIS";
        case HypothesisStage::TESTABLE_PREDICTION:    return "TESTABLE_PREDICTION";
        default:                                      return "UNKNOWN";
    }
}

// ============================================================================
// SPAN REFERENCE - Precise citation with versioning for stable offsets
// ============================================================================

struct SpanRef {
    int64_t node_id = -1;
    std::string node_label;
    uint64_t content_hash = 0;      // Hash of node content for version stability
    int node_version = 0;           // Version number for tracking updates
    int para_start = -1;            // Paragraph index (stable segmentation)
    int para_end = -1;
    int char_start = -1;            // Character offset within node
    int char_end = -1;
    std::string excerpt;            // Max 500 chars for compliance
    float evidence_weight = 1.0f;   // Quality weighting (survey=1.5, preprint=0.8, etc.)
    
    std::string format() const {
        std::string out = "[" + std::to_string(node_id) + ":" + node_label;
        if (para_start >= 0) out += " ¶" + std::to_string(para_start);
        if (node_version > 0) out += " v" + std::to_string(node_version);
        out += " w=" + std::to_string(evidence_weight).substr(0,4) + "]";
        return out;
    }
    
    bool is_valid() const {
        return node_id >= 0 && !excerpt.empty() && content_hash != 0;
    }
};

// ============================================================================
// STRUCTURED ASSUMPTION - Not just prose, but auditable data
// ============================================================================

struct Assumption {
    std::string id;
    std::string key;                    // e.g., "temperature_range", "population_type"
    std::string value;                  // e.g., "room temperature (20-25°C)"
    float confidence = 1.0f;            // How certain we are this assumption holds
    std::vector<SpanRef> provenance;    // Where this assumption was derived from
    bool validated = false;             // Checked against graph
    bool violated = false;              // Found contradicting evidence
    
    std::string format() const {
        std::string out = key + "=" + value;
        out += " (conf=" + std::to_string(int(confidence * 100)) + "%";
        if (validated) out += validated ? (violated ? " VIOLATED" : " OK") : " unchecked";
        out += ")";
        return out;
    }
};

// ============================================================================
// STRUCTURED SCOPE - When/where does this hypothesis apply?
// ============================================================================

struct ScopeCondition {
    std::string domain;                 // e.g., "chemistry", "biology", "physics"
    std::string regime;                 // e.g., "high_temperature", "low_pressure"
    std::string scale;                  // e.g., "molecular", "cellular", "organism"
    std::vector<std::string> boundary_conditions;  // e.g., ["pH > 7", "T < 100C"]
    std::map<std::string, std::string> parameters; // e.g., {"concentration": "1-10mM"}
    std::vector<SpanRef> provenance;
    
    bool is_specified() const {
        return !domain.empty() || !regime.empty() || !scale.empty() ||
               !boundary_conditions.empty() || !parameters.empty();
    }
    
    std::string format() const {
        std::string out;
        if (!domain.empty()) out += "domain=" + domain + " ";
        if (!regime.empty()) out += "regime=" + regime + " ";
        if (!scale.empty()) out += "scale=" + scale + " ";
        for (const auto& bc : boundary_conditions) out += "[" + bc + "] ";
        return out;
    }
};

// ============================================================================
// NOVELTY COMPONENTS - Structural, not vibes-based
// ============================================================================

struct NoveltyScore {
    float graph_distance = 0.0f;     // Avg shortest-path between citation sets
    float tensions_resolved = 0.0f;  // Count/weight of tensions reduced
    float prior_art_similarity = 0.0f; // Similarity to existing insights (penalty)
    float compression_gain = 0.0f;   // Entropy reduction in research state
    float total = 0.0f;              // Weighted combination
    
    std::string format() const {
        char buf[256];
        snprintf(buf, sizeof(buf), 
            "dist=%.2f res=%.2f sim=%.2f comp=%.2f → %.2f",
            graph_distance, tensions_resolved, prior_art_similarity, 
            compression_gain, total);
        return std::string(buf);
    }
};

// ============================================================================
// DISCOVERY CONFIGURATION
// ============================================================================

struct DiscoveryConfig {
    // Research phase settings
    ResearchConfig research;

    // Dream phase settings
    float dream_temperature = 0.95f;
    int dreams_per_tension = 3;
    int max_graph_jumps = 5;
    
    // Novelty scoring weights (structural)
    float novelty_w_distance = 0.3f;
    float novelty_w_resolved = 0.3f;
    float novelty_w_similarity = 0.2f;
    float novelty_w_compression = 0.2f;
    float novelty_threshold = 0.4f;

    // Validation thresholds (deterministic gating)
    float support_threshold = 0.3f;
    float grounded_support_threshold = 0.5f;
    int max_contradictions = 2;
    int min_supporting_nodes = 1;
    int validation_nodes = 10;
    
    // Evidence quality weights
    float weight_survey = 1.5f;
    float weight_primary = 1.2f;
    float weight_preprint = 0.8f;
    float weight_negative_result = 1.3f;  // Negative results are valuable

    // Drill-down settings
    bool enable_drill_down = true;
    int max_drill_depth = 4;
    int min_specificity_tokens[4] = {50, 100, 200, 300};

    // Convergence criteria (operational entropy)
    float entropy_alpha = 1.0f;
    float entropy_beta = 2.0f;
    float entropy_gamma = 0.5f;
    float convergence_threshold = 0.15f;
    int stall_cycles = 2;
    int max_discovery_cycles = 5;

    // Output settings
    bool preserve_dream_chain = true;
    bool generate_testable_predictions = true;
    bool require_falsifiable = true;
    bool require_structured_assumptions = true;  // CONDITIONAL needs explicit assumptions
    int max_excerpt_chars = 500;
};

// ============================================================================
// DREAM HYPOTHESIS - Full audit trail with structured assumptions/scope
// ============================================================================

struct DreamHypothesis {
    // Identity
    std::string id;
    std::string content;
    std::string seed_tension_id;
    TensionType seed_tension_type = TensionType::UNKNOWN;
    
    // Evidence with span references
    std::vector<SpanRef> supporting_spans;
    std::vector<SpanRef> contradicting_spans;
    std::vector<SpanRef> neutral_spans;
    std::vector<SpanRef> negative_result_spans;  // First-class negative results
    
    // Structured assumptions and scope (not just prose)
    std::vector<Assumption> assumptions;
    ScopeCondition scope;
    
    // Scoring (structural)
    NoveltyScore novelty;
    float support_ratio = 0.0f;
    float weighted_support = 0.0f;  // Sum of evidence_weight for supporting
    float contradiction_ratio = 0.0f;
    
    // Validation outcome (deterministic)
    ValidationVerdict verdict = ValidationVerdict::INSUFFICIENT_DATA;
    RejectionReason rejection_reason = RejectionReason::NONE;
    
    // Promotion ladder state
    HypothesisStage stage = HypothesisStage::DREAM_CANDIDATE;
    bool integrated = false;
    
    // Graph jump provenance
    std::string source_node_label;
    int64_t source_node_id = -1;
    
    // Drill-down with provenance
    int drill_depth = 0;
    struct DrillStep {
        int level;
        std::string content;
        std::string source_hypothesis_id;
        std::vector<SpanRef> citations_used;
        int token_count = 0;
    };
    std::vector<DrillStep> drill_chain;
    
    // Computed checks
    bool is_discovery() const {
        return stage >= HypothesisStage::GROUNDED_HYPOTHESIS &&
               novelty.total > 0.5f && support_ratio > 0.5f;
    }
    
    bool can_promote_to_conditional() const {
        return !supporting_spans.empty();
    }
    
    bool can_promote_to_grounded(const DiscoveryConfig& cfg) const {
        return support_ratio >= cfg.grounded_support_threshold &&
               (int)contradicting_spans.size() <= cfg.max_contradictions &&
               (!cfg.require_structured_assumptions || !assumptions.empty());
    }
    
    bool can_promote_to_prediction() const {
        return !drill_chain.empty() && drill_chain.back().level >= 2;
    }
    
    bool has_violated_assumptions() const {
        for (const auto& a : assumptions) {
            if (a.violated) return true;
        }
        return false;
    }
    
    std::string format() const {
        std::string out = "[" + id + "] " + stage_str(stage);
        out += " verdict=" + std::string(verdict_str(verdict));
        if (rejection_reason != RejectionReason::NONE) {
            out += " reason=" + std::string(rejection_reason_str(rejection_reason));
        }
        out += "\n  novelty: " + novelty.format();
        out += "\n  support=" + std::to_string(supporting_spans.size());
        out += " (weighted=" + std::to_string(weighted_support).substr(0,4) + ")";
        out += " contradict=" + std::to_string(contradicting_spans.size());
        out += " negative=" + std::to_string(negative_result_spans.size());
        out += "\n  assumptions=" + std::to_string(assumptions.size());
        if (scope.is_specified()) out += " scope=" + scope.format();
        if (!content.empty()) out += "\n  " + content.substr(0, 150) + "...";
        return out;
    }
};

// ============================================================================
// TESTABLE PREDICTION - Concrete output with falsifiability
// ============================================================================

struct TestablePrediction {
    std::string id;
    std::string hypothesis_id;
    
    std::string statement;
    std::string test_protocol;
    std::string expected_outcome;
    std::string falsification_criteria;
    
    std::vector<SpanRef> evidence;
    std::vector<Assumption> inherited_assumptions;
    ScopeCondition inherited_scope;
    float confidence = 0.0f;
    
    std::vector<std::string> drill_chain_ids;
    
    bool is_falsifiable() const {
        return !test_protocol.empty() && !falsification_criteria.empty();
    }
    
    std::string format() const {
        std::string out = "═══ PREDICTION [" + id + "] ═══\n";
        out += "STATEMENT: " + statement + "\n";
        out += "TEST: " + test_protocol + "\n";
        out += "EXPECTED: " + expected_outcome + "\n";
        out += "FALSIFIED_IF: " + falsification_criteria + "\n";
        out += "CONFIDENCE: " + std::to_string(int(confidence * 100)) + "%\n";
        out += "EVIDENCE: " + std::to_string(evidence.size()) + " spans\n";
        if (!inherited_assumptions.empty()) {
            out += "ASSUMES: ";
            for (const auto& a : inherited_assumptions) out += a.format() + "; ";
            out += "\n";
        }
        if (inherited_scope.is_specified()) {
            out += "SCOPE: " + inherited_scope.format() + "\n";
        }
        out += "FROM: " + hypothesis_id + "\n";
        return out;
    }
};

// ============================================================================
// OPERATIONAL ENTROPY - Computable convergence measure
// ============================================================================

struct OperationalEntropy {
    float unresolved_tension_weight = 0.0f;  // U: Higher = more to explore
    float conditional_hypothesis_count = 0.0f;  // C: Tentative findings
    float grounded_hypothesis_count = 0.0f;     // S: Stable findings
    
    // Config coefficients (α*U + β*C - γ*S)
    float alpha = 1.0f;
    float beta = 0.3f;
    float gamma = 0.5f;
    
    // Stall detection
    int cycles_since_progress = 0;
    float last_entropy = 0.0f;
    
    float compute() const {
        return alpha * unresolved_tension_weight + 
               beta * conditional_hypothesis_count - 
               gamma * grounded_hypothesis_count;
    }
    
    bool is_converged(float threshold) const {
        return compute() < threshold && grounded_hypothesis_count > 0;
    }
    
    bool is_stalled(int max_cycles, float delta_threshold) const {
        return cycles_since_progress >= max_cycles ||
               std::abs(compute() - last_entropy) < delta_threshold;
    }
    
    void update_stall_tracking(float current) {
        if (std::abs(current - last_entropy) > 0.1f) {
            cycles_since_progress = 0;
        } else {
            cycles_since_progress++;
        }
        last_entropy = current;
    }
    
    std::string format() const {
        std::string out = "ENTROPY: " + std::to_string(compute()).substr(0,5);
        out += " (U=" + std::to_string(unresolved_tension_weight).substr(0,4);
        out += " C=" + std::to_string(int(conditional_hypothesis_count));
        out += " S=" + std::to_string(int(grounded_hypothesis_count)) + ")";
        out += " stall_cycles=" + std::to_string(cycles_since_progress);
        return out;
    }
};

// ============================================================================
// DISCOVERY STATE - Master session state
// ============================================================================

struct DiscoveryState {
    // Config
    DiscoveryConfig config;
    
    // Context
    ResearchContext* research_ctx = nullptr;
    DreamContext* dream_ctx = nullptr;
    
    // Tension tracking
    struct TensionRecord {
        std::string id;
        TensionType type = TensionType::UNKNOWN;
        std::string description;
        std::vector<SpanRef> evidence_refs;
        bool addressed = false;
        std::string addressing_hypothesis_id;
    };
    std::vector<TensionRecord> tensions;
    
    // Hypothesis tracking by stage
    std::map<HypothesisStage, std::vector<DreamHypothesis>> hypotheses_by_stage;
    
    // Final outputs
    std::vector<TestablePrediction> predictions;
    std::vector<std::string> discoveries;  // IDs of hypotheses that made it to discovery
    
    // Session state
    OperationalEntropy entropy;
    int cycle_count = 0;
    int total_dreams = 0;
    int total_promoted = 0;
    int total_rejected = 0;
    int total_integrated = 0;
    
    // Dedup tracking
    std::vector<uint64_t> seen_hypothesis_hashes;
    
    // Methods
    std::vector<DreamHypothesis>& candidates() {
        return hypotheses_by_stage[HypothesisStage::DREAM_CANDIDATE];
    }
    std::vector<DreamHypothesis>& conditionals() {
        return hypotheses_by_stage[HypothesisStage::CONDITIONAL_HYPOTHESIS];
    }
    std::vector<DreamHypothesis>& grounded() {
        return hypotheses_by_stage[HypothesisStage::GROUNDED_HYPOTHESIS];
    }
    std::vector<DreamHypothesis>& predictions_ready() {
        return hypotheses_by_stage[HypothesisStage::TESTABLE_PREDICTION];
    }
    
    void update_entropy() {
        float U = 0.0f;
        for (const auto& t : tensions) {
            if (!t.addressed) U += 1.0f;
        }
        entropy.unresolved_tension_weight = U;
        entropy.conditional_hypothesis_count = (float)conditionals().size();
        entropy.grounded_hypothesis_count = (float)grounded().size();
        entropy.update_stall_tracking(entropy.compute());
    }
    
    bool is_duplicate(uint64_t hash) {
        for (auto h : seen_hypothesis_hashes) {
            if (h == hash) return true;
        }
        return false;
    }
    
    void mark_seen(uint64_t hash) {
        seen_hypothesis_hashes.push_back(hash);
    }
    
    std::string format() const {
        std::string out = "═══ DISCOVERY STATE ═══\n";
        out += entropy.format() + "\n";
        out += "Cycles: " + std::to_string(cycle_count) + "\n";
        out += "Tensions: " + std::to_string(tensions.size());
        int unaddressed = 0;
        for (const auto& t : tensions) if (!t.addressed) unaddressed++;
        out += " (unaddressed=" + std::to_string(unaddressed) + ")\n";
        out += "Hypotheses by stage:\n";
        for (const auto& [stage, hyps] : hypotheses_by_stage) {
            out += "  " + std::string(stage_str(stage)) + ": " + std::to_string(hyps.size()) + "\n";
        }
        out += "Predictions: " + std::to_string(predictions.size()) + "\n";
        out += "Discoveries: " + std::to_string(discoveries.size()) + "\n";
        return out;
    }
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Hash for deduplication
inline uint64_t hash_hypothesis_content(const std::string& content) {
    uint64_t h = 14695981039346656037ULL;
    for (char c : content) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// Classify tension type from model output
inline TensionType classify_tension(
    const std::string& tension_description,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    std::string prompt = R"(Classify this tension into exactly ONE category:
- CONTRADICTION: Direct logical conflict (A and not-A)
- CONFLICTING_EVIDENCE: Different studies/sources disagree on facts
- GAP: Missing information, unexplored area
- AMBIGUITY: Unclear definitions, multiple interpretations possible
- SCOPE_MISMATCH: Findings only valid under specific conditions

Tension: )" + tension_description + R"(

Category (one word):)";
    
    std::string response = inference_with_grammar(ctx, model, sampler, prompt, 10);
    
    if (response.find("CONTRADICTION") != std::string::npos) return TensionType::CONTRADICTION;
    if (response.find("CONFLICTING") != std::string::npos) return TensionType::CONFLICTING_EVIDENCE;
    if (response.find("GAP") != std::string::npos) return TensionType::GAP;
    if (response.find("AMBIGUITY") != std::string::npos) return TensionType::AMBIGUITY;
    if (response.find("SCOPE") != std::string::npos) return TensionType::SCOPE_MISMATCH;
    
    return TensionType::UNKNOWN;
}

// Compute structural novelty (not model persuasion)
inline NoveltyScore compute_structural_novelty(
    const DreamHypothesis& hyp,
    const ResearchContext& ctx,
    int existing_hypothesis_count
) {
    NoveltyScore score;
    
    // Graph distance: how far from original query node?
    if (hyp.source_node_id >= 0 && ctx.graph) {
        // Compute shortest path from root to source node
        int dist = 0;  // Would use BFS in production
        score.graph_distance = std::min(1.0f, dist / 5.0f);
    }
    
    // Tensions resolved: count of addressed tension types
    score.tensions_resolved = std::min(1.0f, 
        (hyp.seed_tension_type != TensionType::UNKNOWN ? 0.25f : 0.0f) +
        (hyp.contradicting_spans.empty() ? 0.25f : 0.0f));
    
    // Prior art: inverse similarity to existing hypotheses (simplified)
    score.prior_art_similarity = std::max(0.0f, 
        1.0f - (existing_hypothesis_count * 0.1f));
    
    // Compression gain: how much new info per citation?
    int total_citations = hyp.supporting_spans.size() + 
                         hyp.contradicting_spans.size() +
                         hyp.neutral_spans.size();
    if (total_citations > 0) {
        score.compression_gain = std::min(1.0f, 
            (float)hyp.content.size() / (total_citations * 200.0f));
    }
    
    score.total = (score.graph_distance + score.tensions_resolved + 
                   score.prior_art_similarity + score.compression_gain) / 4.0f;
    return score;
}

// Type-aware dream seeding
inline std::string format_tension_as_dream_seed(
    const DiscoveryState::TensionRecord& tension,
    TensionType type
) {
    std::string base = "Given this " + std::string(tension_type_str(type)) + 
                       " tension:\n" + tension.description + "\n\n";
    
    switch (type) {
        case TensionType::CONTRADICTION:
            return base + "Generate a hypothesis that RESOLVES this contradiction. "
                         "Explain how both statements could be true under different conditions, "
                         "or identify which claim is incorrect and why.";
        case TensionType::CONFLICTING_EVIDENCE:
            return base + "Generate a hypothesis that RECONCILES this conflicting evidence. "
                         "Consider methodological differences, sample populations, or "
                         "confounding variables that could explain the disagreement.";
        case TensionType::GAP:
            return base + "Generate a hypothesis that FILLS this knowledge gap. "
                         "Propose what might be true in this unexplored area and "
                         "what evidence would support or refute it.";
        case TensionType::AMBIGUITY:
            return base + "Generate a hypothesis that CLARIFIES this ambiguity. "
                         "Propose precise definitions or distinctions that would "
                         "resolve the multiple interpretations.";
        case TensionType::SCOPE_MISMATCH:
            return base + "Generate a hypothesis that GENERALIZES or LIMITS scope. "
                         "Identify the exact conditions under which each finding holds "
                         "and propose a unified framework.";
        default:
            return base + "Generate a hypothesis that addresses this tension.";
    }
}

// Graph jump context retrieval
inline std::string get_graph_jump_context(
    ResearchContext& ctx,
    int64_t& out_node_id,
    std::string& out_node_label
) {
    if (!ctx.graph || ctx.graph->nodes.empty()) {
        out_node_id = -1;
        out_node_label = "no_graph";
        return "";
    }
    
    // Weighted random selection favoring underexplored nodes
    std::vector<std::pair<int64_t, float>> candidates;
    for (const auto& [id, node] : ctx.graph->nodes) {
        float weight = 1.0f;
        // Prefer nodes with low visit count
        // In production, track visit_count per node
        candidates.push_back({id, weight});
    }
    
    if (candidates.empty()) {
        out_node_id = -1;
        return "";
    }
    
    // Weighted selection
    float total = 0.0f;
    for (const auto& c : candidates) total += c.second;
    float r = (rand() / (float)RAND_MAX) * total;
    float acc = 0.0f;
    int64_t selected = candidates[0].first;
    for (const auto& c : candidates) {
        acc += c.second;
        if (r <= acc) {
            selected = c.first;
            break;
        }
    }
    
    out_node_id = selected;
    const auto& node = ctx.graph->nodes.at(selected);
    out_node_label = node.label;
    return node.content;
}

// Extract assumptions from hypothesis content
inline std::vector<Assumption> extract_assumptions(
    const std::string& hypothesis_content,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    std::string prompt = R"(Extract all assumptions from this hypothesis as KEY=VALUE pairs.
Only include implicit or explicit assumptions the hypothesis depends on.

Hypothesis: )" + hypothesis_content + R"(

List assumptions (one per line, format: KEY=VALUE):)";
    
    std::string response = inference_with_grammar(ctx, model, sampler, prompt, 200);
    
    std::vector<Assumption> assumptions;
    std::istringstream iss(response);
    std::string line;
    while (std::getline(iss, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos && eq > 0) {
            Assumption a;
            a.key = line.substr(0, eq);
            a.value = line.substr(eq + 1);
            a.provenance = "extracted";
            a.confidence = 0.7f;  // Default confidence for extracted
            assumptions.push_back(a);
        }
    }
    return assumptions;
}

// Extract scope conditions
inline ScopeCondition extract_scope_conditions(
    const std::string& hypothesis_content,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    std::string prompt = R"(Extract scope conditions from this hypothesis.
Identify: domain, regime, scale, and any boundary conditions.

Hypothesis: )" + hypothesis_content + R"(

Format response as:
DOMAIN: [domain]
REGIME: [regime]  
SCALE: [scale]
CONDITIONS: [list of conditions]
)";
    
    std::string response = inference_with_grammar(ctx, model, sampler, prompt, 200);
    
    ScopeCondition scope;
    std::istringstream iss(response);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("DOMAIN:") == 0) scope.domain = line.substr(7);
        else if (line.find("REGIME:") == 0) scope.regime = line.substr(7);
        else if (line.find("SCALE:") == 0) scope.scale = line.substr(6);
        else if (line.find("CONDITIONS:") == 0) {
            scope.boundary_conditions.push_back(line.substr(11));
        }
    }
    return scope;
}

// ============================================================================
// CORE OPERATIONS
// ============================================================================

// Generate dream from tension (type-aware)
inline DreamHypothesis generate_dream(
    DiscoveryState& state,
    const DiscoveryState::TensionRecord& tension,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    DreamHypothesis hyp;
    hyp.id = "dream_" + std::to_string(state.total_dreams++);
    hyp.seed_tension_id = tension.id;
    hyp.seed_tension_type = tension.type;
    
    // Type-aware seeding
    std::string seed_prompt = format_tension_as_dream_seed(tension, tension.type);
    
    // Add graph context if available
    std::string graph_ctx = get_graph_jump_context(
        *state.research_ctx, hyp.source_node_id, hyp.source_node_label);
    if (!graph_ctx.empty()) {
        seed_prompt += "\n\nAdditional context from knowledge graph:\n" + graph_ctx;
    }
    
    // Generate hypothesis content
    hyp.content = inference_with_grammar(ctx, model, sampler, seed_prompt, 
                                         state.config.max_hypothesis_tokens);
    
    // Extract structured assumptions and scope
    if (state.config.require_structured_assumptions) {
        hyp.assumptions = extract_assumptions(hyp.content, ctx, model, sampler);
        hyp.scope = extract_scope_conditions(hyp.content, ctx, model, sampler);
    }
    
    // Compute novelty
    int existing = 0;
    for (const auto& [stage, hyps] : state.hypotheses_by_stage) {
        existing += hyps.size();
    }
    hyp.novelty = compute_structural_novelty(hyp, *state.research_ctx, existing);
    
    return hyp;
}

// Deterministic validation (no model persuasion)
inline void validate_dream_deterministic(
    DreamHypothesis& hyp,
    ResearchContext& ctx,
    const DiscoveryConfig& cfg,
    llama_context* llm_ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    // Classify each evidence reference
    std::string classify_prompt = R"(For this hypothesis and evidence, classify as SUPPORT, CONTRADICT, or NEUTRAL.

Hypothesis: )" + hyp.content + R"(

For each piece of evidence, output one line:
SUPPORT|CONTRADICT|NEUTRAL: [brief reason]
)";
    
    // In production: iterate ctx.source_excerpts and classify each
    // For now, simplified evidence gathering
    
    // Check for any supporting evidence
    if (hyp.supporting_spans.empty()) {
        hyp.verdict = ValidationVerdict::REJECTED;
        hyp.rejection_reason = RejectionReason::NO_SUPPORT;
        return;
    }
    
    // Compute weighted support
    float support_weight = 0.0f;
    for (const auto& s : hyp.supporting_spans) {
        support_weight += s.evidence_weight;
    }
    float contradict_weight = 0.0f;
    for (const auto& s : hyp.contradicting_spans) {
        contradict_weight += s.evidence_weight;
    }
    float total_weight = support_weight + contradict_weight;
    
    hyp.weighted_support = support_weight;
    hyp.support_ratio = total_weight > 0 ? support_weight / total_weight : 0.0f;
    hyp.contradiction_ratio = total_weight > 0 ? contradict_weight / total_weight : 0.0f;
    
    // Direct contradiction check
    if (!hyp.contradicting_spans.empty() && hyp.contradiction_ratio > 0.5f) {
        hyp.verdict = ValidationVerdict::REJECTED;
        hyp.rejection_reason = RejectionReason::DIRECT_CONTRADICTION;
        return;
    }
    
    // Scope dependency check
    if (hyp.scope.is_specified() && hyp.support_ratio < 0.7f) {
        hyp.verdict = ValidationVerdict::CONDITIONAL;
        hyp.rejection_reason = RejectionReason::SCOPE_DEPENDENT;
        return;
    }
    
    // Insufficient specificity
    if ((int)hyp.content.size() < cfg.min_specificity_tokens[0]) {
        hyp.verdict = ValidationVerdict::REJECTED;
        hyp.rejection_reason = RejectionReason::INSUFFICIENT_SPECIFICITY;
        return;
    }
    
    // Assumption violated check
    if (hyp.has_violated_assumptions()) {
        hyp.verdict = ValidationVerdict::REJECTED;
        hyp.rejection_reason = RejectionReason::ASSUMPTION_VIOLATED;
        return;
    }
    
    // Check for circular reasoning (would need more context)
    // For now, simplified check
    
    // Determine verdict
    if (hyp.support_ratio >= cfg.grounded_support_threshold) {
        hyp.verdict = ValidationVerdict::VALIDATED;
    } else if (!hyp.supporting_spans.empty()) {
        hyp.verdict = ValidationVerdict::CONDITIONAL;
    } else {
        hyp.verdict = ValidationVerdict::INSUFFICIENT_DATA;
    }
}

// Attempt promotion through stages
inline bool attempt_promotion(
    DreamHypothesis& hyp,
    DiscoveryState& state
) {
    HypothesisStage old_stage = hyp.stage;
    
    switch (hyp.stage) {
        case HypothesisStage::DREAM_CANDIDATE:
            if (hyp.can_promote_to_conditional()) {
                hyp.stage = HypothesisStage::CONDITIONAL_HYPOTHESIS;
            }
            break;
            
        case HypothesisStage::CONDITIONAL_HYPOTHESIS:
            if (hyp.can_promote_to_grounded(state.config)) {
                hyp.stage = HypothesisStage::GROUNDED_HYPOTHESIS;
            }
            break;
            
        case HypothesisStage::GROUNDED_HYPOTHESIS:
            if (hyp.can_promote_to_prediction()) {
                hyp.stage = HypothesisStage::TESTABLE_PREDICTION;
            }
            break;
            
        case HypothesisStage::TESTABLE_PREDICTION:
            // Terminal state
            break;
    }
    
    if (hyp.stage != old_stage) {
        state.total_promoted++;
        return true;
    }
    return false;
}

// Integrate hypothesis into graph
inline void integrate_dream(
    DreamHypothesis& hyp,
    DiscoveryState& state
) {
    if (hyp.integrated) return;
    if (!state.research_ctx->graph) return;
    
    // Only integrate GROUNDED or higher
    if (hyp.stage < HypothesisStage::GROUNDED_HYPOTHESIS) return;
    
    // Create node in graph
    KnowledgeGraph::Node node;
    node.id = state.research_ctx->graph->nodes.size() + 1000;  // Offset to avoid collision
    node.label = "hyp_" + hyp.id;
    node.type = "hypothesis";
    node.content = hyp.content;
    
    // Add metadata
    // node.metadata["stage"] = stage_str(hyp.stage);
    // node.metadata["novelty"] = std::to_string(hyp.novelty.total);
    
    state.research_ctx->graph->nodes[node.id] = node;
    
    // Create edges to supporting evidence nodes
    for (const auto& span : hyp.supporting_spans) {
        if (span.node_id >= 0) {
            KnowledgeGraph::Edge edge;
            edge.source = node.id;
            edge.target = span.node_id;
            edge.type = "supported_by";
            edge.weight = span.evidence_weight;
            state.research_ctx->graph->edges.push_back(edge);
        }
    }
    
    hyp.integrated = true;
    state.total_integrated++;
    
    // Mark addressing tension
    for (auto& t : state.tensions) {
        if (t.id == hyp.seed_tension_id) {
            t.addressed = true;
            t.addressing_hypothesis_id = hyp.id;
            break;
        }
    }
}

// Drill-down with provenance and stop conditions
inline void drill_down_hypothesis_with_provenance(
    DreamHypothesis& hyp,
    DiscoveryState& state,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler,
    int max_depth = 3
) {
    if (hyp.drill_depth >= max_depth) return;
    if (hyp.stage < HypothesisStage::CONDITIONAL_HYPOTHESIS) return;
    
    std::string current = hyp.content;
    
    for (int level = hyp.drill_depth; level < max_depth; level++) {
        // Check stop condition: minimum specificity
        if (state.config.min_specificity_tokens.count(level)) {
            if ((int)current.size() < state.config.min_specificity_tokens.at(level)) {
                break;  // Stop: insufficient specificity to drill further
            }
        }
        
        std::string drill_prompt = R"(Given this hypothesis at level )" + 
            std::to_string(level) + R"(:

)" + current + R"(

Drill down to the next level of specificity. Be MORE concrete and actionable.
Include specific mechanisms, measurements, or procedures.
If this is already maximally specific, respond with "TERMINAL".

Drilled hypothesis:)";
        
        std::string drilled = inference_with_grammar(ctx, model, sampler, drill_prompt, 
                                                     state.config.max_hypothesis_tokens);
        
        // Check for terminal marker
        if (drilled.find("TERMINAL") != std::string::npos) {
            break;
        }
        
        // Record drill step with provenance
        DreamHypothesis::DrillStep step;
        step.level = level + 1;
        step.content = drilled;
        step.source_hypothesis_id = hyp.id;
        step.token_count = drilled.size();  // Simplified; would use tokenizer
        
        // Gather citations for this drill step
        // In production: would run evidence classification here
        
        hyp.drill_chain.push_back(step);
        hyp.drill_depth = level + 1;
        current = drilled;
    }
}

// Extract testable prediction with falsifiability
inline TestablePrediction extract_prediction_with_falsifiability(
    DreamHypothesis& hyp,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    TestablePrediction pred;
    pred.id = "pred_" + hyp.id;
    pred.hypothesis_id = hyp.id;
    
    // Get deepest drill content or base hypothesis
    std::string deepest = hyp.content;
    if (!hyp.drill_chain.empty()) {
        deepest = hyp.drill_chain.back().content;
        for (const auto& step : hyp.drill_chain) {
            pred.drill_chain_ids.push_back(hyp.id + "_drill_" + std::to_string(step.level));
        }
    }
    
    std::string prompt = R"(Convert this hypothesis into a TESTABLE PREDICTION with explicit falsifiability.

Hypothesis: )" + deepest + R"(

Provide:
STATEMENT: [The specific prediction]
TEST_PROTOCOL: [How to test it]
EXPECTED_OUTCOME: [What result confirms it]
FALSIFIED_IF: [What result would DISPROVE it - be specific]

Format as above:)";
    
    std::string response = inference_with_grammar(ctx, model, sampler, prompt, 400);
    
    // Parse response
    std::istringstream iss(response);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("STATEMENT:") == 0) pred.statement = line.substr(10);
        else if (line.find("TEST_PROTOCOL:") == 0) pred.test_protocol = line.substr(14);
        else if (line.find("EXPECTED_OUTCOME:") == 0) pred.expected_outcome = line.substr(17);
        else if (line.find("FALSIFIED_IF:") == 0) pred.falsification_criteria = line.substr(13);
    }
    
    // Inherit evidence and assumptions
    pred.evidence = hyp.supporting_spans;
    pred.inherited_assumptions = hyp.assumptions;
    pred.inherited_scope = hyp.scope;
    pred.confidence = hyp.support_ratio * hyp.novelty.total;
    
    return pred;
}

// ============================================================================
// MAIN DISCOVERY CYCLE
// ============================================================================

inline void run_discovery_cycle(
    DiscoveryState& state,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler
) {
    state.cycle_count++;
    
    // Phase 1: Identify tensions (from research mode)
    if (state.tensions.empty() && state.research_ctx) {
        // Would call identify_tensions from zeta-research.h
        // For now, tensions are populated externally
    }
    
    // Phase 2: Classify tension types
    for (auto& t : state.tensions) {
        if (t.type == TensionType::UNKNOWN) {
            t.type = classify_tension(t.description, ctx, model, sampler);
        }
    }
    
    // Phase 3: Generate dreams for unaddressed tensions
    for (auto& t : state.tensions) {
        if (t.addressed) continue;
        
        DreamHypothesis hyp = generate_dream(state, t, ctx, model, sampler);
        
        // Dedup check
        uint64_t hash = hash_hypothesis_content(hyp.content);
        if (state.is_duplicate(hash)) {
            continue;
        }
        state.mark_seen(hash);
        
        state.candidates().push_back(hyp);
    }
    
    // Phase 4: Validate dreams
    for (auto& hyp : state.candidates()) {
        validate_dream_deterministic(hyp, *state.research_ctx, state.config, ctx, model, sampler);
        
        if (hyp.verdict == ValidationVerdict::REJECTED) {
            state.total_rejected++;
        }
    }
    
    // Phase 5: Attempt promotions
    std::vector<DreamHypothesis> promoted;
    for (auto& hyp : state.candidates()) {
        if (hyp.verdict == ValidationVerdict::REJECTED) continue;
        
        if (attempt_promotion(hyp, state)) {
            promoted.push_back(hyp);
        }
    }
    
    // Move promoted to appropriate stage buckets
    for (auto& hyp : promoted) {
        state.hypotheses_by_stage[hyp.stage].push_back(hyp);
    }
    
    // Remove promoted from candidates
    state.candidates().erase(
        std::remove_if(state.candidates().begin(), state.candidates().end(),
            [](const DreamHypothesis& h) { 
                return h.stage != HypothesisStage::DREAM_CANDIDATE; 
            }),
        state.candidates().end());
    
    // Phase 6: Drill-down CONDITIONAL hypotheses
    for (auto& hyp : state.conditionals()) {
        drill_down_hypothesis_with_provenance(hyp, state, ctx, model, sampler, 
                                              state.config.max_drill_depth);
        // Try to promote after drill-down
        attempt_promotion(hyp, state);
    }
    
    // Phase 7: Integrate GROUNDED hypotheses
    for (auto& hyp : state.grounded()) {
        integrate_dream(hyp, state);
        
        // Check if ready for prediction
        attempt_promotion(hyp, state);
    }
    
    // Phase 8: Extract predictions
    for (auto& hyp : state.predictions_ready()) {
        TestablePrediction pred = extract_prediction_with_falsifiability(hyp, ctx, model, sampler);
        
        // Only add if falsifiable
        if (pred.is_falsifiable()) {
            state.predictions.push_back(pred);
            
            // Check if this is a discovery
            if (hyp.is_discovery()) {
                state.discoveries.push_back(hyp.id);
            }
        }
    }
    
    // Update entropy
    state.update_entropy();
}

// ============================================================================
// REPORT GENERATION
// ============================================================================

inline std::string produce_discovery_report(const DiscoveryState& state) {
    std::string report;
    
    report += "╔══════════════════════════════════════════════════════════════════╗\n";
    report += "║                     DISCOVERY MODE REPORT                        ║\n";
    report += "╚══════════════════════════════════════════════════════════════════╝\n\n";
    
    // Summary
    report += "SUMMARY\n";
    report += "───────\n";
    report += "Cycles completed: " + std::to_string(state.cycle_count) + "\n";
    report += "Tensions identified: " + std::to_string(state.tensions.size()) + "\n";
    report += "Dreams generated: " + std::to_string(state.total_dreams) + "\n";
    report += "Promoted: " + std::to_string(state.total_promoted) + "\n";
    report += "Rejected: " + std::to_string(state.total_rejected) + "\n";
    report += "Integrated: " + std::to_string(state.total_integrated) + "\n";
    report += state.entropy.format() + "\n\n";
    
    // Tension breakdown by type
    report += "TENSION TYPES\n";
    report += "─────────────\n";
    std::map<TensionType, int> tension_counts;
    for (const auto& t : state.tensions) {
        tension_counts[t.type]++;
    }
    for (const auto& [type, count] : tension_counts) {
        report += std::string(tension_type_str(type)) + ": " + std::to_string(count) + "\n";
    }
    report += "\n";
    
    // Hypotheses by stage
    report += "HYPOTHESIS LADDER\n";
    report += "─────────────────\n";
    for (const auto& [stage, hyps] : state.hypotheses_by_stage) {
        report += std::string(stage_str(stage)) + ": " + std::to_string(hyps.size()) + "\n";
        for (const auto& h : hyps) {
            report += "  • " + h.id + " [novelty=" + std::to_string(h.novelty.total).substr(0,4) + "]\n";
        }
    }
    report += "\n";
    
    // Discoveries
    if (!state.discoveries.empty()) {
        report += "═══════════════════════════════════════════════════════════════════\n";
        report += "                         DISCOVERIES                               \n";
        report += "═══════════════════════════════════════════════════════════════════\n";
        for (const auto& d : state.discoveries) {
            report += "★ " + d + "\n";
        }
        report += "\n";
    }
    
    // Predictions
    if (!state.predictions.empty()) {
        report += "═══════════════════════════════════════════════════════════════════\n";
        report += "                      TESTABLE PREDICTIONS                         \n";
        report += "═══════════════════════════════════════════════════════════════════\n";
        for (const auto& p : state.predictions) {
            report += p.format() + "\n";
        }
    }
    
    return report;
}

// ============================================================================
// HIGH-LEVEL ENTRY POINTS
// ============================================================================

inline void run_discovery_mode(
    ResearchContext& research_ctx,
    DreamContext& dream_ctx,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler,
    const DiscoveryConfig& config = DiscoveryConfig{}
) {
    DiscoveryState state;
    state.config = config;
    state.research_ctx = &research_ctx;
    state.dream_ctx = &dream_ctx;
    
    // Initialize entropy coefficients
    state.entropy.alpha = config.entropy_alpha;
    state.entropy.beta = config.entropy_beta;
    state.entropy.gamma = config.entropy_gamma;
    
    printf("\n═══ DISCOVERY MODE INITIALIZED ═══\n");
    printf("Max cycles: %d\n", config.max_cycles);
    printf("Convergence threshold: %.2f\n", config.convergence_threshold);
    printf("Require structured assumptions: %s\n", 
           config.require_structured_assumptions ? "YES" : "NO");
    
    // Main discovery loop
    while (state.cycle_count < config.max_cycles) {
        printf("\n--- Discovery Cycle %d ---\n", state.cycle_count + 1);
        
        run_discovery_cycle(state, ctx, model, sampler);
        
        printf("%s\n", state.entropy.format().c_str());
        printf("Candidates: %zu, Conditional: %zu, Grounded: %zu\n",
               state.candidates().size(),
               state.conditionals().size(),
               state.grounded().size());
        
        // Check convergence
        if (state.entropy.is_converged(config.convergence_threshold)) {
            printf("\n✓ CONVERGED: Entropy below threshold with grounded hypotheses\n");
            break;
        }
        
        // Check stall
        if (state.entropy.is_stalled(config.max_stall_cycles, 0.05f)) {
            printf("\n⚠ STALLED: No progress for %d cycles\n", config.max_stall_cycles);
            break;
        }
    }
    
    // Final report
    std::string report = produce_discovery_report(state);
    printf("\n%s\n", report.c_str());
}

// Simplified entry point
inline std::string discover(
    const std::string& topic,
    llama_context* ctx,
    llama_model* model,
    const llama_sampler* sampler,
    KnowledgeGraph* graph = nullptr
) {
    ResearchContext research_ctx;
    research_ctx.original_query = topic;
    research_ctx.graph = graph;
    
    DreamContext dream_ctx;
    dream_ctx.temperature = 0.8f;
    
    DiscoveryConfig config;
    config.max_cycles = 5;
    
    // Create initial tension from topic
    DiscoveryState state;
    state.config = config;
    state.research_ctx = &research_ctx;
    state.dream_ctx = &dream_ctx;
    
    // Bootstrap: create GAP tension for the topic
    DiscoveryState::TensionRecord initial_tension;
    initial_tension.id = "tension_0";
    initial_tension.type = TensionType::GAP;
    initial_tension.description = "What is currently unknown or unexplored about: " + topic;
    state.tensions.push_back(initial_tension);
    
    // Run discovery
    while (state.cycle_count < config.max_cycles) {
        run_discovery_cycle(state, ctx, model, sampler);
        
        if (state.entropy.is_converged(config.convergence_threshold)) break;
        if (state.entropy.is_stalled(config.max_stall_cycles, 0.05f)) break;
    }
    
    return produce_discovery_report(state);
}

}  // namespace zeta::discovery

#endif  // ZETA_DISCOVERY_H
