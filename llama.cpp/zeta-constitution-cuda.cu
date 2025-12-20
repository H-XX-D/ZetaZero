// Z.E.T.A. Constitutional Lock - CUDA Implementation
//
// GPU-accelerated weight decryption using constitutional hash binding.
// Decryption is hidden within HBM memory latency - effectively free.
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#include "zeta-constitution-cuda.cuh"
#include <stdio.h>

// ============================================================================
// GPU Constant Memory - L1 Cached Constitutional Seed
// ============================================================================

// Constitutional hash stored in constant memory for ~1 cycle access
// vs ~400 cycles for global memory. This makes decryption essentially free.
__constant__ uint32_t g_zeta_seed[ZETA_CUDA_SEED_WORDS];
__constant__ bool g_zeta_initialized = false;

// Host-side tracking
static bool s_cuda_constitution_ready = false;

// ============================================================================
// Host API Implementation
// ============================================================================

extern "C" int zeta_cuda_constitution_init(const uint8_t hash[ZETA_CUDA_HASH_SIZE]) {
    if (!hash) {
        fprintf(stderr, "[ZETA-CUDA] ERROR: Null hash provided\n");
        return -1;
    }

    // Convert 32 bytes to 8 x uint32_t
    uint32_t seed[ZETA_CUDA_SEED_WORDS];
    for (int i = 0; i < ZETA_CUDA_SEED_WORDS; i++) {
        seed[i] = ((uint32_t)hash[i * 4 + 0] << 0)  |
                  ((uint32_t)hash[i * 4 + 1] << 8)  |
                  ((uint32_t)hash[i * 4 + 2] << 16) |
                  ((uint32_t)hash[i * 4 + 3] << 24);
    }

    // Copy to GPU constant memory
    cudaError_t err = cudaMemcpyToSymbol(g_zeta_seed, seed, sizeof(seed));
    if (err != cudaSuccess) {
        fprintf(stderr, "[ZETA-CUDA] ERROR: Failed to copy seed to GPU: %s\n",
                cudaGetErrorString(err));
        return -1;
    }

    // Set initialized flag
    bool init_flag = true;
    err = cudaMemcpyToSymbol(g_zeta_initialized, &init_flag, sizeof(bool));
    if (err != cudaSuccess) {
        fprintf(stderr, "[ZETA-CUDA] ERROR: Failed to set init flag: %s\n",
                cudaGetErrorString(err));
        return -1;
    }

    s_cuda_constitution_ready = true;
    fprintf(stderr, "[ZETA-CUDA] Constitutional binding initialized on GPU\n");
    return 0;
}

extern "C" void zeta_cuda_constitution_free(void) {
    bool init_flag = false;
    cudaMemcpyToSymbol(g_zeta_initialized, &init_flag, sizeof(bool));
    s_cuda_constitution_ready = false;
    fprintf(stderr, "[ZETA-CUDA] Constitutional binding freed\n");
}

extern "C" bool zeta_cuda_constitution_ready(void) {
    return s_cuda_constitution_ready;
}

// ============================================================================
// Decryption Kernels
// ============================================================================

// FP32 weight decryption kernel
// Each thread decrypts one or more weights using constitutional XOR mask
__global__ void kernel_decrypt_f32(
    float* __restrict__ weights,
    int n,
    int layer_offset
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    // Get layer-specific seed from constant memory (L1 cached)
    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    // Generate deterministic mask using MurmurHash3 finalizer
    uint32_t z = (uint32_t)(idx + layer_offset) + base_seed;
    z ^= z >> 16;
    z *= 0x85ebca6b;
    z ^= z >> 13;
    z *= 0xc2b2ae35;
    z ^= z >> 16;

    // XOR decrypt - reinterpret float bits
    uint32_t encrypted = __float_as_uint(weights[idx]);
    uint32_t decrypted = encrypted ^ z;
    weights[idx] = __uint_as_float(decrypted);
}

// FP16 weight decryption kernel
// Processes pairs for better memory coalescing
__global__ void kernel_decrypt_f16(
    half* __restrict__ weights,
    int n,
    int layer_offset
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    // Generate 16-bit mask
    uint32_t z = (uint32_t)(idx + layer_offset) + base_seed;
    z ^= z >> 16;
    z *= 0x85ebca6b;
    z ^= z >> 13;
    z *= 0xc2b2ae35;
    z ^= z >> 16;
    uint16_t mask = (uint16_t)(z & 0xFFFF);

    // XOR decrypt
    uint16_t encrypted = __half_as_ushort(weights[idx]);
    uint16_t decrypted = encrypted ^ mask;
    weights[idx] = __ushort_as_half(decrypted);
}

// Vectorized FP16 kernel - processes 2 elements per thread
__global__ void kernel_decrypt_f16_vec2(
    half2* __restrict__ weights,
    int n_pairs,
    int layer_offset
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_pairs) return;

    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    // Generate 32-bit mask for the pair
    uint32_t z = (uint32_t)(idx + layer_offset) + base_seed;
    z ^= z >> 16;
    z *= 0x85ebca6b;
    z ^= z >> 13;
    z *= 0xc2b2ae35;
    z ^= z >> 16;

    // XOR decrypt both halves
    half2 val = weights[idx];
    uint32_t encrypted = *reinterpret_cast<uint32_t*>(&val);
    uint32_t decrypted = encrypted ^ z;
    weights[idx] = *reinterpret_cast<half2*>(&decrypted);
}

// Q4_0 block structure (matches ggml)
struct block_q4_0 {
    half d;                 // Scale
    uint8_t qs[16];         // 32 x 4-bit quantized values
};

// Q4_0 decryption kernel
__global__ void kernel_decrypt_q4_0(
    block_q4_0* __restrict__ blocks,
    int n_blocks,
    int layer_offset
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_blocks) return;

    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    // Decrypt scale (FP16)
    uint32_t z_scale = (uint32_t)(idx * 2 + layer_offset) + base_seed;
    z_scale ^= z_scale >> 16;
    z_scale *= 0x85ebca6b;
    z_scale ^= z_scale >> 13;
    z_scale *= 0xc2b2ae35;
    z_scale ^= z_scale >> 16;

    uint16_t scale_enc = __half_as_ushort(blocks[idx].d);
    blocks[idx].d = __ushort_as_half(scale_enc ^ (uint16_t)(z_scale & 0xFFFF));

    // Decrypt quantized values (16 bytes = 4 x uint32)
    uint32_t* qs_ptr = reinterpret_cast<uint32_t*>(blocks[idx].qs);
    #pragma unroll
    for (int i = 0; i < 4; i++) {
        uint32_t z = (uint32_t)(idx * 8 + i + layer_offset) + base_seed;
        z ^= z >> 16;
        z *= 0x85ebca6b;
        z ^= z >> 13;
        z *= 0xc2b2ae35;
        z ^= z >> 16;
        qs_ptr[i] ^= z;
    }
}

// Q8_0 block structure
struct block_q8_0 {
    half d;                 // Scale
    int8_t qs[32];          // 32 x 8-bit quantized values
};

// Q8_0 decryption kernel
__global__ void kernel_decrypt_q8_0(
    block_q8_0* __restrict__ blocks,
    int n_blocks,
    int layer_offset
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_blocks) return;

    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    // Decrypt scale
    uint32_t z_scale = (uint32_t)(idx * 2 + layer_offset) + base_seed;
    z_scale ^= z_scale >> 16;
    z_scale *= 0x85ebca6b;
    z_scale ^= z_scale >> 13;
    z_scale *= 0xc2b2ae35;
    z_scale ^= z_scale >> 16;

    uint16_t scale_enc = __half_as_ushort(blocks[idx].d);
    blocks[idx].d = __ushort_as_half(scale_enc ^ (uint16_t)(z_scale & 0xFFFF));

    // Decrypt quantized values (32 bytes = 8 x uint32)
    uint32_t* qs_ptr = reinterpret_cast<uint32_t*>(blocks[idx].qs);
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        uint32_t z = (uint32_t)(idx * 16 + i + layer_offset) + base_seed;
        z ^= z >> 16;
        z *= 0x85ebca6b;
        z ^= z >> 13;
        z *= 0xc2b2ae35;
        z ^= z >> 16;
        qs_ptr[i] ^= z;
    }
}

// ============================================================================
// Host-Callable Wrapper Functions
// ============================================================================

extern "C" void zeta_cuda_decrypt_weights_f32(
    float* weights,
    int n,
    int layer_offset,
    cudaStream_t stream
) {
    if (!s_cuda_constitution_ready || n <= 0) return;

    const int block_size = 256;
    const int grid_size = (n + block_size - 1) / block_size;

    kernel_decrypt_f32<<<grid_size, block_size, 0, stream>>>(
        weights, n, layer_offset
    );
}

extern "C" void zeta_cuda_decrypt_weights_f16(
    void* weights_ptr,
    int n,
    int layer_offset,
    cudaStream_t stream
) {
    if (!s_cuda_constitution_ready || n <= 0) return;

    half* weights = static_cast<half*>(weights_ptr);

    // Use vectorized kernel if aligned
    if ((n % 2 == 0) && (((uintptr_t)weights % 4) == 0)) {
        const int n_pairs = n / 2;
        const int block_size = 256;
        const int grid_size = (n_pairs + block_size - 1) / block_size;

        kernel_decrypt_f16_vec2<<<grid_size, block_size, 0, stream>>>(
            reinterpret_cast<half2*>(weights), n_pairs, layer_offset
        );
    } else {
        const int block_size = 256;
        const int grid_size = (n + block_size - 1) / block_size;

        kernel_decrypt_f16<<<grid_size, block_size, 0, stream>>>(
            weights, n, layer_offset
        );
    }
}

extern "C" void zeta_cuda_decrypt_weights_q4_0(
    void* blocks,
    int n_blocks,
    int layer_offset,
    cudaStream_t stream
) {
    if (!s_cuda_constitution_ready || n_blocks <= 0) return;

    const int block_size = 256;
    const int grid_size = (n_blocks + block_size - 1) / block_size;

    kernel_decrypt_q4_0<<<grid_size, block_size, 0, stream>>>(
        static_cast<block_q4_0*>(blocks), n_blocks, layer_offset
    );
}

extern "C" void zeta_cuda_decrypt_weights_q8_0(
    void* blocks,
    int n_blocks,
    int layer_offset,
    cudaStream_t stream
) {
    if (!s_cuda_constitution_ready || n_blocks <= 0) return;

    const int block_size = 256;
    const int grid_size = (n_blocks + block_size - 1) / block_size;

    kernel_decrypt_q8_0<<<grid_size, block_size, 0, stream>>>(
        static_cast<block_q8_0*>(blocks), n_blocks, layer_offset
    );
}

// ============================================================================
// Self-Test (for verification)
// ============================================================================

#ifdef ZETA_CUDA_SELFTEST

#include <stdlib.h>
#include <math.h>

__global__ void kernel_encrypt_f32(
    float* __restrict__ weights,
    int n,
    int layer_offset
) {
    // Encryption is same as decryption (XOR is symmetric)
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    uint32_t base_seed = g_zeta_seed[layer_offset % ZETA_CUDA_SEED_WORDS];

    uint32_t z = (uint32_t)(idx + layer_offset) + base_seed;
    z ^= z >> 16;
    z *= 0x85ebca6b;
    z ^= z >> 13;
    z *= 0xc2b2ae35;
    z ^= z >> 16;

    uint32_t plain = __float_as_uint(weights[idx]);
    uint32_t encrypted = plain ^ z;
    weights[idx] = __uint_as_float(encrypted);
}

int main() {
    printf("[ZETA-CUDA] Running self-test...\n");

    // Create test hash
    uint8_t test_hash[32];
    for (int i = 0; i < 32; i++) {
        test_hash[i] = (uint8_t)(i * 7 + 13);
    }

    // Initialize
    if (zeta_cuda_constitution_init(test_hash) != 0) {
        printf("[ZETA-CUDA] FAILED: Could not initialize\n");
        return 1;
    }

    // Create test data
    const int N = 1024;
    float* h_original = (float*)malloc(N * sizeof(float));
    float* h_result = (float*)malloc(N * sizeof(float));
    float* d_weights;
    cudaMalloc(&d_weights, N * sizeof(float));

    for (int i = 0; i < N; i++) {
        h_original[i] = (float)i * 0.001f - 0.5f;
    }

    cudaMemcpy(d_weights, h_original, N * sizeof(float), cudaMemcpyHostToDevice);

    // Encrypt
    kernel_encrypt_f32<<<(N + 255) / 256, 256>>>(d_weights, N, 42);
    cudaDeviceSynchronize();

    // Decrypt
    zeta_cuda_decrypt_weights_f32(d_weights, N, 42, 0);
    cudaDeviceSynchronize();

    // Verify
    cudaMemcpy(h_result, d_weights, N * sizeof(float), cudaMemcpyDeviceToHost);

    bool pass = true;
    float max_err = 0.0f;
    for (int i = 0; i < N; i++) {
        float err = fabsf(h_original[i] - h_result[i]);
        if (err > max_err) max_err = err;
        if (err > 1e-6f) {
            printf("[ZETA-CUDA] FAILED at %d: expected %f, got %f\n",
                   i, h_original[i], h_result[i]);
            pass = false;
            break;
        }
    }

    if (pass) {
        printf("[ZETA-CUDA] PASSED: Encrypt/decrypt roundtrip verified\n");
        printf("[ZETA-CUDA] Max error: %e\n", max_err);
    }

    // Test wrong key
    uint8_t wrong_hash[32];
    for (int i = 0; i < 32; i++) wrong_hash[i] = (uint8_t)(i * 11 + 7);

    cudaMemcpy(d_weights, h_original, N * sizeof(float), cudaMemcpyHostToDevice);
    kernel_encrypt_f32<<<(N + 255) / 256, 256>>>(d_weights, N, 42);
    cudaDeviceSynchronize();

    // Re-init with wrong key
    zeta_cuda_constitution_init(wrong_hash);
    zeta_cuda_decrypt_weights_f32(d_weights, N, 42, 0);
    cudaDeviceSynchronize();

    cudaMemcpy(h_result, d_weights, N * sizeof(float), cudaMemcpyDeviceToHost);

    int garbage_count = 0;
    for (int i = 0; i < N; i++) {
        if (fabsf(h_original[i] - h_result[i]) > 0.1f) {
            garbage_count++;
        }
    }

    if (garbage_count > N * 0.9) {
        printf("[ZETA-CUDA] PASSED: Wrong key produces garbage (%d/%d corrupted)\n",
               garbage_count, N);
    } else {
        printf("[ZETA-CUDA] WARNING: Wrong key only corrupted %d/%d weights\n",
               garbage_count, N);
    }

    // Cleanup
    cudaFree(d_weights);
    free(h_original);
    free(h_result);
    zeta_cuda_constitution_free();

    printf("[ZETA-CUDA] Self-test complete\n");
    return pass ? 0 : 1;
}

#endif // ZETA_CUDA_SELFTEST
