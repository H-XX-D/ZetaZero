// Z.E.T.A. Mode Controller
// ============================================================================
// Unified mode switching: CHAT ↔ CREATIVE ↔ RESEARCH ↔ CODE ↔ DREAM
//
// Architecture:
//   - Each mode has specific epistemic policies
//   - Graph KV integration for persistent state
//   - GitGraph integration for research sessions
//   - Fluid transitions based on query classification + explicit commands
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_MODE_CONTROLLER_H
#define ZETA_MODE_CONTROLLER_H

#include <string>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>
#include "zeta-research.h"

namespace zeta_modes {

// ============================================================================
// MODE DEFINITIONS
// ============================================================================

enum class Mode {
    CHAT,       // Default conversational mode - facts commit to graph
    CREATIVE,   // Fiction/roleplay - NO graph commits (epistemic scoping)
    RESEARCH,   // Deep exploration - branches/tensions/compression
    CODE,       // Programming mode - code-aware context, low temp
    DREAM       // Idle consolidation - high temp, free association
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
// MODE POLICIES
// Each mode has specific behaviors for graph, sampling, and context
// ============================================================================

struct ModePolicy {
    // Sampling parameters
    float temperature;
    float penalty_repeat;
    int penalty_last_n;
    
    // Graph policies
    bool commit_facts_to_graph;      // Should extracted facts go to knowledge graph?
    bool commit_to_gitgraph;         // Should session be persisted to GitGraph?
    bool use_graph_kv_injection;     // Should we inject cached KV from Graph-KV?
    
    // Context policies  
    bool use_trm_context;            // Use Text Recursive Memory retrieval?
    bool use_hrm_decomposition;      // Use Hierarchical Reasoning?
    
    // Research-specific
    bool enable_branching;           // Allow branch/merge dynamics?
    bool preserve_tensions;          // Keep contradictions explicit?
};

inline ModePolicy get_policy(Mode m) {
    switch (m) {
        case Mode::CHAT:
            return {
                .temperature = 0.6f,
                .penalty_repeat = 1.15f,
                .penalty_last_n = 256,
                .commit_facts_to_graph = true,
                .commit_to_gitgraph = true,
                .use_graph_kv_injection = true,
                .use_trm_context = true,
                .use_hrm_decomposition = true,
                .enable_branching = false,
                .preserve_tensions = false
            };
            
        case Mode::CREATIVE:
            return {
                .temperature = 0.85f,
                .penalty_repeat = 1.2f,
                .penalty_last_n = 512,
                .commit_facts_to_graph = false,  // KEY: No graph pollution
                .commit_to_gitgraph = false,     // Don't persist fiction
                .use_graph_kv_injection = false, // Don't inject factual context
                .use_trm_context = false,        // Let creativity flow
                .use_hrm_decomposition = false,  // No reasoning decomposition
                .enable_branching = false,
                .preserve_tensions = false
            };
            
        case Mode::RESEARCH:
            return {
                .temperature = 0.7f,             // Moderate - controlled exploration
                .penalty_repeat = 1.1f,
                .penalty_last_n = 256,
                .commit_facts_to_graph = false,  // Only validated summaries commit
                .commit_to_gitgraph = true,      // Persist research sessions
                .use_graph_kv_injection = true,  // Use existing knowledge
                .use_trm_context = true,         // Ground in memory
                .use_hrm_decomposition = false,  // Research has its own decomposition
                .enable_branching = true,        // KEY: Branch/merge dynamics
                .preserve_tensions = true        // KEY: Keep contradictions
            };
            
        case Mode::CODE:
            return {
                .temperature = 0.2f,             // Low temp for precision
                .penalty_repeat = 1.05f,
                .penalty_last_n = 128,
                .commit_facts_to_graph = true,   // Code patterns worth remembering
                .commit_to_gitgraph = true,
                .use_graph_kv_injection = true,
                .use_trm_context = true,         // Use code context
                .use_hrm_decomposition = false,  // Code has explicit structure
                .enable_branching = false,
                .preserve_tensions = false
            };
            
        case Mode::DREAM:
            return {
                .temperature = 0.9f,             // High temp for association
                .penalty_repeat = 1.0f,          // No penalty - free flow
                .penalty_last_n = 64,
                .commit_facts_to_graph = false,  // Lucid check gates commits
                .commit_to_gitgraph = true,      // Persist dreams for review
                .use_graph_kv_injection = false, // Dream from scratch
                .use_trm_context = true,         // Replay memories
                .use_hrm_decomposition = false,
                .enable_branching = false,
                .preserve_tensions = false
            };
    }
    return get_policy(Mode::CHAT);  // Default
}

// ============================================================================
// RESEARCH SESSION STATE
// Persists across requests while in RESEARCH mode
// ============================================================================

struct ResearchSession {
    std::string session_id;
    std::string topic;
    zeta_research::ResearchState state;
    std::chrono::steady_clock::time_point started;
    std::chrono::steady_clock::time_point last_activity;
    int iteration_count = 0;
    bool active = false;
    
    // GitGraph persistence paths
    std::string gitgraph_branch;     // e.g., "research/quantum_computing_20260103"
    std::string session_file;        // e.g., "/mnt/GitGraph/research/session_xxx.json"
};

// ============================================================================
// MODE CONTROLLER
// Central coordinator for mode transitions and state management
// ============================================================================

class ModeController {
private:
    Mode current_mode_ = Mode::CHAT;
    ModePolicy current_policy_;
    std::mutex mode_mutex_;
    
    // Research session (persists across requests)
    ResearchSession research_session_;
    
    // GitGraph integration
    std::string gitgraph_root_ = "/mnt/GitGraph";
    
    // Generation callback (set by server)
    std::function<std::string(const std::string&, float temp, int max_tokens)> generate_fn_;
    
    // Mode transition hooks
    std::function<void(Mode, Mode)> on_mode_change_;
    
    // Session timeout
    static constexpr int RESEARCH_SESSION_TIMEOUT_SEC = 1800;  // 30 min idle = end session

public:
    ModeController() : current_policy_(get_policy(Mode::CHAT)) {}
    
    // ========================================================================
    // MODE QUERIES
    // ========================================================================
    
    Mode current_mode() const { return current_mode_; }
    const ModePolicy& policy() const { return current_policy_; }
    
    bool is_research_active() const { 
        return current_mode_ == Mode::RESEARCH && research_session_.active; 
    }
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void set_generate_fn(std::function<std::string(const std::string&, float, int)> fn) {
        generate_fn_ = fn;
    }
    
    void set_gitgraph_root(const std::string& root) {
        gitgraph_root_ = root;
    }
    
    void set_mode_change_callback(std::function<void(Mode, Mode)> cb) {
        on_mode_change_ = cb;
    }
    
    // ========================================================================
    // MODE DETECTION (from query classification)
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
        
        // Research continuation (if already in research session)
        if (is_research_active()) {
            // Check for continuation signals
            if (lower.find("continue") != std::string::npos ||
                lower.find("what else") != std::string::npos ||
                lower.find("explore further") != std::string::npos ||
                lower.find("dig deeper") != std::string::npos) {
                return Mode::RESEARCH;
            }
        }
        
        // Default to current mode (sticky)
        return current_mode_;
    }
    
    // ========================================================================
    // MODE TRANSITIONS
    // ========================================================================
    
    bool switch_mode(Mode new_mode, const std::string& context = "") {
        std::lock_guard<std::mutex> lock(mode_mutex_);
        
        if (new_mode == current_mode_) {
            return true;  // No change needed
        }
        
        Mode old_mode = current_mode_;
        
        // Exit current mode
        exit_mode(old_mode);
        
        // Enter new mode
        enter_mode(new_mode, context);
        
        current_mode_ = new_mode;
        current_policy_ = get_policy(new_mode);
        
        // Callback
        if (on_mode_change_) {
            on_mode_change_(old_mode, new_mode);
        }
        
        fprintf(stderr, "[MODE] Switched: %s -> %s\n", 
                mode_name(old_mode), mode_name(new_mode));
        
        return true;
    }
    
private:
    void exit_mode(Mode mode) {
        switch (mode) {
            case Mode::RESEARCH:
                // Save research session state to GitGraph
                if (research_session_.active) {
                    save_research_session();
                    research_session_.active = false;
                }
                break;
                
            case Mode::CREATIVE:
                // Nothing special to clean up
                break;
                
            default:
                break;
        }
    }
    
    void enter_mode(Mode mode, const std::string& context) {
        switch (mode) {
            case Mode::RESEARCH:
                // Initialize or resume research session
                if (!research_session_.active || !context.empty()) {
                    init_research_session(context);
                }
                break;
                
            case Mode::CREATIVE:
                // Clear any factual context injection flags
                break;
                
            default:
                break;
        }
    }
    
    // ========================================================================
    // RESEARCH SESSION MANAGEMENT
    // ========================================================================
    
    void init_research_session(const std::string& topic) {
        research_session_.session_id = generate_session_id();
        research_session_.topic = topic.empty() ? "general_inquiry" : topic;
        research_session_.started = std::chrono::steady_clock::now();
        research_session_.last_activity = research_session_.started;
        research_session_.iteration_count = 0;
        research_session_.active = true;
        
        // Initialize research state
        research_session_.state = zeta_research::ResearchState();
        research_session_.state.generate_fn = generate_fn_;
        zeta_research::initialize_state(research_session_.state, topic);
        
        // Create GitGraph branch for this session
        research_session_.gitgraph_branch = create_research_branch(topic);
        research_session_.session_file = gitgraph_root_ + "/research/" + 
                                         research_session_.session_id + ".json";
        
        fprintf(stderr, "[RESEARCH] New session: %s\n", research_session_.session_id.c_str());
        fprintf(stderr, "[RESEARCH] Topic: %s\n", research_session_.topic.c_str());
        fprintf(stderr, "[RESEARCH] GitGraph branch: %s\n", research_session_.gitgraph_branch.c_str());
    }
    
    std::string generate_session_id() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&time));
        return std::string("research_") + buf;
    }
    
    std::string create_research_branch(const std::string& topic) {
        // Sanitize topic for branch name
        std::string branch = "research/";
        for (char c : topic) {
            if (isalnum(c)) branch += tolower(c);
            else if (c == ' ') branch += '_';
        }
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        strftime(buf, sizeof(buf), "_%Y%m%d", localtime(&time));
        branch += buf;
        
        // TODO: Actually create git branch via system call or libgit2
        // For now, just return the name
        return branch;
    }
    
    void save_research_session() {
        if (!research_session_.active) return;
        
        // Serialize research state to JSON
        std::string json = serialize_research_state();
        
        // Write to session file
        std::ofstream f(research_session_.session_file);
        if (f.is_open()) {
            f << json;
            f.close();
            fprintf(stderr, "[RESEARCH] Session saved: %s\n", 
                    research_session_.session_file.c_str());
        }
        
        // TODO: Commit to GitGraph branch
    }
    
    std::string serialize_research_state() {
        const auto& state = research_session_.state;
        
        std::string json = "{\n";
        json += "  \"session_id\": \"" + research_session_.session_id + "\",\n";
        json += "  \"topic\": \"" + research_session_.topic + "\",\n";
        json += "  \"iteration_count\": " + std::to_string(research_session_.iteration_count) + ",\n";
        json += "  \"global_entropy\": " + std::to_string(state.global_entropy) + ",\n";
        json += "  \"branches_count\": " + std::to_string(state.active_branches.size()) + ",\n";
        json += "  \"tensions_count\": " + std::to_string(state.tensions.size()) + ",\n";
        json += "  \"summaries_count\": " + std::to_string(state.buffer.summaries.size()) + ",\n";
        
        // Serialize branches
        json += "  \"branches\": [\n";
        for (size_t i = 0; i < state.active_branches.size(); ++i) {
            const auto& b = state.active_branches[i];
            json += "    {\"id\": \"" + b.id + "\", ";
            json += "\"concepts\": " + std::to_string(b.concepts.size()) + ", ";
            json += "\"entropy\": " + std::to_string(b.entropy) + ", ";
            json += "\"compressible\": " + std::string(b.compressible ? "true" : "false") + "}";
            if (i < state.active_branches.size() - 1) json += ",";
            json += "\n";
        }
        json += "  ],\n";
        
        // Serialize unresolved tensions
        json += "  \"unresolved_tensions\": [\n";
        bool first = true;
        for (const auto& t : state.tensions) {
            if (!t.resolved) {
                if (!first) json += ",\n";
                first = false;
                json += "    {\"id\": \"" + t.id + "\", ";
                json += "\"description\": \"" + escape_json(t.description.substr(0, 200)) + "\", ";
                json += "\"severity\": " + std::to_string(t.severity) + "}";
            }
        }
        json += "\n  ],\n";
        
        // Serialize validated summaries
        json += "  \"validated_summaries\": [\n";
        for (size_t i = 0; i < state.buffer.summaries.size(); ++i) {
            const auto& s = state.buffer.summaries[i];
            if (s.is_valid()) {
                json += "    {\"text\": \"" + escape_json(s.text.substr(0, 500)) + "\", ";
                json += "\"entropy_reduction\": " + std::to_string(s.entropy_reduction) + "}";
                if (i < state.buffer.summaries.size() - 1) json += ",";
                json += "\n";
            }
        }
        json += "  ]\n";
        
        json += "}\n";
        return json;
    }
    
    static std::string escape_json(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else result += c;
        }
        return result;
    }
    
public:
    // ========================================================================
    // RESEARCH MODE API
    // ========================================================================
    
    // Run one iteration of research exploration
    std::string research_iterate() {
        if (!is_research_active()) {
            return "Error: No active research session";
        }
        
        research_session_.last_activity = std::chrono::steady_clock::now();
        research_session_.iteration_count++;
        
        auto& state = research_session_.state;
        
        fprintf(stderr, "[RESEARCH] Iteration %d\n", research_session_.iteration_count);
        
        // Run one iteration of the research loop
        zeta_research::explore(state);
        zeta_research::adjust_temperature(state);
        zeta_research::compress_branches(state);
        zeta_research::detect_and_record_tensions(state);
        zeta_research::merge_branches(state);
        
        // Build status report
        std::string status = "**Research Iteration " + 
                             std::to_string(research_session_.iteration_count) + "**\n\n";
        status += "Branches: " + std::to_string(state.active_branches.size()) + "\n";
        status += "Global Entropy: " + std::to_string(state.global_entropy) + "\n";
        status += "Unresolved Tensions: " + std::to_string(
            std::count_if(state.tensions.begin(), state.tensions.end(),
                          [](const auto& t) { return !t.resolved; })) + "\n";
        status += "Validated Summaries: " + std::to_string(state.buffer.summaries.size()) + "\n";
        
        return status;
    }
    
    // Conclude research and produce final output
    std::string research_conclude() {
        if (!is_research_active()) {
            return "Error: No active research session";
        }
        
        auto& state = research_session_.state;
        
        // Run final compression and conclusion
        zeta_research::summarize_merges(state);
        
        std::string output;
        zeta_research::produce_conclusion(state, output);
        
        // Save session before closing
        save_research_session();
        
        // Commit validated summaries to graph (if policy allows)
        if (current_policy_.commit_facts_to_graph) {
            commit_validated_summaries_to_graph();
        }
        
        return output;
    }
    
    // Get current research state for display
    std::string research_status() {
        if (!is_research_active()) {
            return "No active research session";
        }
        
        return serialize_research_state();
    }
    
private:
    void commit_validated_summaries_to_graph() {
        // TODO: Extract facts from validated summaries and commit to knowledge graph
        // This is where research findings become persistent knowledge
        fprintf(stderr, "[RESEARCH] Would commit %zu validated summaries to graph\n",
                research_session_.state.buffer.summaries.size());
    }
    
public:
    // ========================================================================
    // GRAPH-KV INTEGRATION
    // ========================================================================
    
    // Determine if KV injection should happen for current mode
    bool should_inject_kv() const {
        return current_policy_.use_graph_kv_injection;
    }
    
    // Determine if facts should commit to graph
    bool should_commit_facts() const {
        return current_policy_.commit_facts_to_graph;
    }
    
    // Determine if session should persist to GitGraph
    bool should_persist_to_gitgraph() const {
        return current_policy_.commit_to_gitgraph;
    }
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static ModeController g_mode_controller;

// ============================================================================
// CONVENIENCE MACROS for server integration
// ============================================================================

#define ZETA_MODE_CURRENT() g_mode_controller.current_mode()
#define ZETA_MODE_POLICY() g_mode_controller.policy()
#define ZETA_MODE_SWITCH(m, ctx) g_mode_controller.switch_mode(m, ctx)
#define ZETA_MODE_DETECT(q, qc) g_mode_controller.detect_mode(q, qc)

#define ZETA_SHOULD_COMMIT_FACTS() g_mode_controller.should_commit_facts()
#define ZETA_SHOULD_INJECT_KV() g_mode_controller.should_inject_kv()
#define ZETA_SHOULD_PERSIST() g_mode_controller.should_persist_to_gitgraph()

#define ZETA_RESEARCH_ITERATE() g_mode_controller.research_iterate()
#define ZETA_RESEARCH_CONCLUDE() g_mode_controller.research_conclude()
#define ZETA_RESEARCH_STATUS() g_mode_controller.research_status()
#define ZETA_RESEARCH_ACTIVE() g_mode_controller.is_research_active()

} // namespace zeta_modes

#endif // ZETA_MODE_CONTROLLER_H
