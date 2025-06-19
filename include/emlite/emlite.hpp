#pragma once

#include "emlite.h"
#undef EMLITE_EVAL

#if __has_include(<new>)
#include <new>
#else
void *operator new(size_t, void *place) noexcept;
#endif

namespace emlite {

namespace detail {
#include "tiny_traits.hpp"

#ifdef EMLITE_USE_THREADS
using refcount_t = _Atomic size_t;
inline void ref_inc(refcount_t &r) noexcept {
    atomic_fetch_add_explicit(&r, 1u, memory_order_acq_rel);
}
inline bool ref_dec(refcount_t &r) noexcept {
    return atomic_fetch_sub_explicit(
               &r, 1u, memory_order_acq_rel
           ) == 1u;
}
#else
using refcount_t = size_t;
inline void ref_inc(refcount_t &r) noexcept { ++r; }
inline bool ref_dec(refcount_t &r) noexcept {
    return --r == 0u;
}
#endif
} // namespace detail

template<class T>
class Uniq {
    T* ptr_ = nullptr;

public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(detail::nullptr_t) noexcept : ptr_(nullptr) {}
    explicit Uniq(T* p) noexcept : ptr_(p) {}

    Uniq(const Uniq&)            = delete;
    Uniq& operator=(const Uniq&) = delete;

    Uniq(Uniq&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    Uniq& operator=(Uniq&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_        = other.ptr_;
            other.ptr_  = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq& other) noexcept { auto tmp = ptr_; ptr_ = other.ptr_; other.ptr_ = tmp; }

    void reset(T* p = nullptr) noexcept {
        if (ptr_) delete ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T* release() noexcept {
        T* tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T& operator*()  const noexcept { return *ptr_; }
    T* operator->() const noexcept { return  ptr_; }
};

template<class T>
class Uniq<T[]> {
    T* ptr_ = nullptr;

public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(detail::nullptr_t) noexcept : ptr_(nullptr) {}
    explicit Uniq(T* p) noexcept : ptr_(p) {}

    Uniq(const Uniq&)            = delete;
    Uniq& operator=(const Uniq&) = delete;

    Uniq(Uniq&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    Uniq& operator=(Uniq&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq& other) noexcept { auto tmp = ptr_; ptr_ = other.ptr_; other.ptr_ = tmp; }

    void reset(T* p = nullptr) noexcept {
        if (ptr_) delete[] ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T* release() noexcept {
        T* tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T& operator[](size_t i) const noexcept { return ptr_[i]; }
};

template<class T>
inline void swap(Uniq<T>& a, Uniq<T>& b) noexcept { a.swap(b); }

template<class T>
inline void swap(Uniq<T[]>& a, Uniq<T[]>& b) noexcept { a.swap(b); }

template <class T>
class Shared {
    struct Control {
        T value;
        void (*deleter)(T *) noexcept;
        detail::refcount_t refs;

        template <class... Args>
        Control(void (*d)(T *) noexcept, Args &&...args)
            : value(detail::forward<Args>(args)...),
              deleter(d), refs(1u) {}
    };

    Control *ctrl_ = nullptr;

    void incref() const noexcept {
        if (ctrl_)
            detail::ref_inc(ctrl_->refs);
    }
    void decref() noexcept {
        if (!ctrl_)
            return;
        if (detail::ref_dec(ctrl_->refs)) {
            if (ctrl_->deleter)
                ctrl_->deleter(&ctrl_->value);
            ::operator delete(ctrl_);
        }
        ctrl_ = nullptr;
    }

  public:
    using element_type = T;

    constexpr Shared() noexcept = default;

    explicit Shared(
        T value, void (*d)(T *) noexcept = nullptr
    )
        : ctrl_(static_cast<Control *>(
              ::operator new(sizeof(Control))
          )) {
        new (&ctrl_->value) T((T &&)(value));
        ctrl_->deleter = d;
        ctrl_->refs    = 1u;
    }

    template <class... Args>
    explicit Shared(detail::in_place_t, Args &&...args)
        : ctrl_(static_cast<Control *>(
              ::operator new(sizeof(Control))
          )) {
        new (&ctrl_->value)
            T(detail::forward<Args>(args)...);
        ctrl_->deleter = nullptr;
        ctrl_->refs    = 1u;
    }

    Shared(const Shared &other) noexcept
        : ctrl_(other.ctrl_) {
        incref();
    }
    Shared(Shared &&other) noexcept : ctrl_(other.ctrl_) {
        other.ctrl_ = nullptr;
    }

    Shared &operator=(const Shared &other) noexcept {
        if (this != &other) {
            decref();
            ctrl_ = other.ctrl_;
            incref();
        }
        return *this;
    }

    Shared &operator=(Shared &&other) noexcept {
        if (this != &other) {
            decref();
            ctrl_       = other.ctrl_;
            other.ctrl_ = nullptr;
        }
        return *this;
    }

    ~Shared() { decref(); }

    void reset() noexcept { decref(); }

    template <class... Args>
    void reset_in_place(Args &&...args) {
        decref();
        ctrl_ = static_cast<Control *>(
            ::operator new(sizeof(Control))
        );
        new (&ctrl_->value)
            T(detail::forward<Args>(args)...);
        ctrl_->deleter = nullptr;
        ctrl_->refs    = 1u;
    }

    void swap(Shared &other) noexcept {
        auto tmp    = ctrl_;
        ctrl_       = other.ctrl_;
        other.ctrl_ = tmp;
    }

    [[nodiscard]] element_type *get() const noexcept {
        return ctrl_ ? const_cast<element_type *>(
                           &ctrl_->value
                       )
                     : nullptr;
    }

    [[nodiscard]] size_t use_count() const noexcept {
        return ctrl_ ? static_cast<size_t>(ctrl_->refs)
                     : 0u;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return ctrl_ != nullptr;
    }

    [[nodiscard]] element_type *forget() noexcept {
        element_type *p = get();
        ctrl_           = nullptr;
        return p;
    }

    element_type &operator*() const noexcept {
        return *get();
    }
    element_type *operator->() const noexcept {
        return get();
    }
};

template <class T>
class Shared<T[]> {
    struct Control {
        T *ptr;
        void (*deleter)(T *) noexcept;
        detail::refcount_t refs;
        Control(T *p, void (*d)(T *) noexcept)
            : ptr(p), deleter(d), refs(1u) {}
    };

    Control *ctrl_ = nullptr;

    void incref() const noexcept {
        if (ctrl_)
            detail::ref_inc(ctrl_->refs);
    }
    void decref() noexcept {
        if (!ctrl_)
            return;
        if (detail::ref_dec(ctrl_->refs)) {
            if (ctrl_->deleter)
                ctrl_->deleter(ctrl_->ptr);
            ::operator delete(ctrl_);
        }
        ctrl_ = nullptr;
    }

  public:
    using element_type = T;

    constexpr Shared() noexcept = default;

    explicit Shared(
        T *p,
        void (*d)(T *) noexcept = [](T *q) { delete[] q; }
    ) noexcept
        : ctrl_(p ? new Control(p, d) : nullptr) {}

    Shared(const Shared &other) noexcept
        : ctrl_(other.ctrl_) {
        incref();
    }
    Shared(Shared &&other) noexcept : ctrl_(other.ctrl_) {
        other.ctrl_ = nullptr;
    }

    Shared &operator=(const Shared &other) noexcept {
        if (this != &other) {
            decref();
            ctrl_ = other.ctrl_;
            incref();
        }
        return *this;
    }

    Shared &operator=(Shared &&other) noexcept {
        if (this != &other) {
            decref();
            ctrl_       = other.ctrl_;
            other.ctrl_ = nullptr;
        }
        return *this;
    }

    ~Shared() { decref(); }

    void reset() noexcept { decref(); }

    void reset(
        T *p,
        void (*d)(T *) noexcept = [](T *q) { delete[] q; }
    ) {
        decref();
        ctrl_ = p ? new Control(p, d) : nullptr;
    }

    void swap(Shared &other) noexcept {
        auto tmp    = ctrl_;
        ctrl_       = other.ctrl_;
        other.ctrl_ = tmp;
    }

    [[nodiscard]] element_type *get() const noexcept {
        return ctrl_ ? ctrl_->ptr : nullptr;
    }

    [[nodiscard]] size_t use_count() const noexcept {
        return ctrl_ ? static_cast<size_t>(ctrl_->refs)
                     : 0u;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return ctrl_ != nullptr;
    }

    [[nodiscard]] element_type *forget() noexcept {
        element_type *p = get();
        ctrl_           = nullptr;
        return p;
    }

    element_type &operator[](size_t i) const noexcept {
        return get()[i];
    }
};

template <class T>
inline void swap(Shared<T> &a, Shared<T> &b) noexcept {
    a.swap(b);
}
template <class T>
inline void swap(Shared<T[]> &a, Shared<T[]> &b) noexcept {
    a.swap(b);
}

void emlite_val_deleter(Handle *h) noexcept { emlite_val_delete(*h); }

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
        Shared<Handle> temp;
        if constexpr (detail::is_integral_v<T>) {
            temp = Shared<Handle>(
                emlite_val_make_int(v), emlite_val_deleter
            );
        } else {
            temp = Shared<Handle>(
                emlite_val_make_double(v), emlite_val_deleter
            );
        }
        v_.swap(temp);
    }
    explicit Val(const char *v);

    [[nodiscard]] Handle as_handle() const __attribute__((always_inline));
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
#if __has_include(<new>)
#else
void *operator new(size_t size) { return malloc(size); }

void *operator new[](size_t size) { return malloc(size); }

void operator delete(void *val) noexcept { free(val); }

void operator delete[](void *val) noexcept { free(val); }

void *operator new(size_t, void *place) noexcept {
    return place;
}
#endif
namespace emlite {
Val::Val() : v_(Shared<Handle>(0)) {}

Val Val::take_ownership(Handle h) {
    auto temp = Shared<Handle>(h, emlite_val_deleter);
    Val val;
    val.v_.swap(temp);
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
          emlite_val_make_str(v, strlen(v)), emlite_val_deleter
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
// clang-format on

bool Val::is_number() const {
    return emlite_val_is_number(*v_);
}

bool Val::is_string() const {
    return emlite_val_is_string(*v_);
}

bool Val:: instanceof (const Val &v) const {
    return emlite_val_instanceof(*v_, *v.v_);
}

bool Val::operator!() const { return emlite_val_not(*v_); }

bool Val::operator==(const Val &other) const {
    return emlite_val_strictly_equals(*v_, *other.v_);
}

bool Val::operator!=(const Val &other) const {
    return !emlite_val_strictly_equals(*v_, *other.v_);
}

bool Val::operator>(const Val &other) const {
    return emlite_val_gt(*v_, *other.v_);
}

bool Val::operator>=(const Val &other) const {
    return emlite_val_gte(*v_, *other.v_);
}

bool Val::operator<(const Val &other) const {
    return emlite_val_lt(*v_, *other.v_);
}

bool Val::operator<=(const Val &other) const {
    return emlite_val_lte(*v_, *other.v_);
}

Console::Console() : Val(Val::global("console")) {}
} // namespace emlite
#endif
