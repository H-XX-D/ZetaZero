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
#include "zeta-dual-process.h"
#include "zeta-critic.h"

// ============================================================================
// Types and Structures
// ============================================================================

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

// ============================================================================
// HRM Implementation
// ============================================================================

class ZetaHRM {
private:
    zeta_dual_ctx_t* ctx;
    std::mutex plan_mutex;
    bool initialized;

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

    // 2. Execute a single step
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
            // Use 7B/Subconscious for retrieval/extraction
            if (g_hrm_generate_subconscious) {
                std::stringstream p;
                p << "Context:\n" << context.str() << "\nTask: " << step.description << "\nExtract facts:";
                result = g_hrm_generate_subconscious(p.str(), 256, "\n\n");
            }
        } else {
            // Use 14B/Conscious for reasoning
            if (g_hrm_generate_conscious) {
                std::stringstream p;
                p << "Context:\n" << context.str() << "\nTask: " << step.description << "\nSolve:";
                result = g_hrm_generate_conscious(p.str(), 512, "Step completed");
            }
        }

        step.result = result;
        step.status = HRM_STEP_COMPLETED;
        step.confidence = 0.9f; // Placeholder
        return true;
    }

    // 3. Run the full loop
    std::string run(const std::string& query) {
        if (!is_ready()) {
            fprintf(stderr, "[HRM] Not initialized, skipping hierarchical reasoning\n");
            return "";
        }

        fprintf(stderr, "[HRM] Decomposing complex query: %.60s...\n", query.c_str());
        std::lock_guard<std::mutex> lock(plan_mutex);

        zeta_hrm_plan_t plan = create_plan(query);
        
        // Execute steps in dependency order (naive linear pass for now)
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& step : plan.steps) {
                if (step.status == HRM_STEP_PENDING) {
                    // Check deps
                    bool ready = true;
                    for (int dep : step.dependencies) {
                        for (const auto& s : plan.steps) {
                            if (s.id == dep && s.status != HRM_STEP_COMPLETED) ready = false;
                        }
                    }
                    
                    if (ready) {
                        execute_step(step, plan);
                        changed = true;
                    }
                }
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
            plan.final_answer = g_hrm_generate_conscious(synthesis_prompt.str(), 1024, "<|im_end|>");
        }

        return plan.final_answer;
    }

private:
    void parse_plan_response(const std::string& response, zeta_hrm_plan_t& plan) {
        // Mock parser - assumes the model follows instructions reasonably well
        // In a real implementation, use nlohmann/json
        // For now, we'll just create a dummy linear plan if parsing fails or for testing
        
        // Heuristic: Split by newlines, look for "id" or numbered lists
        // This is a placeholder. The 14B model is smart enough to output JSON if forced,
        // but we'd need the JSON library included.
        
        // Fallback: Create a simple 2-step plan
        zeta_hrm_step_t step1;
        step1.id = 1;
        step1.type = HRM_TYPE_RETRIEVAL;
        step1.description = "Retrieve relevant context for: " + plan.original_query;
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
};

#endif // ZETA_HRM_H
