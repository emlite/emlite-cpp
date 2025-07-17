import { execSync } from "child_process";
import fs from "fs";
import * as path from "path";

function run(cmd) {
  console.log(`\x1b[36m$ ${cmd}\x1b[0m`);
  execSync(cmd, { stdio: "inherit", shell: true });
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
  const bundle = `${base}.js`;
  const htmlPath = path.join(dir, `${base}.html`);

  if (dir.includes("freestanding")) {
    fs.writeFileSync(
      wrapper,
      `
            import { Emlite } from "../../../node_modules/emlite/src/emlite.js";

            async function main() {
                const emlite = new Emlite();
                const bytes = await emlite.readFile(new URL("${path.basename(
                  wasmPath
                )}", import.meta.url));
                let wasm = await WebAssembly.compile(bytes);
                let inst = await WebAssembly.instantiate(wasm, {
                    env: emlite.env,
                });
                emlite.setExports(inst.exports);
                inst.exports.main?.();
                window.alert("1 + 2 = " + (inst.exports.add?.(1,2) ?? "n/a"));
            }

            await main();\n`
    );
  } else if (dir.includes("emscripten_default")) {
    // do nothing
  } else {
    fs.writeFileSync(
      wrapper,
      `
            import { Emlite } from "../../../node_modules/emlite/src/emlite.js";
            import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";

            async function main() {
                const fds = [
                    new OpenFile(new File([])),
                    ConsoleStdout.lineBuffered(msg => console.log("[WASI] " + msg)),
                    ConsoleStdout.lineBuffered(msg => console.warn("[WASI] " + msg))
                ];
                const wasi = new WASI([], [], fds);
                const emlite = new Emlite();
                const bytes = await emlite.readFile(new URL("${path.basename(
                  wasmPath
                )}", import.meta.url));
                let wasm = await WebAssembly.compile(bytes);
                const inst = await WebAssembly.instantiate(wasm, {
                    wasi_snapshot_preview1: wasi.wasiImport,
                    env: emlite.env,
                });

                emlite.setExports(inst.exports);
                wasi.start(inst);
                window.alert("1 + 2 = " + (inst.exports.add?.(1,2) ?? "n/a"));
            }
            await main();\n`
    );
  }

  if (dir.includes("emscripten_default")) {
    // do nothing!
  } else {
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
      ].join(" ")
    );
  }

  fs.writeFileSync(
    htmlPath,
    `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>${base}</title></head>
<body>
<script ${
      dir.includes("emscripten_default") ? "type=module" : ""
    } src="./${bundle}"></script>
</body>
</html>\n`
  );

  if (!dir.includes("emscripten_default")) fs.unlinkSync(wrapper);

  return { base, html: path.basename(htmlPath) };
}

function writeDirIndex(dir, pages) {
  const items = pages
    .map(
      ({ base, html }) => `  <li><a href="./examples/${html}">${base}</a></li>`
    )
    .join("\n");
  fs.writeFileSync(
    path.join(path.join("./bin", dir), "index.html"),
    `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>Index of ${path.basename(dir)}</title></head>
<body>
<h1>${path.basename(dir)} – demos</h1>
<ul>
${items}
</ul>
</body>
</html>\n`
  );
}

async function main() {
  try {
    console.log("\n\u2705  Starting WASM packaging …");

    for (const binDir of fs
      .readdirSync("./bin")
      .filter((d) => fs.statSync(path.join("./bin", d)).isDirectory())) {
      const pages = [];
      for (const wasm of walk(path.join("./bin", binDir), ".wasm")) {
        if (/^\d/.test(path.basename(wasm))) continue;
        pages.push(bundleWasm(wasm));
      }
      if (pages.length) writeDirIndex(binDir, pages);
    }

    console.log("\n\x1b[32m✔ Bundling completed.\x1b[0m");
  } catch (err) {
    console.error("\n\x1b[31m✖ Bundling failed:\x1b[0m", err.message);
    process.exit(err.status || 1);
  }
}

await main();
