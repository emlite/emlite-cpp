#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

namespace emlite {

namespace detail {

#if __has_include(<stdatomic.h>)
using refcount_t = _Atomic size_t;
#else
using refcount_t = size_t;
#endif

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
constexpr T &&forward(typename remove_reference<T>::type &t
) noexcept {
    return static_cast<T &&>(t);
}

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &&t
) noexcept {
    static_assert(
        !__is_lvalue_reference(T),
        "forwarding an rvalue as an lvalue"
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
constexpr bool is_floating_point_v =
    is_floating_point<T>::value;
} // namespace detail

template <typename T>
    requires(!detail::is_same_v<T, void>)
class Uniq {
  private:
    T *ptr;

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

  public:
    Uniq() : ptr(nullptr) {}

    explicit Uniq(T *p) : ptr(p) {}

    Uniq(Uniq &&other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    Uniq &operator=(Uniq &&other) {
        if (this != &other) {
            if (ptr) {
                delete ptr;
            }
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~Uniq() {
        if (ptr) {
            delete ptr;
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
            delete ptr;
        }
        ptr = p;
    }

    operator bool() const { return ptr != nullptr; }
};

template <typename T>
    requires(!detail::is_same_v<T, void>)
class Uniq<T[]> {
  private:
    T *ptr;

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

  public:
    Uniq() : ptr(nullptr) {}

    explicit Uniq(T *p) : ptr(p) {}

    Uniq(Uniq &&other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    Uniq &operator=(Uniq &&other) {
        if (this != &other) {
            if (ptr) {
                delete[] ptr;
            }
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~Uniq() {
        if (ptr) {
            delete[] ptr;
        }
    }

    T &operator[](size_t i) const { return ptr[i]; }

    T *get() const { return ptr; }

    T *release() {
        T *temp = ptr;
        ptr     = nullptr;
        return temp;
    }

    void reset(T *p = nullptr) {
        if (ptr) {
            delete[] ptr;
        }
        ptr = p;
    }

    operator bool() const { return ptr != nullptr; }
};

template <class T>
    requires(!detail::is_same_v<T, void>)
class Shared {
    struct Control {
        T value;
        void (*deleter)(T)      = nullptr;
        detail::refcount_t refs = 1;

        Control(T v, void (*d)(T)) : value(v), deleter(d) {}
    };

    Control *ctrl_ = nullptr;

    void inc() const {
        if (ctrl_)
            ++ctrl_->refs;
    }

    void dec() {
        if (!ctrl_)
            return;
        if (--ctrl_->refs == 0) {
            if (ctrl_->deleter)
                ctrl_->deleter(ctrl_->value);
            delete ctrl_;
        }
        ctrl_ = nullptr;
    }

  public:
    Shared() = default;

    Shared(T v, void (*d)(T) = nullptr)
        : ctrl_(new Control(v, d)) {}

    Shared(const Shared &o) : ctrl_(o.ctrl_) { inc(); }

    Shared(Shared &&o) noexcept : ctrl_(o.ctrl_) {
        o.ctrl_ = nullptr;
    }

    Shared &operator=(const Shared &o) {
        if (this != &o) {
            dec();
            ctrl_ = o.ctrl_;
            inc();
        }
        return *this;
    }

    Shared &operator=(Shared &&o) noexcept {
        if (this != &o) {
            dec();
            ctrl_   = o.ctrl_;
            o.ctrl_ = nullptr;
        }
        return *this;
    }

    ~Shared() { dec(); }
    T &operator*() const { return ctrl_->value; }

    T *operator->() const { return &ctrl_->value; }

    T *get() const {
        return ctrl_ ? &ctrl_->value : nullptr;
    }
    explicit operator bool() const {
        return ctrl_ != nullptr;
    }
    size_t use_count() const {
        return ctrl_ ? ctrl_->refs : 0;
    }
};

template <class T>
    requires(!detail::is_same_v<T, void>)
class Shared<T[]> {
    struct Control {
        T *ptr;
        void (*deleter)(T *);
        detail::refcount_t refs;

        Control(T *p, void (*d)(T *))
            : ptr(p), deleter(d), refs(1) {}
    };

    Control *ctrl_ = nullptr;

    void inc() const {
        if (ctrl_)
            ++ctrl_->refs;
    }

    void dec() {
        if (!ctrl_)
            return;
        if (--ctrl_->refs == 0) {
            if (ctrl_->deleter)
                ctrl_->deleter(ctrl_->ptr);
            delete ctrl_;
        }
        ctrl_ = nullptr;
    }

  public:
    Shared() = default;

    explicit Shared(
        T *p, void (*d)(T *) = [](T *q) { delete[] q; }
    )
        : ctrl_(p ? new Control(p, d) : nullptr) {}

    Shared(const Shared &o) : ctrl_(o.ctrl_) { inc(); }
    Shared(Shared &&o) noexcept : ctrl_(o.ctrl_) {
        o.ctrl_ = nullptr;
    }

    Shared &operator=(const Shared &o) {
        if (this != &o) {
            dec();
            ctrl_ = o.ctrl_;
            inc();
        }
        return *this;
    }
    Shared &operator=(Shared &&o) noexcept {
        if (this != &o) {
            dec();
            ctrl_   = o.ctrl_;
            o.ctrl_ = nullptr;
        }
        return *this;
    }

    ~Shared() { dec(); }

    T &operator[](size_t i) const { return ctrl_->ptr[i]; }

    T *get() const { return ctrl_ ? ctrl_->ptr : nullptr; }
    size_t use_count() const {
        return ctrl_ ? ctrl_->refs : 0;
    }
    explicit operator bool() const {
        return ctrl_ != nullptr;
    }
};

class Val {
    Handle v_;
    Val();

  public:
    static Val from_handle(Handle v);
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
    Uniq<char[]> type_of() const;
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

    template <typename T>
    static Uniq<T[]> vec_from_js_array(
        const Val &v, size_t &len
    )
        requires(detail::is_integral_v<T> || detail::is_floating_point_v<T>)
    {
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
    template <class... Args>
    void log(Args &&...args) const;
};

template <class... Args>
Val Val::call(const char *method, Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(
         arr, detail::forward<Args>(vals).as_handle()
     ),
     ...);
    return Val::from_handle(
        emlite_val_obj_call(v_, method, strlen(method), arr)
    );
}

template <class... Args>
Val Val::new_(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(
         arr, detail::forward<Args>(vals).as_handle()
     ),
     ...);
    return Val::from_handle(
        emlite_val_construct_new(v_, arr)
    );
}

template <class... Args>
Val Val::operator()(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(
         arr, detail::forward<Args>(vals).as_handle()
     ),
     ...);
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
    else if constexpr (detail::is_same_v<T, Uniq<char[]>>)
        return Uniq<char[]>(emlite_val_get_value_string(v_)
        );
    else
        return T::from_handle(v_);
}

template <class... Args>
void Console::log(Args &&...args) const {
    call("log", detail::forward<Args>(args)...);
}

#define EMLITE_EVAL(x, ...)                                \
    Val::from_handle(em_Val_as_handle(                     \
        emlite_eval_v(#x __VA_OPT__(, __VA_ARGS__))        \
    ))

} // namespace emlite

#ifdef EMLITE_IMPL
#if __has_include(<new>)
#else
void *operator new(size_t size) { return malloc(size); }

void *operator new[](size_t size) { return malloc(size); }

void operator delete(void *val) noexcept { free(val); }

void operator delete[](void *val) noexcept { free(val); }
#endif
namespace emlite {
Val::Val() : v_(0) {}

Val Val::from_handle(Handle h) {
    Val val;
    val.v_ = h;
    return val;
}

Val Val::global(const char *v) {
    return Val::from_handle(emlite_val_global_this())
        .get(v);
}

Val Val::global() {
    return Val::from_handle(emlite_val_global_this());
}

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

Val::Val(const char *v)
    : v_(emlite_val_make_str(v, strlen(v))) {}

Val::Val(Callback f)
    : v_(Val::make_js_function(f).as_handle()) {}

Handle Val::as_handle() const { return v_; }

Uniq<char[]> Val::type_of() const {
    return Uniq<char[]>(emlite_val_typeof(v_));
}

Val Val::get(const char *prop) const {
    return Val::from_handle(
        emlite_val_obj_prop(v_, prop, strlen(prop))
    );
}

void Val::set(const char *prop, const Val &val) const {
    emlite_val_obj_set_prop(
        v_, prop, strlen(prop), val.as_handle()
    );
}

bool Val::has(const char *prop) const {
    return emlite_val_obj_has_prop(v_, prop, strlen(prop));
}

bool Val::has_own_property(const char *prop) const {
    return emlite_val_obj_has_own_prop(
        v_, prop, strlen(prop)
    );
}

Val Val::operator[](size_t idx) const {
    return Val::from_handle(emlite_val_get_elem(v_, idx));
}

Val Val::make_js_function(Callback f) {
    uint32_t fidx =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f)
        );
    return Val::from_handle(emlite_val_make_callback(fidx));
}

// clang-format off
Val Val::await() const {
    return Val::from_handle(em_Val_as_handle(emlite_eval_v(
        "(async() => { let obj = ValMap.toValue(%d); let ret = await obj; "
        "return ValMap.toHandle(ret); })()",
        v_
    )));
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
