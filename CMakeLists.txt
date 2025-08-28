cmake_minimum_required(VERSION 3.28)
project(emlite VERSION 0.1.4 LANGUAGES C CXX)

option(EMLITE_BUILD_EXAMPLES "Build examples" OFF)

add_library(emlite)

set(EMLITE_HEADERS)
set(EMLITE_SOURCES)

set(EMLITE_HEADERS
    include/emlite/emlite.hpp
    include/emlite/detail/func.hpp
    include/emlite/detail/mem.hpp
    include/emlite/detail/tiny_traits.hpp
    include/emlite/detail/utils.hpp
)
set(EMLITE_SOURCES
    src/emlite.cpp
)
target_compile_features(emlite PUBLIC cxx_std_17)
if (CMAKE_C_COMPILER_TARGET STREQUAL "wasm32-wasip2" OR CMAKE_CXX_COMPILER_TARGET STREQUAL "wasm32-wasip2")
  target_compile_definitions(emlite PUBLIC EMLITE_WASIP2)
endif()
set_target_properties(emlite PROPERTIES LINKER_LANGUAGE CXX)

target_sources(emlite 
    PRIVATE 
        ${EMLITE_SOURCES} 
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
        FILES ${EMLITE_HEADERS}
)

find_package(emcore CONFIG QUIET)
if(NOT emcore_FOUND)
    add_subdirectory(emcore)
endif()
target_link_libraries(emlite PUBLIC emcore::emcore)

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


