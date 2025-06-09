# Emlite for Zig

## Example
```zig
const std = @import("std");
const em = @import("emlite");

pub fn main() !void {
    const console = em.Val.global("console");
    const msg     = em.Val.fromStr("Hello from Zig wrapper!");

    _ = console.call("log", &.{msg});

    var arr = em.Val.array();
    _ = arr.call("push", &.{ em.Val.fromInt(1), em.Val.fromInt(2), em.Val.fromInt(3) });

    const len = arr.len();
    std.debug.print("JS array length = {}\n", .{len});

    const first = arr.at(0).asInt();
    std.debug.print("arr[0] = {}\n", .{first});
}
```

In your build.zig:
```zig
    const emlite_dep = b.dependency("emlite", .{
        .target = target,
        .optimize = optimize,
    });
    exe.root_module.addImport("emlite", emlite_dep.module("emlite"));
    exe.import_memory = true;
    exe.export_memory = true;
```

To use build.zig.zon, currently no packages are distributed. You can clone the github repo, and point to the zig/emlite subdirectory:
```zig
    .dependencies = .{
        .emlite = .{
            .path = "path/to/emlite/zig/emlite/",
        },
    },
```

In your web stack:
```
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";
import { Emlite } from "emlite";

window.onload = async () => {
    let fds = [
        new OpenFile(new File([])), // 0, stdin
        ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)), // 1, stdout
        ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)), // 2, stderr
    ];
    let wasi = new WASI([], [], fds);
    // the zig wasm32-wasi target expects an initial memory of 257
    let emlite = new Emlite(new WebAssembly.Memory({ initial: 257 }));
    let wasm = await WebAssembly.compileStreaming(fetch("./zig-out/bin/zigwasm.wasm"));
    let inst = await WebAssembly.instantiate(wasm, {
        wasi_snapshot_preview1: wasi.wasiImport,
        env: emlite.env,
    });
    emlite.setExports(inst.exports);
    wasi.start(inst);
};
```

## Building your project
```
zig build -Dtarget=wasm32-wasi
```