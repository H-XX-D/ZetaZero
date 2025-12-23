// Z.E.T.A. Hierarchical Reasoning Module (HRM)
// Decomposes complex queries into executable sub-plans
// Orchestrates 14B (Planner) and 7B (Executor) in a feedback loop

#ifndef ZETA_HRM_H
#define ZETA_HRM_H

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <iostream>
#include <mutex>
#include <future>
#include <thread>
#include <atomic>
#include "zeta-dual-process.h"
#include "zeta-critic.h"

// Forward declarations for cross-module communication
class ZetaTRM;      // Temporal Reasoning Module
class ZetaDreamState;  // Dream State Manager

// ============================================================================
// DREAM SUGGESTION: Cross-Module Communication Interface
// ============================================================================
// Enables HRM, TRM, and Dream State to share cognitive context

struct ZetaCognitiveSync {
    // Callbacks for cross-module updates
    std::function<void(float)> on_lambda_update;        // TRM lambda adjustment
    std::function<void(const std::string&, float)> on_dream_sync;  // Dream state sync
    std::function<void(const std::string&)> on_trm_push; // Push to TRM stream

    // Current sync state
    bool trm_connected = false;
    bool dream_connected = false;

    void notify_lambda_change(float new_lambda) {
        if (on_lambda_update) on_lambda_update(new_lambda);
    }

    void notify_dream_state(const std::string& state, float anxiety) {
        if (on_dream_sync) on_dream_sync(state, anxiety);
    }

    void push_to_trm(const std::string& content) {
        if (on_trm_push) on_trm_push(content);
    }
};

// Global cognitive sync for inter-module communication
static ZetaCognitiveSync g_cognitive_sync;

// ============================================================================
// Types and Structures
// ============================================================================

// DREAM SUGGESTION: Emotional/Cognitive State Awareness
typedef enum {
    COGNITIVE_STATE_CALM,       // Normal processing
    COGNITIVE_STATE_FOCUSED,    // Deep work, increase depth
    COGNITIVE_STATE_ANXIOUS,    // High load, reduce complexity
    COGNITIVE_STATE_CREATIVE    // Exploration mode, increase branching
} zeta_cognitive_state_t;

// DREAM SUGGESTION: Context container for better encapsulation
typedef struct {
    std::string context_id;
    std::string context_type;    // "emotional", "task", "domain"
    std::string context_value;
    float intensity;             // 0.0 - 1.0
    time_t created_at;
    bool is_active;
} zeta_hrm_context_t;

typedef enum {
    HRM_STEP_PENDING,
    HRM_STEP_IN_PROGRESS,
    HRM_STEP_COMPLETED,
    HRM_STEP_FAILED
} zeta_hrm_status_t;

typedef enum {
    HRM_TYPE_REASONING,   // Pure reasoning/deduction
    HRM_TYPE_RETRIEVAL,   // Fetch from memory graph
    HRM_TYPE_CALCULATION, // Math/Logic
    HRM_TYPE_VERIFICATION // Critic check
} zeta_hrm_step_type_t;

typedef struct {
    int id;
    int parent_id;        // -1 for root
    zeta_hrm_step_type_t type;
    std::string description;
    std::string result;
    zeta_hrm_status_t status;
    float confidence;
    std::vector<int> dependencies; // IDs of steps that must complete first
} zeta_hrm_step_t;

typedef struct {
    std::string original_query;
    std::vector<zeta_hrm_step_t> steps;
    std::string final_answer;
    bool is_complete;
} zeta_hrm_plan_t;

// Callback for model generation (reusing critic's signature or defining new one)
// (prompt, max_tokens, stop_sequence) -> response
typedef std::function<std::string(const std::string&, int, const std::string&)> hrm_gen_fn;

static hrm_gen_fn g_hrm_generate_conscious = nullptr;   // 14B
static hrm_gen_fn g_hrm_generate_subconscious = nullptr; // 7B

// Mutexes for thread-safe model access (models are NOT thread-safe)
static std::mutex g_hrm_conscious_mutex;    // 14B access
static std::mutex g_hrm_subconscious_mutex; // 7B access

// ============================================================================
// HRM Implementation
// ============================================================================

class ZetaHRM {
private:
    zeta_dual_ctx_t* ctx;
    std::mutex plan_mutex;
    bool initialized;

    // DREAM SUGGESTION: Context-aware state management
    zeta_cognitive_state_t cognitive_state = COGNITIVE_STATE_CALM;
    float anxiety_level = 0.0f;           // 0.0 = calm, 1.0 = high anxiety
    int max_recursion_depth = 10;         // Adjusted based on state
    int max_parallel_branches = 4;        // Adjusted based on state
    std::vector<zeta_hrm_context_t> active_contexts;

    // DREAM SUGGESTION: Thresholds for state transitions
    static constexpr float ANXIETY_HIGH_THRESHOLD = 0.7f;
    static constexpr float ANXIETY_LOW_THRESHOLD = 0.3f;
    static constexpr int MIN_RECURSION_DEPTH = 3;
    static constexpr int MAX_RECURSION_DEPTH = 15;

public:
    // Default constructor for static declaration
    ZetaHRM() : ctx(nullptr), initialized(false) {}

    // Constructor with context
    ZetaHRM(zeta_dual_ctx_t* dual_ctx) : ctx(dual_ctx), initialized(true) {}

    // Initialize after construction (for static global)
    void init(zeta_dual_ctx_t* dual_ctx) {
        ctx = dual_ctx;
        initialized = true;
        fprintf(stderr, "[HRM] Initialized with dual context\n");
    }

    bool is_ready() const { return initialized && ctx != nullptr; }

    static void set_models(hrm_gen_fn conscious, hrm_gen_fn subconscious) {
        g_hrm_generate_conscious = conscious;
        g_hrm_generate_subconscious = subconscious;
        fprintf(stderr, "[HRM] Models set for hierarchical reasoning\n");
    }

    // ========================================================================
    // DREAM SUGGESTION: Context-Aware Cognitive State Management
    // ========================================================================

    // Set cognitive/emotional state - adjusts HRM behavior dynamically
    void set_cognitive_state(zeta_cognitive_state_t state) {
        cognitive_state = state;
        adjust_parameters_for_state();
        fprintf(stderr, "[HRM-CONTEXT] Cognitive state: %s\n", get_state_name(state));

        // DREAM SUGGESTION: Sync with TRM and Dream State
        notify_cognitive_change();
    }

    // Set anxiety level (0.0 - 1.0) - affects recursion depth and branching
    void set_anxiety_level(float level) {
        anxiety_level = std::max(0.0f, std::min(1.0f, level));

        // Auto-adjust cognitive state based on anxiety
        if (anxiety_level > ANXIETY_HIGH_THRESHOLD) {
            cognitive_state = COGNITIVE_STATE_ANXIOUS;
        } else if (anxiety_level < ANXIETY_LOW_THRESHOLD) {
            cognitive_state = COGNITIVE_STATE_CALM;
        }

        adjust_parameters_for_state();
        fprintf(stderr, "[HRM-CONTEXT] Anxiety level: %.2f -> depth=%d, branches=%d\n",
                anxiety_level, max_recursion_depth, max_parallel_branches);

        // DREAM SUGGESTION: Sync with TRM and Dream State
        notify_cognitive_change();
    }

    // ========================================================================
    // DREAM SUGGESTION (070606): Cognitive State Change Logging
    // ========================================================================
    // Provides insights into how and why decisions were made over time

    struct CognitiveStateLog {
        time_t timestamp;
        std::string state_before;
        std::string state_after;
        std::string trigger;
        float anxiety_before;
        float anxiety_after;
    };

    std::vector<CognitiveStateLog> cognitive_log;

    void log_cognitive_state_change(const std::string& state_before,
                                     const std::string& state_after,
                                     const std::string& trigger) {
        CognitiveStateLog entry;
        entry.timestamp = time(NULL);
        entry.state_before = state_before;
        entry.state_after = state_after;
        entry.trigger = trigger;
        entry.anxiety_before = anxiety_level;  // Captured before change
        entry.anxiety_after = anxiety_level;   // Will be same unless explicitly changed

        cognitive_log.push_back(entry);

        // Keep log bounded
        if (cognitive_log.size() > 200) {
            cognitive_log.erase(cognitive_log.begin());
        }

        fprintf(stderr, "[HRM-LOG] State change: %s -> %s (trigger: %s)\n",
                state_before.c_str(), state_after.c_str(), trigger.c_str());
    }

    // Export cognitive log as JSON for analysis
    std::string export_cognitive_log_json() {
        std::stringstream ss;
        ss << "[\n";
        bool first = true;
        for (const auto& entry : cognitive_log) {
            if (!first) ss << ",\n";
            first = false;
            ss << "  {\"timestamp\": " << entry.timestamp
               << ", \"before\": \"" << entry.state_before << "\""
               << ", \"after\": \"" << entry.state_after << "\""
               << ", \"trigger\": \"" << entry.trigger << "\""
               << ", \"anxiety_before\": " << entry.anxiety_before
               << ", \"anxiety_after\": " << entry.anxiety_after
               << "}";
        }
        ss << "\n]";
        return ss.str();
    }

    // Get summary of cognitive transitions
    std::string get_cognitive_log_summary() {
        std::stringstream ss;
        ss << "=== Cognitive State Log Summary ===\n";
        ss << "Total transitions: " << cognitive_log.size() << "\n";

        // Count transitions by type
        std::map<std::string, int> transition_counts;
        for (const auto& entry : cognitive_log) {
            std::string key = entry.state_before + " -> " + entry.state_after;
            transition_counts[key]++;
        }

        ss << "\nTransition frequency:\n";
        for (const auto& [transition, count] : transition_counts) {
            ss << "  " << transition << ": " << count << "\n";
        }

        return ss.str();
    }

    // ========================================================================
    // DREAM SUGGESTION: Cross-Module Cognitive Sync
    // ========================================================================

    // Notify TRM and Dream State of cognitive changes
    void notify_cognitive_change() {
        const char* state_name = get_state_name(cognitive_state);

        // Sync with Dream State - adjusts dream intensity/temp
        g_cognitive_sync.notify_dream_state(state_name, anxiety_level);

        // Sync with TRM - adjust lambda based on cognitive load
        float new_lambda = calculate_trm_lambda();
        g_cognitive_sync.notify_lambda_change(new_lambda);

        // Push cognitive event to TRM stream for temporal tracking
        std::stringstream event;
        event << "[COGNITIVE-EVENT] State=" << state_name
              << " Anxiety=" << anxiety_level
              << " Lambda=" << new_lambda;
        g_cognitive_sync.push_to_trm(event.str());

        fprintf(stderr, "[HRM-SYNC] Notified TRM/Dream: state=%s, anxiety=%.2f, lambda=%.4f\n",
                state_name, anxiety_level, new_lambda);
    }

    // Calculate TRM lambda based on current cognitive state
    // Higher anxiety = faster decay (shorter memory)
    // Creative state = slower decay (longer exploration)
    float calculate_trm_lambda() {
        const float BASE_LAMBDA = 0.001f;

        switch (cognitive_state) {
            case COGNITIVE_STATE_ANXIOUS:
                // High anxiety: faster decay to prevent rumination
                return BASE_LAMBDA * 3.0f * (1.0f + anxiety_level);

            case COGNITIVE_STATE_FOCUSED:
                // Focus: moderate decay for sustained attention
                return BASE_LAMBDA * 0.5f;

            case COGNITIVE_STATE_CREATIVE:
                // Creative: slow decay for exploration
                return BASE_LAMBDA * 0.2f;

            case COGNITIVE_STATE_CALM:
            default:
                return BASE_LAMBDA;
        }
    }

    // Get current lambda recommendation for TRM
    float get_recommended_lambda() const {
        return const_cast<ZetaHRM*>(this)->calculate_trm_lambda();
    }

    // DREAM SUGGESTION: HandleContext function for encapsulated context management
    bool handle_context(const std::string& context_type,
                       const std::string& context_value,
                       float intensity = 0.5f) {
        if (context_type.empty() || context_value.empty()) {
            return false;
        }

        // Check for existing context of same type
        for (auto& ctx : active_contexts) {
            if (ctx.context_type == context_type && ctx.is_active) {
                // Update existing context
                ctx.context_value = context_value;
                ctx.intensity = intensity;
                ctx.created_at = time(NULL);
                fprintf(stderr, "[HRM-CONTEXT] Updated %s context: %s (%.2f)\n",
                        context_type.c_str(), context_value.c_str(), intensity);
                return true;
            }
        }

        // Add new context
        zeta_hrm_context_t new_ctx;
        new_ctx.context_id = "ctx_" + std::to_string(time(NULL));
        new_ctx.context_type = context_type;
        new_ctx.context_value = context_value;
        new_ctx.intensity = intensity;
        new_ctx.created_at = time(NULL);
        new_ctx.is_active = true;
        active_contexts.push_back(new_ctx);

        // Special handling for emotional contexts
        if (context_type == "emotional") {
            if (context_value == "anxiety" || context_value == "stress") {
                set_anxiety_level(intensity);
            } else if (context_value == "focus" || context_value == "concentration") {
                set_cognitive_state(COGNITIVE_STATE_FOCUSED);
            } else if (context_value == "creative" || context_value == "exploration") {
                set_cognitive_state(COGNITIVE_STATE_CREATIVE);
            }
        }

        fprintf(stderr, "[HRM-CONTEXT] Added %s context: %s (%.2f)\n",
                context_type.c_str(), context_value.c_str(), intensity);
        return true;
    }

    // Clear a specific context type
    void clear_context(const std::string& context_type) {
        for (auto& ctx : active_contexts) {
            if (ctx.context_type == context_type) {
                ctx.is_active = false;
            }
        }
        fprintf(stderr, "[HRM-CONTEXT] Cleared %s context\n", context_type.c_str());
    }

    // Get current parameters for logging/debugging
    std::string get_context_status() {
        std::stringstream ss;
        ss << "=== HRM Context Status ===\n";
        ss << "Cognitive State: " << get_state_name(cognitive_state) << "\n";
        ss << "Anxiety Level: " << anxiety_level << "\n";
        ss << "Max Recursion: " << max_recursion_depth << "\n";
        ss << "Max Branches: " << max_parallel_branches << "\n";
        ss << "Active Contexts: " << count_active_contexts() << "\n";
        for (const auto& ctx : active_contexts) {
            if (ctx.is_active) {
                ss << "  - " << ctx.context_type << ": " << ctx.context_value
                   << " (" << ctx.intensity << ")\n";
            }
        }
        return ss.str();
    }

private:
    // Adjust HRM parameters based on cognitive state
    void adjust_parameters_for_state() {
        switch (cognitive_state) {
            case COGNITIVE_STATE_ANXIOUS:
                // Reduce complexity under high cognitive load
                max_recursion_depth = MIN_RECURSION_DEPTH;
                max_parallel_branches = 2;
                break;

            case COGNITIVE_STATE_FOCUSED:
                // Deep work mode - increase depth, moderate branching
                max_recursion_depth = MAX_RECURSION_DEPTH;
                max_parallel_branches = 3;
                break;

            case COGNITIVE_STATE_CREATIVE:
                // Exploration mode - moderate depth, high branching
                max_recursion_depth = 8;
                max_parallel_branches = 6;
                break;

            case COGNITIVE_STATE_CALM:
            default:
                // Normal balanced processing
                max_recursion_depth = 10;
                max_parallel_branches = 4;
                break;
        }
    }

    const char* get_state_name(zeta_cognitive_state_t state) {
        switch (state) {
            case COGNITIVE_STATE_CALM: return "CALM";
            case COGNITIVE_STATE_FOCUSED: return "FOCUSED";
            case COGNITIVE_STATE_ANXIOUS: return "ANXIOUS";
            case COGNITIVE_STATE_CREATIVE: return "CREATIVE";
            default: return "UNKNOWN";
        }
    }

    int count_active_contexts() {
        int count = 0;
        for (const auto& ctx : active_contexts) {
            if (ctx.is_active) count++;
        }
        return count;
    }

public:

    // 1. Decompose Query into a Plan
    zeta_hrm_plan_t create_plan(const std::string& query) {
        zeta_hrm_plan_t plan;
        plan.original_query = query;
        plan.is_complete = false;

        if (!g_hrm_generate_conscious) return plan;

        std::stringstream prompt;
        prompt << "Analyze this complex query and break it down into a hierarchical execution plan.\n"
               << "Query: " << query << "\n\n"
               << "Output format (JSON-like):\n"
               << "[\n"
               << "  {\"id\": 1, \"type\": \"RETRIEVAL\", \"desc\": \"Find X...\"},\n"
               << "  {\"id\": 2, \"type\": \"REASONING\", \"desc\": \"Analyze X...\", \"deps\": [1]}\n"
               << "]\n\n"
               << "Plan:";

        std::string response = g_hrm_generate_conscious(prompt.str(), 512, "]");
        
        // Simple parsing (in production, use a real JSON parser)
        // This is a mock parser for the demo
        parse_plan_response(response, plan);
        
        return plan;
    }

    // 2. Execute a single step (thread-safe with model mutexes)
    bool execute_step(zeta_hrm_step_t& step, const zeta_hrm_plan_t& plan) {
        if (step.status == HRM_STEP_COMPLETED) return true;

        // Build context from dependencies
        std::stringstream context;
        for (int dep_id : step.dependencies) {
            for (const auto& s : plan.steps) {
                if (s.id == dep_id) {
                    context << "Context from Step " << s.id << ": " << s.result << "\n";
                }
            }
        }

        step.status = HRM_STEP_IN_PROGRESS;
        std::string result;

        if (step.type == HRM_TYPE_RETRIEVAL) {
            // Use 7B/Subconscious for retrieval/extraction (with mutex)
            if (g_hrm_generate_subconscious) {
                std::stringstream p;
                p << "Context:\n" << context.str() << "\nTask: " << step.description << "\nExtract facts:";

                // Lock 7B model for thread-safe access
                std::lock_guard<std::mutex> lock(g_hrm_subconscious_mutex);
                result = g_hrm_generate_subconscious(p.str(), 256, "\n\n");
            }
        } else {
            // Use 14B/Conscious for reasoning (with mutex)
            if (g_hrm_generate_conscious) {
                std::stringstream p;
                p << "Context:\n" << context.str() << "\nTask: " << step.description << "\nSolve:";

                // Lock 14B model for thread-safe access
                std::lock_guard<std::mutex> lock(g_hrm_conscious_mutex);
                result = g_hrm_generate_conscious(p.str(), 512, "Step completed");
            }
        }

        step.result = result;
        step.status = HRM_STEP_COMPLETED;
        step.confidence = 0.9f; // Placeholder
        return true;
    }

    // 3. Run the full loop with PARALLEL execution of independent steps
    std::string run(const std::string& query) {
        if (!is_ready()) {
            fprintf(stderr, "[HRM] Not initialized, skipping hierarchical reasoning\n");
            return "";
        }

        fprintf(stderr, "[HRM] Decomposing complex query: %.60s...\n", query.c_str());
        std::lock_guard<std::mutex> lock(plan_mutex);

        zeta_hrm_plan_t plan = create_plan(query);

        // Execute steps in dependency order with PARALLEL execution
        // Steps with satisfied dependencies can run concurrently
        bool changed = true;
        int iteration = 0;
        while (changed) {
            changed = false;
            iteration++;

            // Find all ready steps (pending with all deps satisfied)
            std::vector<size_t> ready_indices;
            for (size_t i = 0; i < plan.steps.size(); i++) {
                auto& step = plan.steps[i];
                if (step.status == HRM_STEP_PENDING) {
                    bool deps_satisfied = true;
                    for (int dep : step.dependencies) {
                        for (const auto& s : plan.steps) {
                            if (s.id == dep && s.status != HRM_STEP_COMPLETED) {
                                deps_satisfied = false;
                                break;
                            }
                        }
                        if (!deps_satisfied) break;
                    }

                    if (deps_satisfied) {
                        ready_indices.push_back(i);
                    }
                }
            }

            if (ready_indices.empty()) continue;

            fprintf(stderr, "[HRM-PARALLEL] Iteration %d: %zu ready steps\n",
                    iteration, ready_indices.size());

            if (ready_indices.size() == 1) {
                // Single step: execute directly
                execute_step(plan.steps[ready_indices[0]], plan);
                changed = true;
            } else {
                // Multiple independent steps: execute in PARALLEL
                // Note: We execute sequentially for now to avoid thread safety issues
                // with shared state in plan.steps. The mutex protection in execute_step
                // ensures model access is thread-safe, but step modification isn't.
                //
                // TODO: For true parallelism, copy step data to avoid data races

                fprintf(stderr, "[HRM-PARALLEL] Executing %zu ready steps sequentially (thread-safe mode)\n",
                        ready_indices.size());

                for (size_t idx : ready_indices) {
                    auto& step = plan.steps[idx];
                    fprintf(stderr, "[HRM-STEP] Executing step %d (%s): %s\n",
                            step.id,
                            step.type == HRM_TYPE_RETRIEVAL ? "7B" : "14B",
                            step.description.substr(0, 40).c_str());
                    execute_step(step, plan);
                }

                fprintf(stderr, "[HRM-PARALLEL] Completed %zu steps\n", ready_indices.size());
                changed = true;
            }
        }

        // Synthesize final answer
        std::stringstream synthesis_prompt;
        synthesis_prompt << "Original Query: " << query << "\n\nExecution Results:\n";
        for (const auto& step : plan.steps) {
            synthesis_prompt << "- " << step.description << ": " << step.result << "\n";
        }
        synthesis_prompt << "\nFinal Answer:";

        if (g_hrm_generate_conscious) {
            std::lock_guard<std::mutex> lock(g_hrm_conscious_mutex);
            plan.final_answer = g_hrm_generate_conscious(synthesis_prompt.str(), 1024, "<|im_end|>");
        }

        return plan.final_answer;
    }

private:
    void parse_plan_response(const std::string& response, zeta_hrm_plan_t& plan) {
        // Enhanced plan parser - creates parallelizable multi-step plans
        // Heuristics based on query keywords to create independent steps

        const std::string& q = plan.original_query;

        // Check for multi-part queries that can parallelize
        bool has_compare = (q.find("compare") != std::string::npos ||
                           q.find("versus") != std::string::npos ||
                           q.find(" vs ") != std::string::npos);

        bool has_multiple = (q.find(" and ") != std::string::npos ||
                            q.find("also") != std::string::npos ||
                            q.find("both") != std::string::npos);

        bool has_analyze = (q.find("analyze") != std::string::npos ||
                           q.find("explain") != std::string::npos ||
                           q.find("why") != std::string::npos);

        if (has_compare) {
            // Comparison query: parallel retrieval for both sides
            fprintf(stderr, "[HRM-PLAN] Creating comparison plan (2 parallel retrieval steps)\n");

            zeta_hrm_step_t step1;
            step1.id = 1;
            step1.type = HRM_TYPE_RETRIEVAL;
            step1.description = "Retrieve information about first concept";
            step1.status = HRM_STEP_PENDING;

            zeta_hrm_step_t step2;
            step2.id = 2;
            step2.type = HRM_TYPE_RETRIEVAL;
            step2.description = "Retrieve information about second concept";
            step2.status = HRM_STEP_PENDING;
            // NO dependency - can run in parallel with step 1!

            zeta_hrm_step_t step3;
            step3.id = 3;
            step3.type = HRM_TYPE_REASONING;
            step3.description = "Compare and contrast the two concepts";
            step3.dependencies.push_back(1);
            step3.dependencies.push_back(2);
            step3.status = HRM_STEP_PENDING;

            plan.steps.push_back(step1);
            plan.steps.push_back(step2);
            plan.steps.push_back(step3);

        } else if (has_multiple && has_analyze) {
            // Multi-part analysis: parallel reasoning steps
            fprintf(stderr, "[HRM-PLAN] Creating multi-analysis plan (parallel reasoning)\n");

            zeta_hrm_step_t step1;
            step1.id = 1;
            step1.type = HRM_TYPE_RETRIEVAL;
            step1.description = "Gather all relevant context for: " + q.substr(0, 100);
            step1.status = HRM_STEP_PENDING;

            zeta_hrm_step_t step2;
            step2.id = 2;
            step2.type = HRM_TYPE_REASONING;
            step2.description = "Analyze first aspect of the query";
            step2.dependencies.push_back(1);
            step2.status = HRM_STEP_PENDING;

            zeta_hrm_step_t step3;
            step3.id = 3;
            step3.type = HRM_TYPE_REASONING;
            step3.description = "Analyze second aspect of the query";
            step3.dependencies.push_back(1);
            step3.status = HRM_STEP_PENDING;
            // Note: Step 2 and 3 share same dependency but can run in parallel!

            zeta_hrm_step_t step4;
            step4.id = 4;
            step4.type = HRM_TYPE_VERIFICATION;
            step4.description = "Synthesize analyses into coherent answer";
            step4.dependencies.push_back(2);
            step4.dependencies.push_back(3);
            step4.status = HRM_STEP_PENDING;

            plan.steps.push_back(step1);
            plan.steps.push_back(step2);
            plan.steps.push_back(step3);
            plan.steps.push_back(step4);

        } else {
            // Default: simple 2-step plan (sequential)
            fprintf(stderr, "[HRM-PLAN] Creating simple 2-step plan\n");

            zeta_hrm_step_t step1;
            step1.id = 1;
            step1.type = HRM_TYPE_RETRIEVAL;
            step1.description = "Retrieve relevant context for: " + q.substr(0, 100);
            step1.status = HRM_STEP_PENDING;

            zeta_hrm_step_t step2;
            step2.id = 2;
            step2.type = HRM_TYPE_REASONING;
            step2.description = "Reason about the query using retrieved context";
            step2.dependencies.push_back(1);
            step2.status = HRM_STEP_PENDING;

            plan.steps.push_back(step1);
            plan.steps.push_back(step2);
        }

        fprintf(stderr, "[HRM-PLAN] Created %zu steps\n", plan.steps.size());
    }
};

#endif // ZETA_HRM_H
