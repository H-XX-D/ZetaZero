// Z.E.T.A. Dream State Architecture
// ============================================================================
// Implements the "Dreaming" cognitive mode for memory consolidation
//
// CONCEPT:
//   AWAKE mode  = High precision, low temperature, strict stopping
//   DREAM mode  = High temperature, free association, pattern discovery
//
// The Dream Cycle runs during idle time to:
//   1. Replay recent memories from graph/scratch
//   2. Let the model "reflect" with high-temp free association
//   3. LUCID CHECK: Self-validate if dream is useful (YES/NO)
//   4. Save validated dreams to /dreams/pending/
//   5. Morning Briefing: Display insights on next user interaction
//
// Directory Structure:
//   /mnt/HoloGit/dreams/pending/    - New dreams awaiting review
//   /mnt/HoloGit/dreams/archive/    - Reviewed dreams
//   /mnt/HoloGit/dreams/scratch.txt - Raw session log
// ============================================================================

#ifndef ZETA_DREAM_H
#define ZETA_DREAM_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include "zeta-dual-process.h"

// ============================================================================
// System State
// ============================================================================

typedef enum {
    ZETA_STATE_AWAKE,      // Active user interaction - high precision
    ZETA_STATE_DREAMING,   // Idle consolidation - free association
    ZETA_STATE_LUCID       // Dream review - extracting useful patterns
} zeta_system_state_t;

// ============================================================================
// Dream Configuration
// ============================================================================

typedef struct {
    int idle_threshold_sec;        // Idle time before dreaming (default: 300s)
    float dream_temp;              // High temp for creativity (default: 0.9)
    float dream_penalty_repeat;    // Low penalty in dreams (default: 1.0)
    int max_dream_iterations;      // Max dream cycles (default: 5)
    int max_dream_tokens;          // Tokens per dream (default: 512)
    float compression_confidence;  // Min confidence to save (default: 0.7)
    std::string dreams_dir;        // Base directory for dreams
    int plateau_threshold;         // Consecutive discards before graph jump (default: 3)
} zeta_dream_config_t;

// Default dream configuration
static zeta_dream_config_t g_dream_config = {
    .idle_threshold_sec = 60,      // Reduced to 60s for easier testing/stressing
    .dream_temp = 0.9f,
    .dream_penalty_repeat = 1.0f,
    .max_dream_iterations = 5,
    .max_dream_tokens = 512,
    .compression_confidence = 0.7f,
    .dreams_dir = "/mnt/HoloGit/dreams",
    .plateau_threshold = 3         // Jump to random graph node after 3 discards
};

// ============================================================================
// Dream Entry Structure
// ============================================================================

struct ZetaDreamEntry {
    std::string content;
    std::string category;    // "code_fix", "idea", "insight", "story"
    std::string timestamp;
    float confidence;
    bool reviewed;
};

// ============================================================================
// DREAM REPETITION PENALTY: Prevent fixation on same ideas
// ============================================================================
// Tracks recent dream themes and penalizes repetitive content

class DreamRepetitionTracker {
public:
    struct ThemeEntry {
        std::string theme;
        int count;
        time_t last_seen;
    };

    // Configuration
    int max_theme_history = 100;          // Max themes to track
    int repetition_threshold = 3;         // After 3 occurrences, heavily penalize
    float novelty_weight = 0.4f;          // Weight of novelty in final score
    int theme_decay_hours = 24;           // Themes decay after 24 hours

private:
    std::map<std::string, ThemeEntry> theme_history;
    std::vector<std::string> recent_dream_hashes;  // MD5-style dedup
    std::mutex tracker_mutex;

    // Extract key themes (words 4+ chars that appear meaningful)
    std::vector<std::string> extract_themes(const std::string& content) {
        std::vector<std::string> themes;
        std::string lower = content;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Key domain terms to track
        static const std::vector<std::string> domain_terms = {
            "router", "routing", "cache", "caching", "model", "14b", "7b", "4b",
            "parallel", "load", "balancing", "feedback", "context", "embedding",
            "hrm", "trm", "dream", "memory", "graph", "node", "edge", "query",
            "complexity", "efficiency", "optimization", "performance", "latency",
            "fallback", "hybrid", "selector", "profiling", "metrics", "dynamic"
        };

        for (const auto& term : domain_terms) {
            if (lower.find(term) != std::string::npos) {
                themes.push_back(term);
            }
        }

        return themes;
    }

    // Simple content hash for exact duplicate detection
    std::string content_hash(const std::string& content) {
        // Simple hash: first 20 chars + length + last 20 chars
        std::string hash;
        if (content.length() >= 40) {
            hash = content.substr(0, 20) + std::to_string(content.length()) +
                   content.substr(content.length() - 20);
        } else {
            hash = content + std::to_string(content.length());
        }
        return hash;
    }

public:
    // Record themes from a new dream
    void record_dream(const std::string& content) {
        std::lock_guard<std::mutex> lock(tracker_mutex);

        // Record hash for exact duplicate detection
        std::string hash = content_hash(content);
        recent_dream_hashes.push_back(hash);
        if (recent_dream_hashes.size() > 50) {
            recent_dream_hashes.erase(recent_dream_hashes.begin());
        }

        // Extract and record themes
        auto themes = extract_themes(content);
        time_t now = time(NULL);

        for (const auto& theme : themes) {
            if (theme_history.find(theme) != theme_history.end()) {
                theme_history[theme].count++;
                theme_history[theme].last_seen = now;
            } else {
                theme_history[theme] = {theme, 1, now};
            }
        }

        // Prune old themes
        prune_old_themes();

        fprintf(stderr, "[DREAM-REP] Recorded dream with %zu themes. Total tracked: %zu\n",
                themes.size(), theme_history.size());
    }

    // Calculate novelty score (0.0 = very repetitive, 1.0 = very novel)
    float calculate_novelty(const std::string& content) {
        std::lock_guard<std::mutex> lock(tracker_mutex);

        // Check for exact duplicate
        std::string hash = content_hash(content);
        for (const auto& h : recent_dream_hashes) {
            if (h == hash) {
                fprintf(stderr, "[DREAM-REP] Exact duplicate detected!\n");
                return 0.0f;  // Exact duplicate
            }
        }

        // Calculate theme overlap penalty
        auto themes = extract_themes(content);
        if (themes.empty()) return 1.0f;  // No themes = novel by default

        float total_penalty = 0.0f;
        int theme_count = 0;

        for (const auto& theme : themes) {
            if (theme_history.find(theme) != theme_history.end()) {
                int occurrences = theme_history[theme].count;

                // Exponential penalty for repetition
                if (occurrences >= repetition_threshold) {
                    total_penalty += 0.3f * (occurrences - repetition_threshold + 1);
                } else {
                    total_penalty += 0.1f * occurrences;
                }
                theme_count++;
            }
        }

        if (theme_count == 0) return 1.0f;

        float avg_penalty = total_penalty / theme_count;
        float novelty = std::max(0.0f, 1.0f - avg_penalty);

        fprintf(stderr, "[DREAM-REP] Novelty score: %.2f (themes: %d, avg_penalty: %.2f)\n",
                novelty, theme_count, avg_penalty);

        return novelty;
    }

    // Check if dream should be rejected due to repetition
    bool is_too_repetitive(const std::string& content, float threshold = 0.3f) {
        float novelty = calculate_novelty(content);
        return novelty < threshold;
    }

    // Get themes to avoid in next dream prompt
    std::string get_avoidance_prompt() {
        std::lock_guard<std::mutex> lock(tracker_mutex);

        std::vector<std::pair<std::string, int>> overused;
        for (const auto& [theme, entry] : theme_history) {
            if (entry.count >= repetition_threshold) {
                overused.push_back({theme, entry.count});
            }
        }

        if (overused.empty()) return "";

        // Sort by count descending
        std::sort(overused.begin(), overused.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::stringstream ss;
        ss << "\n\nAVOID these overused topics (think of something NEW): ";
        for (size_t i = 0; i < std::min(overused.size(), (size_t)5); i++) {
            if (i > 0) ss << ", ";
            ss << overused[i].first;
        }
        ss << ".\nFocus on unexplored aspects of the codebase.";

        return ss.str();
    }

    // Get statistics
    std::string get_stats() {
        std::lock_guard<std::mutex> lock(tracker_mutex);

        std::stringstream ss;
        ss << "=== Dream Repetition Stats ===\n";
        ss << "Themes tracked: " << theme_history.size() << "\n";
        ss << "Recent hashes: " << recent_dream_hashes.size() << "\n\n";

        // Top repeated themes
        std::vector<std::pair<std::string, int>> sorted;
        for (const auto& [theme, entry] : theme_history) {
            sorted.push_back({theme, entry.count});
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        ss << "Top repeated themes:\n";
        for (size_t i = 0; i < std::min(sorted.size(), (size_t)10); i++) {
            ss << "  " << sorted[i].first << ": " << sorted[i].second << "x\n";
        }

        return ss.str();
    }

    // Clear all history (for fresh start)
    void clear() {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        theme_history.clear();
        recent_dream_hashes.clear();
        fprintf(stderr, "[DREAM-REP] Cleared all history\n");
    }

private:
    void prune_old_themes() {
        time_t now = time(NULL);
        time_t cutoff = now - (theme_decay_hours * 3600);

        auto it = theme_history.begin();
        while (it != theme_history.end()) {
            if (it->second.last_seen < cutoff) {
                it = theme_history.erase(it);
            } else {
                ++it;
            }
        }

        // Also limit total size
        while (theme_history.size() > (size_t)max_theme_history) {
            // Remove oldest
            auto oldest = theme_history.begin();
            for (auto it = theme_history.begin(); it != theme_history.end(); ++it) {
                if (it->second.last_seen < oldest->second.last_seen) {
                    oldest = it;
                }
            }
            theme_history.erase(oldest);
        }
    }
};

// Global repetition tracker
static DreamRepetitionTracker g_dream_repetition;

// ============================================================================
// Dream State Manager
// ============================================================================

class ZetaDreamState {
private:
    std::atomic<zeta_system_state_t> current_state{ZETA_STATE_AWAKE};
    std::atomic<time_t> last_activity{0};
    std::atomic<bool> dream_thread_running{false};
    std::atomic<int> total_dreams{0};
    std::atomic<int> session_dreams{0};
    std::thread dream_thread;
    std::mutex dream_mutex;

    zeta_dual_ctx_t* ctx;
    zeta_dream_config_t config;

    // Callback for model generation (prompt, max_tokens, temp, penalty) -> response
    std::function<std::string(const std::string&, int, float, float)> generate_fn;

    // Pending dreams for morning briefing
    std::vector<ZetaDreamEntry> pending_dreams;

public:
    ZetaDreamState() : ctx(nullptr) {
        last_activity = time(NULL);
    }

    void init(zeta_dual_ctx_t* dual_ctx,
              std::function<std::string(const std::string&, int, float, float)> gen_fn) {
        ctx = dual_ctx;
        generate_fn = gen_fn;
        config = g_dream_config;

        // Ensure dream directories exist
        ensure_directories();

        // Load any pending dreams from previous session
        load_pending_dreams();

        fprintf(stderr, "[DREAM] Initialized. Pending dreams: %zu\n", pending_dreams.size());
    }

    void set_config(const zeta_dream_config_t& cfg) {
        config = cfg;
    }

    // Called on every user interaction
    void wake() {
        last_activity = time(NULL);
        if (current_state == ZETA_STATE_DREAMING) {
            fprintf(stderr, "[DREAM] Waking from dream (user activity)\n");
            current_state = ZETA_STATE_AWAKE;
        }
    }

    zeta_system_state_t get_state() const { return current_state.load(); }
    bool is_dreaming() const { return current_state != ZETA_STATE_AWAKE; }
    int get_pending_count() const { return (int)pending_dreams.size(); }
    int get_session_dreams() const { return session_dreams.load(); }

    // ========================================================================
    // MORNING BRIEFING: Display insights from overnight dreams
    // ========================================================================
    std::string get_morning_briefing() {
        if (pending_dreams.empty()) {
            return "";
        }

        std::stringstream ss;
        ss << "\n╔══════════════════════════════════════════════════════════════╗\n";
        ss << "║  [MORNING BRIEFING] I processed " << pending_dreams.size() << " insights while idle\n";
        ss << "╠══════════════════════════════════════════════════════════════╣\n";

        int shown = 0;
        for (const auto& dream : pending_dreams) {
            if (shown >= 3) break;  // Show top 3
            ss << "║  [" << dream.category << "] " << dream.content.substr(0, 60);
            if (dream.content.size() > 60) ss << "...";
            ss << "\n";
            shown++;
        }

        if (pending_dreams.size() > 3) {
            ss << "║  ... and " << (pending_dreams.size() - 3) << " more\n";
        }

        ss << "╚══════════════════════════════════════════════════════════════╝\n";
        return ss.str();
    }

    void clear_briefing() {
        // Move pending dreams to archive
        for (const auto& dream : pending_dreams) {
            archive_dream(dream);
        }
        pending_dreams.clear();
    }

    // ========================================================================
    // SCAN PENDING DREAMS: Load dreams from disk
    // ========================================================================
    std::vector<ZetaDreamEntry> scan_pending_dreams() {
        std::vector<ZetaDreamEntry> dreams;
        std::string pending_dir = config.dreams_dir + "/pending";

        DIR* dir = opendir(pending_dir.c_str());
        if (!dir) return dreams;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') continue;

            std::string filepath = pending_dir + "/" + entry->d_name;
            std::ifstream file(filepath);
            if (file.is_open()) {
                ZetaDreamEntry dream;
                std::getline(file, dream.category);
                std::getline(file, dream.timestamp);

                std::stringstream content;
                content << file.rdbuf();
                dream.content = content.str();
                dream.reviewed = false;

                dreams.push_back(dream);
            }
        }
        closedir(dir);

        return dreams;
    }

    // ========================================================================
    // DREAM THREAD
    // ========================================================================
    void start_dream_thread() {
        if (dream_thread_running) return;

        dream_thread_running = true;
        dream_thread = std::thread([this]() {
            fprintf(stderr, "[DREAM] Background thread started\n");

            while (dream_thread_running) {
                std::this_thread::sleep_for(std::chrono::seconds(30));

                if (should_dream() && current_state == ZETA_STATE_AWAKE) {
                    run_dream_cycle();
                }
            }

            fprintf(stderr, "[DREAM] Background thread stopped\n");
        });
    }

    void stop_dream_thread() {
        dream_thread_running = false;
        if (dream_thread.joinable()) {
            dream_thread.join();
        }
    }

private:
    bool should_dream() const {
        if (!ctx || !generate_fn) return false;
        time_t idle_time = time(NULL) - last_activity.load();
        return idle_time >= config.idle_threshold_sec;
    }

    void ensure_directories() {
        mkdir(config.dreams_dir.c_str(), 0755);
        mkdir((config.dreams_dir + "/pending").c_str(), 0755);
        mkdir((config.dreams_dir + "/archive").c_str(), 0755);
    }

    void load_pending_dreams() {
        pending_dreams = scan_pending_dreams();
    }

    // ========================================================================
    // THE DREAM CYCLE
    // ========================================================================
    void run_dream_cycle() {
        std::lock_guard<std::mutex> lock(dream_mutex);

        fprintf(stderr, "\n[DREAM] ════════════════════════════════════════\n");
        fprintf(stderr, "[DREAM] Entering Dream State...\n");
        fprintf(stderr, "[DREAM] ════════════════════════════════════════\n");

        current_state = ZETA_STATE_DREAMING;

        // Phase 1: Gather context and generate dream
        std::string context = gather_recent_memories();
        if (context.empty()) {
            fprintf(stderr, "[DREAM] No recent memories to process\n");
            current_state = ZETA_STATE_AWAKE;
            return;
        }

        // Phase 2: Free association with high temperature
        // Get avoidance prompt from repetition tracker
        std::string avoidance = g_dream_repetition.get_avoidance_prompt();

        int consecutive_discards = 0;  // Track plateau
        bool using_graph_jump = false;

        // DEEP DIVE: When we find a novel theme, drill down to code
        std::string current_theme = "";
        int drill_depth = 0;  // 0=discover, 1=framework, 2=implementation, 3=code
        std::string last_saved_dream = "";

        // Drilling prompts - each level gets more concrete
        const char* drill_prompts[] = {
            "Generate ONE useful insight or creative idea. Be specific and NOVEL.",  // Level 0: Discovery
            "Take this concept and design a concrete FRAMEWORK or ARCHITECTURE. Include specific components, data flows, and interfaces.",  // Level 1: Framework
            "Now create a detailed IMPLEMENTATION PLAN with specific functions, structs, and algorithms. Be technical. Target C++.",  // Level 2: Implementation
            "Write actual C++ CODE. Include function signatures, structs, and core logic. Use modern C++ (C++17/20). No Python."  // Level 3: Code
        };

        for (int i = 0; i < config.max_dream_iterations && current_state == ZETA_STATE_DREAMING; i++) {
            const char* mode_str = using_graph_jump ? "GRAPH JUMP" :
                                   (drill_depth > 0 ? "DEEP DIVE" : "");
            fprintf(stderr, "[DREAM] Iteration %d/%d %s (depth=%d)\n",
                    i+1, config.max_dream_iterations, mode_str, drill_depth);

            // Check if we've hit plateau - switch to random graph node
            if (consecutive_discards >= config.plateau_threshold) {
                fprintf(stderr, "[DREAM] Plateau detected (%d discards) - jumping to random graph node\n",
                        consecutive_discards);
                context = get_random_graph_context();
                if (context.empty()) {
                    context = gather_recent_memories();  // Fallback
                } else {
                    using_graph_jump = true;
                    // Reset deep dive state for new context
                    current_theme = "";
                    drill_depth = 0;
                    last_saved_dream = "";
                }
                consecutive_discards = 0;
            }

            // Build prompt based on drill depth
            std::string dream_prompt;
            if (drill_depth > 0 && !last_saved_dream.empty()) {
                // Deep dive mode - build on previous insight
                dream_prompt =
                    "You are in a DEEP DIVE dream state, drilling down on a specific idea.\n\n"
                    "PREVIOUS INSIGHT:\n" + last_saved_dream.substr(0, 500) + "\n\n"
                    "TASK: " + std::string(drill_prompts[std::min(drill_depth, 3)]) + "\n\n"
                    "Build DIRECTLY on the previous insight. Make it MORE CONCRETE and ACTIONABLE." +
                    avoidance;
            } else {
                // Discovery mode - free association
                dream_prompt =
                    "You are in a reflective dream state. Review these recent interactions:\n\n" +
                    context + "\n\n" +
                    std::string(drill_prompts[0]) + avoidance;
            }

            std::string dream = generate_fn(
                dream_prompt,
                config.max_dream_tokens,
                config.dream_temp,
                config.dream_penalty_repeat
            );

            if (dream.empty()) continue;

            // Phase 2.5: NOVELTY CHECK - Reject repetitive dreams
            float novelty = g_dream_repetition.calculate_novelty(dream);
            if (novelty < 0.3f) {
                fprintf(stderr, "[DREAM] Discarded (too repetitive, novelty=%.2f)\n", novelty);
                consecutive_discards++;

                // If we're in deep dive mode and plateau, reset to discovery
                if (drill_depth > 0 && consecutive_discards >= 2) {
                    fprintf(stderr, "[DREAM] Deep dive exhausted at depth %d - resetting\n", drill_depth);
                    drill_depth = 0;
                    last_saved_dream = "";
                    current_theme = "";
                }
                continue;
            }

            // Reset discard counter on success
            consecutive_discards = 0;

            // Phase 3: LUCID CHECK - Self-validate before saving
            // Skip lucid check during deep dive (we're deliberately drilling)
            current_state = ZETA_STATE_LUCID;
            bool passes_lucid = (drill_depth > 0) ? true : lucid_validate(dream);
            if (passes_lucid) {
                // Categorize and save
                std::string category = categorize_dream(dream);
                save_dream(dream, category);
                session_dreams++;
                total_dreams++;

                // Record in repetition tracker
                g_dream_repetition.record_dream(dream);

                // DEEP DIVE: Drill down on actionable categories (insight, code_idea, code_fix)
                // or any dream with decent novelty
                bool is_drillable = (category == "insight" || category == "code_idea" ||
                                    category == "code_fix" || novelty >= 0.5f);

                if (is_drillable && drill_depth < 3) {
                    last_saved_dream = dream;
                    drill_depth++;
                    fprintf(stderr, "[DREAM] Saved: [%s] novelty=%.2f -> DRILLING to depth %d\n",
                            category.c_str(), novelty, drill_depth);
                } else if (drill_depth >= 3) {
                    // Reached code level - reset for new discovery
                    fprintf(stderr, "[DREAM] Saved: [%s] novelty=%.2f -> CODE REACHED, resetting\n",
                            category.c_str(), novelty);
                    drill_depth = 0;
                    last_saved_dream = "";
                    current_theme = "";
                } else {
                    // Story or low novelty - just save and continue
                    fprintf(stderr, "[DREAM] Saved: [%s] novelty=%.2f %s...\n",
                            category.c_str(), novelty, dream.substr(0, 50).c_str());
                    // Reset drill state for fresh discovery
                    if (drill_depth > 0) {
                        drill_depth = 0;
                        last_saved_dream = "";
                    }
                }
            } else {
                fprintf(stderr, "[DREAM] Discarded (failed lucid check)\n");
            }

            current_state = ZETA_STATE_DREAMING;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        current_state = ZETA_STATE_AWAKE;
        fprintf(stderr, "[DREAM] Cycle complete. Session dreams: %d\n", session_dreams.load());
    }

    // ========================================================================
    // LUCID VALIDATION: Ask model if dream is useful
    // ========================================================================
    bool lucid_validate(const std::string& dream) {
        if (!generate_fn) return true;  // Skip if no generator

        std::string prompt =
            "Evaluate this generated insight:\n\n" + dream + "\n\n"
            "Is this useful, specific, and actionable? Answer only YES or NO:";

        std::string response = generate_fn(prompt, 10, 0.1f, 1.2f);  // Low temp for deterministic

        // Check if response contains YES
        std::string lower = response;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower.find("yes") != std::string::npos;
    }

    // ========================================================================
    // CATEGORIZE DREAM
    // ========================================================================
    std::string categorize_dream(const std::string& dream) {
        std::string lower = dream;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("bug") != std::string::npos ||
            lower.find("fix") != std::string::npos ||
            lower.find("error") != std::string::npos) {
            return "code_fix";
        }
        if (lower.find("function") != std::string::npos ||
            lower.find("def ") != std::string::npos ||
            lower.find("class ") != std::string::npos) {
            return "code_idea";
        }
        if (lower.find("story") != std::string::npos ||
            lower.find("character") != std::string::npos) {
            return "story";
        }
        return "insight";
    }

    // ========================================================================
    // SAVE DREAM TO PENDING
    // ========================================================================
    void save_dream(const std::string& content, const std::string& category) {
        time_t now = time(NULL);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));

        std::string filename = config.dreams_dir + "/pending/dream_" +
                              std::string(timestamp) + "_" + category + ".txt";

        std::ofstream file(filename);
        if (file.is_open()) {
            file << category << "\n";
            file << timestamp << "\n";
            file << content << "\n";
            file.close();

            // Also add to in-memory list
            ZetaDreamEntry entry;
            entry.content = content;
            entry.category = category;
            entry.timestamp = timestamp;
            entry.reviewed = false;
            pending_dreams.push_back(entry);
        }
    }

    // ========================================================================
    // ARCHIVE REVIEWED DREAM
    // ========================================================================
    void archive_dream(const ZetaDreamEntry& dream) {
        time_t now = time(NULL);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d", localtime(&now));

        std::string filename = config.dreams_dir + "/archive/reviewed_" +
                              std::string(timestamp) + "_" + dream.category + ".txt";

        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << "\n--- " << dream.timestamp << " [" << dream.category << "] ---\n";
            file << dream.content << "\n";
            file.close();
        }
    }

    // ========================================================================
    // GATHER RECENT MEMORIES
    // ========================================================================
    std::string gather_recent_memories() {
        std::string memories = "";

        if (ctx) {
            int recent_count = std::min(ctx->num_nodes, 20);
            for (int i = ctx->num_nodes - recent_count; i < ctx->num_nodes; i++) {
                if (i >= 0) {
                    memories += "- [" + std::string(ctx->nodes[i].label) + "]: " +
                               std::string(ctx->nodes[i].value).substr(0, 100) + "\n";
                }
            }
        }

        return memories;
    }

    // ========================================================================
    // RANDOM GRAPH NODE: Jump to distant part of graph when plateau detected
    // ========================================================================
    std::string get_random_graph_context() {
        if (!ctx || ctx->num_nodes == 0) return "";

        // Pick a random node from the ENTIRE graph (not just recent)
        int random_idx = rand() % ctx->num_nodes;
        zeta_graph_node_t* seed_node = &ctx->nodes[random_idx];

        std::string context = "GRAPH JUMP: Exploring a different part of your knowledge:\n\n";
        context += "SEED NODE: [" + std::string(seed_node->label) + "]: " +
                   std::string(seed_node->value) + "\n\n";

        // Find connected nodes via edges (edges stored separately)
        context += "RELATED NODES:\n";
        int neighbor_count = 0;
        for (int e = 0; e < ctx->num_edges && neighbor_count < 5; e++) {
            zeta_graph_edge_t* edge = &ctx->edges[e];
            int64_t neighbor_id = -1;

            if (edge->source_id == seed_node->node_id) {
                neighbor_id = edge->target_id;
            } else if (edge->target_id == seed_node->node_id) {
                neighbor_id = edge->source_id;
            }

            if (neighbor_id >= 0) {
                // Find the neighbor node
                for (int i = 0; i < ctx->num_nodes; i++) {
                    if (ctx->nodes[i].node_id == neighbor_id && ctx->nodes[i].is_active) {
                        context += "- [" + std::string(ctx->nodes[i].label) + "]: " +
                                   std::string(ctx->nodes[i].value).substr(0, 80) + "\n";
                        neighbor_count++;
                        break;
                    }
                }
            }
        }

        // If no edges found, grab random distant nodes for diversity
        if (neighbor_count == 0) {
            for (int j = 0; j < 3 && ctx->num_nodes > 1; j++) {
                int idx = rand() % ctx->num_nodes;
                if (idx != random_idx && ctx->nodes[idx].is_active) {
                    context += "- [" + std::string(ctx->nodes[idx].label) + "]: " +
                               std::string(ctx->nodes[idx].value).substr(0, 80) + "\n";
                }
            }
        }

        fprintf(stderr, "[DREAM] Graph jump to node %d: %s\n",
                random_idx, seed_node->label);

        return context;
    }

    // ========================================================================
    // DREAM SUGGESTION: Dream State Logging for Pattern Analysis
    // ========================================================================
    // Track patterns in dreams for self-improvement insights

    struct DreamLogEntry {
        time_t timestamp;
        std::string category;
        std::string prompt_context;
        std::string dream_output;
        bool passed_lucid;
        float generation_temp;
        int iteration;
    };

    std::vector<DreamLogEntry> dream_log;

    void log_dream_attempt(const std::string& category,
                           const std::string& prompt,
                           const std::string& output,
                           bool passed_lucid,
                           float temp,
                           int iteration) {
        DreamLogEntry entry;
        entry.timestamp = time(NULL);
        entry.category = category;
        entry.prompt_context = prompt.substr(0, 200);
        entry.dream_output = output.substr(0, 500);
        entry.passed_lucid = passed_lucid;
        entry.generation_temp = temp;
        entry.iteration = iteration;
        dream_log.push_back(entry);

        // Keep log bounded
        if (dream_log.size() > 100) {
            dream_log.erase(dream_log.begin());
        }
    }

    // Analyze dream patterns - what categories succeed most?
    std::string analyze_dream_patterns() {
        if (dream_log.empty()) return "No dream data yet.\n";

        std::map<std::string, int> category_attempts;
        std::map<std::string, int> category_successes;
        float total_temp = 0;
        int lucid_passes = 0;

        for (const auto& entry : dream_log) {
            category_attempts[entry.category]++;
            if (entry.passed_lucid) {
                category_successes[entry.category]++;
                lucid_passes++;
            }
            total_temp += entry.generation_temp;
        }

        std::stringstream ss;
        ss << "=== DREAM PATTERN ANALYSIS ===\n";
        ss << "Total dreams: " << dream_log.size() << "\n";
        ss << "Lucid passes: " << lucid_passes << " ("
           << (100.0f * lucid_passes / dream_log.size()) << "%)\n";
        ss << "Avg temperature: " << (total_temp / dream_log.size()) << "\n";
        ss << "\nCategory breakdown:\n";

        for (const auto& [cat, attempts] : category_attempts) {
            int successes = category_successes[cat];
            ss << "  " << cat << ": " << successes << "/" << attempts
               << " (" << (100.0f * successes / attempts) << "% success)\n";
        }

        return ss.str();
    }

    // Get dream log as JSON for external analysis
    std::string export_dream_log_json() {
        std::stringstream ss;
        ss << "[\n";
        bool first = true;
        for (const auto& entry : dream_log) {
            if (!first) ss << ",\n";
            first = false;
            ss << "  {\"timestamp\": " << entry.timestamp
               << ", \"category\": \"" << entry.category << "\""
               << ", \"passed\": " << (entry.passed_lucid ? "true" : "false")
               << ", \"temp\": " << entry.generation_temp
               << ", \"iteration\": " << entry.iteration
               << "}";
        }
        ss << "\n]";
        return ss.str();
    }

    // Save pattern analysis to file
    void save_pattern_analysis() {
        std::string analysis = analyze_dream_patterns();
        std::string filepath = config.dreams_dir + "/pattern_analysis.txt";

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << "Generated: " << time(NULL) << "\n\n";
            file << analysis;
            file.close();
            fprintf(stderr, "[DREAM] Saved pattern analysis to %s\n", filepath.c_str());
        }
    }

public:
    // ========================================================================
    // DREAM SUGGESTION: Sleep-Pruning Mechanism for Memory Consolidation
    // ========================================================================
    // Inspired by how the brain prunes less relevant connections during sleep
    // and strengthens important ones.

    struct PruningStats {
        int nodes_evaluated;
        int connections_pruned;
        int connections_strengthened;
        float avg_activation_before;
        float avg_activation_after;
        time_t last_prune_time;
    };

    PruningStats last_prune_stats = {0, 0, 0, 0.0f, 0.0f, 0};

    // Run sleep-pruning during dream state to consolidate memories
    PruningStats run_sleep_pruning(float prune_threshold = 0.1f,
                                    float strengthen_threshold = 0.7f,
                                    float strengthen_factor = 1.2f,
                                    float weaken_factor = 0.8f) {
        PruningStats stats = {0, 0, 0, 0.0f, 0.0f, time(NULL)};

        if (!ctx || ctx->num_nodes == 0) {
            fprintf(stderr, "[PRUNE] No graph context available\n");
            return stats;
        }

        fprintf(stderr, "\n[PRUNE] ════════════════════════════════════════\n");
        fprintf(stderr, "[PRUNE] Starting Sleep-Pruning Cycle\n");
        fprintf(stderr, "[PRUNE] Nodes: %d | Edges: %d\n", ctx->num_nodes, ctx->num_edges);
        fprintf(stderr, "[PRUNE] ════════════════════════════════════════\n");

        // Phase 1: Evaluate all nodes by recency and access frequency
        std::vector<float> node_importance(ctx->num_nodes, 0.0f);
        time_t now = time(NULL);
        float total_activation = 0.0f;

        // Build ID lookup map
        std::unordered_map<int64_t, int> node_id_to_idx;
        for (int i = 0; i < ctx->num_nodes; i++) {
            node_id_to_idx[ctx->nodes[i].node_id] = i;
        }

        for (int i = 0; i < ctx->num_nodes; i++) {
            stats.nodes_evaluated++;

            // Calculate importance based on:
            // 1. Recency (time decay)
            // 2. Edge count (connectivity)
            // 3. Access count (if tracked)

            float recency_score = 1.0f;
            // Use edge count as connectivity proxy
            int edge_count = 0;
            int64_t current_node_id = ctx->nodes[i].node_id;
            for (int j = 0; j < ctx->num_edges; j++) {
                if (ctx->edges[j].source_id == current_node_id || ctx->edges[j].target_id == current_node_id) {
                    edge_count++;
                }
            }

            float connectivity_score = (float)edge_count / std::max(1, ctx->num_edges) * 10.0f;
            node_importance[i] = (recency_score * 0.5f) + (connectivity_score * 0.5f);
            total_activation += node_importance[i];
        }

        stats.avg_activation_before = total_activation / std::max(1, ctx->num_nodes);

        // Phase 2: Prune weak connections (edges with low-importance endpoints)
        std::vector<bool> edges_to_keep(ctx->num_edges, true);

        for (int i = 0; i < ctx->num_edges; i++) {
            int64_t source_id = ctx->edges[i].source_id;
            int64_t target_id = ctx->edges[i].target_id;
            
            if (node_id_to_idx.find(source_id) == node_id_to_idx.end() || 
                node_id_to_idx.find(target_id) == node_id_to_idx.end()) continue;

            int from_idx = node_id_to_idx[source_id];
            int to_idx = node_id_to_idx[target_id];

            if (from_idx >= ctx->num_nodes || to_idx >= ctx->num_nodes) continue;

            float edge_importance = (node_importance[from_idx] + node_importance[to_idx]) / 2.0f;

            if (edge_importance < prune_threshold) {
                // Mark for pruning (weak connection)
                edges_to_keep[i] = false;
                stats.connections_pruned++;
                fprintf(stderr, "[PRUNE] Pruning edge %d->%d (importance: %.3f)\n",
                        from_idx, to_idx, edge_importance);
            } else if (edge_importance > strengthen_threshold) {
                // Strengthen important connections by boosting edge weight
                ctx->edges[i].weight *= strengthen_factor;
                stats.connections_strengthened++;
                fprintf(stderr, "[PRUNE] Strengthened edge %d->%d (importance: %.3f)\n",
                        from_idx, to_idx, edge_importance);
            } else {
                // Slight decay for moderate connections
                ctx->edges[i].weight *= weaken_factor;
            }
        }

        // Phase 3: Actually remove pruned edges (compact the edge array)
        int write_idx = 0;
        for (int i = 0; i < ctx->num_edges; i++) {
            if (edges_to_keep[i]) {
                if (write_idx != i) {
                    ctx->edges[write_idx] = ctx->edges[i];
                }
                write_idx++;
            }
        }
        ctx->num_edges = write_idx;

        // Recalculate final stats
        total_activation = 0.0f;
        for (int i = 0; i < ctx->num_nodes; i++) {
            total_activation += node_importance[i];
        }
        stats.avg_activation_after = total_activation / std::max(1, ctx->num_nodes);

        fprintf(stderr, "[PRUNE] ════════════════════════════════════════\n");
        fprintf(stderr, "[PRUNE] Complete: %d pruned, %d strengthened\n",
                stats.connections_pruned, stats.connections_strengthened);
        fprintf(stderr, "[PRUNE] Remaining edges: %d\n", ctx->num_edges);
        fprintf(stderr, "[PRUNE] ════════════════════════════════════════\n");

        last_prune_stats = stats;
        return stats;
    }

    // Get pruning statistics for logging
    std::string get_prune_stats() {
        std::stringstream ss;
        ss << "=== Sleep-Pruning Stats ===\n";
        ss << "Last run: " << last_prune_stats.last_prune_time << "\n";
        ss << "Nodes evaluated: " << last_prune_stats.nodes_evaluated << "\n";
        ss << "Connections pruned: " << last_prune_stats.connections_pruned << "\n";
        ss << "Connections strengthened: " << last_prune_stats.connections_strengthened << "\n";
        ss << "Avg activation before: " << last_prune_stats.avg_activation_before << "\n";
        ss << "Avg activation after: " << last_prune_stats.avg_activation_after << "\n";
        return ss.str();
    }

    // ========================================================================
    // DREAM SUGGESTION: Memory Consolidation Scheduler
    // ========================================================================
    // Runs during dream cycles to optimize memory structure

    struct ConsolidationConfig {
        bool enable_pruning;
        bool enable_compression;
        bool enable_pattern_detection;
        float prune_threshold;
        float compress_threshold;
        int consolidation_interval_sec;  // How often to consolidate
    };

    ConsolidationConfig consolidation_config = {
        .enable_pruning = true,
        .enable_compression = true,
        .enable_pattern_detection = true,
        .prune_threshold = 0.1f,
        .compress_threshold = 0.9f,  // Similarity threshold for merging
        .consolidation_interval_sec = 3600  // Every hour during dreams
    };

    time_t last_consolidation = 0;

    // Run full memory consolidation cycle
    void run_memory_consolidation() {
        time_t now = time(NULL);
        if (now - last_consolidation < consolidation_config.consolidation_interval_sec) {
            return;  // Not time yet
        }

        fprintf(stderr, "\n[CONSOLIDATE] ════════════════════════════════════════\n");
        fprintf(stderr, "[CONSOLIDATE] Starting Memory Consolidation Cycle\n");
        fprintf(stderr, "[CONSOLIDATE] ════════════════════════════════════════\n");

        // Step 1: Sleep-pruning
        if (consolidation_config.enable_pruning) {
            auto prune_stats = run_sleep_pruning(
                consolidation_config.prune_threshold,
                0.7f, 1.2f, 0.8f
            );
            fprintf(stderr, "[CONSOLIDATE] Pruning complete: %d removed, %d strengthened\n",
                    prune_stats.connections_pruned, prune_stats.connections_strengthened);
        }

        // Step 2: Pattern detection - find recurring themes
        if (consolidation_config.enable_pattern_detection) {
            auto patterns = detect_memory_patterns();
            fprintf(stderr, "[CONSOLIDATE] Detected %zu recurring patterns\n", patterns.size());

            // Save patterns to dream log
            for (const auto& pattern : patterns) {
                log_dream_attempt("pattern", pattern, pattern, true, 0.0f, 0);
            }
        }

        // Step 3: Save analysis
        save_pattern_analysis();

        last_consolidation = now;
        fprintf(stderr, "[CONSOLIDATE] Complete\n");
        fprintf(stderr, "[CONSOLIDATE] ════════════════════════════════════════\n");
    }

    // Detect recurring patterns in memory nodes
    std::vector<std::string> detect_memory_patterns() {
        std::vector<std::string> patterns;
        if (!ctx || ctx->num_nodes < 5) return patterns;

        // Simple pattern detection: find common substrings in node values
        std::map<std::string, int> word_frequency;

        for (int i = 0; i < ctx->num_nodes; i++) {
            std::string value = ctx->nodes[i].value;

            // Extract words (simple tokenization)
            std::string word;
            for (char c : value) {
                if (isalnum(c)) {
                    word += tolower(c);
                } else if (!word.empty()) {
                    if (word.length() > 3) {  // Skip short words
                        word_frequency[word]++;
                    }
                    word.clear();
                }
            }
            if (word.length() > 3) {
                word_frequency[word]++;
            }
        }

        // Find words that appear in multiple nodes
        for (const auto& [word, count] : word_frequency) {
            if (count >= 3) {  // Appears 3+ times
                patterns.push_back("Recurring theme: " + word + " (count: " + std::to_string(count) + ")");
            }
        }

        return patterns;
    }

    // ========================================================================
    // DREAM SUGGESTION: Cognitive State Integration
    // ========================================================================
    // Allow dream system to receive cognitive state from HRM

    float hrm_anxiety_level = 0.0f;
    std::string hrm_cognitive_state = "CALM";

    // Called by HRM to sync cognitive state
    void sync_cognitive_state(const std::string& state, float anxiety) {
        hrm_cognitive_state = state;
        hrm_anxiety_level = anxiety;

        // Adjust dream parameters based on cognitive state
        if (state == "ANXIOUS" || anxiety > 0.7f) {
            // Reduce dream intensity when system is anxious
            config.dream_temp = 0.7f;  // Lower temp for more focused dreams
            config.max_dream_iterations = 3;  // Fewer cycles
            fprintf(stderr, "[DREAM-SYNC] Anxious state: reduced dream intensity\n");
        } else if (state == "CREATIVE") {
            // Increase creativity in dream state
            config.dream_temp = 0.95f;  // Higher temp
            config.max_dream_iterations = 7;  // More exploration
            fprintf(stderr, "[DREAM-SYNC] Creative state: increased dream exploration\n");
        } else if (state == "FOCUSED") {
            // More targeted dreams
            config.dream_temp = 0.75f;
            config.max_dream_iterations = 4;
            fprintf(stderr, "[DREAM-SYNC] Focused state: targeted dream cycles\n");
        } else {
            // Reset to defaults
            config.dream_temp = g_dream_config.dream_temp;
            config.max_dream_iterations = g_dream_config.max_dream_iterations;
        }
    }

    std::string get_cognitive_sync_status() {
        std::stringstream ss;
        ss << "=== Dream-HRM Sync Status ===\n";
        ss << "HRM Cognitive State: " << hrm_cognitive_state << "\n";
        ss << "HRM Anxiety Level: " << hrm_anxiety_level << "\n";
        ss << "Dream Temperature: " << config.dream_temp << "\n";
        ss << "Dream Iterations: " << config.max_dream_iterations << "\n";
        return ss.str();
    }
};

// Global dream state manager
static ZetaDreamState g_dream_state;

// ============================================================================
// Convenience macros
// ============================================================================

#define ZETA_DREAM_WAKE() g_dream_state.wake()
#define ZETA_DREAM_IS_DREAMING() g_dream_state.is_dreaming()
#define ZETA_DREAM_STATE() g_dream_state.get_state()
#define ZETA_DREAM_BRIEFING() g_dream_state.get_morning_briefing()
#define ZETA_DREAM_PENDING() g_dream_state.get_pending_count()

#endif // ZETA_DREAM_H
