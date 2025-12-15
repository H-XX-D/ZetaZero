#!/usr/bin/env python3
"""
Fix: Add causal extraction to Remember: path - using exact whitespace
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h'

with open(filepath, 'r') as f:
    content = f.read()

# Find and replace just the return statement within Remember section
old_return = '''            fprintf(stderr, "[REMEMBER] Direct storage: %.60s...\\n", content);
            return facts_created;  // Skip 3B extraction'''

new_return = '''            fprintf(stderr, "[REMEMBER] Direct storage: %.60s...\\n", content);

            // Also extract causal relations from the content
            char lower_content[2048];
            int clen = strlen(content);
            if (clen > 2047) clen = 2047;
            for (int i = 0; i < clen; i++) lower_content[i] = tolower(content[i]);
            lower_content[clen] = '\\0';

            // Check for CAUSES patterns
            const char* causes_pats[] = {" wakes ", " eats ", " causes ", " triggers ", " destroys ", " kills ", NULL};
            for (int cp = 0; causes_pats[cp]; cp++) {
                const char* cmatch = strstr(lower_content, causes_pats[cp]);
                if (cmatch) {
                    char subj[128] = {0};
                    const char* s = cmatch - 1;
                    while (s > lower_content && *s == ' ') s--;
                    const char* word_end = s + 1;
                    while (s > lower_content && *s != ' ' && *s != '.' && *s != ',') s--;
                    if (*s == ' ' || *s == '.' || *s == ',') s++;
                    int si = 0;
                    while (s < word_end && si < 127) subj[si++] = *s++;
                    subj[si] = '\\0';

                    char obj[128] = {0};
                    const char* o = cmatch + strlen(causes_pats[cp]);
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
            const char* prevents_pats[] = {" slayed ", " killed ", " destroyed ", " prevents ", " stops ", " before it could ", NULL};
            for (int pp = 0; prevents_pats[pp]; pp++) {
                const char* pmatch = strstr(lower_content, prevents_pats[pp]);
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
                    const char* o = pmatch + strlen(prevents_pats[pp]);
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

            return facts_created;  // Skip full 3B extraction but kept causal edges'''

if old_return in content:
    content = content.replace(old_return, new_return)
    print('Added causal extraction to Remember: path')
else:
    print('ERROR: Could not find return statement in Remember section')

with open(filepath, 'w') as f:
    f.write(content)

print('\\nPatch complete!')
