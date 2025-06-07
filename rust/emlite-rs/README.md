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
    elem.set(&"textContent", Val::from("Click"));
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
```

## Deployment
Building a program generates a .wasm binary. You will have to get the necessary glue via the emlite npm package:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
</head>
<body>
    <script src="./dom.js"></script>
</body>
</html>
```
