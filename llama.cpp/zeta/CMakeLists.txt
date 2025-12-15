# Z.E.T.A. Memory Extension for llama.cpp
# Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.

add_library(zeta STATIC
    ../zeta-memory.c
    ../zeta-memory.h
    ../zeta-integration.c
    ../zeta-integration.h
    ../zeta-kv-extract.c
    ../zeta-kv-extract.h
    ../zeta-constitution.c
    ../zeta-constitution.h
    ../zeta-model-bind.c
    ../zeta-model-bind.h
    ../zeta-metal.h
)

target_include_directories(zeta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
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
    target_sources(zeta PRIVATE ../zeta-metal.m)

    # Enable Objective-C for Metal API
    set_source_files_properties(../zeta-metal.m PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    # Link Metal and Foundation frameworks
    target_link_libraries(zeta PRIVATE
        "-framework Metal"
        "-framework Foundation"
    )

    # Copy Metal kernels to build directory
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../zeta-kernels.metal
        ${CMAKE_BINARY_DIR}/bin/zeta-kernels.metal
        COPYONLY
    )

    target_compile_definitions(zeta PRIVATE ZETA_METAL_AVAILABLE=1)
else()
    # CPU-only stub for non-Apple platforms
    target_compile_definitions(zeta PRIVATE ZETA_METAL_AVAILABLE=0)
endif()
