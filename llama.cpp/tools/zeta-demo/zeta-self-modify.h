#pragma once

/**
 * Z.E.T.A. Autonomous Self-Modification System
 *
 * Allows Z.E.T.A. to:
 * - Read its own pending dreams
 * - Extract actionable C++ patches
 * - Apply patches to source files
 * - Compile and detect errors
 * - Auto-generate fixes for compilation errors
 * - Run autonomously in a self-improvement loop
 *
 * WARNING: This module enables recursive self-modification.
 * Run only in isolated branches.
 */

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <regex>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <functional>

// Forward declarations
struct zeta_dual_ctx_t;

namespace zeta_self_modify {

// ============================================================================
// Configuration
// ============================================================================

struct SelfModifyConfig {
    std::string source_dir = "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo";
    std::string build_dir = "/home/xx/ZetaZero/llama.cpp/build";
    std::string dreams_file = "/tmp/zeta_dreams.txt";
    std::string log_file = "/tmp/zeta_self_modify.log";

    int max_patches_per_cycle = 5;          // Max patches to apply per cycle
    int max_fix_attempts = 3;               // Max attempts to fix a compilation error
    int cycle_delay_seconds = 60;           // Delay between autonomous cycles
    bool dry_run = false;                   // If true, don't actually apply patches
    bool auto_commit = true;                // Auto-commit successful patches
    float min_confidence = 0.7f;            // Min confidence to apply a patch
};

static SelfModifyConfig g_self_modify_config;

// ============================================================================
// Patch Representation
// ============================================================================

enum class PatchType {
    INSERT,     // Insert new code
    REPLACE,    // Replace existing code
    DELETE,     // Delete code
    APPEND,     // Append to end of file
    PREPEND     // Prepend to beginning of file
};

struct CodePatch {
    std::string id;                     // Unique patch ID (from dream)
    std::string target_file;            // File to modify
    PatchType type;                     // Type of modification
    std::string search_pattern;         // Pattern to find (for REPLACE/DELETE)
    std::string new_code;               // New code to insert/replace
    std::string description;            // Human-readable description
    float confidence;                   // Confidence score (0.0-1.0)
    std::string dream_source;           // Original dream content

    // Metadata
    time_t created_at = 0;
    bool applied = false;
    bool compiled = false;
    std::string error_message;
};

struct PatchResult {
    bool success;
    std::string message;
    std::string diff;                   // Unified diff of changes
    std::vector<std::string> errors;    // Compilation errors if any
};

// ============================================================================
// Dream Parser - Extracts patches from dream content
// ============================================================================

class DreamPatchExtractor {
public:
    /**
     * Extract actionable patches from dream content
     */
    std::vector<CodePatch> extract(const std::string& dream_content, const std::string& dream_id) {
        std::vector<CodePatch> patches;

        // Look for code blocks with file annotations
        std::regex code_block_regex(
            R"(```(?:cpp|c\+\+)?\s*(?://\s*FILE:\s*(\S+))?\n([\s\S]*?)```)",
            std::regex::icase
        );

        std::sregex_iterator it(dream_content.begin(), dream_content.end(), code_block_regex);
        std::sregex_iterator end;

        int patch_num = 0;
        for (; it != end; ++it) {
            std::smatch match = *it;
            std::string file_hint = match[1].str();
            std::string code = match[2].str();

            if (code.empty()) continue;

            CodePatch patch;
            patch.id = dream_id + "_p" + std::to_string(patch_num++);
            patch.new_code = trim(code);
            patch.dream_source = dream_content;
            patch.created_at = time(nullptr);

            // Infer target file from hints or code content
            patch.target_file = infer_target_file(file_hint, code, dream_content);
            patch.type = infer_patch_type(code, dream_content);
            patch.confidence = calculate_confidence(code, dream_content);
            patch.description = extract_description(dream_content);

            // Extract search pattern if this is a replacement
            if (patch.type == PatchType::REPLACE) {
                patch.search_pattern = extract_search_pattern(dream_content, code);
            }

            if (!patch.target_file.empty() && patch.confidence >= g_self_modify_config.min_confidence) {
                patches.push_back(patch);
            }
        }

        // Also look for inline suggestions without code blocks
        if (patches.empty()) {
            auto inline_patches = extract_inline_suggestions(dream_content, dream_id);
            patches.insert(patches.end(), inline_patches.begin(), inline_patches.end());
        }

        return patches;
    }

private:
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    std::string infer_target_file(const std::string& hint, const std::string& code, const std::string& dream) {
        // First check explicit hint
        if (!hint.empty()) {
            return normalize_filename(hint);
        }

        // Look for file mentions in dream text
        std::regex file_mention(R"((zeta-\w+\.(?:h|cpp)))");
        std::smatch match;
        if (std::regex_search(dream, match, file_mention)) {
            return match[1].str();
        }

        // Infer from code content
        if (code.find("class ZetaHRM") != std::string::npos ||
            code.find("zeta_hrm") != std::string::npos) {
            return "zeta-hrm.h";
        }
        if (code.find("class ZetaTRM") != std::string::npos ||
            code.find("zeta_trm") != std::string::npos) {
            return "zeta-trm.h";
        }
        if (code.find("DreamState") != std::string::npos ||
            code.find("dream_") != std::string::npos) {
            return "zeta-dream.h";
        }
        if (code.find("EmbeddingCache") != std::string::npos ||
            code.find("zeta_embed") != std::string::npos) {
            return "zeta-embed-integration.h";
        }
        if (code.find("DynamicRouter") != std::string::npos ||
            code.find("route") != std::string::npos) {
            return "zeta-utils.h";
        }
        if (code.find("handle_") != std::string::npos ||
            code.find("endpoint") != std::string::npos) {
            return "zeta-server.cpp";
        }

        // Default to utils for new utilities
        return "zeta-utils.h";
    }

    std::string normalize_filename(const std::string& name) {
        std::string result = name;
        // Remove path if present
        size_t slash = result.rfind('/');
        if (slash != std::string::npos) {
            result = result.substr(slash + 1);
        }
        return result;
    }

    PatchType infer_patch_type(const std::string& code, const std::string& dream) {
        std::string lower_dream = dream;
        std::transform(lower_dream.begin(), lower_dream.end(), lower_dream.begin(), ::tolower);

        if (lower_dream.find("replace") != std::string::npos ||
            lower_dream.find("change") != std::string::npos ||
            lower_dream.find("modify") != std::string::npos) {
            return PatchType::REPLACE;
        }
        if (lower_dream.find("remove") != std::string::npos ||
            lower_dream.find("delete") != std::string::npos) {
            return PatchType::DELETE;
        }
        if (lower_dream.find("add to end") != std::string::npos ||
            lower_dream.find("append") != std::string::npos) {
            return PatchType::APPEND;
        }

        // Default to INSERT for new code
        return PatchType::INSERT;
    }

    float calculate_confidence(const std::string& code, const std::string& dream) {
        float confidence = 0.5f;

        // Boost for complete function/class definitions
        if (code.find("class ") != std::string::npos ||
            code.find("struct ") != std::string::npos) {
            confidence += 0.2f;
        }

        // Boost for proper C++ syntax
        if (code.find("{") != std::string::npos &&
            code.find("}") != std::string::npos) {
            confidence += 0.1f;
        }

        // Boost if dream mentions specific improvement
        std::string lower = dream;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("optimize") != std::string::npos ||
            lower.find("fix") != std::string::npos ||
            lower.find("improve") != std::string::npos) {
            confidence += 0.1f;
        }

        // Penalty for very short code
        if (code.length() < 50) {
            confidence -= 0.2f;
        }

        return std::min(1.0f, std::max(0.0f, confidence));
    }

    std::string extract_description(const std::string& dream) {
        // Take first sentence as description
        size_t period = dream.find('.');
        if (period != std::string::npos && period < 200) {
            return dream.substr(0, period + 1);
        }
        return dream.substr(0, std::min(dream.length(), size_t(100)));
    }

    std::string extract_search_pattern(const std::string& dream, const std::string& new_code) {
        // Look for "replace X with" patterns
        std::regex replace_pattern(R"(replace\s+[`"]?([^`"]+)[`"]?\s+with)", std::regex::icase);
        std::smatch match;
        if (std::regex_search(dream, match, replace_pattern)) {
            return match[1].str();
        }
        return "";
    }

    std::vector<CodePatch> extract_inline_suggestions(const std::string& dream, const std::string& dream_id) {
        std::vector<CodePatch> patches;

        // Look for function suggestions
        std::regex func_suggestion(
            R"(add\s+(?:a\s+)?(?:function|method)\s+(?:called\s+)?[`"]?(\w+)[`"]?)",
            std::regex::icase
        );

        std::smatch match;
        if (std::regex_search(dream, match, func_suggestion)) {
            // Generate a stub function
            CodePatch patch;
            patch.id = dream_id + "_inline";
            patch.type = PatchType::INSERT;
            patch.description = "Auto-generated function stub from dream suggestion";
            patch.confidence = 0.4f;  // Lower confidence for auto-generated
            patch.target_file = "zeta-utils.h";
            patch.new_code = "// TODO: Implement " + match[1].str() + " (from dream)\n";
            patch.dream_source = dream;
            patch.created_at = time(nullptr);

            patches.push_back(patch);
        }

        return patches;
    }
};

// ============================================================================
// File Patcher - Applies patches to source files
// ============================================================================

class FilePatcher {
public:
    FilePatcher(const std::string& source_dir) : source_dir_(source_dir) {}

    /**
     * Apply a patch to its target file
     */
    PatchResult apply(const CodePatch& patch) {
        PatchResult result;
        result.success = false;

        std::string filepath = source_dir_ + "/" + patch.target_file;

        // Read current file content
        std::ifstream file(filepath);
        if (!file.is_open()) {
            result.message = "Cannot open file: " + filepath;
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        std::string original = content;
        std::string modified;

        switch (patch.type) {
            case PatchType::INSERT:
                modified = apply_insert(content, patch);
                break;
            case PatchType::REPLACE:
                modified = apply_replace(content, patch);
                break;
            case PatchType::DELETE:
                modified = apply_delete(content, patch);
                break;
            case PatchType::APPEND:
                modified = content + "\n" + patch.new_code + "\n";
                break;
            case PatchType::PREPEND:
                modified = patch.new_code + "\n" + content;
                break;
        }

        if (modified == original) {
            result.message = "No changes made (pattern not found or already applied)";
            return result;
        }

        // Generate diff
        result.diff = generate_diff(original, modified, patch.target_file);

        // Write modified content
        if (!g_self_modify_config.dry_run) {
            std::ofstream out(filepath);
            if (!out.is_open()) {
                result.message = "Cannot write to file: " + filepath;
                return result;
            }
            out << modified;
            out.close();
        }

        result.success = true;
        result.message = "Patch applied successfully";
        return result;
    }

    /**
     * Revert a patch by restoring from backup
     */
    bool revert(const CodePatch& patch) {
        std::string filepath = source_dir_ + "/" + patch.target_file;
        std::string backup = filepath + ".bak";

        // Check if backup exists
        std::ifstream bak(backup);
        if (!bak.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << bak.rdbuf();
        bak.close();

        std::ofstream out(filepath);
        if (!out.is_open()) {
            return false;
        }
        out << buffer.str();
        out.close();

        return true;
    }

    /**
     * Create backup before patching
     */
    void backup(const std::string& filename) {
        std::string filepath = source_dir_ + "/" + filename;
        std::string backup_path = filepath + ".bak";

        std::ifstream src(filepath, std::ios::binary);
        std::ofstream dst(backup_path, std::ios::binary);
        dst << src.rdbuf();
    }

private:
    std::string source_dir_;

    std::string apply_insert(const std::string& content, const CodePatch& patch) {
        // Find a good insertion point
        // Look for end of last class/struct or before final #endif

        size_t insert_pos = content.rfind("#endif");
        if (insert_pos != std::string::npos) {
            // Insert before #endif
            return content.substr(0, insert_pos) +
                   "\n// === Auto-inserted by self-modify (dream: " + patch.id + ") ===\n" +
                   patch.new_code + "\n\n" +
                   content.substr(insert_pos);
        }

        // Otherwise append
        return content + "\n// === Auto-inserted by self-modify ===\n" + patch.new_code + "\n";
    }

    std::string apply_replace(const std::string& content, const CodePatch& patch) {
        if (patch.search_pattern.empty()) {
            return content;  // Can't replace without pattern
        }

        size_t pos = content.find(patch.search_pattern);
        if (pos == std::string::npos) {
            return content;  // Pattern not found
        }

        return content.substr(0, pos) + patch.new_code +
               content.substr(pos + patch.search_pattern.length());
    }

    std::string apply_delete(const std::string& content, const CodePatch& patch) {
        if (patch.search_pattern.empty()) {
            return content;
        }

        size_t pos = content.find(patch.search_pattern);
        if (pos == std::string::npos) {
            return content;
        }

        return content.substr(0, pos) + content.substr(pos + patch.search_pattern.length());
    }

    std::string generate_diff(const std::string& original, const std::string& modified, const std::string& filename) {
        // Simple line-by-line diff
        std::stringstream diff;
        diff << "--- a/" << filename << "\n";
        diff << "+++ b/" << filename << "\n";

        std::istringstream orig_stream(original);
        std::istringstream mod_stream(modified);
        std::string orig_line, mod_line;
        int line_num = 0;

        while (std::getline(orig_stream, orig_line) || std::getline(mod_stream, mod_line)) {
            line_num++;
            if (orig_line != mod_line) {
                diff << "@@ -" << line_num << " @@\n";
                if (!orig_line.empty()) diff << "-" << orig_line << "\n";
                if (!mod_line.empty()) diff << "+" << mod_line << "\n";
            }
            orig_line.clear();
            mod_line.clear();
        }

        return diff.str();
    }
};

// ============================================================================
// Compiler - Builds and captures errors
// ============================================================================

class CompilerInterface {
public:
    struct CompileResult {
        bool success;
        std::string output;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        float compile_time_seconds;
    };

    CompilerInterface(const std::string& build_dir) : build_dir_(build_dir) {}

    /**
     * Run cmake build and capture output
     */
    CompileResult build() {
        CompileResult result;
        auto start = std::chrono::steady_clock::now();

        // Run cmake build
        std::string cmd = "cd " + build_dir_ + " && cmake --build . --target zeta-server 2>&1";

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.output = "Failed to run build command";
            return result;
        }

        char buffer[256];
        std::stringstream output;
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output << buffer;
        }

        int status = pclose(pipe);
        result.success = (status == 0);
        result.output = output.str();

        auto end = std::chrono::steady_clock::now();
        result.compile_time_seconds = std::chrono::duration<float>(end - start).count();

        // Parse errors and warnings
        parse_compiler_output(result.output, result.errors, result.warnings);

        return result;
    }

    /**
     * Parse a single error message into components
     */
    struct ParsedError {
        std::string file;
        int line;
        int column;
        std::string type;       // "error" or "warning"
        std::string message;
        std::string context;    // Source line if available
    };

    ParsedError parse_error(const std::string& error_line) {
        ParsedError parsed;
        parsed.line = 0;
        parsed.column = 0;

        // GCC/Clang format: file:line:col: error: message
        std::regex error_regex(R"(([^:]+):(\d+):(\d+):\s*(error|warning):\s*(.+))");
        std::smatch match;

        if (std::regex_search(error_line, match, error_regex)) {
            parsed.file = match[1].str();
            parsed.line = std::stoi(match[2].str());
            parsed.column = std::stoi(match[3].str());
            parsed.type = match[4].str();
            parsed.message = match[5].str();
        }

        return parsed;
    }

private:
    std::string build_dir_;

    void parse_compiler_output(const std::string& output,
                               std::vector<std::string>& errors,
                               std::vector<std::string>& warnings) {
        std::istringstream stream(output);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.find("error:") != std::string::npos) {
                errors.push_back(line);
            } else if (line.find("warning:") != std::string::npos) {
                warnings.push_back(line);
            }
        }
    }
};

// ============================================================================
// Error Fixer - Generates fixes for compilation errors
// ============================================================================

class ErrorFixer {
public:
    /**
     * Generate a fix patch for a compilation error
     */
    CodePatch generate_fix(const CompilerInterface::ParsedError& error,
                          const std::string& source_content) {
        CodePatch fix;
        fix.id = "autofix_" + std::to_string(time(nullptr));
        fix.target_file = error.file;
        fix.confidence = 0.6f;
        fix.created_at = time(nullptr);

        // Analyze error type and generate appropriate fix
        if (error.message.find("undeclared identifier") != std::string::npos ||
            error.message.find("was not declared") != std::string::npos) {
            fix = fix_undeclared(error, source_content);
        }
        else if (error.message.find("expected") != std::string::npos) {
            fix = fix_syntax(error, source_content);
        }
        else if (error.message.find("undefined reference") != std::string::npos) {
            fix = fix_undefined_reference(error, source_content);
        }
        else if (error.message.find("cannot convert") != std::string::npos ||
                 error.message.find("no matching function") != std::string::npos) {
            fix = fix_type_mismatch(error, source_content);
        }
        else {
            // Generic fix attempt - comment out problematic line
            fix = fix_generic(error, source_content);
        }

        return fix;
    }

private:
    CodePatch fix_undeclared(const CompilerInterface::ParsedError& error,
                            const std::string& source_content) {
        CodePatch fix;
        fix.type = PatchType::INSERT;
        fix.description = "Auto-fix: Add missing declaration";
        fix.confidence = 0.5f;

        // Extract the undeclared identifier
        std::regex id_regex(R"('(\w+)')");
        std::smatch match;
        std::string identifier;
        if (std::regex_search(error.message, match, id_regex)) {
            identifier = match[1].str();
        }

        // Generate a placeholder declaration
        fix.new_code = "// TODO: Define " + identifier + " (auto-fix placeholder)\n";
        fix.new_code += "// static auto " + identifier + " = /* value needed */;\n";

        return fix;
    }

    CodePatch fix_syntax(const CompilerInterface::ParsedError& error,
                        const std::string& source_content) {
        CodePatch fix;
        fix.type = PatchType::REPLACE;
        fix.description = "Auto-fix: Syntax correction";
        fix.confidence = 0.4f;

        // Try to extract what's expected
        std::regex expected_regex(R"(expected\s+'([^']+)')");
        std::smatch match;
        if (std::regex_search(error.message, match, expected_regex)) {
            std::string expected = match[1].str();

            // Common fixes
            if (expected == ";") {
                fix.new_code = ";\n";
                fix.description = "Auto-fix: Add missing semicolon";
            } else if (expected == "}") {
                fix.new_code = "}\n";
                fix.description = "Auto-fix: Add missing closing brace";
            }
        }

        return fix;
    }

    CodePatch fix_undefined_reference(const CompilerInterface::ParsedError& error,
                                      const std::string& source_content) {
        CodePatch fix;
        fix.type = PatchType::INSERT;
        fix.description = "Auto-fix: Add missing function definition";
        fix.confidence = 0.3f;  // Low confidence - linker errors are complex

        // Extract function name
        std::regex func_regex(R"(undefined reference to `(\w+)')");
        std::smatch match;
        if (std::regex_search(error.message, match, func_regex)) {
            std::string func_name = match[1].str();
            fix.new_code = "// TODO: Implement " + func_name + " (linker error auto-fix)\n";
        }

        return fix;
    }

    CodePatch fix_type_mismatch(const CompilerInterface::ParsedError& error,
                               const std::string& source_content) {
        CodePatch fix;
        fix.type = PatchType::REPLACE;
        fix.description = "Auto-fix: Type conversion";
        fix.confidence = 0.3f;

        // Type mismatches are complex - just add a comment for now
        fix.new_code = "// TODO: Fix type mismatch - " + error.message + "\n";

        return fix;
    }

    CodePatch fix_generic(const CompilerInterface::ParsedError& error,
                         const std::string& source_content) {
        CodePatch fix;
        fix.type = PatchType::REPLACE;
        fix.description = "Auto-fix: Comment out problematic code";
        fix.confidence = 0.2f;

        // Extract the problematic line
        std::istringstream stream(source_content);
        std::string line;
        int current_line = 0;

        while (std::getline(stream, line) && current_line < error.line) {
            current_line++;
            if (current_line == error.line) {
                fix.search_pattern = line;
                fix.new_code = "// FIXME: " + error.message + "\n// " + line;
                break;
            }
        }

        return fix;
    }
};

// ============================================================================
// Self-Modify Orchestrator - Main autonomous loop
// ============================================================================

class SelfModifyOrchestrator {
public:
    using LogCallback = std::function<void(const std::string&)>;

    SelfModifyOrchestrator()
        : extractor_()
        , patcher_(g_self_modify_config.source_dir)
        , compiler_(g_self_modify_config.build_dir)
        , fixer_()
    {}

    void set_log_callback(LogCallback cb) { log_callback_ = cb; }

    /**
     * Run one cycle of autonomous self-modification
     */
    struct CycleResult {
        int patches_attempted;
        int patches_applied;
        int patches_compiled;
        int errors_fixed;
        std::vector<std::string> messages;
    };

    CycleResult run_cycle(const std::vector<std::string>& dreams) {
        CycleResult result = {0, 0, 0, 0, {}};

        log("=== Self-Modification Cycle Started ===");

        // Step 1: Extract patches from dreams
        std::vector<CodePatch> all_patches;
        for (size_t i = 0; i < dreams.size() && all_patches.size() < g_self_modify_config.max_patches_per_cycle; i++) {
            auto patches = extractor_.extract(dreams[i], "dream_" + std::to_string(i));
            all_patches.insert(all_patches.end(), patches.begin(), patches.end());
        }

        log("Extracted " + std::to_string(all_patches.size()) + " patches from " +
            std::to_string(dreams.size()) + " dreams");

        if (all_patches.empty()) {
            log("No actionable patches found");
            return result;
        }

        // Step 2: Sort by confidence
        std::sort(all_patches.begin(), all_patches.end(),
            [](const CodePatch& a, const CodePatch& b) {
                return a.confidence > b.confidence;
            });

        // Step 3: Apply patches one by one
        for (auto& patch : all_patches) {
            result.patches_attempted++;

            log("Applying patch " + patch.id + " to " + patch.target_file +
                " (confidence: " + std::to_string(patch.confidence) + ")");

            // Backup before patching
            patcher_.backup(patch.target_file);

            // Apply the patch
            auto patch_result = patcher_.apply(patch);

            if (!patch_result.success) {
                log("  SKIP: " + patch_result.message);
                continue;
            }

            result.patches_applied++;
            log("  Applied. Diff:\n" + patch_result.diff);

            // Step 4: Compile and check for errors
            auto compile_result = compiler_.build();

            if (compile_result.success) {
                result.patches_compiled++;
                log("  COMPILED OK (" + std::to_string(compile_result.compile_time_seconds) + "s)");

                // Auto-commit if enabled
                if (g_self_modify_config.auto_commit) {
                    git_commit(patch);
                }
            } else {
                log("  COMPILE FAILED: " + std::to_string(compile_result.errors.size()) + " errors");

                // Step 5: Try to fix compilation errors
                bool fixed = try_fix_errors(compile_result, result.errors_fixed);

                if (!fixed) {
                    log("  REVERTING patch due to unfixable errors");
                    patcher_.revert(patch);
                }
            }
        }

        log("=== Cycle Complete: " + std::to_string(result.patches_compiled) + "/" +
            std::to_string(result.patches_attempted) + " patches successful ===");

        return result;
    }

    /**
     * Run autonomous loop continuously
     */
    void run_autonomous(std::function<std::vector<std::string>()> dream_provider,
                       std::function<bool()> should_stop) {
        log("Starting autonomous self-modification loop");

        while (!should_stop()) {
            // Get latest dreams
            auto dreams = dream_provider();

            if (!dreams.empty()) {
                auto result = run_cycle(dreams);

                // Log stats
                log("Cycle stats: " + std::to_string(result.patches_compiled) + " compiled, " +
                    std::to_string(result.errors_fixed) + " auto-fixed");
            }

            // Wait before next cycle
            log("Sleeping for " + std::to_string(g_self_modify_config.cycle_delay_seconds) + " seconds...");
            sleep(g_self_modify_config.cycle_delay_seconds);
        }

        log("Autonomous loop stopped");
    }

private:
    DreamPatchExtractor extractor_;
    FilePatcher patcher_;
    CompilerInterface compiler_;
    ErrorFixer fixer_;
    LogCallback log_callback_;

    void log(const std::string& msg) {
        std::string timestamped = "[" + current_time() + "] " + msg;

        if (log_callback_) {
            log_callback_(timestamped);
        }

        // Also write to log file
        std::ofstream log_file(g_self_modify_config.log_file, std::ios::app);
        if (log_file.is_open()) {
            log_file << timestamped << "\n";
        }
    }

    std::string current_time() {
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return buf;
    }

    bool try_fix_errors(const CompilerInterface::CompileResult& compile_result, int& fix_count) {
        for (int attempt = 0; attempt < g_self_modify_config.max_fix_attempts; attempt++) {
            if (compile_result.errors.empty()) {
                return true;
            }

            log("  Fix attempt " + std::to_string(attempt + 1));

            // Try to fix first error
            auto parsed = compiler_.parse_error(compile_result.errors[0]);

            // Read source file
            std::ifstream src(g_self_modify_config.source_dir + "/" + parsed.file);
            if (!src.is_open()) continue;

            std::stringstream buffer;
            buffer << src.rdbuf();
            std::string source = buffer.str();
            src.close();

            // Generate fix
            auto fix = fixer_.generate_fix(parsed, source);

            if (fix.new_code.empty()) {
                log("  Could not generate fix for: " + parsed.message);
                continue;
            }

            // Apply fix
            auto fix_result = patcher_.apply(fix);
            if (!fix_result.success) continue;

            // Recompile
            auto recompile = compiler_.build();
            if (recompile.success) {
                fix_count++;
                log("  Fix successful!");
                return true;
            }
        }

        return false;
    }

    void git_commit(const CodePatch& patch) {
        std::string cmd = "cd " + g_self_modify_config.source_dir +
                         " && git add " + patch.target_file +
                         " && git commit -m 'Auto-patch: " + patch.description +
                         " (dream: " + patch.id + ")'";
        system(cmd.c_str());
    }
};

// ============================================================================
// Global instance
// ============================================================================

static SelfModifyOrchestrator g_self_modify;

// ============================================================================
// Integration Functions
// ============================================================================

/**
 * Initialize self-modification system
 */
inline bool zeta_self_modify_init(const std::string& source_dir = "",
                                  const std::string& build_dir = "") {
    if (!source_dir.empty()) {
        g_self_modify_config.source_dir = source_dir;
    }
    if (!build_dir.empty()) {
        g_self_modify_config.build_dir = build_dir;
    }

    // Set up logging
    g_self_modify.set_log_callback([](const std::string& msg) {
        printf("[SELF-MODIFY] %s\n", msg.c_str());
    });

    return true;
}

/**
 * Run one self-modification cycle with given dreams
 */
inline SelfModifyOrchestrator::CycleResult zeta_self_modify_cycle(
    const std::vector<std::string>& dreams) {
    return g_self_modify.run_cycle(dreams);
}

/**
 * Get configuration
 */
inline SelfModifyConfig& zeta_self_modify_config() {
    return g_self_modify_config;
}

/**
 * Extract patches from a dream (for testing/inspection)
 */
inline std::vector<CodePatch> zeta_extract_patches(const std::string& dream) {
    DreamPatchExtractor extractor;
    return extractor.extract(dream, "test_dream");
}

} // namespace zeta_self_modify
