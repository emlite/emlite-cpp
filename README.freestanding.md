# Freestanding

clang by default supports targeting wasm32-unknown-unknown by passing the --target=wasm32 flag (or the complete triple if you prefer).
It provides several freestanding C headers like stddef, stdint, stdarg, stdbool and limits. As such you will also have to provide the `-nostdlib` flag to your linker as well, to inform it not to look for libc.

This library vendors dlmalloc for optional use with a CMake option EMLITE_USE_DLMALLOC. dlmalloc is a small allocator library which is also used by emscripten and Rust's wasm32-unknown-unknown target.

If you prefer not to use it, emlite provides a simple bump allocator implementation for malloc.

If you use CMake to fetch emlite, you can enable dlmalloc using:
```cmake
cmake_minimum_required(VERSION 3.20)
project(projname)

include(FetchContent)

FetchContent_Declare(
    emlite
    GIT_REPOSITORY https://github.com/emlite/emlite-cpp.git
    GIT_TAG main
    GIT_SHALLOW True
)

set(EMLITE_USE_DLMALLOC ON)
FetchContent_MakeAvailable(emlite)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE emlite::emlite)
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-nostdlib -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-table,,--import-memory,--export-memory,--strip-all")
```

It's advisable to use a cmake toolchain file since you're technically cross-compiling. A simple one as the following should do the job:
```cmake
# freestanding.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_COMPILER_TARGET wasm32)
set(CMAKE_CXX_COMPILER_TARGET wasm32)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

set(USING_FREESTANDING TRUE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

You can then build with clang and CMake:
```
cmake -Bbin -GNinja -DCMAKE_TOOLCHAIN_FILE=freestanding.cmake
```

Loading the resulting wasm doesn't require node's WASI nor a wasi shim for the browser:
```javascript
// node
import { Emlite } from "emlite";

async function main() {
    const emlite = new Emlite();
    const url = new URL("./bin/console.wasm", import.meta.url);
    const bytes = await emlite.readFile(url);
    let wasm = await WebAssembly.compile(bytes);
    let instance = await WebAssembly.instantiate(wasm, {
        env: emlite.env,
    });
    emlite.setExports(instance.exports);
    instance.exports.main?.();
    // if you have another exported function marked with EMLITE_USED, you can get it in the instance exports
}

await main();
```

