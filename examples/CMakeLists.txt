set(DEFAULT_SUFFIX .wasm)
set(DEFAULT_LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-table,--import-memory,--export-memory,--strip-all")

if (EMSCRIPTEN)
    if (EMSCRIPTEN_STANDALONE_WASM)
        set(DEFAULT_LINK_FLAGS "${DEFAULT_LINK_FLAGS},--export-if-defined=_start -sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1")
    else()
        set(DEFAULT_LINK_FLAGS "-sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=wasmTable,UTF8ToString,lengthBytesUTF8,stringToUTF8 -sEXPORTED_FUNCTIONS=_malloc,_main -Wl,--strip-all")
        set(DEFAULT_SUFFIX .js)
    endif()
endif()

add_executable(console console.c)
target_link_libraries(console PRIVATE emlite::emlite)
set_target_properties(console PROPERTIES LINKER_LANGUAGE C SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

add_executable(cbind cbind.c)
target_link_libraries(cbind PRIVATE emlite::emlite)
set_target_properties(cbind PROPERTIES LINKER_LANGUAGE C SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

add_executable(eval eval.cpp)
target_link_libraries(eval PRIVATE emlite::emlite)
set_target_properties(eval PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

add_executable(dom_simple dom_simple.cpp)
target_link_libraries(dom_simple PRIVATE emlite::emlite)
set_target_properties(dom_simple PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

if (NOT USING_FREESTANDING)
    add_executable(audio audio.cpp)
    target_link_libraries(audio PRIVATE emlite::emlite)
    set_target_properties(audio PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

    add_executable(node_readfile node_readfile.cpp)
    target_link_libraries(node_readfile PRIVATE emlite::emlite)
    set_target_properties(node_readfile PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

    add_executable(bind bind.cpp)
    target_link_libraries(bind PRIVATE emlite::emlite)
    set_target_properties(bind PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})
endif()


if (NOT USING_FREESTANDING)
    if (NOT USING_WASI_LIBC)
        add_executable(dom_test1 dom_test1.cpp)
        target_link_libraries(dom_test1 PRIVATE emlite::emlite)
        set_target_properties(dom_test1 PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})
    endif()
    add_executable(dom_test2 dom_test2.c)
    target_link_libraries(dom_test2 PRIVATE emlite::emlite)
    set_target_properties(dom_test2 PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})
endif()
add_executable(dom_test1_nostdlib dom_test1_nostdlib.cpp)
target_link_libraries(dom_test1_nostdlib PRIVATE emlite::emlite)
set_target_properties(dom_test1_nostdlib PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})

add_executable(dom_test2_nostdlib dom_test2_nostdlib.c)
target_link_libraries(dom_test2_nostdlib PRIVATE emlite::emlite)
set_target_properties(dom_test2_nostdlib PROPERTIES LINKER_LANGUAGE CXX SUFFIX ${DEFAULT_SUFFIX} LINK_FLAGS ${DEFAULT_LINK_FLAGS})