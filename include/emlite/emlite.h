#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef EMLITE_USED
#define EMLITE_USED __attribute__((used))
#endif

typedef uint32_t Handle;

typedef Handle (*Callback)(Handle);

#ifdef __cplusplus
extern "C" {
#endif

// externs
Handle emlite_val_null(void);
Handle emlite_val_undefined(void);
Handle emlite_val_false(void);
Handle emlite_val_true(void);
Handle emlite_val_global_this();
Handle emlite_val_new_array(void);
Handle emlite_val_new_object(void);
const char *emlite_val_typeof(Handle);
Handle emlite_val_construct_new(Handle, Handle argv);
Handle emlite_val_func_call(Handle func, Handle argv);
void emlite_val_push(Handle arr, Handle v);
Handle emlite_val_make_int(int t);
Handle emlite_val_make_double(double t);
Handle emlite_val_make_str(const char *, size_t);
int emlite_val_get_value_int(Handle);
double emlite_val_get_value_double(Handle);
const char *emlite_val_get_value_string(Handle);
Handle emlite_val_get_elem(Handle, size_t);
bool emlite_val_is_string(Handle);
bool emlite_val_is_number(Handle);
bool emlite_val_not(Handle);
bool emlite_val_gt(Handle, Handle);
bool emlite_val_gte(Handle, Handle);
bool emlite_val_lt(Handle, Handle);
bool emlite_val_lte(Handle, Handle);
bool emlite_val_equals(Handle, Handle);
bool emlite_val_strictly_equals(Handle, Handle);
bool emlite_val_instanceof(Handle, Handle);
void emlite_val_delete(Handle);
void emlite_val_throw(Handle);

Handle emlite_val_obj_call(
    Handle obj, const char *name, size_t len, Handle argv
);
Handle emlite_val_obj_prop(Handle obj, const char *prop, size_t len);
void emlite_val_obj_set_prop(
    Handle obj, const char *prop, size_t len, Handle val
);
bool emlite_val_obj_has_prop(Handle, const char *prop, size_t len);
bool emlite_val_obj_has_own_prop(Handle, const char *prop, size_t len);
Handle emlite_val_make_callback(Handle id);
// end externs

Handle emlite_val_global(const char *name);
Handle emlite_val_construct_new_v(Handle, int n, ...);
Handle emlite_val_func_call_v(Handle func, int n, ...);
Handle emlite_val_obj_call_v(Handle obj, const char *name, int n, ...);
Handle emlite_eval(const char *src, ...);

#define VAL_OBJ_CALL(obj, name, ...)                                           \
    emlite_val_obj_call_v(                                                     \
        (obj),                                                                 \
        (name),                                                                \
        (int)(sizeof((Handle[]){__VA_ARGS__}) / sizeof(Handle)),               \
        __VA_ARGS__                                                            \
    )

#define VAL_OBJ_NEW(obj, ...)                                                  \
    emlite_val_construct_new_v(                                                \
        obj,                                                                   \
        (int)(sizeof((Handle[]){__VA_ARGS__}) / sizeof(Handle)),               \
        __VA_ARGS__                                                            \
    )

#define VAL_FUNC_CALL(obj, ...)                                                \
    emlite_val_func_call_v(                                                    \
        obj,                                                                   \
        (int)(sizeof((Handle[]){__VA_ARGS__}) / sizeof(Handle)),               \
        __VA_ARGS__                                                            \
    )

#define EMLITE_EVAL(x, ...) emlite_eval(#x __VA_OPT__(, __VA_ARGS__))

#ifdef EMLITE_IMPL
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

Handle emlite_val_global(const char *name) {
    Handle w = emlite_val_global_this();
    return emlite_val_obj_prop(w, name, strlen(name));
}

Handle emlite_val_obj_call_v(Handle obj, const char *name, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, Handle));
    va_end(args);
    return emlite_val_obj_call(obj, name, strlen(name), arr);
}

Handle emlite_val_construct_new_v(Handle obj, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, Handle));
    va_end(args);
    return emlite_val_construct_new(obj, arr);
}

Handle emlite_val_func_call_v(Handle obj, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, Handle));
    va_end(args);
    return emlite_val_func_call(obj, arr);
}

Handle emlite_eval(const char *src, ...) {
    va_list args;
    va_start(args, src);
    size_t len = vsnprintf(NULL, 0, src, args);
    char *ptr  = (char *)malloc(len);
    (void)vsnprintf(ptr, len + 1, src, args);
    va_end(args);
    Handle global = emlite_val_global_this();
    Handle eval   = emlite_val_obj_prop(global, "eval", strlen("eval"));
    return VAL_FUNC_CALL(eval, emlite_val_make_str(ptr, len));
}

#endif

#ifdef __cplusplus
}
#endif