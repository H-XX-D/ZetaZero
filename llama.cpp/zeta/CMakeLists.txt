# Z.E.T.A. Memory Extension for llama.cpp
# Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.
# NOTE: All zeta sources have been consolidated into tools/zeta-zero/

set(ZETA_DIR ${CMAKE_SOURCE_DIR}/tools/zeta-zero)

add_library(zeta STATIC
    ${ZETA_DIR}/zeta-memory.c
    ${ZETA_DIR}/zeta-memory.h
    ${ZETA_DIR}/zeta-integration.c
    ${ZETA_DIR}/zeta-integration.h
    ${ZETA_DIR}/zeta-kv-extract.c
    ${ZETA_DIR}/zeta-kv-extract.h
    ${ZETA_DIR}/zeta-constitution.c
    ${ZETA_DIR}/zeta-constitution.h
    ${ZETA_DIR}/zeta-model-bind.c
    ${ZETA_DIR}/zeta-model-bind.h
    ${ZETA_DIR}/zeta-metal.h
)

target_include_directories(zeta PUBLIC ${ZETA_DIR})
target_include_directories(zeta PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link against llama for type definitions
target_link_libraries(zeta PRIVATE llama)

# Math library
if(NOT MSVC)
    target_link_libraries(zeta PRIVATE m)
endif()

# Metal support (macOS/iOS only)
if(APPLE)
    # Add Objective-C Metal implementation
    target_sources(zeta PRIVATE ${ZETA_DIR}/zeta-metal.m)

    # Enable Objective-C for Metal API
    set_source_files_properties(${ZETA_DIR}/zeta-metal.m PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    # Link Metal and Foundation frameworks
    target_link_libraries(zeta PRIVATE
        "-framework Metal"
        "-framework Foundation"
    )

    # Copy Metal kernels to build directory
    configure_file(
        ${ZETA_DIR}/zeta-kernels.metal
        ${CMAKE_BINARY_DIR}/bin/zeta-kernels.metal
        COPYONLY
    )

    target_compile_definitions(zeta PRIVATE ZETA_METAL_AVAILABLE=1)
else()
    # CPU-only stub for non-Apple platforms
    target_compile_definitions(zeta PRIVATE ZETA_METAL_AVAILABLE=0)
endif()
