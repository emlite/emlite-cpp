# Publishing to npmjs

Once you have a node project which uses emlite, you might want to publish to npmjs.
Some npmjs packages can be used for the browser (via webpack or other bundlers) or for node or node-like environments like deno or bun, or both. So you might want to avoid code that's specific to node.

The following guide assumes you're targeting wasm32-wasi, but can be modified depending on your target.

Assuming you already have a node project, with the following index.js file:
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

Now you want to export some_func() and some_other_func() from your C++ wasm binary.

You can change the code to the following (if you want it to be used in the browser as well):
```javascript
import { Emlite } from "emlite";

let WASI;                
let exportsPromise;       

export function envIsBrowser() {
  return typeof window !== "undefined" && "document" in window;
}

async function init() {
  if (!exportsPromise) {
    exportsPromise = (async () => {

      if (!WASI) {
        ({ WASI } = envIsBrowser()
          ? await import("@bjorn3/browser_wasi_shim")
          : await import("node:wasi"));
      }

      const emlite = new Emlite();
      const wasi = envIsBrowser()
        ? new WASI([], [], [
            null,
            WASI.ConsoleStdout.lineBuffered(console.log),
            WASI.ConsoleStdout.lineBuffered(console.error),
          ])
        : new WASI({ version: "preview1", args: process.argv, env: process.env });

      const url = new URL("./bin/console.wasm", import.meta.url);
      let instance;
      try {
        ({ instance } = await WebAssembly.instantiateStreaming(fetch(url), {
          wasi_snapshot_preview1: wasi.wasiImport,
          env: emlite.env,
        }));
      } catch {
        const { readFile } = await import("node:fs/promises");
        const bytes = await readFile(url);
        ({ instance } = await WebAssembly.instantiate(bytes, {
          wasi_snapshot_preview1: wasi.wasiImport,
          env: emlite.env,
        }));
      }

      wasi.start(instance);
      emlite.setExports(instance.exports);
      return instance.exports;
    })();
  }
  return exportsPromise;
}

export async function some_func(...args) {
  return (await init()).some_func(...args);
}

export async function some_other_func(...args) {
  return (await init()).some_other_func(...args);
}
```

Or you can use a double file entry approach, one for node and one for browsers by specifying the `exports` entry in your package.json:
```json
"exports": {
  ".": {
    "browser": "./src/index-browser.js",
    "node":    "./src/index-node.js",
  }
}
```

For freestanding environments, a single entry should be sufficient:
```javascript
import { Emlite } from "emlite";
          
let exportsPromise;       

async function init() {
  if (!exportsPromise) {
    exportsPromise = (async () => {

      const emlite = new Emlite();

      const url = new URL("./bin/console.wasm", import.meta.url);
      let instance;
      try {
        ({ instance } = await WebAssembly.instantiateStreaming(fetch(url), {
          env: emlite.env,
        }));
      } catch {
        const { readFile } = await import("node:fs/promises");
        const bytes = await readFile(url);
        ({ instance } = await WebAssembly.instantiate(bytes, {
          env: emlite.env,
        }));
      }
      emlite.setExports(instance.exports);
      return instance.exports;
    })();
  }
  return exportsPromise;
}

export async function some_func(...args) {
  return (await init()).some_func(...args);
}

export async function some_other_func(...args) {
  return (await init()).some_other_func(...args);
}
```

Similarly if you use emscripten with the EMLITE_USE_EMSCRIPTEN_JS_GLUE flag:
```javascript
import { Emlite } from "emlite";
import initModule from "./bin/main.mjs";

let exportsPromise;   

async function init() {
    if (!exportsPromise) {
        exportsPromise = (async () => {
            const emlite = new Emlite();
            const mymain = await initModule();
            return mymain;
        })();
    }
  return exportsPromise;
}

export async function some_func(...args) {
  // notice the leading underscore
  return (await init())._some_func(...args);
}

export async function some_other_func(...args) {
  // notice the leading underscore
  return (await init())._some_other_func(...args);
}
```