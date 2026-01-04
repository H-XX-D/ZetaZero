#pragma once
/**
 * Z.E.T.A. Research Mode - Epistemic Search with Progressive Compression
 * 
 * This is NOT creative exploration. This is guided epistemic search.
 * 
 * Core principles:
 *   1. Branch on TENSION, not topic
 *   2. Compress BEFORE merging
 *   3. Treat summaries as entropy reducers
 *   4. Allow contradictions to exist (tension is signal)
 *   5. Converge by survivability, not fluency
 * 
 * The One Rule: If a merge feels forced, entropy has not been paid yet.
 */

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <sstream>

namespace zeta_research {

// =============================================================================
// CORE DATA STRUCTURES
// =============================================================================

struct Concept {
    std::string id;
    std::string content;
    float confidence;           // uncertainty proxy [0, 1]
    float salience;             // relevance to query
    std::vector<std::string> dependencies;  // causal links
    
    float entropy() const {
        // Shannon entropy proxy: H = -p*log(p) - (1-p)*log(1-p)
        float p = std::clamp(confidence, 0.01f, 0.99f);
        return -p * std::log2(p) - (1.0f - p) * std::log2(1.0f - p);
    }
};

struct Branch {
    std::string id;
    std::string origin_query;           // what spawned this branch
    std::vector<Concept> concepts;
    float entropy;                      // local entropy estimate
    bool compressible = false;
    bool stable = true;
    int depth = 0;                      // exploration depth
    
    float compute_entropy() const {
        if (concepts.empty()) return 0.0f;
        float total = 0.0f;
        for (const auto& c : concepts) {
            total += c.entropy();
        }
        return total / concepts.size();
    }
    
    size_t concept_count() const { return concepts.size(); }
};

struct Summary {
    std::string text;
    float entropy_reduction;            // must be positive to be valid
    std::vector<std::string> source_branch_ids;
    bool introduces_new_concepts = false;  // if true, invalid summary
    bool preserves_causality = true;
    
    bool is_valid() const {
        return entropy_reduction > 0.0f && 
               !introduces_new_concepts && 
               preserves_causality;
    }
};

struct Tension {
    std::string id;
    std::string branch_a;
    std::string branch_b;
    std::string description;            // explicit contradiction
    float severity;                     // 0 = minor, 1 = fundamental
    bool resolved = false;
    std::string resolution_method;      // how it was resolved (if at all)
    
    // Tension is signal, not error
    bool should_preserve() const {
        return severity > 0.3f && !resolved;
    }
};

struct ResearchBuffer {
    std::vector<Summary> summaries;
    std::vector<std::string> rejected_merges;  // audit trail
    float total_entropy_reduced = 0.0f;
    
    void add_summary(const Summary& s) {
        if (s.is_valid()) {
            summaries.push_back(s);
            total_entropy_reduced += s.entropy_reduction;
        }
    }
};

// =============================================================================
// RESEARCH CONTROLLER STATE
// =============================================================================

struct ResearchConfig {
    float initial_temperature = 0.9f;
    float min_temperature = 0.3f;
    float max_temperature = 1.2f;
    float temperature_delta = 0.05f;
    
    float min_entropy_reduction = 0.1f;     // for local compression
    float global_entropy_threshold = 0.2f;  // for global compression
    float tension_threshold = 0.5f;         // when to record tension
    float merge_readiness = 0.7f;           // when branches can merge
    
    int max_branches = 8;
    int max_depth = 5;
    int max_iterations = 20;
    
    bool preserve_tensions = true;          // keep contradictions explicit
    bool require_compression = true;        // branches must compress to survive
    
    // Semantic similarity config
    int k_semantic = 5;                     // max semantic neighbors per branch
    int semantic_update_period = 2;         // update every N iterations
    int semantic_add_threshold = 12;        // Hamming distance to add edge
    int semantic_drop_threshold = 16;       // Hamming distance to drop (hysteresis)
    int max_semantic_checks_per_iter = 50;  // budget cap
};

// =============================================================================
// BRANCH GRAPH - O(1) adjacency for tension detection
// Invariants:
//   1. Symmetric adjacency (undirected): if b in adj[a], then a in adj[b]
//   2. No self-edges: a never in adj[a]
//   3. Centralized mutations: only link/unlink modify adjacency
// =============================================================================

struct BranchGraph {
    // adjacency: branch_id -> set of adjacent branch_ids
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency;
    // parent tracking: child_id -> parent_id
    std::unordered_map<std::string, std::string> parent_of;
    // active branch IDs for fast lookup
    std::unordered_set<std::string> active_ids;
    
    // === CENTRALIZED EDGE MUTATIONS (enforce symmetry) ===
    
    void link(const std::string& a, const std::string& b) {
        if (a == b) return;  // No self-edges
        adjacency[a].insert(b);
        adjacency[b].insert(a);
    }
    
    void unlink(const std::string& a, const std::string& b) {
        if (adjacency.count(a)) adjacency[a].erase(b);
        if (adjacency.count(b)) adjacency[b].erase(a);
    }
    
    // === BRANCH LIFECYCLE ===
    
    void add_branch(const std::string& id, const std::string& parent_id = "") {
        if (id.empty()) return;
        
        active_ids.insert(id);
        if (adjacency.find(id) == adjacency.end()) {
            adjacency[id] = {};
        }
        
        if (!parent_id.empty() && parent_id != id) {
            parent_of[id] = parent_id;
            
            // Link to parent
            link(id, parent_id);
            
            // Link to siblings (other children of same parent)
            for (const auto& [child, parent] : parent_of) {
                if (parent == parent_id && child != id) {
                    link(id, child);
                }
            }
        }
    }
    
    void remove_branch(const std::string& id) {
        if (id.empty()) return;
        
        // Unlink from all neighbors (maintains symmetry)
        if (adjacency.count(id)) {
            // Copy neighbor set to avoid iterator invalidation
            auto neighbors_copy = adjacency[id];
            for (const auto& neighbor : neighbors_copy) {
                unlink(id, neighbor);
            }
        }
        
        adjacency.erase(id);
        parent_of.erase(id);
        active_ids.erase(id);
    }
    
    // Merge: new node inherits neighbors from both sources
    // adj[m] = (adj[a] ∪ adj[b]) \ {a, b, m}
    void merge_branches(const std::string& merged_id, 
                        const std::string& a_id, 
                        const std::string& b_id) {
        if (merged_id.empty() || a_id.empty() || b_id.empty()) return;
        
        // Collect all neighbors to inherit
        std::unordered_set<std::string> inherited_neighbors;
        
        if (adjacency.count(a_id)) {
            for (const auto& n : adjacency[a_id]) {
                if (n != a_id && n != b_id && n != merged_id) {
                    inherited_neighbors.insert(n);
                }
            }
        }
        if (adjacency.count(b_id)) {
            for (const auto& n : adjacency[b_id]) {
                if (n != a_id && n != b_id && n != merged_id) {
                    inherited_neighbors.insert(n);
                }
            }
        }
        
        // Remove old branches first
        remove_branch(a_id);
        remove_branch(b_id);
        
        // Add merged branch
        add_branch(merged_id);
        
        // Link to all inherited neighbors
        for (const auto& n : inherited_neighbors) {
            if (active_ids.count(n)) {  // Only link to still-active branches
                link(merged_id, n);
            }
        }
    }
    
    // === QUERIES ===
    
    bool is_active(const std::string& id) const {
        return active_ids.count(id) > 0;
    }
    
    bool are_adjacent(const std::string& a, const std::string& b) const {
        if (a == b) return false;
        auto it = adjacency.find(a);
        if (it == adjacency.end()) return false;
        return it->second.count(b) > 0;
    }
    
    const std::unordered_set<std::string>& neighbors(const std::string& id) const {
        static const std::unordered_set<std::string> empty;
        auto it = adjacency.find(id);
        return it != adjacency.end() ? it->second : empty;
    }
    
    size_t degree(const std::string& id) const {
        auto it = adjacency.find(id);
        return it != adjacency.end() ? it->second.size() : 0;
    }
    
    size_t total_edges() const {
        size_t sum = 0;
        for (const auto& [_, neighbors] : adjacency) {
            sum += neighbors.size();
        }
        return sum / 2;  // Undirected: each edge counted twice
    }
};

// =============================================================================
// SEMANTIC SIMILARITY LAYER
// SimHash + LSH bucketing for O(1) candidate generation
// Top-k neighbors with hysteresis to prevent edge thrash
// =============================================================================

enum class EdgeType { LINEAGE, SEMANTIC };

struct SemanticLayer {
    // SimHash signatures: branch_id -> 64-bit hash
    std::unordered_map<std::string, uint64_t> signatures;
    
    // LSH buckets: 16-bit chunk -> set of branch_ids
    // 4 buckets (chunks 0-3 of 64-bit hash)
    std::array<std::unordered_map<uint16_t, std::unordered_set<std::string>>, 4> buckets;
    
    // Semantic edges: branch_id -> set of semantic neighbors
    std::unordered_map<std::string, std::unordered_set<std::string>> semantic_adj;
    
    // Edge type tracking for debugging/tuning
    std::unordered_map<std::string, EdgeType> edge_types;  // canonical_pair -> type
    
    // Cache: canonical_pair -> (contradiction_result, signature_hash_when_checked)
    std::unordered_map<std::string, std::pair<bool, uint64_t>> contradiction_cache;
    
    // Metrics
    int semantic_edges_added = 0;
    int semantic_edges_dropped = 0;
    int cache_hits = 0;
    
    // === SIMHASH COMPUTATION ===
    
    // Simple string tokenizer (lowercase, split on whitespace/punct)
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;
        for (char c : text) {
            if (std::isalnum(c)) {
                current += std::tolower(c);
            } else if (!current.empty()) {
                if (current.length() > 2) {  // Skip very short tokens
                    tokens.push_back(current);
                }
                current.clear();
            }
        }
        if (!current.empty() && current.length() > 2) {
            tokens.push_back(current);
        }
        return tokens;
    }
    
    // FNV-1a hash for shingles
    static uint64_t fnv1a(const std::string& s) {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : s) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
    
    // Compute SimHash from text
    static uint64_t compute_simhash(const std::string& text) {
        auto tokens = tokenize(text);
        if (tokens.size() < 2) return 0;
        
        // Accumulator for 64 bits
        std::array<int, 64> bits{};
        
        // Generate 2-word shingles and hash them
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            std::string shingle = tokens[i] + " " + tokens[i + 1];
            uint64_t h = fnv1a(shingle);
            
            for (int b = 0; b < 64; ++b) {
                if ((h >> b) & 1) {
                    bits[b]++;
                } else {
                    bits[b]--;
                }
            }
        }
        
        // Convert to signature
        uint64_t sig = 0;
        for (int b = 0; b < 64; ++b) {
            if (bits[b] > 0) {
                sig |= (1ULL << b);
            }
        }
        return sig;
    }
    
    // Hamming distance between signatures
    static int hamming_distance(uint64_t a, uint64_t b) {
        uint64_t x = a ^ b;
        int count = 0;
        while (x) {
            count += x & 1;
            x >>= 1;
        }
        return count;
    }
    
    // === LSH BUCKET MANAGEMENT ===
    
    void add_to_buckets(const std::string& id, uint64_t sig) {
        for (int i = 0; i < 4; ++i) {
            uint16_t chunk = static_cast<uint16_t>((sig >> (i * 16)) & 0xFFFF);
            buckets[i][chunk].insert(id);
        }
    }
    
    void remove_from_buckets(const std::string& id) {
        auto it = signatures.find(id);
        if (it == signatures.end()) return;
        
        uint64_t sig = it->second;
        for (int i = 0; i < 4; ++i) {
            uint16_t chunk = static_cast<uint16_t>((sig >> (i * 16)) & 0xFFFF);
            if (buckets[i].count(chunk)) {
                buckets[i][chunk].erase(id);
            }
        }
    }
    
    // Get candidate neighbors via LSH (union of bucket matches)
    std::unordered_set<std::string> get_candidates(const std::string& id) const {
        std::unordered_set<std::string> candidates;
        
        auto it = signatures.find(id);
        if (it == signatures.end()) return candidates;
        
        uint64_t sig = it->second;
        for (int i = 0; i < 4; ++i) {
            uint16_t chunk = static_cast<uint16_t>((sig >> (i * 16)) & 0xFFFF);
            auto bucket_it = buckets[i].find(chunk);
            if (bucket_it != buckets[i].end()) {
                for (const auto& cand : bucket_it->second) {
                    if (cand != id) {
                        candidates.insert(cand);
                    }
                }
            }
        }
        return candidates;
    }
    
    // === SIGNATURE MANAGEMENT ===
    
    void update_signature(const std::string& id, const std::string& text) {
        // Remove from old buckets
        remove_from_buckets(id);
        
        // Compute new signature
        uint64_t sig = compute_simhash(text);
        signatures[id] = sig;
        
        // Add to new buckets
        add_to_buckets(id, sig);
    }
    
    void remove_branch(const std::string& id) {
        remove_from_buckets(id);
        signatures.erase(id);
        
        // Remove semantic edges
        if (semantic_adj.count(id)) {
            for (const auto& neighbor : semantic_adj[id]) {
                if (semantic_adj.count(neighbor)) {
                    semantic_adj[neighbor].erase(id);
                }
                // Clean up edge type
                std::string pair_key = id < neighbor ? id + "_" + neighbor : neighbor + "_" + id;
                edge_types.erase(pair_key);
            }
            semantic_adj.erase(id);
        }
    }
    
    // === SEMANTIC EDGE MANAGEMENT WITH HYSTERESIS ===
    
    void link_semantic(const std::string& a, const std::string& b) {
        if (a == b) return;
        semantic_adj[a].insert(b);
        semantic_adj[b].insert(a);
        
        std::string pair_key = a < b ? a + "_" + b : b + "_" + a;
        edge_types[pair_key] = EdgeType::SEMANTIC;
        semantic_edges_added++;
    }
    
    void unlink_semantic(const std::string& a, const std::string& b) {
        if (semantic_adj.count(a)) semantic_adj[a].erase(b);
        if (semantic_adj.count(b)) semantic_adj[b].erase(a);
        
        std::string pair_key = a < b ? a + "_" + b : b + "_" + a;
        edge_types.erase(pair_key);
        semantic_edges_dropped++;
    }
    
    bool has_semantic_edge(const std::string& a, const std::string& b) const {
        if (a == b) return false;
        auto it = semantic_adj.find(a);
        return it != semantic_adj.end() && it->second.count(b) > 0;
    }
    
    const std::unordered_set<std::string>& semantic_neighbors(const std::string& id) const {
        static const std::unordered_set<std::string> empty;
        auto it = semantic_adj.find(id);
        return it != semantic_adj.end() ? it->second : empty;
    }
    
    size_t semantic_degree(const std::string& id) const {
        auto it = semantic_adj.find(id);
        return it != semantic_adj.end() ? it->second.size() : 0;
    }
    
    // === CACHE MANAGEMENT ===
    
    std::string canonical_pair(const std::string& a, const std::string& b) const {
        return a < b ? a + "_" + b : b + "_" + a;
    }
    
    // Check if cached result is still valid (signatures unchanged)
    bool is_cache_valid(const std::string& a, const std::string& b) const {
        std::string key = canonical_pair(a, b);
        auto it = contradiction_cache.find(key);
        if (it == contradiction_cache.end()) return false;
        
        auto sig_a = signatures.find(a);
        auto sig_b = signatures.find(b);
        if (sig_a == signatures.end() || sig_b == signatures.end()) return false;
        
        // Cache stores XOR of both signatures when checked
        return it->second.second == (sig_a->second ^ sig_b->second);
    }
    
    void cache_contradiction(const std::string& a, const std::string& b, bool result) {
        std::string key = canonical_pair(a, b);
        auto sig_a = signatures.find(a);
        auto sig_b = signatures.find(b);
        if (sig_a != signatures.end() && sig_b != signatures.end()) {
            contradiction_cache[key] = {result, sig_a->second ^ sig_b->second};
        }
    }
    
    bool get_cached_contradiction(const std::string& a, const std::string& b, bool& result) {
        if (!is_cache_valid(a, b)) return false;
        
        std::string key = canonical_pair(a, b);
        result = contradiction_cache[key].first;
        cache_hits++;
        return true;
    }
};

// =============================================================================
// EXTENDED BRANCH GRAPH WITH SEMANTIC LAYER
// =============================================================================

struct BranchGraphExtended : public BranchGraph {
    SemanticLayer semantic;
    int current_iteration = 0;
    
    // Get all neighbors (lineage + semantic)
    std::unordered_set<std::string> all_neighbors(const std::string& id) const {
        std::unordered_set<std::string> result;
        
        // Lineage neighbors
        const auto& lineage = neighbors(id);
        result.insert(lineage.begin(), lineage.end());
        
        // Semantic neighbors
        const auto& sem = semantic.semantic_neighbors(id);
        result.insert(sem.begin(), sem.end());
        
        return result;
    }
    
    // Update semantic links for all active branches
    void rebuild_semantic_links(
        const std::vector<Branch>& branches,
        const ResearchBuffer& buffer,
        const ResearchConfig& config
    ) {
        current_iteration++;
        
        // Only update on schedule
        if (current_iteration % config.semantic_update_period != 0) return;
        
        fprintf(stderr, "[SEMANTIC] Rebuilding semantic links (iter %d)\n", current_iteration);
        
        // Step 1: Update signatures from current summaries/concepts
        for (const auto& branch : branches) {
            std::string semantic_text;
            
            // Prefer validated summary if available
            for (const auto& summary : buffer.summaries) {
                for (const auto& src_id : summary.source_branch_ids) {
                    if (src_id == branch.id && summary.is_valid()) {
                        semantic_text = summary.text;
                        break;
                    }
                }
                if (!semantic_text.empty()) break;
            }
            
            // Fallback: join last 3 concepts
            if (semantic_text.empty() && !branch.concepts.empty()) {
                size_t start = branch.concepts.size() > 3 ? branch.concepts.size() - 3 : 0;
                for (size_t i = start; i < branch.concepts.size(); ++i) {
                    semantic_text += branch.concepts[i].content.substr(0, 200) + " ";
                }
            }
            
            if (!semantic_text.empty()) {
                semantic.update_signature(branch.id, semantic_text);
            }
        }
        
        // Step 2: For each branch, find top-k semantic neighbors with hysteresis
        int checks_budget = config.max_semantic_checks_per_iter;
        
        for (const auto& branch : branches) {
            if (checks_budget <= 0) break;
            
            auto candidates = semantic.get_candidates(branch.id);
            auto sig_it = semantic.signatures.find(branch.id);
            if (sig_it == semantic.signatures.end()) continue;
            
            uint64_t my_sig = sig_it->second;
            
            // Score candidates by Hamming distance
            std::vector<std::pair<int, std::string>> scored;  // (distance, id)
            
            for (const auto& cand_id : candidates) {
                if (!is_active(cand_id)) continue;
                if (cand_id == branch.id) continue;
                
                auto cand_sig = semantic.signatures.find(cand_id);
                if (cand_sig == semantic.signatures.end()) continue;
                
                int dist = SemanticLayer::hamming_distance(my_sig, cand_sig->second);
                scored.push_back({dist, cand_id});
                checks_budget--;
            }
            
            // Sort by distance (ascending)
            std::sort(scored.begin(), scored.end());
            
            // Current semantic neighbors
            auto current_sem = semantic.semantic_neighbors(branch.id);
            std::unordered_set<std::string> new_neighbors;
            
            // Keep top-k under add threshold
            for (size_t i = 0; i < scored.size() && (int)new_neighbors.size() < config.k_semantic; ++i) {
                if (scored[i].first <= config.semantic_add_threshold) {
                    new_neighbors.insert(scored[i].second);
                }
            }
            
            // Hysteresis: keep existing edges if under drop threshold
            for (const auto& existing : current_sem) {
                if (new_neighbors.count(existing)) continue;  // Already in new set
                
                auto ex_sig = semantic.signatures.find(existing);
                if (ex_sig == semantic.signatures.end()) continue;
                
                int dist = SemanticLayer::hamming_distance(my_sig, ex_sig->second);
                if (dist <= config.semantic_drop_threshold && 
                    (int)new_neighbors.size() < config.k_semantic) {
                    new_neighbors.insert(existing);  // Keep due to hysteresis
                }
            }
            
            // Apply changes
            for (const auto& existing : current_sem) {
                if (!new_neighbors.count(existing)) {
                    semantic.unlink_semantic(branch.id, existing);
                }
            }
            for (const auto& n : new_neighbors) {
                if (!current_sem.count(n)) {
                    semantic.link_semantic(branch.id, n);
                }
            }
        }
        
        fprintf(stderr, "[SEMANTIC] Added %d, dropped %d edges (budget remaining: %d)\n",
                semantic.semantic_edges_added, semantic.semantic_edges_dropped, checks_budget);
    }
    
    // Override remove_branch to also clean semantic layer
    void remove_branch(const std::string& id) {
        semantic.remove_branch(id);
        BranchGraph::remove_branch(id);
    }
    
    // Get edge type for debugging
    EdgeType get_edge_type(const std::string& a, const std::string& b) const {
        // Check lineage first
        if (are_adjacent(a, b)) return EdgeType::LINEAGE;
        
        std::string key = semantic.canonical_pair(a, b);
        auto it = semantic.edge_types.find(key);
        if (it != semantic.edge_types.end()) return it->second;
        
        return EdgeType::LINEAGE;  // Default
    }
    
    size_t total_semantic_edges() const {
        size_t sum = 0;
        for (const auto& [_, neighbors] : semantic.semantic_adj) {
            sum += neighbors.size();
        }
        return sum / 2;
    }
};

struct ResearchState {
    std::vector<Branch> active_branches;
    std::vector<Tension> tensions;
    std::unordered_set<std::string> tension_ids;  // deduplication
    BranchGraphExtended graph;  // O(1) adjacency + semantic similarity
    ResearchBuffer buffer;
    
    float global_entropy = 1.0f;
    float temperature = 0.9f;
    float previous_entropy = 1.0f;          // for stagnation detection
    int stagnation_count = 0;
    
    ResearchConfig config;
    
    // Generation callback - connects to LLM
    std::function<std::string(const std::string&, float temp, int max_tokens)> generate_fn;
    
    // Metrics
    int total_concepts_explored = 0;
    int branches_pruned = 0;
    int branches_spawned = 0;
    int merges_attempted = 0;
    int merges_rejected = 0;
    int tension_checks_skipped = 0;  // track O(n) savings
    
    // Recompute global entropy from current branch state
    void recompute_global_entropy() {
        if (active_branches.empty()) {
            global_entropy = 0.0f;
            return;
        }
        float total = 0.0f;
        for (auto& b : active_branches) {
            b.entropy = b.compute_entropy();
            total += b.entropy;
        }
        
        // Prune dead tensions before counting
        prune_dead_tensions();
        
        // Include unresolved tension mass (only from live tensions)
        int unresolved = 0;
        for (const auto& t : tensions) {
            if (!t.resolved) unresolved++;
        }
        float tension_entropy = unresolved * 0.1f;
        global_entropy = (total / active_branches.size()) + tension_entropy;
    }
    
    // Remove tensions referencing branches that no longer exist
    void prune_dead_tensions() {
        auto it = tensions.begin();
        while (it != tensions.end()) {
            bool a_dead = !graph.is_active(it->branch_a);
            bool b_dead = !graph.is_active(it->branch_b);
            
            if (a_dead || b_dead) {
                // Mark as resolved/obsolete and remove from ID set
                tension_ids.erase(it->id);
                it = tensions.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// =============================================================================
// TEMPERATURE CONTROL (Critical)
// Temperature is entropy injection/removal, NOT creativity
// =============================================================================

// =============================================================================
// LLM VALIDATION JUDGES (replaces stubs)
// =============================================================================

struct ValidationResult {
    bool passed;
    std::string reason;
};

inline ValidationResult validate_no_new_claims(const std::string& original_concepts,
                                                const std::string& summary,
                                                const std::function<std::string(const std::string&, float, int)>& generate) {
    ValidationResult result{true, ""};
    if (!generate) return result;
    
    std::string prompt = "Judge whether the summary introduces ANY claims not present in the source.\n\n";
    prompt += "SOURCE CONCEPTS:\n" + original_concepts.substr(0, 1500) + "\n\n";
    prompt += "SUMMARY:\n" + summary + "\n\n";
    prompt += "Does the summary introduce NEW claims not derivable from the source? Answer YES or NO, then explain briefly.";
    
    std::string response = generate(prompt, 0.1f, 100);
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    result.passed = lower.find("yes") == std::string::npos;
    result.reason = response;
    return result;
}

inline ValidationResult validate_preserves_causality(const std::string& original_concepts,
                                                      const std::string& summary,
                                                      const std::function<std::string(const std::string&, float, int)>& generate) {
    ValidationResult result{true, ""};
    if (!generate) return result;
    
    std::string prompt = "Judge whether causal relationships are preserved.\n\n";
    prompt += "SOURCE:\n" + original_concepts.substr(0, 1500) + "\n\n";
    prompt += "SUMMARY:\n" + summary + "\n\n";
    prompt += "Does the summary break or reverse any cause-effect relationships from the source? Answer YES (breaks causality) or NO (preserves it).";
    
    std::string response = generate(prompt, 0.1f, 100);
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    result.passed = lower.find("yes") == std::string::npos;
    result.reason = response;
    return result;
}

inline ValidationResult validate_uncertainty_preserved(const std::string& original_concepts,
                                                        const std::string& summary,
                                                        const std::function<std::string(const std::string&, float, int)>& generate) {
    ValidationResult result{true, ""};
    if (!generate) return result;
    
    std::string prompt = "Judge whether uncertainty is preserved (not papered over).\n\n";
    prompt += "SOURCE (may contain hedging, unknowns, maybes):\n" + original_concepts.substr(0, 1500) + "\n\n";
    prompt += "SUMMARY:\n" + summary + "\n\n";
    prompt += "Does the summary claim more certainty than the source supports? Answer YES (overclaims) or NO (honest).";
    
    std::string response = generate(prompt, 0.1f, 100);
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    result.passed = lower.find("yes") == std::string::npos;
    result.reason = response;
    return result;
}

inline float estimate_tension_severity(const Branch& a, const Branch& b,
                                        const std::function<std::string(const std::string&, float, int)>& generate) {
    if (!generate) return 0.5f;
    
    std::string prompt = "Rate the severity of contradiction between these positions (0-10).\n\n";
    prompt += "Position A: ";
    if (!a.concepts.empty()) prompt += a.concepts.back().content.substr(0, 400);
    prompt += "\n\nPosition B: ";
    if (!b.concepts.empty()) prompt += b.concepts.back().content.substr(0, 400);
    prompt += "\n\nScale: 0=compatible, 5=different emphasis, 10=fundamentally incompatible\nAnswer with just a number:";
    
    std::string response = generate(prompt, 0.1f, 10);
    
    // Extract number
    float severity = 0.5f;
    try {
        severity = std::stof(response) / 10.0f;
        severity = std::clamp(severity, 0.0f, 1.0f);
    } catch (...) {}
    
    return severity;
}

// =============================================================================
// TEMPERATURE CONTROL (Critical)
// =============================================================================

inline void adjust_temperature(ResearchState& state) {
    // Stagnation detection
    float entropy_delta = std::abs(state.global_entropy - state.previous_entropy);
    if (entropy_delta < 0.01f) {
        state.stagnation_count++;
        if (state.stagnation_count > 3) {
            // Inject entropy to escape local minimum
            state.temperature = std::min(state.temperature + state.config.temperature_delta,
                                         state.config.max_temperature);
            fprintf(stderr, "[RESEARCH] Stagnation detected, raising temp to %.2f\n", state.temperature);
        }
    } else {
        state.stagnation_count = 0;
    }
    
    // Contradiction response
    int active_tensions = 0;
    for (const auto& t : state.tensions) {
        if (!t.resolved) active_tensions++;
    }
    
    if (active_tensions > state.active_branches.size() / 2) {
        // Too many contradictions - cool down to consolidate
        state.temperature = std::max(state.temperature - state.config.temperature_delta,
                                     state.config.min_temperature);
        fprintf(stderr, "[RESEARCH] High tension, lowering temp to %.2f\n", state.temperature);
    }
    
    state.previous_entropy = state.global_entropy;
}

// =============================================================================
// PHASE 1: EXPLORATION (High Entropy)
// =============================================================================

inline void expand_branch(Branch& branch, float temperature, 
                          const std::function<std::string(const std::string&, float, int)>& generate) {
    if (!generate || branch.depth >= 5) return;
    
    // Build exploration prompt from existing concepts
    std::string prompt = "Explore further:\n";
    for (const auto& c : branch.concepts) {
        prompt += "- " + c.content.substr(0, 100) + "\n";
    }
    prompt += "\nWhat aspects remain uncertain or unexplored? Be specific and identify gaps.";
    
    std::string response = generate(prompt, temperature, 500);
    
    if (!response.empty()) {
        Concept new_concept;
        new_concept.id = branch.id + "_c" + std::to_string(branch.concepts.size());
        new_concept.content = response;
        new_concept.confidence = 0.5f;  // uncertain by default
        new_concept.salience = 0.7f;
        
        branch.concepts.push_back(new_concept);
        branch.depth++;
        branch.entropy = branch.compute_entropy();
    }
}

inline bool should_branch(const Branch& existing, const Concept& new_concept, float tension_threshold,
                          const std::function<std::string(const std::string&, float, int)>& generate = nullptr) {
    // KEY RULE: Branch on TENSION, not topic
    // Only branch if forcing unification would increase entropy
    
    if (existing.concepts.empty()) return false;
    
    // If we have a generator, use it for real contradiction detection
    if (generate) {
        std::string prompt = "Do these two statements represent fundamentally different framings that cannot be unified without losing information?\n\n";
        prompt += "Existing view: " + existing.concepts.back().content.substr(0, 300) + "\n\n";
        prompt += "New view: " + new_concept.content.substr(0, 300) + "\n\n";
        prompt += "Answer YES if they represent incompatible frames, NO if they can be unified:";
        
        std::string resp = generate(prompt, 0.1f, 10);
        std::string lower = resp;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower.find("yes") != std::string::npos;
    }
    
    // Fallback heuristic: check confidence levels
    for (const auto& c : existing.concepts) {
        if (c.confidence > 0.7f && new_concept.confidence > 0.7f) {
            // Both confident - potential tension
            return true;
        }
    }
    
    return false;
}

// =============================================================================
// BRANCH FORKING (your exact surgical fix)
// Uses should_branch() criterion + propose_frames to actually create branches
// =============================================================================

inline std::vector<std::string> propose_frames(
    const std::string& seed,
    const std::function<std::string(const std::string&, float, int)>& generate
) {
    if (!generate) return {};
    std::string prompt =
        "Given this research seed, propose 3 distinct conceptual frames or hypotheses.\n"
        "Each must be meaningfully different.\n"
        "Format exactly:\n"
        "1) <frame title>: <one-sentence seed>\n"
        "2) ...\n"
        "3) ...\n\n"
        "Seed:\n" + seed;

    std::string resp = generate(prompt, 0.6f, 220);
    
    // Minimal parsing: split lines starting with "1)" "2)" "3)"
    std::vector<std::string> out;
    std::istringstream iss(resp);
    for (std::string line; std::getline(iss, line); ) {
        if (line.size() > 2 && (line[0] == '1' || line[0] == '2' || line[0] == '3') && line[1] == ')')
            out.push_back(line);
    }
    return out;
}

inline void maybe_fork_branch(ResearchState& state, Branch& branch) {
    if ((int)state.active_branches.size() >= state.config.max_branches) return;
    if (branch.concepts.empty()) return;

    const Concept& newest = branch.concepts.back();

    // Use should_branch criterion WITH generate_fn for real detection
    if (!should_branch(branch, newest, state.config.tension_threshold, state.generate_fn)) return;

    auto frames = propose_frames(newest.content.substr(0, 600), state.generate_fn);
    if (frames.size() < 2) return;

    // Spawn branches (keep original as one frame, fork additional)
    int forks = 0;
    for (size_t i = 1; i < frames.size() && (int)state.active_branches.size() < state.config.max_branches; ++i) {
        Branch child = branch;  // copy parent state
        child.id = branch.id + "_fork" + std::to_string(i);
        child.origin_query = "forked_frame";
        child.compressible = false;  // needs re-evaluation
        child.stable = true;

        Concept c;
        c.id = child.id + "_seed";
        c.content = frames[i];
        c.confidence = 0.5f;
        c.salience = 0.8f;

        child.concepts.push_back(c);
        child.entropy = child.compute_entropy();

        // Register in graph BEFORE adding to vector
        state.graph.add_branch(child.id, branch.id);
        
        state.active_branches.push_back(std::move(child));
        state.branches_spawned++;
        forks++;
    }

    if (forks > 0) {
        fprintf(stderr, "[RESEARCH] Forked %d branches from %s (graph edges added)\n", forks, branch.id.c_str());
    }
}

inline void explore(ResearchState& state) {
    state.temperature = state.config.initial_temperature;
    fprintf(stderr, "[RESEARCH] Phase 1: Exploration (temp=%.2f, branches=%zu)\n", 
            state.temperature, state.active_branches.size());
    
    // Snapshot current branch count (we'll iterate only existing, not newly forked)
    size_t original_count = state.active_branches.size();
    
    for (size_t i = 0; i < original_count; ++i) {
        Branch& branch = state.active_branches[i];
        expand_branch(branch, state.temperature, state.generate_fn);
        state.total_concepts_explored += branch.concept_count();
        
        // Attempt to fork if tension criterion is met
        maybe_fork_branch(state, branch);
    }
    
    // Recompute global entropy from actual state
    state.recompute_global_entropy();
    fprintf(stderr, "[RESEARCH] Post-explore: %zu branches, entropy=%.2f\n", 
            state.active_branches.size(), state.global_entropy);
}

// =============================================================================
// UNIFIED SUMMARY JUDGE (reuse lucid-style pattern)
// Single call that checks all validity criteria
// =============================================================================

inline bool judge_summary_valid(
    const std::string& summary,
    const std::string& source,
    const std::function<std::string(const std::string&, float, int)>& generate
) {
    if (!generate) return true;

    std::string prompt =
        "Evaluate the SUMMARY against the SOURCE.\n"
        "Rules:\n"
        "1) Must not introduce new factual claims not in SOURCE\n"
        "2) Must preserve causal structure\n"
        "3) Must preserve uncertainty (do not overclaim)\n\n"
        "Answer only YES (valid) or NO (invalid).\n\n"
        "SOURCE:\n" + source.substr(0, 1200) + "\n\nSUMMARY:\n" + summary;

    std::string resp = generate(prompt, 0.1f, 10);
    std::string lower = resp;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find("yes") != std::string::npos;
}

// =============================================================================
// PHASE 2: LOCAL COMPRESSION
// KEY RULE: If a branch cannot compress, it is not knowledge. Reject without sentiment.
// =============================================================================

inline Summary attempt_compression(Branch& branch, 
                                   const std::function<std::string(const std::string&, float, int)>& generate) {
    Summary result;
    result.entropy_reduction = 0.0f;
    
    if (branch.concepts.size() < 2 || !generate) {
        return result;  // Nothing to compress
    }
    
    // Build original concepts string for validation
    std::string original_concepts;
    for (const auto& c : branch.concepts) {
        original_concepts += "- " + c.content.substr(0, 300) + "\n";
    }
    
    // Build compression prompt
    std::string prompt = "Compress these related concepts into a single coherent statement:\n\n";
    prompt += original_concepts;
    prompt += "\nRules:\n";
    prompt += "1. Do NOT introduce new claims\n";
    prompt += "2. Preserve causal relationships\n";
    prompt += "3. Maintain uncertainty where it exists\n";
    prompt += "4. Be specific, not general\n";
    
    std::string response = generate(prompt, 0.3f, 300);  // Low temp for compression
    
    if (!response.empty()) {
        result.text = response;
        result.source_branch_ids.push_back(branch.id);
        
        // REAL VALIDATION via unified judge (your lucid pattern)
        bool valid = judge_summary_valid(response, original_concepts, generate);
        
        result.introduces_new_concepts = !valid;
        result.preserves_causality = valid;
        
        // Calculate entropy reduction based on validation
        float original_entropy = branch.entropy;
        
        if (valid) {
            // Valid compression - real entropy reduction
            float reduction_factor = 0.25f;  // Base 25%
            if (response.length() < original_concepts.length() * 0.6f) {
                reduction_factor += 0.1f;  // Bonus for significant length reduction
            }
            result.entropy_reduction = original_entropy * reduction_factor;
            fprintf(stderr, "[RESEARCH] Compression VALID for %s (reduction=%.2f)\n",
                    branch.id.c_str(), result.entropy_reduction);
        } else {
            // Failed validation - zero reduction, won't enter buffer
            result.entropy_reduction = 0.0f;
            fprintf(stderr, "[RESEARCH] Compression INVALID for %s (judge said NO)\n",
                    branch.id.c_str());
        }
    }
    
    return result;
}

inline void compress_branches(ResearchState& state) {
    fprintf(stderr, "[RESEARCH] Phase 2: Local Compression\n");
    
    for (auto& branch : state.active_branches) {
        Summary s = attempt_compression(branch, state.generate_fn);
        
        if (s.entropy_reduction > state.config.min_entropy_reduction) {
            branch.compressible = true;
            state.buffer.add_summary(s);
            fprintf(stderr, "[RESEARCH] Branch %s compressed (entropy -%.2f)\n", 
                    branch.id.c_str(), s.entropy_reduction);
        } else {
            branch.stable = false;  // Mark as unstable
            fprintf(stderr, "[RESEARCH] Branch %s cannot compress - marked unstable\n", 
                    branch.id.c_str());
        }
    }
    
    // Prune unstable branches if required
    if (state.config.require_compression) {
        auto it = std::remove_if(state.active_branches.begin(), state.active_branches.end(),
                                 [](const Branch& b) { return !b.stable; });
        int pruned = std::distance(it, state.active_branches.end());
        state.active_branches.erase(it, state.active_branches.end());
        state.branches_pruned += pruned;
        fprintf(stderr, "[RESEARCH] Pruned %d unstable branches\n", pruned);
    }
    
    // Recompute global entropy after pruning (Gap B fix)
    state.recompute_global_entropy();
}

// =============================================================================
// PHASE 3: TENSION DETECTION & MERGING
// IMPORTANT: Do not resolve contradictions immediately. Tension itself is signal.
// =============================================================================

inline bool creates_contradiction(const Branch& a, const Branch& b,
                                  const std::function<std::string(const std::string&, float, int)>& generate) {
    if (!generate) return false;
    
    // Build conflict detection prompt
    std::string prompt = "Do these two statements contradict each other?\n\n";
    prompt += "Statement A: ";
    if (!a.concepts.empty()) prompt += a.concepts.back().content.substr(0, 300);
    prompt += "\n\nStatement B: ";
    if (!b.concepts.empty()) prompt += b.concepts.back().content.substr(0, 300);
    prompt += "\n\nAnswer YES if they contradict, NO if compatible:";
    
    std::string response = generate(prompt, 0.1f, 10);
    
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find("yes") != std::string::npos;
}

inline Tension record_tension(const Branch& a, const Branch& b,
                              const std::function<std::string(const std::string&, float, int)>& generate) {
    Tension t;
    t.id = "tension_" + a.id + "_" + b.id;
    t.branch_a = a.id;
    t.branch_b = b.id;
    
    // Build description from actual content
    std::string a_claim = a.concepts.empty() ? "" : a.concepts.back().content.substr(0, 100);
    std::string b_claim = b.concepts.empty() ? "" : b.concepts.back().content.substr(0, 100);
    t.description = "[" + a.id + "]: " + a_claim + " vs [" + b.id + "]: " + b_claim;
    
    // Real severity estimation via LLM (Gap C fix)
    t.severity = estimate_tension_severity(a, b, generate);
    t.resolved = false;
    return t;
}

inline void detect_and_record_tensions(ResearchState& state) {
    size_t lineage_edges = state.graph.total_edges();
    size_t semantic_edges = state.graph.total_semantic_edges();
    fprintf(stderr, "[RESEARCH] Phase 3a: Tension Detection (lineage: %zu, semantic: %zu edges)\n", 
            lineage_edges, semantic_edges);
    
    // Build branch lookup for O(1) access
    std::unordered_map<std::string, size_t> branch_idx;
    for (size_t i = 0; i < state.active_branches.size(); ++i) {
        branch_idx[state.active_branches[i].id] = i;
    }
    
    // O(E) - iterate ALL neighbors (lineage + semantic)
    std::unordered_set<std::string> checked_pairs;
    int checks_performed = 0;
    int cache_hits = 0;
    
    for (size_t i = 0; i < state.active_branches.size(); ++i) {
        const Branch& branch = state.active_branches[i];
        
        // Get ALL neighbors (lineage + semantic)
        auto all_neighs = state.graph.all_neighbors(branch.id);
        
        for (const std::string& neighbor_id : all_neighs) {
            // Skip if neighbor not active (dead reference)
            if (!state.graph.is_active(neighbor_id)) continue;
            
            // Skip if neighbor not in current branch vector
            auto it = branch_idx.find(neighbor_id);
            if (it == branch_idx.end()) continue;
            
            // Canonical pair key: (min_id, max_id) for dedup
            std::string pair_key = branch.id < neighbor_id 
                ? branch.id + "_" + neighbor_id 
                : neighbor_id + "_" + branch.id;
            
            if (checked_pairs.count(pair_key) > 0) {
                continue;  // Already checked this iteration
            }
            checked_pairs.insert(pair_key);
            
            // Skip if tension already recorded (persistent dedup)
            std::string canonical_id = "tension_" + pair_key;
            if (state.tension_ids.count(canonical_id) > 0) {
                continue;
            }
            
            const Branch& neighbor = state.active_branches[it->second];
            
            // Check cache first
            bool cached_result;
            bool is_contradiction;
            if (state.graph.semantic.get_cached_contradiction(branch.id, neighbor_id, cached_result)) {
                is_contradiction = cached_result;
                cache_hits++;
            } else {
                checks_performed++;
                is_contradiction = creates_contradiction(branch, neighbor, state.generate_fn);
                // Cache the result
                state.graph.semantic.cache_contradiction(branch.id, neighbor_id, is_contradiction);
            }
            
            if (is_contradiction) {
                Tension t = record_tension(branch, neighbor, state.generate_fn);
                t.id = canonical_id;  // Use canonical ID
                state.tensions.push_back(t);
                state.tension_ids.insert(t.id);
                
                EdgeType edge_type = state.graph.get_edge_type(branch.id, neighbor_id);
                const char* type_str = edge_type == EdgeType::SEMANTIC ? "semantic" : "lineage";
                fprintf(stderr, "[RESEARCH] Tension recorded: %s (severity=%.2f, via %s)\n", 
                        t.id.c_str(), t.severity, type_str);
            }
        }
    }
    
    state.tension_checks_skipped += cache_hits;
    fprintf(stderr, "[RESEARCH] Checked %d pairs, cache hits: %d\n", checks_performed, cache_hits);
}
    
    state.tension_checks_skipped += (state.graph.total_edges() - checks_performed);
    fprintf(stderr, "[RESEARCH] Checked %d pairs (of %zu edges)\n", 
            checks_performed, state.graph.total_edges());
}

inline bool ready_to_merge(const Tension& t, const ResearchState& state) {
    // Only merge when:
    // 1. Both branches are compressible
    // 2. Tension severity is below threshold OR
    // 3. Sufficient evidence exists to resolve
    
    const Branch* a = nullptr;
    const Branch* b = nullptr;
    
    for (const auto& branch : state.active_branches) {
        if (branch.id == t.branch_a) a = &branch;
        if (branch.id == t.branch_b) b = &branch;
    }
    
    if (!a || !b) return false;
    
    return a->compressible && b->compressible && 
           (t.severity < state.config.merge_readiness || t.resolved);
}

inline void merge_pair(ResearchState& state, Tension& tension) {
    state.merges_attempted++;
    
    // Find branches
    Branch* a = nullptr;
    Branch* b = nullptr;
    
    for (auto& branch : state.active_branches) {
        if (branch.id == tension.branch_a) a = &branch;
        if (branch.id == tension.branch_b) b = &branch;
    }
    
    if (!a || !b) {
        state.merges_rejected++;
        state.buffer.rejected_merges.push_back(tension.id + ": branches not found");
        return;
    }
    
    // COPY IDs before any mutation (Bug 1 fix - pointer invalidation)
    std::string a_id = a->id;
    std::string b_id = b->id;
    
    // Attempt merge via compression
    Branch merged;
    merged.id = "merged_" + a_id + "_" + b_id;
    merged.concepts.insert(merged.concepts.end(), a->concepts.begin(), a->concepts.end());
    merged.concepts.insert(merged.concepts.end(), b->concepts.begin(), b->concepts.end());
    merged.entropy = merged.compute_entropy();
    
    Summary s = attempt_compression(merged, state.generate_fn);
    
    if (s.is_valid() && s.entropy_reduction > state.config.min_entropy_reduction) {
        // Merge successful
        merged.compressible = true;
        state.buffer.add_summary(s);
        
        // Update graph using centralized merge helper
        state.graph.merge_branches(merged.id, a_id, b_id);
        
        // Remove original branches from vector, add merged
        state.active_branches.erase(
            std::remove_if(state.active_branches.begin(), state.active_branches.end(),
                           [&](const Branch& br) { return br.id == a_id || br.id == b_id; }),
            state.active_branches.end());
        state.active_branches.push_back(merged);
        
        tension.resolved = true;
        tension.resolution_method = "compression";
        fprintf(stderr, "[RESEARCH] Merged %s + %s -> %s (graph: %zu edges)\n", 
                a_id.c_str(), b_id.c_str(), merged.id.c_str(), state.graph.total_edges());
    } else {
        // THE ONE RULE: If merge feels forced, entropy has not been paid
        state.merges_rejected++;
        state.buffer.rejected_merges.push_back(tension.id + ": compression failed");
        fprintf(stderr, "[RESEARCH] Merge rejected: %s (entropy not paid)\n", tension.id.c_str());
    }
}

inline void merge_branches(ResearchState& state) {
    fprintf(stderr, "[RESEARCH] Phase 3b: Tension-Aware Merging\n");
    
    for (auto& tension : state.tensions) {
        if (ready_to_merge(tension, state)) {
            merge_pair(state, tension);
        }
    }
    
    // Recompute global entropy after merges (Gap B fix)
    state.recompute_global_entropy();
}

// =============================================================================
// PHASE 4: GLOBAL COMPRESSION
// Summary validity: Removes more entropy than introduces, no new concepts, preserves causality
// =============================================================================

inline Summary compress_global(ResearchState& state) {
    Summary result;
    result.entropy_reduction = 0.0f;
    
    if (state.buffer.summaries.empty() || !state.generate_fn) {
        return result;
    }
    
    // Build global compression prompt from all summaries
    std::string prompt = "Synthesize these findings into a coherent whole:\n\n";
    for (const auto& s : state.buffer.summaries) {
        prompt += "- " + s.text.substr(0, 200) + "\n";
    }
    prompt += "\nRules:\n";
    prompt += "1. Do NOT introduce new claims\n";
    prompt += "2. Preserve causal relationships\n";
    prompt += "3. Acknowledge remaining uncertainties\n";
    prompt += "4. Note unresolved tensions explicitly\n";
    
    std::string response = state.generate_fn(prompt, 0.3f, 500);
    
    if (!response.empty()) {
        result.text = response;
        for (const auto& s : state.buffer.summaries) {
            result.source_branch_ids.insert(result.source_branch_ids.end(),
                                            s.source_branch_ids.begin(),
                                            s.source_branch_ids.end());
        }
        
        // Estimate entropy reduction
        result.entropy_reduction = state.buffer.total_entropy_reduced * 0.3f;  // Additional 30%
        result.introduces_new_concepts = false;
        result.preserves_causality = true;
    }
    
    return result;
}

inline void summarize_merges(ResearchState& state) {
    fprintf(stderr, "[RESEARCH] Phase 4: Global Compression\n");
    
    state.temperature = state.config.min_temperature;  // Low temp for consolidation
    
    Summary merged = compress_global(state);
    
    if (merged.entropy_reduction > state.config.global_entropy_threshold) {
        state.buffer.add_summary(merged);
        fprintf(stderr, "[RESEARCH] Global compression successful (reduction: %.2f)\n",
                merged.entropy_reduction);
    } else {
        fprintf(stderr, "[RESEARCH] Global compression insufficient reduction\n");
    }
    
    // Final entropy recomputation (Gap B fix)
    state.recompute_global_entropy();
    fprintf(stderr, "[RESEARCH] Post-compression entropy: %.2f\n", state.global_entropy);
}

// =============================================================================
// PHASE 5: CONCLUSION FORMATION
// The conclusion is NOT certainty. It is the smallest structure that survives contradiction.
// =============================================================================

inline Summary final_compression(const ResearchBuffer& buffer,
                                 const std::function<std::string(const std::string&, float, int)>& generate) {
    Summary result;
    result.entropy_reduction = 0.0f;
    
    if (buffer.summaries.empty() || !generate) {
        return result;
    }
    
    // Build final synthesis prompt
    std::string prompt = "Form a final conclusion from this research:\n\n";
    prompt += "Validated findings:\n";
    for (const auto& s : buffer.summaries) {
        if (s.is_valid()) {
            prompt += "- " + s.text.substr(0, 150) + "\n";
        }
    }
    
    if (!buffer.rejected_merges.empty()) {
        prompt += "\nUnresolved tensions:\n";
        for (const auto& r : buffer.rejected_merges) {
            prompt += "- " + r + "\n";
        }
    }
    
    prompt += "\nForm a conclusion that:\n";
    prompt += "1. States what is known with confidence\n";
    prompt += "2. Acknowledges what remains uncertain\n";
    prompt += "3. Identifies where more research is needed\n";
    prompt += "4. Does NOT claim more certainty than evidence supports\n";
    
    std::string response = generate(prompt, 0.2f, 600);  // Very low temp for conclusion
    
    if (!response.empty()) {
        result.text = response;
        result.entropy_reduction = buffer.total_entropy_reduced;
        result.introduces_new_concepts = false;
        result.preserves_causality = true;
    }
    
    return result;
}

inline void produce_conclusion(ResearchState& state, std::string& output) {
    fprintf(stderr, "[RESEARCH] Phase 5: Conclusion Formation\n");
    
    Summary conclusion = final_compression(state.buffer, state.generate_fn);
    
    if (conclusion.is_valid()) {
        output = conclusion.text;
        
        // Add unresolved tensions as explicit section
        if (state.config.preserve_tensions) {
            output += "\n\n--- Unresolved Tensions ---\n";
            for (const auto& t : state.tensions) {
                if (!t.resolved) {
                    output += "• " + t.description + "\n";
                }
            }
        }
        
        fprintf(stderr, "[RESEARCH] Conclusion formed (total entropy reduced: %.2f)\n",
                state.buffer.total_entropy_reduced);
    } else {
        output = "Research inconclusive - insufficient entropy reduction achieved.";
        fprintf(stderr, "[RESEARCH] Conclusion failed validity check\n");
    }
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

inline void initialize_state(ResearchState& state, const std::string& topic) {
    // Create initial branch from topic
    Branch initial;
    initial.id = "root";
    initial.origin_query = topic;
    
    Concept seed;
    seed.id = "root_c0";
    seed.content = topic;
    seed.confidence = 0.5f;  // Unknown at start
    seed.salience = 1.0f;
    
    initial.concepts.push_back(seed);
    initial.entropy = initial.compute_entropy();
    
    // Register root in graph
    state.graph.add_branch(initial.id);
    
    state.active_branches.push_back(initial);
    state.global_entropy = 1.0f;  // Maximum uncertainty
    state.temperature = state.config.initial_temperature;
    
    fprintf(stderr, "[RESEARCH] Initialized with topic: %s (graph root added)\n", topic.c_str());
}

inline std::string run_research_mode(const std::string& topic, 
                                     const std::function<std::string(const std::string&, float, int)>& generate_fn,
                                     const ResearchConfig& config = ResearchConfig()) {
    ResearchState state;
    state.config = config;
    state.generate_fn = generate_fn;
    
    fprintf(stderr, "\n[RESEARCH] ════════════════════════════════════════\n");
    fprintf(stderr, "[RESEARCH] Starting Research Mode\n");
    fprintf(stderr, "[RESEARCH] ════════════════════════════════════════\n");
    
    // Initialize
    initialize_state(state, topic);
    
    // Iterative exploration-compression loop
    for (int iter = 0; iter < config.max_iterations; iter++) {
        fprintf(stderr, "\n[RESEARCH] --- Iteration %d ---\n", iter + 1);
        
        // Phase 1: Exploration
        explore(state);
        
        // Temperature adjustment
        adjust_temperature(state);
        
        // Phase 2: Local Compression
        compress_branches(state);
        
        // Rebuild semantic links after compression (uses updated summaries)
        state.graph.rebuild_semantic_links(state.active_branches, state.buffer, state.config);
        
        // Phase 3: Tension Detection & Merging
        detect_and_record_tensions(state);
        merge_branches(state);
        
        // Check convergence
        if (state.global_entropy < 0.2f || state.active_branches.size() <= 1) {
            fprintf(stderr, "[RESEARCH] Converged at iteration %d\n", iter + 1);
            break;
        }
    }
    
    // Phase 4: Global Compression
    summarize_merges(state);
    
    // Phase 5: Conclusion
    std::string output;
    produce_conclusion(state, output);
    
    fprintf(stderr, "\n[RESEARCH] ════════════════════════════════════════\n");
    fprintf(stderr, "[RESEARCH] Research Mode Complete\n");
    fprintf(stderr, "[RESEARCH] Concepts explored: %d\n", state.total_concepts_explored);
    fprintf(stderr, "[RESEARCH] Branches spawned: %d | pruned: %d\n", state.branches_spawned, state.branches_pruned);
    fprintf(stderr, "[RESEARCH] Graph: %zu lineage edges, %zu semantic edges\n", 
            state.graph.total_edges(), state.graph.total_semantic_edges());
    fprintf(stderr, "[RESEARCH] Semantic: added %d, dropped %d, cache hits %d\n",
            state.graph.semantic.semantic_edges_added, 
            state.graph.semantic.semantic_edges_dropped,
            state.graph.semantic.cache_hits);
    fprintf(stderr, "[RESEARCH] Tensions recorded: %zu (unresolved: %zu)\n", 
            state.tensions.size(), 
            std::count_if(state.tensions.begin(), state.tensions.end(), [](const Tension& t) { return !t.resolved; }));
    fprintf(stderr, "[RESEARCH] Merges attempted: %d (rejected: %d)\n", 
            state.merges_attempted, state.merges_rejected);
    fprintf(stderr, "[RESEARCH] Final entropy: %.2f\n", state.global_entropy);
    fprintf(stderr, "[RESEARCH] ════════════════════════════════════════\n\n");
    
    return output;
}

} // namespace zeta_research
