// initialize necessary env variables: WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT
// export WASI_SDK=~/wasi-sdk-25.0-x86_64-linux
// export WASI_LIBC=~/dev/wasi-libc
// export WASI_SYSROOT=~/dev/wasi-sysroot-25.0
// export EMSCRIPTEN_ROOT=~/emsdk/upstream/emscripten

import { execSync } from "child_process";
import fs from 'fs';
import * as path from "path";

function run(cmd) {
    console.log(`\x1b[36m$ ${cmd}\x1b[0m`);
    execSync(cmd, { stdio: "inherit", shell: true });
}

function buildSet(label, binDir, toolchain) {
    console.log(`\n\u25B6  Building for ${label} …`);
    run(
        [
            "cmake",
            `-B${binDir}`,
            "-GNinja",
            "-DEMLITE_BUILD_EXAMPLES=ON",
            "-DEMLITE_BUILD_TESTS=ON",
            "-DCMAKE_BUILD_TYPE=MinSizeRel",
            `-DCMAKE_TOOLCHAIN_FILE=${toolchain}`,
        ].join(" "),
    );
    run(`cmake --build ${binDir}`);
}

function* walk(dir, ext) {
    for (const ent of fs.readdirSync(dir, { withFileTypes: true })) {
        const p = path.join(dir, ent.name);
        if (ent.isDirectory()) yield* walk(p, ext);
        else if (p.endsWith(ext)) yield p;
    }
}

function bundleWasm(wasmPath) {
    const dir = path.dirname(wasmPath);
    const base = path.basename(wasmPath, ".wasm");
    const wrapper = path.join(dir, `${base}.wrapper.js`);
    const bundle = `${base}.bundle.js`;
    const bundlePath = path.join(dir, bundle);
    const htmlPath = path.join(dir, `${base}.html`);

    fs.writeFileSync(
        wrapper,
        `import { Emlite } from "../src/emlite.js";
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";

async function main() {
  const fds = [
    new OpenFile(new File([])),
    ConsoleStdout.lineBuffered(msg => console.log("[WASI] " + msg)),
    ConsoleStdout.lineBuffered(msg => console.warn("[WASI] " + msg))
  ];
  const wasi = new WASI([], [], fds);
  const emlite = new Emlite();

  const wasm = await WebAssembly.compileStreaming(fetch("${path.basename(wasmPath)}"));
  const inst = await WebAssembly.instantiate(wasm, {
    wasi_snapshot_preview1: wasi.wasiImport,
    env: emlite.env,
  });

  emlite.setExports(inst.exports);
  wasi.start(inst);
  alert("1 + 2 = " + (inst.exports.add?.(1,2) ?? "n/a"));
}
main();\n`,
    );

    run(
        [
            "npx",
            "webpack-cli",
            "--entry-reset",
            `./${wrapper}`,
            "--output-path",
            `./${dir}`,
            "--output-filename",
            bundle,
            "--mode",
            "production",
            "--target",
            "web",
        ].join(" "),
    );

    fs.writeFileSync(
        htmlPath,
        `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>${base}</title></head>
<body>
<script type="module" src="./${bundle}"></script>
</body>
</html>\n`,
    );

    fs.unlinkSync(wrapper);

    return { base, html: path.basename(htmlPath) };
}

function writeDirIndex(dir, pages) {
    const items = pages
        .map(({ base, html }) => `  <li><a href="./${html}">${base}</a></li>`)
        .join("\n");
    fs.writeFileSync(
        path.join(dir, "index.html"),
        `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>Index of ${path.basename(dir)}</title></head>
<body>
<h1>${path.basename(dir)} – demos</h1>
<ul>
${items}
</ul>
</body>
</html>\n`,
    );
}

async function main() {
    try {
        // 1 · Freestanding (stock clang, wasm32-unknown-unknown)
        buildSet(
            "FREESTANDING",
            "bin_freestanding",
            "./cmake/freestanding.cmake",
        );

        const { WASI_SDK, WASI_SYSROOT, WASI_LIBC, EMSCRIPTEN_ROOT } = process.env;

        // 2 · WASI SDK
        if (WASI_SDK) {
            buildSet(
                "WASI_SDK",
                "bin_wasi_sdk",
                join(WASI_SDK, "share/cmake/wasi-sdk.cmake"),
            );
        }

        // 3 · WASI Sysroot
        if (WASI_SYSROOT) {
            buildSet(
                "WASI_SYSROOT",
                "bin_wasi_sysroot",
                "./cmake/wasi_sysroot.cmake",
            );
        }

        // 4 · WASI libc
        if (WASI_LIBC) {
            buildSet(
                "WASI_LIBC",
                "bin_wasi_libc",
                "./cmake/wasi_libc.cmake",
            );
        }

        // 5 · Emscripten
        if (EMSCRIPTEN_ROOT) {
            buildSet(
                "EMSCRIPTEN_ROOT",
                "bin_emscripten",
                join(
                    EMSCRIPTEN_ROOT,
                    "cmake/Modules/Platform/Emscripten.cmake",
                ),
            );
        }

        console.log("\n\x1b[32m✔ All requested builds completed successfully.\x1b[0m");

        console.log("\n\u2705  CMake phase done – starting WASM packaging …");

        for (const binDir of fs
            .readdirSync(".")
            .filter((d) => d.startsWith("bin_") && fs.statSync(d).isDirectory())) {
            const pages = [];
            for (const wasm of walk(binDir, ".wasm")) {
                pages.push(bundleWasm(wasm));
            }
            if (pages.length) writeDirIndex(binDir, pages);
        }

        console.log("\n\x1b[32m✔ All builds & bundles complete.\x1b[0m");
    } catch (err) {
        console.error("\n\x1b[31m✖ Build failed:\x1b[0m", err.message);
        process.exit(err.status || 1);
    }
}

await main();