# Examples

## Building with the wasi-sdk
C++ example:
```
clang++ -std=c++20 -I../include -o my.wasm main.cpp -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-table,,--import-memory,--export-memory,--strip-all
```

```
# from the project's root
cmake -Bbin -DCMAKE_TOOLCHAIN_FILE=/path/to/wasi-sdk/share/cmake/wasi-sdk.cmake -DEMLITE_BUILD_EXAMPLES=ON && cmake --build bin
```

## Building with stock clang
You can use wasi-libc if you're only building C sources, otherwise you can use wasi-sysroot for building C++ sources.

CMake can be used. It needs to be invoked while passing clang as your CC and CXX compiler, and you will need to point it to your sysroot (using an absolute path):
```
# from the project's root
CC=clang CXX=clang++ cmake -Bbin -DCMAKE_SYSROOT=/path/to/wasi-sysroot -DEMLITE_BUILD_EXAMPLES=ON && cmake --build bin
```

