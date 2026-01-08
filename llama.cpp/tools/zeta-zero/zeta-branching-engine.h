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
#include <mutex>
#include <fstream>
#include <sys/stat.h>

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

// ============================================================================
// EDGE METADATA: Mode-labeled edges for cross-mode traceability
// ============================================================================

struct EdgeMeta {
    std::string from_id;
    std::string to_id;
    std::string edge_type;      // "lineage", "sibling", "semantic", "tension", "merge"
    std::string source_mode;    // "CHAT", "CODE", "RESEARCH", "CREATIVE", "DREAM"
    float weight = 1.0f;
    int64_t created_at = 0;
    bool persisted = false;     // True if synced to GitGraph

    std::string canonical_key() const {
        return from_id < to_id ? from_id + "|" + to_id : to_id + "|" + from_id;
    }
};

// ============================================================================
// BRANCH GRAPH: O(1) adjacency with mode labeling and persistence
// ============================================================================

class BranchGraph {
private:
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency_;
    std::unordered_set<std::string> active_ids_;
    std::unordered_set<std::string> tension_ids_;

    // === ENHANCED: Edge metadata for GitGraph integration ===
    std::unordered_map<std::string, EdgeMeta> edge_metadata_;

    // === ENHANCED: Session/Mode tracking ===
    std::string session_id_;
    std::string current_mode_ = "CHAT";
    std::string gitgraph_prefix_;
    int64_t next_local_id_ = 1;
    int64_t current_iteration_ = 0;

    // === ENHANCED: Persistence ===
    std::string persist_path_;
    bool dirty_ = false;

    mutable std::mutex mu_;

    static inline std::string json_escape(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if ((unsigned char)c < 0x20) {
                        // Control char: drop (shouldn't occur for our IDs)
                    } else {
                        out += c;
                    }
            }
        }
        return out;
    }

    static inline void trim_ws(std::string& s) {
        size_t b = 0;
        while (b < s.size() && std::isspace((unsigned char)s[b])) b++;
        size_t e = s.size();
        while (e > b && std::isspace((unsigned char)s[e - 1])) e--;
        if (b == 0 && e == s.size()) return;
        s = s.substr(b, e - b);
    }

    static inline bool parse_json_string_at(const std::string& json, size_t& i, std::string& out) {
        // Expect opening quote
        if (i >= json.size() || json[i] != '"') return false;
        i++;
        std::string s;
        while (i < json.size()) {
            char c = json[i++];
            if (c == '"') {
                out = s;
                return true;
            }
            if (c == '\\' && i < json.size()) {
                char esc = json[i++];
                switch (esc) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    default: s += esc; break;
                }
            } else {
                s += c;
            }
        }
        return false;
    }

    static inline bool find_json_key(const std::string& json, const std::string& key, size_t& pos) {
        std::string needle = "\"" + key + "\"";
        pos = json.find(needle);
        return pos != std::string::npos;
    }

    static inline bool parse_json_value_string(const std::string& json, const std::string& key, std::string& out) {
        size_t pos = 0;
        if (!find_json_key(json, key, pos)) return false;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
        size_t i = pos;
        return parse_json_string_at(json, i, out);
    }

    static inline bool parse_json_value_int64(const std::string& json, const std::string& key, int64_t& out) {
        size_t pos = 0;
        if (!find_json_key(json, key, pos)) return false;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
        size_t end = pos;
        while (end < json.size() && (std::isdigit((unsigned char)json[end]) || json[end] == '-' || json[end] == '+')) end++;
        if (end == pos) return false;
        try {
            out = std::stoll(json.substr(pos, end - pos));
            return true;
        } catch (...) {
            return false;
        }
    }

    static inline bool parse_json_value_double(const std::string& json, const std::string& key, double& out) {
        size_t pos = 0;
        if (!find_json_key(json, key, pos)) return false;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
        size_t end = pos;
        while (end < json.size() && (std::isdigit((unsigned char)json[end]) || json[end] == '-' || json[end] == '+' || json[end] == '.' || json[end] == 'e' || json[end] == 'E')) end++;
        if (end == pos) return false;
        try {
            out = std::stod(json.substr(pos, end - pos));
            return true;
        } catch (...) {
            return false;
        }
    }

    static inline bool parse_json_value_bool(const std::string& json, const std::string& key, bool& out) {
        size_t pos = 0;
        if (!find_json_key(json, key, pos)) return false;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
        if (json.compare(pos, 4, "true") == 0) { out = true; return true; }
        if (json.compare(pos, 5, "false") == 0) { out = false; return true; }
        return false;
    }

    static inline bool parse_json_array_strings(const std::string& json, const std::string& key, std::vector<std::string>& out) {
        out.clear();
        size_t pos = 0;
        if (!find_json_key(json, key, pos)) return false;
        pos = json.find('[', pos);
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < json.size()) {
            while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
            if (pos < json.size() && json[pos] == ']') {
                return true;
            }
            if (pos >= json.size()) return false;
            if (json[pos] != '"') {
                // Skip unexpected tokens
                pos++;
                continue;
            }
            size_t i = pos;
            std::string s;
            if (!parse_json_string_at(json, i, s)) return false;
            out.push_back(s);
            pos = i;
            while (pos < json.size() && json[pos] != ',' && json[pos] != ']') pos++;
            if (pos < json.size() && json[pos] == ',') pos++;
        }
        return false;
    }

    static inline bool extract_edges_array(const std::string& json, std::string& out_edges_array_text) {
        // Find "edges": [ ... ] and return the bracketed substring including [..]
        size_t pos = 0;
        if (!find_json_key(json, "edges", pos)) return false;
        pos = json.find('[', pos);
        if (pos == std::string::npos) return false;
        size_t start = pos;
        int depth = 0;
        for (size_t i = pos; i < json.size(); i++) {
            char c = json[i];
            if (c == '[') depth++;
            else if (c == ']') {
                depth--;
                if (depth == 0) {
                    out_edges_array_text = json.substr(start, (i - start) + 1);
                    return true;
                }
            } else if (c == '"') {
                // Skip strings to avoid bracket confusion
                i++;
                while (i < json.size()) {
                    if (json[i] == '\\' && i + 1 < json.size()) { i += 2; continue; }
                    if (json[i] == '"') break;
                    i++;
                }
            }
        }
        return false;
    }

    static inline void parse_edge_objects(const std::string& edges_array_text, std::vector<EdgeMeta>& out) {
        out.clear();
        // Very small, format-coupled parser: scan for { ... } objects.
        size_t i = 0;
        while (i < edges_array_text.size()) {
            if (edges_array_text[i] == '{') {
                size_t start = i;
                int depth = 0;
                for (; i < edges_array_text.size(); i++) {
                    char c = edges_array_text[i];
                    if (c == '{') depth++;
                    else if (c == '}') {
                        depth--;
                        if (depth == 0) {
                            size_t end = i;
                            std::string obj = edges_array_text.substr(start, (end - start) + 1);
                            EdgeMeta m;
                            std::string from, to, type, mode;
                            double weight = 1.0;
                            int64_t created_at = 0;
                            bool persisted = false;
                            if (parse_json_value_string(obj, "from", from) &&
                                parse_json_value_string(obj, "to", to)) {
                                (void)parse_json_value_string(obj, "type", type);
                                (void)parse_json_value_string(obj, "mode", mode);
                                (void)parse_json_value_double(obj, "weight", weight);
                                (void)parse_json_value_int64(obj, "created_at", created_at);
                                (void)parse_json_value_bool(obj, "persisted", persisted);

                                m.from_id = from;
                                m.to_id = to;
                                m.edge_type = type.empty() ? "lineage" : type;
                                m.source_mode = mode.empty() ? "CHAT" : mode;
                                m.weight = (float)weight;
                                m.created_at = created_at;
                                m.persisted = persisted;
                                out.push_back(m);
                            }
                            break;
                        }
                    } else if (c == '"') {
                        // Skip strings
                        i++;
                        while (i < edges_array_text.size()) {
                            if (edges_array_text[i] == '\\' && i + 1 < edges_array_text.size()) { i += 2; continue; }
                            if (edges_array_text[i] == '"') break;
                            i++;
                        }
                    }
                }
            }
            i++;
        }
    }

    std::string serialize_unlocked() const {
        std::string json = "{\n";
        json += "  \"session_id\": \"" + json_escape(session_id_) + "\",\n";
        json += "  \"mode\": \"" + json_escape(current_mode_) + "\",\n";
        json += "  \"gitgraph_prefix\": \"" + json_escape(gitgraph_prefix_) + "\",\n";
        json += "  \"next_local_id\": " + std::to_string(next_local_id_) + ",\n";
        json += "  \"current_iteration\": " + std::to_string(current_iteration_) + ",\n";

        // Active IDs
        json += "  \"active_ids\": [";
        bool first = true;
        for (const auto& id : active_ids_) {
            if (!first) json += ", ";
            json += "\"" + json_escape(id) + "\"";
            first = false;
        }
        json += "],\n";

        // Edges with metadata
        json += "  \"edges\": [\n";
        first = true;
        for (const auto& [key, meta] : edge_metadata_) {
            (void)key;
            if (!first) json += ",\n";
            json += "    {\"from\": \"" + json_escape(meta.from_id);
            json += "\", \"to\": \"" + json_escape(meta.to_id);
            json += "\", \"type\": \"" + json_escape(meta.edge_type);
            json += "\", \"mode\": \"" + json_escape(meta.source_mode);
            json += "\", \"weight\": " + std::to_string(meta.weight);
            json += ", \"created_at\": " + std::to_string(meta.created_at);
            json += ", \"persisted\": " + std::string(meta.persisted ? "true" : "false") + "}";
            first = false;
        }
        json += "\n  ]\n}\n";
        return json;
    }

    bool save_unlocked() {
        if (persist_path_.empty() || !dirty_) return persist_path_.empty() ? false : true;

        FILE* f = fopen(persist_path_.c_str(), "w");
        if (!f) return false;

        std::string json = serialize_unlocked();
        fwrite(json.c_str(), 1, json.size(), f);
        fclose(f);

        dirty_ = false;
        fprintf(stderr, "[BRANCH-GRAPH] Saved to %s (%zu nodes, %zu edges, mode=%s)\n",
                persist_path_.c_str(), active_ids_.size(), edge_metadata_.size(), current_mode_.c_str());
        return true;
    }

public:
    // === Session initialization (ties to GitGraph) ===
    void init_session(const std::string& session_id,
                      const std::string& mode = "CHAT",
                      const std::string& persist_dir = "") {
    std::lock_guard<std::mutex> lock(mu_);
        session_id_ = session_id;
        current_mode_ = mode;
        gitgraph_prefix_ = mode + "_" + session_id + "_";
        next_local_id_ = 1;
        current_iteration_ = 0;

        if (!persist_dir.empty()) {
            persist_path_ = persist_dir + "/branch_graph_" + mode + "_" + session_id + ".json";
        }

        adjacency_.clear();
        active_ids_.clear();
        tension_ids_.clear();
        edge_metadata_.clear();
        dirty_ = false;
    }

    void set_mode(const std::string& mode) {
        std::lock_guard<std::mutex> lock(mu_);
        current_mode_ = mode;
        gitgraph_prefix_ = mode + "_" + session_id_ + "_";
    }

    // === ID generation (unified with GitGraph) ===
    std::string generate_branch_id(const std::string& prefix = "branch") {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s%s%03lld",
                 gitgraph_prefix_.c_str(), prefix.c_str(), (long long)next_local_id_++);
        return buf;
    }

    std::string to_gitgraph_id(const std::string& local_id) const {
        if (local_id.find(gitgraph_prefix_) == 0) return local_id;
        return gitgraph_prefix_ + local_id;
    }

    // === Original API (enhanced with metadata) ===
    void add_branch(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu_);
        if (id.empty()) return;
        active_ids_.insert(id);
        if (adjacency_.find(id) == adjacency_.end()) {
            adjacency_[id] = {};
        }
        dirty_ = true;
    }

    void remove_branch(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu_);
        active_ids_.erase(id);
        dirty_ = true;
        // Don't remove from adjacency_ - keep for tension tracking
    }

    void link(const std::string& a, const std::string& b,
              const std::string& edge_type = "lineage", float weight = 1.0f) {
        std::lock_guard<std::mutex> lock(mu_);
        if (a.empty() || b.empty() || a == b) return;
        adjacency_[a].insert(b);
        adjacency_[b].insert(a);  // Symmetric

        // Track edge metadata
        EdgeMeta meta;
        meta.from_id = a < b ? a : b;
        meta.to_id = a < b ? b : a;
        meta.edge_type = edge_type;
        meta.source_mode = current_mode_;
        meta.weight = weight;
        meta.created_at = (int64_t)time(NULL);
        meta.persisted = false;
        edge_metadata_[meta.canonical_key()] = meta;

        dirty_ = true;
    }

    void unlink(const std::string& a, const std::string& b) {
        std::lock_guard<std::mutex> lock(mu_);
        adjacency_[a].erase(b);
        adjacency_[b].erase(a);

        std::string key = a < b ? a + "|" + b : b + "|" + a;
        edge_metadata_.erase(key);
        dirty_ = true;
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
    void record_tension(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu_);
        tension_ids_.insert(id);
    }
    bool has_tension(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mu_);
        return tension_ids_.count(id) > 0;
    }
    void clear_tension(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu_);
        tension_ids_.erase(id);
    }

    std::string canonical_pair_key(const std::string& a, const std::string& b) const {
        return a < b ? (a + ":" + b) : (b + ":" + a);
    }

    // === RESEARCH-COMPATIBLE API ===
    // These methods provide API compatibility with research mode

    // Add branch with optional parent (for lineage tracking)
    void add_branch(const std::string& id, const std::string& parent_id) {
        add_branch(id);  // Locks internally
        if (!parent_id.empty() && parent_id != id) {
            std::lock_guard<std::mutex> lock(mu_);
            parent_of_[id] = parent_id;
            // link() locks; but we already hold mu_. Do the minimal equivalent.
            adjacency_[id].insert(parent_id);
            adjacency_[parent_id].insert(id);
            EdgeMeta meta;
            meta.from_id = id < parent_id ? id : parent_id;
            meta.to_id = id < parent_id ? parent_id : id;
            meta.edge_type = "lineage";
            meta.source_mode = current_mode_;
            meta.weight = 1.0f;
            meta.created_at = (int64_t)time(NULL);
            meta.persisted = false;
            edge_metadata_[meta.canonical_key()] = meta;
            dirty_ = true;

            // Link to siblings (other children of same parent)
            for (const auto& [child, parent] : parent_of_) {
                if (parent == parent_id && child != id) {
                    adjacency_[id].insert(child);
                    adjacency_[child].insert(id);
                    EdgeMeta sm;
                    sm.from_id = id < child ? id : child;
                    sm.to_id = id < child ? child : id;
                    sm.edge_type = "sibling";
                    sm.source_mode = current_mode_;
                    sm.weight = 0.5f;
                    sm.created_at = (int64_t)time(NULL);
                    sm.persisted = false;
                    edge_metadata_[sm.canonical_key()] = sm;
                    dirty_ = true;
                }
            }
        }
    }

    // Check if two branches are adjacent
    bool are_adjacent(const std::string& a, const std::string& b) const {
        if (a == b) return false;
        auto it = adjacency_.find(a);
        if (it == adjacency_.end()) return false;
        return it->second.count(b) > 0;
    }

    // Get total number of edges (each edge counted once)
    size_t total_edges() const {
        size_t sum = 0;
        for (const auto& [_, neighbors] : adjacency_) {
            sum += neighbors.size();
        }
        return sum / 2;  // Undirected: each edge counted twice
    }

    // Get degree of a node
    size_t degree(const std::string& id) const {
        auto it = adjacency_.find(id);
        return it != adjacency_.end() ? it->second.size() : 0;
    }

    // Merge two branches into one (inherits neighbors from both)
    void merge_branches(const std::string& merged_id,
                        const std::string& a_id,
                        const std::string& b_id) {
        std::lock_guard<std::mutex> lock(mu_);
        if (merged_id.empty() || a_id.empty() || b_id.empty()) return;

        // Collect all neighbors to inherit
        std::unordered_set<std::string> inherited_neighbors;

        if (adjacency_.count(a_id)) {
            for (const auto& n : adjacency_[a_id]) {
                if (n != a_id && n != b_id && n != merged_id) {
                    inherited_neighbors.insert(n);
                }
            }
        }
        if (adjacency_.count(b_id)) {
            for (const auto& n : adjacency_[b_id]) {
                if (n != a_id && n != b_id && n != merged_id) {
                    inherited_neighbors.insert(n);
                }
            }
        }

        // Remove old branches
        active_ids_.erase(a_id);
        active_ids_.erase(b_id);

        // Add merged branch
        active_ids_.insert(merged_id);
        if (adjacency_.find(merged_id) == adjacency_.end()) {
            adjacency_[merged_id] = {};
        }

        // Link to all inherited neighbors
        for (const auto& n : inherited_neighbors) {
            if (active_ids_.count(n)) {
                adjacency_[merged_id].insert(n);
                adjacency_[n].insert(merged_id);
                EdgeMeta meta;
                meta.from_id = merged_id < n ? merged_id : n;
                meta.to_id = merged_id < n ? n : merged_id;
                meta.edge_type = "merge";
                meta.source_mode = current_mode_;
                meta.weight = 0.75f;
                meta.created_at = (int64_t)time(NULL);
                meta.persisted = false;
                edge_metadata_[meta.canonical_key()] = meta;
            }
        }
        dirty_ = true;
    }

    // Parent tracking map (public for research compatibility)
    std::unordered_map<std::string, std::string> parent_of_;

    // === ENHANCED: Edge metadata queries ===
    const EdgeMeta* get_edge_meta(const std::string& a, const std::string& b) const {
        std::string key = a < b ? a + "|" + b : b + "|" + a;
        auto it = edge_metadata_.find(key);
        return it != edge_metadata_.end() ? &it->second : nullptr;
    }

    std::vector<EdgeMeta> get_edges_by_mode(const std::string& mode) const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<EdgeMeta> result;
        for (const auto& [_, meta] : edge_metadata_) {
            if (meta.source_mode == mode) result.push_back(meta);
        }
        return result;
    }

    std::vector<EdgeMeta> get_unpersisted_edges() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<EdgeMeta> result;
        for (const auto& [_, meta] : edge_metadata_) {
            if (!meta.persisted) result.push_back(meta);
        }
        return result;
    }

    void mark_edges_persisted(const std::vector<std::string>& keys) {
        std::lock_guard<std::mutex> lock(mu_);
        for (const auto& key : keys) {
            if (edge_metadata_.count(key)) {
                edge_metadata_[key].persisted = true;
            }
        }
    }

    // === ENHANCED: Iteration tracking ===
    void advance_iteration() { current_iteration_++; }
    int64_t iteration() const { return current_iteration_; }

    // === ENHANCED: Persistence ===
    std::string serialize() const {
        std::lock_guard<std::mutex> lock(mu_);
        return serialize_unlocked();
    }

    bool save() {
        std::lock_guard<std::mutex> lock(mu_);
        return save_unlocked();
    }

    // Reclaim memory: remove inactive branches and any edges that reference them.
    // This is the counterpart to remove_branch(), which intentionally leaves history behind.
    void vacuum_inactive() {
        std::lock_guard<std::mutex> lock(mu_);

        // Collect inactive ids that still exist in adjacency_
        std::vector<std::string> inactive;
        inactive.reserve(adjacency_.size());
        for (const auto& [id, _] : adjacency_) {
            if (active_ids_.count(id) == 0) inactive.push_back(id);
        }

        if (inactive.empty()) return;

        auto erase_edge_if_inactive = [&](const std::string& a, const std::string& b) -> bool {
            return (active_ids_.count(a) == 0) || (active_ids_.count(b) == 0);
        };

        // Remove references from active nodes to inactive nodes
        for (auto& [id, neigh] : adjacency_) {
            if (active_ids_.count(id) == 0) continue;
            for (auto it = neigh.begin(); it != neigh.end();) {
                if (active_ids_.count(*it) == 0) it = neigh.erase(it);
                else ++it;
            }
        }

        // Drop adjacency buckets for inactive nodes
        for (const auto& id : inactive) {
            adjacency_.erase(id);
            parent_of_.erase(id);
            tension_ids_.erase(id);
        }

        // Drop edge metadata that points to inactive nodes
        for (auto it = edge_metadata_.begin(); it != edge_metadata_.end();) {
            const auto& m = it->second;
            if (erase_edge_if_inactive(m.from_id, m.to_id)) it = edge_metadata_.erase(it);
            else ++it;
        }

        dirty_ = true;
    }

    // Thread-safe snapshot of active ids.
    std::vector<std::string> active_ids_vec() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<std::string> r;
        r.reserve(active_ids_.size());
        for (const auto& id : active_ids_) r.push_back(id);
        return r;
    }

    bool load(const std::string& path) {
        std::lock_guard<std::mutex> lock(mu_);
        FILE* f = fopen(path.c_str(), "r");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size <= 0 || size > (1024 * 1024 * 16)) {
            fclose(f);
            return false;
        }

        std::string json((size_t)size, '\0');
        size_t r = fread(&json[0], 1, (size_t)size, f);
        fclose(f);
        if (r != (size_t)size) return false;

        std::string session_id, mode, prefix;
        int64_t next_local = 1;
        int64_t iter = 0;
        (void)parse_json_value_string(json, "session_id", session_id);
        (void)parse_json_value_string(json, "mode", mode);
        (void)parse_json_value_string(json, "gitgraph_prefix", prefix);
        (void)parse_json_value_int64(json, "next_local_id", next_local);
        (void)parse_json_value_int64(json, "current_iteration", iter);

        std::vector<std::string> active;
        (void)parse_json_array_strings(json, "active_ids", active);

        std::string edges_array;
        std::vector<EdgeMeta> edges;
        if (extract_edges_array(json, edges_array)) {
            parse_edge_objects(edges_array, edges);
        }

        // Reset + apply
        persist_path_ = path;
        session_id_ = session_id;
        current_mode_ = mode.empty() ? "CHAT" : mode;
        gitgraph_prefix_ = !prefix.empty() ? prefix : (current_mode_ + "_" + session_id_ + "_");
        next_local_id_ = (next_local > 0) ? next_local : 1;
        current_iteration_ = (iter >= 0) ? iter : 0;

        adjacency_.clear();
        active_ids_.clear();
        tension_ids_.clear();
        edge_metadata_.clear();
        parent_of_.clear();

        for (const auto& id : active) {
            if (id.empty()) continue;
            active_ids_.insert(id);
            if (adjacency_.find(id) == adjacency_.end()) adjacency_[id] = {};
        }

        // Rebuild edges + adjacency from metadata
        for (const auto& e : edges) {
            if (e.from_id.empty() || e.to_id.empty() || e.from_id == e.to_id) continue;
            adjacency_[e.from_id].insert(e.to_id);
            adjacency_[e.to_id].insert(e.from_id);
            edge_metadata_[e.canonical_key()] = e;
        }

        dirty_ = false;
        fprintf(stderr, "[BRANCH-GRAPH] Loaded %ld bytes from %s (%zu active, %zu edges, mode=%s)\n",
                size, path.c_str(), active_ids_.size(), edge_metadata_.size(), current_mode_.c_str());
        return true;
    }

    void checkpoint(int save_interval = 5, int vacuum_interval = 25) {
        std::lock_guard<std::mutex> lock(mu_);
        if (persist_path_.empty()) return;

        if (vacuum_interval > 0 && (current_iteration_ % vacuum_interval == 0)) {
            // Run vacuum first so the persisted file stays compact.
            // (save_unlocked below will set dirty_=false)
            // Note: vacuum_inactive() takes a lock; we're already holding it.
            // Inline the call here.
            std::vector<std::string> inactive;
            inactive.reserve(adjacency_.size());
            for (const auto& [id, _] : adjacency_) {
                if (active_ids_.count(id) == 0) inactive.push_back(id);
            }
            for (auto& [id, neigh] : adjacency_) {
                if (active_ids_.count(id) == 0) continue;
                for (auto it = neigh.begin(); it != neigh.end();) {
                    if (active_ids_.count(*it) == 0) it = neigh.erase(it);
                    else ++it;
                }
            }
            for (const auto& id : inactive) {
                adjacency_.erase(id);
                parent_of_.erase(id);
                tension_ids_.erase(id);
            }
            for (auto it = edge_metadata_.begin(); it != edge_metadata_.end();) {
                const auto& m = it->second;
                if (active_ids_.count(m.from_id) == 0 || active_ids_.count(m.to_id) == 0) it = edge_metadata_.erase(it);
                else ++it;
            }
            if (!inactive.empty()) dirty_ = true;
        }

        if (!dirty_) return;
        if (save_interval > 0 && (current_iteration_ % save_interval == 0)) {
            (void)save_unlocked();
        }
    }

    // === Stats ===
    const std::string& session_id() const { return session_id_; }
    const std::string& current_mode() const { return current_mode_; }
    size_t edge_count() const { return edge_metadata_.size(); }
    bool is_dirty() const { return dirty_; }
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
        graph_.init_session("", "", "");  // Clear without copy
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

// ============================================================================
// BRANCH GRAPH MANAGER: Dataset + Mode Isolation with File-Based Dream Output
// ============================================================================
//
// Architecture:
//   User Session
//   ├── Dataset: "project-a" (isolated)
//   │   ├── CHAT graph    → GitGraph commits
//   │   ├── CODE graph    → GitGraph commits
//   │   ├── RESEARCH graph → GitGraph commits
//   │   ├── CREATIVE graph → File output only (fiction)
//   │   └── DREAM graph   → File output only (pending review)
//   ├── Dataset: "project-b" (isolated)
//   │   └── ... (same structure)
//   └── INCEPTION (cross-dataset synthesis)
//       └── Reads all datasets, outputs cross-link dreams to files
//
// Dream files go to: data/dreams/pending/dream_{timestamp}_{type}.txt
// User reviews, then accepts/rejects → training data pipeline
// ============================================================================

enum class IsolatedMode {
    CHAT,       // Conversational - commits to GitGraph
    CODE,       // Code generation - commits to GitGraph
    RESEARCH,   // Epistemic search - commits to GitGraph
    CREATIVE,   // Fiction/creative - file output only
    DREAM,      // Dream state - file output for review
    INCEPTION   // Cross-dataset synthesis - file output
};

inline const char* isolated_mode_name(IsolatedMode m) {
    switch (m) {
        case IsolatedMode::CHAT: return "CHAT";
        case IsolatedMode::CODE: return "CODE";
        case IsolatedMode::RESEARCH: return "RESEARCH";
        case IsolatedMode::CREATIVE: return "CREATIVE";
        case IsolatedMode::DREAM: return "DREAM";
        case IsolatedMode::INCEPTION: return "INCEPTION";
    }
    return "UNKNOWN";
}

inline IsolatedMode parse_isolated_mode(const std::string& s) {
    if (s == "CHAT") return IsolatedMode::CHAT;
    if (s == "CODE") return IsolatedMode::CODE;
    if (s == "RESEARCH") return IsolatedMode::RESEARCH;
    if (s == "CREATIVE") return IsolatedMode::CREATIVE;
    if (s == "DREAM") return IsolatedMode::DREAM;
    if (s == "INCEPTION") return IsolatedMode::INCEPTION;
    return IsolatedMode::CHAT;
}

inline bool mode_commits_to_graph(IsolatedMode m) {
    return m == IsolatedMode::CHAT || m == IsolatedMode::CODE || m == IsolatedMode::RESEARCH;
}

struct CrossDatasetEdge {
    std::string id;
    std::string from_dataset;
    std::string from_node;
    std::string to_dataset;
    std::string to_node;
    std::string edge_type;
    std::string insight;
    float strength;
    int64_t created_at;
    CrossDatasetEdge() : strength(1.0f), created_at(0) {}
};

struct DreamOutput {
    std::string id;
    std::string dataset_id;
    std::string dataset_anchor;
    IsolatedMode mode;
    std::string dream_type;
    std::string content;
    std::vector<std::string> source_nodes;
    std::vector<CrossDatasetEdge> cross_links;
    int64_t created_at;

    DreamOutput() : mode(IsolatedMode::DREAM), created_at(0) {}

    std::string to_file_content() const {
        std::string result;
        result += "# Dream: " + id + "\n";
        result += "# Type: " + dream_type + "\n";
        result += "# Mode: " + std::string(isolated_mode_name(mode)) + "\n";
        result += "# Dataset: " + dataset_id + "\n";
        result += "# Anchor: " + dataset_anchor + "\n";
        result += "# Created: " + std::to_string(created_at) + "\n";
        result += "#\n# --- DREAM CONTENT ---\n\n";
        result += content;
        result += "\n\n# --- END DREAM ---\n";
        return result;
    }
};

struct DatasetGraphs {
    std::string dataset_id;
    std::string dataset_path;
    std::string dataset_anchor;
    std::string persist_dir;
    int64_t ingested_at;
    std::unordered_map<IsolatedMode, BranchGraph> mode_graphs;

    DatasetGraphs() : ingested_at(0) {}

    void init(const std::string& id, const std::string& path,
              const std::string& anchor, const std::string& persist) {
        dataset_id = id;
        dataset_path = path;
        dataset_anchor = anchor;
        persist_dir = persist;
        ingested_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        for (auto m : {IsolatedMode::CHAT, IsolatedMode::CODE, IsolatedMode::RESEARCH,
                       IsolatedMode::CREATIVE, IsolatedMode::DREAM}) {
            mode_graphs[m].init_session(dataset_id + "_" + isolated_mode_name(m),
                                        isolated_mode_name(m),
                                        persist + "/" + isolated_mode_name(m));
        }
    }

    BranchGraph* get_mode(IsolatedMode m) {
        auto it = mode_graphs.find(m);
        return it != mode_graphs.end() ? &it->second : nullptr;
    }

    void save_all() { for (auto& [m, g] : mode_graphs) g.save(); }
};

// ============================================================================
// INCEPTION BUILDER: Curated Cross-Dataset Synthesis  
// ============================================================================
// Three workflows:
//   A. Explicit picks   - manually select each node
//   B. Filter query     - system suggests, you approve
//   C. Staged review    - system proposes, you accept/reject each

struct InceptionSource {
    std::string dataset_id;
    std::string node_id;
    std::string node_type;
    std::string summary;
    float relevance;
    bool approved;
    bool rejected;
    InceptionSource() : relevance(0.0f), approved(false), rejected(false) {}
    std::string composite_id() const { return dataset_id + "::" + node_id; }
};

class InceptionBuilder {
public:
    InceptionBuilder() : manager_(nullptr), review_idx_(0) {}
    void set_manager(void* mgr) { manager_ = mgr; }
    
    // Option A: Explicit picks
    void pick(const std::string& ds, const std::string& node,
              const std::string& type = "node", const std::string& summary = "") {
        InceptionSource s;
        s.dataset_id = ds; s.node_id = node; s.node_type = type;
        s.summary = summary; s.relevance = 1.0f; s.approved = true;
        sources_.push_back(s);
    }
    
    void unpick(const std::string& ds, const std::string& node) {
        std::string t = ds + "::" + node;
        sources_.erase(std::remove_if(sources_.begin(), sources_.end(),
            [&](const InceptionSource& s) { return s.composite_id() == t; }), sources_.end());
    }
    
    // Option B: Filter query suggestions
    void suggest(const std::string& ds, const std::string& node,
                 const std::string& type, const std::string& summary, float rel) {
        InceptionSource s;
        s.dataset_id = ds; s.node_id = node; s.node_type = type;
        s.summary = summary; s.relevance = rel; s.approved = false;
        suggestions_.push_back(s);
    }
    
    std::vector<InceptionSource>& pending_suggestions() { return suggestions_; }
    
    void approve(size_t i) {
        if (i >= suggestions_.size()) return;
        suggestions_[i].approved = true;
        sources_.push_back(suggestions_[i]);
        suggestions_.erase(suggestions_.begin() + i);
    }
    
    void approve_by_id(const std::string& id) {
        for (size_t i = 0; i < suggestions_.size(); ++i)
            if (suggestions_[i].composite_id() == id) { approve(i); return; }
    }
    
    void reject(size_t i) {
        if (i >= suggestions_.size()) return;
        suggestions_[i].rejected = true;
        rejected_.push_back(suggestions_[i]);
        suggestions_.erase(suggestions_.begin() + i);
    }
    
    void reject_by_id(const std::string& id) {
        for (size_t i = 0; i < suggestions_.size(); ++i)
            if (suggestions_[i].composite_id() == id) { reject(i); return; }
    }
    
    void approve_all() {
        for (auto& s : suggestions_) { s.approved = true; sources_.push_back(s); }
        suggestions_.clear();
    }
    
    void reject_all() {
        for (auto& s : suggestions_) { s.rejected = true; rejected_.push_back(s); }
        suggestions_.clear();
    }
    
    // Option C: Staged review
    void propose_from_datasets(const std::vector<std::string>& ds) { proposed_datasets_ = ds; }
    
    InceptionSource* next_for_review() {
        return review_idx_ < suggestions_.size() ? &suggestions_[review_idx_] : nullptr;
    }
    
    void approve_current() {
        if (review_idx_ < suggestions_.size()) {
            suggestions_[review_idx_].approved = true;
            sources_.push_back(suggestions_[review_idx_++]);
        }
    }
    
    void reject_current() {
        if (review_idx_ < suggestions_.size()) {
            suggestions_[review_idx_].rejected = true;
            rejected_.push_back(suggestions_[review_idx_++]);
        }
    }
    
    void skip_current() { if (review_idx_ < suggestions_.size()) review_idx_++; }
    void reset_review() { review_idx_ = 0; }
    bool review_complete() const { return review_idx_ >= suggestions_.size(); }
    size_t review_remaining() const { 
        return review_idx_ < suggestions_.size() ? suggestions_.size() - review_idx_ : 0;
    }
    
    // Synthesis
    std::vector<InceptionSource> approved_sources() const {
        std::vector<InceptionSource> r;
        for (const auto& s : sources_) if (s.approved && !s.rejected) r.push_back(s);
        return r;
    }
    
    std::vector<std::string> involved_datasets() const {
        std::unordered_set<std::string> ds;
        for (const auto& s : sources_) if (s.approved && !s.rejected) ds.insert(s.dataset_id);
        return std::vector<std::string>(ds.begin(), ds.end());
    }
    
    struct SynthesisContext {
        std::string query;
        std::vector<InceptionSource> sources;
        std::vector<std::string> datasets;
        size_t approved_count, rejected_count;
    };
    
    SynthesisContext build_context(const std::string& q) const {
        SynthesisContext c;
        c.query = q;
        c.sources = approved_sources();
        c.datasets = involved_datasets();
        c.approved_count = c.sources.size();
        c.rejected_count = rejected_.size();
        return c;
    }
    
    void clear() {
        sources_.clear(); suggestions_.clear(); rejected_.clear();
        proposed_datasets_.clear(); review_idx_ = 0;
    }
    
    size_t picked_count() const { return sources_.size(); }
    size_t suggestion_count() const { return suggestions_.size(); }
    size_t rejected_count() const { return rejected_.size(); }

private:
    void* manager_;
    std::vector<InceptionSource> sources_;
    std::vector<InceptionSource> suggestions_;
    std::vector<InceptionSource> rejected_;
    std::vector<std::string> proposed_datasets_;
    size_t review_idx_;
};

class BranchGraphManager {
public:
    BranchGraphManager() : cross_edge_counter_(0), dream_counter_(0), auto_switch_(true) {
        dream_dir_ = "data/dreams";
    }

    bool ingest_dataset(const std::string& id, const std::string& path,
                        const std::string& anchor_hash, const std::string& persist_dir) {
        if (datasets_.count(id)) return false;
        DatasetGraphs dg;
        dg.init(id, path, "dataset_doc:" + anchor_hash, persist_dir);
        datasets_[id] = std::move(dg);
        if (auto_switch_ && active_mode_ == IsolatedMode::DREAM) active_dataset_ = id;
        return true;
    }

    bool remove_dataset(const std::string& id) { return datasets_.erase(id) > 0; }

    std::vector<std::string> list_datasets() const {
        std::vector<std::string> r;
        for (const auto& [id, _] : datasets_) r.push_back(id);
        return r;
    }

    DatasetGraphs* get_dataset(const std::string& id) {
        auto it = datasets_.find(id);
        return it != datasets_.end() ? &it->second : nullptr;
    }

    BranchGraph* get_graph(const std::string& dataset_id, IsolatedMode mode) {
        if (mode == IsolatedMode::INCEPTION) return &inception_graph_;
        auto* ds = get_dataset(dataset_id);
        return ds ? ds->get_mode(mode) : nullptr;
    }

    void set_context(const std::string& dataset_id, IsolatedMode mode) {
        active_dataset_ = dataset_id;
        active_mode_ = mode;
    }

    BranchGraph* active_graph() { return get_graph(active_dataset_, active_mode_); }

    std::string save_dream(const DreamOutput& dream) {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        char ts[32];
        std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", std::localtime(&tt));
        std::string filepath = dream_dir_ + "/pending/dream_" + ts + "_" + dream.dream_type + ".txt";
        mkdir((dream_dir_ + "/pending").c_str(), 0755);
        std::ofstream out(filepath);
        if (out) { out << dream.to_file_content(); out.close(); }
        return filepath;
    }

    DreamOutput create_dream(const std::string& type, const std::string& content,
                             const std::vector<std::string>& sources = {}) {
        DreamOutput d;
        d.id = "dream_" + std::to_string(++dream_counter_);
        d.dataset_id = active_dataset_;
        d.mode = active_mode_;
        d.dream_type = type;
        d.content = content;
        d.source_nodes = sources;
        d.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto* ds = get_dataset(active_dataset_);
        if (ds) d.dataset_anchor = ds->dataset_anchor;
        return d;
    }

    std::string create_cross_link(const std::string& from_ds, const std::string& from_node,
                                   const std::string& to_ds, const std::string& to_node,
                                   const std::string& edge_type, const std::string& insight,
                                   float strength = 1.0f) {
        if (!datasets_.count(from_ds) || !datasets_.count(to_ds)) return "";
        CrossDatasetEdge e;
        e.id = "xlink_" + std::to_string(++cross_edge_counter_);
        e.from_dataset = from_ds; e.from_node = from_node;
        e.to_dataset = to_ds; e.to_node = to_node;
        e.edge_type = edge_type; e.insight = insight; e.strength = strength;
        e.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        cross_edges_.push_back(e);
        inception_graph_.link(from_ds + "::" + from_node, to_ds + "::" + to_node, edge_type, strength);
        return e.id;
    }

    DreamOutput create_inception_dream(const std::string& content,
                                        const std::vector<std::string>& datasets,
                                        const std::vector<CrossDatasetEdge>& links) {
        DreamOutput d;
        d.id = "inception_" + std::to_string(++dream_counter_);
        d.dataset_id = "INCEPTION";
        d.mode = IsolatedMode::INCEPTION;
        d.dream_type = "inception";
        d.content = content;
        d.cross_links = links;
        d.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return d;
    }

    std::vector<CrossDatasetEdge> find_cross_patterns(const std::string& type = "") const {
        std::vector<CrossDatasetEdge> r;
        for (const auto& e : cross_edges_)
            if (type.empty() || e.edge_type == type) r.push_back(e);
        return r;
    }

    struct SynthesisContext {
        std::string query;
        std::vector<std::pair<std::string, std::string>> source_nodes;
        std::vector<CrossDatasetEdge> relevant_links;
    };

    SynthesisContext prepare_synthesis(const std::vector<std::string>& ds_ids, const std::string& query) {
        SynthesisContext ctx;
        ctx.query = query;
        for (const auto& id : ds_ids) {
            auto* ds = get_dataset(id);
            if (!ds) continue;
            for (const auto& [m, g] : ds->mode_graphs)
                for (const auto& node : g.active_ids_vec())
                    ctx.source_nodes.emplace_back(id, node);
        }
        for (const auto& e : cross_edges_) {
            bool f = std::find(ds_ids.begin(), ds_ids.end(), e.from_dataset) != ds_ids.end();
            bool t = std::find(ds_ids.begin(), ds_ids.end(), e.to_dataset) != ds_ids.end();
            if (f && t) ctx.relevant_links.push_back(e);
        }
        return ctx;
    }


    // =========================================================================
    // CHAT MODE: Cross-Dataset Retrieval
    // =========================================================================
    
    struct RetrievalResult {
        std::string dataset_id;
        std::string node_id;
        std::string content;
        float relevance;
        IsolatedMode source_mode;
    };
    
    std::vector<RetrievalResult> query_all_datasets(const std::string& query,
                                                     int max_results = 10,
                                                     float min_rel = 0.3f) {
        std::vector<RetrievalResult> results;
        for (auto& [ds_id, ds] : datasets_) {
            for (auto& [mode, graph] : ds.mode_graphs) {
                for (const auto& nid : graph.active_ids_vec()) {
                    auto* b = graph.get_branch(nid);
                    if (!b) continue;
                    float rel = compute_relevance_(query, b->content);
                    if (rel >= min_rel) {
                        results.push_back({ds_id, nid, b->content, rel, mode});
                    }
                }
            }
        }
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.relevance > b.relevance; });
        if ((int)results.size() > max_results) results.resize(max_results);
        return results;
    }
    
    std::vector<RetrievalResult> query_datasets(const std::vector<std::string>& ds_ids,
                                                 const std::string& query, int max_results = 10) {
        std::vector<RetrievalResult> results;
        for (const auto& ds_id : ds_ids) {
            auto* ds = get_dataset(ds_id);
            if (!ds) continue;
            for (auto& [mode, graph] : ds->mode_graphs) {
                for (const auto& nid : graph.active_ids_vec()) {
                    auto* b = graph.get_branch(nid);
                    if (!b) continue;
                    float rel = compute_relevance_(query, b->content);
                    if (rel > 0.1f) results.push_back({ds_id, nid, b->content, rel, mode});
                }
            }
        }
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.relevance > b.relevance; });
        if ((int)results.size() > max_results) results.resize(max_results);
        return results;
    }
    
    // =========================================================================
    // DREAM MODE: Scoped Dreaming
    // =========================================================================
    
    enum class DreamScope { NORMAL, INCEPTION };
    
    struct DreamSession {
        DreamScope scope = DreamScope::NORMAL;
        std::string primary_dataset;
        std::vector<std::string> datasets;
        std::vector<RetrievalResult> context;
        bool active = false;
    };
    
    // NORMAL dream: single dataset scope
    DreamSession begin_dream(const std::string& dataset_id) {
        DreamSession s;
        s.scope = DreamScope::NORMAL;
        s.primary_dataset = dataset_id;
        s.datasets = {dataset_id};
        s.active = true;
        auto* ds = get_dataset(dataset_id);
        if (ds) {
            for (auto& [mode, graph] : ds->mode_graphs) {
                for (const auto& nid : graph.active_ids_vec()) {
                    auto* b = graph.get_branch(nid);
                    if (b) s.context.push_back({dataset_id, nid, b->content, 1.0f, mode});
                }
            }
        }
        active_dream_ = s;
        return s;
    }
    
    // INCEPTION dream: combine multiple datasets
    DreamSession begin_inception_dream(const std::vector<std::string>& dataset_ids) {
        DreamSession s;
        s.scope = DreamScope::INCEPTION;
        s.datasets = dataset_ids;
        if (!dataset_ids.empty()) s.primary_dataset = dataset_ids[0];
        s.active = true;
        for (const auto& ds_id : dataset_ids) {
            auto* ds = get_dataset(ds_id);
            if (!ds) continue;
            for (auto& [mode, graph] : ds->mode_graphs) {
                for (const auto& nid : graph.active_ids_vec()) {
                    auto* b = graph.get_branch(nid);
                    if (b) s.context.push_back({ds_id, nid, b->content, 1.0f, mode});
                }
            }
        }
        active_dream_ = s;
        return s;
    }
    
    // INCEPTION dream from curated InceptionBuilder
    DreamSession begin_inception_dream(const InceptionBuilder& builder) {
        auto curated = builder.build_context("");
        DreamSession s;
        s.scope = DreamScope::INCEPTION;
        s.datasets = curated.datasets;
        s.active = true;
        for (const auto& src : curated.sources) {
            if (!src.approved || src.rejected) continue;
            auto* ds = get_dataset(src.dataset_id);
            if (!ds) continue;
            for (auto& [mode, graph] : ds->mode_graphs) {
                auto* b = graph.get_branch(src.node_id);
                if (b) {
                    s.context.push_back({src.dataset_id, src.node_id, b->content, src.relevance, mode});
                    break;
                }
            }
        }
        active_dream_ = s;
        return s;
    }
    
    std::string end_dream(const std::string& content, const std::string& dtype) {
        if (!active_dream_.active) return "";
        DreamOutput d;
        d.id = "dream_" + std::to_string(++dream_counter_);
        d.dream_type = dtype;
        d.content = content;
        d.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (active_dream_.scope == DreamScope::NORMAL) {
            d.dataset_id = active_dream_.primary_dataset;
            d.mode = IsolatedMode::DREAM;
            auto* ds = get_dataset(active_dream_.primary_dataset);
            if (ds) d.dataset_anchor = ds->dataset_anchor;
        } else {
            d.dataset_id = "INCEPTION";
            d.mode = IsolatedMode::INCEPTION;
            for (const auto& r : active_dream_.context)
                d.source_nodes.push_back(r.dataset_id + "::" + r.node_id);
        }
        active_dream_.active = false;
        return save_dream(d);
    }
    
    const DreamSession& current_dream() const { return active_dream_; }
    bool is_dreaming() const { return active_dream_.active; }

    void set_dream_output_dir(const std::string& d) { dream_dir_ = d; }
    void set_auto_switch_on_ingest(bool v) { auto_switch_ = v; }
    void save_all() { for (auto& [_, ds] : datasets_) ds.save_all(); inception_graph_.save(); }

    const std::string& active_dataset_id() const { return active_dataset_; }
    IsolatedMode active_mode() const { return active_mode_; }
    size_t dataset_count() const { return datasets_.size(); }
    size_t cross_edge_count() const { return cross_edges_.size(); }
    BranchGraph& inception_graph() { return inception_graph_; }

private:
    std::unordered_map<std::string, DatasetGraphs> datasets_;
    std::string active_dataset_;
    IsolatedMode active_mode_ = IsolatedMode::CHAT;
    BranchGraph inception_graph_;
    std::vector<CrossDatasetEdge> cross_edges_;
    int cross_edge_counter_ = 0;
    int dream_counter_ = 0;
    std::string dream_dir_ = "data/dreams";
    bool auto_switch_ = true;
    DreamSession active_dream_;
    
    float compute_relevance_(const std::string& query, const std::string& content) const {
        if (query.empty() || content.empty()) return 0.0f;
        std::vector<std::string> words;
        std::string w;
        for (char c : query) {
            if (std::isalnum(c)) w += std::tolower(c);
            else if (!w.empty()) { words.push_back(w); w.clear(); }
        }
        if (!w.empty()) words.push_back(w);
        std::string lc; lc.reserve(content.size());
        for (char c : content) lc += std::tolower(c);
        int hits = 0;
        for (const auto& x : words) if (lc.find(x) != std::string::npos) hits++;
        return words.empty() ? 0.0f : (float)hits / (float)words.size();
    }
};

} // namespace zeta_branching

#endif // ZETA_BRANCHING_ENGINE_H
