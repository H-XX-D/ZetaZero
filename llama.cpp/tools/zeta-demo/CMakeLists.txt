# Z.E.T.A. Demo Tool
# Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

set(TARGET llama-zeta-demo)
add_executable(${TARGET} zeta-demo.cpp)

# Include Z.E.T.A. sources directly (simpler than separate library)
target_sources(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/zeta-memory.c
    ${CMAKE_SOURCE_DIR}/zeta-integration.c
    ${CMAKE_SOURCE_DIR}/zeta-kv-extract.c
    ${CMAKE_SOURCE_DIR}/zeta-constitution.c
    ${CMAKE_SOURCE_DIR}/zeta-model-bind.c
    ${CMAKE_SOURCE_DIR}/zeta-graph-kv.c
    ${CMAKE_SOURCE_DIR}/zeta-dedup.c
    ${CMAKE_SOURCE_DIR}/zeta-tunnel-search.c
    ${CMAKE_SOURCE_DIR}/zeta-version.c
)

target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_17)

# Metal support (macOS/iOS only)
if(APPLE)
    target_sources(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/zeta-metal.m)

    set_source_files_properties(${CMAKE_SOURCE_DIR}/zeta-metal.m PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    target_link_libraries(${TARGET} PRIVATE
        "-framework Metal"
        "-framework Foundation"
    )

    # Copy Metal kernels to build directory
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/zeta-kernels.metal
            $<TARGET_FILE_DIR:${TARGET}>/zeta-kernels.metal
        COMMENT "Copying Z.E.T.A. Metal kernels"
    )

    target_compile_definitions(${TARGET} PRIVATE ZETA_METAL_AVAILABLE=1)
else()
    target_compile_definitions(${TARGET} PRIVATE ZETA_METAL_AVAILABLE=0)
endif()

install(TARGETS ${TARGET} RUNTIME)

# Z.E.T.A. Server (HTTP API)
set(SERVER_TARGET zeta-server)
add_executable(${SERVER_TARGET} zeta-server.cpp)

target_sources(${SERVER_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/zeta-memory.c
    ${CMAKE_SOURCE_DIR}/zeta-integration.c
    ${CMAKE_SOURCE_DIR}/zeta-kv-extract.c
    ${CMAKE_SOURCE_DIR}/zeta-constitution.c
    ${CMAKE_SOURCE_DIR}/zeta-model-bind.c
    ${CMAKE_SOURCE_DIR}/zeta-graph-kv.c
    ${CMAKE_SOURCE_DIR}/zeta-dedup.c
    ${CMAKE_SOURCE_DIR}/zeta-tunnel-search.c
    ${CMAKE_SOURCE_DIR}/zeta-version.c
)

target_include_directories(${SERVER_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/cpp-httplib
)

target_link_libraries(${SERVER_TARGET} PRIVATE common llama cpp-httplib ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${SERVER_TARGET} PRIVATE cxx_std_17)

if(NOT APPLE)
    target_compile_definitions(${SERVER_TARGET} PRIVATE ZETA_METAL_AVAILABLE=0)
endif()

install(TARGETS ${SERVER_TARGET} RUNTIME)

# ============================================================================
# CUDA Constitutional Binding (optional)
# ============================================================================

if(GGML_CUDA)
    enable_language(CUDA)

    # Add CUDA sources to both targets
    target_sources(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/zeta-constitution-cuda.cu
        ${CMAKE_SOURCE_DIR}/zeta-constitution-bridge.c
    )

    target_sources(${SERVER_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/zeta-constitution-cuda.cu
        ${CMAKE_SOURCE_DIR}/zeta-constitution-bridge.c
    )

    # Set CUDA properties
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/zeta-constitution-cuda.cu
        PROPERTIES LANGUAGE CUDA
    )

    # Include CUDA directories
    target_include_directories(${TARGET} PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
    target_include_directories(${SERVER_TARGET} PRIVATE ${CUDAToolkit_INCLUDE_DIRS})

    # Link CUDA runtime
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(${TARGET} PRIVATE CUDA::cudart)
    target_link_libraries(${SERVER_TARGET} PRIVATE CUDA::cudart)

    # Define CUDA availability
    target_compile_definitions(${TARGET} PRIVATE GGML_USE_CUDA=1)
    target_compile_definitions(${SERVER_TARGET} PRIVATE GGML_USE_CUDA=1)

    message(STATUS "[Z.E.T.A.] CUDA constitutional binding enabled")
else()
    # CPU-only bridge (no CUDA)
    target_sources(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/zeta-constitution-bridge.c
    )
    target_sources(${SERVER_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/zeta-constitution-bridge.c
    )

    message(STATUS "[Z.E.T.A.] CUDA not available, using CPU constitutional binding")
endif()
