// initialize necessary env variables: WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT
// export WASI_SDK=~/wasi-sdk-25.0-x86_64-linux
// export WASI_LIBC=~/dev/wasi-libc/sysroot # or $MINGW_PREFIX/share/wasi-sysroot or /usr depending on the pkg manager
// export WASI_SYSROOT=~/dev/wasi-sysroot-25.0
// export EMSCRIPTEN_ROOT=~/emsdk/upstream/emscripten

import { execSync } from "child_process";
import { join } from "path";

function run(cmd) {
  console.log(`\x1b[36m$ ${cmd}\x1b[0m`);
  execSync(cmd, { stdio: "inherit", shell: true });
}

function buildSet(label, binDir, toolchain) {
  console.log(`\n\u25B6  Building for ${label} …`);
  let cmd = [
    "cmake",
    `-B${binDir}`,
    "-GNinja",
    "-DEMLITE_BUILD_EXAMPLES=ON",
    "-DCMAKE_BUILD_TYPE=MinSizeRel",
    `-DCMAKE_TOOLCHAIN_FILE=${toolchain}`,
  ];
  if (label === "FREESTANDING_WITH_DLMALLOC")
    cmd.push("-DEMLITE_USE_DLMALLOC=ON");
  if (label === "EMSCRIPTEN_STANDALONE")
    cmd.push("-DEMSCRIPTEN_STANDALONE_WASM=ON");
  run(cmd.join(" "));
  run(`cmake --build ${binDir}`);
}

async function main() {
  try {
    // 1- Freestanding, no dlmalloc (stock clang, wasm32-unknown-unknown)
    buildSet("FREESTANDING", "bin/freestanding", "./cmake/freestanding.cmake");

    buildSet(
      "FREESTANDING_WITH_DLMALLOC",
      "bin/freestanding_dl",
      "./cmake/freestanding.cmake"
    );

    const { WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT } = process.env;

    // 2- WASI SDK
    if (WASI_SDK) {
      buildSet(
        "WASI_SDK",
        "bin/wasi_sdk",
        join(WASI_SDK, "share/cmake/wasi-sdk.cmake")
      );
    }

    // 3- WASI Sysroot
    if (WASI_SYSROOT) {
      buildSet(
        "WASI_SYSROOT",
        "bin/wasi_sysroot",
        "./cmake/wasi_sysroot.cmake"
      );
    }

    // 4- WASI libc
    if (WASI_LIBC) {
      buildSet("WASI_LIBC", "bin/wasi_libc", "./cmake/wasi_libc.cmake");
    }

    // 5- Emscripten
    if (EMSCRIPTEN_ROOT) {
      buildSet(
        "EMSCRIPTEN",
        "bin/emscripten_default",
        join(EMSCRIPTEN_ROOT, "cmake/Modules/Platform/Emscripten.cmake")
      );
    }

    // 6- Emscripten standalone
    if (EMSCRIPTEN_ROOT) {
      buildSet(
        "EMSCRIPTEN_STANDALONE",
        "bin/emscripten_standalone",
        join(EMSCRIPTEN_ROOT, "cmake/Modules/Platform/Emscripten.cmake")
      );
    }

    console.log(
      "\n\x1b[32m✔ All requested builds completed successfully.\x1b[0m"
    );
  } catch (err) {
    console.error("\n\x1b[31m✖ CMake build failed:\x1b[0m", err.message);
    process.exit(err.status || 1);
  }
}

await main();
