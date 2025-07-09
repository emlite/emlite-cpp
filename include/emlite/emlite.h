#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EMLITE_USED
#define EMLITE_USED __attribute__((used, visibility("default")))
#endif

#ifndef EMLITE_IMPORT
#define EMLITE_IMPORT(NAME)                                \
    __attribute__((                                        \
        import_module("env"), import_name(#NAME)           \
    ))
#endif

#ifndef EMLITE_EXPORT
#define EMLITE_EXPORT(NAME)                                \
    __attribute__((                                        \
        export_name(#NAME)                                 \
    ))
#endif

EMLITE_USED int emlite_target(void);

// these are freestanding headers
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int snprintf(char *out, size_t n, const char *fmt, ...);
int vsnprintf(
    char *out, size_t n, const char *fmt, va_list ap
);
EMLITE_USED void *emlite_malloc(size_t);
EMLITE_USED void *emlite_realloc(void *, size_t);
EMLITE_USED void emlite_free(void *);

#if __has_include(<stdlib.h>)
#include <stdlib.h>
#else
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

/// A javascript raw handle
typedef uint32_t Handle;

/// Represents a javascript object
typedef Handle (*Callback)(Handle, Handle data);

/// @returns a null handle
extern Handle emlite_val_null(void);
/// @returns a js undefined handle
extern Handle emlite_val_undefined(void);
/// @returns a false value handle
extern Handle emlite_val_false(void);
/// @returns a true value handle
extern Handle emlite_val_true(void);
/// @returns a the globalThis handle
extern Handle emlite_val_global_this(void);
/// @returns a new array handle
extern Handle emlite_val_new_array(void);
/// @returns a new js Object
extern Handle emlite_val_new_object(void);
/// @returns the typeof of the underlying js value
extern char *emlite_val_typeof(Handle);
/// Constructs a new js value calling its constructor and
/// passing it @param argv then @returns its handle
extern Handle emlite_val_construct_new(Handle, Handle argv);
/// Calls the specified func via its handle @param func
/// Passes it @param argv
/// @returns the result of the call
extern Handle emlite_val_func_call(
    Handle func, Handle argv
);
/// Pushes a js object represented by @param v
/// to array @param arr
extern void emlite_val_push(Handle arr, Handle v);
/// Creates an integer value on the js side and returns its
/// handle
extern Handle emlite_val_make_int(int t);
/// Creates a double value on the js side and returns its
/// handle
extern Handle emlite_val_make_double(double t);
/// Creates a string on the js side and returns its handle.
/// The created string requires deallocation on the caller
/// side
extern Handle emlite_val_make_str(const char *, size_t);
/// @returns the underlying integer value of the js object
/// represted by a Handle
extern int emlite_val_get_value_int(Handle);
/// @returns the underlying double value of the js object
/// represted by a Handle
extern double emlite_val_get_value_double(Handle);
/// @returns the underlying string value of the js object
/// represted by a Handle
extern char *emlite_val_get_value_string(Handle);
/// @returns the element's Handle at the specified index
extern Handle emlite_val_get(Handle, Handle);
/// @returns the element's Handle at the specified index
extern void emlite_val_set(Handle, Handle, Handle);
/// Checks whether an object has a property
extern bool emlite_val_has(Handle, Handle);
/// @returns whether the Handle is a string
extern bool emlite_val_is_string(Handle);
/// @returns whether the Handle is a number
extern bool emlite_val_is_number(Handle);
extern bool emlite_val_not(Handle);
/// @returns whether a Handle is greater than another
extern bool emlite_val_gt(Handle, Handle);
/// @returns whether a Handle is greater than or equals
/// another
extern bool emlite_val_gte(Handle, Handle);
/// @returns whether a Handle is less than another
extern bool emlite_val_lt(Handle, Handle);
/// @returns whether a Handle is less than or equals another
extern bool emlite_val_lte(Handle, Handle);
/// @returns whether a Handle equals another
extern bool emlite_val_equals(Handle, Handle);
/// @returns whether a Handle strictly equals another
extern bool emlite_val_strictly_equals(Handle, Handle);
/// @returns whether a Handle is an instanceof another
extern bool emlite_val_instanceof(Handle, Handle);
/// Throws a js object represented by the passed Handle
extern void emlite_val_throw(Handle);
/// Calls an object's method by name
extern Handle emlite_val_obj_call(
    Handle obj, const char *name, size_t len, Handle argv
);
/// Checks whether an object has a non-inherited property
extern bool emlite_val_obj_has_own_prop(
    Handle, const char *prop, size_t len
);
/// Creates a callback handle
extern Handle emlite_val_make_callback(
    Handle id, Handle data
);
/// Print the js OBJECT_MAP
extern void emlite_print_object_map(void);
/// Reset the OBJECT_MAP
extern void emlite_reset_object_map(void);
/// Increment the refcount of a handle
extern void emlite_val_inc_ref(Handle);
/// Decrement the refcount of a handle
extern void emlite_val_dec_ref(Handle);

/// A higher level wrapper around a Handle
typedef struct {
    Handle h;
} em_Val;

/// Create an em_Val from an integer value @param i
em_Val em_Val_from_int(int i);
/// Create an em_Val from a double value @param i
em_Val em_Val_from_double(double i);
/// Create an em_Val from a string @param i
em_Val em_Val_from_string(const char *i);
/// Create an em_Val from a raw handle @param v
em_Val em_Val_from_handle(Handle v);
/// Gets a global object by name @param name
em_Val em_Val_global(const char *name);
/// Gets globalThis
em_Val em_Val_global_this();
/// Gets a javascript null
em_Val em_Val_null();
/// Gets a javascript undefined
em_Val em_Val_undefined();
/// Gets a new javascript object
em_Val em_Val_object();
/// Gets a new javascript array
em_Val em_Val_array();
/// Creates a javascript function from a C function @param f
em_Val em_Val_make_fn(Callback f, Handle data);
/// Decrements the refcount of the Handle represented by the
/// em_Val
void em_Val_delete(em_Val);
/// Throws an em_Val object
void em_Val_throw(em_Val);

/// @returns the underlying handle of @param self
Handle em_Val_as_handle(em_Val self);
/// Gets the em_Val's property @param prop
em_Val em_Val_get(em_Val self, em_Val prop);
/// Sets the em_Val's property @param prop to @param val
void em_Val_set(em_Val self, em_Val prop, em_Val val);
/// Checks whether an em_Val has a certain property @param
/// prop
bool em_Val_has(em_Val self, em_Val prop);
/// Checks whether an em_Val has a certain property @param
/// prop that's not inherited
bool em_Val_has_own_property(em_Val self, const char *prop);
/// Returns the typeof the underlying javascript object.
/// @returns an allocated string which needs deallocation on
/// the caller side
char *em_Val_typeof(em_Val self);
/// Returns the element at index @param idx
em_Val em_Val_at(em_Val self, em_Val idx);
/// Awaits the em_Val function object
em_Val em_Val_await(em_Val self);
/// Checks whether the underlying type is a number
bool em_Val_is_number(em_Val self);
/// Checks whether the underlying type is a string
bool em_Val_is_string(em_Val self);
/// Checks whether @param self is an instanceof @param v
bool em_Val_instanceof(em_Val self, em_Val v);
bool em_Val_not(em_Val self);
/// Checks whether @param self strictly equals @param other
bool em_Val_seq(em_Val self, em_Val other);
/// Checks whether @param self equals @param other
bool em_Val_eq(em_Val self, em_Val other);
/// Checks whether @param self doesn't equal @param other
bool em_Val_neq(em_Val self, em_Val other);
/// Checks whether @param self is greater than @param other
bool em_Val_gt(em_Val self, em_Val other);
/// Checks whether @param self is greater than or equals
/// @param other
bool em_Val_gte(em_Val self, em_Val other);
/// Checks whether @param self is less than @param other
bool em_Val_lt(em_Val self, em_Val other);
/// Checks whether @param self is less than or equals @param
/// other
bool em_Val_lte(em_Val self, em_Val other);

/// Returns the underlying int representation of the js
/// object
int em_Val_as_int(em_Val self);
/// Returns the underlying bool representation of the js
/// object
bool em_Val_as_bool(em_Val self);
/// Returns the underlying double representation of the js
/// object
double em_Val_as_double(em_Val self);
/// Returns the underlying string representation of the js
/// object.
/// @returns an allocated string that will need deallocation
/// on the caller side
char *em_Val_as_string(em_Val self);

/// Calls a javascript method @param method
/// of the underlying object.
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_call(
    em_Val self, const char *method, int n, ...
);

/// Calls a javascript object's constructor
/// @param n the number of arguments
/// @returns an em_Val object of the type of the caller
em_Val em_Val_new(em_Val self, int n, ...);

/// Invokes the function object
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_invoke(em_Val self, int n, ...);

/// Evaluates the string @param src in the js side
em_Val emlite_eval(const char *src);

/// Evaluates using printf-style arguments, with @param src
/// being the fmt string.
em_Val emlite_eval_v(const char *src, ...);

#define EMLITE_EVAL(x, ...)                                \
    emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__))

#ifdef EMLITE_IMPL
#ifdef EMLITE_USE_EMSCRIPTEN_JS_GLUE
#include "emlite_emscripten_impl.i"
#endif
#include "emlite_impl.i"
#endif

#ifdef __cplusplus
}
#endif