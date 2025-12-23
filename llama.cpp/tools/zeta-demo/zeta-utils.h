// Z.E.T.A. Utility Functions and Classes
// ============================================================================
// DREAM SUGGESTION IMPLEMENTATIONS:
//   - StringUtility class (074528)
//   - is_non_empty_string / is_valid_pointer helpers (074229)
//   - IS_VALID_CONTEXT macro (074153)
//   - is_context_valid helper (074407)
//   - custom_strlen with early termination (074306)
// ============================================================================

#ifndef ZETA_UTILS_H
#define ZETA_UTILS_H

#include <string>
#include <cstring>
#include <vector>
#include <functional>
#include <cassert>
#include <map>
#include <sstream>
#include <algorithm>
#include <mutex>

// ============================================================================
// DREAM SUGGESTION (074153): Validation Macros
// ============================================================================

#define IS_VALID_CONTEXT(ctx) \
    ((ctx) != nullptr && (ctx)->context != nullptr && strlen((ctx)->context) > 0)

#define IS_VALID_PTR(ptr) ((ptr) != nullptr)

#define IS_NON_EMPTY_STR(str) ((str) != nullptr && strlen(str) > 0)

// ============================================================================
// DREAM SUGGESTION (074306): Custom strlen with early termination
// ============================================================================

static inline size_t custom_strlen(const char* str) {
    if (str == nullptr) return 0;

    const char* p = str;
    while (*p) {
        if (*p == '\0') break;  // Early termination on null character
        p++;
    }
    return (size_t)(p - str);
}

// ============================================================================
// DREAM SUGGESTION (074229): Helper Functions
// ============================================================================

static inline bool is_non_empty_string(const char* str) {
    return str != nullptr && strlen(str) > 0;
}

static inline bool is_valid_pointer(const void* ptr) {
    return ptr != nullptr;
}

// C++ string version
static inline bool is_non_empty_string(const std::string& str) {
    return !str.empty();
}

// ============================================================================
// DREAM SUGGESTION (074407): Context Validation Helper
// ============================================================================

static inline bool is_context_valid(const char* context) {
    if (context == nullptr || strlen(context) == 0) {
        return false;
    }
    return true;
}

static inline bool is_context_valid(const std::string& context) {
    return !context.empty();
}

// ============================================================================
// DREAM SUGGESTION (074352/074528): StringUtility Class
// ============================================================================

class StringUtility {
public:
    // Get length of the string
    static size_t getLength(const char* str) {
        if (str == nullptr) return 0;
        return strlen(str);
    }

    static size_t getLength(const std::string& str) {
        return str.length();
    }

    // Check if string is non-empty
    static bool isNonEmpty(const char* str) {
        return getLength(str) > 0;
    }

    static bool isNonEmpty(const std::string& str) {
        return !str.empty();
    }

    // Validate if a string is valid (non-null, non-empty)
    static bool isValid(const char* str) {
        return str != nullptr && strlen(str) > 0;
    }

    static bool isValid(const std::string& str) {
        return !str.empty();
    }

    // Find index of substring within main string
    static int findIndex(const char* str, const char* substring) {
        if (str == nullptr || substring == nullptr) return -1;

        const char* found = strstr(str, substring);
        if (found == nullptr) return -1;

        return (int)(found - str);
    }

    static int findIndex(const std::string& str, const std::string& substring) {
        size_t pos = str.find(substring);
        if (pos == std::string::npos) return -1;
        return (int)pos;
    }

    // Trim whitespace from both ends
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";

        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    // Check if string starts with prefix
    static bool startsWith(const std::string& str, const std::string& prefix) {
        if (prefix.length() > str.length()) return false;
        return str.compare(0, prefix.length(), prefix) == 0;
    }

    // Check if string ends with suffix
    static bool endsWith(const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // Split string by delimiter
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;

        for (char c : str) {
            if (c == delimiter) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }

        if (!token.empty()) {
            tokens.push_back(token);
        }

        return tokens;
    }

    // Convert string to lowercase
    static std::string toLower(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }

    // Convert string to uppercase
    static std::string toUpper(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return result;
    }
};

// ============================================================================
// DREAM SUGGESTION (074426): Unified Context Interface
// ============================================================================

// Forward declarations for context types
struct zeta_dual_ctx_t;
struct fact_t;

// Abstract context wrapper
class ZetaContext {
public:
    enum class Type {
        DUAL_CTX,
        FACT,
        TRM,
        HRM,
        DREAM
    };

    virtual ~ZetaContext() = default;
    virtual Type getType() const = 0;
    virtual bool isValid() const = 0;
    virtual void* getData() = 0;
    virtual const void* getData() const = 0;
    virtual size_t getSize() const = 0;
};

// Context dispatch table for polymorphic operations
struct ZetaContextDispatch {
    std::function<bool(void*)> init_func;
    std::function<int(void*, const char*)> find_branch;
    std::function<bool(void*)> has_context;
    std::function<void(void*)> cleanup;
};

// Global dispatch tables (to be populated by specific modules)
// These allow dynamic dispatch to different context types
class ZetaContextRegistry {
public:
    static ZetaContextRegistry& instance() {
        static ZetaContextRegistry registry;
        return registry;
    }

    void registerDispatch(ZetaContext::Type type, const ZetaContextDispatch& dispatch) {
        dispatches_[static_cast<int>(type)] = dispatch;
    }

    ZetaContextDispatch* getDispatch(ZetaContext::Type type) {
        auto it = dispatches_.find(static_cast<int>(type));
        if (it != dispatches_.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::map<int, ZetaContextDispatch> dispatches_;
};

// ============================================================================
// DREAM SUGGESTION (074841): Dynamic Routing
// ============================================================================

// Task representation for routing decisions
struct ZetaTask {
    std::string name;
    std::string type;          // "reasoning", "coding", "embedding", "simple"
    float complexity;          // 0.0 - 1.0
    float code_likelihood;     // Likelihood this needs code model
    int estimated_tokens;
    bool requires_memory;      // Needs graph access

    ZetaTask() : complexity(0.5f), code_likelihood(0.0f),
                 estimated_tokens(100), requires_memory(false) {}
};

// Resource status for routing decisions
struct ZetaResourceStatus {
    float model_14b_load;      // 0.0 - 1.0 (current load)
    float model_7b_load;
    float model_4b_load;
    bool model_14b_available;
    bool model_7b_available;
    bool model_4b_available;
    int queue_depth_14b;
    int queue_depth_7b;

    ZetaResourceStatus() : model_14b_load(0.0f), model_7b_load(0.0f),
                           model_4b_load(0.0f), model_14b_available(true),
                           model_7b_available(true), model_4b_available(true),
                           queue_depth_14b(0), queue_depth_7b(0) {}
};

// Routing decision result
struct ZetaRoutingDecision {
    std::string target_model;  // "14B", "7B", "4B", "HYBRID"
    float confidence;
    std::string reason;
    bool use_parallel;         // Use 7B in parallel with 14B

    ZetaRoutingDecision() : target_model("14B"), confidence(0.5f),
                            use_parallel(false) {}
};

// ============================================================================
// DREAM SUGGESTION: Query Cache for routing decisions
// ============================================================================

struct RoutingCacheEntry {
    ZetaRoutingDecision decision;
    time_t timestamp;
    int hits;
};

// ============================================================================
// DREAM SUGGESTION: Routing Performance Metrics (Feedback Loop)
// ============================================================================

struct RoutingMetrics {
    std::string model;
    float avg_response_time_ms;
    float avg_accuracy;       // 0.0-1.0 based on critic feedback
    int total_requests;
    int successful_requests;
    time_t last_update;
};

// Dynamic router for context-aware task assignment
class ZetaDynamicRouter {
public:
    ZetaDynamicRouter() : code_threshold_(0.6f), complexity_threshold_(0.5f),
                          cache_ttl_sec_(300), cache_max_size_(100) {}

    // ========================================================================
    // DREAM SUGGESTION: Query Caching
    // ========================================================================

    // Check cache for similar query
    bool checkCache(const std::string& query_hash, ZetaRoutingDecision& out_decision) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = routing_cache_.find(query_hash);
        if (it != routing_cache_.end()) {
            // Check TTL
            time_t now = time(NULL);
            if (now - it->second.timestamp < cache_ttl_sec_) {
                out_decision = it->second.decision;
                it->second.hits++;
                cache_hits_++;
                return true;
            } else {
                // Expired, remove
                routing_cache_.erase(it);
            }
        }
        cache_misses_++;
        return false;
    }

    // Add to cache
    void addToCache(const std::string& query_hash, const ZetaRoutingDecision& decision) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        // Prune if full
        if (routing_cache_.size() >= (size_t)cache_max_size_) {
            pruneCache();
        }

        RoutingCacheEntry entry;
        entry.decision = decision;
        entry.timestamp = time(NULL);
        entry.hits = 0;
        routing_cache_[query_hash] = entry;
    }

    // Simple hash for query (first 50 chars lowercase + length)
    std::string hashQuery(const std::string& query) {
        std::string lower = StringUtility::toLower(query.substr(0, 50));
        return lower + "_" + std::to_string(query.length());
    }

    // ========================================================================
    // DREAM SUGGESTION: Routing Feedback Loop
    // ========================================================================

    // Record routing outcome for feedback
    void recordOutcome(const std::string& model, float response_time_ms,
                       float accuracy_score, bool success) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        if (model_metrics_.find(model) == model_metrics_.end()) {
            model_metrics_[model] = {model, 0.0f, 0.0f, 0, 0, time(NULL)};
        }

        auto& m = model_metrics_[model];
        m.total_requests++;
        if (success) m.successful_requests++;

        // Running average for response time
        m.avg_response_time_ms = (m.avg_response_time_ms * (m.total_requests - 1) +
                                   response_time_ms) / m.total_requests;

        // Running average for accuracy
        m.avg_accuracy = (m.avg_accuracy * (m.total_requests - 1) +
                          accuracy_score) / m.total_requests;

        m.last_update = time(NULL);

        // Auto-adjust thresholds based on feedback
        adaptThresholds();
    }

    // Get metrics for a model
    RoutingMetrics getModelMetrics(const std::string& model) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        if (model_metrics_.find(model) != model_metrics_.end()) {
            return model_metrics_[model];
        }
        return {model, 0.0f, 0.0f, 0, 0, 0};
    }

    // ========================================================================
    // DREAM SUGGESTION: Model Fallback Mechanism
    // ========================================================================

    // Route with fallback support
    ZetaRoutingDecision routeWithFallback(const ZetaTask& task,
                                           const ZetaResourceStatus& status) {
        ZetaRoutingDecision decision = route(task, status);

        // Check if primary model is overloaded/unavailable
        if (decision.target_model == "14B" && !status.model_14b_available) {
            decision = getFallbackDecision("14B", task, status);
            decision.reason = "Fallback: 14B unavailable, using " + decision.target_model;
        } else if (decision.target_model == "7B" && !status.model_7b_available) {
            decision = getFallbackDecision("7B", task, status);
            decision.reason = "Fallback: 7B unavailable, using " + decision.target_model;
        } else if (decision.target_model == "4B" && !status.model_4b_available) {
            decision = getFallbackDecision("4B", task, status);
            decision.reason = "Fallback: 4B unavailable, using " + decision.target_model;
        }

        // Check load-based fallback
        if (decision.target_model == "14B" && status.model_14b_load > 0.9f) {
            if (status.model_7b_available && status.model_7b_load < 0.7f) {
                decision.target_model = "7B";
                decision.confidence *= 0.8f;  // Slight confidence reduction
                decision.reason = "Load fallback: 14B overloaded (90%+), using 7B";
            }
        }

        return decision;
    }

    // Main routing function - assigns task to appropriate model
    ZetaRoutingDecision route(const ZetaTask& task, const ZetaResourceStatus& status) {
        ZetaRoutingDecision decision;

        // Rule 1: High code likelihood -> 7B Coder
        if (task.code_likelihood > code_threshold_) {
            if (status.model_7b_available && status.model_7b_load < 0.8f) {
                decision.target_model = "7B";
                decision.confidence = task.code_likelihood;
                decision.reason = "Code task routed to 7B coder";
                return decision;
            }
        }

        // Rule 2: Simple queries with low complexity -> 7B (faster)
        if (task.complexity < 0.3f && task.estimated_tokens < 200) {
            if (status.model_7b_available && status.queue_depth_7b < status.queue_depth_14b) {
                decision.target_model = "7B";
                decision.confidence = 1.0f - task.complexity;
                decision.reason = "Simple query routed to 7B for speed";
                return decision;
            }
        }

        // Rule 3: Memory/embedding queries -> 4B embedding
        if (task.type == "embedding" || (task.requires_memory && task.complexity < 0.4f)) {
            if (status.model_4b_available) {
                decision.target_model = "4B";
                decision.confidence = 0.8f;
                decision.reason = "Embedding/memory task routed to 4B";
                return decision;
            }
        }

        // Rule 4: Complex reasoning -> 14B
        if (task.complexity > complexity_threshold_ || task.type == "reasoning") {
            decision.target_model = "14B";
            decision.confidence = task.complexity;
            decision.reason = "Complex reasoning routed to 14B";

            // Consider parallel processing for very complex tasks
            if (task.complexity > 0.8f && status.model_7b_available) {
                decision.use_parallel = true;
                decision.reason = "Very complex task: 14B primary with 7B parallel";
            }
            return decision;
        }

        // Rule 5: Load balancing - route to least loaded model
        if (status.model_14b_load > 0.7f && status.model_7b_load < 0.5f) {
            decision.target_model = "7B";
            decision.confidence = 0.6f;
            decision.reason = "Load balancing: 14B busy, routing to 7B";
            return decision;
        }

        // Default: 14B for quality
        decision.target_model = "14B";
        decision.confidence = 0.5f;
        decision.reason = "Default routing to 14B";
        return decision;
    }

    // Analyze query to create task profile
    ZetaTask analyzeQuery(const std::string& query) {
        ZetaTask task;
        task.name = query.substr(0, 50);

        std::string lower = StringUtility::toLower(query);

        // Detect code-related queries
        if (lower.find("code") != std::string::npos ||
            lower.find("function") != std::string::npos ||
            lower.find("implement") != std::string::npos ||
            lower.find("write") != std::string::npos ||
            lower.find("debug") != std::string::npos ||
            lower.find("fix") != std::string::npos) {
            task.code_likelihood = 0.8f;
            task.type = "coding";
        }

        // Detect reasoning queries
        if (lower.find("explain") != std::string::npos ||
            lower.find("why") != std::string::npos ||
            lower.find("analyze") != std::string::npos ||
            lower.find("compare") != std::string::npos ||
            lower.find("reason") != std::string::npos) {
            task.complexity = 0.7f;
            task.type = "reasoning";
        }

        // Detect memory/retrieval queries
        if (lower.find("remember") != std::string::npos ||
            lower.find("recall") != std::string::npos ||
            lower.find("what did") != std::string::npos ||
            lower.find("search") != std::string::npos) {
            task.requires_memory = true;
            task.type = "retrieval";
        }

        // Estimate complexity by length and structure
        task.estimated_tokens = (int)(query.length() / 4);
        if (query.length() > 500) task.complexity += 0.2f;
        if (query.find("?") != std::string::npos) task.complexity += 0.1f;

        // Clamp values
        task.complexity = std::min(1.0f, task.complexity);
        task.code_likelihood = std::min(1.0f, task.code_likelihood);

        return task;
    }

    // Update routing thresholds based on performance
    void updateThresholds(float new_code_threshold, float new_complexity_threshold) {
        code_threshold_ = new_code_threshold;
        complexity_threshold_ = new_complexity_threshold;
    }

    // Get routing statistics
    std::string getStats() const {
        std::stringstream ss;
        ss << "=== Dynamic Router Stats ===\n";
        ss << "Code threshold: " << code_threshold_ << "\n";
        ss << "Complexity threshold: " << complexity_threshold_ << "\n";
        ss << "Routes to 14B: " << routes_14b_ << "\n";
        ss << "Routes to 7B: " << routes_7b_ << "\n";
        ss << "Routes to 4B: " << routes_4b_ << "\n";
        ss << "Parallel routes: " << routes_parallel_ << "\n";
        return ss.str();
    }

    // Get extended stats including cache and metrics
    std::string getExtendedStats() const {
        std::stringstream ss;
        ss << getStats();
        ss << "\n=== Cache Stats ===\n";
        ss << "Cache size: " << routing_cache_.size() << "/" << cache_max_size_ << "\n";
        ss << "Cache hits: " << cache_hits_ << "\n";
        ss << "Cache misses: " << cache_misses_ << "\n";
        if (cache_hits_ + cache_misses_ > 0) {
            ss << "Hit rate: " << (float)cache_hits_ / (cache_hits_ + cache_misses_) * 100 << "%\n";
        }
        ss << "\n=== Model Metrics ===\n";
        for (const auto& [model, metrics] : model_metrics_) {
            ss << model << ": " << metrics.total_requests << " reqs, "
               << metrics.avg_response_time_ms << "ms avg, "
               << (metrics.avg_accuracy * 100) << "% accuracy\n";
        }
        return ss.str();
    }

private:
    float code_threshold_;
    float complexity_threshold_;
    mutable int routes_14b_ = 0;
    mutable int routes_7b_ = 0;
    mutable int routes_4b_ = 0;
    mutable int routes_parallel_ = 0;

    // Cache members
    std::map<std::string, RoutingCacheEntry> routing_cache_;
    mutable std::mutex cache_mutex_;
    int cache_ttl_sec_;
    int cache_max_size_;
    mutable int cache_hits_ = 0;
    mutable int cache_misses_ = 0;

    // Metrics members
    std::map<std::string, RoutingMetrics> model_metrics_;
    mutable std::mutex metrics_mutex_;

    // Prune oldest cache entries
    void pruneCache() {
        // Remove entries with lowest hits and oldest timestamp
        if (routing_cache_.empty()) return;

        auto oldest = routing_cache_.begin();
        for (auto it = routing_cache_.begin(); it != routing_cache_.end(); ++it) {
            if (it->second.hits < oldest->second.hits ||
                (it->second.hits == oldest->second.hits &&
                 it->second.timestamp < oldest->second.timestamp)) {
                oldest = it;
            }
        }
        routing_cache_.erase(oldest);
    }

    // Get fallback model decision
    ZetaRoutingDecision getFallbackDecision(const std::string& unavailable_model,
                                             const ZetaTask& task,
                                             const ZetaResourceStatus& status) {
        ZetaRoutingDecision fallback;

        if (unavailable_model == "14B") {
            // 14B unavailable - try 7B if not too complex
            if (status.model_7b_available && task.complexity < 0.7f) {
                fallback.target_model = "7B";
                fallback.confidence = 0.6f;
            } else {
                // No good fallback, return anyway with low confidence
                fallback.target_model = "7B";
                fallback.confidence = 0.3f;
            }
        } else if (unavailable_model == "7B") {
            // 7B unavailable - route to 14B
            if (status.model_14b_available) {
                fallback.target_model = "14B";
                fallback.confidence = 0.7f;
            } else {
                // Both unavailable, try 4B for simple tasks
                fallback.target_model = "4B";
                fallback.confidence = 0.2f;
            }
        } else if (unavailable_model == "4B") {
            // 4B unavailable - route to 7B
            if (status.model_7b_available) {
                fallback.target_model = "7B";
                fallback.confidence = 0.5f;
            } else {
                fallback.target_model = "14B";
                fallback.confidence = 0.4f;
            }
        }

        return fallback;
    }

    // Auto-adapt thresholds based on performance metrics
    void adaptThresholds() {
        // If 7B is performing well on complex tasks, lower complexity threshold
        auto metrics_7b = model_metrics_.find("7B");
        auto metrics_14b = model_metrics_.find("14B");

        if (metrics_7b != model_metrics_.end() && metrics_14b != model_metrics_.end()) {
            // If 7B has good accuracy (>0.8) and faster response, be more aggressive
            if (metrics_7b->second.avg_accuracy > 0.8f &&
                metrics_7b->second.avg_response_time_ms < metrics_14b->second.avg_response_time_ms * 0.5f) {
                // 7B is fast and accurate - route more to it
                if (complexity_threshold_ < 0.7f) {
                    complexity_threshold_ += 0.01f;  // Gradual increase
                    fprintf(stderr, "[ROUTER-ADAPT] Increased complexity threshold to %.2f\n",
                            complexity_threshold_);
                }
            }
            // If 7B accuracy is poor, be more conservative
            if (metrics_7b->second.avg_accuracy < 0.5f && metrics_7b->second.total_requests > 10) {
                if (complexity_threshold_ > 0.3f) {
                    complexity_threshold_ -= 0.01f;
                    fprintf(stderr, "[ROUTER-ADAPT] Decreased complexity threshold to %.2f\n",
                            complexity_threshold_);
                }
            }
        }
    }
};

// Global router instance
static ZetaDynamicRouter g_dynamic_router;

// Convenience function for quick routing
static inline ZetaRoutingDecision zeta_route_query(const std::string& query,
                                                    const ZetaResourceStatus& status) {
    ZetaTask task = g_dynamic_router.analyzeQuery(query);
    return g_dynamic_router.route(task, status);
}

#endif // ZETA_UTILS_H
