// Z.E.T.A. Critic Layer - Semantic Self-Verification System
// Uses 7B to semantically verify 14B outputs against constraints
// Pattern matching is FALLBACK only - 7B semantic analysis is primary

#ifndef ZETA_CRITIC_H
#define ZETA_CRITIC_H

#include <string>
#include <vector>
#include <regex>
#include <cstring>
#include <map>
#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>

// Forward declarations for semantic critic (defined in server)
struct llama_model;
struct llama_context;

// Critic result structure
typedef struct {
    bool has_issues;
    int issue_count;
    char issues[4][512];        // Up to 4 issues
    char severity[4][16];       // CRITICAL, WARNING, INFO
    float confidence;
    bool was_semantic;          // True if 7B was used, false if pattern fallback
} zeta_critic_result_t;

// Callback type for semantic generation (set by server)
typedef std::function<std::string(const char* prompt, int max_tokens)> semantic_gen_fn;
static semantic_gen_fn g_semantic_generate = nullptr;

// Set the semantic generation callback (called by server on init)
static void zeta_critic_set_semantic_fn(semantic_gen_fn fn) {
    g_semantic_generate = fn;
}

// Domain detection for critic
typedef enum {
    CRITIC_DOMAIN_GENERAL,
    CRITIC_DOMAIN_ALGORITHMS,
    CRITIC_DOMAIN_HFT_TRADING,
    CRITIC_DOMAIN_DISTRIBUTED_SYSTEMS,
    CRITIC_DOMAIN_DEBUGGING,
    CRITIC_DOMAIN_SECURITY,
    CRITIC_DOMAIN_ML_AI
} zeta_critic_domain_t;

// Complexity patterns that indicate O(N) or worse
// Language-agnostic patterns for Python, Java, JS, Rust, etc.
static const char* COMPLEXITY_VIOLATIONS[] = {
    // Java
    "Collections.min(",      // O(N) in Java
    "Collections.max(",      // O(N) in Java
    "Collections.sort(",     // O(N log N)
    // Python
    "min(self.",             // O(N) min on collection
    "max(self.",             // O(N) max on collection
    "sorted(",               // O(N log N)
    "list.sort(",            // O(N log N)
    // JavaScript
    "Math.min(...",          // O(N) spread
    "Math.max(...",          // O(N) spread
    ".reduce(",              // Often O(N)
    // Generic
    ".sort(",                // Generic sort
    ".indexOf(",             // O(N) search
    ".contains(",            // Often O(N)
    ".filter(",              // O(N)
    ".find(",                // O(N)
    "for i in range",        // Loop indicator
    "for (int i",            // Loop indicator
    NULL
};

// Bug patterns for debugging scenarios
static const char* BUG_PATTERNS[] = {
    // Callback/completion issues
    "onComplete.*never.*fire",
    "callback.*not.*called",
    "listener.*not.*registered",
    "event.*not.*triggered",
    // Memory leak patterns
    "map.*never.*remove",
    "collection.*grows.*indefinitely",
    "reference.*not.*cleared",
    "weak.*reference",
    // Concurrency issues
    "race.*condition",
    "deadlock",
    "thread.*safe",
    NULL
};

// HFT-specific anti-patterns
static const char* HFT_ANTIPATTERNS[] = {
    "Mutex",
    "lock(",
    "synchronized",
    "Arc<Mutex",
    "RwLock",
    "atomic.*compare",
    NULL
};

// HFT-required patterns
static const char* HFT_REQUIREMENTS[] = {
    "lock-free",
    "wait-free",
    "single-threaded",
    "ring buffer",
    "SPSC",
    "disruptor",
    "pinned",
    "core affinity",
    NULL
};

// ============================================================================
// REDUNDANCY DETECTION
// ============================================================================

// Check for repetitive n-grams (phrases) in the response
static bool zeta_check_redundancy(const char* response, char* issue_out, float* redundancy_score) {
    if (!response || strlen(response) < 200) return false;

    std::string text(response);

    // Convert to lowercase for comparison
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Count repeated phrases (5+ words)
    std::map<std::string, int> phrase_counts;
    std::vector<std::string> words;

    // Tokenize into words
    std::string word;
    for (char c : lower) {
        if (std::isalnum(c)) {
            word += c;
        } else if (!word.empty()) {
            words.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) words.push_back(word);

    // Count 5-grams
    int total_ngrams = 0;
    int repeated_ngrams = 0;

    for (size_t i = 0; i + 5 <= words.size(); i++) {
        std::string ngram = words[i] + " " + words[i+1] + " " + words[i+2] + " " + words[i+3] + " " + words[i+4];
        phrase_counts[ngram]++;
        total_ngrams++;
    }

    // Count how many n-grams appear more than once
    for (const auto& kv : phrase_counts) {
        if (kv.second > 1) {
            repeated_ngrams += (kv.second - 1);  // Excess occurrences
        }
    }

    // Calculate redundancy score (0.0 = no redundancy, 1.0 = fully redundant)
    float score = (total_ngrams > 0) ? (float)repeated_ngrams / total_ngrams : 0.0f;
    if (redundancy_score) *redundancy_score = score;

    // Also check for exact sentence repetition
    std::vector<std::string> sentences;
    std::string sentence;
    for (char c : text) {
        if (c == '.' || c == '!' || c == '?') {
            if (sentence.length() > 20) {
                // Normalize
                std::string norm = sentence;
                std::transform(norm.begin(), norm.end(), norm.begin(), ::tolower);
                sentences.push_back(norm);
            }
            sentence.clear();
        } else {
            sentence += c;
        }
    }

    std::map<std::string, int> sentence_counts;
    for (const auto& s : sentences) {
        sentence_counts[s]++;
    }

    int repeated_sentences = 0;
    std::string worst_repeat;
    int worst_count = 0;
    for (const auto& kv : sentence_counts) {
        if (kv.second > 1) {
            repeated_sentences++;
            if (kv.second > worst_count) {
                worst_count = kv.second;
                worst_repeat = kv.first.substr(0, 50);
            }
        }
    }

    // Thresholds: >15% repeated n-grams OR >2 repeated sentences = issue
    if (score > 0.15f || repeated_sentences > 2) {
        if (repeated_sentences > 2) {
            snprintf(issue_out, 512,
                "REDUNDANCY: %d sentences repeated. Example: '%s...' (appears %dx). "
                "Response needs more variety.",
                repeated_sentences, worst_repeat.c_str(), worst_count);
        } else {
            snprintf(issue_out, 512,
                "REDUNDANCY: %.0f%% of phrases are repeated. Response is too repetitive.",
                score * 100);
        }
        return true;
    }

    return false;
}

// Detect domain from prompt
static zeta_critic_domain_t zeta_critic_detect_domain(const char* prompt) {
    if (!prompt) return CRITIC_DOMAIN_GENERAL;

    std::string p(prompt);
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    if (p.find("o(1)") != std::string::npos ||
        p.find("o(n)") != std::string::npos ||
        p.find("complexity") != std::string::npos ||
        p.find("algorithm") != std::string::npos) {
        return CRITIC_DOMAIN_ALGORITHMS;
    }

    if (p.find("hft") != std::string::npos ||
        p.find("high-frequency") != std::string::npos ||
        p.find("trading") != std::string::npos ||
        p.find("order book") != std::string::npos ||
        p.find("matching engine") != std::string::npos) {
        return CRITIC_DOMAIN_HFT_TRADING;
    }

    if (p.find("distributed") != std::string::npos ||
        p.find("cluster") != std::string::npos ||
        p.find("replication") != std::string::npos ||
        p.find("failover") != std::string::npos) {
        return CRITIC_DOMAIN_DISTRIBUTED_SYSTEMS;
    }

    if (p.find("debug") != std::string::npos ||
        p.find("bug") != std::string::npos ||
        p.find("memory leak") != std::string::npos ||
        p.find("oom") != std::string::npos ||
        p.find("crash") != std::string::npos) {
        return CRITIC_DOMAIN_DEBUGGING;
    }

    return CRITIC_DOMAIN_GENERAL;
}

// Check if response uses wrong programming language
static bool zeta_check_language_mismatch(const char* prompt, const char* response, char* issue_out) {
    if (!prompt || !response) return false;

    std::string p(prompt);
    std::string r(response);
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    // Detect requested language
    struct LangPattern {
        const char* request;     // What user asks for
        const char* markers[4];  // Code markers that indicate this language
        const char* wrong[4];    // Markers that indicate WRONG language
    };

    static const LangPattern langs[] = {
        {"python", {"def ", "class ", "import ", "print("}, {"fn ", "func ", "public static", "```rust"}},
        {"java", {"public class", "public static void", "System.out"}, {"def ", "fn ", "func ", "```python"}},
        {"rust", {"fn ", "let mut", "impl ", "pub fn"}, {"def ", "class ", "public static", "```python"}},
        {"javascript", {"function ", "const ", "let ", "=>"}, {"def ", "fn ", "public static", "```python"}},
        {"go", {"func ", "package ", "import \"", "fmt."}, {"def ", "fn ", "class ", "```python"}},
        {"c++", {"#include", "std::", "int main", "cout"}, {"def ", "fn ", "func ", "```python"}},
        {nullptr, {}, {}}
    };

    for (int i = 0; langs[i].request; i++) {
        if (p.find(langs[i].request) != std::string::npos) {
            // User asked for this language - check for wrong language markers
            for (int j = 0; j < 4 && langs[i].wrong[j]; j++) {
                if (r.find(langs[i].wrong[j]) != std::string::npos) {
                    // Check if correct language markers are also present
                    bool has_correct = false;
                    for (int k = 0; k < 4 && langs[i].markers[k]; k++) {
                        if (r.find(langs[i].markers[k]) != std::string::npos) {
                            has_correct = true;
                            break;
                        }
                    }
                    if (!has_correct) {
                        snprintf(issue_out, 512,
                            "LANGUAGE: Prompt requests %s but response uses different language (found '%s')",
                            langs[i].request, langs[i].wrong[j]);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Check if response claims O(1) but uses O(N) operations
static bool zeta_check_complexity_violation(const char* prompt, const char* response, char* issue_out) {
    if (!prompt || !response) return false;

    std::string p(prompt);
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    // Only check if prompt asks for O(1)
    if (p.find("o(1)") == std::string::npos) return false;

    std::string r(response);

    for (int i = 0; COMPLEXITY_VIOLATIONS[i]; i++) {
        if (r.find(COMPLEXITY_VIOLATIONS[i]) != std::string::npos) {
            snprintf(issue_out, 512,
                "COMPLEXITY: Prompt requires O(1) but code uses '%s' which is O(N) or worse",
                COMPLEXITY_VIOLATIONS[i]);
            return true;
        }
    }

    return false;
}

// Check if debugging response addresses the actual bug
static bool zeta_check_bug_identification(const char* prompt, const char* response, char* issue_out) {
    if (!prompt || !response) return false;

    std::string p(prompt);
    std::string r(response);
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);

    // Check for callback/completion pattern in prompt
    if (p.find("onComplete") != std::string::npos ||
        p.find("callback") != std::string::npos) {

        bool found_callback_issue = false;
        for (int i = 0; BUG_PATTERNS[i]; i++) {
            if (r.find(BUG_PATTERNS[i]) != std::string::npos) {
                found_callback_issue = true;
                break;
            }
        }

        // Check if response mentions the callback might not fire
        if (!found_callback_issue &&
            r.find("callback") == std::string::npos &&
            r.find("oncomplete") == std::string::npos &&
            r.find("never fire") == std::string::npos &&
            r.find("not called") == std::string::npos) {

            snprintf(issue_out, 512,
                "BUG_MISS: Code has callback-based cleanup but response doesn't analyze if callback fires");
            return true;
        }
    }

    // Check for Map cleanup issues
    if ((p.find("Map") != std::string::npos || p.find("map") != std::string::npos) &&
        (p.find("memory") != std::string::npos || p.find("leak") != std::string::npos)) {

        if (r.find("remove") == std::string::npos &&
            r.find("clear") == std::string::npos &&
            r.find("evict") == std::string::npos &&
            r.find("cleanup") == std::string::npos) {

            snprintf(issue_out, 512,
                "BUG_MISS: Memory leak involves Map but response doesn't discuss entry removal");
            return true;
        }
    }

    return false;
}

// Check HFT-specific requirements
static bool zeta_check_hft_requirements(const char* prompt, const char* response, char* issue_out) {
    if (!prompt || !response) return false;

    zeta_critic_domain_t domain = zeta_critic_detect_domain(prompt);
    if (domain != CRITIC_DOMAIN_HFT_TRADING) return false;

    std::string r(response);

    // Check for anti-patterns (locks in hot path)
    for (int i = 0; HFT_ANTIPATTERNS[i]; i++) {
        if (r.find(HFT_ANTIPATTERNS[i]) != std::string::npos) {
            // Check if they're discussing removing it vs using it
            std::string lower_r = r;
            std::transform(lower_r.begin(), lower_r.end(), lower_r.begin(), ::tolower);

            if (lower_r.find("avoid") == std::string::npos &&
                lower_r.find("don't use") == std::string::npos &&
                lower_r.find("remove") == std::string::npos &&
                lower_r.find("replace") == std::string::npos) {

                snprintf(issue_out, 512,
                    "HFT_PERF: Uses '%s' in HFT context - locks cause unacceptable latency. "
                    "Consider lock-free structures or single-threaded design.",
                    HFT_ANTIPATTERNS[i]);
                return true;
            }
        }
    }

    return false;
}

// =============================================================================
// SEMANTIC CRITIC - Uses 7B model to analyze response intelligently
// =============================================================================

// Build a semantic critique prompt for 7B
static std::string zeta_build_semantic_prompt(const char* user_prompt, const char* response) {
    std::string critique;
    critique.reserve(4096);

    critique += "<|im_start|>system\n";
    critique += "You are a code reviewer analyzing if a response meets the requirements. ";
    critique += "Be concise. Output ONLY in this format:\n";
    critique += "ISSUE|severity|description\n";
    critique += "Or if the response is correct: PASS\n";
    critique += "Severities: CRITICAL (wrong/dangerous), WARNING (suboptimal), INFO (style)\n";
    critique += "<|im_end|>\n";

    critique += "<|im_start|>user\n";
    critique += "REQUIREMENTS:\n";
    critique += std::string(user_prompt).substr(0, 800);
    critique += "\n\nRESPONSE:\n";
    critique += std::string(response).substr(0, 1500);
    critique += "\n\nAnalyze: Does this response satisfy ALL requirements? ";
    critique += "Check for:\n";
    critique += "- Algorithm complexity (O(1) vs O(N) if specified)\n";
    critique += "- Thread safety issues (locks in low-latency contexts)\n";
    critique += "- Correctness of logic and edge cases\n";
    critique += "- Completeness (missing required parts)\n";
    critique += "<|im_end|>\n";
    critique += "<|im_start|>assistant\n";

    return critique;
}

// Parse 7B's semantic critique response
static void zeta_parse_semantic_response(const std::string& response, zeta_critic_result_t& result) {
    result.issue_count = 0;
    result.has_issues = false;
    result.was_semantic = true;
    result.confidence = 0.9f;  // High confidence for semantic analysis

    // Check for PASS
    if (response.find("PASS") != std::string::npos &&
        response.find("ISSUE") == std::string::npos) {
        return;  // No issues
    }

    // Parse ISSUE|severity|description lines
    std::istringstream iss(response);
    std::string line;

    while (std::getline(iss, line) && result.issue_count < 4) {
        // Look for ISSUE|severity|description OR severity|type|description format
        // 7B sometimes outputs: INFO|style|message or WARNING|logic|message
        bool is_issue_line = (line.find("ISSUE") == 0 || line.find("issue") == 0);
        bool is_severity_line = (line.find("INFO") == 0 || line.find("WARNING") == 0 ||
                                  line.find("CRITICAL") == 0 || line.find("ERROR") == 0);

        if (is_issue_line || is_severity_line) {
            size_t first_pipe = line.find('|');
            size_t second_pipe = (first_pipe != std::string::npos) ?
                                  line.find('|', first_pipe + 1) : std::string::npos;

            std::string severity;
            std::string description;

            if (is_severity_line && second_pipe != std::string::npos) {
                // Format: SEVERITY|type|description - severity is before first pipe
                severity = line.substr(0, first_pipe);
                description = line.substr(second_pipe + 1);
            } else if (first_pipe != std::string::npos && second_pipe != std::string::npos) {
                // Format: ISSUE|severity|description
                severity = line.substr(first_pipe + 1, second_pipe - first_pipe - 1);
                description = line.substr(second_pipe + 1);
            } else if (first_pipe != std::string::npos) {
                // Format: SEVERITY|description (no type)
                severity = line.substr(0, first_pipe);
                description = line.substr(first_pipe + 1);
            }

            if (!description.empty()) {

                // Trim whitespace
                while (!severity.empty() && isspace(severity.front())) severity.erase(0, 1);
                while (!severity.empty() && isspace(severity.back())) severity.pop_back();
                while (!description.empty() && isspace(description.front())) description.erase(0, 1);
                while (!description.empty() && isspace(description.back())) description.pop_back();

                if (!description.empty()) {
                    strncpy(result.issues[result.issue_count], description.c_str(), 511);
                    result.issues[result.issue_count][511] = 0;

                    // Normalize severity
                    std::transform(severity.begin(), severity.end(), severity.begin(), ::toupper);
                    if (severity.find("CRIT") != std::string::npos) {
                        strncpy(result.severity[result.issue_count], "CRITICAL", 15);
                    } else if (severity.find("WARN") != std::string::npos) {
                        strncpy(result.severity[result.issue_count], "WARNING", 15);
                    } else {
                        strncpy(result.severity[result.issue_count], "INFO", 15);
                    }

                    result.issue_count++;
                    result.has_issues = true;
                }
            }
        }
        // Also catch free-form issue descriptions
        else if (line.find("complexity") != std::string::npos ||
                 line.find("O(N)") != std::string::npos ||
                 line.find("O(n)") != std::string::npos ||
                 line.find("lock") != std::string::npos ||
                 line.find("Mutex") != std::string::npos ||
                 line.find("incorrect") != std::string::npos ||
                 line.find("wrong") != std::string::npos ||
                 line.find("missing") != std::string::npos) {
            if (line.length() > 10 && line.length() < 500) {
                strncpy(result.issues[result.issue_count], line.c_str(), 511);
                result.issues[result.issue_count][511] = 0;
                strncpy(result.severity[result.issue_count], "WARNING", 15);
                result.issue_count++;
                result.has_issues = true;
            }
        }
    }
}

// Main critic function - SEMANTIC FIRST, pattern matching fallback
static zeta_critic_result_t zeta_critic_analyze(const char* prompt, const char* response) {
    zeta_critic_result_t result = {0};
    result.confidence = 0.0f;
    result.was_semantic = false;

    if (!prompt || !response || strlen(response) < 50) {
        return result;
    }

    // ==========================================================
    // SEMANTIC ANALYSIS (Primary) - Use 7B to understand issues
    // ==========================================================
    if (g_semantic_generate) {
        std::string critique_prompt = zeta_build_semantic_prompt(prompt, response);
        std::string critique_response = g_semantic_generate(critique_prompt.c_str(), 300);

        if (!critique_response.empty()) {
            fprintf(stderr, "[CRITIC-SEMANTIC] 7B Analysis: %s\n",
                    critique_response.substr(0, 200).c_str());
            zeta_parse_semantic_response(critique_response, result);

            if (result.has_issues || critique_response.find("PASS") != std::string::npos) {
                // Semantic analysis completed successfully
                return result;
            }
        }
        fprintf(stderr, "[CRITIC-SEMANTIC] Fallback to pattern matching\n");
    }

    // ==========================================================
    // PATTERN MATCHING (Fallback) - Only if semantic unavailable
    // ==========================================================
    char issue[512];
    int idx = 0;

    // Check complexity violations
    if (zeta_check_complexity_violation(prompt, response, issue) && idx < 4) {
        strncpy(result.issues[idx], issue, 511);
        strncpy(result.severity[idx], "CRITICAL", 15);
        idx++;
    }

    // Check language mismatch (e.g., asked for Python, got Rust)
    if (zeta_check_language_mismatch(prompt, response, issue) && idx < 4) {
        strncpy(result.issues[idx], issue, 511);
        strncpy(result.severity[idx], "CRITICAL", 15);
        idx++;
    }

    // Check bug identification
    if (zeta_check_bug_identification(prompt, response, issue) && idx < 4) {
        strncpy(result.issues[idx], issue, 511);
        strncpy(result.severity[idx], "WARNING", 15);
        idx++;
    }

    // Check HFT requirements
    if (zeta_check_hft_requirements(prompt, response, issue) && idx < 4) {
        strncpy(result.issues[idx], issue, 511);
        strncpy(result.severity[idx], "CRITICAL", 15);
        idx++;
    }

    // Check for redundancy/repetition
    float redundancy_score = 0.0f;
    if (zeta_check_redundancy(response, issue, &redundancy_score) && idx < 4) {
        strncpy(result.issues[idx], issue, 511);
        strncpy(result.severity[idx], "WARNING", 15);
        idx++;
    }

    result.issue_count = idx;
    result.has_issues = (idx > 0);
    result.confidence = (idx > 0) ? 0.6f : 0.0f;  // Lower confidence for pattern matching
    result.was_semantic = false;

    return result;
}

// Format critic feedback as a prompt for 7B to expand on
static std::string zeta_critic_format_feedback(const zeta_critic_result_t& result) {
    if (!result.has_issues) return "";

    std::string feedback = "\n\n---\n**Self-Check Issues Detected:**\n";

    for (int i = 0; i < result.issue_count; i++) {
        feedback += "- [";
        feedback += result.severity[i];
        feedback += "] ";
        feedback += result.issues[i];
        feedback += "\n";
    }

    return feedback;
}

// Generate correction prompt for 7B
static std::string zeta_critic_correction_prompt(const char* original_prompt,
                                                   const char* response,
                                                   const zeta_critic_result_t& result) {
    if (!result.has_issues) return "";

    std::string prompt = "Review this response for correctness issues:\n\n";
    prompt += "ORIGINAL QUESTION: ";
    prompt += std::string(original_prompt).substr(0, 500);
    prompt += "\n\nRESPONSE EXCERPT: ";
    prompt += std::string(response).substr(0, 1000);
    prompt += "\n\nIDENTIFIED ISSUES:\n";

    for (int i = 0; i < result.issue_count; i++) {
        prompt += "- ";
        prompt += result.issues[i];
        prompt += "\n";
    }

    prompt += "\nProvide a brief correction or clarification for each issue.";

    return prompt;
}

// Log critic results
static void zeta_critic_log(const zeta_critic_result_t& result) {
    if (!result.has_issues) {
        fprintf(stderr, "[CRITIC] No issues detected\n");
        return;
    }

    fprintf(stderr, "[CRITIC] Found %d issues:\n", result.issue_count);
    for (int i = 0; i < result.issue_count; i++) {
        fprintf(stderr, "[CRITIC]   [%s] %s\n", result.severity[i], result.issues[i]);
    }
}

// Generate 7B critic prompt for deeper analysis
// This returns a prompt that 7B can process to verify the output
static std::string zeta_critic_7b_prompt(const char* original_prompt,
                                          const char* response,
                                          zeta_critic_domain_t domain) {
    std::string prompt = "VERIFY this response meets the requirements.\n\n";
    prompt += "REQUIREMENTS:\n";

    switch (domain) {
        case CRITIC_DOMAIN_ALGORITHMS:
            prompt += "- Check if code complexity matches claimed O() notation\n";
            prompt += "- Verify data structures support the claimed operations\n";
            prompt += "- Flag any operations that are worse than stated\n";
            break;
        case CRITIC_DOMAIN_HFT_TRADING:
            prompt += "- NO LOCKS in hot path (Mutex, RwLock, synchronized)\n";
            prompt += "- Must use lock-free or single-threaded design\n";
            prompt += "- Latency-critical code cannot block\n";
            break;
        case CRITIC_DOMAIN_DEBUGGING:
            prompt += "- Identify the SPECIFIC bug in the provided code\n";
            prompt += "- Check if callbacks/cleanup handlers fire correctly\n";
            prompt += "- Verify edge cases like network failures, timeouts\n";
            break;
        case CRITIC_DOMAIN_DISTRIBUTED_SYSTEMS:
            prompt += "- Check for split-brain scenarios\n";
            prompt += "- Verify consensus mechanism is sound\n";
            prompt += "- Check CAP theorem trade-offs are addressed\n";
            break;
        default:
            prompt += "- Verify response addresses the question\n";
            prompt += "- Check for logical consistency\n";
            break;
    }

    prompt += "\nQUESTION:\n";
    prompt += std::string(original_prompt).substr(0, 500);
    prompt += "\n\nRESPONSE EXCERPT:\n";
    prompt += std::string(response).substr(0, 800);
    prompt += "\n\nOUTPUT: List issues found or 'VERIFIED' if correct.";

    return prompt;
}

// Determine if we need 7B verification (beyond pattern matching)
static bool zeta_critic_needs_7b_review(const char* prompt,
                                         const zeta_critic_result_t& pattern_result) {
    // If pattern matching already found issues, still worth 7B review
    // for more context
    if (pattern_result.has_issues) return true;

    // Domain-specific prompts always benefit from 7B review
    zeta_critic_domain_t domain = zeta_critic_detect_domain(prompt);
    if (domain != CRITIC_DOMAIN_GENERAL) return true;

    return false;
}

#endif // ZETA_CRITIC_H
