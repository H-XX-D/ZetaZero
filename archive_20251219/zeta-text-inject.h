#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include "zeta-extract.h"

namespace zeta_inject {
    static std::map<int64_t, std::string> texts;
    static std::string storage;

    inline void set_storage(const std::string& dir) { storage = dir; }

    inline void save(int64_t id, const std::string& raw_text) {
        std::vector<zeta_extract::Fact> facts = zeta_extract::extract_facts(raw_text);
        std::string compact = zeta_extract::facts_to_string(facts);
        
        if (compact.empty()) {
            fprintf(stderr, "[ZETA] No key facts found\n");
            return;
        }
        
        texts[id] = compact;
        std::ofstream f(storage + "/facts_" + std::to_string(id) + ".txt");
        if (f) {
            f << compact;
            fprintf(stderr, "[ZETA] Extracted %zu facts\n", facts.size());
        }
    }

    inline std::string load(int64_t id) {
        std::map<int64_t, std::string>::iterator it = texts.find(id);
        if (it != texts.end()) return it->second;
        std::ifstream f(storage + "/facts_" + std::to_string(id) + ".txt");
        if (f) { std::stringstream b; b << f.rdbuf(); texts[id] = b.str(); return texts[id]; }
        std::ifstream f2(storage + "/text_" + std::to_string(id) + ".txt");
        if (f2) { std::stringstream b; b << f2.rdbuf(); texts[id] = b.str(); return texts[id]; }
        return "";
    }

    inline void load_all() {
        DIR* d = opendir(storage.c_str());
        if (!d) return;
        struct dirent* e;
        while ((e = readdir(d))) {
            std::string n = e->d_name;
            if (n.find("facts_") == 0 || n.find("text_") == 0) {
                size_t start = n.find('_') + 1;
                load(std::stoll(n.substr(start)));
            }
        }
        closedir(d);
    }

    inline std::string build_prompt(const std::string& user) {
        if (texts.empty()) load_all();
        if (texts.empty()) return user;
        
        std::string mem;
        for (std::map<int64_t, std::string>::iterator it = texts.begin(); it != texts.end(); ++it) {
            if (!it->second.empty()) {
                std::vector<zeta_extract::Fact> facts = zeta_extract::string_to_facts(it->second);
                std::string natural = zeta_extract::facts_to_natural(facts);
                if (!natural.empty()) mem += natural;
            }
        }
        if (mem.empty()) return user;
        
        return "<|im_start|>system\nYou know these facts:\n" + mem + 
               "\n<|im_end|>\n<|im_start|>user\n" + user + 
               "<|im_end|>\n<|im_start|>assistant\n";
    }
}
