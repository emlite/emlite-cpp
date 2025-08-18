cmake_minimum_required(VERSION 3.20)
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

if(EMSCRIPTEN AND NOT EMSCRIPTEN_STANDALONE_WASM)
    add_subdirectory(emlite_emscripten_adapter)
endif()

target_sources(emlite 
    PRIVATE 
        ${EMLITE_SOURCES} 
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
        FILES ${EMLITE_HEADERS}
)

set(DLMALLOC_LIB)
if (EMLITE_USE_DLMALLOC)
    add_library(dlmalloc src/dlmalloc.c)
    target_compile_definitions(dlmalloc PUBLIC 
        LACKS_UNISTD_H
        LACKS_FCNTL_H
        LACKS_SYS_PARAM_H
        LACKS_SYS_MMAN_H
        LACKS_STRINGS_H
        LACKS_STRING_H
        LACKS_SYS_TYPES_H
        LACKS_ERRNO_H
        LACKS_STDLIB_H
        LACKS_SCHED_H
        LACKS_TIME_H
        NO_MALLOC_STATS=1
        HAVE_MMAP=0
        HAVE_MORECORE=1
        MORECORE=sbrk
        MORECORE_CANNOT_TRIM=1
        MORECORE_CONTIGUOUS=1
        NO_MALLINFO=1
    )
    target_compile_definitions(emlite PUBLIC HAVE_DLMALLOC)
    target_link_libraries(emlite PUBLIC dlmalloc)
    set(DLMALLOC_LIB dlmalloc)
endif()

add_library(emlite::emlite ALIAS emlite)

include(GNUInstallDirs)

install(TARGETS emlite ${DLMALLOC_LIB} EXPORT emliteTargets FILE_SET  HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT emliteTargets
    FILE emliteTargets.cmake
    NAMESPACE emlite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/emlite
)

if (EMLITE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

