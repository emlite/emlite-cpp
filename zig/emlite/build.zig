const std = @import("std");

const Example = struct {
    description: ?[]const u8,
    output: []const u8,
    input: []const u8,

    pub fn init(output: []const u8, input: []const u8, desc: ?[]const u8) Example {
        return Example{
            .description = desc,
            .output = output,
            .input = input,
        };
    }
};


pub const examples = &[_]Example{
    Example.init("array", "examples/array.zig", "A simple emlite array example"),
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const emlite = b.addModule("emlite", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });

    const build_examples = b.option(bool, "emlite-build-examples", "Build emlite examples") orelse false;

    if (build_examples) {
        const examples_step = b.step("examples", "build the emlite examples");
        for (examples) |ex| {
            const exe = b.addExecutable(.{
                .name = ex.output,
                .root_source_file = b.path(ex.input),
                .target = target,
                .optimize = optimize,
            });
            exe.import_memory = true;
            exe.export_memory = true;
            exe.export_table = true;
            exe.rdynamic = true;
            exe.root_module.addImport("emlite", emlite);
            examples_step.dependOn(&exe.step);
            b.installArtifact(exe);
        }
    }
}
