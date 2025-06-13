#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

typedef struct {
    Handle h;
} em_Val;

em_Val em_Val_from_int(int i);
em_Val em_Val_from_double(double i);
em_Val em_Val_from_string(const char *i);
em_Val em_Val_from_handle(uint32_t v);
em_Val em_Val_global(const char *name);
em_Val em_Val_global_this();
em_Val em_Val_null();
em_Val em_Val_undefined();
em_Val em_Val_object();
em_Val em_Val_array();
em_Val em_Val_make_js_function(Callback f);
void em_Val_delete(em_Val);
void em_Val_throw(em_Val);

Handle em_Val_as_handle(em_Val self);
em_Val em_Val_get(em_Val self, const char *prop);
void em_Val_set(em_Val self, const char *prop, em_Val val);
bool em_Val_has(em_Val self, const char *prop);
bool em_Val_has_own_property(em_Val self, const char *prop);
const char *em_Val_type_of(em_Val self);
em_Val em_Val_at(em_Val self, size_t idx);
em_Val em_Val_await(em_Val self);
bool em_Val_is_number(em_Val self);
bool em_Val_is_string(em_Val self);
bool em_Val_instanceof(em_Val self, em_Val v);
bool em_Val_not(em_Val self);
bool em_Val_seq(em_Val self, em_Val other);
bool em_Val_eq(em_Val self, em_Val other);
bool em_Val_neq(em_Val self, em_Val other);
bool em_Val_gt(em_Val self, em_Val other);
bool em_Val_gte(em_Val self, em_Val other);
bool em_Val_lt(em_Val self, em_Val other);
bool em_Val_lte(em_Val self, em_Val other);

int em_Val_as_int(em_Val self);
bool em_Val_as_bool(em_Val self);
double em_Val_as_double(em_Val self);
const char *em_Val_as_string(em_Val self);

em_Val em_Val_call(em_Val self, const char *method, int n, ...);
em_Val em_Val_new(em_Val self, int n, ...);
em_Val em_Val_invoke(em_Val self, int n, ...);

em_Val emlite_eval(const char *src);
em_Val emlite_eval_v(const char *src, ...);
#define EMLITE_EVAL(x, ...) emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__))

#ifdef EMLITE_IMPL

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

em_Val em_Val_from_int(int i) { return (em_Val){.h = emlite_val_make_int(i)}; }

em_Val em_Val_from_double(double i) {
    return (em_Val){.h = emlite_val_make_double(i)};
}
em_Val em_Val_from_string(const char *s) {
    return (em_Val){.h = emlite_val_make_str(s, strlen(s))};
}

em_Val em_Val_from_handle(uint32_t v) { return (em_Val){.h = v}; }

em_Val em_Val_global(const char *name) {
    Handle global = emlite_val_global_this();
    return em_Val_from_handle(emlite_val_obj_prop(global, name, strlen(name)));
}

em_Val em_Val_global_this() {
    return em_Val_from_handle(emlite_val_global_this());
}

em_Val em_Val_null() { return em_Val_from_handle(0); }

em_Val em_Val_undefined() { return em_Val_from_handle(1); }

em_Val em_Val_object() { return em_Val_from_handle(emlite_val_new_object()); }

em_Val em_Val_array() { return em_Val_from_handle(emlite_val_new_array()); }

em_Val em_Val_make_js_function(Callback f) {
    uint32_t fidx = (uint32_t)f;
    return em_Val_from_handle(emlite_val_make_callback(fidx));
}

void em_Val_delete(em_Val v) { emlite_val_delete(v.h); }

void em_Val_throw(em_Val v) { emlite_val_throw(v.h); }

Handle em_Val_as_handle(em_Val self) { return self.h; }

em_Val em_Val_get(em_Val self, const char *prop) {
    return em_Val_from_handle(emlite_val_obj_prop(self.h, prop, strlen(prop)));
}

void em_Val_set(em_Val self, const char *prop, em_Val val) {
    emlite_val_obj_set_prop(self.h, prop, strlen(prop), val.h);
}

bool em_Val_has(em_Val self, const char *prop) {
    return emlite_val_obj_has_prop(self.h, prop, strlen(prop));
}

bool em_Val_has_own_property(em_Val self, const char *prop) {
    return emlite_val_obj_has_own_prop(self.h, prop, strlen(prop));
}

const char *em_Val_type_of(em_Val self) { return emlite_val_typeof(self.h); }

em_Val em_Val_at(em_Val self, size_t idx) {
    return em_Val_from_handle(emlite_val_get_elem(self.h, idx));
}

em_Val em_Val_await(em_Val self) {
    // clang-format off
    return EMLITE_EVAL({
        (async() =>
            {
                let obj = ValMap.toValue(% d);
                let ret = await obj;
                return ValMap.toHandle(ret);
            })()
        }, self.h
    );
    // clang-format on
}

bool em_Val_is_number(em_Val self) { return emlite_val_is_number(self.h); }

bool em_Val_is_string(em_Val self) { return emlite_val_is_string(self.h); }

bool em_Val_instanceof(em_Val self, em_Val v) {
    return emlite_val_instanceof(self.h, v.h);
}

bool em_Val_not(em_Val self) { return emlite_val_not(self.h); }

bool em_Val_seq(em_Val self, em_Val other) {
    return emlite_val_strictly_equals(self.h, other.h);
}

bool em_Val_eq(em_Val self, em_Val other) {
    return emlite_val_equals(self.h, other.h);
}

bool em_Val_neq(em_Val self, em_Val other) {
    return !emlite_val_strictly_equals(self.h, other.h);
}

bool em_Val_gt(em_Val self, em_Val other) {
    return emlite_val_gt(self.h, other.h);
}

bool em_Val_gte(em_Val self, em_Val other) {
    return emlite_val_gte(self.h, other.h);
}

bool em_Val_lt(em_Val self, em_Val other) {
    return emlite_val_lt(self.h, other.h);
}

bool em_Val_lte(em_Val self, em_Val other) {
    return emlite_val_lte(self.h, other.h);
}

int em_Val_as_int(em_Val self) { return emlite_val_get_value_int(self.h); }

bool em_Val_as_bool(em_Val self) { return self.h > 3; }

double em_Val_as_double(em_Val self) {
    return emlite_val_get_value_double(self.h);
}

const char *em_Val_as_string(em_Val self) {
    return emlite_val_get_value_string(self.h);
}

em_Val em_Val_call(em_Val self, const char *method, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    return em_Val_from_handle(
        emlite_val_obj_call(self.h, method, strlen(method), arr)
    );
}

em_Val em_Val_new(em_Val self, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    return em_Val_from_handle(emlite_val_construct_new(self.h, arr));
}

em_Val em_Val_invoke(em_Val self, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    return em_Val_from_handle(emlite_val_func_call(self.h, arr));
}

em_Val emlite_eval(const char *src) {
    em_Val eval = em_Val_global("eval");
    return em_Val_invoke(eval, 1, em_Val_from_string(src));
}

em_Val emlite_eval_v(const char *src, ...) {
    va_list args;
    va_start(args, src);
    size_t len = vsnprintf(NULL, 0, src, args);
    char *ptr  = (char *)malloc(len);
    (void)vsnprintf(ptr, len + 1, src, args);
    va_end(args);
    return emlite_eval(ptr);
}

#endif

#ifdef __cplusplus
}
#endif