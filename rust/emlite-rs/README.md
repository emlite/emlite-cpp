# emlite-rs

A Rust wrapper around emlite, which allows native code to target javascript.

## Usage
Add emlite to your Cargo.toml:
```toml
[dependencies]
emlite = "0.1.0"
```

Then you can import and use the Val wrapper and its associated methods:
```rust
use emlite::{Console, Val};

fn main() {
    let con = Console::get();
    con.log(&[Val::from("Hello from Emlite!")]);
}
```

```rust
use emlite::*;

fn main() {
    let document = Val::global("document");
    let elem = document.call("createElement", &argv!["BUTTON"]);
    elem.set("textContent", Val::from("Click"));
    let body = document.call("getElementsByTagName", &argv!["body"]).at(0);
    elem.call(
        "addEventListener",
        &argv![
            "click",
            Val::make_js_function(|ev| {
                let console = Val::global("console");
                console.call("clear", &[]);
                println!("client x: {}", Val::from_handle(ev).get("clientX").as_i32());
                println!("hello from Rust");
                ().into()
            })
        ],
    );
    body.call("appendChild", &argv![elem]);
}
```

```rust
use emlite::*;

fn main() {
    #[allow(non_snake_case)]
    let mut AudioContext = Val::global("AudioContext");
    if !AudioContext.as_bool() {
        println!("No global AudioContext, trying webkitAudioContext");
        AudioContext = Val::global("webkitAudioContext");
    }

    println!("Got an AudioContext");
    let context = AudioContext.new(&[]);
    let oscillator = context.call("createOscillator", &[]);

    println!("Configuring oscillator");
    oscillator.set("type", Val::from("triangle"));
    oscillator.get("frequency").set("value", Val::from(261.63)); // Middle C

    println!("Playing");
    oscillator.call("connect", &argv![context.get("destination")]);
    oscillator.call("start", &argv![0]);

    println!("All done!");
}
```

## Building
To build, you need:
- wasm32-wasip1 target.

To get the rust target:
```bash
rustup target add wasm32-wasip1
```

Running the build, you only need to pass the target to cargo:
```
cargo build --target=wasm32-wasip1
```

## Passing necessary flags for javascript engines (browser, node ...etc)
The most convenient way to pass extra flags to the toolchain is via a .cargo/config.toml file:
```toml
[target.wasm32-wasip1]
rustflags = ["-Clink-args=--no-entry --allow-undefined --export-all --import-memory --export-memory --strip-all"]

[profile.release]
lto = true # to get smaller builds
```

## Deployment

You can get 
### In the browser

To use it in your web stack, you will need a wasi javascript polyfill, here we use @bjorn3/browser_wasi_shim and the emlite npm packages:

```javascript
// see the index.html for an example
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";
import { Emlite } from "emlite";

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

### With a javascript engine like nodejs

If you're vendoring the emlite.js file:
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
Note that nodejs as of version 22.16 requires a _start function in the wasm module. That can be achieved by defining an `fn main() {}` function. It's also why we use `wasi.start(instance)` in the js module.