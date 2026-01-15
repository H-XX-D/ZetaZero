// Z.E.T.A. Research-Graph Integration
// ============================================================================
// Connects Research Mode branching to GitGraph persistence and GraphKV caching
//
// Architecture:
//   Research Branch → GitGraph Node (with block persistence)
//   Research Tension → GitGraph Edge (with tension metadata)
//   Branch Summaries → GraphKV segments (for resumption)
//   Validated Facts → Salience-weighted graph commits
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
// ============================================================================

#ifndef ZETA_RESEARCH_GRAPH_INTEGRATION_H
#define ZETA_RESEARCH_GRAPH_INTEGRATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <system_error>
#include "zeta-research.h"
#include "zeta-branching-engine.h"
// Forward declarations for existing headers
// #include "zeta-gitgraph-persist.h"
// #include "zeta-graph-kv-integration.h"

namespace zeta_research_graph {

// ============================================================================
// CONFIGURATION
// ============================================================================

struct GraphIntegrationConfig {
    std::string gitgraph_root = "/mnt/GitGraph";
    std::string research_subdir = "research";
    std::string graphkv_root = "/mnt/GitGraph/graph_kv";
    
    // Thresholds for committing research results
    float min_summary_entropy_reduction = 0.1f;  // Only commit if reduced entropy
    float min_fact_confidence = 0.7f;            // Only commit high-confidence facts
    int min_supporting_branches = 2;             // Facts need multiple branch support
    
    // KV cache settings
    int max_kv_segments_per_session = 100;
    int kv_segment_size = 512;                   // Tokens per segment
};

static GraphIntegrationConfig g_config;

// ============================================================================
// RESEARCH → GITGRAPH NODE MAPPING
// ============================================================================

struct ResearchNode {
    std::string node_id;           // GitGraph node ID
    std::string branch_id;         // Research branch ID  
    std::string block_hash;        // Content-addressed storage
    
    // Metadata
    std::string topic;
    float entropy;
    int concept_count;
    bool is_summary;               // True if this is a compressed summary
    
    // Lineage
    std::string parent_node_id;    // Parent in research tree
    std::vector<std::string> child_node_ids;
};

// Map research branch to GitGraph node
inline ResearchNode branch_to_node(
    const zeta_research::Branch& branch,
    const std::string& session_id,
    const std::string& parent_id = ""
) {
    ResearchNode node;
    
    // Generate deterministic node ID from branch
    node.node_id = "research_" + session_id + "_" + branch.id;
    node.branch_id = branch.id;
    
    // Build content block from Concept objects
    std::string content;
    for (const auto& c : branch.concepts) {
        content += c.content + "\n---\n";  // Use content field
    }
    
    // Hash for content-addressed storage
    // (Simplified - real implementation would use proper hash)
    size_t h = 0;
    for (char ch : content) h = h * 31 + ch;
    char hash_buf[32];
    snprintf(hash_buf, sizeof(hash_buf), "%016zx", h);
    node.block_hash = hash_buf;
    
    node.topic = branch.concepts.empty() ? "" : branch.concepts[0].content.substr(0, 100);
    node.entropy = branch.entropy;
    node.concept_count = branch.concepts.size();
    node.is_summary = branch.compressible;
    node.parent_node_id = parent_id;
    
    return node;
}

// BranchingEngine branch -> ResearchNode (for RESEARCH mode wiring)
inline ResearchNode branch_to_node(
    const zeta_branching::Branch& branch,
    const std::string& session_id,
    const std::string& parent_id = ""
) {
    ResearchNode node;

    node.node_id = "research_" + session_id + "_" + branch.id;
    node.branch_id = branch.id;

    std::string content;
    if (!branch.summary.empty()) {
        content = branch.summary;
    } else {
        for (const auto& c : branch.concepts) {
            content += c + "\n---\n";
        }
    }

    size_t h = 0;
    for (char ch : content) h = h * 31 + ch;
    char hash_buf[32];
    snprintf(hash_buf, sizeof(hash_buf), "%016zx", h);
    node.block_hash = hash_buf;

    node.topic = content.empty() ? "" : content.substr(0, 100);
    node.entropy = branch.entropy;
    node.concept_count = branch.concepts.size();
    node.is_summary = !branch.summary.empty();
    node.parent_node_id = parent_id.empty() ? "" : ("research_" + session_id + "_" + parent_id);

    return node;
}

// ============================================================================
// RESEARCH → GITGRAPH EDGE MAPPING
// ============================================================================

struct ResearchEdge {
    std::string edge_id;
    std::string from_node_id;
    std::string to_node_id;
    std::string edge_type;         // "tension", "merge", "fork", "supports", "contradicts"
    float weight;                  // Strength of relationship
    std::string metadata_json;     // Additional context
};

// Forward declaration
static std::string escape_json(const std::string& s);
// Map research tension to GitGraph edge
inline ResearchEdge tension_to_edge(
    const zeta_research::Tension& tension,
    const std::string& session_id,
    const std::string& node_a_id,
    const std::string& node_b_id
) {
    ResearchEdge edge;
    
    edge.edge_id = "edge_" + tension.id;
    edge.from_node_id = node_a_id;
    edge.to_node_id = node_b_id;
    edge.edge_type = tension.resolved ? "resolved_tension" : "active_tension";
    edge.weight = tension.severity;
    
    // Build metadata JSON
    edge.metadata_json = "{";
    edge.metadata_json += "\"description\": \"" + escape_json(tension.description.substr(0, 500)) + "\", ";
    edge.metadata_json += "\"severity\": " + std::to_string(tension.severity) + ", ";
    edge.metadata_json += "\"resolved\": " + std::string(tension.resolved ? "true" : "false");
    if (tension.resolved && !tension.resolution_method.empty()) {
        edge.metadata_json += ", \"resolution_method\": \"" + escape_json(tension.resolution_method.substr(0, 200)) + "\"";
    }
    edge.metadata_json += "}";
    
    return edge;
}

// BranchingEngine tension -> ResearchEdge (for RESEARCH mode wiring)
inline ResearchEdge tension_to_edge(
    const zeta_branching::Tension& tension,
    const std::string& session_id,
    const std::string& node_a_id,
    const std::string& node_b_id
) {
    ResearchEdge edge;

    edge.edge_id = "edge_" + tension.id;
    edge.from_node_id = node_a_id;
    edge.to_node_id = node_b_id;
    edge.edge_type = tension.resolved ? "resolved_tension" : "active_tension";
    edge.weight = tension.severity;

    edge.metadata_json = "{";
    edge.metadata_json += "\"description\": \"" + escape_json(tension.description.substr(0, 500)) + "\", ";
    edge.metadata_json += "\"severity\": " + std::to_string(tension.severity) + ", ";
    edge.metadata_json += "\"resolved\": " + std::string(tension.resolved ? "true" : "false");
    if (tension.resolved && !tension.resolution.empty()) {
        edge.metadata_json += ", \"resolution\": \"" + escape_json(tension.resolution.substr(0, 200)) + "\"";
    }
    edge.metadata_json += "}";

    return edge;
}



static std::string escape_json(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else result += c;
    }
    return result;
}

// ============================================================================
// GRAPH-KV INTEGRATION FOR RESEARCH STATE
// ============================================================================

struct ResearchKVSegment {
    std::string segment_id;
    std::string session_id;
    std::string branch_id;
    int segment_index;
    
    // KV cache data (opaque blob from llama.cpp)
    std::vector<uint8_t> kv_data;
    
    // Metadata
    int token_start;
    int token_end;
    float relevance_score;         // How relevant for resumption
};

// Capture KV state for a research branch
inline ResearchKVSegment capture_branch_kv(
    const zeta_research::Branch& branch,
    const std::string& session_id,
    const void* kv_cache,          // llama_kv_cache*
    int start_pos,
    int end_pos
) {
    ResearchKVSegment seg;
    
    seg.segment_id = "kv_" + session_id + "_" + branch.id + "_" + std::to_string(start_pos);
    seg.session_id = session_id;
    seg.branch_id = branch.id;
    seg.segment_index = start_pos / g_config.kv_segment_size;
    seg.token_start = start_pos;
    seg.token_end = end_pos;
    
    // TODO: Actually capture KV cache data
    // seg.kv_data = zeta_gkv_capture_range(kv_cache, start_pos, end_pos);
    
    // Relevance based on branch entropy (low entropy = more stable = more relevant)
    seg.relevance_score = 1.0f - std::min(branch.entropy, 1.0f);
    
    return seg;
}

// ============================================================================
// SESSION PERSISTENCE
// ============================================================================

class ResearchGraphBridge {
private:
    std::string session_id_;
    std::string gitgraph_root_;
    
    // Mapping from branch IDs to node IDs
    std::unordered_map<std::string, std::string> branch_to_node_map_;
    
    // Queued writes (batched for efficiency)
    std::vector<ResearchNode> pending_nodes_;
    std::vector<ResearchEdge> pending_edges_;
    std::vector<ResearchKVSegment> pending_kv_segments_;

public:
    void init(const std::string& session_id, const std::string& gitgraph_root) {
        session_id_ = session_id;
        gitgraph_root_ = gitgraph_root;
        branch_to_node_map_.clear();
        pending_nodes_.clear();
        pending_edges_.clear();
        pending_kv_segments_.clear();
    }
    
    // ========================================================================
    // INCREMENTAL PERSISTENCE
    // Called during research iterations
    // ========================================================================
    
    void persist_branch(const zeta_research::Branch& branch, const std::string& parent_branch_id = "") {
        std::string parent_node_id;
        if (!parent_branch_id.empty()) {
            auto it = branch_to_node_map_.find(parent_branch_id);
            if (it != branch_to_node_map_.end()) {
                parent_node_id = it->second;
            }
        }
        
        ResearchNode node = branch_to_node(branch, session_id_, parent_node_id);
        branch_to_node_map_[branch.id] = node.node_id;
        pending_nodes_.push_back(node);
    }

    void persist_branch(const zeta_branching::Branch& branch) {
        ResearchNode node = branch_to_node(branch, session_id_, branch.parent_id);
        branch_to_node_map_[branch.id] = node.node_id;
        pending_nodes_.push_back(node);
    }
    
    void persist_tension(const zeta_research::Tension& tension) {
        // Get node IDs for the branches involved
        auto it_a = branch_to_node_map_.find(tension.branch_a);
        auto it_b = branch_to_node_map_.find(tension.branch_b);
        
        if (it_a == branch_to_node_map_.end() || it_b == branch_to_node_map_.end()) {
            // One or both branches not yet persisted
            return;
        }
        
        ResearchEdge edge = tension_to_edge(tension, session_id_, it_a->second, it_b->second);
        pending_edges_.push_back(edge);
    }

    void persist_tension(const zeta_branching::Tension& tension) {
        auto it_a = branch_to_node_map_.find(tension.branch_a);
        auto it_b = branch_to_node_map_.find(tension.branch_b);

        if (it_a == branch_to_node_map_.end() || it_b == branch_to_node_map_.end()) {
            return;
        }

        ResearchEdge edge = tension_to_edge(tension, session_id_, it_a->second, it_b->second);
        pending_edges_.push_back(edge);
    }
    
    void persist_kv_segment(const ResearchKVSegment& seg) {
        pending_kv_segments_.push_back(seg);
    }
    
    // ========================================================================
    // BATCH FLUSH
    // Write all pending items to storage
    // ========================================================================
    
    void flush() {
        flush_nodes();
        flush_edges();
        flush_kv_segments();
    }
    
private:
    void flush_nodes() {
        if (pending_nodes_.empty()) return;
        
        std::string nodes_dir = gitgraph_root_ + "/" + g_config.research_subdir + "/nodes";
        std::error_code ec;
        std::filesystem::create_directories(nodes_dir, ec);
        if (ec) {
            fprintf(stderr, "[RESEARCH-GRAPH] Failed to create nodes dir: %s (%s)\n",
                    nodes_dir.c_str(), ec.message().c_str());
        }
        
        for (const auto& node : pending_nodes_) {
            std::string node_file = nodes_dir + "/" + node.node_id + ".json";
            
            std::string json = "{\n";
            json += "  \"node_id\": \"" + node.node_id + "\",\n";
            json += "  \"branch_id\": \"" + node.branch_id + "\",\n";
            json += "  \"block_hash\": \"" + node.block_hash + "\",\n";
            json += "  \"topic\": \"" + escape_json(node.topic) + "\",\n";
            json += "  \"entropy\": " + std::to_string(node.entropy) + ",\n";
            json += "  \"concept_count\": " + std::to_string(node.concept_count) + ",\n";
            json += "  \"is_summary\": " + std::string(node.is_summary ? "true" : "false") + ",\n";
            json += "  \"parent_node_id\": \"" + node.parent_node_id + "\"\n";
            json += "}\n";
            
            std::ofstream f(node_file, std::ios::binary);
            if (f.is_open()) {
                f << json;
                f.close();
            } else {
                fprintf(stderr, "[RESEARCH-GRAPH] Failed to write node: %s\n", node_file.c_str());
            }
            
            fprintf(stderr, "[RESEARCH-GRAPH] Node: %s -> %s\n", 
                    node.branch_id.c_str(), node.node_id.c_str());
        }
        
        pending_nodes_.clear();
    }
    
    void flush_edges() {
        if (pending_edges_.empty()) return;
        
        std::string edges_dir = gitgraph_root_ + "/" + g_config.research_subdir + "/edges";
        std::error_code ec;
        std::filesystem::create_directories(edges_dir, ec);
        if (ec) {
            fprintf(stderr, "[RESEARCH-GRAPH] Failed to create edges dir: %s (%s)\n",
                    edges_dir.c_str(), ec.message().c_str());
        }
        
        for (const auto& edge : pending_edges_) {
            std::string edge_file = edges_dir + "/" + edge.edge_id + ".json";
            
            std::string json = "{\n";
            json += "  \"edge_id\": \"" + edge.edge_id + "\",\n";
            json += "  \"from_node\": \"" + edge.from_node_id + "\",\n";
            json += "  \"to_node\": \"" + edge.to_node_id + "\",\n";
            json += "  \"type\": \"" + edge.edge_type + "\",\n";
            json += "  \"weight\": " + std::to_string(edge.weight) + ",\n";
            json += "  \"metadata\": " + edge.metadata_json + "\n";
            json += "}\n";
            
            std::ofstream f(edge_file, std::ios::binary);
            if (f.is_open()) {
                f << json;
                f.close();
            } else {
                fprintf(stderr, "[RESEARCH-GRAPH] Failed to write edge: %s\n", edge_file.c_str());
            }
            
            fprintf(stderr, "[RESEARCH-GRAPH] Edge: %s [%s] %s (%.2f)\n",
                    edge.from_node_id.c_str(), edge.edge_type.c_str(),
                    edge.to_node_id.c_str(), edge.weight);
        }
        
        pending_edges_.clear();
    }
    
    void flush_kv_segments() {
        if (pending_kv_segments_.empty()) return;
        
        std::string kv_dir = g_config.graphkv_root + "/research";
        std::error_code ec;
        std::filesystem::create_directories(kv_dir, ec);
        if (ec) {
            fprintf(stderr, "[RESEARCH-GKV] Failed to create kv dir: %s (%s)\n",
                    kv_dir.c_str(), ec.message().c_str());
        }
        
        for (const auto& seg : pending_kv_segments_) {
            std::string kv_file = kv_dir + "/" + seg.segment_id + ".lkv";
            
            // TODO: Actually write KV cache data
            // zeta_gkv_write_segment(kv_file, seg.kv_data);
            
            fprintf(stderr, "[RESEARCH-GKV] Segment: %s (tokens %d-%d, relevance %.2f)\n",
                    seg.segment_id.c_str(), seg.token_start, seg.token_end,
                    seg.relevance_score);
        }
        
        pending_kv_segments_.clear();
    }
    
public:
    // ========================================================================
    // SESSION RESUMPTION
    // Restore research state from GitGraph
    // ========================================================================
    
    bool load_session(const std::string& session_id, zeta_research::ResearchState& state) {
        session_id_ = session_id;
        
        // Load session manifest
        std::string manifest_file = gitgraph_root_ + "/" + g_config.research_subdir + 
                                   "/session_" + session_id + ".json";
        
        // TODO: Read manifest and restore:
        // 1. Branch nodes → state.active_branches
        // 2. Tension edges → state.tensions
        // 3. KV segments → inject into cache
        
        fprintf(stderr, "[RESEARCH-GRAPH] Would load session: %s\n", session_id.c_str());
        
        return false;  // Not implemented yet
    }
    
    // ========================================================================
    // FACT EXTRACTION & COMMIT
    // Convert validated summaries to permanent knowledge graph facts
    // ========================================================================
    
    void commit_validated_facts(const zeta_research::ResearchState& state) {
        for (const auto& summary : state.buffer.summaries) {
            if (!summary.is_valid()) continue;
            if (summary.entropy_reduction < g_config.min_summary_entropy_reduction) continue;
            
            // Extract facts from validated summary
            std::vector<std::string> facts = extract_facts(summary.text);
            
            for (const auto& fact : facts) {
                // Commit to main knowledge graph
                // gitgraph_commit_fact(fact, summary.entropy_reduction, session_id_);
                
                fprintf(stderr, "[RESEARCH-GRAPH] FACT: %s\n", fact.substr(0, 100).c_str());
            }
        }
    }
    
private:
    std::vector<std::string> extract_facts(const std::string& summary) {
        // Simple sentence-based extraction
        // Real implementation would use NER/fact extraction model
        
        std::vector<std::string> facts;
        std::string current;
        
        for (char c : summary) {
            if (c == '.' || c == '!' || c == '?') {
                if (!current.empty() && current.length() > 20) {
                    // Trim and add
                    while (!current.empty() && isspace(current[0])) current.erase(0, 1);
                    facts.push_back(current);
                }
                current.clear();
            } else {
                current += c;
            }
        }
        
        return facts;
    }
};

// ============================================================================
// GLOBAL BRIDGE INSTANCE
// ============================================================================

static ResearchGraphBridge g_research_bridge;

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

inline void init_research_graph(const std::string& session_id) {
    g_research_bridge.init(session_id, g_config.gitgraph_root);
}

inline void persist_research_state(const zeta_research::ResearchState& state) {
    // Persist all active branches
    for (const auto& branch : state.active_branches) {
        g_research_bridge.persist_branch(branch);
    }
    
    // Persist all tensions
    for (const auto& tension : state.tensions) {
        g_research_bridge.persist_tension(tension);
    }
    
    // Flush to storage
    g_research_bridge.flush();
}

inline void persist_branching_state(const zeta_branching::BranchingEngine& engine) {
    for (const auto& branch : engine.branches()) {
        g_research_bridge.persist_branch(branch);
    }

    for (const auto& tension : engine.tensions()) {
        g_research_bridge.persist_tension(tension);
    }

    g_research_bridge.flush();
}

inline void commit_research_conclusions(const zeta_research::ResearchState& state) {
    g_research_bridge.commit_validated_facts(state);
}

} // namespace zeta_research_graph

#endif // ZETA_RESEARCH_GRAPH_INTEGRATION_H
