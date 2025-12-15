#!/usr/bin/env python3
"""
Fix: Add causal extraction to Remember: path so it doesn't skip
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h'

with open(filepath, 'r') as f:
    content = f.read()

# Find the remember short-circuit and add causal extraction before return
old_remember = '''    // ===== REMEMBER SHORT-CIRCUIT =====
    // "Remember:" prefix stores EXACTLY what follows
    if (strncasecmp(text, "remember:", 9) == 0) {
        const char* content = text + 9;
        while (*content == ' ') content++;  // Skip spaces

        if (strlen(content) > 5) {
            // Store as raw_memory with high salience
            zeta_create_node(ctx, NODE_FACT, "raw_memory", content, 0.95f);
            facts_created++;
            fprintf(stderr, "[REMEMBER] Direct storage: %.60s...\\n", content);
            return facts_created;  // Skip 3B extraction
        }
    }'''

new_remember = '''    // ===== REMEMBER SHORT-CIRCUIT =====
    // "Remember:" prefix stores EXACTLY what follows, but also extract causal relations
    if (strncasecmp(text, "remember:", 9) == 0) {
        const char* content = text + 9;
        while (*content == ' ') content++;  // Skip spaces

        if (strlen(content) > 5) {
            // Store as raw_memory with high salience
            zeta_create_node(ctx, NODE_FACT, "raw_memory", content, 0.95f);
            facts_created++;
            fprintf(stderr, "[REMEMBER] Direct storage: %.60s...\\n", content);

            // Also extract causal relations from the content
            char lower_content[2048];
            int clen = strlen(content);
            if (clen > 2047) clen = 2047;
            for (int i = 0; i < clen; i++) lower_content[i] = tolower(content[i]);
            lower_content[clen] = '\\0';

            // Check for CAUSES patterns
            const char* causes_patterns[] = {" wakes ", " eats ", " causes ", " triggers ", " destroys ", " kills ", NULL};
            for (int cp = 0; causes_patterns[cp]; cp++) {
                const char* cmatch = strstr(lower_content, causes_patterns[cp]);
                if (cmatch) {
                    // Extract subject (word before pattern)
                    char subj[128] = {0};
                    const char* s = cmatch - 1;
                    while (s > lower_content && *s == ' ') s--;
                    const char* word_end = s + 1;
                    while (s > lower_content && *s != ' ' && *s != '.' && *s != ',') s--;
                    if (*s == ' ' || *s == '.' || *s == ',') s++;
                    int si = 0;
                    while (s < word_end && si < 127) subj[si++] = *s++;
                    subj[si] = '\\0';

                    // Extract object (words after pattern)
                    char obj[128] = {0};
                    const char* o = cmatch + strlen(causes_patterns[cp]);
                    int oi = 0;
                    while (*o && oi < 127 && *o != '.' && *o != ',') obj[oi++] = *o++;
                    while (oi > 0 && obj[oi-1] == ' ') oi--;
                    obj[oi] = '\\0';

                    if (strlen(subj) > 1 && strlen(obj) > 1) {
                        int64_t subj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_agent", subj, 0.9f);
                        int64_t obj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_target", obj, 0.9f);
                        zeta_create_edge(ctx, subj_id, obj_id, EDGE_CAUSES, 1.0f);
                        facts_created++;
                        fprintf(stderr, "[CAUSAL] %s --CAUSES--> %s\\n", subj, obj);
                    }
                }
            }

            // Check for PREVENTS patterns
            const char* prevents_patterns[] = {" slayed ", " killed ", " destroyed ", " prevents ", " stops ", " before it could ", NULL};
            for (int pp = 0; prevents_patterns[pp]; pp++) {
                const char* pmatch = strstr(lower_content, prevents_patterns[pp]);
                if (pmatch) {
                    char subj[128] = {0};
                    const char* s = pmatch - 1;
                    while (s > lower_content && *s == ' ') s--;
                    const char* word_end = s + 1;
                    while (s > lower_content && *s != ' ' && *s != '.' && *s != ',') s--;
                    if (*s == ' ' || *s == '.' || *s == ',') s++;
                    int si = 0;
                    while (s < word_end && si < 127) subj[si++] = *s++;
                    subj[si] = '\\0';

                    char obj[128] = {0};
                    const char* o = pmatch + strlen(prevents_patterns[pp]);
                    int oi = 0;
                    while (*o && oi < 127 && *o != '.' && *o != ',') obj[oi++] = *o++;
                    while (oi > 0 && obj[oi-1] == ' ') oi--;
                    obj[oi] = '\\0';

                    if (strlen(subj) > 1 && strlen(obj) > 1) {
                        int64_t subj_id = zeta_create_node(ctx, NODE_ENTITY, "preventer", subj, 0.95f);
                        int64_t obj_id = zeta_create_node(ctx, NODE_ENTITY, "prevented", obj, 0.9f);
                        zeta_create_edge(ctx, subj_id, obj_id, EDGE_PREVENTS, 1.0f);
                        facts_created++;
                        fprintf(stderr, "[PREVENTS] %s --PREVENTS--> %s\\n", subj, obj);
                    }
                }
            }

            return facts_created;  // Skip full 3B extraction but kept causal edges
        }
    }'''

if old_remember in content:
    content = content.replace(old_remember, new_remember)
    print('Added causal extraction to Remember: path')
else:
    print('ERROR: Could not find Remember: section')
    # Try to see what's there
    if 'REMEMBER SHORT-CIRCUIT' in content:
        print('Found marker but pattern mismatch')

with open(filepath, 'w') as f:
    f.write(content)

print('\\nPatch complete!')
