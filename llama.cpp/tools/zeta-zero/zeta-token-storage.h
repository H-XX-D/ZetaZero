// Z.E.T.A. Token Cache (persistent, LRU-capped)
// Caches tokenized snippets to avoid repeated llama_tokenize calls and to surface
// accurate token counts for streaming / KV warmup.

#ifndef ZETA_TOKEN_STORAGE_H
#define ZETA_TOKEN_STORAGE_H

#include "llama.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <time.h>

namespace zeta_token_cache {

struct Entry {
    std::vector<llama_token> tokens;
    uint64_t text_hash = 0;
    uint64_t last_access = 0;
};

// Simple capped cache with whole-file persistence.
static std::unordered_map<int64_t, Entry> g_cache;
static size_t g_total_tokens = 0;
static size_t g_max_tokens = 200000;   // Total tokens across entries
static size_t g_max_entries = 4096;    // Hard entry cap
static std::string g_persist_path;
static bool g_enabled = false;
static std::mutex g_mu;

static inline uint64_t fnv1a_hash(const char* data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline void maybe_evict() {
    if (g_total_tokens <= g_max_tokens && g_cache.size() <= g_max_entries) return;

    // Evict lowest-access entries until within budget
    std::vector<std::pair<int64_t, uint64_t>> order;
    order.reserve(g_cache.size());
    for (const auto& kv : g_cache) {
        order.push_back({kv.first, kv.second.last_access});
    }
    std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    for (const auto& victim : order) {
        auto it = g_cache.find(victim.first);
        if (it == g_cache.end()) continue;
        if (g_total_tokens > it->second.tokens.size()) {
            g_total_tokens -= it->second.tokens.size();
        } else {
            g_total_tokens = 0;
        }
        g_cache.erase(it);
        if (g_total_tokens <= g_max_tokens && g_cache.size() <= g_max_entries) break;
    }
}

static inline void persist() {
    if (!g_enabled || g_persist_path.empty()) return;
    std::ofstream out(g_persist_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;

    uint32_t magic = 0x5a455441;  // 'ZETA'
    uint32_t version = 1;
    uint32_t entries = (uint32_t)g_cache.size();
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&entries), sizeof(entries));

    for (const auto& kv : g_cache) {
        int64_t id = kv.first;
        const Entry& e = kv.second;
        uint32_t count = (uint32_t)e.tokens.size();
        out.write(reinterpret_cast<const char*>(&id), sizeof(id));
        out.write(reinterpret_cast<const char*>(&e.text_hash), sizeof(e.text_hash));
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (!e.tokens.empty()) {
            out.write(reinterpret_cast<const char*>(e.tokens.data()), sizeof(llama_token) * e.tokens.size());
        }
    }
}

static inline void load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return;

    uint32_t magic = 0, version = 0, entries = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&entries), sizeof(entries));
    if (magic != 0x5a455441 || version != 1) return;

    for (uint32_t i = 0; i < entries; i++) {
        int64_t id = 0;
        Entry e;
        uint32_t count = 0;
        in.read(reinterpret_cast<char*>(&id), sizeof(id));
        in.read(reinterpret_cast<char*>(&e.text_hash), sizeof(e.text_hash));
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        e.tokens.resize(count);
        if (count > 0) {
            in.read(reinterpret_cast<char*>(e.tokens.data()), sizeof(llama_token) * count);
        }
        e.last_access = (uint64_t)time(nullptr);
        g_total_tokens += e.tokens.size();
        g_cache[id] = std::move(e);
        if (g_total_tokens > g_max_tokens) break;  // stop if loaded cache exceeds cap
    }
}

static inline void init(const std::string& persist_dir, size_t max_tokens, size_t max_entries) {
    std::lock_guard<std::mutex> lock(g_mu);
    g_enabled = true;
    g_max_tokens = max_tokens > 0 ? max_tokens : g_max_tokens;
    g_max_entries = max_entries > 0 ? max_entries : g_max_entries;
    g_persist_path = persist_dir + "/token_cache.bin";
    g_cache.clear();
    g_total_tokens = 0;
    load(g_persist_path);
}

// Get tokens for text; tokenizes and caches on miss.
static inline int get_or_tokenize(
    struct llama_vocab* vocab,
    const char* text,
    int64_t id,
    std::vector<llama_token>& out_tokens,
    bool add_special = true,
    bool parse_special = true
) {
    if (!g_enabled || !vocab || !text) return 0;

    size_t len = strlen(text);
    uint64_t h = fnv1a_hash(text, len);

    {
        std::lock_guard<std::mutex> lock(g_mu);
        auto it = g_cache.find(id);
        if (it != g_cache.end() && it->second.text_hash == h) {
            out_tokens = it->second.tokens;
            it->second.last_access = (uint64_t)time(nullptr);
            return (int)out_tokens.size();
        }
    }

    // Miss: tokenize now
    out_tokens.resize(len + 16);  // upper bound
    int n_tokens = llama_tokenize(vocab, text, (int)len,
                                  out_tokens.data(), (int)out_tokens.size(),
                                  add_special, parse_special);
    if (n_tokens <= 0) {
        out_tokens.clear();
        return 0;
    }
    out_tokens.resize(n_tokens);

    // Store in cache
    {
        std::lock_guard<std::mutex> lock(g_mu);
        Entry e;
        e.tokens = out_tokens;
        e.text_hash = h;
        e.last_access = (uint64_t)time(nullptr);

        auto it = g_cache.find(id);
        if (it != g_cache.end()) {
            if (g_total_tokens > it->second.tokens.size()) {
                g_total_tokens -= it->second.tokens.size();
            } else {
                g_total_tokens = 0;
            }
        }

        g_cache[id] = std::move(e);
        g_total_tokens += out_tokens.size();
        maybe_evict();
        persist();
    }

    return n_tokens;
}

static inline int token_count(int64_t id, const char* text) {
    if (!g_enabled || !text) return 0;
    uint64_t h = fnv1a_hash(text, strlen(text));
    std::lock_guard<std::mutex> lock(g_mu);
    auto it = g_cache.find(id);
    if (it != g_cache.end() && it->second.text_hash == h) {
        return (int)it->second.tokens.size();
    }
    return 0;
}

static inline void stats(size_t& entries, size_t& total_tokens, size_t& max_tokens, size_t& max_entries) {
    std::lock_guard<std::mutex> lock(g_mu);
    entries = g_cache.size();
    total_tokens = g_total_tokens;
    max_tokens = g_max_tokens;
    max_entries = g_max_entries;
}

} // namespace zeta_token_cache

#endif // ZETA_TOKEN_STORAGE_H
