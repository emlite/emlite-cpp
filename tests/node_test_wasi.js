// nodejs example for examples/node_readfile.cpp
// node tests/index.js

import fs from "node:fs";
import { Emlite } from "../src/emlite.js";
import { WASI } from "node:wasi";
import { argv, env } from "node:process";

import { createRequire } from "module";


async function main() {
    const wasi = new WASI({
        version: 'preview1',
        args: argv,
        env,
    });

    const emlite = new Emlite({
        globals: {
            fs,
            require: createRequire(import.meta.url),
        }
    });
    const bytes = await emlite.readFile(new URL("../bin/wasi_sdk/node_readfile.wasm", import.meta.url));
    const wasm = await WebAssembly.compile(bytes);
    const instance = await WebAssembly.instantiate(wasm, {
        wasi_snapshot_preview1: wasi.wasiImport,
        env: emlite.env,
    });
    emlite.setExports(instance.exports);
    wasi.start(instance);
}

await main();