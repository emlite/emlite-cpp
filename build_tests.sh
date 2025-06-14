# with wasi sdk
cmake -Bbin_wasi_sdk -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=~/wasi-sdk-25.0-x86_64-linux/share/cmake/wasi-sdk.cmake && cmake --build bin_wasi_sdk

# stock clang with wasm32-unknown-unknown target
cmake -Bbin_freestanding -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER_TARGET=wasm32 -DCMAKE_CXX_COMPILER_TARGET=wasm32 -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_SYSTEM_PROCESSOR=wasm32 && cmake --build bin_freestanding

# with wasi sysroot
cmake -Bbin_wasi_sysroot -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER_TARGET=wasm32-wasi -DCMAKE_CXX_COMPILER_TARGET=wasm32-wasi -DCMAKE_SYSROOT="/home/ray/dev/wasi-sysroot-25.0" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_SYSTEM_PROCESSOR=wasm32 && cmake --build bin_wasi_sysroot

# with wasi libc
cmake -Bbin_wasi_libc -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER_TARGET=wasm32-wasi -DCMAKE_CXX_COMPILER_TARGET=wasm32-wasi -DCMAKE_SYSROOT="/home/ray/dev/wasi-libc/sysroot" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_SYSTEM_PROCESSOR=wasm32 && cmake --build bin_wasi_libc