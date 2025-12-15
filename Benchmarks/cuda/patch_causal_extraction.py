#!/usr/bin/env python3
"""
Add causal relationship extraction to 3B extraction
Detect patterns like "X causes Y", "X wakes Y", "X eats Y" etc.
Create EDGE_CAUSES and EDGE_PREVENTS edges
"""

filepath = '/home/xx/ZetaZero/llama.cpp/tools/zeta-demo/zeta-dual-process.h'

with open(filepath, 'r') as f:
    content = f.read()

# Add causal extraction before the function returns
if 'CAUSAL patterns' not in content:
    old_marker = '''    // Update patterns (for versioning)
    if (strstr(lower, "changed to ") || strstr(lower, "actually ") ||
        strstr(lower, "now it's ") || strstr(lower, "updated to ")) {
        // TODO: Extract old and new values for versioning
        fprintf(stderr, "[3B] Version update detected (TODO: implement)\\n");
    }

    return facts_created;
}'''

    new_marker = '''    // Update patterns (for versioning)
    if (strstr(lower, "changed to ") || strstr(lower, "actually ") ||
        strstr(lower, "now it's ") || strstr(lower, "updated to ")) {
        // TODO: Extract old and new values for versioning
        fprintf(stderr, "[3B] Version update detected (TODO: implement)\\n");
    }

    // ============================================================
    // CAUSAL patterns - detect "X causes Y", "X prevents Y" etc.
    // Creates EDGE_CAUSES and EDGE_PREVENTS for causal reasoning
    // ============================================================
    const char* causal_verbs[] = {
        " causes ", " triggers ", " leads to ", " results in ",
        " wakes ", " awakens ", " activates ", " starts ",
        " eats ", " consumes ", " destroys ", " kills ",
        " creates ", " produces ", " generates ",
        NULL
    };
    const char* prevent_verbs[] = {
        " prevents ", " stops ", " blocks ", " inhibits ",
        " slays ", " slayed ", " killed ", " destroyed ",
        " before it could ", " before he could ", " before she could ",
        NULL
    };

    // Check for causal relationships (CAUSES)
    for (int cv = 0; causal_verbs[cv]; cv++) {
        const char* match = strstr(lower, causal_verbs[cv]);
        if (match) {
            // Extract subject (before verb)
            char subject[128] = {0};
            int si = 0;
            const char* subj_start = match - 1;
            while (subj_start > lower && si < 127) {
                if (*subj_start == ' ' || *subj_start == '.' || *subj_start == ',') break;
                subj_start--;
            }
            if (*subj_start == ' ' || *subj_start == '.' || *subj_start == ',') subj_start++;
            while (subj_start < match && si < 127) subject[si++] = *subj_start++;
            while (si > 0 && subject[si-1] == ' ') si--;
            subject[si] = '\\0';

            // Extract object (after verb)
            char object[128] = {0};
            int oi = 0;
            const char* obj_start = match + strlen(causal_verbs[cv]);
            while (*obj_start && oi < 127 && *obj_start != '.' && *obj_start != ',' && *obj_start != '\\n') {
                object[oi++] = *obj_start++;
            }
            while (oi > 0 && object[oi-1] == ' ') oi--;
            object[oi] = '\\0';

            if (strlen(subject) > 1 && strlen(object) > 1) {
                // Create nodes for subject and object
                int64_t subj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_agent", subject, 0.85f);
                int64_t obj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_target", object, 0.85f);

                // Create CAUSES edge
                zeta_create_edge(ctx, subj_id, obj_id, EDGE_CAUSES, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] CAUSAL: %s --CAUSES--> %s\\n", subject, object);
            }
        }
    }

    // Check for preventive relationships (PREVENTS)
    for (int pv = 0; prevent_verbs[pv]; pv++) {
        const char* match = strstr(lower, prevent_verbs[pv]);
        if (match) {
            // Extract subject (before verb)
            char subject[128] = {0};
            int si = 0;
            const char* subj_start = match - 1;
            while (subj_start > lower && si < 127) {
                if (*subj_start == ' ' || *subj_start == '.' || *subj_start == ',') break;
                subj_start--;
            }
            if (*subj_start == ' ' || *subj_start == '.' || *subj_start == ',') subj_start++;
            while (subj_start < match && si < 127) subject[si++] = *subj_start++;
            while (si > 0 && subject[si-1] == ' ') si--;
            subject[si] = '\\0';

            // Extract object (after verb)
            char object[128] = {0};
            int oi = 0;
            const char* obj_start = match + strlen(prevent_verbs[pv]);
            while (*obj_start && oi < 127 && *obj_start != '.' && *obj_start != ',' && *obj_start != '\\n') {
                object[oi++] = *obj_start++;
            }
            while (oi > 0 && object[oi-1] == ' ') oi--;
            object[oi] = '\\0';

            if (strlen(subject) > 1 && strlen(object) > 1) {
                // Create nodes for subject and object
                int64_t subj_id = zeta_create_node(ctx, NODE_ENTITY, "preventer", subject, 0.9f);  // Higher salience for prevention
                int64_t obj_id = zeta_create_node(ctx, NODE_ENTITY, "prevented", object, 0.85f);

                // Create PREVENTS edge
                zeta_create_edge(ctx, subj_id, obj_id, EDGE_PREVENTS, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] PREVENTS: %s --PREVENTS--> %s\\n", subject, object);
            }
        }
    }

    return facts_created;
}'''

    if old_marker in content:
        content = content.replace(old_marker, new_marker)
        print('Added causal extraction patterns')
    else:
        print('ERROR: Could not find update patterns marker')
        # Try simpler pattern
        if 'return facts_created;' in content and '// Update patterns' in content:
            print('Found markers but exact pattern mismatch - check whitespace')

with open(filepath, 'w') as f:
    f.write(content)

print('\\nCausal extraction patch complete!')
print('Features:')
print('  - Detects "X causes Y" patterns -> EDGE_CAUSES')
print('  - Detects "X prevents Y" patterns -> EDGE_PREVENTS')
print('  - Higher salience for prevention events (important for reasoning)')
