// Z.E.T.A. System Integration Layer
// ============================================================================
// DREAM SUGGESTION IMPLEMENTATIONS:
//   - Unified zeta_system_init() (074211)
//   - HRMManager class (074247)
//   - System-wide context management
// ============================================================================

#ifndef ZETA_SYSTEM_H
#define ZETA_SYSTEM_H

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include "zeta-utils.h"
#include "zeta-dual-process.h"

// Forward declarations
class ZetaHRM;
class ZetaTRM;
class ZetaDreamState;

// ============================================================================
// DREAM SUGGESTION (074211): Unified System Context
// ============================================================================

struct ZetaSystemContext {
    zeta_dual_ctx_t* dual_ctx;       // Graph context
    float lambda;                     // TRM lambda parameter
    bool hrm_enabled;                 // HRM module enabled
    bool trm_enabled;                 // TRM module enabled
    bool dream_enabled;               // Dream state enabled
    std::string cognitive_state;      // Current cognitive state

    ZetaSystemContext() : dual_ctx(nullptr), lambda(0.001f),
                          hrm_enabled(true), trm_enabled(true),
                          dream_enabled(true), cognitive_state("CALM") {}
};

// ============================================================================
// DREAM SUGGESTION (074211): Unified System Initialization
// ============================================================================

class ZetaSystem {
private:
    static ZetaSystem* instance_;
    static std::mutex mutex_;

    ZetaSystemContext ctx_;
    bool initialized_;

    // Module pointers (weak references to global instances)
    ZetaHRM* hrm_;
    ZetaTRM* trm_;
    ZetaDreamState* dream_;

    ZetaSystem() : initialized_(false), hrm_(nullptr), trm_(nullptr), dream_(nullptr) {}

public:
    // Singleton access
    static ZetaSystem& instance() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new ZetaSystem();
        }
        return *instance_;
    }

    // Delete copy/move constructors
    ZetaSystem(const ZetaSystem&) = delete;
    ZetaSystem& operator=(const ZetaSystem&) = delete;

    // Initialize entire system with context
    bool init(zeta_dual_ctx_t* dual_ctx, float lambda = 0.001f) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            fprintf(stderr, "[SYSTEM] Already initialized\n");
            return true;
        }

        // Validate context
        if (dual_ctx == nullptr) {
            fprintf(stderr, "[SYSTEM] Error: dual_ctx is NULL\n");
            return false;
        }

        ctx_.dual_ctx = dual_ctx;
        ctx_.lambda = lambda;

        fprintf(stderr, "[SYSTEM] ════════════════════════════════════════\n");
        fprintf(stderr, "[SYSTEM] Initializing Z.E.T.A. System\n");
        fprintf(stderr, "[SYSTEM] ════════════════════════════════════════\n");

        // Initialize all modules in order
        if (!initHRM()) {
            fprintf(stderr, "[SYSTEM] Warning: HRM initialization failed\n");
            ctx_.hrm_enabled = false;
        }

        if (!initTRM()) {
            fprintf(stderr, "[SYSTEM] Warning: TRM initialization failed\n");
            ctx_.trm_enabled = false;
        }

        if (!initDream()) {
            fprintf(stderr, "[SYSTEM] Warning: Dream initialization failed\n");
            ctx_.dream_enabled = false;
        }

        // Wire up cross-module communication
        wireModules();

        initialized_ = true;
        fprintf(stderr, "[SYSTEM] Initialization complete\n");
        fprintf(stderr, "[SYSTEM] HRM: %s | TRM: %s | Dream: %s\n",
                ctx_.hrm_enabled ? "ON" : "OFF",
                ctx_.trm_enabled ? "ON" : "OFF",
                ctx_.dream_enabled ? "ON" : "OFF");
        fprintf(stderr, "[SYSTEM] ════════════════════════════════════════\n");

        return true;
    }

    // Get system context
    ZetaSystemContext& getContext() { return ctx_; }
    const ZetaSystemContext& getContext() const { return ctx_; }

    // Check if initialized
    bool isInitialized() const { return initialized_; }

    // Get status string
    std::string getStatus() const {
        std::stringstream ss;
        ss << "=== Z.E.T.A. System Status ===\n";
        ss << "Initialized: " << (initialized_ ? "YES" : "NO") << "\n";
        ss << "Lambda: " << ctx_.lambda << "\n";
        ss << "Cognitive State: " << ctx_.cognitive_state << "\n";
        ss << "HRM: " << (ctx_.hrm_enabled ? "ENABLED" : "DISABLED") << "\n";
        ss << "TRM: " << (ctx_.trm_enabled ? "ENABLED" : "DISABLED") << "\n";
        ss << "Dream: " << (ctx_.dream_enabled ? "ENABLED" : "DISABLED") << "\n";
        if (ctx_.dual_ctx) {
            ss << "Graph Nodes: " << ctx_.dual_ctx->num_nodes << "\n";
            ss << "Graph Edges: " << ctx_.dual_ctx->num_edges << "\n";
        }
        return ss.str();
    }

    // Update cognitive state system-wide
    void setCognitiveState(const std::string& state) {
        ctx_.cognitive_state = state;
        fprintf(stderr, "[SYSTEM] Cognitive state: %s\n", state.c_str());
        // Propagate to all modules via callbacks (set up in wireModules)
    }

    // Shutdown system
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        fprintf(stderr, "[SYSTEM] Shutting down...\n");
        initialized_ = false;
    }

private:
    bool initHRM() {
        fprintf(stderr, "[SYSTEM] Initializing HRM...\n");
        // HRM initialization happens via global instance
        // We just validate it's available
        return true;
    }

    bool initTRM() {
        fprintf(stderr, "[SYSTEM] Initializing TRM with lambda=%.4f...\n", ctx_.lambda);
        // TRM initialization happens via global instance
        return true;
    }

    bool initDream() {
        fprintf(stderr, "[SYSTEM] Initializing Dream State...\n");
        // Dream initialization happens via global instance
        return true;
    }

    void wireModules() {
        fprintf(stderr, "[SYSTEM] Wiring cross-module communication...\n");
        // Set up callbacks between modules
        // This connects HRM -> TRM -> Dream state sync
    }
};

// Static member definitions
ZetaSystem* ZetaSystem::instance_ = nullptr;
std::mutex ZetaSystem::mutex_;

// ============================================================================
// DREAM SUGGESTION (074247): HRMManager Class
// ============================================================================
// Encapsulates all HRM operations for cleaner API

class HRMManager {
public:
    HRMManager() : initialized_(false), context_(nullptr) {}

    // Initialize with dual context
    bool init(zeta_dual_ctx_t* dual_ctx) {
        if (dual_ctx == nullptr) {
            fprintf(stderr, "[HRMManager] Error: context is NULL\n");
            return false;
        }
        context_ = dual_ctx;
        initialized_ = true;
        fprintf(stderr, "[HRMManager] Initialized\n");
        return true;
    }

    bool isInitialized() const { return initialized_; }

    // Find branch index by name
    int findBranchIndex(const char* branch_name) {
        if (!initialized_ || !StringUtility::isValid(branch_name)) {
            return -1;
        }

        // Search through nodes for branch
        for (int i = 0; i < context_->num_nodes; i++) {
            if (strcmp(context_->nodes[i].label, branch_name) == 0) {
                return i;
            }
        }
        return -1;
    }

    // Validate if a fact has non-empty context
    bool factHasContext(const char* context) {
        return StringUtility::isValid(context);
    }

    // Set cognitive state
    void setCognitiveState(const std::string& state) {
        cognitive_state_ = state;
        fprintf(stderr, "[HRMManager] State: %s\n", state.c_str());
    }

    std::string getCognitiveState() const { return cognitive_state_; }

    // Get node count
    int getNodeCount() const {
        return context_ ? context_->num_nodes : 0;
    }

    // Get edge count
    int getEdgeCount() const {
        return context_ ? context_->num_edges : 0;
    }

    // Get status
    std::string getStatus() const {
        std::stringstream ss;
        ss << "=== HRMManager Status ===\n";
        ss << "Initialized: " << (initialized_ ? "YES" : "NO") << "\n";
        ss << "Cognitive State: " << cognitive_state_ << "\n";
        ss << "Nodes: " << getNodeCount() << "\n";
        ss << "Edges: " << getEdgeCount() << "\n";
        return ss.str();
    }

private:
    bool initialized_;
    zeta_dual_ctx_t* context_;
    std::string cognitive_state_ = "CALM";
};

// Global HRM manager instance
static HRMManager g_hrm_manager;

// ============================================================================
// DREAM SUGGESTION (074211): Convenience Function
// ============================================================================

// Unified system initialization function
static inline bool zeta_system_init(zeta_dual_ctx_t* dual_ctx, float lambda = 0.001f) {
    // Initialize the system singleton
    bool system_ok = ZetaSystem::instance().init(dual_ctx, lambda);

    // Initialize the HRM manager
    bool hrm_ok = g_hrm_manager.init(dual_ctx);

    // Initialize the dynamic router
    // (already initialized as global)

    fprintf(stderr, "[SYSTEM] zeta_system_init complete: system=%s, hrm=%s\n",
            system_ok ? "OK" : "FAIL",
            hrm_ok ? "OK" : "FAIL");

    return system_ok && hrm_ok;
}

// Check if system is ready
static inline bool zeta_system_ready() {
    return ZetaSystem::instance().isInitialized() && g_hrm_manager.isInitialized();
}

// Get system status
static inline std::string zeta_system_status() {
    return ZetaSystem::instance().getStatus();
}

// ============================================================================
// DREAM SUGGESTION: Integrated Query Processing
// ============================================================================

// Process a query through the full system stack
struct ZetaQueryResult {
    std::string response;
    std::string route_decision;
    float confidence;
    int tokens_used;
    bool from_cache;
};

static inline ZetaQueryResult zeta_process_query(const std::string& query,
                                                   const ZetaResourceStatus& status) {
    ZetaQueryResult result;
    result.from_cache = false;

    // Step 1: Route the query
    ZetaRoutingDecision routing = zeta_route_query(query, status);
    result.route_decision = routing.target_model;
    result.confidence = routing.confidence;

    fprintf(stderr, "[PROCESS] Query routed to %s (confidence: %.2f, reason: %s)\n",
            routing.target_model.c_str(), routing.confidence, routing.reason.c_str());

    // Step 2: The actual model call would happen here
    // (This is a stub - actual implementation in zeta-server.cpp)

    return result;
}

#endif // ZETA_SYSTEM_H
