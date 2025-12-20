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

# Z.E.T.A. Server (HTTP daemon with full ZETA features)
set(SERVER_TARGET llama-zeta-server)
add_executable(${SERVER_TARGET} zeta-server.cpp)

target_sources(${SERVER_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/zeta-memory.c
    ${CMAKE_SOURCE_DIR}/zeta-integration.c
    ${CMAKE_SOURCE_DIR}/zeta-kv-extract.c
    ${CMAKE_SOURCE_DIR}/zeta-constitution.c
    ${CMAKE_SOURCE_DIR}/zeta-model-bind.c
)

target_include_directories(${SERVER_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/cpp-httplib
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${SERVER_TARGET} PRIVATE common llama cpp-httplib ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${SERVER_TARGET} PRIVATE cxx_std_17)

if(APPLE)
    target_sources(${SERVER_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/zeta-metal.m)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/zeta-metal.m PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(${SERVER_TARGET} PRIVATE "-framework Metal" "-framework Foundation")
    target_compile_definitions(${SERVER_TARGET} PRIVATE ZETA_METAL_AVAILABLE=1)
else()
    target_compile_definitions(${SERVER_TARGET} PRIVATE ZETA_METAL_AVAILABLE=0)
endif()
