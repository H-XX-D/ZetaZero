// Z.E.T.A. Metal Kernel Dispatch Implementation
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

#import "zeta-metal.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Metal Context Structure
// ============================================================================

struct zeta_metal_ctx {
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLLibrary> library;

    // Compute pipelines
    id<MTLComputePipelineState> temporal_decay;
    id<MTLComputePipelineState> sparse_gate;
    id<MTLComputePipelineState> attention_modifier;
    id<MTLComputePipelineState> generate_mask;
    id<MTLComputePipelineState> memory_injection;
    id<MTLComputePipelineState> sparse_softmax;
    id<MTLComputePipelineState> cosine_similarity;
};

// ============================================================================
// Initialization
// ============================================================================

bool zeta_metal_available(void) {
    @autoreleasepool {
        NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
        bool available = devices.count > 0;
        // ARC handles release
        return available;
    }
}

static id<MTLComputePipelineState> create_pipeline(id<MTLDevice> device, id<MTLLibrary> library, const char* name) {
    NSString* funcName = [NSString stringWithUTF8String:name];
    id<MTLFunction> function = [library newFunctionWithName:funcName];

    if (!function) {
        fprintf(stderr, "[Z.E.T.A.] Failed to find kernel: %s\n", name);
        return nil;
    }

    NSError* error = nil;
    id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];

    if (error) {
        fprintf(stderr, "[Z.E.T.A.] Failed to create pipeline for %s: %s\n",
                name, [[error localizedDescription] UTF8String]);
        return nil;
    }

    return pipeline;
}

zeta_metal_ctx_t* zeta_metal_init(void) {
    @autoreleasepool {
        zeta_metal_ctx_t* ctx = calloc(1, sizeof(zeta_metal_ctx_t));
        if (!ctx) return NULL;

        // Get default Metal device
        ctx->device = MTLCreateSystemDefaultDevice();
        if (!ctx->device) {
            fprintf(stderr, "[Z.E.T.A.] No Metal device available\n");
            free(ctx);
            return NULL;
        }

        // Create command queue
        ctx->queue = [ctx->device newCommandQueue];
        if (!ctx->queue) {
            fprintf(stderr, "[Z.E.T.A.] Failed to create command queue\n");
            free(ctx);
            return NULL;
        }

        // Find Z.E.T.A. kernel library
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* libPath = [bundle pathForResource:@"zeta-kernels" ofType:@"metallib"];

        NSError* error = nil;

        if (libPath) {
            NSURL* libURL = [NSURL fileURLWithPath:libPath];
            ctx->library = [ctx->device newLibraryWithURL:libURL error:&error];
        }

        // Second try: Load from source file
        if (!ctx->library) {
            NSString* sourcePath = [bundle pathForResource:@"zeta-kernels" ofType:@"metal"];

            if (!sourcePath) {
                NSFileManager* fm = [NSFileManager defaultManager];
                NSString* cwd = [fm currentDirectoryPath];
                sourcePath = [cwd stringByAppendingPathComponent:@"zeta-kernels.metal"];

                if (![fm fileExistsAtPath:sourcePath]) {
                    NSString* execPath = [[NSBundle mainBundle] executablePath];
                    NSString* execDir = [execPath stringByDeletingLastPathComponent];
                    sourcePath = [execDir stringByAppendingPathComponent:@"zeta-kernels.metal"];

                    if (![fm fileExistsAtPath:sourcePath]) {
                        sourcePath = [execDir stringByAppendingPathComponent:@"../../zeta-kernels.metal"];
                    }
                }
            }

            if (sourcePath && [[NSFileManager defaultManager] fileExistsAtPath:sourcePath]) {
                NSString* source = [NSString stringWithContentsOfFile:sourcePath
                                                             encoding:NSUTF8StringEncoding
                                                                error:&error];
                if (source) {
                    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
                    options.fastMathEnabled = YES;

                    ctx->library = [ctx->device newLibraryWithSource:source options:options error:&error];

                    if (ctx->library) {
                        fprintf(stderr, "[Z.E.T.A.] Compiled Metal kernels from source\n");
                    }
                }
            }
        }

        if (!ctx->library) {
            fprintf(stderr, "[Z.E.T.A.] Failed to load Metal library: %s\n",
                    error ? [[error localizedDescription] UTF8String] : "not found");
            fprintf(stderr, "[Z.E.T.A.] Falling back to CPU implementation\n");
            free(ctx);
            return NULL;
        }

        // Create pipelines
        ctx->temporal_decay = create_pipeline(ctx->device, ctx->library, "zeta_temporal_decay");
        ctx->sparse_gate = create_pipeline(ctx->device, ctx->library, "zeta_sparse_gate");
        ctx->attention_modifier = create_pipeline(ctx->device, ctx->library, "zeta_attention_modifier");
        ctx->generate_mask = create_pipeline(ctx->device, ctx->library, "zeta_generate_mask");
        ctx->memory_injection = create_pipeline(ctx->device, ctx->library, "zeta_memory_injection");
        ctx->sparse_softmax = create_pipeline(ctx->device, ctx->library, "zeta_sparse_softmax");
        ctx->cosine_similarity = create_pipeline(ctx->device, ctx->library, "zeta_cosine_similarity");

        fprintf(stderr, "[Z.E.T.A.] Metal kernels initialized on %s\n",
                [[ctx->device name] UTF8String]);

        return ctx;
    }
}

void zeta_metal_free(zeta_metal_ctx_t* ctx) {
    if (!ctx) return;

    @autoreleasepool {
        // ARC handles all object releases when we nil them
        ctx->temporal_decay = nil;
        ctx->sparse_gate = nil;
        ctx->attention_modifier = nil;
        ctx->generate_mask = nil;
        ctx->memory_injection = nil;
        ctx->sparse_softmax = nil;
        ctx->cosine_similarity = nil;
        ctx->library = nil;
        ctx->queue = nil;
        ctx->device = nil;

        free(ctx);
    }
}

// ============================================================================
// Temporal Decay Kernel
// ============================================================================

int zeta_metal_temporal_decay(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda
) {
    if (!ctx || !ctx->temporal_decay) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * kv_len * sizeof(float);

        id<MTLBuffer> scores_buf = [ctx->device newBufferWithBytes:attention_scores
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->temporal_decay];
        [enc setBuffer:scores_buf offset:0 atIndex:0];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:1];
        [enc setBytes:&kv_len length:sizeof(int) atIndex:2];
        [enc setBytes:&current_pos length:sizeof(int) atIndex:3];
        [enc setBytes:&lambda length:sizeof(float) atIndex:4];

        MTLSize grid = MTLSizeMake(kv_len, seq_len, 1);
        MTLSize threadgroup = MTLSizeMake(
            MIN(16, kv_len),
            MIN(16, seq_len),
            1
        );

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(attention_scores, [scores_buf contents], buffer_size);
        // ARC handles buffer release

        return 0;
    }
}

// ============================================================================
// Sparse Gating Kernel
// ============================================================================

int zeta_metal_sparse_gate(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,
    int seq_len,
    int kv_len,
    float threshold
) {
    if (!ctx || !ctx->sparse_gate) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * kv_len * sizeof(float);

        id<MTLBuffer> scores_buf = [ctx->device newBufferWithBytes:attention_scores
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->sparse_gate];
        [enc setBuffer:scores_buf offset:0 atIndex:0];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:1];
        [enc setBytes:&kv_len length:sizeof(int) atIndex:2];
        [enc setBytes:&threshold length:sizeof(float) atIndex:3];

        MTLSize grid = MTLSizeMake(kv_len, seq_len, 1);
        MTLSize threadgroup = MTLSizeMake(
            MIN(16, kv_len),
            MIN(16, seq_len),
            1
        );

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(attention_scores, [scores_buf contents], buffer_size);

        return 0;
    }
}

// ============================================================================
// Combined Attention Modifier
// ============================================================================

int zeta_metal_attention_modifier(
    zeta_metal_ctx_t* ctx,
    float* attention_scores,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold
) {
    if (!ctx || !ctx->attention_modifier) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * kv_len * sizeof(float);

        id<MTLBuffer> scores_buf = [ctx->device newBufferWithBytes:attention_scores
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->attention_modifier];
        [enc setBuffer:scores_buf offset:0 atIndex:0];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:1];
        [enc setBytes:&kv_len length:sizeof(int) atIndex:2];
        [enc setBytes:&current_pos length:sizeof(int) atIndex:3];
        [enc setBytes:&lambda length:sizeof(float) atIndex:4];
        [enc setBytes:&threshold length:sizeof(float) atIndex:5];

        MTLSize grid = MTLSizeMake(kv_len, seq_len, 1);
        MTLSize threadgroup = MTLSizeMake(
            MIN(16, kv_len),
            MIN(16, seq_len),
            1
        );

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(attention_scores, [scores_buf contents], buffer_size);

        return 0;
    }
}

// ============================================================================
// Generate Mask
// ============================================================================

int zeta_metal_generate_mask(
    zeta_metal_ctx_t* ctx,
    float* mask,
    int seq_len,
    int kv_len,
    int current_pos,
    float lambda,
    float threshold,
    int causal
) {
    if (!ctx || !ctx->generate_mask) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * kv_len * sizeof(float);

        id<MTLBuffer> mask_buf = [ctx->device newBufferWithLength:buffer_size
                                                          options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->generate_mask];
        [enc setBuffer:mask_buf offset:0 atIndex:0];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:1];
        [enc setBytes:&kv_len length:sizeof(int) atIndex:2];
        [enc setBytes:&current_pos length:sizeof(int) atIndex:3];
        [enc setBytes:&lambda length:sizeof(float) atIndex:4];
        [enc setBytes:&threshold length:sizeof(float) atIndex:5];
        [enc setBytes:&causal length:sizeof(int) atIndex:6];

        MTLSize grid = MTLSizeMake(kv_len, seq_len, 1);
        MTLSize threadgroup = MTLSizeMake(
            MIN(16, kv_len),
            MIN(16, seq_len),
            1
        );

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(mask, [mask_buf contents], buffer_size);

        return 0;
    }
}

// ============================================================================
// Memory Injection
// ============================================================================

int zeta_metal_memory_injection(
    zeta_metal_ctx_t* ctx,
    float* output,
    const float* memory_output,
    int seq_len,
    int dim,
    float alpha
) {
    if (!ctx || !ctx->memory_injection) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * dim * sizeof(float);

        id<MTLBuffer> output_buf = [ctx->device newBufferWithBytes:output
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];
        id<MTLBuffer> memory_buf = [ctx->device newBufferWithBytes:memory_output
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->memory_injection];
        [enc setBuffer:output_buf offset:0 atIndex:0];
        [enc setBuffer:memory_buf offset:0 atIndex:1];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:2];
        [enc setBytes:&dim length:sizeof(int) atIndex:3];
        [enc setBytes:&alpha length:sizeof(float) atIndex:4];
        // Buffer indices: 0=output, 1=memory_output, 2=seq_len, 3=dim, 4=alpha

        MTLSize grid = MTLSizeMake(dim, seq_len, 1);
        MTLSize threadgroup = MTLSizeMake(
            MIN(16, dim),
            MIN(16, seq_len),
            1
        );

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(output, [output_buf contents], buffer_size);

        return 0;
    }
}

// ============================================================================
// Sparse Softmax
// ============================================================================

int zeta_metal_sparse_softmax(
    zeta_metal_ctx_t* ctx,
    float* scores,
    int seq_len,
    int kv_len,
    float min_attention
) {
    if (!ctx || !ctx->sparse_softmax) return -1;

    @autoreleasepool {
        size_t buffer_size = seq_len * kv_len * sizeof(float);

        id<MTLBuffer> scores_buf = [ctx->device newBufferWithBytes:scores
                                                            length:buffer_size
                                                           options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->sparse_softmax];
        [enc setBuffer:scores_buf offset:0 atIndex:0];
        [enc setBytes:&seq_len length:sizeof(int) atIndex:1];
        [enc setBytes:&kv_len length:sizeof(int) atIndex:2];
        [enc setBytes:&min_attention length:sizeof(float) atIndex:3];

        int tg_size = kv_len < 256 ? kv_len : 256;
        [enc setBytes:&tg_size length:sizeof(int) atIndex:4];

        NSUInteger shared_mem = tg_size * sizeof(float);
        [enc setThreadgroupMemoryLength:shared_mem atIndex:0];

        MTLSize threadgroup = MTLSizeMake(tg_size, 1, 1);

        [enc dispatchThreadgroups:MTLSizeMake(seq_len, 1, 1) threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(scores, [scores_buf contents], buffer_size);

        return 0;
    }
}

// ============================================================================
// Cosine Similarity
// ============================================================================

int zeta_metal_cosine_similarity(
    zeta_metal_ctx_t* ctx,
    const float* query,
    const float* summaries,
    float* similarities,
    int n_blocks,
    int dim
) {
    if (!ctx || !ctx->cosine_similarity) return -1;

    @autoreleasepool {
        id<MTLBuffer> query_buf = [ctx->device newBufferWithBytes:query
                                                           length:dim * sizeof(float)
                                                          options:MTLResourceStorageModeShared];
        id<MTLBuffer> summaries_buf = [ctx->device newBufferWithBytes:summaries
                                                               length:n_blocks * dim * sizeof(float)
                                                              options:MTLResourceStorageModeShared];
        id<MTLBuffer> sim_buf = [ctx->device newBufferWithLength:n_blocks * sizeof(float)
                                                         options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx->queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx->cosine_similarity];
        [enc setBuffer:query_buf offset:0 atIndex:0];
        [enc setBuffer:summaries_buf offset:0 atIndex:1];
        [enc setBuffer:sim_buf offset:0 atIndex:2];
        [enc setBytes:&n_blocks length:sizeof(int) atIndex:3];
        [enc setBytes:&dim length:sizeof(int) atIndex:4];

        int tg_size = dim < 256 ? dim : 256;
        [enc setBytes:&tg_size length:sizeof(int) atIndex:5];

        NSUInteger shared_mem = 3 * tg_size * sizeof(float);
        [enc setThreadgroupMemoryLength:shared_mem atIndex:0];

        [enc dispatchThreadgroups:MTLSizeMake(n_blocks, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(tg_size, 1, 1)];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted];

        memcpy(similarities, [sim_buf contents], n_blocks * sizeof(float));

        return 0;
    }
}


