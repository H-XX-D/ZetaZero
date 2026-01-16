// Z.E.T.A. Research Project Management
// ============================================================================
// Each research project gets its own isolated knowledge graph for materials.
// This prevents cross-contamination between different research domains and
// allows focused analysis of specific paper sets.
//
// Architecture:
//   ResearchProject -> owns zeta_dual_ctx_t (isolated graph)
//                   -> tracks ingested materials
//                   -> maintains research sessions
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_RESEARCH_PROJECT_H
#define ZETA_RESEARCH_PROJECT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <ctime>
#include <sstream>
#include "zeta-dual-process.h"

namespace zeta_research_project {

// ============================================================================
// RESEARCH ENTITY LABELS (use NODE_ENTITY/NODE_FACT with these labels)
// ============================================================================

// Entity labels for structured knowledge graph
constexpr const char* LABEL_PAPER        = "paper";        // Research paper
constexpr const char* LABEL_DATASET      = "dataset";      // Dataset/corpus
constexpr const char* LABEL_METHOD       = "method";       // Algorithm/architecture
constexpr const char* LABEL_BENCHMARK    = "benchmark";    // Evaluation task
constexpr const char* LABEL_RESULT       = "result";       // Quantitative result
constexpr const char* LABEL_AUTHOR       = "author";       // Paper author
constexpr const char* LABEL_METRIC       = "metric";       // Evaluation metric
constexpr const char* LABEL_ARCHITECTURE = "architecture"; // Model specs (layers, dims, heads)
constexpr const char* LABEL_HYPERPARAMS  = "hyperparams";  // Training config (lr, batch, epochs)
constexpr const char* LABEL_COMPUTE      = "compute";      // Training resources (GPUs, hours, FLOPs)
constexpr const char* LABEL_CONTRIBUTION = "contribution"; // Key novelty claim
constexpr const char* LABEL_LIMITATION   = "limitation";   // Acknowledged weakness
constexpr const char* LABEL_ABLATION     = "ablation";     // Ablation study finding
constexpr const char* LABEL_PRIOR_WORK   = "prior_work";   // Referenced baseline/method

// Relationship edge types (using value field for specifics)
constexpr const char* REL_PROPOSES     = "proposes";     // paper -> method
constexpr const char* REL_EVALUATES    = "evaluates";    // method -> benchmark
constexpr const char* REL_USES         = "uses";         // paper -> dataset
constexpr const char* REL_ACHIEVES     = "achieves";     // method -> result
constexpr const char* REL_OUTPERFORMS  = "outperforms";  // method -> method
constexpr const char* REL_CITES        = "cites";        // paper -> paper
constexpr const char* REL_EXTENDS      = "extends";      // method -> prior_work
constexpr const char* REL_AUTHORED_BY  = "authored_by";  // paper -> author
constexpr const char* REL_HAS_ARCH     = "has_arch";     // method -> architecture
constexpr const char* REL_TRAINED_WITH = "trained_with"; // method -> hyperparams
constexpr const char* REL_REQUIRED     = "required";     // method -> compute
constexpr const char* REL_CLAIMS       = "claims";       // paper -> contribution
constexpr const char* REL_ACKNOWLEDGES = "acknowledges"; // paper -> limitation
constexpr const char* REL_ABLATES      = "ablates";      // paper -> ablation

// ============================================================================
// EXTRACTED ENTITY (structured extraction result)
// ============================================================================

struct ExtractedEntity {
    std::string label;       // LABEL_* constant
    std::string name;        // Entity name
    std::string details;     // Additional info (size, metrics, etc.)
    float salience;          // Importance 0-1
    int64_t node_id;         // Assigned after graph insertion
};

struct ExtractedRelation {
    std::string source_name;
    std::string target_name;
    std::string relation;    // REL_* constant
    float weight;
    int64_t edge_id;         // Assigned after graph insertion
};

// ============================================================================
// RESEARCH MATERIAL
// ============================================================================

struct ResearchMaterial {
    std::string id;
    std::string title;
    std::string source;        // e.g., "arXiv:2412.18288"
    std::string content;       // Full text or abstract
    std::string material_type; // "paper", "dataset", "notes", "webpage"
    time_t ingested_at;
    int node_count;            // Nodes created from this material

    // Structured extraction results
    std::vector<ExtractedEntity> entities;
    std::vector<ExtractedRelation> relations;
};

// ============================================================================
// RESEARCH PROJECT
// ============================================================================

struct ResearchProject {
    std::string project_id;
    std::string name;
    std::string description;
    time_t created_at;
    time_t last_accessed;

    // Isolated knowledge graph for this project
    zeta_dual_ctx_t* graph;

    // Ingested materials
    std::vector<ResearchMaterial> materials;

    // Research session IDs
    std::vector<std::string> sessions;

    // Stats
    int total_queries;
    int total_facts_extracted;
    int total_entities;
    int total_relations;

    // Entity name -> node_id mapping for relationship linking
    std::unordered_map<std::string, int64_t> entity_index;

    ResearchProject() : graph(nullptr), created_at(0), last_accessed(0),
                        total_queries(0), total_facts_extracted(0),
                        total_entities(0), total_relations(0) {}

    ~ResearchProject() {
        if (graph) {
            free(graph);
            graph = nullptr;
        }
    }
};

// ============================================================================
// PROJECT MANAGER
// ============================================================================

class ResearchProjectManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ResearchProject>> projects_;
    std::mutex mutex_;
    std::string storage_root_;

    // Generate unique project ID
    std::string generate_id() {
        char buf[32];
        snprintf(buf, sizeof(buf), "proj_%lx_%d",
                (long)time(nullptr), rand() % 10000);
        return buf;
    }

public:
    void init(const std::string& storage_root) {
        storage_root_ = storage_root;
        fprintf(stderr, "[RESEARCH-PROJECT] Initialized with root: %s\n", storage_root.c_str());
    }

    // ========================================================================
    // PROJECT CRUD
    // ========================================================================

    // Create a new research project with isolated graph
    std::string create_project(const std::string& name, const std::string& description = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        auto project = std::make_unique<ResearchProject>();
        project->project_id = generate_id();
        project->name = name;
        project->description = description;
        project->created_at = time(nullptr);
        project->last_accessed = project->created_at;

        // Create isolated graph for this project (graph-only, no 3B model)
        project->graph = (zeta_dual_ctx_t*)calloc(1, sizeof(zeta_dual_ctx_t));
        if (project->graph) {
            // Initialize graph structure without 3B model
            project->graph->model_subconscious = nullptr;  // No 3B for project graphs
            project->graph->next_node_id = 1;
            project->graph->next_edge_id = 1;
            project->graph->num_nodes = 0;
            project->graph->num_edges = 0;
            snprintf(project->graph->storage_dir, sizeof(project->graph->storage_dir),
                     "%s/%s", storage_root_.c_str(), project->project_id.c_str());
            fprintf(stderr, "[RESEARCH-PROJECT] Created project '%s' with ID: %s\n",
                    name.c_str(), project->project_id.c_str());
        } else {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Failed to allocate graph for '%s'\n",
                    name.c_str());
            return "";
        }

        std::string id = project->project_id;
        projects_[id] = std::move(project);
        return id;
    }

    // Get project by ID
    ResearchProject* get_project(const std::string& project_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projects_.find(project_id);
        if (it != projects_.end()) {
            it->second->last_accessed = time(nullptr);
            return it->second.get();
        }
        return nullptr;
    }

    // List all projects
    std::vector<std::string> list_projects() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        for (const auto& kv : projects_) {
            ids.push_back(kv.first);
        }
        return ids;
    }

    // Delete project
    bool delete_project(const std::string& project_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projects_.find(project_id);
        if (it != projects_.end()) {
            fprintf(stderr, "[RESEARCH-PROJECT] Deleted project: %s\n", project_id.c_str());
            projects_.erase(it);
            return true;
        }
        return false;
    }

    // ========================================================================
    // MATERIAL INGESTION
    // ========================================================================

    // Ingest research material into project graph
    bool ingest_material(
        const std::string& project_id,
        const std::string& title,
        const std::string& content,
        const std::string& source = "",
        const std::string& material_type = "paper"
    ) {
        ResearchProject* project = get_project(project_id);
        if (!project || !project->graph) {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Project not found: %s\n", project_id.c_str());
            return false;
        }

        // Create material record
        ResearchMaterial mat;
        mat.id = "mat_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand() % 1000);
        mat.title = title;
        mat.source = source;
        mat.content = content;
        mat.material_type = material_type;
        mat.ingested_at = time(nullptr);
        mat.node_count = 0;

        // Extract facts from content and add to project graph
        // Simple fact extraction - split by sentences
        std::vector<std::string> facts;
        std::string current;
        for (char c : content) {
            if (c == '.' || c == '!' || c == '?') {
                if (current.length() > 30) {  // Minimum fact length
                    // Trim whitespace
                    size_t start = current.find_first_not_of(" \t\n\r");
                    size_t end = current.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        facts.push_back(current.substr(start, end - start + 1));
                    }
                }
                current.clear();
            } else {
                current += c;
            }
        }

        // Add facts as nodes to project graph using zeta_create_node
        for (const auto& fact : facts) {
            int64_t node_id = zeta_create_node(
                project->graph,
                NODE_FACT,        // type
                "research_fact",  // label
                fact.c_str(),     // value
                0.8f              // salience
            );

            if (node_id >= 0) {
                mat.node_count++;
            }
        }

        // Also add a summary node for the material itself
        std::string summary = title + " [" + source + "]";
        int64_t summary_id = zeta_create_node(
            project->graph,
            NODE_FACT,
            "research_material",
            summary.c_str(),
            0.9f
        );

        if (summary_id >= 0) {
            mat.node_count++;
        }

        project->materials.push_back(mat);
        project->total_facts_extracted += mat.node_count;

        fprintf(stderr, "[RESEARCH-PROJECT] Ingested '%s' into '%s': %d facts extracted\n",
                title.c_str(), project_id.c_str(), mat.node_count);

        return true;
    }

    // ========================================================================
    // GRAPH QUERY
    // ========================================================================

    // Surface relevant context from project graph
    std::string surface_context(const std::string& project_id, const std::string& query) {
        ResearchProject* project = get_project(project_id);
        if (!project || !project->graph) {
            return "";
        }

        project->total_queries++;

        // Use existing context surfacing
        zeta_surfaced_context_t surfaced;
        zeta_surface_context(project->graph, query.c_str(), &surfaced);

        if (surfaced.num_nodes > 0) {
            return std::string(surfaced.formatted_context);
        }

        return "";
    }

    // ========================================================================
    // JSON SERIALIZATION
    // ========================================================================

    std::string project_to_json(const ResearchProject* project) {
        if (!project) return "{}";

        std::string json = "{\n";
        json += "  \"project_id\": \"" + project->project_id + "\",\n";
        json += "  \"name\": \"" + project->name + "\",\n";
        json += "  \"description\": \"" + project->description + "\",\n";
        json += "  \"created_at\": " + std::to_string(project->created_at) + ",\n";
        json += "  \"graph_nodes\": " + std::to_string(project->graph ? project->graph->num_nodes : 0) + ",\n";
        json += "  \"graph_edges\": " + std::to_string(project->graph ? project->graph->num_edges : 0) + ",\n";
        json += "  \"materials_count\": " + std::to_string(project->materials.size()) + ",\n";
        json += "  \"total_queries\": " + std::to_string(project->total_queries) + ",\n";
        json += "  \"total_facts\": " + std::to_string(project->total_facts_extracted) + ",\n";
        json += "  \"materials\": [\n";

        for (size_t i = 0; i < project->materials.size(); i++) {
            const auto& mat = project->materials[i];
            json += "    {\"id\": \"" + mat.id + "\", ";
            json += "\"title\": \"" + mat.title + "\", ";
            json += "\"source\": \"" + mat.source + "\", ";
            json += "\"type\": \"" + mat.material_type + "\", ";
            json += "\"nodes\": " + std::to_string(mat.node_count) + "}";
            if (i < project->materials.size() - 1) json += ",";
            json += "\n";
        }

        json += "  ]\n";
        json += "}";

        return json;
    }

    std::string list_projects_json() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string json = "{\"projects\": [\n";
        bool first = true;
        for (const auto& kv : projects_) {
            if (!first) json += ",\n";
            first = false;
            json += "  {\"id\": \"" + kv.second->project_id + "\", ";
            json += "\"name\": \"" + kv.second->name + "\", ";
            json += "\"materials\": " + std::to_string(kv.second->materials.size()) + ", ";
            json += "\"nodes\": " + std::to_string(kv.second->graph ? kv.second->graph->num_nodes : 0) + "}";
        }
        json += "\n]}";

        return json;
    }

    // ========================================================================
    // PERSISTENCE - Save/Load projects to disk
    // ========================================================================

    // Escape string for JSON
    std::string escape_json(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 10);
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c >= 0 && c < 32) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }
        return result;
    }

    // Save project to JSON file
    bool save_project(const std::string& project_id, const std::string& filepath) {
        ResearchProject* project = get_project(project_id);
        if (!project || !project->graph) {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Cannot save - project not found: %s\n", project_id.c_str());
            return false;
        }

        FILE* f = fopen(filepath.c_str(), "w");
        if (!f) {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Cannot open file for writing: %s\n", filepath.c_str());
            return false;
        }

        fprintf(f, "{\n");
        fprintf(f, "  \"project_id\": \"%s\",\n", project->project_id.c_str());
        fprintf(f, "  \"name\": \"%s\",\n", escape_json(project->name).c_str());
        fprintf(f, "  \"description\": \"%s\",\n", escape_json(project->description).c_str());
        fprintf(f, "  \"created_at\": %ld,\n", (long)project->created_at);
        fprintf(f, "  \"total_queries\": %d,\n", project->total_queries);
        fprintf(f, "  \"total_entities\": %d,\n", project->total_entities);
        fprintf(f, "  \"total_relations\": %d,\n", project->total_relations);

        // Save materials
        fprintf(f, "  \"materials\": [\n");
        for (size_t i = 0; i < project->materials.size(); i++) {
            const auto& mat = project->materials[i];
            fprintf(f, "    {\"id\": \"%s\", \"title\": \"%s\", \"source\": \"%s\", \"type\": \"%s\", \"nodes\": %d}%s\n",
                    mat.id.c_str(), escape_json(mat.title).c_str(), escape_json(mat.source).c_str(),
                    mat.material_type.c_str(), mat.node_count,
                    i < project->materials.size() - 1 ? "," : "");
        }
        fprintf(f, "  ],\n");

        // Save graph nodes
        fprintf(f, "  \"nodes\": [\n");
        zeta_dual_ctx_t* g = project->graph;
        bool first_node = true;
        for (int i = 0; i < g->num_nodes; i++) {
            if (!g->nodes[i].is_active) continue;
            if (!first_node) fprintf(f, ",\n");
            first_node = false;
            fprintf(f, "    {\"id\": %lld, \"type\": %d, \"label\": \"%s\", \"value\": \"%s\", \"salience\": %.3f}",
                    (long long)g->nodes[i].node_id,
                    (int)g->nodes[i].type,
                    escape_json(g->nodes[i].label).c_str(),
                    escape_json(g->nodes[i].value).c_str(),
                    g->nodes[i].salience);
        }
        fprintf(f, "\n  ],\n");

        // Save graph edges
        fprintf(f, "  \"edges\": [\n");
        bool first_edge = true;
        for (int i = 0; i < g->num_edges; i++) {
            if (!first_edge) fprintf(f, ",\n");
            first_edge = false;
            fprintf(f, "    {\"id\": %lld, \"source\": %lld, \"target\": %lld, \"type\": %d, \"weight\": %.3f, \"belief\": %.3f}",
                    (long long)g->edges[i].edge_id,
                    (long long)g->edges[i].source_id,
                    (long long)g->edges[i].target_id,
                    (int)g->edges[i].type,
                    g->edges[i].weight,
                    g->edges[i].belief);
        }
        fprintf(f, "\n  ],\n");

        // Save entity index
        fprintf(f, "  \"entity_index\": {\n");
        bool first_entity = true;
        for (const auto& kv : project->entity_index) {
            if (!first_entity) fprintf(f, ",\n");
            first_entity = false;
            fprintf(f, "    \"%s\": %lld", escape_json(kv.first).c_str(), (long long)kv.second);
        }
        fprintf(f, "\n  }\n");

        fprintf(f, "}\n");
        fclose(f);

        fprintf(stderr, "[RESEARCH-PROJECT] Saved project '%s' to %s (%d nodes, %d edges)\n",
                project->name.c_str(), filepath.c_str(), g->num_nodes, g->num_edges);
        return true;
    }

    // Load project from JSON file (simplified parser for our known format)
    std::string load_project(const std::string& filepath) {
        FILE* f = fopen(filepath.c_str(), "r");
        if (!f) {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Cannot open file: %s\n", filepath.c_str());
            return "";
        }

        // Read entire file
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::string content(size, '\0');
        if (fread(&content[0], 1, size, f) != (size_t)size) {
            fclose(f);
            return "";
        }
        fclose(f);

        // Simple JSON parsing - extract key fields
        auto extract_string = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\": \"";
            size_t pos = content.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            size_t end = content.find("\"", pos);
            if (end == std::string::npos) return "";
            return content.substr(pos, end - pos);
        };

        auto extract_int = [&](const std::string& key) -> int64_t {
            std::string search = "\"" + key + "\": ";
            size_t pos = content.find(search);
            if (pos == std::string::npos) return 0;
            pos += search.length();
            return atoll(content.c_str() + pos);
        };

        // Create new project
        std::string name = extract_string("name");
        std::string description = extract_string("description");
        std::string project_id = create_project(name, description);

        if (project_id.empty()) {
            fprintf(stderr, "[RESEARCH-PROJECT] ERROR: Failed to create project during load\n");
            return "";
        }

        ResearchProject* project = get_project(project_id);
        if (!project) return "";

        project->total_queries = (int)extract_int("total_queries");
        project->total_entities = (int)extract_int("total_entities");
        project->total_relations = (int)extract_int("total_relations");

        // Parse nodes array
        size_t nodes_start = content.find("\"nodes\": [");
        size_t nodes_end = content.find("],", nodes_start);
        if (nodes_start != std::string::npos && nodes_end != std::string::npos) {
            std::string nodes_section = content.substr(nodes_start, nodes_end - nodes_start);

            size_t pos = 0;
            while ((pos = nodes_section.find("{\"id\":", pos)) != std::string::npos) {
                // Extract node fields
                size_t node_end = nodes_section.find("}", pos);
                if (node_end == std::string::npos) break;
                std::string node_str = nodes_section.substr(pos, node_end - pos + 1);

                // Parse node fields
                int64_t node_id = 0;
                int type = 0;
                float salience = 0.8f;
                char label[128] = "";
                char value[512] = "";

                sscanf(node_str.c_str(), "{\"id\": %lld, \"type\": %d", &node_id, &type);

                size_t label_pos = node_str.find("\"label\": \"");
                if (label_pos != std::string::npos) {
                    label_pos += 10;
                    size_t label_end = node_str.find("\"", label_pos);
                    if (label_end != std::string::npos) {
                        strncpy(label, node_str.substr(label_pos, label_end - label_pos).c_str(), 127);
                    }
                }

                size_t value_pos = node_str.find("\"value\": \"");
                if (value_pos != std::string::npos) {
                    value_pos += 10;
                    size_t value_end = node_str.find("\"", value_pos);
                    if (value_end != std::string::npos) {
                        strncpy(value, node_str.substr(value_pos, value_end - value_pos).c_str(), 511);
                    }
                }

                size_t sal_pos = node_str.find("\"salience\": ");
                if (sal_pos != std::string::npos) {
                    salience = atof(node_str.c_str() + sal_pos + 12);
                }

                // Create node in graph
                zeta_create_node(project->graph, (zeta_node_type_t)type, label, value, salience);

                pos = node_end + 1;
            }
        }

        // Parse edges array
        size_t edges_start = content.find("\"edges\": [");
        size_t edges_end = content.find("],", edges_start);
        if (edges_start != std::string::npos && edges_end != std::string::npos) {
            std::string edges_section = content.substr(edges_start, edges_end - edges_start);

            size_t pos = 0;
            while ((pos = edges_section.find("{\"id\":", pos)) != std::string::npos) {
                size_t edge_end = edges_section.find("}", pos);
                if (edge_end == std::string::npos) break;
                std::string edge_str = edges_section.substr(pos, edge_end - pos + 1);

                int64_t edge_id = 0, source_id = 0, target_id = 0;
                int type = 0;
                float weight = 0.5f, belief = 0.0f;

                sscanf(edge_str.c_str(), "{\"id\": %lld, \"source\": %lld, \"target\": %lld, \"type\": %d, \"weight\": %f, \"belief\": %f",
                       &edge_id, &source_id, &target_id, &type, &weight, &belief);

                // Create edge in graph
                zeta_create_edge(project->graph, source_id, target_id, (zeta_edge_type_t)type, weight);

                pos = edge_end + 1;
            }
        }

        fprintf(stderr, "[RESEARCH-PROJECT] Loaded project '%s' from %s (%d nodes, %d edges)\n",
                project->name.c_str(), filepath.c_str(),
                project->graph->num_nodes, project->graph->num_edges);

        return project_id;
    }

    // List saved projects in a directory
    std::vector<std::string> list_saved_projects(const std::string& dir) {
        std::vector<std::string> files;
        // Simple implementation - would use dirent.h in full version
        return files;
    }
};

// Global manager instance
static ResearchProjectManager g_project_manager;

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

inline void init_research_projects(const std::string& storage_root) {
    g_project_manager.init(storage_root);
}

inline std::string create_research_project(const std::string& name, const std::string& desc = "") {
    return g_project_manager.create_project(name, desc);
}

inline ResearchProject* get_research_project(const std::string& id) {
    return g_project_manager.get_project(id);
}

inline bool ingest_to_project(const std::string& project_id, const std::string& title,
                              const std::string& content, const std::string& source = "") {
    return g_project_manager.ingest_material(project_id, title, content, source);
}

inline std::string surface_project_context(const std::string& project_id, const std::string& query) {
    return g_project_manager.surface_context(project_id, query);
}

// ============================================================================
// STRUCTURED EXTRACTION PROMPTS
// ============================================================================

// Paper extraction prompt - comprehensive structured extraction
inline std::string get_paper_extraction_prompt(const std::string& content) {
    return R"(Extract ALL structured information from this research paper. Output in this exact format:

AUTHORS:
- [author_name] ([affiliation if mentioned])

METHODS:
- [method_name]: [brief description]

ARCHITECTURE:
- [method_name]: [layers], [dimensions], [heads], [parameters] (e.g. "GPT-3: 96 layers, 12288 dims, 96 heads, 175B params")

DATASETS:
- [dataset_name]: [size/details if mentioned]

BENCHMARKS:
- [benchmark_name]: [task type]

HYPERPARAMETERS:
- [method_name]: lr=[value], batch=[value], epochs=[value], optimizer=[name]

COMPUTE:
- [method_name]: [GPUs/TPUs], [training time], [FLOPs if mentioned]

RESULTS:
- [method] achieves [metric_value] on [benchmark/dataset]

COMPARISONS:
- [method_a] outperforms [method_b] by [margin] on [task]

CONTRIBUTIONS:
- [key novelty or claim made by this paper]

PRIOR_WORK:
- [method_name]: extends/builds on [prior_method] from [prior_paper if mentioned]

LIMITATIONS:
- [acknowledged weakness or constraint]

ABLATIONS:
- Removing [component] causes [effect] (e.g. "Removing layer norm causes 2.3% accuracy drop")

CRITICAL RULES:
1. Only include items EXPLICITLY stated in the text with real values
2. NEVER write "not mentioned", "N/A", "not specified", or similar placeholders
3. If a section has no data, skip it entirely - do not output the section header
4. Be specific with numbers when available
5. Output ONLY extracted data, no commentary or verification

Paper content:
)" + content.substr(0, 4000) + "\n\nExtracted information:\n";
}

// Dataset extraction prompt - extracts structured dataset info
inline std::string get_dataset_extraction_prompt(const std::string& content) {
    return R"(Extract structured information from this dataset description. Output in this exact format:

NAME: [dataset name]
SIZE: [number of samples, tokens, or size in GB]
TASK: [primary task type: language modeling, translation, QA, classification, etc.]
SPLITS: [train/val/test sizes if mentioned]
METRICS: [evaluation metrics used]
DOMAIN: [text domain: web, books, scientific, code, etc.]
PAPERS: [papers that introduced or commonly use this dataset]

Only include fields with explicit information. Be specific with numbers.

Dataset description:
)" + content.substr(0, 2000) + "\n\nExtracted information:\n";
}

// ============================================================================
// EXTRACTION RESULT PARSING
// ============================================================================

// Parse paper extraction output into entities and relations
inline void parse_paper_extraction(
    const std::string& output,
    const std::string& paper_name,
    const std::string& source,
    std::vector<ExtractedEntity>& entities,
    std::vector<ExtractedRelation>& relations
) {
    std::istringstream stream(output);
    std::string line;
    std::string current_section;
    std::string main_method;  // Track primary method for linking arch/hyperparams

    while (std::getline(stream, line)) {
        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Detect section headers
        if (line.find("AUTHORS:") == 0) { current_section = "author"; continue; }
        if (line.find("METHODS:") == 0) { current_section = "method"; continue; }
        if (line.find("ARCHITECTURE:") == 0) { current_section = "architecture"; continue; }
        if (line.find("DATASETS:") == 0) { current_section = "dataset"; continue; }
        if (line.find("BENCHMARKS:") == 0) { current_section = "benchmark"; continue; }
        if (line.find("HYPERPARAMETERS:") == 0) { current_section = "hyperparams"; continue; }
        if (line.find("COMPUTE:") == 0) { current_section = "compute"; continue; }
        if (line.find("RESULTS:") == 0) { current_section = "result"; continue; }
        if (line.find("COMPARISONS:") == 0) { current_section = "comparison"; continue; }
        if (line.find("CONTRIBUTIONS:") == 0) { current_section = "contribution"; continue; }
        if (line.find("PRIOR_WORK:") == 0) { current_section = "prior_work"; continue; }
        if (line.find("LIMITATIONS:") == 0) { current_section = "limitation"; continue; }
        if (line.find("ABLATIONS:") == 0) { current_section = "ablation"; continue; }

        // Parse list items
        if (line.size() > 2 && line[0] == '-' && line[1] == ' ') {
            std::string item = line.substr(2);

            // Skip commentary/verification lines (e.g. "**Authors**: The authors are...")
            if (item.size() > 2 && item[0] == '*' && item[1] == '*') continue;
            if (item.find("correctly") != std::string::npos) continue;
            if (item.find("are listed") != std::string::npos) continue;

            // Skip placeholder entries - these add no value
            std::string item_lower = item;
            for (auto& c : item_lower) c = std::tolower(c);
            if (item_lower.find("not mentioned") != std::string::npos) continue;
            if (item_lower.find("not explicitly") != std::string::npos) continue;
            if (item_lower.find("[not ") != std::string::npos) continue;
            if (item_lower.find("(not ") != std::string::npos) continue;
            if (item_lower.find("none mentioned") != std::string::npos) continue;
            if (item_lower.find("n/a") != std::string::npos) continue;
            if (item_lower.find("not applicable") != std::string::npos) continue;
            if (item_lower.find("not specified") != std::string::npos) continue;
            if (item_lower.find("not provided") != std::string::npos) continue;
            if (item_lower.find("no specific") != std::string::npos) continue;
            if (item_lower.find("(no ") != std::string::npos) continue;
            if (item_lower.find("not stated") != std::string::npos) continue;
            if (item_lower.find("not given") != std::string::npos) continue;
            if (item_lower.find("not available") != std::string::npos) continue;
            if (item_lower.find("none provided") != std::string::npos) continue;
            if (item_lower.find("none given") != std::string::npos) continue;

            size_t colon = item.find(':');
            std::string name = (colon != std::string::npos) ? item.substr(0, colon) : item;
            std::string details = (colon != std::string::npos) ? item.substr(colon + 1) : "";

            // Trim name
            size_t name_end = name.find_last_not_of(" \t");
            if (name_end != std::string::npos) name = name.substr(0, name_end + 1);
            size_t name_start = name.find_first_not_of(" \t");
            if (name_start != std::string::npos) name = name.substr(name_start);

            if (current_section == "author") {
                ExtractedEntity e;
                e.label = LABEL_AUTHOR;
                e.name = name;
                e.details = details + " [" + source + "]";
                e.salience = 0.7f;
                entities.push_back(e);

                // Relation: paper AUTHORED_BY author
                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = name;
                r.relation = REL_AUTHORED_BY;
                r.weight = 0.9f;
                relations.push_back(r);
            }
            else if (current_section == "method") {
                if (main_method.empty()) main_method = name;  // First method is main

                ExtractedEntity e;
                e.label = LABEL_METHOD;
                e.name = name;
                e.details = details + " [" + source + "]";
                e.salience = 0.9f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = name;
                r.relation = REL_PROPOSES;
                r.weight = 0.95f;
                relations.push_back(r);
            }
            else if (current_section == "architecture") {
                ExtractedEntity e;
                e.label = LABEL_ARCHITECTURE;
                e.name = name + " arch";
                e.details = details + " [" + source + "]";
                e.salience = 0.85f;
                entities.push_back(e);

                // Link to method
                ExtractedRelation r;
                r.source_name = name;
                r.target_name = name + " arch";
                r.relation = REL_HAS_ARCH;
                r.weight = 0.9f;
                relations.push_back(r);
            }
            else if (current_section == "dataset") {
                ExtractedEntity e;
                e.label = LABEL_DATASET;
                e.name = name;
                e.details = details + " [" + source + "]";
                e.salience = 0.85f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = name;
                r.relation = REL_USES;
                r.weight = 0.9f;
                relations.push_back(r);
            }
            else if (current_section == "benchmark") {
                ExtractedEntity e;
                e.label = LABEL_BENCHMARK;
                e.name = name;
                e.details = details + " [" + source + "]";
                e.salience = 0.85f;
                entities.push_back(e);
            }
            else if (current_section == "hyperparams") {
                ExtractedEntity e;
                e.label = LABEL_HYPERPARAMS;
                e.name = name + " training";
                e.details = details + " [" + source + "]";
                e.salience = 0.75f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = name;
                r.target_name = name + " training";
                r.relation = REL_TRAINED_WITH;
                r.weight = 0.85f;
                relations.push_back(r);
            }
            else if (current_section == "compute") {
                ExtractedEntity e;
                e.label = LABEL_COMPUTE;
                e.name = name + " compute";
                e.details = details + " [" + source + "]";
                e.salience = 0.8f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = name;
                r.target_name = name + " compute";
                r.relation = REL_REQUIRED;
                r.weight = 0.85f;
                relations.push_back(r);
            }
            else if (current_section == "result") {
                ExtractedEntity e;
                e.label = LABEL_RESULT;
                e.name = item.substr(0, 50);
                e.details = item + " [" + source + "]";
                e.salience = 0.9f;
                entities.push_back(e);
            }
            else if (current_section == "comparison") {
                size_t outperforms = item.find("outperforms");
                if (outperforms != std::string::npos) {
                    std::string method_a = item.substr(0, outperforms);
                    size_t end = method_a.find_last_not_of(" \t");
                    if (end != std::string::npos) method_a = method_a.substr(0, end + 1);

                    std::string rest = item.substr(outperforms + 11);
                    size_t by_pos = rest.find(" by ");
                    std::string method_b = (by_pos != std::string::npos) ? rest.substr(0, by_pos) : rest;
                    size_t start_b = method_b.find_first_not_of(" \t");
                    if (start_b != std::string::npos) method_b = method_b.substr(start_b);

                    ExtractedRelation r;
                    r.source_name = method_a;
                    r.target_name = method_b;
                    r.relation = REL_OUTPERFORMS;
                    r.weight = 0.85f;
                    relations.push_back(r);
                }

                ExtractedEntity e;
                e.label = LABEL_RESULT;
                e.name = "comparison";
                e.details = item + " [" + source + "]";
                e.salience = 0.85f;
                entities.push_back(e);
            }
            else if (current_section == "contribution") {
                ExtractedEntity e;
                e.label = LABEL_CONTRIBUTION;
                e.name = item.substr(0, 60);
                e.details = item + " [" + source + "]";
                e.salience = 0.9f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = item.substr(0, 60);
                r.relation = REL_CLAIMS;
                r.weight = 0.9f;
                relations.push_back(r);
            }
            else if (current_section == "prior_work") {
                // Parse "method: extends prior_method"
                ExtractedEntity e;
                e.label = LABEL_PRIOR_WORK;
                e.name = name;
                e.details = details + " [" + source + "]";
                e.salience = 0.75f;
                entities.push_back(e);

                // Look for "extends" or "builds on"
                size_t extends_pos = details.find("extends");
                size_t builds_pos = details.find("builds on");
                if (extends_pos != std::string::npos || builds_pos != std::string::npos) {
                    size_t pos = (extends_pos != std::string::npos) ? extends_pos + 8 : builds_pos + 10;
                    std::string prior = details.substr(pos);
                    size_t prior_start = prior.find_first_not_of(" \t");
                    if (prior_start != std::string::npos) prior = prior.substr(prior_start);
                    size_t prior_end = prior.find_first_of(" \t,");
                    if (prior_end != std::string::npos) prior = prior.substr(0, prior_end);

                    if (!prior.empty()) {
                        ExtractedRelation r;
                        r.source_name = name;
                        r.target_name = prior;
                        r.relation = REL_EXTENDS;
                        r.weight = 0.85f;
                        relations.push_back(r);
                    }
                }
            }
            else if (current_section == "limitation") {
                ExtractedEntity e;
                e.label = LABEL_LIMITATION;
                e.name = item.substr(0, 60);
                e.details = item + " [" + source + "]";
                e.salience = 0.7f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = item.substr(0, 60);
                r.relation = REL_ACKNOWLEDGES;
                r.weight = 0.8f;
                relations.push_back(r);
            }
            else if (current_section == "ablation") {
                ExtractedEntity e;
                e.label = LABEL_ABLATION;
                e.name = item.substr(0, 60);
                e.details = item + " [" + source + "]";
                e.salience = 0.8f;
                entities.push_back(e);

                ExtractedRelation r;
                r.source_name = paper_name;
                r.target_name = item.substr(0, 60);
                r.relation = REL_ABLATES;
                r.weight = 0.8f;
                relations.push_back(r);
            }
        }
    }
}

// Parse dataset extraction output
inline void parse_dataset_extraction(
    const std::string& output,
    const std::string& source,
    std::vector<ExtractedEntity>& entities,
    std::vector<ExtractedRelation>& relations
) {
    std::istringstream stream(output);
    std::string line;

    std::string ds_name, ds_size, ds_task, ds_splits, ds_metrics, ds_domain;

    while (std::getline(stream, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Parse key: value pairs
        if (line.find("NAME:") == 0) ds_name = line.substr(5);
        else if (line.find("SIZE:") == 0) ds_size = line.substr(5);
        else if (line.find("TASK:") == 0) ds_task = line.substr(5);
        else if (line.find("SPLITS:") == 0) ds_splits = line.substr(7);
        else if (line.find("METRICS:") == 0) ds_metrics = line.substr(8);
        else if (line.find("DOMAIN:") == 0) ds_domain = line.substr(7);
    }

    // Trim all
    auto trim = [](std::string& s) {
        size_t start = s.find_first_not_of(" \t");
        size_t end = s.find_last_not_of(" \t");
        if (start != std::string::npos) s = s.substr(start, end - start + 1);
        else s = "";
    };
    trim(ds_name); trim(ds_size); trim(ds_task);
    trim(ds_splits); trim(ds_metrics); trim(ds_domain);

    if (!ds_name.empty()) {
        // Main dataset entity
        ExtractedEntity ds;
        ds.label = LABEL_DATASET;
        ds.name = ds_name;
        ds.details = "";
        if (!ds_size.empty()) ds.details += "Size: " + ds_size + ". ";
        if (!ds_task.empty()) ds.details += "Task: " + ds_task + ". ";
        if (!ds_splits.empty()) ds.details += "Splits: " + ds_splits + ". ";
        if (!ds_domain.empty()) ds.details += "Domain: " + ds_domain + ". ";
        ds.details += "[" + source + "]";
        ds.salience = 0.9f;
        entities.push_back(ds);

        // Create metric entities if present
        if (!ds_metrics.empty()) {
            ExtractedEntity m;
            m.label = LABEL_METRIC;
            m.name = ds_metrics;
            m.details = "Used for " + ds_name + " [" + source + "]";
            m.salience = 0.7f;
            entities.push_back(m);

            // Relation: dataset EVALUATES metric
            ExtractedRelation r;
            r.source_name = ds_name;
            r.target_name = ds_metrics;
            r.relation = REL_EVALUATES;
            r.weight = 0.8f;
            relations.push_back(r);
        }

        // Create benchmark entity if task is clear
        if (!ds_task.empty()) {
            ExtractedEntity b;
            b.label = LABEL_BENCHMARK;
            b.name = ds_name + " (" + ds_task + ")";
            b.details = "Task: " + ds_task + " [" + source + "]";
            b.salience = 0.8f;
            entities.push_back(b);
        }
    }
}

} // namespace zeta_research_project

#endif // ZETA_RESEARCH_PROJECT_H
