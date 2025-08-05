#pragma once

#include "detail/externs.h"

#define EMLITE_EVAL(x, ...) emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__))

// clang-format off
#define em_Val_from(x) \
    _Generic((x), \
    _Bool: em_Val_from_bool, \
    char: em_Val_from_int, \
    signed char: em_Val_from_int, \
    unsigned char: em_Val_from_uint, \
    short: em_Val_from_int, \
    unsigned short: em_Val_from_uint, \
    int: em_Val_from_int, \
    unsigned int: em_Val_from_uint, \
    long: em_Val_from_int, \
    unsigned long: em_Val_from_uint, \
    long long: em_Val_from_bigint, \
    unsigned long long: em_Val_from_biguint, \
    float: em_Val_from_double, \
    double: em_Val_from_double, \
    long double: em_Val_from_double, \
    char *: em_Val_from_string, \
    const char *: em_Val_from_string, \
    uint16_t *: em_Val_from_string_utf16, \
    const uint16_t *: em_Val_from_string_utf16, \
    default: em_Val_from_val)( \
        x       \
    )
// clang-format on

#define em_Val_as(TYPE, VAL)                                                                       \
    _Generic(                                                                                      \
        ((TYPE){0}),                                                                               \
        _Bool: (!emlite_val_not(em_Val_as_handle(VAL))),                                           \
        char: (char)emlite_val_get_value_int(em_Val_as_handle(VAL)),                               \
        signed char: (signed char)emlite_val_get_value_int(em_Val_as_handle(VAL)),                 \
        unsigned char: (unsigned char)emlite_val_get_value_uint(em_Val_as_handle(VAL)),            \
        short: (short)emlite_val_get_value_int(em_Val_as_handle(VAL)),                             \
        unsigned short: (unsigned short)emlite_val_get_value_uint(em_Val_as_handle(VAL)),          \
        int: (int)emlite_val_get_value_int(em_Val_as_handle(VAL)),                                 \
        unsigned int: (unsigned int)emlite_val_get_value_uint(em_Val_as_handle(VAL)),              \
        long: (long)emlite_val_get_value_int(em_Val_as_handle(VAL)),                               \
        unsigned long: (unsigned long)emlite_val_get_value_uint(em_Val_as_handle(VAL)),            \
        long long: (long long)emlite_val_get_value_bigint(em_Val_as_handle(VAL)),                  \
        unsigned long long: (unsigned long long)emlite_val_get_value_biguint(em_Val_as_handle(VAL) \
        ),                                                                                         \
        float: (float)emlite_val_get_value_double(em_Val_as_handle(VAL)),                          \
        double: (double)emlite_val_get_value_double(em_Val_as_handle(VAL)),                        \
        long double: (long double)emlite_val_get_value_double(em_Val_as_handle(VAL)),              \
        char *: emlite_val_get_value_string(em_Val_as_handle(VAL)),                                \
        uint16_t *: emlite_val_get_value_string_utf16(em_Val_as_handle(VAL)),                      \
        default: *(TYPE *)&(em_Val){em_Val_as_handle(VAL)}                                         \
    )

#define EM_NARG_(...) ((int)(sizeof((em_Val[]){(em_Val){0}, ##__VA_ARGS__}) / sizeof(em_Val) - 1))

#define em_Val_call(self, method, ...)                                                             \
    em_Val_call_((self), (method), EM_NARG_(__VA_ARGS__), ##__VA_ARGS__)

#define em_Val_new(self, ...) em_Val_new_((self), EM_NARG_(__VA_ARGS__), ##__VA_ARGS__)

#define em_Val_invoke(self, ...) em_Val_invoke_((self), EM_NARG_(__VA_ARGS__), ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/// A higher level wrapper around a Handle
typedef struct {
    Handle h;
} em_Val;

/// Create an em_Val from a boolean value @param i
em_Val em_Val_from_bool(bool i);
/// Create an em_Val from an integer value @param i
em_Val em_Val_from_int(int i);
/// Create an em_Val from an unsigned integer value @param i
em_Val em_Val_from_uint(unsigned int i);
/// Create an em_Val from a big integer value @param i
em_Val em_Val_from_bigint(long long i);
/// Create an em_Val from a big unsigned integer value @param i
em_Val em_Val_from_biguint(unsigned long long i);
/// Create an em_Val from a double value @param i
em_Val em_Val_from_double(double i);
/// Create an em_Val from a string @param i
em_Val em_Val_from_string(const char *i);
/// Create an em_Val from a UTF-16 string @param i
em_Val em_Val_from_string_utf16(const uint16_t *i);
/// Create an em_Val from a val @param i
em_Val em_Val_from_val(void *i);
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
/// Decrements the refcount of the Handle represented by the em_Val
void em_Val_delete(em_Val);
/// Throws an em_Val object
void em_Val_throw(em_Val);

/// @returns the underlying handle of @param self
Handle em_Val_as_handle(em_Val self);
/// Gets the em_Val's property @param prop
em_Val em_Val_get(em_Val self, em_Val prop);
/// Sets the em_Val's property @param prop to @param val
void em_Val_set(em_Val self, em_Val prop, em_Val val);
/// Checks whether an em_Val has a certain property @param prop
bool em_Val_has(em_Val self, em_Val prop);
/// Checks whether an em_Val has a certain property @param prop that's not inherited
bool em_Val_has_own_property(em_Val self, const char *prop);
/// Returns the typeof the underlying javascript object.
/// @returns an allocated string which needs deallocation on the caller side
char *em_Val_typeof(em_Val self);
/// Returns the element at index @param idx
em_Val em_Val_at(em_Val self, em_Val idx);
/// Awaits the em_Val function object
em_Val em_Val_await(em_Val self);
/// Checks whether the underlying type is a boolean
bool em_Val_is_bool(em_Val self);
/// Checks whether the underlying type is a number
bool em_Val_is_number(em_Val self);
/// Checks whether the underlying type is a string
bool em_Val_is_string(em_Val self);
/// Checks whether the underlying type is a error
bool em_Val_is_error(em_Val self);
/// Checks whether the underlying type is a function
bool em_Val_is_function(em_Val self);
/// Checks whether the underlying type is a undefined
bool em_Val_is_undefined(em_Val self);
/// Checks whether the underlying type is a null
bool em_Val_is_null(em_Val self);
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
/// Checks whether @param self is greater than or equals @param other
bool em_Val_gte(em_Val self, em_Val other);
/// Checks whether @param self is less than @param other
bool em_Val_lt(em_Val self, em_Val other);
/// Checks whether @param self is less than or equals @param other
bool em_Val_lte(em_Val self, em_Val other);
/// Returns the underlying bool representation of the js object
bool em_Val_as_bool(em_Val self);
/// Returns the underlying int representation of the js object
int em_Val_as_int(em_Val self);
/// Returns the underlying uint representation of the js object
unsigned int em_Val_as_uint(em_Val self);
/// Returns the underlying bigint representation of the js object
long long em_Val_as_bigint(em_Val self);
/// Returns the underlying biguint representation of the js object
unsigned long long em_Val_as_biguint(em_Val self);
/// Returns the underlying double representation of the js object
double em_Val_as_double(em_Val self);
/// Returns the underlying string representation of the js object.
/// @returns an allocated string that will need deallocation on the caller side
char *em_Val_as_string(em_Val self);
/// Returns the underlying UTF-16 string representation of the js object.
/// @returns an allocated UTF-16 string that will need deallocation on the caller side
uint16_t *em_Val_as_string_utf16(em_Val self);

em_Val em_Val_as_val(em_Val self);

/// Calls a javascript method @param method
/// of the underlying object.
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_call_v(em_Val self, const char *method, int n, va_list ap);

/// Calls a javascript method @param method
/// of the underlying object.
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_call_(em_Val self, const char *method, int n, ...);

/// Calls a javascript object's constructor
/// @param n the number of arguments
/// @returns an em_Val object of the type of the caller
em_Val em_Val_new_v(em_Val self, int n, va_list ap);

/// Calls a javascript object's constructor
/// @param n the number of arguments
/// @returns an em_Val object of the type of the caller
em_Val em_Val_new_(em_Val self, int n, ...);

/// Invokes the function object
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_invoke_v(em_Val self, int n, va_list ap);

/// Invokes the function object
/// @param n the number of arguments
/// @returns an em_Val object which could be a js undefined
em_Val em_Val_invoke_(em_Val self, int n, ...);

/// Evaluates the string @param src in the js side
em_Val emlite_eval(const char *src);

/// Evaluates using printf-style arguments, with @param src
/// being the fmt string.
em_Val emlite_eval_v(const char *src, ...);

#ifdef __cplusplus
}
#endif