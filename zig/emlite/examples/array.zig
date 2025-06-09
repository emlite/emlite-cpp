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