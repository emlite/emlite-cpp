# Emlite for Zig

## Example
```zig
const std = @import("std");
const em = @import("emlite");

pub fn main() !void {
    var general_purpose_allocator: std.heap.GeneralPurposeAllocator(.{}) = .init;
    const gpa = general_purpose_allocator.allocator();

    _ = try em.emlite_eval(gpa,
    \\
    \\ console.log('{d}');
    \\
    , .{ 5 });

    const console = em.Val.global("console");
    const msg     = em.Val.from("Hello from Zig wrapper!");

    _ = console.call("log", .{msg});

    var arr = em.Val.array();
    _ = arr.call("push", .{ 1, 2, 3 });

    const len = arr.len();
    std.debug.print("JS array length = {}\n", .{len});

    const first = arr.get(0).as(i32);
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
```javascript
import { WASI, File, OpenFile, ConsoleStdout } from "@bjorn3/browser_wasi_shim";
import { Emlite } from "emlite";

async function main() => {
    let fds = [
        new OpenFile(new File([])), // 0, stdin
        ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)), // 1, stdout
        ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)), // 2, stderr
    ];
    let wasi = new WASI([], [], fds);
    // the zig wasm32-wasi target expects an initial memory of 257
    const emlite = new Emlite({ memory: new WebAssembly.Memory({ initial: 257 }) });
    const bytes = await emlite.readFile(new URL("./zig-out/bin/zigwasm.wasm", import.meta.url));
    let wasm = await WebAssembly.compile(bytes);
    let inst = await WebAssembly.instantiate(wasm, {
        wasi_snapshot_preview1: wasi.wasiImport,
        env: emlite.env,
    });
    emlite.setExports(inst.exports);
    wasi.start(inst);
}

await main();
```

## Building your project
```bash
zig build -Dtarget=wasm32-wasi
```