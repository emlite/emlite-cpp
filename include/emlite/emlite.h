#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef EMLITE_USED
#define EMLITE_USED __attribute__((used))
#endif

typedef uint32_t handle;

typedef void (*Callback)(handle);

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct CallbackList {
    Callback *fns;
    size_t len;
    size_t cap;
} CallbackList;

int CallbackList_reserve(CallbackList *list, size_t new_cap);
int CallbackList_push_back(CallbackList *list, Callback cb);
int CallbackList_remove(CallbackList *list, size_t index);
Callback CallbackList_at(const CallbackList *list, size_t index);
void CallbackList_free(CallbackList *list);

static CallbackList cb_table = {0, 0, 0};

handle emlite_val_null(void);
handle emlite_val_undefined(void);
handle emlite_val_false(void);
handle emlite_val_true(void);
handle emlite_val_global_this();
handle emlite_val_new_array(void);
handle emlite_val_new_object(void);
const char *emlite_val_typeof(handle);
handle emlite_val_construct_new(handle, handle argv);
handle emlite_val_func_call(handle func, handle argv);
void emlite_val_push(handle arr, handle v);
handle emlite_val_make_int(int t);
handle emlite_val_make_double(double t);
handle emlite_val_make_str(const char *, size_t);
int emlite_val_get_value_int(handle);
double emlite_val_get_value_double(handle);
const char *emlite_val_get_value_string(handle);
handle emlite_val_get_elem(handle, size_t);
handle emlite_val_obj_call(
    handle obj, const char *name, size_t len, handle argv
);
handle emlite_val_obj_prop(handle obj, const char *prop, size_t len);
void emlite_val_obj_set_prop(
    handle obj, const char *prop, size_t len, handle val
);
bool emlite_val_obj_has_prop(handle, const char *prop, size_t len);
bool emlite_val_obj_has_own_prop(handle, const char *prop, size_t len);
handle emlite_val_make_callback(handle id);

handle emlite_val_global(const char *name);
handle emlite_val_construct_new_v(handle, int n, ...);
handle emlite_val_func_call_v(handle func, int n, ...);
handle emlite_val_obj_call_v(handle obj, const char *name, int n, ...);

#define VAL_OBJ_CALL(obj, name, ...)                                           \
    emlite_val_obj_call_v(                                                      \
        (obj),                                                                 \
        (name),                                                                \
        (int)(sizeof((handle[]){__VA_ARGS__}) / sizeof(handle)),               \
        __VA_ARGS__                                                            \
    )

#define VAL_OBJ_NEW(obj, ...)                                                  \
    emlite_val_construct_new_v(                                                 \
        obj,                                                                   \
        (int)(sizeof((handle[]){__VA_ARGS__}) / sizeof(handle)),               \
        __VA_ARGS__                                                            \
    )

#define VAL_FUNC_CALL(obj, ...)                                                \
    emlite_val_func_call_v(                                                     \
        obj,                                                                   \
        (int)(sizeof((handle[]){__VA_ARGS__}) / sizeof(handle)),               \
        __VA_ARGS__                                                            \
    )

#ifdef EMLITE_IMPL
#include <stdarg.h>

int CallbackList_reserve(CallbackList *list, size_t new_cap) {
    if (new_cap <= list->cap)
        return 0;
    Callback *tmp = (Callback *)realloc(list->fns, new_cap * sizeof(Callback));
    if (!tmp)
        return -1;
    list->fns = tmp;
    list->cap = new_cap;
    return 0;
}

int CallbackList_push_back(CallbackList *list, Callback cb) {
    if (list->len == list->cap) {
        size_t new_cap = list->cap ? list->cap * 2 : 4;
        if (CallbackList_reserve(list, new_cap) != 0)
            return -1;
    }
    list->fns[list->len++] = cb;
    return 0;
}

int CallbackList_remove(CallbackList *list, size_t index) {
    if (index >= list->len)
        return -1;
    memmove(
        &list->fns[index],
        &list->fns[index + 1],
        (list->len - index - 1) * sizeof(Callback)
    );
    list->len--;
    return 0;
}

Callback CallbackList_at(const CallbackList *list, size_t index) {
    if (index >= list->len)
        return (Callback)0;
    return list->fns[index];
}

void CallbackList_free(CallbackList *list) {
    free(list->fns);
    list->fns = NULL;
    list->len = list->cap = 0;
}

handle emlite_val_global(const char *name) {
    handle w = emlite_val_global_this();
    return emlite_val_obj_prop(w, name, strlen(name));
}

handle emlite_val_obj_call_v(handle obj, const char *name, int n, ...) {
    handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, handle));
    va_end(args);
    return emlite_val_obj_call(obj, name, strlen(name), arr);
}

handle emlite_val_construct_new_v(handle obj, int n, ...) {
    handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, handle));
    va_end(args);
    return emlite_val_construct_new(obj, arr);
}

handle emlite_val_func_call_v(handle obj, int n, ...) {
    handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++)
        emlite_val_push(arr, va_arg(args, handle));
    va_end(args);
    return emlite_val_func_call(obj, arr);
}

__attribute__((used)) void *memalloc(size_t s) { return malloc(s); }

__attribute__((used)) void wasm_invoke_cb(uint32_t id, handle evt) {
    if (id < cb_table.len && cb_table.fns[id])
        cb_table.fns[id](evt);
}
#endif

#ifdef __cplusplus
}
#include <concepts>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace emlite {
class Val {
    handle v_;
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

    template <typename T>
        requires(std::integral<T> || std::floating_point<T>)
    explicit Val(T v) : v_(0) {
        if constexpr (std::is_integral_v<T>) {
            v_ = emlite_val_make_int(v);
        } else {
            v_ = emlite_val_make_double(v);
        }
    }
    explicit Val(const char *v);
    explicit Val(std::string_view v);
    explicit Val(Callback f);

    handle as_handle() const;
    Val get(std::string_view prop) const;
    void set(std::string_view prop, const Val &val) const;
    bool has(std::string_view prop) const;
    bool has_own_property(std::string_view prop) const;
    std::string type_of() const;
    Val operator[](size_t idx) const;
    Val await() const;

    template <class... Args>
    Val call(const char *method, Args &&...vals) const;

    template <class... Args>
    Val new_(Args &&...vals) const;

    template <class... Args>
    Val operator()(Args &&...vals) const;

    template <typename T>
    T as() const;

    template <typename T> requires(std::integral<T> || std::floating_point<T>)
    static std::vector<T> vec_from_js_array(const Val &v) {
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
    handle arr = emlite_val_new_array();
    (emlite_val_push(arr, std::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_obj_call(v_, method, strlen(method), arr)
    );
}

template <class... Args>
Val Val::new_(Args &&...vals) const {
    handle arr = emlite_val_new_array();
    (emlite_val_push(arr, std::forward<Args>(vals).as_handle()), ...);
    return Val::from_handle(emlite_val_construct_new(v_, arr));
}

template <class... Args>
Val Val::operator()(Args &&...vals) const {
    handle arr = emlite_val_new_array();
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

#define EMLITE_EVAL(x, ...) Val::global("eval")(Val(std::format(#x, __VA_ARGS__)))

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

Val::Val(const char *v) : v_(emlite_val_make_str(v, std::strlen(v))) {}

Val::Val(std::string_view v) : v_(emlite_val_make_str(v.data(), v.size())) {}

Val::Val(Callback f) : v_(Val::make_js_function(f).as_handle()) {}

handle Val::as_handle() const { return v_; }

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
    uint32_t id = cb_table.len;
    CallbackList_push_back(&cb_table, f);
    return Val::from_handle(emlite_val_make_callback(id));
}

// clang-format off
Val Val::await() const {
    return EMLITE_EVAL({{
       (async () => {{
        let obj = ValMap.toValue({});
        let ret = await obj;
        return ValMap.toHandle(ret);
       }})()
    }}, v_);
}
// clang-format on

Console::Console() : Val(Val::global("console")) {}
} // namespace emlite
#endif
#endif