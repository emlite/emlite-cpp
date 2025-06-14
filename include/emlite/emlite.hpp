#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef EMLITE_USED
#define EMLITE_USED __attribute__((used))
#endif

using Handle = uint32_t;

using Callback = Handle (*)(Handle);

// externs
extern "C" {
Handle emlite_val_null(void);
Handle emlite_val_undefined(void);
Handle emlite_val_false(void);
Handle emlite_val_true(void);
Handle emlite_val_global_this();
Handle emlite_val_new_array(void);
Handle emlite_val_new_object(void);
char *emlite_val_typeof(Handle);
Handle emlite_val_construct_new(Handle, Handle argv);
Handle emlite_val_func_call(Handle func, Handle argv);
void emlite_val_push(Handle arr, Handle v);
Handle emlite_val_make_int(int t);
Handle emlite_val_make_double(double t);
Handle emlite_val_make_str(const char *, size_t);
int emlite_val_get_value_int(Handle);
double emlite_val_get_value_double(Handle);
char *emlite_val_get_value_string(Handle);
Handle emlite_val_get_elem(Handle, size_t);
bool emlite_val_is_string(Handle);
bool emlite_val_is_number(Handle);
bool emlite_val_not(Handle);
bool emlite_val_gt(Handle, Handle);
bool emlite_val_gte(Handle, Handle);
bool emlite_val_lt(Handle, Handle);
bool emlite_val_lte(Handle, Handle);
bool emlite_val_equals(Handle, Handle);
bool emlite_val_strictly_equals(Handle, Handle);
bool emlite_val_instanceof(Handle, Handle);
void emlite_val_delete(Handle);
void emlite_val_throw(Handle);

Handle emlite_val_obj_call(
    Handle obj, const char *name, size_t len, Handle argv
);
Handle emlite_val_obj_prop(Handle obj, const char *prop, size_t len);
void emlite_val_obj_set_prop(
    Handle obj, const char *prop, size_t len, Handle val
);
bool emlite_val_obj_has_prop(Handle, const char *prop, size_t len);
bool emlite_val_obj_has_own_prop(Handle, const char *prop, size_t len);
Handle emlite_val_make_callback(Handle id);

void *emlite_malloc(size_t);
void *emlite_realloc(void *, size_t);
void emlite_free(void *);
}

namespace emlite {
class Val {
    Handle v_;
    Val();

  public:
    static Val from_handle(uint32_t v);
    static Val global(std::string_view name);
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
        requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
        : v_(0) {
        if constexpr (std::is_integral_v<T>) {
            v_ = emlite_val_make_int(v);
        } else {
            v_ = emlite_val_make_double(v);
        }
    }
    explicit Val(const char *v);
    explicit Val(std::string_view v);
    explicit Val(Callback f);

    Handle as_handle() const;
    Val get(std::string_view prop) const;
    void set(std::string_view prop, const Val &val) const;
    bool has(std::string_view prop) const;
    bool has_own_property(std::string_view prop) const;
    std::string type_of() const;
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
    static std::vector<T> vec_from_js_array(const Val &v)
        requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
    {
        auto sz = v.get("length").as<int>();
        std::vector<T> ret;
        ret.reserve(sz);
        for (int i = 0; i < sz; i++) {
            ret.push_back(v[i].as<T>());
        }
        return ret;
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
    (emlite_val_push(arr, std::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_obj_call(v_, method, strlen(method), arr)
    );
}

template <class... Args>
Val Val::new_(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(arr, std::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_construct_new(v_, arr));
}

template <class... Args>
Val Val::operator()(Args &&...vals) const {
    Handle arr = emlite_val_new_array();
    (emlite_val_push(arr, std::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_func_call(v_, arr));
}

template <typename T>
T Val::as() const {
    if constexpr (std::is_integral_v<T>) {
        if constexpr (std::is_same_v<T, bool>) {
            if (v_ > 3)
                return true;
            else
                return false;
        } else {
            return emlite_val_get_value_int(v_);
        }
    } else if constexpr (std::is_floating_point_v<T>)
        return emlite_val_get_value_int(v_);
    else if constexpr (std::is_same_v<T, std::string>)
        return std::string(emlite_val_get_value_string(v_));
    return T{};
}

template <class... Args>
void Console::log(Args &&...args) const {
    call("log", std::forward<Args>(args)...);
}

template <class... Args>
std::string emlite_eval_impl(const char *src, Args &&...args) {
    if constexpr (sizeof...(Args) == 0) {
        return std::string(src);
    } else {
        auto len = snprintf(nullptr, 0, src, std::forward<Args>(args)...);
        auto s   = std::string(len, '\0');
        (void
        )snprintf(s.data(), s.size() + 1, src, std::forward<Args>(args)...);
        return s;
    }
}

#define EMLITE_EVAL(x, ...)                                                    \
    Val::global("eval")(Val(emlite_eval_impl(#x __VA_OPT__(, __VA_ARGS__))))

} // namespace emlite

#ifdef EMLITE_IMPL
namespace emlite {
Val::Val() : v_(0) {}

Val Val::from_handle(uint32_t v) {
    Val val;
    val.v_ = v;
    return val;
}

Val Val::global(std::string_view v) {
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

Val::Val(const char *v) : v_(emlite_val_make_str(v, std::strlen(v))) {}

Val::Val(std::string_view v) : v_(emlite_val_make_str(v.data(), v.size())) {}

Val::Val(Callback f) : v_(Val::make_js_function(f).as_handle()) {}

Handle Val::as_handle() const { return v_; }

std::string Val::type_of() const { return std::string(emlite_val_typeof(v_)); }

Val Val::get(std::string_view prop) const {
    return Val::from_handle(emlite_val_obj_prop(v_, prop.data(), prop.size()));
}

void Val::set(std::string_view prop, const Val &val) const {
    emlite_val_obj_set_prop(v_, prop.data(), prop.size(), val.as_handle());
}

bool Val::has(std::string_view prop) const {
    return emlite_val_obj_has_prop(v_, prop.data(), prop.size());
}

bool Val::has_own_property(std::string_view prop) const {
    return emlite_val_obj_has_own_prop(v_, prop.data(), prop.size());
}

Val Val::operator[](size_t idx) const {
    return Val::from_handle(emlite_val_get_elem(v_, idx));
}

Val Val::make_js_function(Callback f) {
    uint32_t fidx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f)
    );
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
