cmake_minimum_required(VERSION 3.28)
project(emlite VERSION 0.1.0 LANGUAGES C CXX)

option(EMLITE_BUILD_EXAMPLES "Build examples" OFF)
option(EMLITE_USE_DLMALLOC "Use dlmalloc for freestanding environments" OFF)
option(EMLITE_CLIB_ONLY "Build only the emlite C library" OFF)
option(EMSCRIPTEN_STANDALONE_WASM "Use emscripten's default js output" OFF)

add_library(emlite)

set(EMLITE_HEADERS)
set(EMLITE_SOURCES)

if (EMLITE_CLIB_ONLY)
    set(EMLITE_HEADERS
        include/emlite/emlite.h
        include/emlite/detail/externs.h
    )
    set(EMLITE_SOURCES
        src/emlite.c
    )
    target_compile_features(emlite PUBLIC c_std_11)
    set_target_properties(emlite PROPERTIES LINKER_LANGUAGE C)
else()
    set(EMLITE_HEADERS
        include/emlite/emlite.h
        include/emlite/emlite.hpp
        include/emlite/detail/externs.h
        include/emlite/detail/func.hpp
        include/emlite/detail/mem.hpp
        include/emlite/detail/tiny_traits.hpp
        include/emlite/detail/utils.hpp
    )
    set(EMLITE_SOURCES
        src/emlite.c
        src/emlite.cpp
    )
    target_compile_features(emlite PUBLIC cxx_std_17)
    set_target_properties(emlite PROPERTIES LINKER_LANGUAGE CXX)
endif()

target_sources(emlite 
    PRIVATE 
        ${EMLITE_SOURCES} 
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
        FILES ${EMLITE_HEADERS}
)

if(EMSCRIPTEN AND NOT EMSCRIPTEN_STANDALONE_WASM)
    find_package(ems_env QUIET)
    if (NOT ems_env_FOUND)
        add_subdirectory(emlite_emscripten_adapter)
    endif()
    target_link_libraries(emlite PRIVATE ems_env::ems_env)
endif()

if (EMLITE_USE_DLMALLOC)
    find_package(dlmalloc QUIET)
    if(NOT dlmalloc_FOUND)
        add_subdirectory(dlmalloc)
    endif()
    target_compile_definitions(emlite PUBLIC HAVE_DLMALLOC)
    target_link_libraries(emlite PRIVATE dlmalloc)
endif()

add_library(emlite::emlite ALIAS emlite)

if (EMLITE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)

set(emlite_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/emlite")

install(TARGETS emlite EXPORT emliteTargets FILE_SET  HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT emliteTargets
    FILE emliteTargets.cmake
    NAMESPACE emlite::
    DESTINATION ${emlite_INSTALL_CMAKEDIR}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/emliteConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/emliteConfig.cmake"
  INSTALL_DESTINATION "${emlite_INSTALL_CMAKEDIR}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/emliteConfigVersion.cmake"
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/emliteConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/emliteConfigVersion.cmake"
  DESTINATION "${emlite_INSTALL_CMAKEDIR}"
)


