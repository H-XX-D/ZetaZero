// ZETA Text Memory - System Prompt Injection
// Stores prompt text alongside memory blocks for hard retrieval
#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>

namespace zeta_text {

static std::map<int64_t, std::string> g_block_texts;
static std::string g_storage_dir = "/tmp/zeta";

inline void set_storage_dir(const std::string& dir) {
    g_storage_dir = dir;
}

inline void save_block_text(int64_t block_id, const std::string& text) {
    g_block_texts[block_id] = text;
    std::string path = g_storage_dir + "/block_" + std::to_string(block_id) + ".txt";
    std::ofstream f(path);
    if (f.is_open()) {
        f << text;
        f.close();
    }
}

inline std::string load_block_text(int64_t block_id) {
    auto it = g_block_texts.find(block_id);
    if (it != g_block_texts.end()) return it->second;
    
    std::string path = g_storage_dir + "/block_" + std::to_string(block_id) + ".txt";
    std::ifstream f(path);
    if (f.is_open()) {
        std::stringstream buf;
        buf << f.rdbuf();
        std::string text = buf.str();
        g_block_texts[block_id] = text;
        return text;
    }
    return "";
}

inline void load_all_texts() {
    // Load all .txt files from storage dir
    for (int i = 0; i < 1000; i++) {
        std::string path = g_storage_dir + "/block_" + std::to_string(i) + ".txt";
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buf;
            buf << f.rdbuf();
            g_block_texts[i] = buf.str();
            f.close();
        }
    }
}

inline std::string build_memory_prompt(const std::string& user_prompt, float score_threshold = 0.5f) {
    // Check if we have any stored texts
    if (g_block_texts.empty()) {
        load_all_texts();
    }
    
    if (g_block_texts.empty()) {
        return user_prompt;
    }
    
    // Gather all stored memories
    std::string all_memories;
    for (const auto& [id, text] : g_block_texts) {
        if (!text.empty() && text.find(user_prompt) == std::string::npos) {
            all_memories += "[Memory " + std::to_string(id) + "]: " + text + "\n";
        }
    }
    
    if (all_memories.empty()) {
        return user_prompt;
    }
    
    // Build system prompt with memory context (Qwen format)
    std::string memory_prompt = 
        "<|im_start|>system\n"
        "You have access to memories from previous conversations. Relevant context:\n"
        "---\n" + all_memories + "---\n"
        "Use this context to answer questions about previous interactions.\n"
        "<|im_end|>\n"
        "<|im_start|>user\n" + user_prompt + "<|im_end|>\n"
        "<|im_start|>assistant\n";
    
    return memory_prompt;
}

} // namespace zeta_text
