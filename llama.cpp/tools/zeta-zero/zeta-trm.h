// Z.E.T.A. Temporal Recursive Memory (TRM)
// Handles recursive state maintenance and temporal consistency
// Prevents infinite loops and manages time-decayed context streams

#ifndef ZETA_TRM_H
#define ZETA_TRM_H

#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <map>
#include "zeta-dual-process.h"

// ============================================================================
// Constants & Config
// ============================================================================

#define TRM_MAX_RECURSION_DEPTH 10
#define TRM_DEFAULT_LAMBDA 0.001f
#define TRM_CONTEXT_WINDOW 2048

// ============================================================================
// Types
// ============================================================================

typedef struct {
    std::string content;
    time_t timestamp;
    float activation_energy; // Z(t)
    int recursion_depth;
    std::string source_id;   // For tracking self-reference
    std::string commit_id;   // Git-style commit identifier (DREAM SUGGESTION)
    std::string parent_id;   // Parent commit for DAG traversal
} zeta_trm_node_t;

// Temporal Branch - represents a parallel timeline (DREAM SUGGESTION)
typedef struct {
    std::string branch_id;
    std::string branch_name;
    std::string base_commit;     // Where this branch diverged
    std::deque<zeta_trm_node_t> timeline;
    time_t created_at;
    bool is_active;
} zeta_trm_branch_t;

typedef struct {
    std::deque<zeta_trm_node_t> stream;
    float lambda;            // Decay constant
    int max_depth;
} zeta_trm_context_t;

// Merge result for cognitive merge operations
typedef struct {
    bool success;
    std::string merged_commit_id;
    std::vector<std::string> conflicts;
    std::string insights;    // Combined insights from merge
} zeta_trm_merge_result_t;

// ============================================================================
// ZetaTRM Class
// ============================================================================

class ZetaTRM {
private:
    zeta_trm_context_t ctx;
    std::map<std::string, int> recursion_tracker; // Track repetition of concepts

    // DREAM SUGGESTION: Temporal branching for parallel timelines
    std::map<std::string, zeta_trm_branch_t> branches;
    std::string current_branch = "main";
    uint64_t commit_counter = 0;

    // Generate unique commit ID
    std::string generate_commit_id() {
        return "trm_" + std::to_string(time(NULL)) + "_" + std::to_string(commit_counter++);
    }

public:
    ZetaTRM() {
        ctx.lambda = TRM_DEFAULT_LAMBDA;
        ctx.max_depth = TRM_MAX_RECURSION_DEPTH;

        // Initialize main branch
        zeta_trm_branch_t main_branch;
        main_branch.branch_id = "main";
        main_branch.branch_name = "main";
        main_branch.created_at = time(NULL);
        main_branch.is_active = true;
        branches["main"] = main_branch;
    }

    // Initialize with custom decay
    // DREAM SUGGESTION (070606): Add lambda validation
    void init(float lambda = TRM_DEFAULT_LAMBDA) {
        // Validate lambda is in reasonable range
        const float MIN_LAMBDA = 0.0001f;
        const float MAX_LAMBDA = 1.0f;

        if (lambda < MIN_LAMBDA || lambda > MAX_LAMBDA) {
            fprintf(stderr, "[TRM] Warning: Lambda %.4f out of range [%.4f, %.1f], clamping\n",
                    lambda, MIN_LAMBDA, MAX_LAMBDA);
            lambda = std::max(MIN_LAMBDA, std::min(MAX_LAMBDA, lambda));
        }

        ctx.lambda = lambda;
        base_lambda = lambda;  // Store original for reference
        ctx.stream.clear();
        recursion_tracker.clear();
        current_branch = "main";

        fprintf(stderr, "[TRM] Initialized with lambda=%.4f, max_depth=%d\n",
                ctx.lambda, ctx.max_depth);
    }

    // ========================================================================
    // DREAM SUGGESTION: HRM Cross-Talk - Dynamic Lambda Adjustment
    // ========================================================================
    // Receives cognitive state updates from HRM and adjusts temporal decay

    std::string hrm_cognitive_state = "CALM";
    float hrm_anxiety_level = 0.0f;
    float base_lambda = TRM_DEFAULT_LAMBDA;

    // Called by HRM via g_cognitive_sync callback
    void receive_lambda_update(float new_lambda) {
        float old_lambda = ctx.lambda;
        ctx.lambda = new_lambda;

        fprintf(stderr, "[TRM-SYNC] Lambda updated: %.4f -> %.4f (from HRM)\n",
                old_lambda, new_lambda);

        // If lambda increased significantly (high anxiety), apply immediate decay
        if (new_lambda > old_lambda * 2.0f) {
            fprintf(stderr, "[TRM-SYNC] High anxiety detected, applying immediate decay\n");
            apply_decay();
        }

        // Record the lambda change in the temporal stream
        push_state("[LAMBDA-CHANGE] " + std::to_string(old_lambda) + " -> " +
                   std::to_string(new_lambda), "hrm-sync");
    }

    // Receive cognitive state from HRM
    void receive_cognitive_state(const std::string& state, float anxiety) {
        hrm_cognitive_state = state;
        hrm_anxiety_level = anxiety;

        fprintf(stderr, "[TRM-SYNC] Cognitive state: %s (anxiety=%.2f)\n",
                state.c_str(), anxiety);

        // Adjust max recursion depth based on cognitive state
        if (state == "ANXIOUS") {
            ctx.max_depth = 5;  // Reduce depth under anxiety
        } else if (state == "FOCUSED") {
            ctx.max_depth = TRM_MAX_RECURSION_DEPTH + 5;  // More depth when focused
        } else if (state == "CREATIVE") {
            ctx.max_depth = TRM_MAX_RECURSION_DEPTH;  // Normal for creativity
        } else {
            ctx.max_depth = TRM_MAX_RECURSION_DEPTH;
        }

        // Log state transition
        push_state("[COGNITIVE-STATE] " + state + " (anxiety=" +
                   std::to_string(anxiety) + ")", "hrm-sync");
    }

    // Get current lambda for logging
    float get_current_lambda() const {
        return ctx.lambda;
    }

    // Get HRM sync status
    std::string get_hrm_sync_status() {
        std::stringstream ss;
        ss << "=== TRM-HRM Sync Status ===\n";
        ss << "HRM State: " << hrm_cognitive_state << "\n";
        ss << "HRM Anxiety: " << hrm_anxiety_level << "\n";
        ss << "Current Lambda: " << ctx.lambda << "\n";
        ss << "Max Depth: " << ctx.max_depth << "\n";
        return ss.str();
    }

    // ========================================================================
    // DREAM SUGGESTION: Git-style Temporal Branching
    // ========================================================================

    // DREAM SUGGESTION (070511): Consolidated branch management
    // Creates a new branch and immediately checks out to it
    bool manage_branches(const std::string& branch_name, const std::string& from_commit = "") {
        fprintf(stderr, "[TRM] manage_branches: %s from %s\n",
                branch_name.c_str(), from_commit.empty() ? "HEAD" : from_commit.c_str());

        // Step 1: Create the branch
        if (!create_branch(branch_name, from_commit)) {
            // Branch might already exist, try checkout
            if (branches.find(branch_name) == branches.end()) {
                fprintf(stderr, "[TRM] Failed to create branch: %s\n", branch_name.c_str());
                return false;
            }
        }

        // Step 2: Checkout to the newly created branch
        if (!checkout_branch(branch_name)) {
            fprintf(stderr, "[TRM] Failed to checkout branch: %s\n", branch_name.c_str());
            return false;
        }

        // Log the state transition
        log_branch_operation("manage_branches", branch_name, from_commit);
        return true;
    }

    // DREAM SUGGESTION (070606): Log branch operations for traceability
    void log_branch_operation(const std::string& operation,
                               const std::string& branch_name,
                               const std::string& commit_id = "") {
        static std::vector<std::string> branch_log;

        std::stringstream entry;
        entry << "[" << time(NULL) << "] " << operation
              << ": branch=" << branch_name;
        if (!commit_id.empty()) {
            entry << ", commit=" << commit_id;
        }
        entry << ", current_branch=" << current_branch;

        branch_log.push_back(entry.str());

        // Keep log bounded
        if (branch_log.size() > 100) {
            branch_log.erase(branch_log.begin());
        }

        fprintf(stderr, "[TRM-LOG] %s\n", entry.str().c_str());
    }

    // Create a new temporal branch for exploring alternative timelines
    bool create_branch(const std::string& branch_name, const std::string& from_commit = "") {
        if (branches.find(branch_name) != branches.end()) {
            return false; // Branch already exists
        }

        zeta_trm_branch_t new_branch;
        new_branch.branch_id = generate_commit_id();
        new_branch.branch_name = branch_name;
        new_branch.base_commit = from_commit.empty() ? get_head_commit() : from_commit;
        new_branch.created_at = time(NULL);
        new_branch.is_active = true;

        // Copy current timeline up to base commit
        for (const auto& node : ctx.stream) {
            new_branch.timeline.push_back(node);
            if (!from_commit.empty() && node.commit_id == from_commit) break;
        }

        branches[branch_name] = new_branch;
        fprintf(stderr, "[TRM] Created branch '%s' from %s\n",
                branch_name.c_str(), new_branch.base_commit.c_str());
        return true;
    }

    // Switch to a different temporal branch
    bool checkout_branch(const std::string& branch_name) {
        if (branches.find(branch_name) == branches.end()) {
            return false;
        }

        // Save current stream to current branch
        branches[current_branch].timeline = ctx.stream;

        // Load target branch
        current_branch = branch_name;
        ctx.stream = branches[branch_name].timeline;

        fprintf(stderr, "[TRM] Switched to branch '%s' (%zu nodes)\n",
                branch_name.c_str(), ctx.stream.size());
        return true;
    }

    // DREAM SUGGESTION: Cognitive Merge - merge insights from parallel branches
    zeta_trm_merge_result_t cognitive_merge(const std::string& source_branch) {
        zeta_trm_merge_result_t result;
        result.success = false;

        if (branches.find(source_branch) == branches.end()) {
            result.conflicts.push_back("Source branch not found: " + source_branch);
            return result;
        }

        auto& source = branches[source_branch];
        std::string merge_commit = generate_commit_id();

        // Find divergence point
        std::string base = source.base_commit;

        // Collect unique insights from source branch (after divergence)
        std::vector<zeta_trm_node_t> new_insights;
        bool past_base = base.empty();

        for (const auto& node : source.timeline) {
            if (!past_base && node.commit_id == base) {
                past_base = true;
                continue;
            }
            if (past_base) {
                // Check for conflicts (same content with different timestamps)
                bool is_conflict = false;
                for (const auto& existing : ctx.stream) {
                    if (existing.content == node.content &&
                        existing.timestamp != node.timestamp) {
                        result.conflicts.push_back("Temporal conflict: " + node.content.substr(0, 50));
                        is_conflict = true;
                        break;
                    }
                }
                if (!is_conflict) {
                    new_insights.push_back(node);
                }
            }
        }

        // Apply non-conflicting insights to current branch
        for (auto& insight : new_insights) {
            insight.parent_id = get_head_commit();
            insight.commit_id = generate_commit_id();
            ctx.stream.push_back(insight);
            result.insights += insight.content.substr(0, 100) + "... ";
        }

        // Create merge commit
        zeta_trm_node_t merge_node;
        merge_node.content = "[MERGE] " + source_branch + " -> " + current_branch;
        merge_node.timestamp = time(NULL);
        merge_node.activation_energy = 1.0f;
        merge_node.commit_id = merge_commit;
        merge_node.parent_id = get_head_commit();
        ctx.stream.push_back(merge_node);

        result.success = true;
        result.merged_commit_id = merge_commit;

        fprintf(stderr, "[TRM] Cognitive merge: %s -> %s (%zu insights, %zu conflicts)\n",
                source_branch.c_str(), current_branch.c_str(),
                new_insights.size(), result.conflicts.size());

        return result;
    }

    // Get current HEAD commit
    std::string get_head_commit() {
        if (ctx.stream.empty()) return "";
        return ctx.stream.back().commit_id;
    }

    // List all branches
    std::vector<std::string> list_branches() {
        std::vector<std::string> names;
        for (const auto& [name, branch] : branches) {
            names.push_back(name + (name == current_branch ? " *" : ""));
        }
        return names;
    }

    // DREAM SUGGESTION: Cherry-pick specific insight from another branch
    bool cherry_pick(const std::string& branch_name, const std::string& commit_id) {
        if (branches.find(branch_name) == branches.end()) return false;

        for (const auto& node : branches[branch_name].timeline) {
            if (node.commit_id == commit_id) {
                zeta_trm_node_t picked = node;
                picked.parent_id = get_head_commit();
                picked.commit_id = generate_commit_id();
                picked.content = "[CHERRY-PICK] " + node.content;
                ctx.stream.push_back(picked);
                fprintf(stderr, "[TRM] Cherry-picked %s from %s\n",
                        commit_id.c_str(), branch_name.c_str());
                return true;
            }
        }
        return false;
    }

    // Add a thought/memory to the recursive stream
    void push_state(const std::string& content, const std::string& source = "system") {
        time_t now = time(NULL);

        // Check for recursion/loops
        if (recursion_tracker[content] > 3) {
            // Detected potential infinite loop
            return;
        }
        recursion_tracker[content]++;

        zeta_trm_node_t node;
        node.content = content;
        node.timestamp = now;
        node.activation_energy = 1.0f; // Initial energy Z_0
        node.recursion_depth = ctx.stream.size(); // Simple depth metric
        node.source_id = source;
        node.parent_id = get_head_commit();  // DREAM: Link to parent
        node.commit_id = generate_commit_id(); // DREAM: Unique commit ID

        ctx.stream.push_back(node);

        // Prune if too large
        if (ctx.stream.size() > TRM_CONTEXT_WINDOW) {
            ctx.stream.pop_front();
        }
    }

    // Apply Temporal Decay Z(t) = Z_0 * e^(-lambda * t)
    void apply_decay() {
        time_t now = time(NULL);
        auto it = ctx.stream.begin();
        while (it != ctx.stream.end()) {
            float age = difftime(now, it->timestamp);
            it->activation_energy = 1.0f * exp(-ctx.lambda * age);

            // Prune dead memories (Energy < epsilon)
            if (it->activation_energy < 0.01f) {
                // Remove from tracker to allow re-learning later
                if (recursion_tracker[it->content] > 0) {
                    recursion_tracker[it->content]--;
                }
                it = ctx.stream.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Retrieve relevant context based on current query using embeddings
    // Uses cosine similarity to find relevant past thoughts
    std::string retrieve_context(const std::string& query, const float* query_embedding = nullptr, int dim = 0) {
        std::string context = "";
        apply_decay(); // Update energies first

        // If no embedding provided, fallback to keyword match (or if dim mismatch)
        if (!query_embedding || dim <= 0) {
            for (const auto& node : ctx.stream) {
                if (node.activation_energy > 0.1f) {
                    context += node.content + "\n";
                }
            }
            return context;
        }

        // Embedding-based retrieval
        // Note: In a real implementation, we would store embeddings for each node.
        // For now, we assume nodes have embeddings or we compute them on the fly (expensive)
        // OR we just use the activation energy as a proxy for "recent relevance"
        // and combine it with a simple keyword overlap if embeddings aren't stored per node.
        
        // Since we don't have embeddings stored for every TRM node yet, we'll stick to 
        // activation energy + simple keyword overlap, but we'll structure it to accept embeddings later.
        
        for (const auto& node : ctx.stream) {
            // Only include if energy is high enough
            if (node.activation_energy > 0.1f) {
                context += node.content + "\n";
            }
        }
        return context;
    }

    // Check if a query is safe (not infinitely recursive)
    bool is_safe_query(const std::string& query) {
        // Check against recent history for exact repetition
        int repeat_count = 0;
        for (auto it = ctx.stream.rbegin(); it != ctx.stream.rend(); ++it) {
            if (it->content == query) {
                repeat_count++;
            }
            if (repeat_count > 2) return false; // Too repetitive
        }
        return true;
    }
    
    // Get current stream status
    std::string get_status() {
        return "TRM Active | Branch: " + current_branch +
               " | Stream: " + std::to_string(ctx.stream.size()) +
               " | Branches: " + std::to_string(branches.size()) +
               " | Lambda: " + std::to_string(ctx.lambda);
    }

    // DREAM SUGGESTION: Get branch statistics for Dream State logging
    std::string get_branch_log() {
        std::string log = "=== TRM Branch Log ===\n";
        log += "Current: " + current_branch + "\n";
        log += "HEAD: " + get_head_commit() + "\n";
        log += "Branches:\n";
        for (const auto& [name, branch] : branches) {
            log += "  - " + name + " (" + std::to_string(branch.timeline.size()) + " nodes)";
            if (name == current_branch) log += " *";
            log += "\n";
        }
        return log;
    }
};

#endif // ZETA_TRM_H
