#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

#if __has_include(<vector>)
#include <vector>
#define EMLITE_HAS_STD_VECTOR 1
#else
#define EMLITE_HAS_STD_VECTOR 0
#endif

namespace emlite {

namespace detail {

template <typename T>
struct remove_reference {
    typedef T type;
};

template <typename T>
struct remove_reference<T &> {
    typedef T type;
};

template <typename T>
struct remove_reference<T &&> {
    typedef T type;
};

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &t) noexcept {
    return static_cast<T &&>(t);
}

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &&t) noexcept {
    static_assert(
        !__is_lvalue_reference(T), "forwarding an rvalue as an lvalue"
    );
    return static_cast<T &&>(t);
}

template <typename T, typename U>
struct is_same {
    static constexpr bool value = false;
};

template <typename T>
struct is_same<T, T> {
    static constexpr bool value = true;
};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

template <typename T>
struct is_integral {
    static constexpr bool value = false;
};

// Specializations for integral types
template <>
struct is_integral<bool> {
    static constexpr bool value = true;
};
template <>
struct is_integral<char> {
    static constexpr bool value = true;
};
template <>
struct is_integral<signed char> {
    static constexpr bool value = true;
};
template <>
struct is_integral<unsigned char> {
    static constexpr bool value = true;
};
template <>
struct is_integral<wchar_t> {
    static constexpr bool value = true;
};
template <>
struct is_integral<char16_t> {
    static constexpr bool value = true;
};
template <>
struct is_integral<char32_t> {
    static constexpr bool value = true;
};
template <>
struct is_integral<short> {
    static constexpr bool value = true;
};
template <>
struct is_integral<unsigned short> {
    static constexpr bool value = true;
};
template <>
struct is_integral<int> {
    static constexpr bool value = true;
};
template <>
struct is_integral<unsigned int> {
    static constexpr bool value = true;
};
template <>
struct is_integral<long> {
    static constexpr bool value = true;
};
template <>
struct is_integral<unsigned long> {
    static constexpr bool value = true;
};
template <>
struct is_integral<long long> {
    static constexpr bool value = true;
};
template <>
struct is_integral<unsigned long long> {
    static constexpr bool value = true;
};

template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;

template <typename T>
struct is_floating_point {
    static constexpr bool value = false;
};

template <>
struct is_floating_point<float> {
    static constexpr bool value = true;
};
template <>
struct is_floating_point<double> {
    static constexpr bool value = true;
};
template <>
struct is_floating_point<long double> {
    static constexpr bool value = true;
};

template <typename T>
constexpr bool is_floating_point_v = is_floating_point<T>::value;
} // namespace detail

template <typename T>
    requires(!detail::is_same_v<T, void>)
class UniqCPtr {
  private:
    T *ptr;

    UniqCPtr(const UniqCPtr &)            = delete;
    UniqCPtr &operator=(const UniqCPtr &) = delete;

  public:
    UniqCPtr() : ptr(nullptr) {}

    explicit UniqCPtr(T *p) : ptr(p) {}

    UniqCPtr(UniqCPtr &&other) : ptr(other.ptr) { other.ptr = nullptr; }

    UniqCPtr &operator=(UniqCPtr &&other) {
        if (this != &other) {
            if (ptr) {
                delete_(ptr);
            }
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~UniqCPtr() {
        if (ptr) {
            delete_(ptr);
        }
    }

    T &operator*() const { return *ptr; }
    T *operator->() const { return ptr; }

    T *get() const { return ptr; }

    T *release() {
        T *temp = ptr;
        ptr     = nullptr;
        return temp;
    }

    void reset(T *p = nullptr) {
        if (ptr) {
            delete_(ptr);
        }
        ptr = p;
    }

    operator bool() const { return ptr != nullptr; }

    void delete_(void *ptr) {
#if EMLITE_HAVE_LIBC_MALLOC
        free(ptr);
#else
        emlite_free(ptr);
#endif
    }
};

template <typename T>
    requires(!detail::is_same_v<T, void>)
class UniqCPtr<T[]> {
  private:
    T *ptr;

    UniqCPtr(const UniqCPtr &)            = delete;
    UniqCPtr &operator=(const UniqCPtr &) = delete;

  public:
    UniqCPtr() : ptr(nullptr) {}

    explicit UniqCPtr(T *p) : ptr(p) {}

    UniqCPtr(UniqCPtr &&other) : ptr(other.ptr) { other.ptr = nullptr; }

    UniqCPtr &operator=(UniqCPtr &&other) {
        if (this != &other) {
            if (ptr) {
                delete_(ptr);
            }
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~UniqCPtr() {
        if (ptr) {
            delete_(ptr);
        }
    }

    T& operator[](size_t i) const { return ptr[i]; }

    T *get() const { return ptr; }

    T *release() {
        T *temp = ptr;
        ptr     = nullptr;
        return temp;
    }

    void reset(T *p = nullptr) {
        if (ptr) {
            delete_(ptr);
        }
        ptr = p;
    }

    operator bool() const { return ptr != nullptr; }

    void delete_(void *ptr) {
#if EMLITE_HAVE_LIBC_MALLOC
        free(ptr);
#else
        emlite_free(ptr);
#endif
    }
};

class Val {
    Handle v_;
    Val();

  public:
    static Val from_handle(uint32_t v);
    static Val global(const char *name);
    static Val global();
    static Val null();
    static Val undefined();
    static Val object();
    static Val array();
    static Val make_js_function(Callback f);
    static void delete_(Val);
    static void throw_(Val);

    template <typename T>
    explicit Val(T v)
        requires(detail::is_integral_v<T> || detail::is_floating_point_v<T>)
        : v_(0) {
        if constexpr (detail::is_integral_v<T>) {
            v_ = emlite_val_make_int(v);
        } else {
            v_ = emlite_val_make_double(v);
        }
    }
    explicit Val(const char *v);
    explicit Val(Callback f);

    Handle as_handle() const;
    Val get(const char *prop) const;
    void set(const char *prop, const Val &val) const;
    bool has(const char *prop) const;
    bool has_own_property(const char *prop) const;
    UniqCPtr<char[]> type_of() const;
    Val operator[](size_t idx) const;
    Val await() const;
    bool is_number() const;
    bool is_string() const;
    bool instanceof (const Val &v) const;
    bool operator!() const;
    bool operator==(const Val &other) const;
    bool operator!=(const Val &other) const;
    bool operator>(const Val &other) const;
    bool operator>=(const Val &other) const;
    bool operator<(const Val &other) const;
    bool operator<=(const Val &other) const;

    template <class... Args>
    Val call(const char *method, Args &&...vals) const;

    template <class... Args>
    Val new_(Args &&...vals) const;

    template <class... Args>
    Val operator()(Args &&...vals) const;

    template <typename T>
    T as() const;

#if EMLITE_HAS_STD_VECTOR
    template <typename T>
    static std::vector<T> vec_from_js_array(const Val &v)
        requires(detail::is_integral_v<T> || detail::is_floating_point_v<T>)
    {
        auto sz = v.get("length").as<int>();
        std::vector<T> ret;
        ret.reserve(sz);
        for (int i = 0; i < sz; i++) {
            ret.push_back(v[i].as<T>());
        }
        return ret;
    }
#endif
};

class Console : public Val {
  public:
    Console();
    template <class... Args>
    void log(Args &&...args) const;
};

template <class... Args>
Val Val::call(const char *method, Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(arr, detail::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_obj_call(v_, method, strlen(method), arr)
    );
}

template <class... Args>
Val Val::new_(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(arr, detail::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_construct_new(v_, arr));
}

template <class... Args>
Val Val::operator()(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(arr, detail::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_func_call(v_, arr));
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
    else if constexpr (detail::is_same_v<T, UniqCPtr<char[]>>)
        return UniqCPtr<char[]>(emlite_val_get_value_string(v_));
    return T{};
}

template <class... Args>
void Console::log(Args &&...args) const {
    call("log", detail::forward<Args>(args)...);
}

#define EMLITE_EVAL(x, ...)                                                    \
    Val::from_handle(                                                          \
        em_Val_as_handle(emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__)))          \
    )

} // namespace emlite

#ifdef EMLITE_IMPL
namespace emlite {
Val::Val() : v_(0) {}

Val Val::from_handle(uint32_t v) {
    Val val;
    val.v_ = v;
    return val;
}

Val Val::global(const char *v) {
    return Val::from_handle(emlite_val_global_this()).get(v);
}

Val Val::global() { return Val::from_handle(emlite_val_global_this()); }

Val Val::null() { return Val::from_handle(0); }

Val Val::undefined() { return Val::from_handle(1); }

Val Val::object() {
    Val val;
    val.v_ = emlite_val_new_object();
    return val;
}

Val Val::array() {
    Val val;
    val.v_ = emlite_val_new_array();
    return val;
}

void Val::delete_(Val v) { emlite_val_delete(v.v_); }

void Val::throw_(Val v) { return emlite_val_throw(v.v_); }

Val::Val(const char *v) : v_(emlite_val_make_str(v, strlen(v))) {}

Val::Val(Callback f) : v_(Val::make_js_function(f).as_handle()) {}

Handle Val::as_handle() const { return v_; }

UniqCPtr<char[]> Val::type_of() const { return UniqCPtr<char[]>(emlite_val_typeof(v_)); }

Val Val::get(const char *prop) const {
    return Val::from_handle(emlite_val_obj_prop(v_, prop, strlen(prop)));
}

void Val::set(const char *prop, const Val &val) const {
    emlite_val_obj_set_prop(v_, prop, strlen(prop), val.as_handle());
}

bool Val::has(const char *prop) const {
    return emlite_val_obj_has_prop(v_, prop, strlen(prop));
}

bool Val::has_own_property(const char *prop) const {
    return emlite_val_obj_has_own_prop(v_, prop, strlen(prop));
}

Val Val::operator[](size_t idx) const {
    return Val::from_handle(emlite_val_get_elem(v_, idx));
}

Val Val::make_js_function(Callback f) {
    uint32_t fidx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f));
    return Val::from_handle(emlite_val_make_callback(fidx));
}

// clang-format off
Val Val::await() const {
    return EMLITE_EVAL({
       (async () => {
        let obj = ValMap.toValue(%d);
        let ret = await obj;
        return ValMap.toHandle(ret);
       })()
    }, v_);
}

bool Val::is_number() const {
    return emlite_val_is_number(v_);
}

bool Val::is_string() const {
    return emlite_val_is_string(v_);
}

bool Val::instanceof(const Val &v) const {
    return emlite_val_instanceof(v_, v.v_);
}

bool Val::operator!() const {
    return emlite_val_not(v_);
}

bool Val::operator==(const Val& other) const {
    return emlite_val_strictly_equals(v_, other.v_);
}

bool Val::operator!=(const Val& other) const {
    return !emlite_val_strictly_equals(v_, other.v_);
}

bool Val::operator>(const Val& other) const {
    return emlite_val_gt(v_, other.v_);
}

bool Val::operator>=(const Val& other) const {
    return emlite_val_gte(v_, other.v_);
}

bool Val::operator<(const Val& other) const {
    return emlite_val_lt(v_, other.v_);
}

bool Val::operator<=(const Val& other) const {
    return emlite_val_lte(v_, other.v_);
}
// clang-format on

Console::Console() : Val(Val::global("console")) {}
} // namespace emlite
#endif
