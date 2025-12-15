
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
        const char* cmatch = strstr(lower, causal_verbs[cv]);
        if (cmatch) {
            char c_subject[128] = {0};
            const char* c_subj_start = cmatch - 1;
            while (c_subj_start > lower && *c_subj_start == ' ') c_subj_start--;
            const char* c_word_end = c_subj_start + 1;
            while (c_subj_start > lower && *c_subj_start != ' ' && *c_subj_start != '.' && *c_subj_start != ',') {
                c_subj_start--;
            }
            if (*c_subj_start == ' ' || *c_subj_start == '.' || *c_subj_start == ',') c_subj_start++;
            int csi = 0;
            while (c_subj_start < c_word_end && csi < 127) c_subject[csi++] = *c_subj_start++;
            c_subject[csi] = '\0';

            char c_object[128] = {0};
            int coi = 0;
            const char* c_obj_start = cmatch + strlen(causal_verbs[cv]);
            while (*c_obj_start && coi < 127 && *c_obj_start != '.' && *c_obj_start != ',' && *c_obj_start != '\n') {
                c_object[coi++] = *c_obj_start++;
            }
            while (coi > 0 && c_object[coi-1] == ' ') coi--;
            c_object[coi] = '\0';

            if (strlen(c_subject) > 1 && strlen(c_object) > 1) {
                int64_t c_subj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_agent", c_subject, 0.85f);
                int64_t c_obj_id = zeta_create_node(ctx, NODE_ENTITY, "causal_target", c_object, 0.85f);
                zeta_create_edge(ctx, c_subj_id, c_obj_id, EDGE_CAUSES, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] CAUSAL: %s --CAUSES--> %s\n", c_subject, c_object);
            }
        }
    }

    // Check for preventive relationships (PREVENTS)
    for (int pv = 0; prevent_verbs[pv]; pv++) {
        const char* pmatch = strstr(lower, prevent_verbs[pv]);
        if (pmatch) {
            char p_subject[128] = {0};
            const char* p_subj_start = pmatch - 1;
            while (p_subj_start > lower && *p_subj_start == ' ') p_subj_start--;
            const char* p_word_end = p_subj_start + 1;
            while (p_subj_start > lower && *p_subj_start != ' ' && *p_subj_start != '.' && *p_subj_start != ',') {
                p_subj_start--;
            }
            if (*p_subj_start == ' ' || *p_subj_start == '.' || *p_subj_start == ',') p_subj_start++;
            int psi = 0;
            while (p_subj_start < p_word_end && psi < 127) p_subject[psi++] = *p_subj_start++;
            p_subject[psi] = '\0';

            char p_object[128] = {0};
            int poi = 0;
            const char* p_obj_start = pmatch + strlen(prevent_verbs[pv]);
            while (*p_obj_start && poi < 127 && *p_obj_start != '.' && *p_obj_start != ',' && *p_obj_start != '\n') {
                p_object[poi++] = *p_obj_start++;
            }
            while (poi > 0 && p_object[poi-1] == ' ') poi--;
            p_object[poi] = '\0';

            if (strlen(p_subject) > 1 && strlen(p_object) > 1) {
                int64_t p_subj_id = zeta_create_node(ctx, NODE_ENTITY, "preventer", p_subject, 0.9f);
                int64_t p_obj_id = zeta_create_node(ctx, NODE_ENTITY, "prevented", p_object, 0.85f);
                zeta_create_edge(ctx, p_subj_id, p_obj_id, EDGE_PREVENTS, 1.0f);
                facts_created++;
                fprintf(stderr, "[3B] PREVENTS: %s --PREVENTS--> %s\n", p_subject, p_object);
            }
        }
    }

