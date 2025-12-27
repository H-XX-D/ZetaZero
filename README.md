# Z.E.T.A. - Zero-shot Evolving Thought Architecture

> A self-improving multi-model AI system that dreams about its own code

## Architecture Overview

Z.E.T.A. is a cognitive architecture that orchestrates multiple AI models in a brain-inspired architecture:

```
                    ┌─────────────────────────────────────┐
                    │           Z.E.T.A. CORE             │
                    ├─────────────────────────────────────┤
                    │                                     │
  ┌─────────┐       │   ┌─────────┐    ┌─────────┐       │       ┌──────────┐
  │  User   │──────►│   │   HRM   │◄──►│   TRM   │       │◄─────►│  Graph   │
  │ Query   │       │   │  (14B)  │    │ (Decay) │       │       │  Memory  │
  └─────────┘       │   └────┬────┘    └────┬────┘       │       └──────────┘
                    │        │              │             │
                    │   ┌────▼────────────▼────┐         │
                    │   │    Dynamic Router     │         │
                    │   └──┬──────┬──────┬─────┘         │
                    │      │      │      │               │
                    │   ┌──▼──┐┌──▼──┐┌──▼──┐           │
                    │   │ 14B ││ 7B  ││ 4B  │           │
                    │   │Brain││Coder││Embed│           │
                    │   └─────┘└─────┘└─────┘           │
                    │                                     │
                    │        ┌───────────┐               │
                    │        │  DREAM    │ (idle)        │
                    │        │  STATE    │───────────────┤
                    │        └───────────┘               │
                    └─────────────────────────────────────┘
```

## Models

| Model | Size | Purpose | Context |
|-------|------|---------|---------|
| **Conscious** | 14B | Complex reasoning, planning, analysis | 4096 |
| **Coder** | 7B | Code generation, syntax, execution | 2048 |
| **Embedding** | 4B | Semantic search, memory retrieval | 512 |

## Core Modules

### HRM - Hierarchical Reasoning Module
Decomposes complex queries into executable sub-plans. Orchestrates 14B (Planner) and 7B (Executor) in a feedback loop.

```cpp
class ZetaHRM {
    void set_cognitive_state(zeta_cognitive_state_t state);
    void set_anxiety_level(float level);
    zeta_hrm_plan_t create_plan(const std::string& query);
    std::string run(const std::string& query);
};
```

**Cognitive States:**
- `CALM` - Normal processing (depth=10, branches=4)
- `FOCUSED` - Deep work mode (depth=15, branches=3)
- `ANXIOUS` - High load, reduce complexity (depth=3, branches=2)
- `CREATIVE` - Exploration mode (depth=8, branches=6)

### TRM - Temporal Reasoning Memory
Handles recursive state maintenance and temporal consistency. Implements Git-style branching for thought management.

```cpp
class ZetaTRM {
    void init(float lambda = 0.001f);
    bool manage_branches(const std::string& branch_name, const std::string& from_commit);
    bool create_branch(const std::string& branch_name);
    bool checkout_branch(const std::string& branch_name);
    zeta_trm_merge_result_t cognitive_merge(const std::string& source_branch);
    bool cherry_pick(const std::string& branch_name, const std::string& commit_id);
};
```

### Dream State
Runs during idle time to consolidate memories and generate insights.

```cpp
class ZetaDreamState {
    void run_dream_cycle();
    void run_sleep_pruning(float prune_threshold);
    void run_memory_consolidation();
    void sync_cognitive_state(const std::string& state, float anxiety);
    std::vector<std::string> detect_memory_patterns();
};
```

**Dream Categories:**
- `code_fix` - Bug fixes and improvements
- `code_idea` - New feature suggestions
- `insight` - Architectural patterns
- `story` - Narrative explorations

### Dynamic Router
Context-aware task routing between models.

```cpp
class ZetaDynamicRouter {
    ZetaRoutingDecision route(const ZetaTask& task, const ZetaResourceStatus& status);
    ZetaTask analyzeQuery(const std::string& query);
};
```

**Routing Rules:**
1. High code likelihood → 7B Coder
2. Simple queries → 7B (faster)
3. Embedding/memory → 4B
4. Complex reasoning → 14B
5. Load balancing when 14B busy

## Key Features

### Cross-Module Cognitive Sync
HRM, TRM, and Dream State share cognitive context:

```cpp
struct ZetaCognitiveSync {
    std::function<void(float)> on_lambda_update;
    std::function<void(const std::string&, float)> on_dream_sync;
    std::function<void(const std::string&)> on_trm_push;
};
```

### Sleep-Pruning Memory Consolidation
Brain-inspired memory optimization during idle:
- Evaluates node importance by recency and connectivity
- Prunes weak connections
- Strengthens important connections
- Detects recurring patterns

### Lambda-Based Temporal Decay
Adjusts memory decay based on cognitive state:
- **ANXIOUS**: λ × 3.0 (faster decay, prevent rumination)
- **FOCUSED**: λ × 0.5 (sustained attention)
- **CREATIVE**: λ × 0.2 (longer exploration)
- **CALM**: λ × 1.0 (baseline)

## Utility Classes

### StringUtility
```cpp
class StringUtility {
    static size_t getLength(const char* str);
    static bool isNonEmpty(const std::string& str);
    static bool isValid(const char* str);
    static int findIndex(const std::string& str, const std::string& substring);
    static std::string trim(const std::string& str);
    static std::string toLower(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
};
```

### HRMManager
```cpp
class HRMManager {
    bool init(zeta_dual_ctx_t* dual_ctx);
    int findBranchIndex(const char* branch_name);
    bool factHasContext(const char* context);
    void setCognitiveState(const std::string& state);
};
```

### ZetaContextChecker
```cpp
class ZetaContextChecker {
    static ValidationResult validate_context(const std::string& context);
    static bool validate_context_type(const std::string& type);
    static bool validate_intensity(float intensity);
    static bool validate_lambda(float lambda);
    static ValidationResult validate_causal_relation(const std::string& cause, const std::string& effect);
};
```

## System Initialization

```cpp
// Unified system initialization
bool zeta_system_init(zeta_dual_ctx_t* dual_ctx, float lambda = 0.001f);

// Check system readiness
bool zeta_system_ready();

// Get system status
std::string zeta_system_status();

// Route a query
ZetaRoutingDecision zeta_route_query(const std::string& query, const ZetaResourceStatus& status);
```

## Self-Improvement Loop

Z.E.T.A. can analyze its own source code and dream about improvements:

1. **Feed Source** - Ingest own codebase into memory graph
2. **Dream** - Generate improvement suggestions during idle
3. **Implement** - Apply valid dream suggestions
4. **Repeat** - Feed updated code, dream about improvements to improvements

### Dream Statistics
- Total dreams generated: 128+
- Categories: code_fix, code_idea, insight, story
- Lucid validation rate: ~70%

### Dream Repetition Penalty
Prevents fixation on the same ideas:
```cpp
class DreamRepetitionTracker {
    float calculate_novelty(const std::string& content);  // 0.0-1.0
    bool is_too_repetitive(const std::string& content);   // threshold check
    std::string get_avoidance_prompt();                   // inject into dream prompt
};
```
- Tracks key domain terms (router, cache, model, etc.)
- Rejects dreams with novelty < 0.3
- Theme decay after 24 hours

## Dynamic Router Enhancements

### Query Caching
```cpp
// Check cache before routing
bool checkCache(const std::string& query_hash, ZetaRoutingDecision& out);
void addToCache(const std::string& query_hash, const ZetaRoutingDecision& decision);
```
- TTL: 300 seconds
- Max entries: 100
- LRU eviction

### Feedback Loop
```cpp
// Record outcomes for adaptive routing
void recordOutcome(const std::string& model, float response_time_ms,
                   float accuracy_score, bool success);
```
- Tracks avg response time per model
- Tracks accuracy (from critic)
- Auto-adapts complexity threshold based on performance

### Model Fallback
```cpp
ZetaRoutingDecision routeWithFallback(const ZetaTask& task, const ZetaResourceStatus& status);
```
- Falls back to 7B if 14B unavailable
- Load-based fallback at 90%+ load
- Cascading fallback chain: 14B → 7B → 4B

### Embedding Cache (Dream 085129)
```cpp
class EmbeddingCache {
    bool get(const char* text, float* output, int dim);   // Check cache
    void put(const char* text, const float* embedding, int dim);  // Store
};
```
- Caches 4B embedding results with 10-minute TTL
- Max 500 entries with LRU eviction
- Avoids redundant computation for repeated queries

### Query Prioritization (Dream 085038)
```cpp
class ZetaQueryPrioritizer {
    bool enqueue(const ZetaTask& task, ZetaPriority priority, time_t deadline);
    bool dequeue(ZetaPrioritizedTask& out_task);
    float calculatePriorityScore(...);
};
```
- Priority levels: LOW, MEDIUM, HIGH, URGENT
- Weighted scoring: urgency (30%), complexity (30%), user tier (20%), wait time (20%)
- Age-based priority upgrades for waiting tasks

### User Satisfaction Feedback (Dream 084453)
```cpp
class ZetaSatisfactionTracker {
    void recordFeedback(const std::string& request_id, const std::string& model, int rating);
    float getRoutingAdjustment(const std::string& model);
};
```
- Collects 1-5 star ratings per request
- Calculates per-model satisfaction scores
- Provides routing adjustment recommendations

### Meta-Router (Dream 085148)
```cpp
class ZetaMetaRouter {
    void registerRouter(const std::string& name, ZetaDynamicRouter* router);
    ZetaRoutingDecision routeWithMeta(const std::string& query, const ZetaResourceStatus& status);
};
```
- "Router of routers" for strategy selection
- Evaluates router performance every 100 requests
- Weighted scoring: response time (40%), accuracy (40%), satisfaction (20%)

## Files

| File | Purpose |
|------|---------|
| `zeta-server.cpp` | Main server, model orchestration |
| `zeta-dual-process.h` | Graph context, node/edge management |
| `zeta-hrm.h` | Hierarchical Reasoning Module |
| `zeta-trm.h` | Temporal Reasoning Memory |
| `zeta-dream.h` | Dream State consolidation |
| `zeta-utils.h` | String/context utilities |
| `zeta-system.h` | System integration, HRMManager |
| `zeta-config.h` | Configuration, ContextChecker |
| `zeta-embed-integration.h` | 4B embedding integration |
| `zeta-code-mode.h` | 7B coder integration |
| `zeta-critic.h` | Output validation |
| `zeta-conflict.h` | Memory protection, fact validation |
| `zeta-semantic-attacks.h` | Attack detection |
| `zeta-graph-git.h` | Git-style graph operations |
| `zeta-ternary.h` | Ternary logic emulation |

## Building

```bash
cd llama.cpp
cmake -B build
cmake --build build --target zeta-server
```

## Running

```bash
./build/bin/zeta-server \
  --model /path/to/14b.gguf \
  --model-coder /path/to/7b-coder.gguf \
  --model-embed /path/to/4b-embed.gguf \
  -ngl 99 -c 4096 --host 0.0.0.0 --port 8080
```

## API Endpoints

- `GET /health` - Server status and graph stats
- `POST /generate` - Generate response with full pipeline
- `POST /embedding` - Get text embedding
- `GET /git/status` - Graph memory status
- `POST /memory/query` - Semantic memory search

## Contact

For consulting, integration, or enterprise inquiries: todd@hendrixxdesign.com

## License

Research prototype - created in 2025

---

*This README was generated through Z.E.T.A.'s recursive self-improvement loop*
