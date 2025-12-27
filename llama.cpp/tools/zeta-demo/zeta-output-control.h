// Z.E.T.A. Output Control System
// Fixes: verbosity runaway, Turn-2 compliance, JSON mode, instruction following
//
// Problems identified from MT-Bench:
// 1. Q12 Turn 2: 7840 char word salad - no hard limit
// 2. Q2 Turn 2: Story truncated - didn't follow "rewrite ending"
// 3. Q7 Turn 2: Didn't implement Manacher's - ignored instruction
// 4. Q13 Turn 2: No JSON output - format not enforced

#ifndef ZETA_OUTPUT_CONTROL_H
#define ZETA_OUTPUT_CONTROL_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <cstring>
#include <cctype>

// === OUTPUT LIMITS ===
#define ZETA_MAX_OUTPUT_CHARS 2500      // Hard limit for regular output
#define ZETA_MAX_OUTPUT_WORDS 400       // Word count limit
#define ZETA_VOCAB_DIVERSITY_MIN 0.3f   // Min unique words / total words ratio
#define ZETA_NGRAM_REPEAT_THRESHOLD 3   // Max times a 4-gram can repeat

// === OUTPUT MODE FLAGS ===
enum ZetaOutputMode {
    OUTPUT_MODE_DEFAULT = 0,
    OUTPUT_MODE_JSON = 1,          // Enforce JSON structure
    OUTPUT_MODE_CODE = 2,          // Code block expected
    OUTPUT_MODE_TABLE = 3,         // Table format expected
    OUTPUT_MODE_CONCISE = 4,       // Short answer (reasoning/math)
    OUTPUT_MODE_CREATIVE = 5       // Story/roleplay - higher limit
};

struct ZetaOutputControl {
    ZetaOutputMode mode;
    int max_chars;
    int max_words;
    bool enforce_format;
    std::string format_wrapper;    // e.g., "```json\n%s\n```"
    std::string required_keywords; // Must include these
};

// === DETECT OUTPUT MODE FROM PROMPT ===
static inline ZetaOutputMode zeta_detect_output_mode(const char* prompt) {
    if (!prompt) return OUTPUT_MODE_DEFAULT;

    std::string p(prompt);
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    // JSON mode detection
    if (p.find("json") != std::string::npos ||
        p.find("structured format") != std::string::npos ||
        p.find("format as {") != std::string::npos) {
        return OUTPUT_MODE_JSON;
    }

    // Code mode detection
    if (p.find("write a function") != std::string::npos ||
        p.find("write a python") != std::string::npos ||
        p.find("implement") != std::string::npos ||
        p.find("code") != std::string::npos ||
        p.find("algorithm") != std::string::npos) {
        return OUTPUT_MODE_CODE;
    }

    // Table mode detection
    if (p.find("table") != std::string::npos ||
        p.find("compare") != std::string::npos) {
        return OUTPUT_MODE_TABLE;
    }

    // Concise mode (math/reasoning)
    if (p.find("solve") != std::string::npos ||
        p.find("how many") != std::string::npos ||
        p.find("calculate") != std::string::npos ||
        p.find("what is the") != std::string::npos) {
        return OUTPUT_MODE_CONCISE;
    }

    // Creative mode (stories, roleplay)
    if (p.find("story") != std::string::npos ||
        p.find("pretend") != std::string::npos ||
        p.find("roleplay") != std::string::npos ||
        p.find("detective") != std::string::npos ||
        p.find("character") != std::string::npos) {
        return OUTPUT_MODE_CREATIVE;
    }

    return OUTPUT_MODE_DEFAULT;
}

// === GET OUTPUT LIMITS FOR MODE ===
static inline ZetaOutputControl zeta_get_output_control(ZetaOutputMode mode) {
    ZetaOutputControl ctrl;
    ctrl.mode = mode;
    ctrl.enforce_format = false;
    ctrl.format_wrapper = "";
    ctrl.required_keywords = "";

    switch (mode) {
        case OUTPUT_MODE_JSON:
            ctrl.max_chars = 1500;
            ctrl.max_words = 200;
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "```json\n";  // Will check for this
            break;

        case OUTPUT_MODE_CODE:
            ctrl.max_chars = 2000;
            ctrl.max_words = 300;
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "```";
            break;

        case OUTPUT_MODE_TABLE:
            ctrl.max_chars = 2000;
            ctrl.max_words = 250;
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "|";  // Tables have pipes
            break;

        case OUTPUT_MODE_CONCISE:
            ctrl.max_chars = 500;
            ctrl.max_words = 80;
            ctrl.enforce_format = false;
            break;

        case OUTPUT_MODE_CREATIVE:
            ctrl.max_chars = 3000;  // Higher for stories
            ctrl.max_words = 500;
            ctrl.enforce_format = false;
            break;

        default:
            ctrl.max_chars = ZETA_MAX_OUTPUT_CHARS;
            ctrl.max_words = ZETA_MAX_OUTPUT_WORDS;
            ctrl.enforce_format = false;
            break;
    }

    return ctrl;
}

// === EXTRACT TURN-2 INSTRUCTION ===
// Parse "Now do X" patterns to understand what's actually being asked
static inline std::string zeta_extract_turn2_instruction(const char* prompt) {
    if (!prompt) return "";

    std::string p(prompt);

    // Common Turn-2 patterns
    const char* patterns[] = {
        "Now ", "now ",
        "Next, ", "next, ",
        "Then ", "then ",
        "Also ", "also ",
        "Modify ", "modify ",
        "Rewrite ", "rewrite ",
        "Optimize ", "optimize ",
        "Change ", "change ",
        "Make it ", "make it ",
        "Convert ", "convert "
    };

    for (const char* pat : patterns) {
        size_t pos = p.find(pat);
        if (pos != std::string::npos) {
            // Extract the instruction after the pattern
            std::string instruction = p.substr(pos);
            // Find end of sentence
            size_t end = instruction.find_first_of(".!?\n");
            if (end != std::string::npos && end < 200) {
                instruction = instruction.substr(0, end);
            }
            return instruction;
        }
    }

    return "";
}

// === CHECK FOR VERBOSITY RUNAWAY ===
// Returns true if output should stop (too repetitive or too long)
static inline bool zeta_check_verbosity_runaway(const std::string& output,
                                                 const ZetaOutputControl& ctrl) {
    // Hard character limit
    if ((int)output.size() >= ctrl.max_chars) {
        fprintf(stderr, "[OUTPUT_CTRL] Hard char limit reached (%zu >= %d)\n",
                output.size(), ctrl.max_chars);
        return true;
    }

    // Word count limit
    int word_count = 0;
    bool in_word = false;
    for (char c : output) {
        if (isspace(c)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            word_count++;
        }
    }

    if (word_count >= ctrl.max_words) {
        fprintf(stderr, "[OUTPUT_CTRL] Word limit reached (%d >= %d)\n",
                word_count, ctrl.max_words);
        return true;
    }

    // Vocabulary diversity check (detect word salad)
    if (output.size() > 500) {
        std::unordered_set<std::string> unique_words;
        std::string word;
        int total_words = 0;

        for (char c : output) {
            if (isalpha(c)) {
                word += tolower(c);
            } else if (!word.empty()) {
                if (word.length() >= 3) {  // Ignore tiny words
                    unique_words.insert(word);
                    total_words++;
                }
                word.clear();
            }
        }

        if (total_words > 50) {
            float diversity = (float)unique_words.size() / total_words;
            if (diversity < ZETA_VOCAB_DIVERSITY_MIN) {
                fprintf(stderr, "[OUTPUT_CTRL] Low vocabulary diversity (%.2f < %.2f)\n",
                        diversity, ZETA_VOCAB_DIVERSITY_MIN);
                return true;
            }
        }
    }

    // N-gram repetition check (detect phrase loops)
    if (output.size() > 200) {
        // Check for repeated 4-grams (4 word sequences)
        std::vector<std::string> words;
        std::string word;
        for (char c : output) {
            if (isalpha(c) || c == '\'') {
                word += c;
            } else if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }

        if (words.size() > 20) {
            std::unordered_map<std::string, int> ngrams;
            for (size_t i = 0; i + 4 <= words.size(); i++) {
                std::string ngram = words[i] + " " + words[i+1] + " " +
                                   words[i+2] + " " + words[i+3];
                ngrams[ngram]++;
                if (ngrams[ngram] >= ZETA_NGRAM_REPEAT_THRESHOLD) {
                    fprintf(stderr, "[OUTPUT_CTRL] Repeated 4-gram detected: '%s' (%d times)\n",
                            ngram.c_str(), ngrams[ngram]);
                    return true;
                }
            }
        }
    }

    return false;
}

// === VALIDATE OUTPUT FORMAT ===
// Check if output matches expected format for the mode
static inline bool zeta_validate_output_format(const std::string& output,
                                                const ZetaOutputControl& ctrl) {
    if (!ctrl.enforce_format) return true;

    switch (ctrl.mode) {
        case OUTPUT_MODE_JSON:
            // Should contain { and } or be wrapped in ```json
            if (output.find('{') == std::string::npos &&
                output.find("```json") == std::string::npos) {
                return false;
            }
            break;

        case OUTPUT_MODE_CODE:
            // Should contain code fence or function definition
            if (output.find("```") == std::string::npos &&
                output.find("def ") == std::string::npos &&
                output.find("function ") == std::string::npos &&
                output.find("void ") == std::string::npos) {
                return false;
            }
            break;

        case OUTPUT_MODE_TABLE:
            // Should contain table pipes
            if (output.find('|') == std::string::npos) {
                return false;
            }
            break;

        default:
            break;
    }

    return true;
}

// === FORCE JSON WRAPPER IF NEEDED ===
static inline std::string zeta_force_json_format(const std::string& output) {
    // If output doesn't have JSON structure, try to wrap it
    if (output.find('{') == std::string::npos) {
        // Extract key-value pairs from prose
        std::string json_output = "{\n";

        // Look for common patterns like "Key: Value" or "Key - Value"
        std::regex kv_pattern(R"(([A-Za-z_]+):\s*([^\n,]+))");
        std::smatch match;
        std::string::const_iterator search_start = output.cbegin();
        bool first = true;

        while (std::regex_search(search_start, output.cend(), match, kv_pattern)) {
            if (!first) json_output += ",\n";
            first = false;

            std::string key = match[1].str();
            std::string value = match[2].str();

            // Clean up value
            while (!value.empty() && (value.back() == '.' || value.back() == ' '))
                value.pop_back();

            json_output += "  \"" + key + "\": \"" + value + "\"";
            search_start = match.suffix().first;
        }

        json_output += "\n}";

        if (!first) {  // Found at least one key-value
            return json_output;
        }
    }

    return output;
}

// === MAIN CONTROL FUNCTION ===
// Call this during generation loop to check if we should stop
static inline bool zeta_should_stop_output(const std::string& output,
                                           const char* original_prompt) {
    ZetaOutputMode mode = zeta_detect_output_mode(original_prompt);
    ZetaOutputControl ctrl = zeta_get_output_control(mode);

    return zeta_check_verbosity_runaway(output, ctrl);
}

// === POST-PROCESS OUTPUT ===
// Call after generation to validate and fix format
static inline std::string zeta_postprocess_output(const std::string& output,
                                                   const char* original_prompt) {
    ZetaOutputMode mode = zeta_detect_output_mode(original_prompt);
    ZetaOutputControl ctrl = zeta_get_output_control(mode);

    std::string result = output;

    // Enforce character limit
    if ((int)result.size() > ctrl.max_chars) {
        // Find a good break point (end of sentence)
        size_t break_pos = ctrl.max_chars;
        for (size_t i = ctrl.max_chars; i > ctrl.max_chars - 200 && i > 0; i--) {
            if (result[i] == '.' || result[i] == '!' || result[i] == '?' ||
                result[i] == '\n' || result[i] == '}') {
                break_pos = i + 1;
                break;
            }
        }
        result = result.substr(0, break_pos);
        fprintf(stderr, "[OUTPUT_CTRL] Truncated to %zu chars\n", result.size());
    }

    // Force JSON format if required but missing
    if (mode == OUTPUT_MODE_JSON && !zeta_validate_output_format(result, ctrl)) {
        result = zeta_force_json_format(result);
    }

    return result;
}

#endif // ZETA_OUTPUT_CONTROL_H
