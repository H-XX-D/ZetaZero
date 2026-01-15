// Z.E.T.A. Project Manager - Multi-Project 4-Layer Graph Isolation
// ============================================================================
// Each project gets its own complete 4-layer graph:
//   L1: Tokenized Node Graph (facts, entities, concepts)
//   L2: Graph-KV Cache (transformer state snapshots)
//   L3: Branch Graph (ternary edges with belief values)
//   L4: Operations Graph (reasoning chains)
//
// Plus per-project:
//   - Corpus: Source material (code/docs) that gets INGESTED to build the graph
//   - Dreams: Idle consolidation cycles that strengthen/prune the graph
//
// Projects isolate knowledge domains:
//   - CODE projects: codebase knowledge + indexed source files
//   - CREATIVE projects: fiction worlds (characters, plots, drafts)
//   - RESEARCH projects: papers, notes, investigation threads
//   - IDEAS projects: brainstorming sessions
//
// Storage Layout (dedicated NVMe):
//   /mnt/GitGraph/
//   ├── config.json              # Global config
//   ├── global/                  # Default user knowledge
//   │   ├── manifest.json
//   │   ├── L1_nodes.bin
//   │   ├── L2_kv_cache/
//   │   ├── L3_edges.bin
//   │   ├── L4_operations.bin
//   │   ├── corpus/              # User documents
//   │   └── dreams/              # Global dream state
//   └── projects/
//       └── {project}/
//           ├── manifest.json
//           ├── L1_nodes.bin          # Tokenized nodes (facts, entities)
//           ├── L2_kv_cache/          # KV state snapshots per node
//           ├── L3_edges.bin          # Ternary edges (belief -1 to +1)
//           ├── L4_operations.bin     # Reasoning chains log
//           ├── corpus/               # SOURCE MATERIAL for ingestion
//           │   ├── raw/              # Original files (code, docs, text)
//           │   └── indexed/          # Tracking what's been ingested
//           └── dreams/               # Idle consolidation state
//               ├── pending/          # Dreams awaiting review
//               ├── archive/          # Reviewed/accepted dreams
//               └── state.json        # Dream cycle state
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_PROJECT_MANAGER_H
#define ZETA_PROJECT_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

// Note: zeta_dual_ctx_t and zeta_git_ctx_t must be defined before including this header
// (include zeta-dual-process.h and zeta-graph-git.h first)

namespace zeta_project {

// ============================================================================
// PROJECT MODE (mirrors zeta_modes::Mode)
// ============================================================================

enum class ProjectMode {
    CHAT,       // Default conversational
    CREATIVE,   // Fiction/roleplay - isolated world
    RESEARCH,   // Deep exploration
    CODE,       // Programming - codebase knowledge
    DISCOVERY,  // Research + associative
    IDEAS       // Brainstorming sessions
};

inline const char* mode_to_string(ProjectMode m) {
    switch (m) {
        case ProjectMode::CHAT:      return "CHAT";
        case ProjectMode::CREATIVE:  return "CREATIVE";
        case ProjectMode::RESEARCH:  return "RESEARCH";
        case ProjectMode::CODE:      return "CODE";
        case ProjectMode::DISCOVERY: return "DISCOVERY";
        case ProjectMode::IDEAS:     return "IDEAS";
    }
    return "UNKNOWN";
}

inline ProjectMode string_to_mode(const std::string& s) {
    if (s == "CHAT")      return ProjectMode::CHAT;
    if (s == "CREATIVE")  return ProjectMode::CREATIVE;
    if (s == "RESEARCH")  return ProjectMode::RESEARCH;
    if (s == "CODE")      return ProjectMode::CODE;
    if (s == "DISCOVERY") return ProjectMode::DISCOVERY;
    if (s == "IDEAS")     return ProjectMode::IDEAS;
    return ProjectMode::CHAT;  // Default
}

// ============================================================================
// ISOLATION LEVEL
// ============================================================================

enum class IsolationLevel {
    FULL,           // Complete isolation, no access to other projects
    READ_GLOBAL,    // Can read global user facts, but writes stay in project
    SHARED          // Full access to global (legacy behavior)
};

inline const char* isolation_to_string(IsolationLevel i) {
    switch (i) {
        case IsolationLevel::FULL:        return "full";
        case IsolationLevel::READ_GLOBAL: return "read-global";
        case IsolationLevel::SHARED:      return "shared";
    }
    return "full";
}

inline IsolationLevel string_to_isolation(const std::string& s) {
    if (s == "full")        return IsolationLevel::FULL;
    if (s == "read-global") return IsolationLevel::READ_GLOBAL;
    if (s == "shared")      return IsolationLevel::SHARED;
    return IsolationLevel::FULL;
}

// ============================================================================
// PROJECT MANIFEST
// ============================================================================

struct ProjectManifest {
    std::string name;
    ProjectMode mode;
    std::string description;
    IsolationLevel isolation;

    // For CODE projects
    std::string source_path;

    // Timestamps
    int64_t created_at;
    int64_t last_accessed;

    // Branch info
    std::vector<std::string> branches;
    std::string current_branch;

    // Stats (cached, updated on save)
    int64_t node_count;
    int64_t edge_count;
    int64_t kv_segment_count;
    int64_t operation_count;

    // Corpus stats
    int64_t corpus_file_count;
    int64_t corpus_indexed_count;
    int64_t corpus_size_bytes;

    // Dream stats
    int64_t dream_count_pending;
    int64_t dream_count_archived;
    int64_t last_dream_at;
    bool dream_enabled;
};

// ============================================================================
// DREAM STATE (per-project)
// ============================================================================

struct ProjectDreamState {
    bool enabled;
    int64_t last_dream_at;
    int64_t dream_count;
    int consecutive_discards;
    std::string last_seed_node;         // Last graph node used as dream seed
    std::vector<std::string> pending;   // Dream IDs awaiting review
};

// ============================================================================
// PROJECT CONTEXT
// ============================================================================

struct ProjectContext {
    ProjectManifest manifest;
    std::string path;           // Full path to project directory

    // Loaded graph state (owned by project manager)
    zeta_dual_ctx_t* graph;     // L1 nodes + L3 edges
    zeta_git_ctx_t* git;        // Branch management

    // Layer file paths
    std::string l1_path;        // L1_nodes.bin
    std::string l2_path;        // L2_kv_cache/
    std::string l3_path;        // L3_edges.bin
    std::string l4_path;        // L4_operations.bin

    // Corpus paths
    std::string corpus_path;    // corpus/
    std::string corpus_raw;     // corpus/raw/
    std::string corpus_indexed; // corpus/indexed/

    // Dream paths
    std::string dream_path;     // dreams/
    std::string dream_pending;  // dreams/pending/
    std::string dream_archive;  // dreams/archive/
    ProjectDreamState dream_state;

    bool is_loaded;
    bool is_dirty;              // Has unsaved changes
};

// ============================================================================
// PROJECT MANAGER
// ============================================================================

// Default NVMe mount point for dedicated GitGraph storage
#define ZETA_GITGRAPH_NVME "/mnt/GitGraph"

class ProjectManager {
public:
    // Base directory for all projects (dedicated NVMe)
    std::string nvme_path;      // /mnt/GitGraph/
    std::string base_path;      // /mnt/GitGraph/projects/
    std::string global_path;    // /mnt/GitGraph/global/

    // All known projects
    std::unordered_map<std::string, ProjectManifest> projects;

    // Currently active project
    std::string active_project;
    ProjectContext* active_ctx;

    // Global project (always available for cross-reference)
    ProjectContext* global_ctx;

    ProjectManager() : active_ctx(nullptr), global_ctx(nullptr) {}
    ~ProjectManager() { shutdown(); }

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    bool init(const std::string& storage_path = "") {
        // Determine base path - prefer dedicated NVMe, fallback to provided path
        struct stat st;
        if (stat(ZETA_GITGRAPH_NVME, &st) == 0 && S_ISDIR(st.st_mode)) {
            // Dedicated NVMe available
            nvme_path = ZETA_GITGRAPH_NVME;
            fprintf(stderr, "[PROJECT] Using dedicated NVMe: %s\n", nvme_path.c_str());
        } else if (!storage_path.empty()) {
            // Fallback to provided path
            nvme_path = storage_path;
            fprintf(stderr, "[PROJECT] Using storage path: %s\n", nvme_path.c_str());
        } else {
            // Last resort: home directory
            const char* home = getenv("HOME");
            if (!home) home = "/tmp";
            nvme_path = std::string(home) + "/.zeta";
            fprintf(stderr, "[PROJECT] Using home fallback: %s\n", nvme_path.c_str());
        }

        // Ensure trailing slash
        if (nvme_path.back() != '/') nvme_path += '/';

        base_path = nvme_path + "projects/";
        global_path = nvme_path + "global/";

        // Create directories if needed
        mkdir_recursive(nvme_path);
        mkdir_recursive(base_path);
        mkdir_recursive(global_path);

        // Scan for existing projects
        scan_projects();

        // Ensure global project exists
        if (projects.find("global") == projects.end()) {
            create_project("global", ProjectMode::CHAT, "Global user knowledge",
                          IsolationLevel::SHARED, "");
        }

        fprintf(stderr, "[PROJECT] Initialized. NVMe: %s, Projects: %zu\n",
                nvme_path.c_str(), projects.size());
        return true;
    }

    void shutdown() {
        if (active_ctx) {
            save_project(active_ctx);
            unload_project(active_ctx);
            delete active_ctx;
            active_ctx = nullptr;
        }
        if (global_ctx && global_ctx != active_ctx) {
            save_project(global_ctx);
            unload_project(global_ctx);
            delete global_ctx;
            global_ctx = nullptr;
        }
    }

    // ========================================================================
    // PROJECT OPERATIONS
    // ========================================================================

    // Create a new project
    bool create_project(
        const std::string& name,
        ProjectMode mode,
        const std::string& description,
        IsolationLevel isolation,
        const std::string& source_path
    ) {
        // Validate name
        if (name.empty() || name.find('/') != std::string::npos) {
            fprintf(stderr, "[PROJECT] Invalid project name: %s\n", name.c_str());
            return false;
        }

        // Check if exists
        if (projects.find(name) != projects.end()) {
            fprintf(stderr, "[PROJECT] Project already exists: %s\n", name.c_str());
            return false;
        }

        // Create directory structure
        std::string project_path = (name == "global") ? global_path : (base_path + name + "/");
        mkdir_recursive(project_path);

        // 4-Layer graph directories
        mkdir_recursive(project_path + "L2_kv_cache/");

        // Corpus directories (per-project document/code storage)
        mkdir_recursive(project_path + "corpus/");
        mkdir_recursive(project_path + "corpus/raw/");
        mkdir_recursive(project_path + "corpus/indexed/");

        // Dream directories (per-project consolidation)
        mkdir_recursive(project_path + "dreams/");
        mkdir_recursive(project_path + "dreams/pending/");
        mkdir_recursive(project_path + "dreams/archive/");

        // Create manifest
        ProjectManifest manifest;
        manifest.name = name;
        manifest.mode = mode;
        manifest.description = description;
        manifest.isolation = isolation;
        manifest.source_path = source_path;
        manifest.created_at = (int64_t)time(NULL);
        manifest.last_accessed = manifest.created_at;
        manifest.branches.push_back("main");
        manifest.current_branch = "main";

        // Graph stats
        manifest.node_count = 0;
        manifest.edge_count = 0;
        manifest.kv_segment_count = 0;
        manifest.operation_count = 0;

        // Corpus stats
        manifest.corpus_file_count = 0;
        manifest.corpus_indexed_count = 0;
        manifest.corpus_size_bytes = 0;

        // Dream stats
        manifest.dream_count_pending = 0;
        manifest.dream_count_archived = 0;
        manifest.last_dream_at = 0;
        manifest.dream_enabled = (mode != ProjectMode::CREATIVE);  // Dreams off for CREATIVE by default

        // Save manifest
        save_manifest(project_path, manifest);

        // Register project
        projects[name] = manifest;

        fprintf(stderr, "[PROJECT] Created project '%s' (mode=%s, isolation=%s)\n",
                name.c_str(), mode_to_string(mode), isolation_to_string(isolation));
        fprintf(stderr, "[PROJECT]   Path: %s\n", project_path.c_str());
        fprintf(stderr, "[PROJECT]   Corpus: %scorpus/\n", project_path.c_str());
        fprintf(stderr, "[PROJECT]   Dreams: %sdreams/ (enabled=%s)\n",
                project_path.c_str(), manifest.dream_enabled ? "yes" : "no");
        return true;
    }

    // Switch to a project
    bool switch_project(const std::string& name, const std::string& branch = "") {
        auto it = projects.find(name);
        if (it == projects.end()) {
            fprintf(stderr, "[PROJECT] Project not found: %s\n", name.c_str());
            return false;
        }

        // Save current project
        if (active_ctx) {
            save_project(active_ctx);
            if (active_ctx != global_ctx) {
                unload_project(active_ctx);
                delete active_ctx;
            }
            active_ctx = nullptr;
        }

        // Load new project
        active_ctx = load_project(name);
        if (!active_ctx) {
            fprintf(stderr, "[PROJECT] Failed to load project: %s\n", name.c_str());
            return false;
        }

        active_project = name;

        // Update last accessed
        projects[name].last_accessed = (int64_t)time(NULL);

        // Switch branch if specified
        if (!branch.empty() && active_ctx->git) {
            // TODO: Call zeta_git_checkout
        }

        fprintf(stderr, "[PROJECT] Switched to '%s' (nodes=%lld, edges=%lld)\n",
                name.c_str(),
                (long long)active_ctx->manifest.node_count,
                (long long)active_ctx->manifest.edge_count);
        return true;
    }

    // List all projects
    std::vector<ProjectManifest> list_projects() const {
        std::vector<ProjectManifest> result;
        for (const auto& kv : projects) {
            result.push_back(kv.second);
        }
        return result;
    }

    // Get current project status
    const ProjectManifest* get_active_manifest() const {
        if (active_project.empty()) return nullptr;
        auto it = projects.find(active_project);
        if (it == projects.end()) return nullptr;
        return &it->second;
    }

    // Delete a project
    bool delete_project(const std::string& name, bool confirm = false) {
        if (name == "global") {
            fprintf(stderr, "[PROJECT] Cannot delete global project\n");
            return false;
        }
        if (!confirm) {
            fprintf(stderr, "[PROJECT] Delete requires confirmation\n");
            return false;
        }

        auto it = projects.find(name);
        if (it == projects.end()) {
            fprintf(stderr, "[PROJECT] Project not found: %s\n", name.c_str());
            return false;
        }

        // If active, switch to global first
        if (active_project == name) {
            switch_project("global");
        }

        // Remove directory (dangerous - should add more safeguards)
        std::string project_path = base_path + name + "/";
        // TODO: Implement recursive delete or archive instead

        projects.erase(it);
        fprintf(stderr, "[PROJECT] Deleted project: %s\n", name.c_str());
        return true;
    }

    // ========================================================================
    // GRAPH ACCESS (for server integration)
    // ========================================================================

    zeta_dual_ctx_t* get_active_graph() {
        return active_ctx ? active_ctx->graph : nullptr;
    }

    zeta_git_ctx_t* get_active_git() {
        return active_ctx ? active_ctx->git : nullptr;
    }

    zeta_dual_ctx_t* get_global_graph() {
        return global_ctx ? global_ctx->graph : nullptr;
    }

    // Check if we can read from global (based on isolation policy)
    bool can_read_global() const {
        if (!active_ctx) return true;
        return active_ctx->manifest.isolation != IsolationLevel::FULL;
    }

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    void mkdir_recursive(const std::string& path) {
        std::string current;
        for (char c : path) {
            current += c;
            if (c == '/') {
                mkdir(current.c_str(), 0755);
            }
        }
        mkdir(path.c_str(), 0755);
    }

    void scan_projects() {
        projects.clear();

        // Scan global
        ProjectManifest global_manifest;
        if (load_manifest(global_path, global_manifest)) {
            projects["global"] = global_manifest;
        }

        // Scan projects directory
        DIR* dir = opendir(base_path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (entry->d_type != DT_DIR) continue;

            std::string project_path = base_path + entry->d_name + "/";
            ProjectManifest manifest;
            if (load_manifest(project_path, manifest)) {
                projects[manifest.name] = manifest;
            }
        }
        closedir(dir);
    }

    bool load_manifest(const std::string& project_path, ProjectManifest& manifest) {
        std::string manifest_path = project_path + "manifest.json";
        std::ifstream f(manifest_path);
        if (!f.is_open()) return false;

        std::stringstream buffer;
        buffer << f.rdbuf();
        std::string json = buffer.str();

        // Simple JSON parsing (should use proper JSON library in production)
        manifest.name = extract_json_string(json, "name");
        manifest.mode = string_to_mode(extract_json_string(json, "mode"));
        manifest.description = extract_json_string(json, "description");
        manifest.isolation = string_to_isolation(extract_json_string(json, "isolation"));
        manifest.source_path = extract_json_string(json, "source_path");
        manifest.created_at = extract_json_int(json, "created_at");
        manifest.last_accessed = extract_json_int(json, "last_accessed");
        manifest.current_branch = extract_json_string(json, "current_branch");
        manifest.node_count = extract_json_int(json, "node_count");
        manifest.edge_count = extract_json_int(json, "edge_count");
        manifest.kv_segment_count = extract_json_int(json, "kv_segment_count");
        manifest.operation_count = extract_json_int(json, "operation_count");

        // Parse branches array
        manifest.branches.clear();
        size_t arr_start = json.find("\"branches\"");
        if (arr_start != std::string::npos) {
            size_t bracket = json.find('[', arr_start);
            size_t end_bracket = json.find(']', bracket);
            if (bracket != std::string::npos && end_bracket != std::string::npos) {
                std::string arr = json.substr(bracket + 1, end_bracket - bracket - 1);
                size_t pos = 0;
                while ((pos = arr.find('"', pos)) != std::string::npos) {
                    size_t end = arr.find('"', pos + 1);
                    if (end != std::string::npos) {
                        manifest.branches.push_back(arr.substr(pos + 1, end - pos - 1));
                        pos = end + 1;
                    } else break;
                }
            }
        }
        if (manifest.branches.empty()) {
            manifest.branches.push_back("main");
        }

        // Corpus stats
        manifest.corpus_file_count = extract_json_int(json, "corpus_file_count");
        manifest.corpus_indexed_count = extract_json_int(json, "corpus_indexed_count");
        manifest.corpus_size_bytes = extract_json_int(json, "corpus_size_bytes");

        // Dream stats
        manifest.dream_count_pending = extract_json_int(json, "dream_count_pending");
        manifest.dream_count_archived = extract_json_int(json, "dream_count_archived");
        manifest.last_dream_at = extract_json_int(json, "last_dream_at");
        std::string dream_enabled_str = extract_json_string(json, "dream_enabled");
        manifest.dream_enabled = (dream_enabled_str != "false");  // Default true

        return !manifest.name.empty();
    }

    void save_manifest(const std::string& project_path, const ProjectManifest& manifest) {
        std::string manifest_path = project_path + "manifest.json";
        std::ofstream f(manifest_path);
        if (!f.is_open()) {
            fprintf(stderr, "[PROJECT] Failed to save manifest: %s\n", manifest_path.c_str());
            return;
        }

        f << "{\n";
        f << "  \"name\": \"" << manifest.name << "\",\n";
        f << "  \"mode\": \"" << mode_to_string(manifest.mode) << "\",\n";
        f << "  \"description\": \"" << manifest.description << "\",\n";
        f << "  \"isolation\": \"" << isolation_to_string(manifest.isolation) << "\",\n";
        f << "  \"source_path\": \"" << manifest.source_path << "\",\n";
        f << "  \"created_at\": " << manifest.created_at << ",\n";
        f << "  \"last_accessed\": " << manifest.last_accessed << ",\n";
        f << "  \"current_branch\": \"" << manifest.current_branch << "\",\n";
        f << "  \"node_count\": " << manifest.node_count << ",\n";
        f << "  \"edge_count\": " << manifest.edge_count << ",\n";
        f << "  \"kv_segment_count\": " << manifest.kv_segment_count << ",\n";
        f << "  \"operation_count\": " << manifest.operation_count << ",\n";

        // Corpus stats
        f << "  \"corpus_file_count\": " << manifest.corpus_file_count << ",\n";
        f << "  \"corpus_indexed_count\": " << manifest.corpus_indexed_count << ",\n";
        f << "  \"corpus_size_bytes\": " << manifest.corpus_size_bytes << ",\n";

        // Dream stats
        f << "  \"dream_count_pending\": " << manifest.dream_count_pending << ",\n";
        f << "  \"dream_count_archived\": " << manifest.dream_count_archived << ",\n";
        f << "  \"last_dream_at\": " << manifest.last_dream_at << ",\n";
        f << "  \"dream_enabled\": " << (manifest.dream_enabled ? "true" : "false") << ",\n";

        f << "  \"branches\": [";
        for (size_t i = 0; i < manifest.branches.size(); i++) {
            f << "\"" << manifest.branches[i] << "\"";
            if (i < manifest.branches.size() - 1) f << ", ";
        }
        f << "]\n";
        f << "}\n";

        f.close();
    }

    ProjectContext* load_project(const std::string& name) {
        auto it = projects.find(name);
        if (it == projects.end()) return nullptr;

        ProjectContext* ctx = new ProjectContext();
        ctx->manifest = it->second;
        ctx->path = (name == "global") ? global_path : (base_path + name + "/");

        // 4-Layer graph paths
        ctx->l1_path = ctx->path + "L1_nodes.bin";
        ctx->l2_path = ctx->path + "L2_kv_cache/";
        ctx->l3_path = ctx->path + "L3_edges.bin";
        ctx->l4_path = ctx->path + "L4_operations.bin";

        // Corpus paths
        ctx->corpus_path = ctx->path + "corpus/";
        ctx->corpus_raw = ctx->path + "corpus/raw/";
        ctx->corpus_indexed = ctx->path + "corpus/indexed/";

        // Dream paths
        ctx->dream_path = ctx->path + "dreams/";
        ctx->dream_pending = ctx->path + "dreams/pending/";
        ctx->dream_archive = ctx->path + "dreams/archive/";

        // Initialize dream state
        ctx->dream_state.enabled = ctx->manifest.dream_enabled;
        ctx->dream_state.last_dream_at = ctx->manifest.last_dream_at;
        ctx->dream_state.dream_count = ctx->manifest.dream_count_archived;
        ctx->dream_state.consecutive_discards = 0;

        ctx->graph = nullptr;  // Will be set by server integration
        ctx->git = nullptr;    // Will be set by server integration
        ctx->is_loaded = true;
        ctx->is_dirty = false;

        fprintf(stderr, "[PROJECT] Loaded project context: %s\n", name.c_str());
        fprintf(stderr, "[PROJECT]   Corpus: %s (%lld files)\n",
                ctx->corpus_path.c_str(), (long long)ctx->manifest.corpus_file_count);
        fprintf(stderr, "[PROJECT]   Dreams: %s (pending=%lld, archived=%lld)\n",
                ctx->dream_path.c_str(),
                (long long)ctx->manifest.dream_count_pending,
                (long long)ctx->manifest.dream_count_archived);
        return ctx;
    }

    void unload_project(ProjectContext* ctx) {
        if (!ctx) return;
        // Graph cleanup handled by server
        ctx->is_loaded = false;
        fprintf(stderr, "[PROJECT] Unloaded project: %s\n", ctx->manifest.name.c_str());
    }

    void save_project(ProjectContext* ctx) {
        if (!ctx || !ctx->is_dirty) return;

        // Update stats from graph
        if (ctx->graph) {
            // TODO: ctx->manifest.node_count = ctx->graph->num_nodes;
            // TODO: ctx->manifest.edge_count = ctx->graph->num_edges;
        }

        save_manifest(ctx->path, ctx->manifest);
        ctx->is_dirty = false;
        fprintf(stderr, "[PROJECT] Saved project: %s\n", ctx->manifest.name.c_str());
    }

    // Simple JSON helpers (should use proper library)
    std::string extract_json_string(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";

        pos = json.find('"', pos);
        if (pos == std::string::npos) return "";

        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";

        return json.substr(pos + 1, end - pos - 1);
    }

    int64_t extract_json_int(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return 0;

        pos = json.find(':', pos);
        if (pos == std::string::npos) return 0;

        // Skip whitespace
        while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) pos++;

        return std::strtoll(json.c_str() + pos, nullptr, 10);
    }
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

inline ProjectManager* g_project_manager = nullptr;

inline bool zeta_project_init(const std::string& zeta_home = "") {
    if (g_project_manager) return true;
    g_project_manager = new ProjectManager();
    return g_project_manager->init(zeta_home);
}

inline void zeta_project_shutdown() {
    if (g_project_manager) {
        delete g_project_manager;
        g_project_manager = nullptr;
    }
}

} // namespace zeta_project

#endif // ZETA_PROJECT_MANAGER_H
