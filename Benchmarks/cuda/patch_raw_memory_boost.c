
        // BOOST: raw_memory nodes contain full context - always boost them
        if (strcmp(node->label, "raw_memory") == 0) {
            priority *= 3.0f;  // 3x boost for raw memories
        }

