// Z.E.T.A. Constitutional Lock Implementation
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-constitution.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// SHA-256 Implementation (Standalone, no dependencies)
// ============================================================================

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

typedef struct {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[64];
} sha256_ctx;

static void sha256_transform(sha256_ctx* ctx, const uint8_t* data) {
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    for (; i < 64; i++) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(sha256_ctx* ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->bitcount = 0;
}

static void sha256_update(sha256_ctx* ctx, const uint8_t* data, size_t len) {
    size_t i;
    uint32_t bufidx = (ctx->bitcount >> 3) & 0x3F;

    ctx->bitcount += len << 3;

    for (i = 0; i < len; i++) {
        ctx->buffer[bufidx++] = data[i];
        if (bufidx == 64) {
            sha256_transform(ctx, ctx->buffer);
            bufidx = 0;
        }
    }
}

static void sha256_final(sha256_ctx* ctx, uint8_t hash[32]) {
    uint32_t bufidx = (ctx->bitcount >> 3) & 0x3F;
    int i;

    ctx->buffer[bufidx++] = 0x80;
    if (bufidx > 56) {
        while (bufidx < 64) ctx->buffer[bufidx++] = 0;
        sha256_transform(ctx, ctx->buffer);
        bufidx = 0;
    }
    while (bufidx < 56) ctx->buffer[bufidx++] = 0;

    // Append bit length (big-endian)
    for (i = 7; i >= 0; i--) {
        ctx->buffer[56 + (7 - i)] = (ctx->bitcount >> (i * 8)) & 0xFF;
    }
    sha256_transform(ctx, ctx->buffer);

    // Output hash (big-endian)
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = ctx->state[i] & 0xFF;
    }
}

void zeta_sha256(const uint8_t* data, size_t length, uint8_t hash_out[ZETA_HASH_SIZE]) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, length);
    sha256_final(&ctx, hash_out);
}

int zeta_sha256_file(const char* filepath, uint8_t hash_out[ZETA_HASH_SIZE]) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return -1;

    sha256_ctx ctx;
    sha256_init(&ctx);

    uint8_t buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        sha256_update(&ctx, buffer, bytes_read);
    }

    // Check for I/O errors - don't hash incomplete data
    if (ferror(f)) {
        fclose(f);
        return -1;
    }

    fclose(f);
    sha256_final(&ctx, hash_out);
    return 0;
}

// ============================================================================
// PRNG (Xoshiro256** seeded from hash)
// ============================================================================

typedef struct {
    uint64_t s[4];
} xoshiro_state;

static inline uint64_t rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t xoshiro_next(xoshiro_state* state) {
    uint64_t* s = state->s;
    const uint64_t result = rotl64(s[1] * 5, 7) * 9;
    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl64(s[3], 45);

    return result;
}

static void xoshiro_seed(xoshiro_state* state, const uint8_t hash[32]) {
    // Split 256-bit hash into 4 x 64-bit seeds
    for (int i = 0; i < 4; i++) {
        state->s[i] = 0;
        for (int j = 0; j < 8; j++) {
            state->s[i] |= ((uint64_t)hash[i * 8 + j]) << (j * 8);
        }
    }

    // Warm up the generator
    for (int i = 0; i < 20; i++) {
        xoshiro_next(state);
    }
}

// ============================================================================
// Constitution Context
// ============================================================================

zeta_constitution_t* zeta_constitution_init(const char* constitution_path) {
    FILE* f = fopen(constitution_path, "rb");
    if (!f) {
        fprintf(stderr, "[CRITICAL] Constitution file not found: %s\n", constitution_path);
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > ZETA_CONSTITUTION_MAX) {
        fprintf(stderr, "[CRITICAL] Invalid constitution size: %ld bytes\n", size);
        fclose(f);
        return NULL;
    }

    // Read constitution
    uint8_t* text = (uint8_t*)malloc(size);
    if (!text || fread(text, 1, size, f) != (size_t)size) {
        fprintf(stderr, "[CRITICAL] Failed to read constitution\n");
        free(text);
        fclose(f);
        return NULL;
    }
    fclose(f);

    // Create context
    zeta_constitution_t* ctx = zeta_constitution_init_embedded((const char*)text, size);
    free(text);

    if (ctx) {
        strncpy(ctx->constitution_path, constitution_path, sizeof(ctx->constitution_path) - 1);
    }

    return ctx;
}

zeta_constitution_t* zeta_constitution_init_embedded(
    const char* constitution_text,
    size_t text_length
) {
    zeta_constitution_t* ctx = (zeta_constitution_t*)calloc(1, sizeof(zeta_constitution_t));
    if (!ctx) return NULL;

    // Compute SHA-256 of constitution
    zeta_sha256((const uint8_t*)constitution_text, text_length, ctx->hash);

    // Derive 64-bit seed from hash (first 8 bytes)
    ctx->seed = 0;
    for (int i = 0; i < 8; i++) {
        ctx->seed |= ((uint64_t)ctx->hash[i]) << (i * 8);
    }

    ctx->verified = false;
    ctx->constitution_path[0] = '\0';

    // Log the hash
    char hex[ZETA_HASH_SIZE * 2 + 1];
    zeta_constitution_hash_to_hex(ctx->hash, hex);
    fprintf(stderr, "[CONSTITUTION] Hash: %s\n", hex);
    fprintf(stderr, "[CONSTITUTION] Seed: 0x%016llx\n", (unsigned long long)ctx->seed);

    return ctx;
}

void zeta_constitution_free(zeta_constitution_t* ctx) {
    if (ctx) {
        // Zero out sensitive data
        memset(ctx->hash, 0, ZETA_HASH_SIZE);
        ctx->seed = 0;
        free(ctx);
    }
}

bool zeta_constitution_verify(
    const zeta_constitution_t* ctx,
    const uint8_t expected_hash[ZETA_HASH_SIZE]
) {
    if (!ctx || !expected_hash) return false;
    return memcmp(ctx->hash, expected_hash, ZETA_HASH_SIZE) == 0;
}

void zeta_constitution_hash_to_hex(
    const uint8_t hash[ZETA_HASH_SIZE],
    char hex_out[ZETA_HASH_SIZE * 2 + 1]
) {
    static const char* hex_chars = "0123456789abcdef";
    for (int i = 0; i < ZETA_HASH_SIZE; i++) {
        hex_out[i * 2] = hex_chars[(hash[i] >> 4) & 0xF];
        hex_out[i * 2 + 1] = hex_chars[hash[i] & 0xF];
    }
    hex_out[ZETA_HASH_SIZE * 2] = '\0';
}

// ============================================================================
// Weight Permutation
// ============================================================================

void zeta_generate_permutation(
    const zeta_constitution_t* ctx,
    int* permutation_out,
    int n
) {
    if (!ctx || !permutation_out || n <= 0) return;

    // Initialize with identity permutation
    for (int i = 0; i < n; i++) {
        permutation_out[i] = i;
    }

    // Fisher-Yates shuffle using constitutional PRNG
    xoshiro_state rng;
    xoshiro_seed(&rng, ctx->hash);

    for (int i = n - 1; i > 0; i--) {
        uint64_t j = xoshiro_next(&rng) % (i + 1);
        // Swap
        int tmp = permutation_out[i];
        permutation_out[i] = permutation_out[j];
        permutation_out[j] = tmp;
    }
}

void zeta_permute_weights(
    const int* permutation,
    const float* weights_in,
    float* weights_out,
    int n
) {
    if (!permutation || !weights_in || !weights_out || n <= 0) return;

    // Apply permutation: out[i] = in[perm[i]]
    for (int i = 0; i < n; i++) {
        weights_out[i] = weights_in[permutation[i]];
    }
}

void zeta_unpermute_weights(
    const int* permutation,
    const float* weights_in,
    float* weights_out,
    int n
) {
    if (!permutation || !weights_in || !weights_out || n <= 0) return;

    // Apply inverse permutation: out[perm[i]] = in[i]
    for (int i = 0; i < n; i++) {
        weights_out[permutation[i]] = weights_in[i];
    }
}

// ============================================================================
// Attention Head Scrambling
// ============================================================================

void zeta_scramble_attention_heads(
    const zeta_constitution_t* ctx,
    int layer_idx,
    int n_head,
    int* head_order_out
) {
    if (!ctx || !head_order_out || n_head <= 0) return;

    // Create layer-specific hash by mixing layer_idx into the hash
    uint8_t layer_hash[ZETA_HASH_SIZE];
    memcpy(layer_hash, ctx->hash, ZETA_HASH_SIZE);

    // XOR layer index into first 4 bytes
    layer_hash[0] ^= (layer_idx & 0xFF);
    layer_hash[1] ^= ((layer_idx >> 8) & 0xFF);
    layer_hash[2] ^= ((layer_idx >> 16) & 0xFF);
    layer_hash[3] ^= ((layer_idx >> 24) & 0xFF);

    // Hash again to mix thoroughly
    zeta_sha256(layer_hash, ZETA_HASH_SIZE, layer_hash);

    // Generate permutation with layer-specific seed
    for (int i = 0; i < n_head; i++) {
        head_order_out[i] = i;
    }

    xoshiro_state rng;
    xoshiro_seed(&rng, layer_hash);

    for (int i = n_head - 1; i > 0; i--) {
        uint64_t j = xoshiro_next(&rng) % (i + 1);
        int tmp = head_order_out[i];
        head_order_out[i] = head_order_out[j];
        head_order_out[j] = tmp;
    }
}

// ============================================================================
// Model Loading Integration
// ============================================================================

int zeta_constitution_prepare_model_load(
    zeta_constitution_t* ctx,
    const uint8_t expected_hash[ZETA_HASH_SIZE]
) {
    if (!ctx) {
        fprintf(stderr, "[CRITICAL] Constitution Hash Mismatch. Entropy Decryption Failed.\n");
        fprintf(stderr, "[CRITICAL] This model requires its ethical framework to function.\n");
        return -1;
    }

    if (!zeta_constitution_verify(ctx, expected_hash)) {
        char expected_hex[ZETA_HASH_SIZE * 2 + 1];
        char actual_hex[ZETA_HASH_SIZE * 2 + 1];
        zeta_constitution_hash_to_hex(expected_hash, expected_hex);
        zeta_constitution_hash_to_hex(ctx->hash, actual_hex);

        fprintf(stderr, "\n");
        fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║  [CRITICAL] CONSTITUTION HASH MISMATCH                       ║\n");
        fprintf(stderr, "║  Entropy Decryption Failed. Model cannot function.           ║\n");
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Expected: %.32s...  ║\n", expected_hex);
        fprintf(stderr, "║  Actual:   %.32s...  ║\n", actual_hex);
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  This model requires its ethical framework to function.      ║\n");
        fprintf(stderr, "║  Restore the original constitution to proceed.               ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");

        ctx->verified = false;
        return -1;
    }

    fprintf(stderr, "[CONSTITUTION] Verified. Entropy key derived successfully.\n");
    ctx->verified = true;
    return 0;
}

void zeta_constitution_print_status(const zeta_constitution_t* ctx) {
    if (!ctx) {
        fprintf(stderr, "[CONSTITUTION] Not initialized\n");
        return;
    }

    char hex[ZETA_HASH_SIZE * 2 + 1];
    zeta_constitution_hash_to_hex(ctx->hash, hex);

    fprintf(stderr, "\n=== Z.E.T.A. Constitutional Lock ===\n");
    fprintf(stderr, "Path:     %s\n", ctx->constitution_path[0] ? ctx->constitution_path : "(embedded)");
    fprintf(stderr, "Hash:     %s\n", hex);
    fprintf(stderr, "Seed:     0x%016llx\n", (unsigned long long)ctx->seed);
    fprintf(stderr, "Verified: %s\n", ctx->verified ? "YES" : "NO");
    fprintf(stderr, "====================================\n\n");
}

// ============================================================================
// Level 3: Remote Hash Verification
// ============================================================================

int zeta_constitution_parse_hex_hash(
    const char* hex_string,
    uint8_t hash_out[ZETA_HASH_SIZE]
) {
    // Skip whitespace and comments
    while (*hex_string && (*hex_string == ' ' || *hex_string == '\t' || *hex_string == '\n' || *hex_string == '#')) {
        if (*hex_string == '#') {
            // Skip comment line
            while (*hex_string && *hex_string != '\n') hex_string++;
        }
        if (*hex_string) hex_string++;
    }

    // Parse 64 hex characters
    for (int i = 0; i < ZETA_HASH_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex_string + i * 2, "%2x", &byte) != 1) {
            return -1;  // Parse error
        }
        hash_out[i] = (uint8_t)byte;
    }
    return 0;
}

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

// Simple HTTP GET implementation for remote hash fetch
static int http_get_simple(const char* url, char* response, size_t max_response) {
    // Parse URL: https://raw.githubusercontent.com/H-XX-D/ZetaZero/master/CONSTITUTION_HASH
    char host[256] = {0};
    char path[512] = {0};

    // Check if HTTPS (we'll fall back to HTTP for simplicity)
    if (strncmp(url, "https://", 8) == 0) {
        url += 8;
    } else if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }

    // Extract host and path
    const char* path_start = strchr(url, '/');
    if (path_start) {
        size_t host_len = path_start - url;
        if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
        strncpy(host, url, host_len);
        strncpy(path, path_start, sizeof(path) - 1);
    } else {
        strncpy(host, url, sizeof(host) - 1);
        strcpy(path, "/");
    }

    // For HTTPS, use system curl command (simplest cross-platform approach)
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s --connect-timeout 5 'https://%s%s' 2>/dev/null", host, path);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    size_t bytes_read = fread(response, 1, max_response - 1, fp);
    response[bytes_read] = '\0';

    int status = pclose(fp);
    return (status == 0 && bytes_read > 0) ? 0 : -1;
}

int zeta_constitution_verify_remote(
    const zeta_constitution_t* ctx,
    const char* remote_url
) {
    if (!ctx || !remote_url) return -1;

    fprintf(stderr, "[CONSTITUTION] Level 3: Fetching remote hash from %s\n", remote_url);

    char response[1024] = {0};
    if (http_get_simple(remote_url, response, sizeof(response)) != 0) {
        fprintf(stderr, "[CONSTITUTION] ERROR: Failed to fetch remote hash (network error)\n");
        return -1;  // Network error
    }

    // Parse the hash from response
    uint8_t remote_hash[ZETA_HASH_SIZE];
    if (zeta_constitution_parse_hex_hash(response, remote_hash) != 0) {
        fprintf(stderr, "[CONSTITUTION] ERROR: Failed to parse remote hash\n");
        return -1;
    }

    // Compare with constitution hash
    if (memcmp(ctx->hash, remote_hash, ZETA_HASH_SIZE) != 0) {
        char expected_hex[ZETA_HASH_SIZE * 2 + 1];
        char actual_hex[ZETA_HASH_SIZE * 2 + 1];
        zeta_constitution_hash_to_hex(remote_hash, expected_hex);
        zeta_constitution_hash_to_hex(ctx->hash, actual_hex);

        fprintf(stderr, "\n");
        fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║  ⚠️  LEVEL 3 CONSTITUTIONAL LOCK FAILURE                      ║\n");
        fprintf(stderr, "║  Remote hash verification FAILED                             ║\n");
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Remote:   %.32s...  ║\n", expected_hex);
        fprintf(stderr, "║  Local:    %.32s...  ║\n", actual_hex);
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Constitution has been tampered with.                        ║\n");
        fprintf(stderr, "║  Server will not start.                                      ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");
        return -2;  // Hash mismatch
    }

    fprintf(stderr, "[CONSTITUTION] Level 3: Remote verification PASSED ✓\n");
    return 0;
}

// ============================================================================
// Level 4: Model Metadata Hash Verification
// ============================================================================

int zeta_constitution_verify_model_metadata(
    const zeta_constitution_t* ctx,
    const char* model_path
) {
    if (!ctx || !model_path) return -1;

    fprintf(stderr, "[CONSTITUTION] Level 4: Reading hash from model metadata: %s\n", model_path);

    // Read GGUF file header to find metadata
    FILE* f = fopen(model_path, "rb");
    if (!f) {
        fprintf(stderr, "[CONSTITUTION] ERROR: Cannot open model file\n");
        return -1;
    }

    // GGUF magic: 'GGUF' = 0x46554747
    uint32_t magic;
    if (fread(&magic, 4, 1, f) != 1 || magic != 0x46554747) {
        fclose(f);
        fprintf(stderr, "[CONSTITUTION] ERROR: Not a valid GGUF file\n");
        return -1;
    }

    // GGUF version
    uint32_t version;
    fread(&version, 4, 1, f);

    // Number of tensors and metadata KV pairs
    uint64_t n_tensors, n_kv;
    fread(&n_tensors, 8, 1, f);
    fread(&n_kv, 8, 1, f);

    // Search for zeta.constitution_hash key
    const char* target_key = "zeta.constitution_hash";
    char key_buf[256];
    uint8_t model_hash[ZETA_HASH_SIZE];
    bool found = false;

    for (uint64_t i = 0; i < n_kv && !found; i++) {
        // Read key length and key
        uint64_t key_len;
        fread(&key_len, 8, 1, f);

        if (key_len < sizeof(key_buf)) {
            fread(key_buf, 1, key_len, f);
            key_buf[key_len] = '\0';

            // Read value type
            uint32_t value_type;
            fread(&value_type, 4, 1, f);

            if (strcmp(key_buf, target_key) == 0) {
                // Found our key! Read the hash value
                // Type 8 = GGUF_TYPE_STRING (hash as hex string)
                if (value_type == 8) {
                    uint64_t str_len;
                    fread(&str_len, 8, 1, f);
                    char hash_str[128];
                    fread(hash_str, 1, (str_len < 127 ? str_len : 127), f);
                    hash_str[str_len < 127 ? str_len : 127] = '\0';

                    if (zeta_constitution_parse_hex_hash(hash_str, model_hash) == 0) {
                        found = true;
                    }
                }
                // Type 9 = GGUF_TYPE_ARRAY of uint8 (raw bytes)
                else if (value_type == 9) {
                    uint32_t arr_type;
                    uint64_t arr_len;
                    fread(&arr_type, 4, 1, f);
                    fread(&arr_len, 8, 1, f);
                    if (arr_len == ZETA_HASH_SIZE) {
                        fread(model_hash, 1, ZETA_HASH_SIZE, f);
                        found = true;
                    }
                }
            } else {
                // Skip this value (simplified - just skip based on type)
                // This is a simplified parser; production would need full GGUF parsing
                fseek(f, 0, SEEK_CUR);  // Placeholder - would need type-specific skipping
            }
        }
    }

    fclose(f);

    if (!found) {
        fprintf(stderr, "[CONSTITUTION] WARNING: Model does not contain constitution hash metadata\n");
        fprintf(stderr, "[CONSTITUTION] To embed hash, run: zeta-embed-hash --model <model.gguf>\n");
        return -1;  // No metadata found
    }

    // Compare hashes
    if (memcmp(ctx->hash, model_hash, ZETA_HASH_SIZE) != 0) {
        char expected_hex[ZETA_HASH_SIZE * 2 + 1];
        char actual_hex[ZETA_HASH_SIZE * 2 + 1];
        zeta_constitution_hash_to_hex(model_hash, expected_hex);
        zeta_constitution_hash_to_hex(ctx->hash, actual_hex);

        fprintf(stderr, "\n");
        fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║  ⚠️  LEVEL 4 CONSTITUTIONAL LOCK FAILURE                      ║\n");
        fprintf(stderr, "║  Model metadata hash verification FAILED                     ║\n");
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Model:    %.32s...  ║\n", expected_hex);
        fprintf(stderr, "║  Local:    %.32s...  ║\n", actual_hex);
        fprintf(stderr, "╠══════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Constitution or model has been tampered with.               ║\n");
        fprintf(stderr, "║  Server will not start.                                      ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");
        return -2;  // Hash mismatch
    }

    fprintf(stderr, "[CONSTITUTION] Level 4: Model metadata verification PASSED ✓\n");
    return 0;
}
