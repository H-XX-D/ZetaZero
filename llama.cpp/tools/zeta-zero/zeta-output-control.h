// Z.E.T.A. Output Control System
// Dynamic complexity-based limits instead of hard caps
//
// Problems identified from MT-Bench:
// 1. Q12 Turn 2: 7840 char word salad - needs runaway detection
// 2. Q2 Turn 2: Story truncated - needs dynamic limits for creative tasks
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
#include <cmath>

// === BASE LIMITS (scaled by complexity) ===
#define ZETA_BASE_OUTPUT_CHARS 1000     // Base for simple queries
#define ZETA_BASE_OUTPUT_WORDS 150      // Base word count
#define ZETA_MAX_COMPLEXITY_SCALE 6.0f  // Max multiplier (6x base = 6000 chars)
#define ZETA_VOCAB_DIVERSITY_MIN 0.25f  // Min unique words / total words ratio
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
    float complexity;              // 0.0-1.0 complexity score
};

// === ESTIMATE PROMPT COMPLEXITY ===
// Returns 0.0-1.0 score based on prompt characteristics
static inline float zeta_estimate_complexity(const char* prompt) {
    if (!prompt) return 0.3f;  // Default to low-medium

    std::string p(prompt);
    float score = 0.0f;

    // Length factor (longer prompts = more complex tasks)
    size_t len = p.length();
    if (len > 100) score += 0.1f;
    if (len > 300) score += 0.1f;
    if (len > 600) score += 0.1f;
    if (len > 1000) score += 0.1f;

    // Multi-step indicators
    if (p.find("step by step") != std::string::npos) score += 0.15f;
    if (p.find("first") != std::string::npos && p.find("then") != std::string::npos) score += 0.1f;
    if (p.find("explain") != std::string::npos && p.find("detail") != std::string::npos) score += 0.15f;

    // Code complexity indicators
    if (p.find("implement") != std::string::npos) score += 0.2f;
    if (p.find("algorithm") != std::string::npos) score += 0.15f;
    if (p.find("optimize") != std::string::npos) score += 0.1f;
    if (p.find("refactor") != std::string::npos) score += 0.15f;
    if (p.find("debug") != std::string::npos) score += 0.1f;

    // Creative complexity
    if (p.find("story") != std::string::npos) score += 0.25f;
    if (p.find("chapter") != std::string::npos) score += 0.2f;
    if (p.find("dialogue") != std::string::npos) score += 0.15f;
    if (p.find("character") != std::string::npos) score += 0.1f;

    // Analysis complexity
    if (p.find("compare") != std::string::npos) score += 0.1f;
    if (p.find("analyze") != std::string::npos) score += 0.15f;
    if (p.find("evaluate") != std::string::npos) score += 0.1f;
    if (p.find("pros and cons") != std::string::npos) score += 0.1f;

    // Question count (multiple questions = more complex)
    int questions = 0;
    for (char c : p) if (c == '?') questions++;
    score += questions * 0.05f;

    // Clamp to 0.0-1.0
    return std::min(1.0f, std::max(0.1f, score));
}

// === CALCULATE DYNAMIC LIMITS ===
static inline void zeta_calc_dynamic_limits(ZetaOutputControl& ctrl, float complexity) {
    // Scale factor: complexity 0.1 = 1x, complexity 1.0 = MAX_COMPLEXITY_SCALE
    float scale = 1.0f + (complexity * (ZETA_MAX_COMPLEXITY_SCALE - 1.0f));

    ctrl.complexity = complexity;
    ctrl.max_chars = (int)(ZETA_BASE_OUTPUT_CHARS * scale);
    ctrl.max_words = (int)(ZETA_BASE_OUTPUT_WORDS * scale);

    // Mode-specific adjustments on top of complexity scaling
    switch (ctrl.mode) {
        case OUTPUT_MODE_JSON:
            ctrl.max_chars = std::min(ctrl.max_chars, 2000);  // JSON shouldn't be huge
            break;
        case OUTPUT_MODE_CONCISE:
            ctrl.max_chars = std::min(ctrl.max_chars, 800);   // Keep concise short
            ctrl.max_words = std::min(ctrl.max_words, 120);
            break;
        case OUTPUT_MODE_CREATIVE:
            ctrl.max_chars = (int)(ctrl.max_chars * 1.5f);    // Creative gets extra room
            ctrl.max_words = (int)(ctrl.max_words * 1.5f);
            break;
        case OUTPUT_MODE_CODE:
            ctrl.max_chars = (int)(ctrl.max_chars * 1.3f);    // Code needs more space
            break;
        default:
            break;
    }

    fprintf(stderr, "[OUTPUT_CTRL] Complexity=%.2f → scale=%.1fx → max_chars=%d, max_words=%d\n",
            complexity, scale, ctrl.max_chars, ctrl.max_words);
}

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

// === GET OUTPUT LIMITS FOR MODE (with dynamic complexity) ===
static inline ZetaOutputControl zeta_get_output_control(ZetaOutputMode mode, const char* prompt = nullptr) {
    ZetaOutputControl ctrl;
    ctrl.mode = mode;
    ctrl.enforce_format = false;
    ctrl.format_wrapper = "";
    ctrl.required_keywords = "";
    ctrl.complexity = 0.3f;  // Default

    // Estimate complexity from prompt
    float complexity = zeta_estimate_complexity(prompt);

    // Set base values then apply dynamic scaling
    switch (mode) {
        case OUTPUT_MODE_JSON:
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "```json\n";
            break;

        case OUTPUT_MODE_CODE:
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "```";
            break;

        case OUTPUT_MODE_TABLE:
            ctrl.enforce_format = true;
            ctrl.format_wrapper = "|";
            break;

        case OUTPUT_MODE_CONCISE:
            ctrl.enforce_format = false;
            break;

        case OUTPUT_MODE_CREATIVE:
            ctrl.enforce_format = false;
            break;

        default:
            ctrl.enforce_format = false;
            break;
    }

    // Apply dynamic limits based on complexity
    zeta_calc_dynamic_limits(ctrl, complexity);

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
// Returns: 0 = OK, 1 = soft warning (approaching limit), 2 = hard stop (runaway detected)
// DISABLED FOR FAIR BENCHMARKING - let max_tokens control output length
static inline int zeta_check_verbosity_runaway(const std::string& output,
                                                const ZetaOutputControl& ctrl) {
    (void)output;
    (void)ctrl;
    return 0;  // DISABLED - always OK
#if 0  // Original code disabled for benchmarking
    int status = 0;

    // Soft warning at 80% of limit
    int soft_char_limit = (int)(ctrl.max_chars * 0.8f);
    int soft_word_limit = (int)(ctrl.max_words * 0.8f);

    // Character limit check
    if ((int)output.size() >= ctrl.max_chars) {
        fprintf(stderr, "[OUTPUT_CTRL] Char limit reached (%zu >= %d) - stopping\n",
                output.size(), ctrl.max_chars);
        return 2;  // Hard stop
    } else if ((int)output.size() >= soft_char_limit) {
        status = 1;  // Soft warning
    }

    // Word count check
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
        fprintf(stderr, "[OUTPUT_CTRL] Word limit reached (%d >= %d) - stopping\n",
                word_count, ctrl.max_words);
        return 2;  // Hard stop
    } else if (word_count >= soft_word_limit) {
        status = std::max(status, 1);  // Soft warning
    }

    // Vocabulary diversity check (detect word salad) - this is a HARD stop for quality
    if (output.size() > 500) {
        std::unordered_set<std::string> unique_words;
        std::string word;
        int total_words = 0;

        for (char c : output) {
            if (isalpha(c)) {
                word += tolower(c);
            } else if (!word.empty()) {
                if (word.length() >= 3) {
                    unique_words.insert(word);
                    total_words++;
                }
                word.clear();
            }
        }

        if (total_words > 50) {
            float diversity = (float)unique_words.size() / total_words;
            if (diversity < ZETA_VOCAB_DIVERSITY_MIN) {
                fprintf(stderr, "[OUTPUT_CTRL] LOW DIVERSITY RUNAWAY (%.2f < %.2f) - hard stop\n",
                        diversity, ZETA_VOCAB_DIVERSITY_MIN);
                return 2;  // Hard stop - quality issue
            }
        }
    }

    // N-gram repetition check (detect phrase loops) - HARD stop for quality
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
                    fprintf(stderr, "[OUTPUT_CTRL] Repeated 4-gram detected: '%s' (%d times) - hard stop\n",
                            ngram.c_str(), ngrams[ngram]);
                    return 2;  // Hard stop - phrase looping
                }
            }
        }
    }

    return status;  // 0 = OK, 1 = soft warning
#endif
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
// Returns: 0 = OK, 1 = soft warning, 2 = hard stop
static inline int zeta_should_stop_output(const std::string& output,
                                           const char* original_prompt) {
    ZetaOutputMode mode = zeta_detect_output_mode(original_prompt);
    ZetaOutputControl ctrl = zeta_get_output_control(mode, original_prompt);  // Pass prompt for dynamic limits!

    return zeta_check_verbosity_runaway(output, ctrl);
}

// === POST-PROCESS OUTPUT ===
// Call after generation to validate and fix format
static inline std::string zeta_postprocess_output(const std::string& output,
                                                   const char* original_prompt) {
    ZetaOutputMode mode = zeta_detect_output_mode(original_prompt);
    ZetaOutputControl ctrl = zeta_get_output_control(mode, original_prompt);  // Pass prompt for dynamic limits!

    std::string result = output;

    // Enforce character limit
    if (result.size() > (size_t)ctrl.max_chars) {
        // Find a good break point (end of sentence)
        size_t break_pos = (size_t)ctrl.max_chars;
        for (size_t i = (size_t)ctrl.max_chars; i > (size_t)ctrl.max_chars - 200 && i > 0; i--) {
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
