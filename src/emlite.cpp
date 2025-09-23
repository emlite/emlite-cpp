#include <emlite/emlite.hpp>

// Unified JS-side callback handling; no registry needed across targets

#if __has_include(<new>)
#else
void *operator new(size_t size) { return emlite_malloc(size); }

void *operator new[](size_t size) { return emlite_malloc(size); }

void operator delete(void *val) noexcept { emlite_free(val); }

void operator delete(void *ptr, size_t) noexcept {
  emlite_free(ptr);
}

void operator delete[](void *ptr, size_t) noexcept {
  emlite_free(ptr);
}

void operator delete[](void *val) noexcept { emlite_free(val); }

void *operator new(size_t, void *place) noexcept { return place; }
#endif
namespace emlite {
void init() {
    #ifndef EMSCRIPTEN
    // assert(emlite_target() == EMLITE_TARGET);
    #endif
    emlite_init_handle_table();
}

Val::Val() noexcept : v_(0) {}

Val::Val(const Val &other) noexcept : v_(other.v_) {
    if (v_)
        emlite_val_inc_ref(v_);
}
Val &Val::operator=(const Val &other) noexcept {
    if (this == &other)
        return *this;
    if (v_)
        emlite_val_dec_ref(v_);
    v_ = other.v_;
    if (v_)
        emlite_val_inc_ref(v_);
    return *this;
}

Val &Val::operator=(Val &&other) noexcept {
    if (this != &other) {
        if (v_)
            emlite_val_dec_ref(v_);
        v_       = other.v_;
        other.v_ = 0;
    }
    return *this;
}

Val::Val(Val &&other) noexcept : v_(other.v_) { other.v_ = 0; }

Val::~Val() {
    if (v_)
        emlite_val_dec_ref(v_);
}

Val Val::clone() const noexcept { return *this; }

Val Val::take_ownership(Handle h) noexcept {
    Val v;
    v.v_ = h;
    return v;
}

Val Val::global(const char *v) noexcept { return Val::take_ownership(EMLITE_GLOBALTHIS).get(v); }

Val Val::global() noexcept { return Val::take_ownership(EMLITE_GLOBALTHIS); }

Val Val::null() noexcept { return Val::take_ownership(EMLITE_NULL); }

Val Val::undefined() noexcept { return Val::take_ownership(EMLITE_UNDEFINED); }

Val Val::object() noexcept { return Val::take_ownership(emlite_val_new_object()); }

Val Val::array() noexcept { return Val::take_ownership(emlite_val_new_array()); }

Val Val::dup(Handle h) noexcept {
    emlite_val_inc_ref(h);
    return Val::take_ownership(h);
}

Handle Val::release(Val &&v) noexcept {
    auto v_   = detail::move(v);
    auto temp = v_.v_;
    v_.v_     = 0;
    return temp;
}

void Val::delete_(Val &&v) noexcept { emlite_val_dec_ref(detail::move(v).v_); }

void Val::throw_(const Val &v) { return emlite_val_throw(v.v_); }

Handle Val::as_handle() const noexcept { return v_; }

Uniq<char[]> Val::type_of() const noexcept { return Uniq<char[]>(emlite_val_typeof(v_)); }

bool Val::has_own_property(const char *prop) const noexcept {
    return emlite_val_obj_has_own_prop(v_, prop, strlen(prop));
}

Val Val::make_fn(Callback f, const Val &data) noexcept {
#ifdef EMLITE_WASIP2
    // JS-side callback storage for all targets: pack function pointer + user data
    if (data.v_) emlite_val_inc_ref(data.v_);
    auto pack = (EmliteCbPack *)emlite_malloc(sizeof(EmliteCbPack));
    if (!pack) return Val::undefined();
    pack->fn = f;
    pack->user_data = data.v_;
    Handle packed = emlite_val_make_biguint((uint64_t)(uintptr_t)pack);
    return Val::take_ownership(emlite_val_make_callback(0, packed));
#else
    Handle fidx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f));
    return Val::take_ownership(emlite_val_make_callback(fidx, Val::release((Val &&)data)));
#endif
}

Val Val::make_fn(Closure<Val(Params)> &&f) noexcept {
    return Val::make_fn(
        [](auto h, auto data) -> Handle {
            Val func0  = Val::take_ownership(data);
            auto func  = (Closure<Val(Params)> *)func0.template as<uintptr_t>();
            size_t len = 0;
            auto v     = Val::vec_from_js_array<Val>(Val::take_ownership(h), len);
            Params params{v.get(), len};
            auto ret = (*func)(params);
            Val::release((Val &&)func0);
            return ret.as_handle();
        },
        Val((uintptr_t) new Closure<Val(Params)>(detail::move(f)))
    );
}

// clang-format off
Val Val::await() const {
    return emlite_eval_cpp(
        "(async() => { let obj = EMLITE_VALMAP.toValue(%d); let ret = await obj; "
        "return EMLITE_VALMAP.toHandle(ret); })()",
        v_
    );
}
// clang-format on

bool Val::is_bool() const noexcept { return emlite_val_is_bool(v_); }

bool Val::is_number() const noexcept { return emlite_val_is_number(v_); }

bool Val::is_string() const noexcept { return emlite_val_is_string(v_); }

bool Val:: instanceof (const Val &v) const noexcept { return emlite_val_instanceof(v_, v.v_); }

bool Val::is_function() const noexcept { return instanceof (Val::global("Function")); }

bool Val::is_error() const noexcept { return instanceof (Val::global("Error")); }

bool Val::is_undefined() const noexcept { return v_ == EMLITE_UNDEFINED; }

bool Val::is_null() const noexcept { return v_ == EMLITE_NULL; }

bool Val::operator!() const { return emlite_val_not(v_); }

bool Val::operator==(const Val &other) const { return emlite_val_strictly_equals(v_, other.v_); }

bool Val::operator!=(const Val &other) const { return !emlite_val_strictly_equals(v_, other.v_); }

bool Val::operator>(const Val &other) const { return emlite_val_gt(v_, other.v_); }

bool Val::operator>=(const Val &other) const { return emlite_val_gte(v_, other.v_); }

bool Val::operator<(const Val &other) const { return emlite_val_lt(v_, other.v_); }

bool Val::operator<=(const Val &other) const { return emlite_val_lte(v_, other.v_); }

Console::Console() : Val(Val::take_ownership(EMLITE_CONSOLE)) {}

void Console::clear() const { call("clear"); }
} // namespace emlite
