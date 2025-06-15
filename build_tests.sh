# initialize necessary env variables: WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT

# with wasi sdk
cmake -Bbin_wasi_sdk -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=$WASI_SDK/share/cmake/wasi-sdk.cmake && cmake --build bin_wasi_sdk

# stock clang with wasm32-unknown-unknown target
cmake -Bbin_freestanding -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/freestanding.cmake && cmake --build bin_freestanding

# with wasi sysroot
cmake -Bbin_wasi_sysroot -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/wasi_sysroot.cmake && cmake --build bin_wasi_sysroot

# with wasi libc
cmake -Bbin_wasi_libc -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/wasi_libc.cmake && cmake --build bin_wasi_libc

# with emscripten
cmake -Bbin_emscripten -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN_ROOT/cmake/Modules/Platform/Emscripten.cmake && cmake --build bin_emscripten