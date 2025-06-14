# Emlite

Emlite is a tiny JS bridge for native code (C/C++/Rust/Zig) via Wasm (wasi), which doesn't require the emscripten toolchain.
It provides a single C or C++ header and a single javascript file that allows plain C or C++ code — compiled with wasm32-wasi — to talk to interoperate with javascript (including the DOM) and other JavaScript objects without writing much JS “glue.”
It provides both a C api and a higher level C++ api similar to emscripten's val api.

## Requirements

To use the C++ api, you need a C++-20 capable compiler. The wasi-sdk bundles clang-19 which should cover your requirements.
To use the C api, a C99 capable compiler should be sufficient.

## Why wasm32-wasi instead of emscripten

- Emscripten is a large install (around 1.4 gb), and bundles clang, python, node and java.
- In contrast, if you already have clang installed, wasi-libc's sysroot is around 2.4mb if you're only using Emlite's C api.
- If you're using C++, the wasi-sysroot/wasm32-wasi is only 44mb (headers and libraries).
- Even if you install the wasi-sdk, it still is less than 1/4 the size of emscripten.
- Emscripten javascript glue is sometimes difficult to navigate when a problem occurs.

## Why emscripten instead of wasm32-wasi

- More established.
- Offers other bundled libraries like SDL, boost and other ports.
- Offers emscripten_sleep which allows better compatibilty for compiling sources containing event-loops like games.
- Automatic translation of OpenGL to WebGL.
- Offers more optimisations by bundling binaryen, wasm-opt and google's Closure compiler.

## Examples

C++ example:
```c++
// define EMLITE_IMPL in only one implementation unit (source file)!
#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

EMLITE_USED extern "C" void some_func() {
    Console().log(Val("Hello from Emlite"));

    auto doc  = Val::global("document");
    auto body = doc.call("getElementsByTagName", Val("body"))[0];
    auto btn  = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    body.call("appendChild", btn);
    btn.call("addEventListener", Val("click"), Val([](auto h) -> Handle {
                 Console().log(Val::from_handle(h));
                 return Val::undefined().as_handle();
             }));
}
```

C example:
```c
// define EMLITE_IMPL in only one implementation unit (source file)!
#define EMLITE_IMPL
#include <emlite/emlite.h>

EMLITE_USED int main() {
    em_Val console = em_Val_global("console");
    em_Val_call(console, "log", 1, em_Val_from_string("200"));
}
```

## Usage

### In the browser

If you target wasm32 (freestanding) with stock clang, you can import emlite using:
```javascript
import { Emlite } from "./src/emlite.js";

window.onload = async () => {
    let emlite = new Emlite();
    let wasm = await WebAssembly.compileStreaming(fetch("./dom_test2_nostdlib.wasm"));
    let inst = await WebAssembly.instantiate(wasm, {
        env: emlite.env,
    });
    emlite.setExports(inst.exports);
    window.alert(inst.exports.add(1, 2));
};
```

To use emlite with wasm32-wasi in your web stack, you will need a wasi javascript polyfill, here we use @bjorn3/browser_wasi_shim (via unpkg) and we vendor the emlite.js file into our src directory:
```javascript
// see the index.html for an example
import { WASI, File, OpenFile, ConsoleStdout } from "https://unpkg.com/@bjorn3/browser_wasi_shim";
import { Emlite } from "./src/emlite.js";

window.onload = async () => {
    let fds = [
        new OpenFile(new File([])), // 0, stdin
        ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)), // 1, stdout
        ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)), // 2, stderr
    ];
    let wasi = new WASI([], [], fds);
    let emlite = new Emlite();
    let wasm = await WebAssembly.compileStreaming(fetch("./bin/dom_test1.wasm"));
    let inst = await WebAssembly.instantiate(wasm, {
        "wasi_snapshot_preview1": wasi.wasiImport,
        "env": emlite.env,
    });
    emlite.setExports(inst.exports);
    // if your C/C++ has a main function, use: `wasi.start(inst)`. If not, use `wasi.initialize(inst)`.
    wasi.start(inst);
    // test our exported function `add` in tests/dom_test1.cpp works
    window.alert(inst.exports.add(1, 2));
};
```
Note that the example uses unpkg for demonstration purposes, to get better results, use the npm package and a bundler.
Both @bjorn3/browser_wasi_shim and emlite can be installed via npm.
If installed via npm, they can be imported using:
```javascript
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";
import { Emlite } from "emlite";
```

### With a javascript engine like nodejs

If you're vendoring the emlite.js file:
```javascript
import { Emlite } from "./src/emlite.js";
import { WASI } from "node:wasi";
import { readFile } from "node:fs/promises";
import { argv, env } from "node:process";

async function main() {
    const wasi = new WASI({
        version: 'preview1',
        args: argv,
        env,
    });
    
    const emlite = new Emlite();
    const wasm = await WebAssembly.compile(
        await readFile("./bin/console.wasm"),
    );
    const instance = await WebAssembly.instantiate(wasm, {
        wasi_snapshot_preview1: wasi.wasiImport,
        env: emlite.env,
    });
    wasi.start(instance);
    emlite.setExports(instance.exports);
    // if you have another exported function marked with EMLITE_USED, you can get it in the instance exports
    instance.exports.some_func();
}

await main();
```
Note that nodejs as of version 22.16 requires a _start function in the wasm module. That can be achieved by defining an `int main() {}` function. It's also why we use `wasi.start(instance)` in the js module.

You can also install emlite via npm `npm install emlite` and import it using:
```javascript
import { Emlite } from "emlite";
```

## Building

### Using clang bundled with wasi-sdk

- No need to pass a sysroot, nor a target:
```
clang++ -Iinclude -o my.wasm main.cpp -Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all
```
To use CMake, you can pass the wasi-sdk.cmake to CMake via the `-DCMAKE_TOOLCHAIN_FILE=/path/to/wasi-sdk/share/cmake/wasi-sdk.cmake`.

### Using stock clang

- clang capable of targeting wasm32-wasi is required. 
If you installed your clang via a package manager, you might require an extra package like libclang-rt-dev-wasm32 (note that it should match the version of your clang install, i.e libclang-rt-18-dev-wasm32). 
Additionally you might require lld to get wasm-ld. Similarly, it should match your clang version.
- If only using C, you can get the wasi-libc sources from the [wasi-libc](https://github.com/WebAssembly/wasi-libc) repo. This will require compiling the source using the given instructions. There are also packages for debian/ubuntu, arch linux, and msys2.
- If using C++ as well, you can grab the wasi-sysroot from the [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases) releases page.

To compile, you'll need to tell clang to target wasm32-wasi, and point it to the sysroot you require:
```
clang++ --target=wasm32-wasi -Iinclude -o my.wasm main.cpp --sysroot /path/to/wasi-sysroot -Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all
```

To use CMake, you can configure the target and sysroot using:
```cmake
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(triple wasm32-wasi)

set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
```

You can also instruct CMake to generate a wasm file, and apply the necessary link flags:
```cmake
add_executable(mytarget main.cpp)
target_include_directories(cppexample PRIVATE path/to/include/dir)
set_target_properties(mytarget PROPERTIES SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")
```

Then when invoking cmake, set the CC and/or CXX environment variables to point to clang, or set the CMAKE_C_COMPILER/CMAKE_CXX_COMPILER via the command line or in your CMakeLists.txt:
```
CC=clang CXX=clang++ cmake -Bbin -DCMAKE_SYSROOT=/path/to/wasi-sysroot && cmake --build bin --parallel
```

## Testing

You can build the current test using:
```
cmake -Bbin -DCMAKE_TOOLCHAIN_FILE=~/wasi-sdk-25.0-x86_64-macos/share/cmake/wasi-sdk.cmake -DEMLITE_BUILD_TESTS=ON -DEMLITE_BUILD_EXAMPLES=ON && cmake --build bin
```
The index.html file in the root of the repo is there to run the tests.
Starting a server is required to run wasm code in the browser, this can be done using any of the lightweight server apps available, or if you have python, you can run `python3 -m http.server`

## TODO

- Support binding C/C++ classes to js.
- Support ownership.