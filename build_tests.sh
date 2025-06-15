# # initialize necessary env variables: WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT
# export WASI_SDK=~/wasi-sdk-25.0-x86_64-linux
# export WASI_LIBC=~/dev/wasi-libc
# export WASI_SYSROOT=~/dev/wasi-sysroot-25.0
# export EMSCRIPTEN_ROOT=~/emsdk/upstream/emscripten

# stock clang with wasm32-unknown-unknown target
cmake -Bbin_freestanding -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON \
 -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=./cmake/freestanding.cmake && \
 cmake --build bin_freestanding 

# with wasi sdk
[ -n "${WASI_SDK:-}" ] && \
 echo "Building for WASI_SDK" && \
 cmake -Bbin_wasi_sdk -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=$WASI_SDK/share/cmake/wasi-sdk.cmake && \
 cmake --build bin_wasi_sdk 

# with wasi sysroot
[ -n "${WASI_SYSROOT:-}" ] && \
 echo "Building for WASI_SYSROOT" && \
 cmake -Bbin_wasi_sysroot -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=./cmake/wasi_sysroot.cmake && \
 cmake --build bin_wasi_sysroot 

# with wasi libc
[ -n "${WASI_SYSROOT:-}" ] && \
 echo "Building for WASI_SYSROOT" && \
 cmake -Bbin_wasi_libc -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=./cmake/wasi_libc.cmake && \
 cmake --build bin_wasi_libc 

# with emscripten
[ -n "${EMSCRIPTEN_ROOT:-}" ] && \
 echo "Building for EMSCRIPTEN_ROOT" && \
 cmake -Bbin_emscripten -GNinja -DEMLITE_BUILD_EXAMPLES=ON -DEMLITE_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN_ROOT/cmake/Modules/Platform/Emscripten.cmake && \
 cmake --build bin_emscripten 