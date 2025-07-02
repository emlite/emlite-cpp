#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

namespace emlite {

namespace detail {
#include "tiny_traits.hpp"
} // namespace detail

/// A smart pointer class with unique ownershipt
template <
    class T,
    typename = typename detail::enable_if_t<
        !detail::is_same_v<T, void>>>
class Uniq {
    T *ptr_ = nullptr;

  public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(decltype(nullptr)) noexcept
        : ptr_(nullptr) {}
    explicit Uniq(T *p) noexcept : ptr_(p) {}

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

    Uniq(Uniq &&other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    Uniq &operator=(Uniq &&other) noexcept {
        if (this != &other) {
            reset();
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq &other) noexcept {
        auto tmp   = ptr_;
        ptr_       = other.ptr_;
        other.ptr_ = tmp;
    }

    void reset(T *p = nullptr) noexcept {
        if (ptr_)
            delete ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T *release() noexcept {
        T *tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T *get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T &operator*() const noexcept { return *ptr_; }
    T *operator->() const noexcept { return ptr_; }
};

/// A specialization of Uniq, the smart pointer with unique
/// ownership, for array types
template <class T>
class Uniq<T[]> {
    T *ptr_ = nullptr;

  public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(decltype(nullptr)) noexcept
        : ptr_(nullptr) {}
    explicit Uniq(T *p) noexcept : ptr_(p) {}

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

    Uniq(Uniq &&other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    Uniq &operator=(Uniq &&other) noexcept {
        if (this != &other) {
            reset();
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq &other) noexcept {
        auto tmp   = ptr_;
        ptr_       = other.ptr_;
        other.ptr_ = tmp;
    }

    void reset(T *p = nullptr) noexcept {
        if (ptr_)
            delete[] ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T *release() noexcept {
        T *tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T *get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T &operator[](size_t i) const noexcept {
        return ptr_[i];
    }
};

/// A helper swap function for Uniq objects
template <class T>
inline void swap(Uniq<T> &a, Uniq<T> &b) noexcept {
    a.swap(b);
}

/// A helper swap function for Uniq objects
template <class T>
inline void swap(Uniq<T[]> &a, Uniq<T[]> &b) noexcept {
    a.swap(b);
}

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

    /// Clone Val into a new val, this increments the refcount
    Val clone() const noexcept;

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
    static Val make_fn(Callback f) noexcept;
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
    /// Releases the underlying handle from the passed Val paramater
    static Handle release(Val &&v) noexcept;

    /// A Val constructor from numeric types
    /// @tparam T any numeric value which conforms to
    /// is_integral or is_floating_point or a string or has
    /// a `T::as_handle()` method
    template <typename T>
    explicit Val(T v) noexcept : v_(0) {
        if constexpr (detail::is_integral_v<T>) {
            v_ = emlite_val_make_int(v);
        } else if constexpr (detail::is_floating_point_v<
                                 T>) {
            v_ = emlite_val_make_double(v);
        } else if constexpr (detail::is_same_v<
                                 T,
                                 const char *> ||
                             detail::is_same_v<T, char *>) {
            v_ = emlite_val_make_str(v, strlen(v));
        } else {
            emlite_val_inc_ref(v.as_handle());
            v_ = v.as_handle();
        }
    }

    /// @returns the raw javascript handle from this Val
    [[nodiscard]] Handle as_handle() const noexcept
        __attribute__((always_inline));
    /// Get the Val object's property
    /// @param prop the property name
    Val get(const char *prop) const noexcept;
    /// Set the Val object's property
    /// @param prop the property name
    /// @param val the property's value
    template <typename T>
    void set(const char *prop, T &&v) const noexcept {
        emlite_val_obj_set_prop(
            v_, prop, strlen(prop), Val(detail::forward<T>(v)).as_handle()
        );
    }
    /// Checks whether a property exists
    /// @param prop the property to check
    bool has(const char *prop) const noexcept;
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
    Val operator[](size_t idx) const;
    /// Awaits the function object
    [[nodiscard]] Val await() const;
    /// @returns bool if Val is a number
    [[nodiscard]] bool is_number() const noexcept;
    /// @returns bool if Val is a string
    [[nodiscard]] bool is_string() const noexcept;
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
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val call(const char *method, Args &&...vals) const noexcept;

    /// Calls the specified constructor of the Val object
    /// @tparam the arguments to the method should be of
    /// type Val or derived from it
    /// @param vals the arguments to the constructor
    /// @returns a Val object
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val new_(Args &&...vals) const;

    /// Invokes the function object represented by Val
    /// @tparam the arguments to the method should be of
    /// type Val or derived from it
    /// @param vals the arguments the invocation
    /// @returns a Val object which also could be undefined
    /// in js terms
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val operator()(Args &&...vals) const;

    /// @tparam the type of the returned  object
    /// @returns the underlying value of the Val object if
    /// possible. requires that the underlying type is a
    /// numeric or string, or a type which has a
    /// `take_ownership` static method which returns Val
    template <typename T>
    [[nodiscard]] T as() const noexcept;

    /// Converts a javascript array to a Uniq C++ array
    /// @tparam any numeric type
    /// @param v The Val representing the javascript array
    /// @param[in,out] len the length of the C++ array that
    /// was returned
    /// @returns a Uniq C++ array
    template <
        typename T>
    static Uniq<T[]> vec_from_js_array(
        const Val &v, size_t &len
    ) {
        auto sz = v.get("length").as<int>();
        len     = sz;
        T *ret  = new T[sz];
        for (int i = 0; i < sz; i++) {
            ret[i] = v[i].as<T>();
        }
        return Uniq<T[]>(ret);
    }
    
    template<>
    Uniq<Val[]> vec_from_js_array<Val>(const Val& v, size_t& len)
    {
        auto sz = v.get("length").as<int>();
        len     = sz;
        Val* ret = new Val[sz];

        for (int i = 0; i < sz; ++i) {
            auto val = v[i].as<Handle>();
            ret[i] = Val::take_ownership(val);
        }
        return Uniq<Val[]>(ret);
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
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    void log(Args &&...args) const;

    /// console.warn
    /// @tparam the arguments to `warn` should be of type
    /// Val or derived from it
    /// @param args the arguments passed to `warn`
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    void warn(Args &&...args) const;

    /// console.info
    /// @tparam the arguments to `info` should be of type
    /// Val or derived from it
    /// @param args the arguments passed to `info`
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    void info(Args &&...args) const;
};

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
Val Val::call(const char *method, Args &&...vals) const noexcept {
    auto arr = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {
        Val(detail::forward<Args>(vals))...
    };
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(emlite_val_obj_call(
        v_, method, strlen(method), arr.as_handle()
    ));
}

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
Val Val::new_(Args &&...vals) const {
    auto arr = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {
        Val(detail::forward<Args>(vals))...
    };
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(
        emlite_val_construct_new(v_, arr.as_handle())
    );
}

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
Val Val::operator()(Args &&...vals) const {
    auto arr = Val::take_ownership(emlite_val_new_array());
    Val keep_alive[sizeof...(Args)] = {
        Val(detail::forward<Args>(vals))...
    };
    for (auto &v : keep_alive)
        emlite_val_push(arr.as_handle(), v.as_handle());
    return Val::take_ownership(
        emlite_val_func_call(v_, arr.as_handle())
    );
}

template <typename T>
T Val::as() const noexcept {
    if constexpr (detail::is_integral_v<T>) {
        if constexpr (detail::is_same_v<T, bool>) {
            if (v_ > 3)
                return true;
            else
                return false;
        } else {
            return emlite_val_get_value_int(v_);
        }
    } else if constexpr (detail::is_floating_point_v<T>)
        return emlite_val_get_value_int(v_);
    else if constexpr (detail::is_same_v<T, Uniq<char[]>>)
        return Uniq<char[]>(emlite_val_get_value_string(v_)
        );
    else {
        emlite_val_inc_ref(v_);
        return T::take_ownership(v_);
    }
}

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
void Console::log(Args &&...args) const {
    call("log", detail::forward<Args>(args)...);
}

/// A helper function to run javascript eval using a string
/// literal and printf style arguments
template <typename... Args>
Val emlite_eval_cpp(const char *fmt, Args &&...args) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    auto len = snprintf(
        NULL, 0, fmt, detail::forward<Args>(args)...
    );
    auto *ptr = (char *)malloc(len + 1);
    if (!ptr) {
        return Val::null();
    }
    (void)snprintf(
        ptr, len + 1, fmt, detail::forward<Args>(args)...
    );
#pragma clang diagnostic pop
    auto ret = Val::global("eval")(Val(ptr));
    free(ptr);
    return ret;
}
} // namespace emlite

#define EMLITE_EVAL(x, ...)                                \
    emlite::emlite_eval_cpp(#x __VA_OPT__(, __VA_ARGS__))

#ifdef EMLITE_IMPL
#include "emlite_impl.ipp"
#endif
