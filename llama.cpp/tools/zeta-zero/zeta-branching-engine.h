// Z.E.T.A. Branching Engine - First-Class Multi-Hypothesis Exploration
// ============================================================================
// Mode-agnostic primitives for fork, adjacency, tension detection, merge,
// and summary validation. ModePolicy controls WHEN and HOW branching occurs.
//
// Design principle: Branches are PROVISIONAL. Only validated merges can
// become durable knowledge. This prevents graph contamination.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_BRANCHING_ENGINE_H
#define ZETA_BRANCHING_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace zeta_branching {

// ============================================================================
// BRANCH KIND: What triggered the fork
// ============================================================================

enum class BranchKind {
    AMBIGUITY,           // Missing details, unclear referents, multiple interpretations
    FRAME,               // Different conceptual framings of the same problem
    IMPLEMENTATION,      // Alternative technical approaches (CODE mode)
    CREATIVE_VARIATION,  // Stylistic/narrative alternatives (CREATIVE mode)
    ASSOCIATION,         // Free association chains (DREAM mode)
    TRADE_OFF,           // Explicit design trade-off (perf vs readability, etc.)
    UNCERTAINTY,         // Epistemic uncertainty requiring exploration
    CONTRADICTION        // Detected conflict requiring resolution
};

inline const char* branch_kind_name(BranchKind k) {
    switch (k) {
        case BranchKind::AMBIGUITY: return "AMBIGUITY";
        case BranchKind::FRAME: return "FRAME";
        case BranchKind::IMPLEMENTATION: return "IMPLEMENTATION";
        case BranchKind::CREATIVE_VARIATION: return "CREATIVE_VARIATION";
        case BranchKind::ASSOCIATION: return "ASSOCIATION";
        case BranchKind::TRADE_OFF: return "TRADE_OFF";
        case BranchKind::UNCERTAINTY: return "UNCERTAINTY";
        case BranchKind::CONTRADICTION: return "CONTRADICTION";
    }
    return "UNKNOWN";
}

// ============================================================================
// COMMIT ELIGIBILITY: What can hit durable storage
// ============================================================================

enum class CommitEligibility {
    NEVER,           // Branch content can NEVER be committed (CREATIVE, DREAM)
    VALIDATED_ONLY,  // Only after passing validators (CHAT facts, RESEARCH summaries)
    SELECTED_ONLY,   // Only the chosen/merged result (CODE solutions)
    ALWAYS           // Rarely used - everything commits (dangerous)
};

// ============================================================================
// BRANCHING LEVEL: How aggressive branching is
// ============================================================================

enum class BranchingLevel {
    NONE,   // No branching - single-path generation
    LIGHT,  // Conservative branching (2-4 branches, low triggers)
    FULL    // Aggressive branching (6-12 branches, many triggers)
};

// ============================================================================
// VALIDATOR PROFILE: Which judges must pass for commit eligibility
// ============================================================================

struct ValidatorProfile {
    bool require_facts_validator = false;      // Factual accuracy check
    bool require_causality_check = false;      // Logical consistency
    bool require_uncertainty_preserved = false; // Doesn't overclaim
    bool require_compile_check = false;        // CODE: compiles
    bool require_interface_check = false;      // CODE: satisfies interface
    bool require_test_check = false;           // CODE: passes tests
    bool require_style_check = false;          // CREATIVE: meets style constraints
    bool require_lucid_gate = false;           // DREAM: passes lucid review
    
    // Minimum confidence threshold
    float min_confidence = 0.7f;
};

// ============================================================================
// BRANCH BUDGET: Hard limits to prevent explosion
// ============================================================================

struct BranchBudget {
    int max_branches = 4;           // Max active branches at any time
    int max_depth = 3;              // Max fork depth (branch of branch of branch)
    int max_tension_checks = 10;    // Max contradiction checks per iteration
    int max_iterations = 20;        // Max exploration iterations
    int max_concepts_per_branch = 50; // Prevent runaway accumulation
    
    // Time limits (ms)
    int64_t max_branch_time_ms = 5000;   // Max time for a single branch expansion
    int64_t max_total_time_ms = 30000;   // Max total branching time
};

// ============================================================================
// BRANCH TRIGGER: What events can cause a fork
// ============================================================================

struct BranchTrigger {
    // Semantic triggers
    bool on_ambiguity = false;       // Unclear referents, multiple interpretations
    bool on_uncertainty = false;     // Low confidence, hedge words
    bool on_contradiction = false;   // Detected conflict
    bool on_alternative_frame = false; // Different conceptual approach
    
    // Code-specific triggers
    bool on_design_tradeoff = false; // Perf vs readability, etc.
    bool on_implementation_choice = false; // Multiple valid approaches
    bool on_api_constraint = false;  // External interface requirements
    
    // Creative triggers
    bool on_stylistic_choice = false; // Tone, voice, narrative style
    bool on_plot_branch = false;     // Story direction options
    bool on_character_motivation = false; // Character psychology
    
    // Dream triggers
    bool on_novelty = false;         // Unexpected associations
    bool on_motif_recombination = false; // Pattern mixing
    
    // Uncertainty thresholds
    float entropy_threshold = 0.5f;  // Fork if branch entropy exceeds this
    float confidence_threshold = 0.6f; // Fork if confidence below this
};

// ============================================================================
// MERGE POLICY: When and how merges occur
// ============================================================================

enum class MergeStrategy {
    SYNTHESIS,       // Combine insights from branches
    SELECTION,       // Pick best branch, discard others
    CURATED,         // Present options to user for selection
    CONDITIONAL      // "If X then Y, else Z" output
};

struct MergePolicy {
    MergeStrategy strategy = MergeStrategy::SYNTHESIS;
    
    // When to merge
    bool merge_on_convergence = true;    // Branches agree
    bool merge_on_resolution = true;     // Tension resolved
    bool merge_on_budget_hit = true;     // Budget exhausted
    bool merge_on_confidence = true;     // High confidence reached
    
    // Validation requirements
    ValidatorProfile validators;
    
    // Minimum branches before merge allowed
    int min_branches_for_merge = 2;
    
    // Confidence threshold for auto-merge
    float auto_merge_confidence = 0.85f;
};

// ============================================================================
// BRANCH: Single exploration path
// ============================================================================

struct Branch {
    std::string id;
    BranchKind kind = BranchKind::UNCERTAINTY;
    CommitEligibility eligibility = CommitEligibility::NEVER;
    
    // Content
    std::vector<std::string> concepts;
    std::string summary;
    
    // Metadata
    float entropy = 1.0f;
    float confidence = 0.5f;
    int depth = 0;
    std::string parent_id;
    std::vector<std::string> child_ids;
    
    // Timestamps
    int64_t created_at = 0;
    int64_t last_updated = 0;
    
    // Validation state
    bool validated = false;
    std::string validation_notes;
    
    bool is_active() const { return !concepts.empty() && entropy > 0.01f; }
    bool can_fork() const { return depth < 10 && concepts.size() < 100; }
};

// ============================================================================
// TENSION: Detected conflict between branches
// ============================================================================

enum class TensionType {
    CONTRADICTION,   // Mutually exclusive claims
    UNCERTAINTY,     // Both uncertain, need more info
    GAP,             // Missing connection
    DEPENDENCY,      // One requires the other
    TRADE_OFF        // Design decision, not error
};

struct Tension {
    std::string id;
    std::string branch_a;
    std::string branch_b;
    TensionType type = TensionType::UNCERTAINTY;
    float severity = 0.5f;
    std::string description;
    
    // Resolution
    bool resolved = false;
    std::string resolution;
    std::string resolution_branch;
    
    // For trade-offs: preserve as explicit choice, not contradiction
    bool is_trade_off() const { return type == TensionType::TRADE_OFF; }
};

// ============================================================================
// BRANCH GRAPH: O(1) adjacency tracking
// ============================================================================

class BranchGraph {
private:
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency_;
    std::unordered_set<std::string> active_ids_;
    std::unordered_set<std::string> tension_ids_;

public:
    void add_branch(const std::string& id) {
        if (id.empty()) return;
        active_ids_.insert(id);
        if (adjacency_.find(id) == adjacency_.end()) {
            adjacency_[id] = {};
        }
    }
    
    void remove_branch(const std::string& id) {
        active_ids_.erase(id);
        // Don't remove from adjacency_ - keep for tension tracking
    }
    
    void link(const std::string& a, const std::string& b) {
        if (a.empty() || b.empty() || a == b) return;
        adjacency_[a].insert(b);
        adjacency_[b].insert(a);  // Symmetric
    }
    
    void unlink(const std::string& a, const std::string& b) {
        adjacency_[a].erase(b);
        adjacency_[b].erase(a);
    }
    
    const std::unordered_set<std::string>& neighbors(const std::string& id) const {
        static const std::unordered_set<std::string> empty;
        auto it = adjacency_.find(id);
        return it != adjacency_.end() ? it->second : empty;
    }
    
    bool is_active(const std::string& id) const {
        return active_ids_.count(id) > 0;
    }
    
    size_t active_count() const { return active_ids_.size(); }
    
    const std::unordered_set<std::string>& active_ids() const { return active_ids_; }
    
    // Tension tracking
    void record_tension(const std::string& id) { tension_ids_.insert(id); }
    bool has_tension(const std::string& id) const { return tension_ids_.count(id) > 0; }
    void clear_tension(const std::string& id) { tension_ids_.erase(id); }
    
    std::string canonical_pair_key(const std::string& a, const std::string& b) const {
        return a < b ? (a + ":" + b) : (b + ":" + a);
    }
};

// ============================================================================
// BRANCHING ENGINE: Core multi-hypothesis exploration
// ============================================================================

class BranchingEngine {
private:
    // State
    std::vector<Branch> branches_;
    std::vector<Tension> tensions_;
    BranchGraph graph_;
    
    // Configuration (set by ModePolicy)
    BranchingLevel level_ = BranchingLevel::NONE;
    BranchBudget budget_;
    BranchTrigger triggers_;
    MergePolicy merge_policy_;
    CommitEligibility default_eligibility_ = CommitEligibility::NEVER;
    
    // Callbacks
    std::function<std::string(const std::string&, float temp, int max_tokens)> generate_fn_;
    std::function<bool(const std::string&, const std::string&)> validate_fn_;
    
    // Metrics
    int iteration_count_ = 0;
    int64_t start_time_ = 0;
    float global_entropy_ = 1.0f;
    
    // ID generation
    int branch_counter_ = 0;
    int tension_counter_ = 0;

public:
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void configure(BranchingLevel level, 
                   const BranchBudget& budget,
                   const BranchTrigger& triggers,
                   const MergePolicy& merge_policy,
                   CommitEligibility default_eligibility) {
        level_ = level;
        budget_ = budget;
        triggers_ = triggers;
        merge_policy_ = merge_policy;
        default_eligibility_ = default_eligibility;
        
        fprintf(stderr, "[BRANCH-ENGINE] Configured: level=%s, max_branches=%d, max_depth=%d\n",
                level == BranchingLevel::NONE ? "NONE" : 
                level == BranchingLevel::LIGHT ? "LIGHT" : "FULL",
                budget.max_branches, budget.max_depth);
    }
    
    void set_generate_fn(std::function<std::string(const std::string&, float, int)> fn) {
        generate_fn_ = fn;
    }
    
    void set_validate_fn(std::function<bool(const std::string&, const std::string&)> fn) {
        validate_fn_ = fn;
    }
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void reset() {
        branches_.clear();
        tensions_.clear();
        graph_ = BranchGraph();
        iteration_count_ = 0;
        branch_counter_ = 0;
        tension_counter_ = 0;
        global_entropy_ = 1.0f;
        start_time_ = current_time_ms();
    }
    
    void seed(const std::string& initial_content, BranchKind kind = BranchKind::UNCERTAINTY) {
        reset();
        
        if (level_ == BranchingLevel::NONE) {
            // Single-path mode - just store content, no branching
            Branch b;
            b.id = generate_branch_id();
            b.kind = kind;
            b.eligibility = default_eligibility_;
            b.concepts.push_back(initial_content);
            b.entropy = 0.5f;
            b.confidence = 0.5f;
            b.created_at = current_time_ms();
            branches_.push_back(b);
            graph_.add_branch(b.id);
            return;
        }
        
        // Create initial branch
        Branch root;
        root.id = generate_branch_id();
        root.kind = kind;
        root.eligibility = default_eligibility_;
        root.concepts.push_back(initial_content);
        root.entropy = compute_initial_entropy(initial_content);
        root.confidence = 0.5f;
        root.depth = 0;
        root.created_at = current_time_ms();
        
        branches_.push_back(root);
        graph_.add_branch(root.id);
        
        fprintf(stderr, "[BRANCH-ENGINE] Seeded with root branch %s (entropy=%.2f)\n",
                root.id.c_str(), root.entropy);
    }
    
    // ========================================================================
    // CORE OPERATIONS
    // ========================================================================
    
    // Explore: Expand branches based on triggers
    void explore() {
        if (level_ == BranchingLevel::NONE) return;
        if (!check_budget()) return;
        
        iteration_count_++;
        
        // Collect branches that should fork
        std::vector<std::pair<std::string, BranchKind>> fork_candidates;
        
        for (const auto& b : branches_) {
            if (!graph_.is_active(b.id)) continue;
            if (b.depth >= budget_.max_depth) continue;
            
            // Check triggers
            BranchKind trigger_kind;
            if (should_fork(b, trigger_kind)) {
                fork_candidates.emplace_back(b.id, trigger_kind);
            }
        }
        
        // Fork within budget
        int forks_allowed = budget_.max_branches - (int)graph_.active_count();
        for (const auto& [parent_id, kind] : fork_candidates) {
            if (forks_allowed <= 0) break;
            
            int num_forks = (level_ == BranchingLevel::LIGHT) ? 2 : 3;
            num_forks = std::min(num_forks, forks_allowed);
            
            fork_branch(parent_id, num_forks, kind);
            forks_allowed -= num_forks;
        }
        
        recompute_global_entropy();
    }
    
    // Detect tensions between branches
    void detect_tensions() {
        if (level_ == BranchingLevel::NONE) return;
        
        int checks = 0;
        std::unordered_set<std::string> checked_pairs;
        
        // Only check between adjacent branches (O(n × avg_neighbors))
        for (const auto& id : graph_.active_ids()) {
            if (checks >= budget_.max_tension_checks) break;
            
            for (const auto& neighbor_id : graph_.neighbors(id)) {
                if (!graph_.is_active(neighbor_id)) continue;
                
                std::string pair_key = graph_.canonical_pair_key(id, neighbor_id);
                if (checked_pairs.count(pair_key)) continue;
                checked_pairs.insert(pair_key);
                
                if (checks >= budget_.max_tension_checks) break;
                checks++;
                
                // Check for tension
                const Branch* a = find_branch(id);
                const Branch* b = find_branch(neighbor_id);
                if (!a || !b) continue;
                
                TensionType tension_type;
                float severity;
                std::string description;
                
                if (detect_tension_between(*a, *b, tension_type, severity, description)) {
                    if (!graph_.has_tension(pair_key)) {
                        record_tension(id, neighbor_id, tension_type, severity, description);
                        graph_.record_tension(pair_key);
                    }
                }
            }
        }
    }
    
    // Merge branches based on policy
    void merge() {
        if (level_ == BranchingLevel::NONE) return;
        if (branches_.size() < 2) return;
        
        // Find convergent branches
        std::vector<std::pair<std::string, std::string>> merge_candidates;
        
        for (size_t i = 0; i < branches_.size(); ++i) {
            if (!graph_.is_active(branches_[i].id)) continue;
            
            for (size_t j = i + 1; j < branches_.size(); ++j) {
                if (!graph_.is_active(branches_[j].id)) continue;
                
                if (should_merge(branches_[i], branches_[j])) {
                    merge_candidates.emplace_back(branches_[i].id, branches_[j].id);
                }
            }
        }
        
        // Execute merges
        for (const auto& [id_a, id_b] : merge_candidates) {
            execute_merge(id_a, id_b);
        }
        
        recompute_global_entropy();
    }
    
    // Compress: Summarize branches that have stabilized
    void compress() {
        if (level_ == BranchingLevel::NONE) return;
        
        for (auto& b : branches_) {
            if (!graph_.is_active(b.id)) continue;
            if (b.entropy > 0.3f) continue;  // Only compress low-entropy branches
            if (b.concepts.size() < 3) continue;  // Need content to compress
            
            // Generate summary
            std::string content;
            for (const auto& c : b.concepts) {
                content += c + "\n";
            }
            
            if (generate_fn_) {
                std::string prompt = "Summarize the following concepts into a single coherent statement:\n" + content;
                b.summary = generate_fn_(prompt, 0.3f, 256);
                
                // Compress concepts into summary
                b.concepts.clear();
                b.concepts.push_back(b.summary);
                b.entropy *= 0.5f;  // Reduce entropy after compression
                
                fprintf(stderr, "[BRANCH-ENGINE] Compressed branch %s\n", b.id.c_str());
            }
        }
    }
    
    // ========================================================================
    // OUTPUT
    // ========================================================================
    
    // Produce final output based on merge strategy
    std::string produce_output() {
        if (branches_.empty()) return "";
        
        switch (merge_policy_.strategy) {
            case MergeStrategy::SYNTHESIS:
                return synthesize_all();
                
            case MergeStrategy::SELECTION:
                return select_best();
                
            case MergeStrategy::CURATED:
                return present_options();
                
            case MergeStrategy::CONDITIONAL:
                return produce_conditional();
        }
        
        return synthesize_all();
    }
    
    // Get branches eligible for commit
    std::vector<const Branch*> get_committable_branches() const {
        std::vector<const Branch*> result;
        
        for (const auto& b : branches_) {
            if (b.eligibility == CommitEligibility::NEVER) continue;
            if (b.eligibility == CommitEligibility::VALIDATED_ONLY && !b.validated) continue;
            result.push_back(&b);
        }
        
        return result;
    }
    
    // Get unresolved tensions
    std::vector<const Tension*> get_unresolved_tensions() const {
        std::vector<const Tension*> result;
        for (const auto& t : tensions_) {
            if (!t.resolved) result.push_back(&t);
        }
        return result;
    }
    
    // ========================================================================
    // ACCESSORS
    // ========================================================================
    
    float global_entropy() const { return global_entropy_; }
    int iteration_count() const { return iteration_count_; }
    size_t branch_count() const { return graph_.active_count(); }
    size_t tension_count() const { return tensions_.size(); }
    BranchingLevel level() const { return level_; }
    
    const std::vector<Branch>& branches() const { return branches_; }
    const std::vector<Tension>& tensions() const { return tensions_; }

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================
    
    std::string generate_branch_id() {
        return "b" + std::to_string(++branch_counter_);
    }
    
    std::string generate_tension_id() {
        return "t" + std::to_string(++tension_counter_);
    }
    
    int64_t current_time_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    
    bool check_budget() const {
        if (iteration_count_ >= budget_.max_iterations) {
            fprintf(stderr, "[BRANCH-ENGINE] Budget exhausted: max iterations\n");
            return false;
        }
        
        int64_t elapsed = current_time_ms() - start_time_;
        if (elapsed > budget_.max_total_time_ms) {
            fprintf(stderr, "[BRANCH-ENGINE] Budget exhausted: max time\n");
            return false;
        }
        
        return true;
    }
    
    float compute_initial_entropy(const std::string& content) const {
        // Simple entropy estimate based on content characteristics
        float entropy = 0.5f;
        
        // More questions = higher entropy
        int question_count = std::count(content.begin(), content.end(), '?');
        entropy += question_count * 0.1f;
        
        // Hedge words increase entropy
        if (content.find("maybe") != std::string::npos ||
            content.find("perhaps") != std::string::npos ||
            content.find("might") != std::string::npos ||
            content.find("could be") != std::string::npos) {
            entropy += 0.15f;
        }
        
        // Contradictions increase entropy
        if (content.find("however") != std::string::npos ||
            content.find("but") != std::string::npos ||
            content.find("on the other hand") != std::string::npos) {
            entropy += 0.1f;
        }
        
        return std::min(entropy, 1.0f);
    }
    
    bool should_fork(const Branch& b, BranchKind& out_kind) const {
        // Check entropy threshold
        if (triggers_.on_uncertainty && b.entropy > triggers_.entropy_threshold) {
            out_kind = BranchKind::UNCERTAINTY;
            return true;
        }
        
        // Check confidence threshold
        if (triggers_.on_ambiguity && b.confidence < triggers_.confidence_threshold) {
            out_kind = BranchKind::AMBIGUITY;
            return true;
        }
        
        // Check for specific triggers in content
        std::string content;
        for (const auto& c : b.concepts) content += c + " ";
        
        if (triggers_.on_contradiction && 
            (content.find("contradict") != std::string::npos ||
             content.find("conflict") != std::string::npos)) {
            out_kind = BranchKind::CONTRADICTION;
            return true;
        }
        
        if (triggers_.on_alternative_frame &&
            (content.find("alternatively") != std::string::npos ||
             content.find("another approach") != std::string::npos ||
             content.find("different perspective") != std::string::npos)) {
            out_kind = BranchKind::FRAME;
            return true;
        }
        
        if (triggers_.on_design_tradeoff &&
            (content.find("trade-off") != std::string::npos ||
             content.find("tradeoff") != std::string::npos ||
             content.find("vs") != std::string::npos ||
             content.find("versus") != std::string::npos)) {
            out_kind = BranchKind::TRADE_OFF;
            return true;
        }
        
        if (triggers_.on_implementation_choice &&
            (content.find("could implement") != std::string::npos ||
             content.find("another way") != std::string::npos ||
             content.find("option") != std::string::npos)) {
            out_kind = BranchKind::IMPLEMENTATION;
            return true;
        }
        
        return false;
    }
    
    void fork_branch(const std::string& parent_id, int num_forks, BranchKind kind) {
        Branch* parent = nullptr;
        for (auto& b : branches_) {
            if (b.id == parent_id) { parent = &b; break; }
        }
        if (!parent) return;
        
        fprintf(stderr, "[BRANCH-ENGINE] Forking %s into %d branches (kind=%s)\n",
                parent_id.c_str(), num_forks, branch_kind_name(kind));
        
        for (int i = 0; i < num_forks; ++i) {
            Branch child;
            child.id = generate_branch_id();
            child.kind = kind;
            child.eligibility = default_eligibility_;
            child.parent_id = parent_id;
            child.depth = parent->depth + 1;
            child.entropy = parent->entropy * 0.9f;  // Slightly reduce entropy
            child.confidence = parent->confidence;
            child.created_at = current_time_ms();
            
            // Copy parent concepts
            child.concepts = parent->concepts;
            
            // Generate divergent content if we have generate_fn
            if (generate_fn_) {
                std::string prompt = build_fork_prompt(*parent, kind, i);
                std::string expansion = generate_fn_(prompt, 0.7f, 512);
                if (!expansion.empty()) {
                    child.concepts.push_back(expansion);
                }
            }
            
            branches_.push_back(child);
            graph_.add_branch(child.id);
            graph_.link(parent_id, child.id);
            parent->child_ids.push_back(child.id);
        }
        
        // Deactivate parent after forking
        graph_.remove_branch(parent_id);
    }
    
    std::string build_fork_prompt(const Branch& parent, BranchKind kind, int variant) const {
        std::string content;
        for (const auto& c : parent.concepts) content += c + "\n";
        
        switch (kind) {
            case BranchKind::AMBIGUITY:
                return "Given: " + content + "\nExplore interpretation #" + 
                       std::to_string(variant + 1) + " of this ambiguous content:";
                       
            case BranchKind::FRAME:
                return "Given: " + content + "\nReframe this from perspective #" +
                       std::to_string(variant + 1) + ":";
                       
            case BranchKind::IMPLEMENTATION:
                return "Given: " + content + "\nProvide implementation approach #" +
                       std::to_string(variant + 1) + ":";
                       
            case BranchKind::TRADE_OFF:
                return "Given: " + content + "\nExplore trade-off option #" +
                       std::to_string(variant + 1) + ":";
                       
            case BranchKind::CREATIVE_VARIATION:
                return "Given: " + content + "\nCreate variation #" +
                       std::to_string(variant + 1) + ":";
                       
            case BranchKind::ASSOCIATION:
                return "Given: " + content + "\nFollow association chain #" +
                       std::to_string(variant + 1) + ":";
                       
            default:
                return "Given: " + content + "\nExplore alternative #" +
                       std::to_string(variant + 1) + ":";
        }
    }
    
    bool detect_tension_between(const Branch& a, const Branch& b,
                                TensionType& out_type, float& out_severity,
                                std::string& out_description) const {
        // Simple heuristic detection - real implementation would use LLM judge
        std::string content_a, content_b;
        for (const auto& c : a.concepts) content_a += c + " ";
        for (const auto& c : b.concepts) content_b += c + " ";
        
        // Check for explicit contradiction markers
        if ((content_a.find("not") != std::string::npos && 
             content_b.find("is") != std::string::npos) ||
            (content_a.find("should") != std::string::npos &&
             content_b.find("shouldn't") != std::string::npos)) {
            out_type = TensionType::CONTRADICTION;
            out_severity = 0.7f;
            out_description = "Potential contradiction between branches";
            return true;
        }
        
        // Check for trade-off patterns
        if (content_a.find("performance") != std::string::npos &&
            content_b.find("readability") != std::string::npos) {
            out_type = TensionType::TRADE_OFF;
            out_severity = 0.5f;
            out_description = "Performance vs readability trade-off";
            return true;
        }
        
        return false;
    }
    
    void record_tension(const std::string& branch_a, const std::string& branch_b,
                        TensionType type, float severity, const std::string& description) {
        Tension t;
        t.id = generate_tension_id();
        t.branch_a = branch_a;
        t.branch_b = branch_b;
        t.type = type;
        t.severity = severity;
        t.description = description;
        tensions_.push_back(t);
        
        fprintf(stderr, "[BRANCH-ENGINE] Tension %s: %s <-> %s (severity=%.2f)\n",
                t.id.c_str(), branch_a.c_str(), branch_b.c_str(), severity);
    }
    
    bool should_merge(const Branch& a, const Branch& b) const {
        // Check convergence
        if (merge_policy_.merge_on_convergence) {
            if (a.entropy < 0.2f && b.entropy < 0.2f) return true;
        }
        
        // Check confidence
        if (merge_policy_.merge_on_confidence) {
            if (a.confidence > merge_policy_.auto_merge_confidence &&
                b.confidence > merge_policy_.auto_merge_confidence) {
                return true;
            }
        }
        
        return false;
    }
    
    void execute_merge(const std::string& id_a, const std::string& id_b) {
        Branch* a = nullptr;
        Branch* b = nullptr;
        for (auto& br : branches_) {
            if (br.id == id_a) a = &br;
            if (br.id == id_b) b = &br;
        }
        if (!a || !b) return;
        
        fprintf(stderr, "[BRANCH-ENGINE] Merging %s + %s\n", id_a.c_str(), id_b.c_str());
        
        // Create merged branch
        Branch merged;
        merged.id = generate_branch_id();
        merged.kind = a->kind;  // Inherit kind from first branch
        merged.eligibility = default_eligibility_;
        merged.depth = std::max(a->depth, b->depth);
        merged.parent_id = a->id;  // Track lineage
        merged.created_at = current_time_ms();
        
        // Combine concepts
        for (const auto& c : a->concepts) merged.concepts.push_back(c);
        for (const auto& c : b->concepts) {
            // Avoid duplicates
            bool dup = false;
            for (const auto& mc : merged.concepts) {
                if (mc == c) { dup = true; break; }
            }
            if (!dup) merged.concepts.push_back(c);
        }
        
        // Average entropy/confidence
        merged.entropy = (a->entropy + b->entropy) / 2.0f * 0.8f;  // Reduce after merge
        merged.confidence = std::max(a->confidence, b->confidence);
        
        // Generate synthesis if we have generate_fn
        if (generate_fn_) {
            std::string content;
            for (const auto& c : merged.concepts) content += c + "\n";
            std::string prompt = "Synthesize these concepts into a coherent whole:\n" + content;
            merged.summary = generate_fn_(prompt, 0.5f, 512);
        }
        
        branches_.push_back(merged);
        graph_.add_branch(merged.id);
        graph_.link(a->id, merged.id);
        graph_.link(b->id, merged.id);
        
        // Deactivate merged branches
        graph_.remove_branch(a->id);
        graph_.remove_branch(b->id);
        
        // Resolve related tensions
        for (auto& t : tensions_) {
            if ((t.branch_a == id_a || t.branch_a == id_b) &&
                (t.branch_b == id_a || t.branch_b == id_b)) {
                t.resolved = true;
                t.resolution_branch = merged.id;
            }
        }
    }
    
    void recompute_global_entropy() {
        if (branches_.empty()) {
            global_entropy_ = 1.0f;
            return;
        }
        
        float sum = 0.0f;
        int count = 0;
        for (const auto& b : branches_) {
            if (graph_.is_active(b.id)) {
                sum += b.entropy;
                count++;
            }
        }
        
        global_entropy_ = count > 0 ? sum / count : 1.0f;
    }
    
    const Branch* find_branch(const std::string& id) const {
        for (const auto& b : branches_) {
            if (b.id == id) return &b;
        }
        return nullptr;
    }
    
    // Output strategies
    std::string synthesize_all() const {
        std::string result;
        for (const auto& b : branches_) {
            if (graph_.is_active(b.id)) {
                if (!b.summary.empty()) {
                    result += b.summary + "\n\n";
                } else {
                    for (const auto& c : b.concepts) {
                        result += c + "\n";
                    }
                }
            }
        }
        return result;
    }
    
    std::string select_best() const {
        const Branch* best = nullptr;
        float best_score = -1.0f;
        
        for (const auto& b : branches_) {
            if (!graph_.is_active(b.id)) continue;
            float score = b.confidence * (1.0f - b.entropy);
            if (score > best_score) {
                best_score = score;
                best = &b;
            }
        }
        
        if (!best) return "";
        
        if (!best->summary.empty()) return best->summary;
        
        std::string result;
        for (const auto& c : best->concepts) result += c + "\n";
        return result;
    }
    
    std::string present_options() const {
        std::string result = "## Options\n\n";
        int option = 1;
        
        for (const auto& b : branches_) {
            if (!graph_.is_active(b.id)) continue;
            
            result += "### Option " + std::to_string(option++) + "\n";
            if (!b.summary.empty()) {
                result += b.summary + "\n\n";
            } else {
                for (const auto& c : b.concepts) {
                    result += c + "\n";
                }
            }
            result += "\n";
        }
        
        return result;
    }
    
    std::string produce_conditional() const {
        std::string result;
        int condition = 1;
        
        for (const auto& b : branches_) {
            if (!graph_.is_active(b.id)) continue;
            
            if (condition == 1) {
                result += std::string("If exploring ") + branch_kind_name(b.kind) + ":\n";
            } else {
                result += std::string("Alternatively, if ") + branch_kind_name(b.kind) + ":\n";
            }
            
            if (!b.summary.empty()) {
                result += b.summary + "\n\n";
            } else {
                for (const auto& c : b.concepts) {
                    result += "- " + c + "\n";
                }
            }
            result += "\n";
            condition++;
        }
        
        return result;
    }
};

} // namespace zeta_branching

#endif // ZETA_BRANCHING_ENGINE_H
