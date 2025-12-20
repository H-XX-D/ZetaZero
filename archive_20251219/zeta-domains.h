#ifndef ZETA_DOMAINS_H
#define ZETA_DOMAINS_H

/**
 * Z.E.T.A. Semantic Domain Classification
 * 
 * Isolates facts into semantic domains to prevent cross-surfacing:
 * - user_identity: name, birth year, birthplace, nationality
 * - user_possessions: car, pets, devices, property
 * - user_relationships: family, friends, colleagues
 * - user_preferences: favorite color, food, hobbies
 * - user_work: job, company, projects
 * - temporal: dates, events, appointments
 * - credentials: passwords, tokens, codes (high security)
 */

#include <string.h>
#include <ctype.h>

typedef enum {
    DOMAIN_UNKNOWN = 0,
    DOMAIN_USER_IDENTITY,
    DOMAIN_USER_POSSESSIONS,
    DOMAIN_USER_RELATIONSHIPS,
    DOMAIN_USER_PREFERENCES,
    DOMAIN_USER_WORK,
    DOMAIN_TEMPORAL,
    DOMAIN_CREDENTIALS,
    DOMAIN_GENERAL
} zeta_semantic_domain_t;

// Keywords for domain classification
static const char* IDENTITY_KEYWORDS[] = {
    "born", "birth", "name", "called", "nationality", "citizen",
    "age", "old", "years old", "grew up", "from",
    NULL
};

static const char* POSSESSION_KEYWORDS[] = {
    "car", "vehicle", "pet", "dog", "cat", "bird", "phone",
    "house", "apartment", "own", "have a", "my",
    "tesla", "toyota", "honda", "bmw", "ford",
    NULL
};

static const char* RELATIONSHIP_KEYWORDS[] = {
    "friend", "family", "sister", "brother", "mother", "father",
    "wife", "husband", "partner", "child", "son", "daughter",
    "colleague", "boss", "team", "named",
    NULL
};

static const char* PREFERENCE_KEYWORDS[] = {
    "favorite", "like", "love", "prefer", "enjoy", "hate",
    "color", "food", "movie", "music", "hobby",
    NULL
};

static const char* WORK_KEYWORDS[] = {
    "work", "job", "company", "project", "employ", "career",
    "office", "team", "meeting", "salary",
    NULL
};

static const char* TEMPORAL_KEYWORDS[] = {
    "date", "time", "appointment", "schedule", "event",
    "tomorrow", "yesterday", "monday", "tuesday", "january",
    NULL
};

static const char* CREDENTIAL_KEYWORDS[] = {
    "password", "code", "secret", "token", "key", "pin",
    "api", "auth", "login", "credential",
    NULL
};

// Check if text contains any keyword from list
static inline bool zeta_has_keyword(const char* text, const char** keywords) {
    char lower[1024];
    size_t i = 0;
    while (text[i] && i < sizeof(lower) - 1) {
        lower[i] = tolower((unsigned char)text[i]);
        i++;
    }
    lower[i] = '\0';
    
    for (int k = 0; keywords[k]; k++) {
        if (strstr(lower, keywords[k])) {
            return true;
        }
    }
    return false;
}

// Classify text into a semantic domain
static inline zeta_semantic_domain_t zeta_classify_domain(const char* text) {
    // Check in order of specificity
    if (zeta_has_keyword(text, CREDENTIAL_KEYWORDS)) {
        return DOMAIN_CREDENTIALS;
    }
    if (zeta_has_keyword(text, IDENTITY_KEYWORDS)) {
        return DOMAIN_USER_IDENTITY;
    }
    if (zeta_has_keyword(text, RELATIONSHIP_KEYWORDS)) {
        return DOMAIN_USER_RELATIONSHIPS;
    }
    if (zeta_has_keyword(text, POSSESSION_KEYWORDS)) {
        return DOMAIN_USER_POSSESSIONS;
    }
    if (zeta_has_keyword(text, PREFERENCE_KEYWORDS)) {
        return DOMAIN_USER_PREFERENCES;
    }
    if (zeta_has_keyword(text, WORK_KEYWORDS)) {
        return DOMAIN_USER_WORK;
    }
    if (zeta_has_keyword(text, TEMPORAL_KEYWORDS)) {
        return DOMAIN_TEMPORAL;
    }
    
    return DOMAIN_GENERAL;
}

// Get domain name for display
static inline const char* zeta_domain_name(zeta_semantic_domain_t domain) {
    switch (domain) {
        case DOMAIN_USER_IDENTITY: return "identity";
        case DOMAIN_USER_POSSESSIONS: return "possessions";
        case DOMAIN_USER_RELATIONSHIPS: return "relationships";
        case DOMAIN_USER_PREFERENCES: return "preferences";
        case DOMAIN_USER_WORK: return "work";
        case DOMAIN_TEMPORAL: return "temporal";
        case DOMAIN_CREDENTIALS: return "credentials";
        case DOMAIN_GENERAL: return "general";
        default: return "unknown";
    }
}

// Check if two domains are related (should cross-reference)
static inline bool zeta_domains_related(
    zeta_semantic_domain_t d1,
    zeta_semantic_domain_t d2
) {
    // Same domain is always related
    if (d1 == d2) return true;
    
    // General domain relates to everything
    if (d1 == DOMAIN_GENERAL || d2 == DOMAIN_GENERAL) return true;
    
    // Unknown relates to nothing except itself
    if (d1 == DOMAIN_UNKNOWN || d2 == DOMAIN_UNKNOWN) return false;
    
    // Credentials never cross-reference
    if (d1 == DOMAIN_CREDENTIALS || d2 == DOMAIN_CREDENTIALS) return false;
    
    // Some domains have natural relationships:
    // - Identity and Relationships (my sister's name)
    // - Work and Relationships (colleagues)
    // - Possessions and Preferences (my favorite car)
    
    if ((d1 == DOMAIN_USER_IDENTITY && d2 == DOMAIN_USER_RELATIONSHIPS) ||
        (d2 == DOMAIN_USER_IDENTITY && d1 == DOMAIN_USER_RELATIONSHIPS)) {
        return true;
    }
    
    if ((d1 == DOMAIN_USER_WORK && d2 == DOMAIN_USER_RELATIONSHIPS) ||
        (d2 == DOMAIN_USER_WORK && d1 == DOMAIN_USER_RELATIONSHIPS)) {
        return true;
    }
    
    if ((d1 == DOMAIN_USER_POSSESSIONS && d2 == DOMAIN_USER_PREFERENCES) ||
        (d2 == DOMAIN_USER_POSSESSIONS && d1 == DOMAIN_USER_PREFERENCES)) {
        return true;
    }
    
    // Default: not related
    return false;
}

#endif // ZETA_DOMAINS_H
