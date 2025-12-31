// =============================================================================
// Z.E.T.A. Hierarchical Storage Management (HSM) v2
// =============================================================================
// Four-tier storage with predictive prefetch and tensor-aware compression
//
// Storage Tiers:
//   HOT        - VRAM (mlockall, instant 0.3ms access)
//   WARM       - NVMe uncompressed (mmap, 0.3ms)
//   WARM-DENSE - NVMe compressed (AURA-GKV Fast, ~5ms)
//   COLD       - HDD compressed (AURA-GKV Balanced, ~50ms)
//
// Key Features:
//   - Tensor-aware delta encoding (graph parent→child deltas)
//   - Predictive prefetcher (pre-warm related nodes)
//   - Automatic tier migration based on access patterns
//   - AURA-GKV compression integration
//
// (C) 2025 - Patent Pending
// =============================================================================

#ifndef ZETA_HSM_H
#define ZETA_HSM_H

#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

#include "aura-gkv.h"

// =============================================================================
// Storage Tier Definitions
// =============================================================================

enum class ZetaStorageTier {
    HOT,        // VRAM - mlockall'd, instant access (0.3ms)
    WARM,       // NVMe - mmap'd, uncompressed (0.3ms)
    WARM_DENSE, // NVMe - compressed, AURA-GKV Fast (~5ms)
    COLD        // HDD  - compressed, AURA-GKV Balanced (~50ms)
};

static const char* tier_name(ZetaStorageTier tier) {
    switch (tier) {
        case ZetaStorageTier::HOT:        return "HOT";
        case ZetaStorageTier::WARM:       return "WARM";
        case ZetaStorageTier::WARM_DENSE: return "WARM-DENSE";
        case ZetaStorageTier::COLD:       return "COLD";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// GKV Node Metadata
// =============================================================================

struct ZetaGKVNode {
    std::string node_id;           // Unique identifier (e.g., "gkv_12345")
    ZetaStorageTier tier;          // Current storage tier

    // Size tracking
    uint64_t raw_size;             // Uncompressed size
    uint64_t stored_size;          // Size on disk (compressed if applicable)

    // Access patterns
    uint64_t access_count;         // Total accesses
    time_t last_access;            // Unix timestamp of last access
    time_t created;                // Creation time

    // Graph-aware scoring
    float salience;                // Importance score (0-1)
    float momentum;                // Recent access momentum (decays)
    float momentum_integral;       // Cumulative importance over time (decays slower)

    // Tensor-aware delta encoding
    std::string parent_id;         // Parent node for delta encoding
    bool is_delta_encoded;         // True if stored as delta from parent

    // Graph relationships (for prefetcher)
    std::vector<std::string> children;   // Child nodes in graph
    std::vector<std::string> siblings;   // Related nodes

    // File paths
    std::string warm_path;         // /storage/graph_kv/node.bin
    std::string dense_path;        // /storage/graph_kv/dense/node.agkv
    std::string cold_path;         // /mnt/HDD/zeta/cold/node.agkv

    // State flags
    bool is_locked;                // Prevent migration
    bool is_prefetching;           // Currently being prefetched
};

// =============================================================================
// HSM Configuration
// =============================================================================

struct ZetaHSMConfig {
    // Paths
    std::string warm_dir;          // NVMe: /storage/graph_kv
    std::string dense_dir;         // NVMe compressed: /storage/graph_kv/dense
    std::string cold_dir;          // HDD: /mnt/HDD/zeta/cold

    // Capacity thresholds (percentage)
    float warm_to_dense_threshold;   // Compress when NVMe > 70%
    float dense_to_cold_threshold;   // Move to HDD when NVMe > 85%
    float cold_warning_threshold;    // Warn when HDD > 90%

    // Time thresholds (seconds)
    int dense_idle_threshold;        // Compress after 1 hour idle
    int cold_idle_threshold;         // Move to HDD after 24 hours idle
    int min_age_for_migration;       // Min age before eligible (1 hour)

    // Access thresholds
    int hot_access_threshold;        // Accesses to promote to HOT

    // Prefetcher settings
    int prefetch_depth;              // How many related nodes to prefetch
    int prefetch_threads;            // Parallel prefetch workers
    bool prefetch_enabled;           // Enable/disable prefetcher

    // Migration settings
    int migration_batch_size;        // Nodes per migration cycle
    int migration_interval_sec;      // Check interval

    // Memory limits
    size_t max_hot_bytes;            // Max VRAM for GKV (2GB)
    size_t max_warm_bytes;           // Max uncompressed NVMe (50GB)
    size_t max_dense_bytes;          // Max compressed NVMe (100GB)
};

static ZetaHSMConfig g_hsm_config = {
    "/storage/graph_kv",             // warm_dir
    "/storage/graph_kv/dense",       // dense_dir
    "/mnt/HDD/zeta/cold",            // cold_dir
    0.70f,                           // warm_to_dense_threshold
    0.85f,                           // dense_to_cold_threshold
    0.90f,                           // cold_warning_threshold
    3600,                            // dense_idle_threshold (1 hour)
    86400,                           // cold_idle_threshold (24 hours)
    3600,                            // min_age_for_migration (1 hour)
    100,                             // hot_access_threshold
    3,                               // prefetch_depth
    2,                               // prefetch_threads
    true,                            // prefetch_enabled
    10,                              // migration_batch_size
    300,                             // migration_interval_sec (5 min)
    2ULL * 1024 * 1024 * 1024,       // max_hot_bytes (2GB)
    50ULL * 1024 * 1024 * 1024,      // max_warm_bytes (50GB)
    100ULL * 1024 * 1024 * 1024      // max_dense_bytes (100GB)
};

// =============================================================================
// Tensor-Aware Delta Encoder
// =============================================================================
// Stores child nodes as delta from parent node for massive compression gains
// Parent-child nodes in same context have ~90% similarity

class ZetaTensorDelta {
public:
    // Encode node as delta from parent
    // Returns delta-encoded data (much smaller if related)
    static std::vector<uint8_t> encode_delta(
        const uint8_t* child_data, size_t child_size,
        const uint8_t* parent_data, size_t parent_size
    ) {
        // If sizes don't match, can't delta encode
        if (child_size != parent_size || child_size == 0) {
            // Return copy with "no delta" header
            std::vector<uint8_t> result(child_size + 1);
            result[0] = 0x00;  // No delta flag
            memcpy(result.data() + 1, child_data, child_size);
            return result;
        }

        // Compute XOR delta (child XOR parent)
        std::vector<uint8_t> delta(child_size + 1);
        delta[0] = 0x01;  // Delta flag

        // SIMD XOR for speed
        size_t i = 0;
#if defined(__SSE2__)
        for (; i + 16 <= child_size; i += 16) {
            __m128i c = _mm_loadu_si128((__m128i*)(child_data + i));
            __m128i p = _mm_loadu_si128((__m128i*)(parent_data + i));
            __m128i d = _mm_xor_si128(c, p);
            _mm_storeu_si128((__m128i*)(delta.data() + 1 + i), d);
        }
#elif defined(__ARM_NEON)
        for (; i + 16 <= child_size; i += 16) {
            uint8x16_t c = vld1q_u8(child_data + i);
            uint8x16_t p = vld1q_u8(parent_data + i);
            uint8x16_t d = veorq_u8(c, p);
            vst1q_u8(delta.data() + 1 + i, d);
        }
#endif
        // Remaining bytes
        for (; i < child_size; i++) {
            delta[1 + i] = child_data[i] ^ parent_data[i];
        }

        return delta;
    }

    // Decode delta to get original child data
    static std::vector<uint8_t> decode_delta(
        const uint8_t* delta_data, size_t delta_size,
        const uint8_t* parent_data, size_t parent_size
    ) {
        if (delta_size < 1) return {};

        uint8_t flag = delta_data[0];

        if (flag == 0x00) {
            // No delta, raw data
            return std::vector<uint8_t>(delta_data + 1, delta_data + delta_size);
        }

        // Delta encoded - XOR with parent
        size_t child_size = delta_size - 1;
        if (child_size != parent_size) {
            fprintf(stderr, "[HSM] Delta size mismatch!\n");
            return {};
        }

        std::vector<uint8_t> child(child_size);

        size_t i = 0;
#if defined(__SSE2__)
        for (; i + 16 <= child_size; i += 16) {
            __m128i d = _mm_loadu_si128((__m128i*)(delta_data + 1 + i));
            __m128i p = _mm_loadu_si128((__m128i*)(parent_data + i));
            __m128i c = _mm_xor_si128(d, p);
            _mm_storeu_si128((__m128i*)(child.data() + i), c);
        }
#elif defined(__ARM_NEON)
        for (; i + 16 <= child_size; i += 16) {
            uint8x16_t d = vld1q_u8(delta_data + 1 + i);
            uint8x16_t p = vld1q_u8(parent_data + i);
            uint8x16_t c = veorq_u8(d, p);
            vst1q_u8(child.data() + i, c);
        }
#endif
        for (; i < child_size; i++) {
            child[i] = delta_data[1 + i] ^ parent_data[i];
        }

        return child;
    }

    // Calculate compression ratio from delta encoding
    // Returns estimated ratio (higher = more similar, better compression)
    static float estimate_delta_ratio(
        const uint8_t* child_data, size_t child_size,
        const uint8_t* parent_data, size_t parent_size
    ) {
        if (child_size != parent_size || child_size == 0) return 1.0f;

        // Count non-zero bytes in delta
        size_t non_zero = 0;
        for (size_t i = 0; i < child_size; i++) {
            if ((child_data[i] ^ parent_data[i]) != 0) {
                non_zero++;
            }
        }

        // Ratio: lower non_zero = better compression
        float zero_ratio = 1.0f - (float)non_zero / (float)child_size;
        return 1.0f + (zero_ratio * 9.0f);  // 1x to 10x based on similarity
    }
};

// =============================================================================
// Predictive Prefetcher
// =============================================================================
// Pre-warms related nodes from cold storage before they're needed

class ZetaPrefetcher {
private:
    std::queue<std::string> prefetch_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_;

    // Callback to actually load a node
    std::function<void(const std::string&)> load_callback_;

public:
    ZetaPrefetcher() : running_(false) {}

    ~ZetaPrefetcher() {
        stop();
    }

    void set_load_callback(std::function<void(const std::string&)> callback) {
        load_callback_ = callback;
    }

    void start(int num_threads = 2) {
        if (running_) return;
        running_ = true;

        for (int i = 0; i < num_threads; i++) {
            workers_.emplace_back(&ZetaPrefetcher::worker_loop, this);
        }
        fprintf(stderr, "[HSM-PREFETCH] Started %d worker threads\n", num_threads);
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        queue_cv_.notify_all();

        for (auto& t : workers_) {
            if (t.joinable()) t.join();
        }
        workers_.clear();
        fprintf(stderr, "[HSM-PREFETCH] Stopped\n");
    }

    // Queue nodes for prefetching
    void queue_prefetch(const std::vector<std::string>& node_ids) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (const auto& id : node_ids) {
            prefetch_queue_.push(id);
        }
        queue_cv_.notify_all();
    }

    void queue_prefetch(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        prefetch_queue_.push(node_id);
        queue_cv_.notify_one();
    }

    size_t queue_size() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return prefetch_queue_.size();
    }

private:
    void worker_loop() {
        while (running_) {
            std::string node_id;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] {
                    return !prefetch_queue_.empty() || !running_;
                });

                if (!running_) break;
                if (prefetch_queue_.empty()) continue;

                node_id = prefetch_queue_.front();
                prefetch_queue_.pop();
            }

            // Do the prefetch
            if (load_callback_ && !node_id.empty()) {
                load_callback_(node_id);
            }
        }
    }
};

// =============================================================================
// Main HSM Manager
// =============================================================================

class ZetaHSM {
private:
    std::unordered_map<std::string, ZetaGKVNode> nodes_;
    mutable std::shared_mutex nodes_mutex_;

    ZetaPrefetcher prefetcher_;

    std::atomic<bool> running_;
    std::thread migration_thread_;
    std::condition_variable migration_cv_;
    std::mutex migration_mutex_;

    // Statistics
    std::atomic<uint64_t> hot_bytes_;
    std::atomic<uint64_t> warm_bytes_;
    std::atomic<uint64_t> dense_bytes_;
    std::atomic<uint64_t> cold_bytes_;
    std::atomic<uint64_t> total_prefetches_;
    std::atomic<uint64_t> total_migrations_;

public:
    ZetaHSM() : running_(false), hot_bytes_(0), warm_bytes_(0),
                dense_bytes_(0), cold_bytes_(0), total_prefetches_(0),
                total_migrations_(0) {}

    ~ZetaHSM() {
        stop();
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    bool init(const ZetaHSMConfig& config = g_hsm_config) {
        g_hsm_config = config;

        // Create directories
        create_directory(g_hsm_config.warm_dir);
        create_directory(g_hsm_config.dense_dir);
        create_directory(g_hsm_config.cold_dir);

        // Scan existing files
        scan_directory(g_hsm_config.warm_dir, ZetaStorageTier::WARM);
        scan_directory(g_hsm_config.dense_dir, ZetaStorageTier::WARM_DENSE);
        scan_directory(g_hsm_config.cold_dir, ZetaStorageTier::COLD);

        // Set up prefetcher callback
        prefetcher_.set_load_callback([this](const std::string& node_id) {
            this->prefetch_node(node_id);
        });

        fprintf(stderr, "[HSM] Initialized - %zu nodes loaded\n", nodes_.size());
        print_stats();

        return true;
    }

    void start() {
        if (running_) return;
        running_ = true;

        // Start prefetcher
        if (g_hsm_config.prefetch_enabled) {
            prefetcher_.start(g_hsm_config.prefetch_threads);
        }

        // Start migration thread
        migration_thread_ = std::thread(&ZetaHSM::migration_loop, this);

        fprintf(stderr, "[HSM] Background threads started\n");
    }

    void stop() {
        if (!running_) return;
        running_ = false;

        prefetcher_.stop();
        migration_cv_.notify_all();

        if (migration_thread_.joinable()) {
            migration_thread_.join();
        }

        fprintf(stderr, "[HSM] Stopped\n");
    }

    // =========================================================================
    // Node Registration & Access
    // =========================================================================

    void register_node(const std::string& node_id,
                       const std::string& warm_path,
                       uint64_t size,
                       float salience = 0.5f,
                       const std::string& parent_id = "") {
        std::unique_lock<std::shared_mutex> lock(nodes_mutex_);

        ZetaGKVNode node;
        node.node_id = node_id;
        node.tier = ZetaStorageTier::WARM;
        node.raw_size = size;
        node.stored_size = size;
        node.access_count = 0;
        node.last_access = time(nullptr);
        node.created = time(nullptr);
        node.salience = salience;
        node.momentum = 1.0f;
        node.momentum_integral = 1.0f;  // Start with some credit
        node.parent_id = parent_id;
        node.is_delta_encoded = false;
        node.warm_path = warm_path;
        node.dense_path = g_hsm_config.dense_dir + "/" + node_id + ".agkv";
        node.cold_path = g_hsm_config.cold_dir + "/" + node_id + ".agkv";
        node.is_locked = false;
        node.is_prefetching = false;

        // Link to parent
        if (!parent_id.empty()) {
            auto it = nodes_.find(parent_id);
            if (it != nodes_.end()) {
                it->second.children.push_back(node_id);
            }
        }

        nodes_[node_id] = node;
        warm_bytes_ += size;

        fprintf(stderr, "[HSM] Registered: %s (%.2f MB, parent=%s)\n",
                node_id.c_str(), size / (1024.0 * 1024.0),
                parent_id.empty() ? "none" : parent_id.c_str());
    }

    // Record access and trigger prefetch of related nodes
    void record_access(const std::string& node_id) {
        std::unique_lock<std::shared_mutex> lock(nodes_mutex_);

        auto it = nodes_.find(node_id);
        if (it == nodes_.end()) return;

        it->second.access_count++;
        it->second.last_access = time(nullptr);
        it->second.momentum = it->second.momentum * 0.9f + 0.1f;
        it->second.momentum_integral = it->second.momentum_integral * 0.98f + it->second.momentum;

        // Trigger prefetch of related nodes
        if (g_hsm_config.prefetch_enabled) {
            std::vector<std::string> to_prefetch;

            // Add children
            for (const auto& child : it->second.children) {
                auto child_it = nodes_.find(child);
                if (child_it != nodes_.end() &&
                    child_it->second.tier >= ZetaStorageTier::WARM_DENSE) {
                    to_prefetch.push_back(child);
                }
            }

            // Add siblings
            for (const auto& sib : it->second.siblings) {
                auto sib_it = nodes_.find(sib);
                if (sib_it != nodes_.end() &&
                    sib_it->second.tier >= ZetaStorageTier::WARM_DENSE) {
                    to_prefetch.push_back(sib);
                }
            }

            lock.unlock();

            if (!to_prefetch.empty()) {
                prefetcher_.queue_prefetch(to_prefetch);
            }
        }
    }

    // Get tier for a node
    ZetaStorageTier get_tier(const std::string& node_id) {
        std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
        auto it = nodes_.find(node_id);
        if (it == nodes_.end()) return ZetaStorageTier::COLD;
        return it->second.tier;
    }

    // Get path to node data (may trigger decompression)
    std::string get_path(const std::string& node_id, bool blocking = true) {
        std::shared_lock<std::shared_mutex> lock(nodes_mutex_);

        auto it = nodes_.find(node_id);
        if (it == nodes_.end()) return "";

        ZetaStorageTier tier = it->second.tier;
        std::string path;

        switch (tier) {
            case ZetaStorageTier::HOT:
            case ZetaStorageTier::WARM:
                path = it->second.warm_path;
                break;

            case ZetaStorageTier::WARM_DENSE:
                lock.unlock();
                if (blocking) {
                    promote_to_warm(node_id);
                    return get_path(node_id, false);
                }
                return it->second.dense_path;

            case ZetaStorageTier::COLD:
                lock.unlock();
                if (blocking) {
                    promote_to_warm(node_id);
                    return get_path(node_id, false);
                }
                return it->second.cold_path;
        }

        return path;
    }

    // Add graph relationship
    void add_relationship(const std::string& node_id, const std::string& related_id) {
        std::unique_lock<std::shared_mutex> lock(nodes_mutex_);

        auto it = nodes_.find(node_id);
        auto rel_it = nodes_.find(related_id);

        if (it != nodes_.end() && rel_it != nodes_.end()) {
            it->second.siblings.push_back(related_id);
            rel_it->second.siblings.push_back(node_id);
        }
    }

    // =========================================================================
    // Statistics
    // =========================================================================

    void print_stats() {
        std::shared_lock<std::shared_mutex> lock(nodes_mutex_);

        int hot = 0, warm = 0, dense = 0, cold = 0;
        for (const auto& [id, node] : nodes_) {
            switch (node.tier) {
                case ZetaStorageTier::HOT: hot++; break;
                case ZetaStorageTier::WARM: warm++; break;
                case ZetaStorageTier::WARM_DENSE: dense++; break;
                case ZetaStorageTier::COLD: cold++; break;
            }
        }

        fprintf(stderr, "\n╔══════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║           Z.E.T.A. HSM Statistics                ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════╝\n");
        fprintf(stderr, "Tier         │ Nodes │ Size       │ Latency\n");
        fprintf(stderr, "─────────────┼───────┼────────────┼─────────\n");
        fprintf(stderr, "HOT (VRAM)   │ %5d │ %7.1f MB │  0.3 ms\n",
                hot, hot_bytes_.load() / (1024.0 * 1024.0));
        fprintf(stderr, "WARM (NVMe)  │ %5d │ %7.1f MB │  0.3 ms\n",
                warm, warm_bytes_.load() / (1024.0 * 1024.0));
        fprintf(stderr, "DENSE (NVMe) │ %5d │ %7.1f MB │  ~5 ms\n",
                dense, dense_bytes_.load() / (1024.0 * 1024.0));
        fprintf(stderr, "COLD (HDD)   │ %5d │ %7.1f MB │ ~50 ms\n",
                cold, cold_bytes_.load() / (1024.0 * 1024.0));
        fprintf(stderr, "─────────────┴───────┴────────────┴─────────\n");
        fprintf(stderr, "Prefetches: %lu  Migrations: %lu\n",
                (unsigned long)total_prefetches_.load(),
                (unsigned long)total_migrations_.load());
        fprintf(stderr, "NVMe usage: %.1f%%  HDD usage: %.1f%%\n",
                get_nvme_usage() * 100.0f, get_hdd_usage() * 100.0f);
        fprintf(stderr, "\n");
    }

    std::string get_stats_json() {
        std::shared_lock<std::shared_mutex> lock(nodes_mutex_);

        int hot = 0, warm = 0, dense = 0, cold = 0;
        for (const auto& [id, node] : nodes_) {
            switch (node.tier) {
                case ZetaStorageTier::HOT: hot++; break;
                case ZetaStorageTier::WARM: warm++; break;
                case ZetaStorageTier::WARM_DENSE: dense++; break;
                case ZetaStorageTier::COLD: cold++; break;
            }
        }

        char buf[1024];
        snprintf(buf, sizeof(buf),
            R"({"hsm":{"hot":{"nodes":%d,"bytes":%lu},"warm":{"nodes":%d,"bytes":%lu},"dense":{"nodes":%d,"bytes":%lu},"cold":{"nodes":%d,"bytes":%lu},"nvme_pct":%.1f,"hdd_pct":%.1f,"prefetches":%lu,"migrations":%lu}})",
            hot, (unsigned long)hot_bytes_.load(),
            warm, (unsigned long)warm_bytes_.load(),
            dense, (unsigned long)dense_bytes_.load(),
            cold, (unsigned long)cold_bytes_.load(),
            get_nvme_usage() * 100.0f,
            get_hdd_usage() * 100.0f,
            (unsigned long)total_prefetches_.load(),
            (unsigned long)total_migrations_.load()
        );
        return std::string(buf);
    }

private:
    // =========================================================================
    // Directory Operations
    // =========================================================================

    void create_directory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            std::string cmd = "mkdir -p " + path;
            system(cmd.c_str());
        }
    }

    void scan_directory(const std::string& path, ZetaStorageTier tier) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') continue;

            std::string name = entry->d_name;
            std::string full_path = path + "/" + name;

            struct stat st;
            if (stat(full_path.c_str(), &st) != 0) continue;

            // Extract node ID
            std::string node_id = name;
            size_t dot = node_id.find('.');
            if (dot != std::string::npos) {
                node_id = node_id.substr(0, dot);
            }

            // Create node entry
            ZetaGKVNode node;
            node.node_id = node_id;
            node.tier = tier;
            node.stored_size = st.st_size;
            node.raw_size = st.st_size;  // Will be updated if compressed
            node.access_count = 0;
            node.last_access = st.st_atime;
            node.created = st.st_ctime;
            node.salience = 0.5f;
            node.momentum = 0.1f;
            node.momentum_integral = 0.1f;  // Discovered nodes start low
            node.is_delta_encoded = false;
            node.warm_path = g_hsm_config.warm_dir + "/" + node_id + ".bin";
            node.dense_path = g_hsm_config.dense_dir + "/" + node_id + ".agkv";
            node.cold_path = g_hsm_config.cold_dir + "/" + node_id + ".agkv";
            node.is_locked = false;
            node.is_prefetching = false;

            {
                std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
                nodes_[node_id] = node;
            }

            // Update tier stats
            switch (tier) {
                case ZetaStorageTier::HOT: hot_bytes_ += st.st_size; break;
                case ZetaStorageTier::WARM: warm_bytes_ += st.st_size; break;
                case ZetaStorageTier::WARM_DENSE: dense_bytes_ += st.st_size; break;
                case ZetaStorageTier::COLD: cold_bytes_ += st.st_size; break;
            }
        }

        closedir(dir);
    }

    float get_nvme_usage() {
        struct statvfs st;
        if (statvfs(g_hsm_config.warm_dir.c_str(), &st) != 0) return 0.0f;
        uint64_t total = st.f_blocks * st.f_frsize;
        uint64_t free = st.f_bfree * st.f_frsize;
        return (float)(total - free) / (float)total;
    }

    float get_hdd_usage() {
        struct statvfs st;
        if (statvfs(g_hsm_config.cold_dir.c_str(), &st) != 0) return 0.0f;
        uint64_t total = st.f_blocks * st.f_frsize;
        uint64_t free = st.f_bfree * st.f_frsize;
        return (float)(total - free) / (float)total;
    }

    // =========================================================================
    // Migration Logic
    // =========================================================================

    void migration_loop() {
        fprintf(stderr, "[HSM] Migration thread started\n");

        while (running_) {
            std::unique_lock<std::mutex> lock(migration_mutex_);
            migration_cv_.wait_for(lock,
                std::chrono::seconds(g_hsm_config.migration_interval_sec));

            if (!running_) break;

            // Check if we need to migrate
            float nvme_usage = get_nvme_usage();

            if (nvme_usage > g_hsm_config.dense_to_cold_threshold) {
                // Migrate dense → cold
                migrate_tier(ZetaStorageTier::WARM_DENSE, ZetaStorageTier::COLD);
            }

            if (nvme_usage > g_hsm_config.warm_to_dense_threshold) {
                // Migrate warm → dense
                migrate_tier(ZetaStorageTier::WARM, ZetaStorageTier::WARM_DENSE);
            }

            // Warn on HDD usage
            float hdd_usage = get_hdd_usage();
            if (hdd_usage > g_hsm_config.cold_warning_threshold) {
                fprintf(stderr, "[HSM] WARNING: HDD at %.1f%% capacity!\n",
                        hdd_usage * 100.0f);
            }
        }

        fprintf(stderr, "[HSM] Migration thread stopped\n");
    }

    void migrate_tier(ZetaStorageTier from, ZetaStorageTier to) {
        // Get candidates for migration
        std::vector<std::string> candidates;
        time_t now = time(nullptr);

        {
            std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
            for (const auto& [id, node] : nodes_) {
                if (node.tier != from) continue;
                if (node.is_locked) continue;
                if (now - node.created < g_hsm_config.min_age_for_migration) continue;

                int idle_threshold = (to == ZetaStorageTier::COLD)
                    ? g_hsm_config.cold_idle_threshold
                    : g_hsm_config.dense_idle_threshold;

                if (now - node.last_access >= idle_threshold) {
                    candidates.push_back(id);
                }
            }
        }

        // Sort by coldness (oldest access first)
        std::sort(candidates.begin(), candidates.end(),
            [this](const std::string& a, const std::string& b) {
                std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
                return nodes_[a].last_access < nodes_[b].last_access;
            });

        // Migrate batch
        int migrated = 0;
        for (const auto& id : candidates) {
            if (migrated >= g_hsm_config.migration_batch_size) break;

            if (migrate_node(id, to)) {
                migrated++;
                total_migrations_++;
            }
        }

        if (migrated > 0) {
            fprintf(stderr, "[HSM] Migrated %d nodes: %s → %s\n",
                    migrated, tier_name(from), tier_name(to));
        }
    }

    bool migrate_node(const std::string& node_id, ZetaStorageTier to) {
        std::string src_path, dst_path;
        ZetaStorageTier from;
        uint64_t src_size;

        {
            std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
            auto it = nodes_.find(node_id);
            if (it == nodes_.end()) return false;

            from = it->second.tier;
            src_size = it->second.stored_size;

            switch (from) {
                case ZetaStorageTier::WARM: src_path = it->second.warm_path; break;
                case ZetaStorageTier::WARM_DENSE: src_path = it->second.dense_path; break;
                default: return false;
            }

            switch (to) {
                case ZetaStorageTier::WARM_DENSE: dst_path = it->second.dense_path; break;
                case ZetaStorageTier::COLD: dst_path = it->second.cold_path; break;
                default: return false;
            }
        }

        // Read source
        FILE* f = fopen(src_path.c_str(), "rb");
        if (!f) return false;

        std::vector<uint8_t> data(src_size);
        size_t read = fread(data.data(), 1, src_size, f);
        fclose(f);

        if (read != src_size) return false;

        // Compress
        std::vector<uint8_t> compressed;
        if (to == ZetaStorageTier::WARM_DENSE) {
            compressed = aura_gkv_compress_fast(data.data(), data.size());
        } else {
            compressed = aura_gkv_compress_balanced(data.data(), data.size());
        }

        // Write destination
        FILE* out = fopen(dst_path.c_str(), "wb");
        if (!out) return false;

        fwrite(compressed.data(), 1, compressed.size(), out);
        fclose(out);

        // Update node
        {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            auto it = nodes_.find(node_id);
            if (it != nodes_.end()) {
                // Update stats
                switch (from) {
                    case ZetaStorageTier::WARM: warm_bytes_ -= src_size; break;
                    case ZetaStorageTier::WARM_DENSE: dense_bytes_ -= src_size; break;
                    default: break;
                }
                switch (to) {
                    case ZetaStorageTier::WARM_DENSE: dense_bytes_ += compressed.size(); break;
                    case ZetaStorageTier::COLD: cold_bytes_ += compressed.size(); break;
                    default: break;
                }

                it->second.tier = to;
                it->second.stored_size = compressed.size();
            }
        }

        // Remove source if migrating to different storage
        if (from == ZetaStorageTier::WARM && to == ZetaStorageTier::WARM_DENSE) {
            // Keep warm file, just mark as dense
        } else {
            unlink(src_path.c_str());
        }

        return true;
    }

    // =========================================================================
    // Prefetch & Promotion
    // =========================================================================

    void prefetch_node(const std::string& node_id) {
        ZetaStorageTier tier;
        std::string src_path;
        std::string dst_path;
        uint64_t stored_size;

        {
            std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
            auto it = nodes_.find(node_id);
            if (it == nodes_.end()) return;
            if (it->second.is_prefetching) return;
            if (it->second.tier < ZetaStorageTier::WARM_DENSE) return;

            tier = it->second.tier;
            stored_size = it->second.stored_size;

            if (tier == ZetaStorageTier::COLD) {
                src_path = it->second.cold_path;
                dst_path = it->second.dense_path;  // Prefetch to dense first
            } else {
                src_path = it->second.dense_path;
                dst_path = it->second.warm_path;
            }
        }

        // Mark as prefetching
        {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            nodes_[node_id].is_prefetching = true;
        }

        // Read compressed file
        FILE* f = fopen(src_path.c_str(), "rb");
        if (!f) {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            nodes_[node_id].is_prefetching = false;
            return;
        }

        std::vector<uint8_t> compressed(stored_size);
        size_t bytes_read = fread(compressed.data(), 1, stored_size, f);
        fclose(f);
        
        if (bytes_read != stored_size) {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            nodes_[node_id].is_prefetching = false;
            return;
        }

        // Decompress
        std::vector<uint8_t> data;
        if (tier == ZetaStorageTier::COLD) {
            data = aura_gkv_decompress_balanced(compressed.data(), compressed.size());
        } else {
            data = aura_gkv_decompress_fast(compressed.data(), compressed.size());
        }

        if (data.empty()) {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            nodes_[node_id].is_prefetching = false;
            return;
        }

        // Write to warmer tier
        FILE* out = fopen(dst_path.c_str(), "wb");
        if (out) {
            fwrite(data.data(), 1, data.size(), out);
            fclose(out);

            // Update node
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            auto it = nodes_.find(node_id);
            if (it != nodes_.end()) {
                ZetaStorageTier new_tier = (tier == ZetaStorageTier::COLD)
                    ? ZetaStorageTier::WARM_DENSE
                    : ZetaStorageTier::WARM;

                // Update stats
                if (tier == ZetaStorageTier::COLD) {
                    cold_bytes_ -= stored_size;
                    dense_bytes_ += data.size();
                } else {
                    dense_bytes_ -= stored_size;
                    warm_bytes_ += data.size();
                }

                it->second.tier = new_tier;
                it->second.stored_size = data.size();
                it->second.is_prefetching = false;
            }
        } else {
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            nodes_[node_id].is_prefetching = false;
        }

        total_prefetches_++;
    }

    void promote_to_warm(const std::string& node_id) {
        prefetch_node(node_id);

        // If it was cold, prefetch again to get to warm
        ZetaStorageTier tier;
        {
            std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
            auto it = nodes_.find(node_id);
            if (it == nodes_.end()) return;
            tier = it->second.tier;
        }

        if (tier == ZetaStorageTier::WARM_DENSE) {
            prefetch_node(node_id);
        }
    }
};

// =============================================================================
// Global Instance & Convenience Functions
// =============================================================================

static ZetaHSM g_hsm;

static inline bool zeta_hsm_init(const std::string& cold_dir = "/mnt/HDD/zeta/cold") {
    g_hsm_config.cold_dir = cold_dir;
    return g_hsm.init();
}

static inline void zeta_hsm_start() { g_hsm.start(); }
static inline void zeta_hsm_stop() { g_hsm.stop(); }

static inline void zeta_hsm_register_gkv(
    const std::string& node_id,
    const std::string& path,
    uint64_t size,
    float salience = 0.5f,
    const std::string& parent_id = ""
) {
    g_hsm.register_node(node_id, path, size, salience, parent_id);
}

static inline void zeta_hsm_access(const std::string& node_id) {
    g_hsm.record_access(node_id);
}

static inline std::string zeta_hsm_get_path(const std::string& node_id) {
    return g_hsm.get_path(node_id);
}

static inline void zeta_hsm_add_relation(const std::string& a, const std::string& b) {
    g_hsm.add_relationship(a, b);
}

static inline void zeta_hsm_print_stats() { g_hsm.print_stats(); }
static inline std::string zeta_hsm_stats_json() { return g_hsm.get_stats_json(); }

#endif // ZETA_HSM_H
