# Emscripten interop

Emlite can be used with emscripten whether in [standalone_wasm mode](https://github.com/emscripten-core/emscripten/wiki/WebAssembly-Standalone) or the emscripten default mode (js glue code with or without an html shell).

For use with standalone_wasm mode, you will require a wasi shim/polyfill as described in the main [README.md](../README.md). You will also need to export the _start symbol in your link flags:
```cmake
set(${DEFAULT_LINK_FLAGS} "-sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1 -sLINKABLE=1 -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-if-defined=_start,--export-table,,--export-memory,--strip-all")
```

For use with the default mode, you will require passing an extra defintion to emlite: EMLITE_USE_EMSCRIPTEN_JS_GLUE, and a tweak in the link flags:
```
-sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1 -sLINKABLE=1 -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-table,,--export-memory,--strip-all
```
Notice how we remove the `--import-memory` link flag. To load in the browser where you depend on automatic loading of the js glue code, you can add a script tag which just imports and initializes Emlite before the `<script>` tag which loads the emscripten-generated js glue code:
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
        import { Emlite } from "./src/emlite.js";
        const emlite = new Emlite();
    </script>
    <script async src="./bin/mywasms.js"></script>
</body>
</html>
```

If you rely on emscripten to generate an ES6 module, you can pass emscripten the `-sMODULARIZE` and `-sEXPORT_ES6=1` flags. Or you can instruct emscripten to generate an `.mjs` file in which case it will automatically generate an ES6 module. This can be done in the command-line by specifying the name of the output file, or using CMake:
```cmake
set(${DEFAULT_LINK_FLAGS} "-sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1 -sLINKABLE=1 -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-if-defined=_start,--export-table,,--export-memory,--strip-all")

add_executable(main src/main.cpp)
target_compile_definitions(main PRIVATE EMLITE_USE_EMSCRIPTEN_JS_GLUE)
target_link_libraries(main PRIVATE emlite::emlite)
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX SUFFIX .mjs LINK_FLAGS ${DEFAULT_LINK_FLAGS})
```
Notice how we set the SUFFIX property to `.mjs`.

This can then be imported in your browser for example using:
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
        import { Emlite } from "./src/emlite.js";
        // main.mjs is generated from emscripten
        import initModule from "./bin/main.mjs";
        window.onload = async () => {
            const emlite = new Emlite();
            const mymain = await initModule();
        };
    </script>
</body>
</html>
```

If you generate html files using Emscripten. You just need to change the SUFFIX property to .html and supply a shell html file in your link args:
```
set(${DEFAULT_LINK_FLAGS} "-sERROR_ON_UNDEFINED_SYMBOLS=0 -sALLOW_MEMORY_GROWTH=1 -sLINKABLE=1 -Wl,--no-entry,--allow-undefined,--export-dynamic,--export-if-defined=main,--export-table,,--export-memory,--strip-all --shell-file ${CMAKE_CURRENT_LIST_DIR}/my_shell.html")
```

You can create a shell my_shell.html file:
```html
<!doctype html>
<html lang="en-us">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
  </head>
  <body>
    <script type="module">
        import { Emlite } from "./src/emlite.js";
        new Emlite();
    </script>
  {{{ SCRIPT }}}
  </body>
</html>
```