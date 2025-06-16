// nodejs example for examples/node_readfile.cpp
// node tests/index.js

import { Emlite } from "../src/emlite.js";
import { WASI } from "node:wasi";
import { readFile } from "node:fs/promises";
import { argv, env } from "node:process";

import { createRequire } from "module";
const require = createRequire(import.meta.url);
globalThis.require = require;

async function main() {
    const wasi = new WASI({
        version: 'preview1',
        args: argv,
        env,
    });

    const emlite = new Emlite();
    const wasm = await WebAssembly.compile(
        await readFile("./bin/node_readfile.wasm"),
    );
    const instance = await WebAssembly.instantiate(wasm, {
        wasi_snapshot_preview1: wasi.wasiImport,
        env: emlite.env,
    });
    emlite.setExports(instance.exports);
    wasi.start(instance);
}

await main();