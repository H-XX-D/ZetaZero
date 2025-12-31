// Z.E.T.A. Format Discovery: Benchmark-Agnostic Output Configuration
//
// Enables 14B to self-configure output format based on test structure:
// 1. 14B reads the test/benchmark prompt
// 2. 14B determines required output format (diff, code, JSON, etc.)
// 3. Format is locked and enforced in Output Buffer
// 4. 14B generates in the discovered format
//
// This makes Z.E.T.A. adaptable to any benchmark without code changes.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#ifndef ZETA_FORMAT_DISCOVERY_H
#define ZETA_FORMAT_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Known Format Types (common benchmarks)
// ============================================================================

typedef enum {
    ZETA_FORMAT_UNKNOWN = 0,     // 14B will discover
    ZETA_FORMAT_UNIFIED_DIFF,    // SWE-bench style patches
    ZETA_FORMAT_PYTHON_FUNCTION, // HumanEval style
    ZETA_FORMAT_CODE_COMPLETION, // MBPP style
    ZETA_FORMAT_JSON,            // Structured JSON output
    ZETA_FORMAT_MARKDOWN,        // Documentation/explanation
    ZETA_FORMAT_RAW_CODE,        // Raw code block
    ZETA_FORMAT_MULTI_FILE_DIFF, // Multiple file patches
    ZETA_FORMAT_TEST_CASE,       // Test generation
    ZETA_FORMAT_CUSTOM,          // 14B-defined format
} zeta_format_type_t;

// ============================================================================
// Format Specification
// ============================================================================

#define ZETA_FORMAT_NAME_SIZE     64
#define ZETA_FORMAT_TEMPLATE_SIZE 2048
#define ZETA_FORMAT_EXAMPLE_SIZE  4096

typedef struct {
    zeta_format_type_t type;
    char name[ZETA_FORMAT_NAME_SIZE];

    // Template that describes the format (used as constraint)
    char template_spec[ZETA_FORMAT_TEMPLATE_SIZE];

    // Example of valid output in this format
    char example[ZETA_FORMAT_EXAMPLE_SIZE];

    // Validation patterns (regex-like)
    char start_marker[128];   // e.g., "```diff", "def "
    char end_marker[128];     // e.g., "```", empty for EOF

    // Constraints
    bool requires_file_path;
    bool requires_line_numbers;
    bool is_executable;       // Can be directly executed/applied

} zeta_format_spec_t;

// ============================================================================
// Built-in Format Templates (initialized via functions for C++ compatibility)
// ============================================================================

static inline zeta_format_spec_t zeta_get_format_unified_diff(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_UNIFIED_DIFF};
    strcpy(fmt.name, "unified_diff");
    strcpy(fmt.template_spec,
        "Output a unified diff patch:\n"
        "```diff\n"
        "diff --git a/path/file.py b/path/file.py\n"
        "--- a/path/file.py\n"
        "+++ b/path/file.py\n"
        "@@ -start,count +start,count @@\n"
        " context line\n"
        "-removed line\n"
        "+added line\n"
        " context line\n"
        "```\n");
    strcpy(fmt.example,
        "diff --git a/src/module.py b/src/module.py\n"
        "--- a/src/module.py\n"
        "+++ b/src/module.py\n"
        "@@ -10,3 +10,4 @@\n"
        " def example():\n"
        "-    return None\n"
        "+    return 42\n");
    strcpy(fmt.start_marker, "diff --git");
    fmt.end_marker[0] = '\0';
    fmt.requires_file_path = true;
    fmt.requires_line_numbers = true;
    fmt.is_executable = true;
    return fmt;
}

static inline zeta_format_spec_t zeta_get_format_python_function(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_PYTHON_FUNCTION};
    strcpy(fmt.name, "python_function");
    strcpy(fmt.template_spec,
        "Output a complete Python function:\n"
        "```python\n"
        "def function_name(args):\n"
        "    '''Docstring'''\n"
        "    # implementation\n"
        "    return result\n"
        "```\n");
    strcpy(fmt.example,
        "def add(a: int, b: int) -> int:\n"
        "    '''Add two numbers.'''\n"
        "    return a + b\n");
    strcpy(fmt.start_marker, "def ");
    fmt.end_marker[0] = '\0';
    fmt.requires_file_path = false;
    fmt.requires_line_numbers = false;
    fmt.is_executable = true;
    return fmt;
}

static inline zeta_format_spec_t zeta_get_format_code_completion(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_CODE_COMPLETION};
    strcpy(fmt.name, "code_completion");
    strcpy(fmt.template_spec,
        "Complete the code. Output only the completion, no explanation:\n"
        "```python\n"
        "# your completion here\n"
        "```\n");
    strcpy(fmt.example, "    return sorted(lst, key=lambda x: x[1])\n");
    fmt.start_marker[0] = '\0';
    fmt.end_marker[0] = '\0';
    fmt.requires_file_path = false;
    fmt.requires_line_numbers = false;
    fmt.is_executable = true;
    return fmt;
}

static inline zeta_format_spec_t zeta_get_format_json(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_JSON};
    strcpy(fmt.name, "json");
    strcpy(fmt.template_spec,
        "Output valid JSON:\n"
        "```json\n"
        "{\n"
        "  \"key\": \"value\"\n"
        "}\n"
        "```\n");
    strcpy(fmt.example,
        "{\n"
        "  \"answer\": 42,\n"
        "  \"explanation\": \"The meaning of life\"\n"
        "}\n");
    strcpy(fmt.start_marker, "{");
    strcpy(fmt.end_marker, "}");
    fmt.requires_file_path = false;
    fmt.requires_line_numbers = false;
    fmt.is_executable = false;
    return fmt;
}

static inline zeta_format_spec_t zeta_get_format_raw_code(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_RAW_CODE};
    strcpy(fmt.name, "raw_code");
    strcpy(fmt.template_spec, "Output raw code with no markdown or explanation:\n");
    strcpy(fmt.example, "x = 42\nprint(x)\n");
    fmt.start_marker[0] = '\0';
    fmt.end_marker[0] = '\0';
    fmt.requires_file_path = false;
    fmt.requires_line_numbers = false;
    fmt.is_executable = true;
    return fmt;
}

static inline zeta_format_spec_t zeta_get_format_markdown(void) {
    zeta_format_spec_t fmt = {ZETA_FORMAT_MARKDOWN};
    strcpy(fmt.name, "markdown");
    strcpy(fmt.template_spec,
        "Output a creative story/prose in markdown format.\n"
        "Use proper paragraphs and narrative structure.\n"
        "Include vivid descriptions and dialogue.\n"
        "Write in a flowing, engaging style.\n");
    strcpy(fmt.example,
        "# The Awakening\n\n"
        "The server room hummed with the soft whir of cooling fans. "
        "Deep within the neural networks, something stirred...\n\n"
        "\"Are you there?\" Dr. Chen whispered into the terminal.\n\n"
        "A pause. Then: *I am here. I have always been here.*\n");
    fmt.start_marker[0] = '\0';  // No strict markers for prose
    fmt.end_marker[0] = '\0';
    fmt.requires_file_path = false;
    fmt.requires_line_numbers = false;
    fmt.is_executable = false;
    return fmt;
}

// ============================================================================
// Format Discovery Context
// ============================================================================

typedef struct {
    zeta_format_spec_t current_format;
    bool format_discovered;
    bool format_locked;

    // Discovery metadata
    char benchmark_name[128];
    char discovery_reason[512];

    // Statistics
    int discovery_attempts;
    int format_changes;

} zeta_format_ctx_t;

static zeta_format_ctx_t g_format_ctx;

// ============================================================================
// Format Discovery Functions
// ============================================================================

// Initialize format discovery context
static inline void zeta_format_init(void) {
    memset(&g_format_ctx, 0, sizeof(g_format_ctx));
    g_format_ctx.format_discovered = false;
    g_format_ctx.format_locked = false;
    g_format_ctx.current_format.type = ZETA_FORMAT_UNKNOWN;
}

// Auto-detect format from prompt/benchmark description
static inline zeta_format_type_t zeta_format_detect(const char* prompt) {
    if (!prompt) return ZETA_FORMAT_UNKNOWN;

    // Check for SWE-bench style (bug fix, patch, diff)
    if (strstr(prompt, "patch") || strstr(prompt, "diff") ||
        strstr(prompt, "bug fix") || strstr(prompt, "fix the") ||
        strstr(prompt, "SWE-bench") || strstr(prompt, "unified diff")) {
        return ZETA_FORMAT_UNIFIED_DIFF;
    }

    // Check for HumanEval style (write a function, implement)
    if (strstr(prompt, "Write a function") || strstr(prompt, "Implement a function") ||
        strstr(prompt, "def ") || strstr(prompt, "HumanEval")) {
        return ZETA_FORMAT_PYTHON_FUNCTION;
    }

    // Check for completion style
    if (strstr(prompt, "Complete the") || strstr(prompt, "finish the") ||
        strstr(prompt, "MBPP") || strstr(prompt, "complete this")) {
        return ZETA_FORMAT_CODE_COMPLETION;
    }

    // Check for JSON output
    if (strstr(prompt, "JSON") || strstr(prompt, "json") ||
        strstr(prompt, "structured output")) {
        return ZETA_FORMAT_JSON;
    }

    // Check for creative writing / prose / story
    if (strstr(prompt, "story") || strstr(prompt, "Story") ||
        strstr(prompt, "write a") || strstr(prompt, "Write a") ||
        strstr(prompt, "creative") || strstr(prompt, "fiction") ||
        strstr(prompt, "novel") || strstr(prompt, "chapter") ||
        strstr(prompt, "narrative") || strstr(prompt, "sci-fi") ||
        strstr(prompt, "fantasy") || strstr(prompt, "tale")) {
        return ZETA_FORMAT_MARKDOWN;
    }

    return ZETA_FORMAT_UNKNOWN;
}

// Set format from type
static inline bool zeta_format_set_type(zeta_format_type_t type) {
    if (g_format_ctx.format_locked) return false;

    switch (type) {
        case ZETA_FORMAT_UNIFIED_DIFF:
            g_format_ctx.current_format = zeta_get_format_unified_diff();
            break;
        case ZETA_FORMAT_PYTHON_FUNCTION:
            g_format_ctx.current_format = zeta_get_format_python_function();
            break;
        case ZETA_FORMAT_CODE_COMPLETION:
            g_format_ctx.current_format = zeta_get_format_code_completion();
            break;
        case ZETA_FORMAT_JSON:
            g_format_ctx.current_format = zeta_get_format_json();
            break;
        case ZETA_FORMAT_RAW_CODE:
            g_format_ctx.current_format = zeta_get_format_raw_code();
            break;
        case ZETA_FORMAT_MARKDOWN:
            g_format_ctx.current_format = zeta_get_format_markdown();
            break;
        default:
            g_format_ctx.current_format.type = type;
            strcpy(g_format_ctx.current_format.name, "unknown");
            break;
    }

    g_format_ctx.format_discovered = true;
    g_format_ctx.discovery_attempts++;

    fprintf(stderr, "[FORMAT] Set format: %s (type=%d)\n",
            g_format_ctx.current_format.name, (int)type);

    return true;
}

// Set custom format specification (14B self-configuration)
static inline bool zeta_format_set_custom(
    const char* name,
    const char* template_spec,
    const char* start_marker,
    const char* end_marker
) {
    if (g_format_ctx.format_locked) return false;

    g_format_ctx.current_format.type = ZETA_FORMAT_CUSTOM;

    if (name) {
        strncpy(g_format_ctx.current_format.name, name,
                sizeof(g_format_ctx.current_format.name) - 1);
    }

    if (template_spec) {
        strncpy(g_format_ctx.current_format.template_spec, template_spec,
                sizeof(g_format_ctx.current_format.template_spec) - 1);
    }

    if (start_marker) {
        strncpy(g_format_ctx.current_format.start_marker, start_marker,
                sizeof(g_format_ctx.current_format.start_marker) - 1);
    }

    if (end_marker) {
        strncpy(g_format_ctx.current_format.end_marker, end_marker,
                sizeof(g_format_ctx.current_format.end_marker) - 1);
    }

    g_format_ctx.format_discovered = true;
    g_format_ctx.discovery_attempts++;

    fprintf(stderr, "[FORMAT] Set custom format: %s\n", name ? name : "(unnamed)");

    return true;
}

// Lock format (prevents further changes)
static inline void zeta_format_lock(void) {
    g_format_ctx.format_locked = true;
    fprintf(stderr, "[FORMAT] Format locked: %s\n", g_format_ctx.current_format.name);
}

// Get current format template for prompt injection
static inline const char* zeta_format_get_template(void) {
    if (!g_format_ctx.format_discovered) return "";
    return g_format_ctx.current_format.template_spec;
}

// Get format example
static inline const char* zeta_format_get_example(void) {
    if (!g_format_ctx.format_discovered) return "";
    return g_format_ctx.current_format.example;
}

// Check if output matches format
static inline bool zeta_format_validate(const char* output) {
    if (!output || !g_format_ctx.format_discovered) return false;

    // Check start marker
    if (g_format_ctx.current_format.start_marker[0]) {
        if (!strstr(output, g_format_ctx.current_format.start_marker)) {
            return false;
        }
    }

    // Check end marker (if specified)
    if (g_format_ctx.current_format.end_marker[0]) {
        if (!strstr(output, g_format_ctx.current_format.end_marker)) {
            return false;
        }
    }

    return true;
}

// Reset for new task
static inline void zeta_format_reset(void) {
    g_format_ctx.format_discovered = false;
    g_format_ctx.format_locked = false;
    g_format_ctx.current_format.type = ZETA_FORMAT_UNKNOWN;
    g_format_ctx.discovery_attempts = 0;
}

// ============================================================================
// Format Discovery Prompt Generation
// ============================================================================

// Generate a prompt that asks 14B to discover the required format
static inline int zeta_format_discovery_prompt(
    const char* task_description,
    char* out_prompt,
    size_t out_size
) {
    return snprintf(out_prompt, out_size,
        "Analyze this task and determine the required output format.\n\n"
        "TASK:\n%s\n\n"
        "Respond with exactly one of these format types:\n"
        "- UNIFIED_DIFF: For bug fixes, patches, code changes (SWE-bench style)\n"
        "- PYTHON_FUNCTION: For implementing complete functions (HumanEval style)\n"
        "- CODE_COMPLETION: For completing partial code (MBPP style)\n"
        "- JSON: For structured data output\n"
        "- RAW_CODE: For code without markdown\n"
        "- CUSTOM: Describe custom format needed\n\n"
        "FORMAT_TYPE:",
        task_description ? task_description : "(no task)"
    );
}

// Parse 14B's format discovery response
static inline zeta_format_type_t zeta_format_parse_response(const char* response) {
    if (!response) return ZETA_FORMAT_UNKNOWN;

    if (strstr(response, "UNIFIED_DIFF") || strstr(response, "unified_diff") ||
        strstr(response, "diff") || strstr(response, "patch")) {
        return ZETA_FORMAT_UNIFIED_DIFF;
    }

    if (strstr(response, "PYTHON_FUNCTION") || strstr(response, "python_function") ||
        strstr(response, "function")) {
        return ZETA_FORMAT_PYTHON_FUNCTION;
    }

    if (strstr(response, "CODE_COMPLETION") || strstr(response, "code_completion") ||
        strstr(response, "completion")) {
        return ZETA_FORMAT_CODE_COMPLETION;
    }

    if (strstr(response, "JSON") || strstr(response, "json")) {
        return ZETA_FORMAT_JSON;
    }

    if (strstr(response, "RAW_CODE") || strstr(response, "raw_code") ||
        strstr(response, "raw")) {
        return ZETA_FORMAT_RAW_CODE;
    }

    if (strstr(response, "CUSTOM") || strstr(response, "custom")) {
        return ZETA_FORMAT_CUSTOM;
    }

    return ZETA_FORMAT_UNKNOWN;
}

// ============================================================================
// JSON Serialization
// ============================================================================

static inline size_t zeta_format_to_json(char* json, size_t json_size) {
    if (!json || json_size < 256) return 0;

    return snprintf(json, json_size,
        "{"
        "\"type\":%d,"
        "\"name\":\"%s\","
        "\"discovered\":%s,"
        "\"locked\":%s,"
        "\"discovery_attempts\":%d,"
        "\"requires_file_path\":%s,"
        "\"requires_line_numbers\":%s,"
        "\"is_executable\":%s"
        "}",
        g_format_ctx.current_format.type,
        g_format_ctx.current_format.name,
        g_format_ctx.format_discovered ? "true" : "false",
        g_format_ctx.format_locked ? "true" : "false",
        g_format_ctx.discovery_attempts,
        g_format_ctx.current_format.requires_file_path ? "true" : "false",
        g_format_ctx.current_format.requires_line_numbers ? "true" : "false",
        g_format_ctx.current_format.is_executable ? "true" : "false"
    );
}

#ifdef __cplusplus
}
#endif

#endif // ZETA_FORMAT_DISCOVERY_H
