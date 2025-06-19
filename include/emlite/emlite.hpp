#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

namespace emlite {

namespace detail {
#include "tiny_traits.hpp"

#ifdef EMLITE_USE_THREADS
using refcount_t = _Atomic size_t;
#else
using refcount_t = size_t;
#endif
} // namespace detail

template <typename T>
class Uniq {
  private:
    T *ptr;

  public:
    Uniq() : ptr(nullptr) {}
    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;
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
class Uniq<T[]> {
  private:
    T *ptr;

  public:
    Uniq() : ptr(nullptr) {}
    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;
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
    T *forget() noexcept {
        T *ptr = ctrl_ ? &ctrl_->value : nullptr;
        ctrl_  = nullptr; // destructor will now do nothing
        return ptr;
    }

    /* optional: a version that forgets but does *not* hand
     * out the ptr */
    void forget_no_ptr() noexcept { ctrl_ = nullptr; }
};

template <class T>
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
    T *forget() noexcept {
        T *ptr = ctrl_ ? &ctrl_->ptr : nullptr;
        ctrl_  = nullptr; // destructor will now do nothing
        return ptr;
    }

    /* optional: a version that forgets but does *not* hand
     * out the ptr */
    void forget_no_ptr() noexcept { ctrl_ = nullptr; }
};

class Val {
    Shared<Handle> v_;
    Val();

  public:
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

    template <
        typename T,
        typename = typename detail::enable_if_t<
            detail::is_integral_v<T> ||
            detail::is_floating_point_v<T>>>
    explicit Val(T v) : v_(0) {
        if constexpr (detail::is_integral_v<T>) {
            v_ = Shared<Handle>(
                emlite_val_make_int(v), emlite_val_delete
            );
        } else {
            v_ = Shared<Handle>(
                emlite_val_make_double(v), emlite_val_delete
            );
        }
    }
    explicit Val(const char *v);

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
    Val forget() {
        v_.forget_no_ptr();
        return *this;
    }

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
    T as() const;

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
        *v_, method, strlen(method), arr.as_handle()
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
        emlite_val_construct_new(*v_, arr.as_handle())
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
        emlite_val_func_call(*v_, arr.as_handle())
    );
}

template <typename T>
T Val::as() const {
    if constexpr (detail::is_integral_v<T>) {
        if constexpr (detail::is_same_v<T, bool>) {
            if (*v_ > 3)
                return true;
            else
                return false;
        } else {
            return emlite_val_get_value_int(*v_);
        }
    } else if constexpr (detail::is_floating_point_v<T>)
        return emlite_val_get_value_int(*v_);
    else if constexpr (detail::is_same_v<T, Uniq<char[]>>)
        return Uniq<char[]>(emlite_val_get_value_string(*v_)
        );
    else
        return T::take_ownership(*v_);
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
#if __has_include(<new>)
#else
void *operator new(size_t size) { return malloc(size); }

void *operator new[](size_t size) { return malloc(size); }

void operator delete(void *val) noexcept { free(val); }

void operator delete[](void *val) noexcept { free(val); }
#endif
namespace emlite {
Val::Val() : v_(0) {}

Val Val::take_ownership(Handle h) {
    Val val;
    val.v_ = Shared<Handle>(h, emlite_val_delete);
    return val;
}

Val Val::global(const char *v) {
    return Val::take_ownership(emlite_val_global_this())
        .get(v);
}

Val Val::global() {
    return Val::take_ownership(emlite_val_global_this());
}

Val Val::null() { return Val::take_ownership(0); }

Val Val::undefined() { return Val::take_ownership(1); }

Val Val::object() {
    return Val::take_ownership(emlite_val_new_object());
}

Val Val::array() {
    return Val::take_ownership(emlite_val_new_array());
}

void Val::delete_(Val v) { emlite_val_delete(*v.v_); }

void Val::throw_(Val v) { return emlite_val_throw(*v.v_); }

Val::Val(const char *v)
    : v_(Shared<Handle>(
          emlite_val_make_str(v, strlen(v)),
          emlite_val_delete
      )) {}

Handle Val::as_handle() const { return *v_; }

Uniq<char[]> Val::type_of() const {
    return Uniq<char[]>(emlite_val_typeof(*v_));
}

Val Val::get(const char *prop) const {
    return Val::take_ownership(
        emlite_val_obj_prop(*v_, prop, strlen(prop))
    );
}

void Val::set(const char *prop, const Val &val) const {
    emlite_val_obj_set_prop(
        *v_, prop, strlen(prop), val.as_handle()
    );
}

bool Val::has(const char *prop) const {
    return emlite_val_obj_has_prop(*v_, prop, strlen(prop));
}

bool Val::has_own_property(const char *prop) const {
    return emlite_val_obj_has_own_prop(
        *v_, prop, strlen(prop)
    );
}

Val Val::operator[](size_t idx) const {
    return Val::take_ownership(emlite_val_get_elem(*v_, idx)
    );
}

Val Val::make_fn(Callback f) {
    uint32_t fidx =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f)
        );
    return Val::take_ownership(emlite_val_make_callback(fidx
    ));
}

// clang-format off
Val Val::await() const {
    return emlite_eval_cpp(
        "(async() => { let obj = ValMap.toValue(%d); let ret = await obj; "
        "return ValMap.toHandle(ret); })()",
        *v_
    );
}

bool Val::is_number() const {
    return emlite_val_is_number(*v_);
}

bool Val::is_string() const {
    return emlite_val_is_string(*v_);
}

bool Val::instanceof(const Val &v) const {
    return emlite_val_instanceof(*v_, *v.v_);
}

bool Val::operator!() const {
    return emlite_val_not(*v_);
}

bool Val::operator==(const Val& other) const {
    return emlite_val_strictly_equals(*v_, *other.v_);
}

bool Val::operator!=(const Val& other) const {
    return !emlite_val_strictly_equals(*v_, *other.v_);
}

bool Val::operator>(const Val& other) const {
    return emlite_val_gt(*v_, *other.v_);
}

bool Val::operator>=(const Val& other) const {
    return emlite_val_gte(*v_, *other.v_);
}

bool Val::operator<(const Val& other) const {
    return emlite_val_lt(*v_, *other.v_);
}

bool Val::operator<=(const Val& other) const {
    return emlite_val_lte(*v_, *other.v_);
}
// clang-format on

Console::Console() : Val(Val::global("console")) {}
} // namespace emlite
#endif
