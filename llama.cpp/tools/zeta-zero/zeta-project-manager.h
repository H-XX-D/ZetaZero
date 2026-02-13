#pragma once
// Z.E.T.A. Project Manager - Multi-Project 4-Layer Graph Isolation
// ============================================================================
//
// Each project owns an isolated 4-layer graph:
//   L1: Nodes (knowledge blocks)
//   L2: KV segments (cached attention state)
//   L3: Edges (relationships between nodes)
//   L4: Operations (mutation history)
//
// Projects can be independently created, switched, and deleted.
// The "global" project is the default workspace.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <sys/stat.h>

// Forward declarations for graph types used by project context
struct zeta_dual_ctx_t;
struct zeta_git_ctx_t;

namespace zeta_project {

// ============================================================================
// ENUMS
// ============================================================================

enum class Mode {
    CHAT,
    CODE,
    RESEARCH,
    CREATIVE,
    DREAM,
    DISCOVERY
};

inline Mode string_to_mode(const std::string& s) {
    if (s == "CODE" || s == "code") return Mode::CODE;
    if (s == "RESEARCH" || s == "research") return Mode::RESEARCH;
    if (s == "CREATIVE" || s == "creative") return Mode::CREATIVE;
    if (s == "DREAM" || s == "dream") return Mode::DREAM;
    if (s == "DISCOVERY" || s == "discovery") return Mode::DISCOVERY;
    return Mode::CHAT;
}

inline const char* mode_to_string(Mode m) {
    switch (m) {
        case Mode::CHAT:      return "CHAT";
        case Mode::CODE:      return "CODE";
        case Mode::RESEARCH:  return "RESEARCH";
        case Mode::CREATIVE:  return "CREATIVE";
        case Mode::DREAM:     return "DREAM";
        case Mode::DISCOVERY: return "DISCOVERY";
    }
    return "CHAT";
}

enum class IsolationLevel {
    FULL,       // Completely isolated graph
    SHARED,     // Shared base graph with project overlay
    INHERIT     // Inherits parent project graph
};

inline IsolationLevel string_to_isolation(const std::string& s) {
    if (s == "shared" || s == "SHARED") return IsolationLevel::SHARED;
    if (s == "inherit" || s == "INHERIT") return IsolationLevel::INHERIT;
    return IsolationLevel::FULL;
}

inline const char* isolation_to_string(IsolationLevel l) {
    switch (l) {
        case IsolationLevel::FULL:    return "full";
        case IsolationLevel::SHARED:  return "shared";
        case IsolationLevel::INHERIT: return "inherit";
    }
    return "full";
}

// ============================================================================
// PROJECT MANIFEST - Metadata for a project
// ============================================================================

struct ProjectManifest {
    std::string name;
    Mode mode = Mode::CHAT;
    std::string description;
    IsolationLevel isolation = IsolationLevel::FULL;
    std::string current_branch = "main";
    std::string source_path;

    int64_t created_at = 0;
    int64_t last_accessed = 0;

    // Layer counts
    int64_t node_count = 0;
    int64_t edge_count = 0;
    int64_t kv_segment_count = 0;
    int64_t operation_count = 0;

    // Corpus stats
    int64_t corpus_file_count = 0;
    int64_t corpus_indexed_count = 0;
    int64_t corpus_size_bytes = 0;

    // Dream state
    int64_t last_dream_at = 0;
    int64_t dream_count_pending = 0;
    int64_t dream_count_archived = 0;
    bool dream_enabled = true;

    std::vector<std::string> branches = {"main"};
};

// ============================================================================
// PROJECT CONTEXT - Runtime state for an active project
// ============================================================================

struct ProjectContext {
    zeta_dual_ctx_t* graph = nullptr;
    zeta_git_ctx_t* git = nullptr;
    ProjectManifest manifest;

    // Corpus paths
    std::string corpus_path;
    std::string corpus_raw;
    std::string dream_path;
    std::string dream_pending;
    std::string dream_archive;

    bool is_dirty = false;
};

// ============================================================================
// PROJECT INFO - Lightweight struct for list_projects()
// ============================================================================

struct ProjectInfo {
    std::string name;
    Mode mode = Mode::CHAT;
    std::string description;
    IsolationLevel isolation = IsolationLevel::FULL;
    std::string current_branch = "main";
    int64_t node_count = 0;
    int64_t edge_count = 0;
    std::vector<std::string> branches;
};

// ============================================================================
// PROJECT MANAGER - Core manager class
// ============================================================================

class ProjectManager {
public:
    std::string active_project = "global";
    ProjectContext* active_ctx = nullptr;

    bool create_project(
        const std::string& name,
        Mode mode,
        const std::string& description,
        IsolationLevel isolation,
        const std::string& source_path
    ) {
        if (name.empty() || projects_.count(name)) return false;

        ProjectContext ctx;
        ctx.manifest.name = name;
        ctx.manifest.mode = mode;
        ctx.manifest.description = description;
        ctx.manifest.isolation = isolation;
        ctx.manifest.source_path = source_path;
        ctx.manifest.created_at = (int64_t)std::time(nullptr);
        ctx.manifest.last_accessed = ctx.manifest.created_at;

        // Setup project directories
        std::string project_dir = storage_dir_ + "/" + name;
        mkdir(project_dir.c_str(), 0755);

        ctx.corpus_path = project_dir + "/corpus";
        ctx.corpus_raw = project_dir + "/corpus/raw";
        ctx.dream_path = project_dir + "/dreams";
        ctx.dream_pending = project_dir + "/dreams/pending";
        ctx.dream_archive = project_dir + "/dreams/archive";

        mkdir(ctx.corpus_path.c_str(), 0755);
        mkdir(ctx.corpus_raw.c_str(), 0755);
        mkdir(ctx.dream_path.c_str(), 0755);
        mkdir(ctx.dream_pending.c_str(), 0755);
        mkdir(ctx.dream_archive.c_str(), 0755);

        projects_[name] = ctx;

        fprintf(stderr, "[PROJECT] Created project '%s' (mode=%s, isolation=%s)\n",
                name.c_str(), mode_to_string(mode), isolation_to_string(isolation));
        return true;
    }

    bool switch_project(const std::string& name, const std::string& branch = "") {
        auto it = projects_.find(name);
        if (it == projects_.end()) {
            // Auto-create if switching to "global"
            if (name == "global") {
                create_project("global", Mode::CHAT, "Default workspace",
                              IsolationLevel::FULL, "");
                it = projects_.find(name);
                if (it == projects_.end()) return false;
            } else {
                return false;
            }
        }

        active_project = name;
        active_ctx = &it->second;
        active_ctx->manifest.last_accessed = (int64_t)std::time(nullptr);

        if (!branch.empty()) {
            // Check if branch exists, create if not
            auto& branches = active_ctx->manifest.branches;
            if (std::find(branches.begin(), branches.end(), branch) == branches.end()) {
                branches.push_back(branch);
            }
            active_ctx->manifest.current_branch = branch;
        }

        fprintf(stderr, "[PROJECT] Switched to '%s' (branch=%s)\n",
                name.c_str(), active_ctx->manifest.current_branch.c_str());
        return true;
    }

    bool delete_project(const std::string& name, bool confirm) {
        if (!confirm) return false;
        if (name == "global") return false;  // Cannot delete global
        if (name == active_project) return false;  // Cannot delete active

        auto it = projects_.find(name);
        if (it == projects_.end()) return false;

        projects_.erase(it);
        fprintf(stderr, "[PROJECT] Deleted project '%s'\n", name.c_str());
        return true;
    }

    std::vector<ProjectInfo> list_projects() {
        std::vector<ProjectInfo> result;
        for (const auto& [name, ctx] : projects_) {
            ProjectInfo info;
            info.name = ctx.manifest.name;
            info.mode = ctx.manifest.mode;
            info.description = ctx.manifest.description;
            info.isolation = ctx.manifest.isolation;
            info.current_branch = ctx.manifest.current_branch;
            info.node_count = ctx.manifest.node_count;
            info.edge_count = ctx.manifest.edge_count;
            info.branches = ctx.manifest.branches;
            result.push_back(info);
        }
        return result;
    }

    ProjectManifest* get_active_manifest() {
        if (!active_ctx) return nullptr;
        return &active_ctx->manifest;
    }

private:
    std::unordered_map<std::string, ProjectContext> projects_;
    std::string storage_dir_ = "/storage";
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

inline ProjectManager* g_project_manager = nullptr;

inline void zeta_project_init(const std::string& storage_dir = "") {
    if (g_project_manager) return;

    g_project_manager = new ProjectManager();

    if (!storage_dir.empty()) {
        // Create storage directory
        mkdir(storage_dir.c_str(), 0755);
    }

    // Auto-create global project
    g_project_manager->create_project("global", Mode::CHAT,
                                       "Default workspace",
                                       IsolationLevel::FULL, "");
    g_project_manager->switch_project("global");

    fprintf(stderr, "[PROJECT] Project Manager initialized (storage=%s)\n",
            storage_dir.empty() ? "/storage" : storage_dir.c_str());
}

}  // namespace zeta_project
