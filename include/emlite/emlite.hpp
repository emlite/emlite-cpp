#pragma once

#include "detail/externs.h"

#if __has_include(<new>)
#include <new>
#define EMLITE_HAVE_STD_NEW 1
#else
#define EMLITE_HAVE_STD_NEW 0
#endif

#if !EMLITE_HAVE_STD_NEW
extern "C++" void *operator new(size_t, void *) noexcept;
#endif

namespace emlite {

namespace detail {
#include "detail/func.hpp"
#include "detail/mem.hpp"
#include "detail/tiny_traits.hpp"
#include "detail/utils.hpp"

// Type traits for Option detection
template <typename T>
struct is_option : false_type {};

template <typename T>
struct is_option<Option<T>> : true_type {};

template <typename T>
constexpr bool is_option_v = is_option<T>::value;

// Type traits for Result detection
template <typename T>
struct is_result : false_type {};

template <typename T, typename E>
struct is_result<Result<T, E>> : true_type {};

template <typename T>
constexpr bool is_result_v = is_result<T>::value;

template <typename... Args, size_t... I, typename F, typename P>
constexpr decltype(auto) call_with_params_impl(F &&f, P &&p, index_sequence<I...>) {
    return forward<F>(f)(forward<P>(p).vals[I].template as<Args>()...);
}

template <typename... Args, typename F, typename P>
constexpr decltype(auto) call_with_params(F &&f, P &&p) {
    return call_with_params_impl<Args...>(
        forward<F>(f), forward<P>(p), make_index_sequence<sizeof...(Args)>{}
    );
}
} // namespace detail

using detail::Closure;
using detail::none;
using detail::nullopt;
using detail::Option;
using detail::Result;
using detail::some;
using detail::ok;
using detail::err;
using detail::Uniq;

class Val;

struct Params {
    Val *vals;
    size_t len;
};

/// A high-level RAII wrapper around javascript Handle's
class Val {
    Handle v_;
    Val() noexcept;

  public:
    /// The copy constructor. This increments the refcount
    /// of the javascript object
    Val(const Val &other) noexcept;
    /// The copy assignment operator. This increments the
    /// refcount of the javascript object
    Val &operator=(const Val &other) noexcept;
    /// The default move constructor. This changes ownership
    /// without incrementing the ref count.
    Val(Val &&other) noexcept;
    /// The move assignment operator. This doesn't increment
    /// the ref count
    Val &operator=(Val &&other) noexcept;
    /// The destructor, this decrements the ref count.
    virtual ~Val();

    /// Clone Val into a new val, this increments the
    /// refcount
    [[nodiscard]] Val clone() const noexcept;

    /// Creates a new Val object from a raw handle.
    /// @param v is a raw javascript handle
    /// @returns a Val object
    static Val take_ownership(Handle v) noexcept;
    /// Gets a global object by its name.
    /// @param name the name of the object
    static Val global(const char *name) noexcept;
    /// Gets the globalThis object.
    static Val global() noexcept;
    /// Returns a javascript null
    static Val null() noexcept;
    /// Returns a javascript undefined
    static Val undefined() noexcept;
    /// Returns a javascript empty object
    static Val object() noexcept;
    /// Returns an empty javascript array
    static Val array() noexcept;
    /// Creates a javascript function
    /// @param f is function pointer of type Handle
    /// (*)(Handle)
    static Val make_fn(Callback f, const Val &data = Val::null()) noexcept;
    static Val make_fn(Closure<Val(Params)> &&f) noexcept;
    template <typename Ret, typename... Args, typename F>
    static Val make_fn(F &&f) noexcept {
        return make_fn([=](Params p) -> Val {
            // maybe check length of p.len against
            // sizeof...(Args)
            if constexpr (!detail::is_same_v<void, Ret>)
                return Val(detail::call_with_params<Args...>(detail::forward<F>(f), p));
            else {
                detail::call_with_params<Args...>((Closure<Ret(Args...)> &&)f, p);
                return Val::undefined();
            }
        });
    }
    /// Deletes a Val object
    /// @param v has its refcount decremented
    static void delete_(Val &&v) noexcept;
    /// Throws a Val on the js side
    /// @param v the object thrown
    static void throw_(const Val &v);
    /// Creates a new Val from a Handle, while also
    /// incrementing its refcount
    /// @param h the Handle to duplicate
    static Val dup(Handle h) noexcept;
    /// Releases the underlying handle from the passed Val
    /// paramater
    static Handle release(Val &&v) noexcept;

    /// A Val constructor from numeric types
    /// @tparam T any numeric value which conforms to
    /// is_integral or is_floating_point or a string or has
    /// a `T::as_handle()` method
  private:
    /// Template helper to dispatch integer types to
    /// appropriate creation functions
    template <typename T>
    static Handle make_integer_value(T value) noexcept {
        if constexpr (detail::is_same_v<T, bool>) {
            return emlite_val_make_bool(value ? 1 : 0);
        } else if constexpr (sizeof(T) <= 4 && detail::is_signed_v<T>) {
            // int8_t, int16_t, int32_t, short, int (if
            // 32-bit), signed char
            return emlite_val_make_int(static_cast<int>(value));
        } else if constexpr (sizeof(T) <= 4 && !detail::is_signed_v<T>) {
            // uint8_t, uint16_t, uint32_t, unsigned short,
            // unsigned int (if 32-bit), unsigned char
            return emlite_val_make_uint(static_cast<unsigned int>(value));
        } else if constexpr (sizeof(T) == 8 && detail::is_signed_v<T>) {
            // int64_t, long long, long (if 64-bit)
            return emlite_val_make_bigint(static_cast<long long>(value));
        } else if constexpr (sizeof(T) == 8 && !detail::is_signed_v<T>) {
            // uint64_t, unsigned long long, size_t (if
            // 64-bit)
            return emlite_val_make_biguint(static_cast<unsigned long long>(value));
        } else {
            // Fallback for unusual integer types
            return emlite_val_make_int(static_cast<int>(value));
        }
    }

    /// Template helper to dispatch integer types to
    /// appropriate getter functions
    template <typename T>
    T get_integer_value(Handle h) const noexcept {
        if constexpr (detail::is_same_v<T, bool>) {
            return !emlite_val_not(h);
        } else if constexpr (sizeof(T) <= 4 && detail::is_signed_v<T>) {
            // int8_t, int16_t, int32_t, short, int (if
            // 32-bit), signed char
            return static_cast<T>(emlite_val_get_value_int(h));
        } else if constexpr (sizeof(T) <= 4 && !detail::is_signed_v<T>) {
            // uint8_t, uint16_t, uint32_t, unsigned short,
            // unsigned int (if 32-bit), unsigned char
            return static_cast<T>(emlite_val_get_value_uint(h));
        } else if constexpr (sizeof(T) == 8 && detail::is_signed_v<T>) {
            // int64_t, long long, long (if 64-bit)
            return static_cast<T>(emlite_val_get_value_bigint(h));
        } else if constexpr (sizeof(T) == 8 && !detail::is_signed_v<T>) {
            // uint64_t, unsigned long long, size_t (if
            // 64-bit)
            return static_cast<T>(emlite_val_get_value_biguint(h));
        } else {
            // Fallback for unusual integer types
            return static_cast<T>(emlite_val_get_value_int(h));
        }
    }

  public:
    template <typename T>
    explicit Val(T v) noexcept : v_(0) {
        if constexpr (detail::is_integral_v<T>) {
            v_ = make_integer_value(v); // No overflow, preserves signedness
        } else if constexpr (detail::is_floating_point_v<T>) {
            v_ = emlite_val_make_double(v);
        } else if constexpr (detail::is_same_v<T, const char *> || detail::is_same_v<T, char *>) {
            v_ = emlite_val_make_str(v, strlen(v));
        } else if constexpr (detail::is_same_v<T, const char16_t *> || detail::is_same_v<T, char16_t *>) {
            // Calculate length of char16_t string
            size_t len = 0;
            const char16_t *ptr = v;
            while (ptr && *ptr != 0) { ++len; ++ptr; }
            v_ = emlite_val_make_str_utf16(v, len);
        } else {
            emlite_val_inc_ref(v.as_handle());
            v_ = v.as_handle();
        }
    }

    /// @returns the raw javascript handle from this Val
    [[nodiscard]] Handle as_handle() const noexcept __attribute__((always_inline));
    /// Get the Val object's property
    /// @param prop the property name
    template <typename T>
    [[nodiscard]] Val get(T &&prop) const {
        return Val::take_ownership(emlite_val_get(v_, Val(detail::forward<T>(prop)).as_handle()));
    }
    /// Set the Val object's property
    /// @param prop the property name
    /// @param val the property's value
    template <typename T, typename U>
    void set(T &&prop, U &&v) const {
        emlite_val_set(
            v_, Val(detail::forward<T>(prop)).as_handle(), Val(detail::forward<U>(v)).as_handle()
        );
    }
    /// Checks whether a property exists
    /// @param prop the property to check
    template <typename T>
    bool has(T &&prop) const {
        return emlite_val_has(v_, Val(detail::forward<T>(prop)).as_handle());
    }
    /// Determine whether an object possesses a direct,
    /// own property with a specified name,
    /// as opposed to an inherited property from its
    /// prototype chain
    /// @param prop the property name
    bool has_own_property(const char *prop) const noexcept;
    /// @returns a string indicating the type of the
    /// javascript object
    [[nodiscard]] Uniq<char[]> type_of() const noexcept;
    /// @returns an element in the array
    /// @param idx at the specified index
    template <typename T>
    Val operator[](T &&idx) const {
        return get(detail::forward<T>(idx));
    }
    /// Awaits the function object
    [[nodiscard]] Val await() const;
    /// @returns bool if Val is a number
    [[nodiscard]] bool is_bool() const noexcept;
    /// @returns bool if Val is a number
    [[nodiscard]] bool is_number() const noexcept;
    /// @returns bool if Val is a string
    [[nodiscard]] bool is_string() const noexcept;
    /// @returns bool if Val is a function
    [[nodiscard]] bool is_function() const noexcept;
    /// @returns bool if Val is an error
    [[nodiscard]] bool is_error() const noexcept;
    /// @returns bool if Val is undefined
    [[nodiscard]] bool is_undefined() const noexcept;
    /// @returns bool if Val is null
    [[nodiscard]] bool is_null() const noexcept;
    /// @returns bool if Val is an instanceof
    /// @param v the other Val
    [[nodiscard]] bool instanceof (const Val &v) const noexcept;
    /// Not applied to Val
    bool operator!() const;
    /// @returns whether this Val strictly equals
    /// @param other the other Val
    bool operator==(const Val &other) const;
    /// @returns whether this Val doesn't equal
    /// @param other the other Val
    bool operator!=(const Val &other) const;
    /// @returns whether this Val is greater than
    /// @param other the other Val
    bool operator>(const Val &other) const;
    /// @returns whether this Val is greater than or equals
    /// @param other the other Val
    bool operator>=(const Val &other) const;
    /// @returns whether this Val is less than
    /// @param other the other Val
    bool operator<(const Val &other) const;
    /// @returns whether this Val is less than or equals
    /// @param other the other Val
    bool operator<=(const Val &other) const;

    /// Calls the specified method of the Val object
    /// @param method the method name
    /// @tparam the arguments to the method should be of
    /// type Val or derived from it
    /// @param vals the arguments to the method
    /// @returns a Val object which also could be undefined
    /// in js terms
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    Val call(const char *method, Args &&...vals) const noexcept;

    /// Calls the specified constructor of the Val object
    /// @tparam the arguments to the method should be of
    /// type Val or derived from it
    /// @param vals the arguments to the constructor
    /// @returns a Val object
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    Val new_(Args &&...vals) const;

    /// Invokes the function object represented by Val
    /// @tparam the arguments to the method should be of
    /// type Val or derived from it
    /// @param vals the arguments the invocation
    /// @returns a Val object which also could be undefined
    /// in js terms
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    Val operator()(Args &&...vals) const;

    /// @tparam the type of the returned  object
    /// @returns the underlying value of the Val object if
    /// possible. requires that the underlying type is a
    /// numeric or string, or a type which has a
    /// `take_ownership` static method which returns Val
    template <typename T>
    [[nodiscard]] T as() const noexcept;

    template <typename T>
    [[nodiscard]] Option<T> safe_cast() const noexcept {
        if constexpr (detail::is_same_v<T, bool> && is_bool()) {
            return as<bool>();
        } else if constexpr (detail::is_integral_v<T> && is_number()) {
            return get_integer_value<T>(v_);
        } else if constexpr (detail::is_floating_point_v<T> && is_number()) {
            return as<T>();
        } else if constexpr (detail::is_same_v<T, Uniq<char[]>> && is_string()) {
            return as<T>();
        } else if constexpr (detail::is_same_v<T, Uniq<char16_t[]>> && is_string()) {
            return as<T>();
        } else if (instanceof (T::instance()) && detail::is_base_of_v<Val, T>) {
            return as<T>();
        } else {
            return nullopt;
        }
    }

    /// Converts a javascript array to a Uniq C++ array
    /// @tparam any numeric type
    /// @param v The Val representing the javascript array
    /// @param[in,out] len the length of the C++ array that
    /// was returned
    /// @returns a Uniq C++ array
    template <typename T>
    static Uniq<T[]> vec_from_js_array(const Val &v, size_t &len) {
        auto sz = v.get("length").as<int>();
        len     = sz;
        T *ret  = new T[sz];
        for (int i = 0; i < sz; i++) {
            ret[i] = v[i].as<T>();
        }
        return Uniq<T[]>(ret);
    }
};

/// A wrapper around a console js object
class Console : public Val {
  public:
    Console();
    /// Logs to the console
    /// @tparam the arguments to `log` should be of type Val
    /// or derived from it
    /// @param args the arguments passed to `log`
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    void log(Args &&...args) const;

    /// console.warn
    /// @tparam the arguments to `warn` should be of type
    /// Val or derived from it
    /// @param args the arguments passed to `warn`
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    void warn(Args &&...args) const;

    /// console.info
    /// @tparam the arguments to `info` should be of type
    /// Val or derived from it
    /// @param args the arguments passed to `info`
    template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
    void info(Args &&...args) const;
    void clear() const;
};

template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
Val Val::call(const char *method, Args &&...vals) const noexcept {
    auto arr                        = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {Val(detail::forward<Args>(vals))...};
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(emlite_val_obj_call(v_, method, strlen(method), arr.as_handle()));
}

template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
Val Val::new_(Args &&...vals) const {
    auto arr                        = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {Val(detail::forward<Args>(vals))...};
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(emlite_val_construct_new(v_, arr.as_handle()));
}

template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
Val Val::operator()(Args &&...vals) const {
    auto arr                        = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {Val(detail::forward<Args>(vals))...};
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(emlite_val_func_call(v_, arr.as_handle()));
}

template <typename T>
T Val::as() const noexcept {
    if constexpr (detail::is_option_v<T>) {
        // Option<U> - return Some(value) or None
        using U = typename T::value_type;
        // Simple type checking - return None for type
        // mismatches
        if constexpr (detail::is_integral_v<U>) {
            if (is_number()) {
                return T(get_integer_value<U>(v_));
            }
            return T(); // None
        } else if constexpr (detail::is_floating_point_v<U>) {
            if (is_number()) {
                return T(emlite_val_get_value_double(v_));
            }
            return T(); // None
        } else if constexpr (detail::is_same_v<U, Uniq<char[]>>) {
            if (is_string()) {
                auto str_ptr = emlite_val_get_value_string(v_);
                if (str_ptr) {
                    return T(Uniq<char[]>(str_ptr));
                }
            }
            return T(); // None
        } else if constexpr (detail::is_same_v<U, Uniq<char16_t[]>>) {
            if (is_string()) {
                auto str_ptr = (char16_t *)emlite_val_get_value_string_utf16(v_);
                if (str_ptr) {
                    return T(Uniq<char16_t[]>(str_ptr));
                }
            }
            return T(); // None
        } else {
            return T(U(*this));
        }
    } else if constexpr (detail::is_result_v<T>) {
        // Result<U, E> - return Ok(value) or Err(error)
        using U = typename T::value_type;
        using E = typename T::error_type;
        if constexpr (detail::is_integral_v<U>) {
            if (is_number()) {
                return ok<U, E>(get_integer_value<U>(v_));
            } else {
                if constexpr (detail::is_same_v<E, Val>) {
                    return err<U, E>(Val::global("Error").new_("Expected number"));
                } else {
                    return err<U, E>(*this);
                }
            }
        } else if constexpr (detail::is_floating_point_v<U>) {
            if (is_number()) {
                return ok<U, E>(emlite_val_get_value_double(v_));
            } else {
                if constexpr (detail::is_same_v<E, Val>) {
                    return err<U, E>(Val::global("Error").new_("Expected number"));
                } else {
                    return err<U, E>(*this);
                }
            }
        } else if constexpr (detail::is_same_v<U, Uniq<char[]>>) {
            if (is_string()) {
                auto str_ptr = emlite_val_get_value_string(v_);
                if (str_ptr) {
                    return ok<U, E>(Uniq<char[]>(str_ptr));
                }
            } else {
                if constexpr (detail::is_same_v<E, Val>) {
                    return T(Val::global("Error").new_("Expected string"));
                } else {
                    return T(*this);
                }
            }
        } else if constexpr (detail::is_same_v<U, Uniq<char16_t[]>>) {
            if (is_string()) {
                auto str_ptr = (char16_t *)emlite_val_get_value_string_utf16(v_);
                if (str_ptr) {
                    return ok<U, E>(Uniq<char16_t[]>(str_ptr));
                }
            } else {
                if constexpr (detail::is_same_v<E, Val>) {
                    return T(Val::global("Error").new_("Expected string"));
                } else {
                    return T(*this);
                }
            }
        } else {
            if (is_error()) {
                return err<U, E>(this->as<E>());
            } else {
                return ok<U, E>(this->as<U>());
            }
        }
    } else if constexpr (detail::is_integral_v<T>) {
        return get_integer_value<T>(v_); // Use type-specific getters
    } else if constexpr (detail::is_floating_point_v<T>)
        return emlite_val_get_value_double(v_);
    else if constexpr (detail::is_same_v<T, Uniq<char[]>>)
        return Uniq<char[]>(emlite_val_get_value_string(v_));
    else if constexpr (detail::is_same_v<T, Uniq<char16_t[]>>)
        return Uniq<char16_t[]>((char16_t *)emlite_val_get_value_string_utf16(v_));
    else {
        return T(*this);
    }
}

template <class... Args, typename detail::enable_if_t<detail::is_base_of_v<Val, Args>>...>
void Console::log(Args &&...args) const {
    call("log", detail::forward<Args>(args)...);
}

/// A helper function to run javascript eval using a string
/// literal and printf style arguments
template <typename... Args>
Val emlite_eval_cpp(const char *fmt, Args &&...args) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    auto len  = snprintf(NULL, 0, fmt, detail::forward<Args>(args)...);
    auto *ptr = (char *)malloc(len + 1);
    if (!ptr) {
        return Val::null();
    }
    (void)snprintf(ptr, len + 1, fmt, detail::forward<Args>(args)...);
#pragma clang diagnostic pop
    auto ret = Val::global("eval")(Val(ptr));
    free(ptr);
    return ret;
}

// Option method implementations
template <typename T>
const T &Option<T>::value() const {
    if (!has_value_) {
        Val::throw_(Val::global("Error").new_("Option has no value"));
    }
    return value_;
}

template <typename T>
T &Option<T>::value() {
    if (!has_value_) {
        Val::throw_(Val::global("Error").new_("Option has no value"));
    }
    return value_;
}

template <typename T>
T Option<T>::expect(const char *message) const {
    if (!has_value_) {
        Val::throw_(Val::global("Error").new_(message));
    }
    return value_;
}

// Result method implementations
template <typename T, typename E>
const T &Result<T, E>::value() const {
    if (!has_value_) {
        if (has_error_) {
            Val::throw_(error_);
        }
        Val::throw_(Val::global("Error").new_("Result has no value"));
    }
    return value_;
}

template <typename T, typename E>
T &Result<T, E>::value() {
    if (!has_value_) {
        if (has_error_) {
            Val::throw_(error_);
        }
        Val::throw_(Val::global("Error").new_("Result has no value"));
    }
    return value_;
}

template <typename T, typename E>
const E &Result<T, E>::error() const {
    if (!has_error_) {
        Val::throw_(Val::global("Error").new_("Result has no error"));
    }
    return error_;
}

} // namespace emlite

#define EMLITE_EVAL(x, ...) emlite::emlite_eval_cpp(#x __VA_OPT__(, __VA_ARGS__))
