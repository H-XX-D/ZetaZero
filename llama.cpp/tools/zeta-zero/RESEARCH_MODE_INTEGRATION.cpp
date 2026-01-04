// Z.E.T.A. Server Integration Patch for Research Mode
// ============================================================================
// This file documents the integration points needed to add Research Mode
// to zeta-server.cpp. Apply these changes to enable fluid mode switching.
// ============================================================================

/*
============================================================================
PATCH 1: Add include at top of zeta-server.cpp (after line ~95)
============================================================================

Add this include after the other C++ headers:

    #include "zeta-mode-controller.h"       // Mode switching: CHAT/CREATIVE/RESEARCH/CODE
    #include "zeta-research.h"              // Research Mode core
    #include "zeta-research-graph-integration.h"  // Research → GitGraph persistence


============================================================================
PATCH 2: Initialize mode controller in main() (before server startup)
============================================================================

Add initialization after model loading but before svr.listen():

    // Initialize Mode Controller
    zeta_modes::g_mode_controller.set_gitgraph_root("/mnt/GitGraph");
    zeta_modes::g_mode_controller.set_generate_fn([](const std::string& prompt, float temp, int max_tokens) -> std::string {
        // Use the main 14B model for research generation
        // This hooks into the existing generation pipeline
        return generate_with_ctx(g_ctx, prompt, max_tokens);
    });
    zeta_modes::g_mode_controller.set_mode_change_callback([](zeta_modes::Mode old_mode, zeta_modes::Mode new_mode) {
        fprintf(stderr, "[MODE-SWITCH] %s -> %s\n", 
                zeta_modes::mode_name(old_mode), zeta_modes::mode_name(new_mode));
        
        // Trigger model swap if needed
        if (new_mode == zeta_modes::Mode::CODE) {
            zeta_switch_to_code_mode();
        } else if (old_mode == zeta_modes::Mode::CODE) {
            zeta_switch_to_chat_mode();
        }
    });


============================================================================
PATCH 3: Modify route_query() to detect RESEARCH mode (around line 920)
============================================================================

Add RESEARCH detection BEFORE the CREATIVE check:

    // 0a. RESEARCH: Deep exploration mode - epistemic branching
    if (lower.find("/research") != std::string::npos ||
        lower.find("enter research mode") != std::string::npos ||
        lower.find("let's research") != std::string::npos ||
        lower.find("deep dive into") != std::string::npos ||
        lower.find("investigate thoroughly") != std::string::npos ||
        lower.find("explore all angles") != std::string::npos) {
        fprintf(stderr, "[ROUTER] Query classified as RESEARCH (epistemic exploration)\n");
        return "RESEARCH";
    }


============================================================================
PATCH 4: Update apply_task_sampling() to handle RESEARCH (around line 1065)
============================================================================

Add RESEARCH case:

    else if (query_class == "RESEARCH") {
        // Research mode: Moderate temperature for controlled exploration
        // Lower penalty to allow revisiting concepts during branching
        g_params.sampling.temp = 0.7f;
        g_params.sampling.top_p = 0.9f;
        g_params.sampling.top_k = 40;
        g_params.sampling.penalty_repeat = 1.1f;
        g_params.sampling.penalty_freq = 0.15f;
        g_params.sampling.penalty_present = 0.15f;
        g_params.sampling.penalty_last_n = 256;
        fprintf(stderr, "[SAMPLING] Task-aware: RESEARCH mode (temp=0.7, penalty=1.1)\n");
    }


============================================================================
PATCH 5: Add RESEARCH handling in /generate endpoint (around line 3750)
============================================================================

After the query routing and before main generation, add:

        // Handle RESEARCH mode specially - uses branching exploration
        if (query_class == "RESEARCH") {
            using namespace zeta_modes;
            
            // Switch to research mode if not already
            Mode detected = g_mode_controller.detect_mode(prompt, query_class);
            if (detected == Mode::RESEARCH) {
                g_mode_controller.switch_mode(Mode::RESEARCH, prompt);
            }
            
            if (g_mode_controller.is_research_active()) {
                // Check for continuation commands
                if (prompt.find("continue") != std::string::npos ||
                    prompt.find("explore more") != std::string::npos) {
                    // Run another iteration
                    std::string status = g_mode_controller.research_iterate();
                    
                    // Return iteration status
                    char json[4096];
                    snprintf(json, sizeof(json),
                        "{\"output\": \"%s\", \"mode\": \"RESEARCH\", \"action\": \"iterate\"}",
                        escape_json(status).c_str());
                    res.set_content(json, "application/json");
                    return;
                }
                
                if (prompt.find("conclude") != std::string::npos ||
                    prompt.find("summarize research") != std::string::npos ||
                    prompt.find("/conclude") != std::string::npos) {
                    // Conclude and produce output
                    std::string conclusion = g_mode_controller.research_conclude();
                    
                    // Switch back to chat mode
                    g_mode_controller.switch_mode(Mode::CHAT);
                    
                    char json[32768];
                    snprintf(json, sizeof(json),
                        "{\"output\": \"%s\", \"mode\": \"CHAT\", \"action\": \"research_concluded\"}",
                        escape_json(conclusion).c_str());
                    res.set_content(json, "application/json");
                    return;
                }
                
                // Regular research query - run one iteration then generate response
                g_mode_controller.research_iterate();
                // Fall through to normal generation with research context
            }
        }
        
        // Apply mode policy
        const auto& policy = zeta_modes::ZETA_MODE_POLICY();
        is_creative_mode = !policy.commit_facts_to_graph;  // Override creative check


============================================================================
PATCH 6: Add /research endpoint for direct research API (after /generate)
============================================================================

    // Research Mode API
    svr.Post("/research", [](const httplib::Request& req, httplib::Response& res) {
        g_last_activity = time(NULL);
        ZETA_DREAM_WAKE();
        res.set_header("Access-Control-Allow-Origin", "*");
        
        using namespace zeta_modes;
        
        // Parse action from body
        std::string action = "status";
        std::string topic;
        
        size_t action_pos = req.body.find("\"action\":");
        if (action_pos != std::string::npos) {
            size_t as = req.body.find('"', action_pos + 9);
            if (as != std::string::npos) {
                size_t ae = req.body.find('"', as + 1);
                if (ae != std::string::npos) action = req.body.substr(as + 1, ae - as - 1);
            }
        }
        
        size_t topic_pos = req.body.find("\"topic\":");
        if (topic_pos != std::string::npos) {
            size_t ts = req.body.find('"', topic_pos + 8);
            if (ts != std::string::npos) {
                size_t te = req.body.find('"', ts + 1);
                if (te != std::string::npos) topic = req.body.substr(ts + 1, te - ts - 1);
            }
        }
        
        std::string result;
        
        if (action == "start") {
            g_mode_controller.switch_mode(Mode::RESEARCH, topic);
            result = "{\"status\": \"started\", \"session\": \"" + 
                     g_mode_controller.research_status() + "\"}";
        }
        else if (action == "iterate") {
            if (!g_mode_controller.is_research_active()) {
                result = "{\"error\": \"No active research session\"}";
            } else {
                std::string status = g_mode_controller.research_iterate();
                result = "{\"status\": \"iterated\", \"result\": \"" + 
                         escape_json(status) + "\"}";
            }
        }
        else if (action == "conclude") {
            if (!g_mode_controller.is_research_active()) {
                result = "{\"error\": \"No active research session\"}";
            } else {
                std::string conclusion = g_mode_controller.research_conclude();
                g_mode_controller.switch_mode(Mode::CHAT);
                result = "{\"status\": \"concluded\", \"output\": \"" + 
                         escape_json(conclusion) + "\"}";
            }
        }
        else if (action == "status") {
            result = "{\"mode\": \"" + std::string(mode_name(g_mode_controller.current_mode())) + 
                     "\", \"research_active\": " + 
                     (g_mode_controller.is_research_active() ? "true" : "false") + "}";
        }
        else {
            result = "{\"error\": \"Unknown action: " + action + "\"}";
        }
        
        res.set_content(result, "application/json");
    });
    
    // Mode switching API
    svr.Post("/mode", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        using namespace zeta_modes;
        
        std::string mode_str = "chat";
        size_t mode_pos = req.body.find("\"mode\":");
        if (mode_pos != std::string::npos) {
            size_t ms = req.body.find('"', mode_pos + 7);
            if (ms != std::string::npos) {
                size_t me = req.body.find('"', ms + 1);
                if (me != std::string::npos) mode_str = req.body.substr(ms + 1, me - ms - 1);
            }
        }
        
        Mode target = Mode::CHAT;
        if (mode_str == "creative") target = Mode::CREATIVE;
        else if (mode_str == "research") target = Mode::RESEARCH;
        else if (mode_str == "code") target = Mode::CODE;
        
        bool success = g_mode_controller.switch_mode(target);
        
        char json[256];
        snprintf(json, sizeof(json),
            "{\"success\": %s, \"mode\": \"%s\", \"policy\": {\"commit_facts\": %s, \"persist_gitgraph\": %s}}",
            success ? "true" : "false",
            mode_name(g_mode_controller.current_mode()),
            g_mode_controller.should_commit_facts() ? "true" : "false",
            g_mode_controller.should_persist_to_gitgraph() ? "true" : "false");
        
        res.set_content(json, "application/json");
    });


============================================================================
PATCH 7: Graph commit gatekeeping (where facts are extracted, ~line 4200)
============================================================================

Replace direct graph commits with policy check:

    // Before: unconditional commit
    // zeta_graph_commit_fact(fact);
    
    // After: policy-gated commit
    if (zeta_modes::ZETA_SHOULD_COMMIT_FACTS()) {
        zeta_graph_commit_fact(fact);
    } else {
        fprintf(stderr, "[GRAPH] Skipped fact commit - mode policy\n");
    }


============================================================================
USAGE EXAMPLE
============================================================================

# Start research session
curl -X POST http://localhost:8080/research -d '{"action": "start", "topic": "quantum computing fundamentals"}'

# Run iterations
curl -X POST http://localhost:8080/research -d '{"action": "iterate"}'

# Conclude research
curl -X POST http://localhost:8080/research -d '{"action": "conclude"}'

# Check current mode
curl -X POST http://localhost:8080/mode -d '{}'

# Switch modes
curl -X POST http://localhost:8080/mode -d '{"mode": "creative"}'
curl -X POST http://localhost:8080/mode -d '{"mode": "chat"}'

# Or via natural language in /generate:
curl -X POST http://localhost:8080/generate -d '{"prompt": "/research quantum computing"}'
curl -X POST http://localhost:8080/generate -d '{"prompt": "continue exploring"}'
curl -X POST http://localhost:8080/generate -d '{"prompt": "/conclude"}'

*/
