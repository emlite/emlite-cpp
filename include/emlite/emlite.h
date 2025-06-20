#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// these are freestanding headers
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if __has_include(<stdlib.h>)
#include <stdlib.h>
#else
void *emlite_malloc(size_t);
void *emlite_realloc(void *, size_t);
void emlite_free(void *);
extern void *malloc(size_t);
extern void *realloc(void *, size_t);
extern void free(void *);
#endif

#if __has_include(<string.h>)
#include <string.h>
#else
void *memset(void *dest, int ch, size_t count);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *);
#endif

#ifndef EMLITE_USED
#define EMLITE_USED __attribute__((used))
#endif

typedef uint32_t Handle;

typedef Handle (*Callback)(Handle);

extern Handle emlite_val_null(void);
extern Handle emlite_val_undefined(void);
extern Handle emlite_val_false(void);
extern Handle emlite_val_true(void);
extern Handle emlite_val_global_this();
extern Handle emlite_val_new_array(void);
extern Handle emlite_val_new_object(void);
extern char *emlite_val_typeof(Handle);
extern Handle emlite_val_construct_new(Handle, Handle argv);
extern Handle emlite_val_func_call(
    Handle func, Handle argv
);
extern void emlite_val_push(Handle arr, Handle v);
extern Handle emlite_val_make_int(int t);
extern Handle emlite_val_make_double(double t);
extern Handle emlite_val_make_str(const char *, size_t);
extern int emlite_val_get_value_int(Handle);
extern double emlite_val_get_value_double(Handle);
extern char *emlite_val_get_value_string(Handle);
extern Handle emlite_val_get_elem(Handle, size_t);
extern bool emlite_val_is_string(Handle);
extern bool emlite_val_is_number(Handle);
extern bool emlite_val_not(Handle);
extern bool emlite_val_gt(Handle, Handle);
extern bool emlite_val_gte(Handle, Handle);
extern bool emlite_val_lt(Handle, Handle);
extern bool emlite_val_lte(Handle, Handle);
extern bool emlite_val_equals(Handle, Handle);
extern bool emlite_val_strictly_equals(Handle, Handle);
extern bool emlite_val_instanceof(Handle, Handle);
extern void emlite_val_throw(Handle);
extern Handle emlite_val_obj_call(
    Handle obj, const char *name, size_t len, Handle argv
);
extern Handle emlite_val_obj_prop(
    Handle obj, const char *prop, size_t len
);
extern void emlite_val_obj_set_prop(
    Handle obj, const char *prop, size_t len, Handle val
);
extern bool emlite_val_obj_has_prop(
    Handle, const char *prop, size_t len
);
extern bool emlite_val_obj_has_own_prop(
    Handle, const char *prop, size_t len
);
extern Handle emlite_val_make_callback(Handle id);
extern void emlite_print_object_map(void);
extern void emlite_reset_object_map(void);
extern void   emlite_val_inc_ref(Handle);
extern void   emlite_val_dec_ref(Handle);
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
em_Val em_Val_make_fn(Callback f);
void em_Val_delete(em_Val);
void em_Val_throw(em_Val);

Handle em_Val_as_handle(em_Val self);
em_Val em_Val_get(em_Val self, const char *prop);
void em_Val_set(em_Val self, const char *prop, em_Val val);
bool em_Val_has(em_Val self, const char *prop);
bool em_Val_has_own_property(em_Val self, const char *prop);
char *em_Val_typeof(em_Val self);
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
char *em_Val_as_string(em_Val self);

em_Val em_Val_call(
    em_Val self, const char *method, int n, ...
);
em_Val em_Val_new(em_Val self, int n, ...);
em_Val em_Val_invoke(em_Val self, int n, ...);

em_Val emlite_eval(const char *src);

em_Val emlite_eval_v(const char *src, ...);
#define EMLITE_EVAL(x, ...)                                \
    emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__))

#ifdef EMLITE_IMPL
#include "emlite_impl.i"
#endif

#ifdef __cplusplus
}
#endif