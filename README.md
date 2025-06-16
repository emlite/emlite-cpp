# Emlite
Emlite is a tiny JS bridge for native code (C/C++/Rust/Zig) via Wasm, which is agnostic of the underlying toolchain. Thus it can target wasm32-unknown-unknown (freestanding, via stock clang), wasm32-wasi (via wasi-libc or wasi-sysroot), wasm32-wasip1 (via wasi-sdk or wasi-sysroot) and emscripten. 
It provides a header only library and a single javascript file that allows plain C or C++ code — compiled for wasm — to interoperate with javascript (including the DOM) and other JavaScript objects/runtimes without writing much JS “glue.”
It provides both a C api and a higher level C++ api similar to emscripten's val api. The repo also provides higher-level Rust and Zig bindings to emlite.

## Requirements
To use the C++ api, you need a C++-20 capable compiler. 
To use the C api, a C99 capable compiler should be sufficient.

## Since emscripten exists, why would I want to use wasm32-wasi or wasm32-wasip1?
- Emscripten is a large install (around 1.4 gb), and bundles clang, python, node and java.
- In contrast, if you already have clang installed, wasi-libc's sysroot is around 2.4mb if you're only using Emlite's C api.
- The wasi-sysroot/wasm32-wasi is only 44mb (headers and libraries).
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

To quickly try out emlite in the browser, create an index.html file:
(Note this is not the recommended way to deploy. You should install the required dependencies via npm and use a bundler like webpack to handle bundling, minifying, tree-shaking ...etc).
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
</head>
<body>
    <script type="module">
        import { WASI, File, OpenFile, ConsoleStdout } from "https://unpkg.com/@bjorn3/browser_wasi_shim";
        import { Emlite } from "https://unpkg.com/emlite";
        // or (if you decide to vendor emlite.js)
        // import { Emlite } from "./src/emlite.js";

        window.onload = async () => {
            let fds = [
                new OpenFile(new File([])), // 0, stdin
                ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)), // 1, stdout
                ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)), // 2, stderr
            ];
            let wasi = new WASI([], [], fds);
            let emlite = new Emlite();
            let wasm = await WebAssembly.compileStreaming(fetch("./bin/mywasm.wasm"));
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
    </script>
</body>
</html>
```

The @bjorn3/browser_wasi_shim dependency is not required for freestanding builds:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
</head>
<body>
    <script type="module">
        import { Emlite } from "https://unpkg.com/emlite";
        // or (if you decide to vendor emlite.js)
        // import { Emlite } from "./src/emlite.js";

        window.onload = async () => {
            let emlite = new Emlite();
            let wasm = await WebAssembly.compileStreaming(fetch("./bin/mywasm.wasm"));
            let inst = await WebAssembly.instantiate(wasm, {
                "env": emlite.env,
            });
            emlite.setExports(inst.exports);
            // test our exported function `add` in tests/dom_test1.cpp works
            window.alert(inst.exports.add(1, 2));
        };
    </script>
</body>
</html>
```

## Deployment

### Using wasm32-unknown-unknow
#### In the browser
```javascript
import { Emlite } from "emlite";

async function main() {
    let emlite = new Emlite();
    let wasm = await WebAssembly.compileStreaming(fetch("./bin_freestanding/dom_test1_nostdlib.wasm"));
    let inst = await WebAssembly.instantiate(wasm, {
        env: emlite.env,
    });
    emlite.setExports(inst.exports);
    window.alert(inst.exports.add(1, 2));
}

await main();
```

#### With a javascript engine like nodejs
You can get emlite from npm:
```
npm install emlite
```

Then in your javascript file:
```javascript
import { Emlite } from "emlite";
import { readFile } from "node:fs/promises";

async function main() {
    const emlite = new Emlite();
    const wasm = await WebAssembly.compile(
        await readFile("./bin/console.wasm"),
    );
    const instance = await WebAssembly.instantiate(wasm, {
        env: emlite.env,
    });
    emlite.setExports(instance.exports);
    // if you have another exported function marked with EMLITE_USED, you can get it in the instance exports
    instance.exports.some_func();
}

await main();
```

### Using wasm32-wasi, wasm32-wasip1 or emscripten
#### In the browser
To use emlite with wasm32-wasi, wasm32-wasip1 or emscripten** in your web stack, you will need a wasi javascript polyfill, here we use @bjorn3/browser_wasi_shim (via unpkg) and we vendor the emlite.js file into our src directory (note that both can also be installed via npm):
```javascript
import { Emlite } from "emlite";
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";

async function main() => {
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
}

await main();
```
Note that the example uses unpkg for demonstration purposes, to get better results, use the npm package and a bundler.
Both @bjorn3/browser_wasi_shim and emlite can be installed via npm.
If installed via npm, they can be imported using:
```javascript
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";
import { Emlite } from "emlite";
```

** Note that this depends on emscripten's ability to create standalone wasm files, which will also require a wasi shim:
https://v8.dev/blog/emscripten-standalone-wasm

#### With a javascript engine like nodejs
You can get emlite from npm:
```
npm install emlite
```

Then in your javascript file:
```javascript
import { Emlite } from "emlite";
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

## Building
### Using CMake
You can use CMake's FetchContent to get this repo, otherwise you can just copy the header files into your project.

To build with the wasi-sdk or emscripten, it's sufficient to pass the necessary toolchain file:
```bash
cmake -Bbin -GNinja -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN_ROOT/cmake/Modules/Platform/Emscripten.cmake && cmake --build bin
# or
cmake -Bbin -GNinja -DCMAKE_TOOLCHAIN_FILE=$WASI_SDK/share/cmake/wasi-sdk.cmake && cmake --build bin
# You would have to set $EMSCRIPTEN_ROOT or $WASI_SDK accordingly
```

To build using cmake for freestanding or with wasi-libc or wasi-sysroot, it's preferable to create a cmake toolchain file and pass that to your invocation:
```
cmake -Bbin -GNinja -DCMAME_TOOLCHAIN_FILE=./my_toolchain_file.cmake
```

The contents of your toolchain file should be adjust according to your needs. Please check the cmake directory of this repo for examples.

Note that there are certain flags which must be passed to wasm-ld in your CMakeLists.txt file:
```cmake
set_target_properties(mytarget PROPERTIES LINKER_LANGUAGE CXX SUFFIX .wasm LINK_FLAGS "-Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all")
```
Also check the CMakeLists.txt file in the repo to see how the examples and tests are built.

### Using clang bundled with wasi-sdk
- No need to pass a sysroot, nor a target:
```
clang++ -Iinclude -o my.wasm main.cpp -Wl,--no-entry,--allow-undefined,--export-all,--import-memory,--export-memory,--strip-all
```

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

## Testing
```bash
git clone https://github.com/MoAlyousef/emlite
cd emlite
npm install
npm run test_node
npm run build_tests
npm run serve
```

build_tests builds by default for freestanding.
It will also build for wasi-libc, wasi-sysroot, wasi-sdk, and emscripten if the necessary environment variables are set:
- WASI_LIBC
- WASI_SYSROOT
- WASI_SDK
- EMSCRIPTEN_ROOT

The build_tests script also genererates the necessary javascript glue code, runs webpack and creates the html files for testing. Each build directory should have an index.html file which has links to the rest of the files.
Running wasm code requires starting a server, which can be done using npm run serve.

## TODO
- Support binding C/C++ classes to js.
- Support ownership.