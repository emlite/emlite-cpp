cmake_minimum_required(VERSION 3.20)
project(emlite VERSION 0.1.0)

option(EMLITE_BUILD_EXAMPLES "Build examples" OFF)
option(EMLITE_BUILD_TESTS "Build tests" OFF)

add_library(emlite INTERFACE)

target_include_directories(emlite INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(emlite INTERFACE cxx_std_20)
add_library(emlite::emlite ALIAS emlite)

include(GNUInstallDirs)

install(TARGETS emlite EXPORT emliteTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT emliteTargets
    FILE emliteTargets.cmake
    NAMESPACE emlite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/emlite
)

if (EMLITE_BUILD_EXAMPLES OR EMLITE_BUILD_TESTS)
    if(NOT WASI_SDK_PREFIX)
        set(CMAKE_SYSTEM_NAME WASI)
        set(CMAKE_SYSTEM_VERSION 1)
        set(CMAKE_SYSTEM_PROCESSOR wasm32)
        set(triple wasm32-wasi)

        set(CMAKE_C_COMPILER_TARGET ${triple})
        set(CMAKE_CXX_COMPILER_TARGET ${triple})
        set(CMAKE_ASM_COMPILER_TARGET ${triple})
    endif()
endif()

if (EMLITE_BUILD_EXAMPLES)
    add_executable(console examples/console.c)
    target_link_libraries(console PRIVATE emlite::emlite)
    set_target_properties(console PROPERTIES LINKER_LANGUAGE C SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")
    
    add_executable(eval examples/eval.cpp)
    target_link_libraries(eval PRIVATE emlite::emlite)
    set_target_properties(eval PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")

    add_executable(dom_simple examples/dom_simple.cpp)
    target_link_libraries(dom_simple PRIVATE emlite::emlite)
    set_target_properties(dom_simple PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")

    add_executable(audio examples/audio.cpp)
    target_link_libraries(audio PRIVATE emlite::emlite)
    set_target_properties(audio PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")

    add_executable(node_readfile examples/node_readfile.cpp)
    target_link_libraries(node_readfile PRIVATE emlite::emlite)
    set_target_properties(node_readfile PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")
endif()

if (EMLITE_BUILD_TESTS)
    add_executable(dom_test1 tests/dom_test1.cpp)
    target_link_libraries(dom_test1 PRIVATE emlite::emlite)
    set_target_properties(dom_test1 PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")
endif()