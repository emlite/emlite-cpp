const std = @import("std");

pub const Handle = u32;

extern "env" fn emlite_val_null() Handle;
extern "env" fn emlite_val_undefined() Handle;
extern "env" fn emlite_val_false() Handle;
extern "env" fn emlite_val_true() Handle;
extern "env" fn emlite_val_global_this() Handle;
extern "env" fn emlite_val_new_array() Handle;
extern "env" fn emlite_val_new_object() Handle;
extern "env" fn emlite_val_typeof(val: Handle) [*:0]const u8;
extern "env" fn emlite_val_construct_new(ctor: Handle, argv: Handle) Handle;
extern "env" fn emlite_val_func_call(func: Handle, argv: Handle) Handle;
extern "env" fn emlite_val_push(arr: Handle, v: Handle) void;
extern "env" fn emlite_val_make_int(t: c_int) Handle;
extern "env" fn emlite_val_make_double(t: f64) Handle;
extern "env" fn emlite_val_make_str(s: [*]const u8, len: usize) Handle;
extern "env" fn emlite_val_get_value_int(val: Handle) c_int;
extern "env" fn emlite_val_get_value_double(val: Handle) f64;
extern "env" fn emlite_val_get_value_string(val: Handle) [*:0]const u8;
extern "env" fn emlite_val_get_elem(val: Handle, idx: usize) Handle;
extern "env" fn emlite_val_is_string(val: Handle) bool;
extern "env" fn emlite_val_is_number(val: Handle) bool;
extern "env" fn emlite_val_not(val: Handle) bool;
extern "env" fn emlite_val_gt(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_lt(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_strictly_equals(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_obj_call(obj: Handle, name: [*]const u8, len: usize, argv: Handle) Handle;
extern "env" fn emlite_val_obj_prop(obj: Handle, prop: [*]const u8, len: usize) Handle;
extern "env" fn emlite_val_obj_set_prop(obj: Handle, prop: [*]const u8, len: usize, val: Handle) void;
extern "env" fn emlite_val_obj_has_prop(obj: Handle, prop: [*]const u8, len: usize) bool;
extern "env" fn emlite_val_obj_has_own_prop(obj: Handle, prop: [*]const u8, len: usize) bool;
extern "env" fn emlite_val_make_callback(id: Handle) Handle;
extern "env" fn emlite_val_instanceof(a: Handle, b: Handle) bool;
extern "env" fn emlite_val_delete(val: Handle) void;
extern "env" fn emlite_val_throw(val: Handle) void;
// extern "env" fn emlite_malloc(len: usize) -> *mut c_void;
// extern "env" fn emlite_realloc(ptr: *mut c_void, len: usize) -> *mut c_void;
// extern "env" fn emlite_free(ptr: *mut c_void);

pub const Val = struct {
    handle: Handle,

    pub fn fromHandle(h: Handle) Val { return .{ .handle = h }; }

    pub fn nil() Val       { return fromHandle(emlite_val_null()); }
    pub fn undefined_() Val  { return fromHandle(emlite_val_undefined()); }
    pub fn globalThis() Val { return fromHandle(emlite_val_global_this()); }
    pub fn object() Val     { return fromHandle(emlite_val_new_object()); }
    pub fn array() Val      { return fromHandle(emlite_val_new_array()); }

    pub fn fromInt(i: i32)  Val { return fromHandle(emlite_val_make_int(i)); }
    pub fn fromF64(f: f64)  Val { return fromHandle(emlite_val_make_double(f)); }
    pub fn fromStr(s: []const u8) Val {
        return fromHandle(emlite_val_make_str(s.ptr, s.len));
    }

    pub fn global(name: []const u8) Val {
        return Val.globalThis().get(name);
    }

    pub inline fn toHandle(self: Val) Handle { return self.handle; }

    pub fn get(self: Val, prop: []const u8) Val {
        return fromHandle(emlite_val_obj_prop(
            self.handle, prop.ptr, prop.len));
    }

    pub fn set(self: Val, prop: []const u8, value: Val) void {
        emlite_val_obj_set_prop(
            self.handle, prop.ptr, prop.len, value.handle);
    }

    pub fn has(self: Val, prop: []const u8) bool {
        return emlite_val_obj_has_prop(self.handle, prop.ptr, prop.len);
    }

    pub fn hasOwn(self: Val, prop: []const u8) bool {
        return emlite_val_obj_has_own_prop(self.handle, prop.ptr, prop.len);
    }

    pub fn at(self: Val, idx: usize) Val {
        return fromHandle(emlite_val_get_elem(self.handle, idx));
    }

    pub fn len(self: Val) usize {
        return @intCast(self.get("length").asInt());
    }

    pub fn asInt(self: Val) i32  { return @as(i32, emlite_val_get_value_int(self.handle)); }
    pub fn asF64(self: Val) f64  { return emlite_val_get_value_double(self.handle); }
    pub fn asBool(self: Val) bool { return self.handle > 3; }

    pub fn toOwnedString(self: Val, alloc: std.mem.Allocator) ![]u8 {
        const z = emlite_val_get_value_string(self.handle);
        const slice = std.mem.spanZ(z);
        return alloc.dupe(u8, slice);
    }

    pub fn call(self: Val, method: []const u8, args: []const Val) Val {
        const arr = emlite_val_new_array();
        for (args) |v| emlite_val_push(arr, v.handle);
        return fromHandle(emlite_val_obj_call(
            self.handle, method.ptr, method.len, arr));
    }

    pub fn construct(self: Val, args: []const Val) Val {
        const arr = emlite_val_new_array();
        for (args) |v| emlite_val_push(arr, v.handle);
        return fromHandle(emlite_val_construct_new(self.handle, arr));
    }

    pub fn invoke(self: Val, args: []const Val) Val {
        const arr = emlite_val_new_array();
        for (args) |v| emlite_val_push(arr, v.handle);
        return fromHandle(emlite_val_func_call(self.handle, arr));
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

    pub fn delete(self: Val) void { emlite_val_delete(self.handle); }
    pub fn throw(self: Val) void  { emlite_val_throw(self.handle); }

    pub fn makeCallback(fn_ptr: fn (Handle) Handle) Val {
        return fromHandle(emlite_val_make_callback(@intCast(@intFromPtr(fn_ptr))));
    }
};

pub fn emlite_eval(alloc: std.mem.Allocator, comptime fmt: [] const u8, args: anytype) !Val {
    return Val.global("eval").invoke(&.{Val.fromStr(try std.fmt.allocPrint(alloc, fmt, args))});
}
