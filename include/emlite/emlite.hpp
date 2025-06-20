#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

namespace emlite {

namespace detail {
#if __has_include(<type_traits>)
#include <type_traits>
using namespace std;
#else
#include "tiny_traits.hpp"
#endif
} // namespace detail

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

template <class T>
inline void swap(Uniq<T> &a, Uniq<T> &b) noexcept {
    a.swap(b);
}

template <class T>
inline void swap(Uniq<T[]> &a, Uniq<T[]> &b) noexcept {
    a.swap(b);
}

class Val {
    Handle v_;
    Val();

  public:
    Val(const Val &other);
    Val &operator=(const Val &other);
    Val &operator=(Val &&other) noexcept;
    Val(Val &&other) noexcept;
    ~Val();

    static Val take_ownership(Handle v);
    static Val global(const char *name);
    static Val global();
    static Val null();
    static Val undefined();
    static Val object();
    static Val array();
    static Val make_fn(Callback f);
    static void delete_(Val);
    static void throw_(Val);
    static Val dup(Handle);

    template <
        typename T,
        typename = typename detail::enable_if_t<
            detail::is_integral_v<T> ||
            detail::is_floating_point_v<T>>>
    explicit Val(T v) : v_(0) {
        if constexpr (detail::is_integral_v<T>) {
            v_ = emlite_val_make_int(v);
        } else {
            v_ = emlite_val_make_double(v);
        }
    }
    explicit Val(const char *v);

    [[nodiscard]] Handle as_handle() const
        __attribute__((always_inline));
    Val get(const char *prop) const;
    void set(const char *prop, const Val &val) const;
    bool has(const char *prop) const;
    bool has_own_property(const char *prop) const;
    [[nodiscard]] Uniq<char[]> type_of() const;
    Val operator[](size_t idx) const;
    [[nodiscard]] Val await() const;
    [[nodiscard]] bool is_number() const;
    [[nodiscard]] bool is_string() const;
    [[nodiscard]] bool instanceof (const Val &v) const;
    bool operator!() const;
    bool operator==(const Val &other) const;
    bool operator!=(const Val &other) const;
    bool operator>(const Val &other) const;
    bool operator>=(const Val &other) const;
    bool operator<(const Val &other) const;
    bool operator<=(const Val &other) const;

    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val call(const char *method, Args &&...vals) const;

    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val new_(Args &&...vals) const;

    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    Val operator()(Args &&...vals) const;

    template <typename T>
    [[nodiscard]] T as() const;

    template <
        typename T,
        typename = typename detail::enable_if_t<
            detail::is_integral_v<T> ||
            detail::is_floating_point_v<T>>>
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
};

class Console : public Val {
  public:
    Console();
    template <
        class... Args,
        typename detail::enable_if_t<
            detail::is_base_of_v<Val, Args>>...>
    void log(Args &&...args) const;
};

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
Val Val::call(const char *method, Args &&...vals) const {
    auto arr = Val::take_ownership(emlite_val_new_array());
    (emlite_val_push(
         arr.as_handle(),
         detail::forward<Args>(vals).as_handle()
     ),
     ...);
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
    (emlite_val_push(
         arr.as_handle(),
         detail::forward<Args>(vals).as_handle()
     ),
     ...);
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
    (emlite_val_push(
         arr.as_handle(),
         detail::forward<Args>(vals).as_handle()
     ),
     ...);
    return Val::take_ownership(
        emlite_val_func_call(v_, arr.as_handle())
    );
}

template <typename T>
T Val::as() const {
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
    else
        return T::take_ownership(v_);
}

template <
    class... Args,
    typename detail::enable_if_t<
        detail::is_base_of_v<Val, Args>>...>
void Console::log(Args &&...args) const {
    call("log", detail::forward<Args>(args)...);
}

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

#define EMLITE_EVAL(x, ...)                                \
    emlite_eval_cpp(#x __VA_OPT__(, __VA_ARGS__))

} // namespace emlite

#ifdef EMLITE_IMPL
#include "emlite_impl.ipp"
#endif
