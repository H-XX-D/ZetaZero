// Z.E.T.A. Mode Controller v2 - Unified Branching Across All Modes
// ============================================================================
// Branching is a first-class engine, not a RESEARCH-only feature.
// ModePolicy controls WHEN and HOW branching occurs per mode.
//
// KEY INVARIANT: Branches are PROVISIONAL. Only validated merges can
// become durable knowledge. This prevents graph contamination.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_MODE_CONTROLLER_H
#define ZETA_MODE_CONTROLLER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>
#include <fstream>
#include "zeta-branching-engine.h"

namespace zeta_modes {

using namespace zeta_branching;

// ============================================================================
// MODE DEFINITIONS
// ============================================================================

enum class Mode {
    CHAT,       // Default conversational - multi-hypothesis for better answers
    CREATIVE,   // Fiction/roleplay - divergence is the product
    RESEARCH,   // Deep exploration - disciplined branching with validation
    CODE,       // Programming - explore implementations, select best
    DREAM       // Idle consolidation - associative exploration
};

inline const char* mode_name(Mode m) {
    switch (m) {
        case Mode::CHAT: return "CHAT";
        case Mode::CREATIVE: return "CREATIVE";
        case Mode::RESEARCH: return "RESEARCH";
        case Mode::CODE: return "CODE";
        case Mode::DREAM: return "DREAM";
    }
    return "UNKNOWN";
}

// ============================================================================
// KV INJECTION POLICY
// ============================================================================

enum class KVInjectionPolicy {
    NONE,           // No KV injection
    VALIDATED_ONLY, // Only inject from validated sources
    FULL            // Inject all available context
};

// ============================================================================
// COMMIT POLICY
// What can hit GitGraph/GraphKV
// ============================================================================

enum class CommitPolicy {
    NEVER,              // Nothing commits (CREATIVE, DREAM default)
    FACTS_VALIDATED,    // Only validated facts (CHAT)
    SUMMARIES_VALIDATED,// Only validated summaries (RESEARCH)
    SELECTED_SOLUTION,  // Chosen solution + rationale (CODE)
    LUCID_GATED         // Only if passes lucid review (DREAM special)
};

// ============================================================================
// MODE POLICY
// Complete behavioral specification per mode
// ============================================================================

struct ModePolicy {
    // === SAMPLING ===
    float temperature;
    float penalty_repeat;
    int penalty_last_n;
    
    // === BRANCHING (NEW: first-class configuration) ===
    BranchingLevel branching_level;
    BranchBudget branch_budget;
    BranchTrigger branch_triggers;
    MergePolicy merge_policy;
    CommitEligibility default_branch_eligibility;
    
    // Permitted branch kinds (bit flags would be cleaner but this is explicit)
    std::unordered_set<BranchKind> permitted_branch_kinds;
    
    // === GRAPH POLICIES ===
    CommitPolicy commit_policy;
    KVInjectionPolicy kv_policy;
    bool persist_to_gitgraph;
    
    // === CONTEXT POLICIES ===
    bool use_trm_context;
    bool use_hrm_decomposition;
    
    // === VALIDATORS (which must pass for commits) ===
    ValidatorProfile validators;
};

// ============================================================================
// DEFAULT POLICIES PER MODE
// ============================================================================

inline ModePolicy get_policy(Mode m) {
    ModePolicy p;
    
    switch (m) {
        // ====================================================================
        // CHAT: Better answers via multi-hypothesis reasoning
        // Branching: LIGHT (ambiguity only), commit facts validated only
        // ====================================================================
        case Mode::CHAT:
            p.temperature = 0.6f;
            p.penalty_repeat = 1.15f;
            p.penalty_last_n = 256;
            
            p.branching_level = BranchingLevel::LIGHT;
            p.branch_budget = {
                .max_branches = 4,
                .max_depth = 2,
                .max_tension_checks = 6,
                .max_iterations = 10,
                .max_concepts_per_branch = 30,
                .max_branch_time_ms = 3000,
                .max_total_time_ms = 15000
            };
            p.branch_triggers = {
                .on_ambiguity = true,
                .on_uncertainty = true,
                .on_contradiction = true,
                .on_alternative_frame = false,
                .on_design_tradeoff = false,
                .on_implementation_choice = false,
                .on_api_constraint = false,
                .on_stylistic_choice = false,
                .on_plot_branch = false,
                .on_character_motivation = false,
                .on_novelty = false,
                .on_motif_recombination = false,
                .entropy_threshold = 0.6f,
                .confidence_threshold = 0.5f
            };
            p.merge_policy = {
                .strategy = MergeStrategy::SYNTHESIS,
                .merge_on_convergence = true,
                .merge_on_resolution = true,
                .merge_on_budget_hit = true,
                .merge_on_confidence = true,
                .validators = {},
                .min_branches_for_merge = 2,
                .auto_merge_confidence = 0.8f
            };
            p.default_branch_eligibility = CommitEligibility::VALIDATED_ONLY;
            p.permitted_branch_kinds = {
                BranchKind::AMBIGUITY,
                BranchKind::UNCERTAINTY,
                BranchKind::CONTRADICTION
            };
            
            p.commit_policy = CommitPolicy::FACTS_VALIDATED;
            p.kv_policy = KVInjectionPolicy::FULL;
            p.persist_to_gitgraph = true;
            p.use_trm_context = true;
            p.use_hrm_decomposition = true;
            
            p.validators = {
                .require_facts_validator = true,
                .require_causality_check = true,
                .require_uncertainty_preserved = true,
                .require_compile_check = false,
                .require_interface_check = false,
                .require_test_check = false,
                .require_style_check = false,
                .require_lucid_gate = false,
                .min_confidence = 0.7f
            };
            break;
            
        // ====================================================================
        // CREATIVE: Divergence is the product
        // Branching: FULL (creative variation), no commits
        // ====================================================================
        case Mode::CREATIVE:
            p.temperature = 0.85f;
            p.penalty_repeat = 1.2f;
            p.penalty_last_n = 512;
            
            p.branching_level = BranchingLevel::FULL;
            p.branch_budget = {
                .max_branches = 12,
                .max_depth = 4,
                .max_tension_checks = 4,  // Less focus on tensions
                .max_iterations = 15,
                .max_concepts_per_branch = 100,
                .max_branch_time_ms = 5000,
                .max_total_time_ms = 60000
            };
            p.branch_triggers = {
                .on_ambiguity = false,
                .on_uncertainty = false,
                .on_contradiction = false,
                .on_alternative_frame = true,
                .on_design_tradeoff = false,
                .on_implementation_choice = false,
                .on_api_constraint = false,
                .on_stylistic_choice = true,
                .on_plot_branch = true,
                .on_character_motivation = true,
                .on_novelty = true,
                .on_motif_recombination = false,
                .entropy_threshold = 0.3f,  // Branch even at low entropy
                .confidence_threshold = 0.9f  // Don't branch on confidence
            };
            p.merge_policy = {
                .strategy = MergeStrategy::CURATED,  // Present options, don't force synthesis
                .merge_on_convergence = false,
                .merge_on_resolution = false,
                .merge_on_budget_hit = true,
                .merge_on_confidence = false,
                .validators = {},
                .min_branches_for_merge = 3,
                .auto_merge_confidence = 0.95f  // Almost never auto-merge
            };
            p.default_branch_eligibility = CommitEligibility::NEVER;
            p.permitted_branch_kinds = {
                BranchKind::CREATIVE_VARIATION,
                BranchKind::FRAME
            };
            
            p.commit_policy = CommitPolicy::NEVER;
            p.kv_policy = KVInjectionPolicy::NONE;  // Don't drag facts into fiction
            p.persist_to_gitgraph = false;
            p.use_trm_context = false;
            p.use_hrm_decomposition = false;
            
            p.validators = {
                .require_facts_validator = false,
                .require_causality_check = false,
                .require_uncertainty_preserved = false,
                .require_compile_check = false,
                .require_interface_check = false,
                .require_test_check = false,
                .require_style_check = true,  // Only style matters
                .require_lucid_gate = false,
                .min_confidence = 0.0f  // No confidence requirement
            };
            break;
            
        // ====================================================================
        // RESEARCH: Highest rigor mode with disciplined branching
        // Branching: FULL (frames), commit validated only
        // ====================================================================
        case Mode::RESEARCH:
            p.temperature = 0.7f;
            p.penalty_repeat = 1.1f;
            p.penalty_last_n = 256;
            
            p.branching_level = BranchingLevel::FULL;
            p.branch_budget = {
                .max_branches = 8,
                .max_depth = 5,
                .max_tension_checks = 20,
                .max_iterations = 30,
                .max_concepts_per_branch = 50,
                .max_branch_time_ms = 10000,
                .max_total_time_ms = 120000  // 2 minutes for deep research
            };
            p.branch_triggers = {
                .on_ambiguity = true,
                .on_uncertainty = true,
                .on_contradiction = true,
                .on_alternative_frame = true,
                .on_design_tradeoff = true,
                .on_implementation_choice = false,
                .on_api_constraint = false,
                .on_stylistic_choice = false,
                .on_plot_branch = false,
                .on_character_motivation = false,
                .on_novelty = false,
                .on_motif_recombination = false,
                .entropy_threshold = 0.5f,
                .confidence_threshold = 0.6f
            };
            p.merge_policy = {
                .strategy = MergeStrategy::SYNTHESIS,
                .merge_on_convergence = true,
                .merge_on_resolution = true,
                .merge_on_budget_hit = true,
                .merge_on_confidence = true,
                .validators = {
                    .require_facts_validator = true,
                    .require_causality_check = true,
                    .require_uncertainty_preserved = true,
                    .min_confidence = 0.75f
                },
                .min_branches_for_merge = 2,
                .auto_merge_confidence = 0.85f
            };
            p.default_branch_eligibility = CommitEligibility::VALIDATED_ONLY;
            p.permitted_branch_kinds = {
                BranchKind::AMBIGUITY,
                BranchKind::FRAME,
                BranchKind::UNCERTAINTY,
                BranchKind::CONTRADICTION,
                BranchKind::TRADE_OFF
            };
            
            p.commit_policy = CommitPolicy::SUMMARIES_VALIDATED;
            p.kv_policy = KVInjectionPolicy::VALIDATED_ONLY;
            p.persist_to_gitgraph = true;
            p.use_trm_context = true;
            p.use_hrm_decomposition = false;  // Research has its own decomposition
            
            p.validators = {
                .require_facts_validator = true,
                .require_causality_check = true,
                .require_uncertainty_preserved = true,
                .require_compile_check = false,
                .require_interface_check = false,
                .require_test_check = false,
                .require_style_check = false,
                .require_lucid_gate = false,
                .min_confidence = 0.75f
            };
            break;
            
        // ====================================================================
        // CODE: Explore implementations, select best
        // Branching: LIGHT to FULL (implementation), commit chosen solution
        // ====================================================================
        case Mode::CODE:
            p.temperature = 0.2f;
            p.penalty_repeat = 1.05f;
            p.penalty_last_n = 128;
            
            p.branching_level = BranchingLevel::LIGHT;  // User can override to FULL
            p.branch_budget = {
                .max_branches = 6,
                .max_depth = 3,
                .max_tension_checks = 8,
                .max_iterations = 12,
                .max_concepts_per_branch = 40,
                .max_branch_time_ms = 8000,
                .max_total_time_ms = 45000
            };
            p.branch_triggers = {
                .on_ambiguity = false,
                .on_uncertainty = false,
                .on_contradiction = false,
                .on_alternative_frame = false,
                .on_design_tradeoff = true,
                .on_implementation_choice = true,
                .on_api_constraint = true,
                .on_stylistic_choice = false,
                .on_plot_branch = false,
                .on_character_motivation = false,
                .on_novelty = false,
                .on_motif_recombination = false,
                .entropy_threshold = 0.7f,
                .confidence_threshold = 0.4f
            };
            p.merge_policy = {
                .strategy = MergeStrategy::SELECTION,  // Pick best, not synthesize
                .merge_on_convergence = true,
                .merge_on_resolution = true,
                .merge_on_budget_hit = true,
                .merge_on_confidence = true,
                .validators = {
                    .require_compile_check = true,
                    .require_interface_check = true,
                    .min_confidence = 0.8f
                },
                .min_branches_for_merge = 2,
                .auto_merge_confidence = 0.9f
            };
            p.default_branch_eligibility = CommitEligibility::SELECTED_ONLY;
            p.permitted_branch_kinds = {
                BranchKind::IMPLEMENTATION,
                BranchKind::TRADE_OFF
            };
            
            p.commit_policy = CommitPolicy::SELECTED_SOLUTION;
            p.kv_policy = KVInjectionPolicy::FULL;
            p.persist_to_gitgraph = true;
            p.use_trm_context = true;
            p.use_hrm_decomposition = false;
            
            p.validators = {
                .require_facts_validator = false,
                .require_causality_check = false,
                .require_uncertainty_preserved = false,
                .require_compile_check = true,
                .require_interface_check = true,
                .require_test_check = true,
                .require_style_check = false,
                .require_lucid_gate = false,
                .min_confidence = 0.85f
            };
            break;
            
        // ====================================================================
        // DREAM: Associative exploration with strong guardrails
        // Branching: FULL but gated, lucid-gated persistence only
        // ====================================================================
        case Mode::DREAM:
            p.temperature = 0.9f;
            p.penalty_repeat = 1.0f;
            p.penalty_last_n = 64;
            
            p.branching_level = BranchingLevel::FULL;
            p.branch_budget = {
                .max_branches = 10,
                .max_depth = 6,
                .max_tension_checks = 3,  // Preserve contradictions as creative tension
                .max_iterations = 20,
                .max_concepts_per_branch = 80,
                .max_branch_time_ms = 5000,
                .max_total_time_ms = 90000
            };
            p.branch_triggers = {
                .on_ambiguity = false,
                .on_uncertainty = false,
                .on_contradiction = false,  // Don't resolve in dream state
                .on_alternative_frame = true,
                .on_design_tradeoff = false,
                .on_implementation_choice = false,
                .on_api_constraint = false,
                .on_stylistic_choice = true,
                .on_plot_branch = true,
                .on_character_motivation = true,
                .on_novelty = true,
                .on_motif_recombination = true,
                .entropy_threshold = 0.2f,  // Branch freely
                .confidence_threshold = 0.95f  // Never on confidence
            };
            p.merge_policy = {
                .strategy = MergeStrategy::CURATED,  // Keep options, don't force
                .merge_on_convergence = false,
                .merge_on_resolution = false,
                .merge_on_budget_hit = true,
                .merge_on_confidence = false,
                .validators = {
                    .require_lucid_gate = true,  // KEY: Lucid gate for any persistence
                    .min_confidence = 0.0f
                },
                .min_branches_for_merge = 4,
                .auto_merge_confidence = 1.0f  // Never auto-merge
            };
            p.default_branch_eligibility = CommitEligibility::NEVER;
            p.permitted_branch_kinds = {
                BranchKind::ASSOCIATION,
                BranchKind::CREATIVE_VARIATION,
                BranchKind::FRAME
            };
            
            p.commit_policy = CommitPolicy::LUCID_GATED;
            p.kv_policy = KVInjectionPolicy::NONE;  // Dream from scratch
            p.persist_to_gitgraph = true;  // Persist dreams for review
            p.use_trm_context = true;  // Replay memories
            p.use_hrm_decomposition = false;
            
            p.validators = {
                .require_facts_validator = false,
                .require_causality_check = false,
                .require_uncertainty_preserved = false,
                .require_compile_check = false,
                .require_interface_check = false,
                .require_test_check = false,
                .require_style_check = false,
                .require_lucid_gate = true,
                .min_confidence = 0.0f
            };
            break;
    }
    
    return p;
}

// ============================================================================
// BRANCHING SESSION
// Manages a branching exploration session across requests
// ============================================================================

struct BranchingSession {
    std::string session_id;
    Mode mode;
    BranchingEngine engine;
    
    std::chrono::steady_clock::time_point started;
    std::chrono::steady_clock::time_point last_activity;
    int iteration_count = 0;
    bool active = false;
    
    // GitGraph persistence
    std::string gitgraph_branch;
    std::string session_file;
};

// ============================================================================
// RUNTIME OVERRIDES
// For live tuning without recompile
// ============================================================================

struct RuntimeOverrides {
    bool has_branching_level = false;
    BranchingLevel branching_level;
    
    bool has_budget = false;
    BranchBudget budget;
    
    // Per-field overrides
    bool override_max_branches = false;
    int max_branches;
    
    bool override_max_depth = false;
    int max_depth;
    
    bool override_max_iterations = false;
    int max_iterations;
};

// ============================================================================
// MODE CONTROLLER
// Central coordinator for mode transitions and branching
// ============================================================================

class ModeController {
private:
    Mode current_mode_ = Mode::CHAT;
    ModePolicy current_policy_;
    RuntimeOverrides overrides_;
    std::mutex mode_mutex_;
    
    // Branching session
    BranchingSession session_;
    
    // Configuration
    std::string gitgraph_root_ = "/mnt/GitGraph";
    
    // Generation callback
    std::function<std::string(const std::string&, float temp, int max_tokens)> generate_fn_;
    
    // Validation callback
    std::function<bool(const std::string&, const std::string&)> validate_fn_;
    
    // Mode transition hooks
    std::function<void(Mode, Mode)> on_mode_change_;

public:
    ModeController() : current_policy_(get_policy(Mode::CHAT)) {}
    
    // ========================================================================
    // MODE QUERIES
    // ========================================================================
    
    Mode current_mode() const { return current_mode_; }
    const ModePolicy& policy() const { return current_policy_; }
    
    bool is_branching_active() const {
        return session_.active && 
               current_policy_.branching_level != BranchingLevel::NONE;
    }
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void set_generate_fn(std::function<std::string(const std::string&, float, int)> fn) {
        generate_fn_ = fn;
    }
    
    void set_validate_fn(std::function<bool(const std::string&, const std::string&)> fn) {
        validate_fn_ = fn;
    }
    
    void set_gitgraph_root(const std::string& root) {
        gitgraph_root_ = root;
    }
    
    void set_mode_change_callback(std::function<void(Mode, Mode)> cb) {
        on_mode_change_ = cb;
    }
    
    // ========================================================================
    // RUNTIME OVERRIDES (for live tuning)
    // ========================================================================
    
    void set_branching_level_override(BranchingLevel level) {
        overrides_.has_branching_level = true;
        overrides_.branching_level = level;
        apply_overrides();
    }
    
    void set_budget_override(int max_branches, int max_depth, int max_iterations) {
        if (max_branches > 0) {
            overrides_.override_max_branches = true;
            overrides_.max_branches = max_branches;
        }
        if (max_depth > 0) {
            overrides_.override_max_depth = true;
            overrides_.max_depth = max_depth;
        }
        if (max_iterations > 0) {
            overrides_.override_max_iterations = true;
            overrides_.max_iterations = max_iterations;
        }
        apply_overrides();
    }
    
    void clear_overrides() {
        overrides_ = RuntimeOverrides();
        current_policy_ = get_policy(current_mode_);
    }
    
private:
    void apply_overrides() {
        if (overrides_.has_branching_level) {
            current_policy_.branching_level = overrides_.branching_level;
        }
        if (overrides_.override_max_branches) {
            current_policy_.branch_budget.max_branches = overrides_.max_branches;
        }
        if (overrides_.override_max_depth) {
            current_policy_.branch_budget.max_depth = overrides_.max_depth;
        }
        if (overrides_.override_max_iterations) {
            current_policy_.branch_budget.max_iterations = overrides_.max_iterations;
        }
        
        // Reconfigure engine if active
        if (session_.active) {
            configure_engine();
        }
    }
    
public:
    // ========================================================================
    // MODE DETECTION
    // ========================================================================
    
    Mode detect_mode(const std::string& query, const std::string& query_class) {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Explicit mode commands (highest priority)
        if (lower.find("/research") != std::string::npos ||
            lower.find("enter research mode") != std::string::npos ||
            lower.find("let's research") != std::string::npos ||
            lower.find("deep dive into") != std::string::npos) {
            return Mode::RESEARCH;
        }
        
        if (lower.find("/creative") != std::string::npos ||
            lower.find("/story") != std::string::npos ||
            lower.find("enter creative mode") != std::string::npos) {
            return Mode::CREATIVE;
        }
        
        if (lower.find("/code") != std::string::npos ||
            lower.find("enter code mode") != std::string::npos) {
            return Mode::CODE;
        }
        
        if (lower.find("/chat") != std::string::npos ||
            lower.find("exit research") != std::string::npos ||
            lower.find("back to chat") != std::string::npos) {
            return Mode::CHAT;
        }
        
        // Use query_class from router
        if (query_class == "CREATIVE") return Mode::CREATIVE;
        if (query_class == "CODE" || query_class == "MATH") return Mode::CODE;
        if (query_class == "RESEARCH") return Mode::RESEARCH;
        
        return current_mode_;
    }
    
    // ========================================================================
    // MODE TRANSITIONS
    // ========================================================================
    
    bool switch_mode(Mode new_mode, const std::string& context = "") {
        std::lock_guard<std::mutex> lock(mode_mutex_);
        
        if (new_mode == current_mode_) {
            return true;
        }
        
        Mode old_mode = current_mode_;
        
        // Exit current mode
        exit_mode(old_mode);
        
        // Enter new mode
        enter_mode(new_mode, context);
        
        current_mode_ = new_mode;
        current_policy_ = get_policy(new_mode);
        apply_overrides();  // Re-apply any runtime overrides
        
        if (on_mode_change_) {
            on_mode_change_(old_mode, new_mode);
        }
        
        fprintf(stderr, "[MODE] Switched: %s -> %s (branching=%s)\n", 
                mode_name(old_mode), mode_name(new_mode),
                current_policy_.branching_level == BranchingLevel::NONE ? "NONE" :
                current_policy_.branching_level == BranchingLevel::LIGHT ? "LIGHT" : "FULL");
        
        return true;
    }
    
private:
    void exit_mode(Mode mode) {
        if (session_.active) {
            save_session();
            session_.active = false;
        }
    }
    
    void enter_mode(Mode mode, const std::string& context) {
        if (current_policy_.branching_level != BranchingLevel::NONE || !context.empty()) {
            init_session(context);
        }
    }
    
    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================
    
    void init_session(const std::string& topic) {
        session_.session_id = generate_session_id();
        session_.mode = current_mode_;
        session_.started = std::chrono::steady_clock::now();
        session_.last_activity = session_.started;
        session_.iteration_count = 0;
        session_.active = true;
        
        configure_engine();
        
        // Seed with topic if provided
        if (!topic.empty()) {
            BranchKind initial_kind = BranchKind::UNCERTAINTY;
            if (current_mode_ == Mode::CREATIVE) initial_kind = BranchKind::CREATIVE_VARIATION;
            else if (current_mode_ == Mode::CODE) initial_kind = BranchKind::IMPLEMENTATION;
            
            session_.engine.seed(topic, initial_kind);
        }
        
        // Create GitGraph branch
        if (current_policy_.persist_to_gitgraph) {
            session_.gitgraph_branch = create_gitgraph_branch(topic);
            session_.session_file = gitgraph_root_ + "/sessions/" + 
                                   session_.session_id + ".json";
        }
        
        fprintf(stderr, "[SESSION] Started: %s (mode=%s)\n", 
                session_.session_id.c_str(), mode_name(current_mode_));
    }
    
    void configure_engine() {
        session_.engine.configure(
            current_policy_.branching_level,
            current_policy_.branch_budget,
            current_policy_.branch_triggers,
            current_policy_.merge_policy,
            current_policy_.default_branch_eligibility
        );
        
        if (generate_fn_) {
            session_.engine.set_generate_fn(generate_fn_);
        }
        if (validate_fn_) {
            session_.engine.set_validate_fn(validate_fn_);
        }
    }
    
    std::string generate_session_id() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&time));
        return std::string(mode_name(current_mode_)) + "_" + buf;
    }
    
    std::string create_gitgraph_branch(const std::string& topic) {
        std::string branch = std::string(mode_name(current_mode_)) + "/";
        for (char c : topic) {
            if (isalnum(c)) branch += tolower(c);
            else if (c == ' ') branch += '_';
        }
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        strftime(buf, sizeof(buf), "_%Y%m%d", localtime(&time));
        branch += buf;
        
        return branch;
    }
    
    void save_session() {
        if (!session_.active || !current_policy_.persist_to_gitgraph) return;
        
        std::string json = serialize_session();
        
        std::ofstream f(session_.session_file);
        if (f.is_open()) {
            f << json;
            f.close();
            fprintf(stderr, "[SESSION] Saved: %s\n", session_.session_file.c_str());
        }
    }
    
    std::string serialize_session() {
        std::string json = "{\n";
        json += "  \"session_id\": \"" + session_.session_id + "\",\n";
        json += "  \"mode\": \"" + std::string(mode_name(session_.mode)) + "\",\n";
        json += "  \"iteration_count\": " + std::to_string(session_.iteration_count) + ",\n";
        json += "  \"branch_count\": " + std::to_string(session_.engine.branch_count()) + ",\n";
        json += "  \"tension_count\": " + std::to_string(session_.engine.tension_count()) + ",\n";
        json += "  \"global_entropy\": " + std::to_string(session_.engine.global_entropy()) + "\n";
        json += "}\n";
        return json;
    }
    
public:
    // ========================================================================
    // BRANCHING API
    // ========================================================================
    
    // Seed a new exploration
    void seed_exploration(const std::string& content, BranchKind kind = BranchKind::UNCERTAINTY) {
        if (!session_.active) {
            init_session(content);
        }
        session_.engine.seed(content, kind);
    }
    
    // Run one iteration of exploration
    std::string iterate() {
        if (!session_.active) {
            return "Error: No active session";
        }
        
        session_.last_activity = std::chrono::steady_clock::now();
        session_.iteration_count++;
        
        // Run engine phases
        session_.engine.explore();
        session_.engine.detect_tensions();
        session_.engine.compress();
        session_.engine.merge();
        
        // Build status
        std::string status = "**Iteration " + std::to_string(session_.iteration_count) + "**\n";
        status += "Mode: " + std::string(mode_name(current_mode_)) + "\n";
        status += "Branches: " + std::to_string(session_.engine.branch_count()) + "\n";
        status += "Entropy: " + std::to_string(session_.engine.global_entropy()) + "\n";
        status += "Tensions: " + std::to_string(session_.engine.tension_count()) + "\n";
        
        return status;
    }
    
    // Produce final output
    std::string conclude() {
        if (!session_.active) {
            return "Error: No active session";
        }
        
        std::string output = session_.engine.produce_output();
        
        // Commit if policy allows
        if (should_commit()) {
            commit_results();
        }
        
        save_session();
        
        return output;
    }
    
    // Get current status
    std::string status() {
        if (!session_.active) {
            return "{\"active\": false, \"mode\": \"" + 
                   std::string(mode_name(current_mode_)) + "\"}";
        }
        return serialize_session();
    }
    
    // Access engine directly (for advanced use)
    BranchingEngine& engine() { return session_.engine; }
    const BranchingEngine& engine() const { return session_.engine; }
    
    // ========================================================================
    // COMMIT CONTROL
    // ========================================================================
    
    bool should_commit() const {
        switch (current_policy_.commit_policy) {
            case CommitPolicy::NEVER:
                return false;
            case CommitPolicy::FACTS_VALIDATED:
            case CommitPolicy::SUMMARIES_VALIDATED:
            case CommitPolicy::SELECTED_SOLUTION:
                return true;  // Let commit_results() filter
            case CommitPolicy::LUCID_GATED:
                return false;  // Requires explicit lucid gate
        }
        return false;
    }
    
    bool should_inject_kv() const {
        return current_policy_.kv_policy != KVInjectionPolicy::NONE;
    }
    
    bool should_persist() const {
        return current_policy_.persist_to_gitgraph;
    }
    
private:
    void commit_results() {
        auto committable = session_.engine.get_committable_branches();
        
        for (const auto* branch : committable) {
            // Apply validators
            if (!validate_for_commit(*branch)) continue;
            
            // Commit based on policy
            fprintf(stderr, "[COMMIT] Branch %s -> graph\n", branch->id.c_str());
            // TODO: Actual GitGraph commit
        }
    }
    
    bool validate_for_commit(const Branch& branch) const {
        const auto& v = current_policy_.validators;
        
        // Check required validators
        // In real implementation, these would call actual validation functions
        if (v.require_facts_validator && !branch.validated) return false;
        if (v.require_compile_check && !branch.validated) return false;
        if (v.require_lucid_gate && !branch.validated) return false;
        if (branch.confidence < v.min_confidence) return false;
        
        return true;
    }
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static ModeController g_mode_controller;

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define ZETA_MODE_CURRENT() g_mode_controller.current_mode()
#define ZETA_MODE_POLICY() g_mode_controller.policy()
#define ZETA_MODE_SWITCH(m, ctx) g_mode_controller.switch_mode(m, ctx)
#define ZETA_MODE_DETECT(q, qc) g_mode_controller.detect_mode(q, qc)

#define ZETA_SHOULD_COMMIT() g_mode_controller.should_commit()
#define ZETA_SHOULD_INJECT_KV() g_mode_controller.should_inject_kv()
#define ZETA_SHOULD_PERSIST() g_mode_controller.should_persist()

#define ZETA_BRANCHING_ACTIVE() g_mode_controller.is_branching_active()
#define ZETA_BRANCHING_ITERATE() g_mode_controller.iterate()
#define ZETA_BRANCHING_CONCLUDE() g_mode_controller.conclude()
#define ZETA_BRANCHING_STATUS() g_mode_controller.status()

} // namespace zeta_modes

#endif // ZETA_MODE_CONTROLLER_H
