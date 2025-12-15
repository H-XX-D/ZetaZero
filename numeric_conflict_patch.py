#!/usr/bin/env python3
"""Add explicit numeric pattern matching for conflict detection"""

import sys

filepath = sys.argv[1] if len(sys.argv) > 1 else "/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-conflict.h"

with open(filepath, 'r') as f:
    content = f.read()

# Check if already patched
if 'zeta_extract_numeric_facts' in content:
    print("Already has numeric extraction!")
    sys.exit(0)

# Find the end of the file (before final endif)
insert_marker = '#endif // ZETA_CONFLICT_H'

numeric_extraction_code = '''
// ============================================================================
// NUMERIC FACT EXTRACTION AND CONFLICT DETECTION
// Pattern-based extraction for hard numeric data
// ============================================================================

typedef struct {
    char label[64];
    char value[32];
    int64_t numeric;
} zeta_numeric_fact_t;

// Extract numeric facts from text using pattern matching
static inline int zeta_extract_numeric_facts(
    const char* text,
    zeta_numeric_fact_t* facts,
    int max_facts
) {
    int count = 0;
    char lower[1024];
    
    // Convert to lowercase for matching
    size_t i = 0;
    while (text[i] && i < sizeof(lower) - 1) {
        lower[i] = tolower((unsigned char)text[i]);
        i++;
    }
    lower[i] = '\\0';
    
    // Pattern: "born in YYYY" or "was born in YYYY"
    const char* born_patterns[] = {"born in ", "was born ", "birth year is ", "birthday is ", NULL};
    for (int p = 0; born_patterns[p] && count < max_facts; p++) {
        const char* match = strstr(lower, born_patterns[p]);
        if (match) {
            const char* num_start = match + strlen(born_patterns[p]);
            // Find 4-digit year
            while (*num_start && !isdigit(*num_start)) num_start++;
            if (isdigit(num_start[0]) && isdigit(num_start[1]) && 
                isdigit(num_start[2]) && isdigit(num_start[3])) {
                strncpy(facts[count].label, "birth_year", sizeof(facts[count].label));
                snprintf(facts[count].value, sizeof(facts[count].value), "%.4s", num_start);
                facts[count].numeric = atol(facts[count].value);
                count++;
            }
        }
    }
    
    // Pattern: "age is N" or "I am N years old"
    const char* age_patterns[] = {"age is ", "i am ", "i\\'m ", NULL};
    for (int p = 0; age_patterns[p] && count < max_facts; p++) {
        const char* match = strstr(lower, age_patterns[p]);
        if (match) {
            const char* num_start = match + strlen(age_patterns[p]);
            while (*num_start && !isdigit(*num_start)) num_start++;
            if (isdigit(*num_start)) {
                // Check for "years old" nearby
                if (strstr(num_start, "years") || strstr(num_start, "year") || 
                    strcmp(age_patterns[p], "age is ") == 0) {
                    strncpy(facts[count].label, "age", sizeof(facts[count].label));
                    snprintf(facts[count].value, sizeof(facts[count].value), "%d", atoi(num_start));
                    facts[count].numeric = atol(facts[count].value);
                    count++;
                }
            }
        }
    }
    
    // Pattern: "salary is $N" or "make $N" or "earn $N"
    const char* salary_patterns[] = {"salary is ", "make $", "earn $", "paid $", NULL};
    for (int p = 0; salary_patterns[p] && count < max_facts; p++) {
        const char* match = strstr(lower, salary_patterns[p]);
        if (match) {
            const char* num_start = match + strlen(salary_patterns[p]);
            while (*num_start && (*num_start == '$' || *num_start == ' ')) num_start++;
            if (isdigit(*num_start)) {
                strncpy(facts[count].label, "salary", sizeof(facts[count].label));
                // Parse number (handle commas)
                char num_buf[32];
                int j = 0;
                while (*num_start && (isdigit(*num_start) || *num_start == ',') && j < 31) {
                    if (*num_start != ',') num_buf[j++] = *num_start;
                    num_start++;
                }
                num_buf[j] = '\\0';
                strncpy(facts[count].value, num_buf, sizeof(facts[count].value));
                facts[count].numeric = atol(num_buf);
                count++;
            }
        }
    }
    
    return count;
}

// Check for numeric conflicts between input and graph
static inline int zeta_check_numeric_conflicts(
    zeta_dual_ctx_t* ctx,
    const char* input,
    char* warning_buffer,
    int buffer_size
) {
    zeta_numeric_fact_t new_facts[8];
    int new_count = zeta_extract_numeric_facts(input, new_facts, 8);
    
    if (new_count == 0) return 0;  // No numeric facts in input
    
    int conflicts_found = 0;
    char* p = warning_buffer;
    int remaining = buffer_size - 1;
    
    for (int i = 0; i < new_count; i++) {
        // Search graph for existing node with same label
        for (int j = 0; j < ctx->num_nodes; j++) {
            zeta_graph_node_t* node = &ctx->nodes[j];
            if (!node->is_active) continue;
            
            // Check if label matches
            if (strcasecmp(node->label, new_facts[i].label) == 0) {
                // Extract numeric from existing node value
                int64_t existing_num = 0;
                const char* np = node->value;
                while (*np && !isdigit(*np)) np++;
                if (isdigit(*np)) {
                    existing_num = atol(np);
                }
                
                // Check for conflict
                if (existing_num != 0 && existing_num != new_facts[i].numeric) {
                    fprintf(stderr, "[NUMERIC_CONFLICT] %s: stored=%lld vs input=%lld\\n",
                            new_facts[i].label, (long long)existing_num, (long long)new_facts[i].numeric);
                    
                    int n = snprintf(p, remaining, 
                        "[MEMORY CONFLICT: You previously stated %s was %lld, but now claim %lld. "
                        "Please clarify which is correct.]\\n",
                        new_facts[i].label, (long long)existing_num, (long long)new_facts[i].numeric);
                    p += n;
                    remaining -= n;
                    conflicts_found++;
                }
            }
        }
    }
    
    return conflicts_found;
}

'''

if insert_marker in content:
    content = content.replace(insert_marker, numeric_extraction_code + "\n" + insert_marker)
    print("Added numeric extraction and conflict detection")
else:
    print("Could not find insertion point")
    sys.exit(1)

with open(filepath, 'w') as f:
    f.write(content)

print("Done!")
