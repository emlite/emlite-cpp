#![allow(unused)]

#[unsafe(no_mangle)]
#[unsafe(export_name = "emlite_target")]
pub extern "C" fn emlite_target() -> i32 {
    1024
}

use core::ffi::{c_char, c_double, c_int, c_void};
pub type Handle = u32;

unsafe extern "C" {
    pub fn emlite_val_new_array() -> Handle;
    pub fn emlite_val_new_object() -> Handle;

    pub fn emlite_val_typeof(val: Handle) -> *mut c_char;

    pub fn emlite_val_construct_new(ctor: Handle, argv: Handle) -> Handle;
    pub fn emlite_val_func_call(func: Handle, argv: Handle) -> Handle;

    pub fn emlite_val_push(arr: Handle, v: Handle);

    pub fn emlite_val_make_int(t: c_int) -> Handle;
    pub fn emlite_val_make_double(t: c_double) -> Handle;
    pub fn emlite_val_make_str(s: *const c_char, len: usize) -> Handle;

    pub fn emlite_val_get_value_int(val: Handle) -> c_int;
    pub fn emlite_val_get_value_double(val: Handle) -> c_double;
    pub fn emlite_val_get_value_string(val: Handle) -> *mut c_char;

    pub fn emlite_val_get(val: Handle, idx: Handle) -> Handle;
    pub fn emlite_val_set(val: Handle, idx: Handle, val: Handle);
    pub fn emlite_val_has(val: Handle, idx: Handle) -> bool;

    pub fn emlite_val_is_string(val: Handle) -> bool;
    pub fn emlite_val_is_number(val: Handle) -> bool;
    pub fn emlite_val_not(val: Handle) -> bool;
    pub fn emlite_val_gt(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_gte(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_lt(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_lte(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_equals(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_strictly_equals(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_instanceof(arg1: Handle, arg2: Handle) -> bool;
    pub fn emlite_val_inc_ref(val: Handle);
    pub fn emlite_val_dec_ref(val: Handle);
    pub fn emlite_val_throw(val: Handle) -> !;

    pub fn emlite_val_obj_call(
        obj: Handle,
        name: *const c_char,
        len: usize,
        argv: Handle,
    ) -> Handle;

    pub fn emlite_val_obj_has_own_prop(obj: Handle, prop: *const c_char, len: usize) -> bool;

    pub fn emlite_val_make_callback(id: Handle, data: Handle) -> Handle;

    pub fn emlite_print_object_map();

    pub fn emlite_reset_object_map();
}
