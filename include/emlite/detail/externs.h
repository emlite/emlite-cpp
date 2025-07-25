#pragma once

// these are freestanding headers
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifndef EMLITE_USED
#define EMLITE_USED                                        \
    __attribute__((used, visibility("default")))
#endif

#ifndef EMLITE_IMPORT
#define EMLITE_IMPORT(NAME)                                \
    __attribute__((                                        \
        import_module("env"), import_name(#NAME)           \
    ))
#endif

#ifndef EMLITE_EXPORT
#define EMLITE_EXPORT(NAME)                                \
    __attribute__((export_name(#NAME)))
#endif


/// A javascript raw handle
typedef uint32_t Handle;

/// Represents a javascript object
typedef Handle (*Callback)(Handle, Handle data);

enum EMLITE_PREDEFINED_HANDLES {
    EMLITE_NULL = 0,
    EMLITE_UNDEFINED,
    EMLITE_FALSE,
    EMLITE_TRUE,
    EMLITE_GLOBALTHIS,
    EMLITE_CONSOLE,
    EMLITE_RESERVED,
};

#ifdef __cplusplus
extern "C" {
#endif


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

EMLITE_USED int emlite_target(void);

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

#ifdef __cplusplus
}
#endif