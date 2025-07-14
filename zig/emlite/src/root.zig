const std = @import("std");
const meta = std.meta;

pub export fn emlite_target() i32 {
    return 1024;
}

pub const Handle = u32;

const EmlitePrefHandles = enum(i32) {
    Null = 0,
    Undefined,
    False,
    True,
    GlobalThis,
    Console,
    Reserved,
};

extern "env" fn emlite_val_new_array() Handle;
extern "env" fn emlite_val_new_object() Handle;
extern "env" fn emlite_val_typeof(val: Handle) [*:0] u8;
extern "env" fn emlite_val_construct_new(ctor: Handle, argv: Handle) Handle;
extern "env" fn emlite_val_func_call(func: Handle, argv: Handle) Handle;
extern "env" fn emlite_val_push(arr: Handle, v: Handle) void;
extern "env" fn emlite_val_make_int(t: c_int) Handle;
extern "env" fn emlite_val_make_double(t: f64) Handle;
extern "env" fn emlite_val_make_str(s: [*]const u8, len: usize) Handle;
extern "env" fn emlite_val_get_value_int(val: Handle) c_int;
extern "env" fn emlite_val_get_value_double(val: Handle) f64;
extern "env" fn emlite_val_get_value_string(val: Handle) [*:0] u8;
extern "env" fn emlite_val_get(val: Handle, idx: Handle) Handle;
extern "env" fn emlite_val_is_string(val: Handle) bool;
extern "env" fn emlite_val_is_number(val: Handle) bool;
extern "env" fn emlite_val_not(val: Handle) bool;
extern "env" fn emlite_val_gt(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_lt(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_strictly_equals(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_obj_call(obj: Handle, name: [*]const u8, len: usize, argv: Handle) Handle;
extern "env" fn emlite_val_obj_prop(obj: Handle, prop: [*]const u8, len: usize) Handle;
extern "env" fn emlite_val_set(obj: Handle, prop: Handle, val: Handle) void;
extern "env" fn emlite_val_has(obj: Handle, prop: Handle) bool;
extern "env" fn emlite_val_obj_has_own_prop(obj: Handle, prop: [*]const u8, len: usize) bool;
extern "env" fn emlite_val_make_callback(id: Handle, data: Handle) Handle;
extern "env" fn emlite_val_instanceof(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_dec_ref(val: Handle) void;
extern "env" fn emlite_val_inc_ref(val: Handle) void;
extern "env" fn emlite_val_throw(val: Handle) void;
extern "env" fn emlite_print_object_map() void;
extern "env" fn emlite_reset_object_map() void;

pub const Val = struct {
    handle: Handle,

    pub fn fromHandle(h: Handle) Val { return .{ .handle = h }; }

    pub fn nil() Val       { return fromHandle(@intFromEnum(EmlitePrefHandles.Null)); }
    pub fn undefined_() Val  { return fromHandle(@intFromEnum(EmlitePrefHandles.Undefined)); }
    pub fn globalThis() Val { return fromHandle(@intFromEnum(EmlitePrefHandles.GlobalThis)); }
    pub fn object() Val     { return fromHandle(emlite_val_new_object()); }
    pub fn array() Val      { return fromHandle(emlite_val_new_array()); }

    pub fn fromInt(i: i32)  Val { return fromHandle(emlite_val_make_int(i)); }
    pub fn fromF64(f: f64)  Val { return fromHandle(emlite_val_make_double(f)); }
    pub fn fromStr(s: []const u8) Val {
        return fromHandle(emlite_val_make_str(s.ptr, s.len));
    }
    pub inline fn from(v: anytype) Val {
        const T = @TypeOf(v);
        if (T == Val) {
            return v;
        } else return switch(@typeInfo(T)) {
            .int, .comptime_int => fromInt(@intCast(v)),
            .float, .comptime_float => fromF64(@floatCast(v)),
            .pointer => |info| switch(info.size) {
                .slice => if (info.child == u8 and info.is_const)
                    fromStr(v) else @compileError("Val.from: unsupported type " ++ @typeName(T)),
                .one => {
                    const child_info = @typeInfo(info.child);
                    if (child_info == .array and child_info.array.child == u8 and info.is_const) {
                        const full_len = child_info.array.len;
                        const slice    = v.*[0 .. full_len - 1];
                        return fromStr(slice);
                    }
                    @compileError("Val.from: unsupported pointer type " ++ @typeName(T));
                },
                else => @compileError("Val.from: unsupported pointer type " ++ @typeName(T)),
            },
            else => @compileError("Val.from: unsupported type " ++ @typeName(T)),
        };
    }

    pub fn global(name: []const u8) Val {
        return Val.globalThis().get(Val.fromStr(name));
    }

    pub inline fn toHandle(self: Val) Handle { return self.handle; }

    pub fn get(self: Val, prop: anytype) Val {
        return fromHandle(emlite_val_get(
            self.handle, Val.from(prop).handle));
    }

    pub fn set(self: Val, prop: anytype, val: anytype) void {
        emlite_val_set(
            self.handle, Val.from(prop).handle, Val.from(val).handle);
    }

    pub fn has(self: Val, prop: anytype) bool {
        return emlite_val_has(self.handle, Val.from(prop).handle);
    }

    pub fn hasOwn(self: Val, prop: []const u8) bool {
        return emlite_val_obj_has_own_prop(self.handle, prop.ptr, prop.len);
    }

    pub fn len(self: Val) usize {
        const l = self.get("length");
        const ret: usize = @intCast(l.asInt());
        Val.delete(l);
        return ret;
    }

    pub fn typeof(self: Val) [*:0] u8 {
        return emlite_val_typeof(self.handle);
    }
    pub fn asInt(self: Val) i32  { return @as(i32, emlite_val_get_value_int(self.handle)); }
    pub fn asF64(self: Val) f64  { return emlite_val_get_value_double(self.handle); }
    pub fn asBool(self: Val) bool { return !emlite_val_not(self.handle); }
    pub fn asOwnedString(self: Val) [*:0] u8 {
        return emlite_val_get_value_string(self.handle);
    }

    pub fn as(self: Val, comptime T: type) T {
        if (T == Val)
            return self;

        return switch (@typeInfo(T)) {
            .int, .comptime_int => @intCast(self.asInt()),
            .float, .comptime_float => @floatCast(self.asF64()),
            .bool => self.asBool(),
            .pointer => |ptr| {
                if (ptr.child != u8 or ptr.is_const)
                    @compileError("Val.as: unsupported target type " ++ @typeName(T));
                const zstr  = emlite_val_get_value_string(self.handle);
                return zstr;
            },
            else => @compileError("Val.as: unsupported target type " ++ @typeName(T)),
        };
    }

    pub fn call(self: Val, method: []const u8, args: anytype) Val {
        const arr = emlite_val_new_array();
        const T = @TypeOf(args);
        inline for (std.meta.fields(T)) |field| {
            const elem = @field(args, field.name);
            const v    = Val.from(elem);
            emlite_val_push(arr, v.handle);
        }
        const ret = Val.fromHandle(emlite_val_obj_call(
            self.handle, method.ptr, method.len, arr));
        emlite_val_dec_ref(arr);
        return ret;
    }

    pub fn construct(self: Val, args: anytype) Val {
        const arr = emlite_val_new_array();
        const T = @TypeOf(args);
        inline for (std.meta.fields(T)) |field| {
            const elem = @field(args, field.name);
            const v    = Val.from(elem);
            emlite_val_push(arr, v.handle);
        }
        const ret = Val.fromHandle(emlite_val_construct_new(self.handle, arr));
        emlite_val_dec_ref(arr);
        return ret;
    }

    pub fn invoke(self: Val, args: anytype) Val {
        const arr = emlite_val_new_array();
        const T = @TypeOf(args);
        inline for (std.meta.fields(T)) |field| {
            const elem = @field(args, field.name);
            const v    = Val.from(elem);
            emlite_val_push(arr, v.handle);
        }
        const ret = fromHandle(emlite_val_func_call(self.handle, arr));
        emlite_val_dec_ref(arr);
        return ret;
    }

    pub fn strictEquals(a: Val, b: Val) bool {
        return emlite_val_strictly_equals(a.handle, b.handle);
    }

    pub fn gt(a: Val, b: Val) bool { return emlite_val_gt(a.handle, b.handle); }
    pub fn lt(a: Val, b: Val) bool { return emlite_val_lt(a.handle, b.handle); }
    pub fn not(self: Val) bool     { return emlite_val_not(self.handle); }

    pub fn instanceof(a: Val, ctor: Val) bool {
        return emlite_val_instanceof(a.handle, ctor.handle);
    }

    pub fn delete(self: Val) void { emlite_val_dec_ref(self.handle); }
    pub fn throw(self: Val) void  { emlite_val_throw(self.handle); }

    pub fn makeFn(fn_ptr: fn (Handle) Handle, data: Handle) Val {
        return fromHandle(emlite_val_make_callback(@intCast(@intFromPtr(fn_ptr))), data);
    }
};

pub fn emlite_eval(alloc: std.mem.Allocator, comptime fmt: [] const u8, args: anytype) !Val {
    const eval = Val.global("eval");
    const str = try std.fmt.allocPrint(alloc, fmt, args);
    const str_val = Val.fromStr(str);
    const ret = eval.invoke(.{str_val});
    Val.delete(str_val);
    alloc.free(str);
    Val.delete(eval);
    return ret;
}

test "all" {
    @import("std").testing.refAllDeclsRecursive(@This());
}