#pragma once
#include <string>
#include <vector>
#include <regex>
#include <sstream>

namespace zeta_extract {

struct Fact {
    std::string type;
    std::string key;
    std::string value;
};

inline std::vector<Fact> extract_facts(const std::string& text) {
    std::vector<Fact> facts;
    std::smatch m;
    std::string::const_iterator start;
    
    // Name pattern
    std::regex name_re("(?:my name is|i am|call me)\\s+([A-Z][a-z]+)", std::regex::icase);
    start = text.begin();
    while (std::regex_search(start, text.end(), m, name_re)) {
        Fact f; f.type = "name"; f.key = "user_name"; f.value = m[1].str();
        facts.push_back(f);
        start = m.suffix().first;
    }
    
    // Number pattern
    std::regex num_re("(?:favorite|lucky|secret|number|is)\\s*(?:is)?\\s*(\\d+)");
    start = text.begin();
    while (std::regex_search(start, text.end(), m, num_re)) {
        Fact f; f.type = "number"; f.key = "important_number"; f.value = m[1].str();
        facts.push_back(f);
        start = m.suffix().first;
    }
    
    // Preference pattern
    std::regex like_re("i\\s+(?:love|like|prefer)\\s+([a-z]+)", std::regex::icase);
    start = text.begin();
    while (std::regex_search(start, text.end(), m, like_re)) {
        Fact f; f.type = "preference"; f.key = "likes"; f.value = m[1].str();
        facts.push_back(f);
        start = m.suffix().first;
    }
    
    // Code pattern
    std::regex code_re("code\\s+(?:is\\s+)?([A-Z0-9-]+)", std::regex::icase);
    start = text.begin();
    while (std::regex_search(start, text.end(), m, code_re)) {
        Fact f; f.type = "code"; f.key = "secret_code"; f.value = m[1].str();
        facts.push_back(f);
        start = m.suffix().first;
    }
    
    return facts;
}

inline std::string facts_to_string(const std::vector<Fact>& facts) {
    if (facts.empty()) return "";
    std::ostringstream ss;
    for (size_t i = 0; i < facts.size(); i++) {
        ss << facts[i].type << ":" << facts[i].key << "=" << facts[i].value << "\n";
    }
    return ss.str();
}

inline std::vector<Fact> string_to_facts(const std::string& s) {
    std::vector<Fact> facts;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line)) {
        size_t c = line.find(58);
        size_t e = line.find(61);
        if (c != std::string::npos && e != std::string::npos) {
            Fact f;
            f.type = line.substr(0, c);
            f.key = line.substr(c+1, e-c-1);
            f.value = line.substr(e+1);
            facts.push_back(f);
        }
    }
    return facts;
}

inline std::string facts_to_natural(const std::vector<Fact>& facts) {
    if (facts.empty()) return "";
    std::ostringstream ss;
    for (size_t i = 0; i < facts.size(); i++) {
        const Fact& f = facts[i];
        if (f.type == "name") ss << "User name is " << f.value << ". ";
        else if (f.type == "number") ss << "Important number: " << f.value << ". ";
        else if (f.type == "preference") ss << "User likes " << f.value << ". ";
        else if (f.type == "code") ss << "Secret code: " << f.value << ". ";
        else ss << f.key << ": " << f.value << ". ";
    }
    return ss.str();
}

}
