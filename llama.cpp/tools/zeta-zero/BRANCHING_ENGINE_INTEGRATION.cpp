// Z.E.T.A. Branching Engine - Server Integration Guide
// ============================================================================
// Branching is now a FIRST-CLASS engine available across all modes.
// This file documents the integration points for zeta-server.cpp.
//
// KEY INVARIANT: Branches are PROVISIONAL. Only validated merges can
// become durable knowledge. This prevents graph contamination.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

/*
============================================================================
ARCHITECTURE OVERVIEW
============================================================================

┌─────────────────────────────────────────────────────────────────────────┐
│                         BRANCHING ENGINE                                 │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ BranchingEngine (zeta-branching-engine.h)                        │   │
│  │  - fork(), merge(), detect_tensions(), compress()               │   │
│  │  - BranchGraph: O(1) adjacency, tension tracking                │   │
│  │  - Mode-agnostic primitives                                     │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                │                                        │
│                    ┌───────────┴───────────┐                           │
│                    ▼                       ▼                           │
│  ┌──────────────────────────┐  ┌──────────────────────────────────┐   │
│  │ ModePolicy (per mode)    │  │ RuntimeOverrides (live tuning)   │   │
│  │  - branching_level       │  │  - branching_level_override      │   │
│  │  - branch_budget         │  │  - budget_override               │   │
│  │  - branch_triggers       │  │                                  │   │
│  │  - merge_policy          │  │                                  │   │
│  │  - commit_policy         │  │                                  │   │
│  │  - kv_policy             │  │                                  │   │
│  │  - validators            │  │                                  │   │
│  └──────────────────────────┘  └──────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘

============================================================================
MODE BRANCHING SUMMARY
============================================================================

| Mode     | Level | Triggers                  | Output      | Commit           |
|----------|-------|---------------------------|-------------|------------------|
| CHAT     | LIGHT | ambiguity, uncertainty    | synthesis   | facts validated  |
| CREATIVE | FULL  | style, plot, character    | curated     | NEVER            |
| RESEARCH | FULL  | frames, trade-offs        | synthesis   | summaries valid  |
| CODE     | LIGHT | impl choice, trade-offs   | selection   | chosen solution  |
| DREAM    | FULL  | novelty, motifs           | curated     | lucid-gated      |


============================================================================
PATCH 1: Add includes (top of zeta-server.cpp, after line ~95)
============================================================================
*/

// Add after other C++ headers:
#include "zeta-branching-engine.h"    // First-class branching primitives
#include "zeta-mode-controller.h"      // Mode switching with branching policies

/*
============================================================================
PATCH 2: Initialize mode controller (in main(), before svr.listen())
============================================================================
*/

void init_mode_controller_example() {
    using namespace zeta_modes;
    
    // Set generation function (hooks into main 14B model)
    g_mode_controller.set_generate_fn([](const std::string& prompt, float temp, int max_tokens) -> std::string {
        // Use existing generation infrastructure
        // return generate_with_ctx(g_ctx, prompt, max_tokens);
        return ""; // Placeholder
    });
    
    // Set validation function (hooks into semantic critic)
    g_mode_controller.set_validate_fn([](const std::string& content, const std::string& criteria) -> bool {
        // Use existing validation infrastructure
        // return zeta_critic_validate(content, criteria);
        return true; // Placeholder
    });
    
    // Set GitGraph root
    g_mode_controller.set_gitgraph_root("/mnt/GitGraph");
    
    // Set mode change callback (for model swapping, etc.)
    g_mode_controller.set_mode_change_callback([](Mode old_mode, Mode new_mode) {
        fprintf(stderr, "[MODE-SWITCH] %s -> %s\n", mode_name(old_mode), mode_name(new_mode));
        
        // Swap models if needed
        if (new_mode == Mode::CODE) {
            // zeta_switch_to_code_mode();
        } else if (old_mode == Mode::CODE) {
            // zeta_switch_to_chat_mode();
        }
    });
}

/*
============================================================================
PATCH 3: Add RESEARCH to route_query() (around line 920)
============================================================================
*/

// Add before CREATIVE check:
/*
    // 0a. RESEARCH: Deep exploration with epistemic branching
    if (lower.find("/research") != std::string::npos ||
        lower.find("enter research mode") != std::string::npos ||
        lower.find("let's research") != std::string::npos ||
        lower.find("deep dive into") != std::string::npos ||
        lower.find("investigate thoroughly") != std::string::npos ||
        lower.find("explore all angles") != std::string::npos) {
        fprintf(stderr, "[ROUTER] Query classified as RESEARCH (epistemic exploration)\n");
        return "RESEARCH";
    }
*/

/*
============================================================================
PATCH 4: Update apply_task_sampling() for all modes (around line 1065)
============================================================================
*/

void apply_mode_sampling_example(const std::string& query_class) {
    using namespace zeta_modes;
    
    // Get current policy (already includes branching config)
    const ModePolicy& policy = g_mode_controller.policy();
    
    // Apply sampling parameters from policy
    // g_params.sampling.temp = policy.temperature;
    // g_params.sampling.penalty_repeat = policy.penalty_repeat;
    // g_params.sampling.penalty_last_n = policy.penalty_last_n;
    
    fprintf(stderr, "[SAMPLING] Mode=%s temp=%.2f penalty=%.2f branching=%s\n",
            mode_name(g_mode_controller.current_mode()),
            policy.temperature,
            policy.penalty_repeat,
            policy.branching_level == zeta_branching::BranchingLevel::NONE ? "NONE" :
            policy.branching_level == zeta_branching::BranchingLevel::LIGHT ? "LIGHT" : "FULL");
}

/*
============================================================================
PATCH 5: Branching integration in /generate (around line 3750)
============================================================================
*/

void handle_branching_in_generate_example(const std::string& prompt, const std::string& query_class) {
    using namespace zeta_modes;
    
    // Detect and switch mode
    Mode detected = g_mode_controller.detect_mode(prompt, query_class);
    g_mode_controller.switch_mode(detected, prompt);
    
    const ModePolicy& policy = g_mode_controller.policy();
    
    // Check if branching is enabled for this mode
    if (policy.branching_level != zeta_branching::BranchingLevel::NONE) {
        // Seed exploration with prompt
        g_mode_controller.seed_exploration(prompt);
        
        // Run exploration iterations (can be async or limited)
        int max_iterations = 3;  // Adjust based on mode/time budget
        for (int i = 0; i < max_iterations; i++) {
            std::string status = g_mode_controller.iterate();
            fprintf(stderr, "[BRANCHING] %s\n", status.c_str());
            
            // Early exit if converged
            if (g_mode_controller.engine().global_entropy() < 0.2f) break;
        }
        
        // Produce output based on merge strategy
        std::string branched_output = g_mode_controller.conclude();
        
        // For CREATIVE: present options instead of synthesis
        // For CODE: select best implementation
        // For CHAT: synthesize into single answer
        // ... use branched_output as context for final generation
    }
    
    // Apply commit policy
    bool should_commit = ZETA_SHOULD_COMMIT();
    bool should_inject_kv = ZETA_SHOULD_INJECT_KV();
    
    // ... rest of generation
}

/*
============================================================================
PATCH 6: Add /branching endpoint for control and status
============================================================================
*/

/*
    svr.Post("/branching", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        using namespace zeta_modes;
        using namespace zeta_branching;
        
        // Parse action
        std::string action = "status";
        // ... parse from req.body
        
        std::string result;
        
        if (action == "override") {
            // Parse override parameters
            std::string level_str;  // "none", "light", "full"
            int max_branches = -1, max_depth = -1, max_iterations = -1;
            // ... parse from req.body
            
            if (!level_str.empty()) {
                BranchingLevel level = BranchingLevel::NONE;
                if (level_str == "light") level = BranchingLevel::LIGHT;
                else if (level_str == "full") level = BranchingLevel::FULL;
                g_mode_controller.set_branching_level_override(level);
            }
            
            if (max_branches > 0 || max_depth > 0 || max_iterations > 0) {
                g_mode_controller.set_budget_override(max_branches, max_depth, max_iterations);
            }
            
            result = "{\"status\": \"override_applied\"}";
        }
        else if (action == "clear_overrides") {
            g_mode_controller.clear_overrides();
            result = "{\"status\": \"overrides_cleared\"}";
        }
        else if (action == "iterate") {
            std::string status = g_mode_controller.iterate();
            result = "{\"status\": \"iterated\", \"result\": \"" + status + "\"}";
        }
        else if (action == "conclude") {
            std::string output = g_mode_controller.conclude();
            result = "{\"status\": \"concluded\", \"output\": \"" + output + "\"}";
        }
        else {  // status
            result = g_mode_controller.status();
        }
        
        res.set_content(result, "application/json");
    });
*/

/*
============================================================================
PATCH 7: Add /mode endpoint for explicit mode switching
============================================================================
*/

/*
    svr.Post("/mode", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        using namespace zeta_modes;
        
        // Parse mode from body
        std::string mode_str = "chat";
        // ... parse from req.body
        
        Mode target = Mode::CHAT;
        if (mode_str == "creative") target = Mode::CREATIVE;
        else if (mode_str == "research") target = Mode::RESEARCH;
        else if (mode_str == "code") target = Mode::CODE;
        else if (mode_str == "dream") target = Mode::DREAM;
        
        g_mode_controller.switch_mode(target);
        
        const ModePolicy& policy = g_mode_controller.policy();
        
        char json[512];
        snprintf(json, sizeof(json),
            "{"
            "\"mode\": \"%s\", "
            "\"branching\": \"%s\", "
            "\"max_branches\": %d, "
            "\"commit_policy\": \"%s\", "
            "\"kv_injection\": %s"
            "}",
            mode_name(g_mode_controller.current_mode()),
            policy.branching_level == zeta_branching::BranchingLevel::NONE ? "none" :
            policy.branching_level == zeta_branching::BranchingLevel::LIGHT ? "light" : "full",
            policy.branch_budget.max_branches,
            policy.commit_policy == CommitPolicy::NEVER ? "never" :
            policy.commit_policy == CommitPolicy::FACTS_VALIDATED ? "facts_validated" :
            policy.commit_policy == CommitPolicy::SUMMARIES_VALIDATED ? "summaries_validated" :
            policy.commit_policy == CommitPolicy::SELECTED_SOLUTION ? "selected_solution" : "lucid_gated",
            policy.kv_policy != KVInjectionPolicy::NONE ? "true" : "false"
        );
        
        res.set_content(json, "application/json");
    });
*/

/*
============================================================================
PATCH 8: Graph commit gatekeeping
============================================================================

Replace direct graph commits with policy check:

    // Before: unconditional commit
    // zeta_graph_commit_fact(fact);
    
    // After: policy-gated commit
    if (zeta_modes::ZETA_SHOULD_COMMIT()) {
        // Check validators based on current policy
        const auto& validators = zeta_modes::g_mode_controller.policy().validators;
        bool valid = true;
        
        if (validators.require_facts_validator) {
            valid = valid && validate_fact(fact);
        }
        if (validators.require_causality_check) {
            valid = valid && check_causality(fact);
        }
        
        if (valid) {
            zeta_graph_commit_fact(fact);
        } else {
            fprintf(stderr, "[GRAPH] Blocked commit - validation failed\n");
        }
    } else {
        fprintf(stderr, "[GRAPH] Skipped commit - mode policy (commit_policy=%d)\n",
                (int)zeta_modes::g_mode_controller.policy().commit_policy);
    }


============================================================================
API USAGE EXAMPLES
============================================================================

# Check current mode and branching config
curl -X POST localhost:8080/mode -d '{}'

# Switch to research mode with full branching
curl -X POST localhost:8080/mode -d '{"mode": "research"}'

# Override branching level for current session
curl -X POST localhost:8080/branching -d '{"action": "override", "level": "full", "max_branches": 10}'

# Run exploration iteration
curl -X POST localhost:8080/branching -d '{"action": "iterate"}'

# Get branching status
curl -X POST localhost:8080/branching -d '{"action": "status"}'

# Conclude and get output
curl -X POST localhost:8080/branching -d '{"action": "conclude"}'

# Clear runtime overrides
curl -X POST localhost:8080/branching -d '{"action": "clear_overrides"}'

# Natural language in /generate automatically triggers mode detection:
curl localhost:8080/generate -d '{"prompt": "Write a story about a robot"}'  # -> CREATIVE
curl localhost:8080/generate -d '{"prompt": "Implement a binary search"}'    # -> CODE  
curl localhost:8080/generate -d '{"prompt": "/research quantum computing"}' # -> RESEARCH
curl localhost:8080/generate -d '{"prompt": "What is the capital of France?"}' # -> CHAT


============================================================================
BRANCHING BEHAVIOR BY MODE
============================================================================

CHAT (facts mode):
  - Branching: LIGHT (2-4 branches)
  - Triggers: ambiguity, uncertainty, contradictions
  - Output: SYNTHESIS (merge into single answer)
  - Commit: FACTS_VALIDATED (only high-confidence validated facts)
  - Tensions: Surface as "open questions", not creative resolution
  - Example: "What causes climate change?" → explores multiple factors, synthesizes

CODE (implementation mode):
  - Branching: LIGHT (2-6 branches, user can override to FULL)
  - Triggers: design trade-offs, implementation choices, API constraints
  - Output: SELECTION (pick best, not synthesize)
  - Commit: SELECTED_SOLUTION (chosen solution + rationale)
  - Tensions: Treat as explicit trade-offs, preserve for documentation
  - Example: "Implement sorting" → explores quicksort vs mergesort, selects best

CREATIVE (fiction mode):
  - Branching: FULL (6-12 branches)
  - Triggers: style, plot, character motivation
  - Output: CURATED (present options for user selection)
  - Commit: NEVER (no graph pollution)
  - Tensions: Creative tension is the product
  - Example: "Write a mystery" → explores multiple plot branches, presents options

RESEARCH (epistemic mode):
  - Branching: FULL (8 branches, depth 5)
  - Triggers: frames, trade-offs, contradictions, uncertainty
  - Output: SYNTHESIS (with explicit tensions preserved)
  - Commit: SUMMARIES_VALIDATED (only after validation)
  - Tensions: Explicit tracking, resolution required for commit
  - Example: "/research quantum computing" → deep exploration with validation

DREAM (idle mode):
  - Branching: FULL (10 branches, depth 6)
  - Triggers: novelty, motif recombination
  - Output: CURATED (don't force resolution)
  - Commit: LUCID_GATED (requires explicit lucid review)
  - Tensions: Preserve contradictions as creative tension
  - Example: Memory replay during idle → free association chains

*/
