#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EMLITE_USED __attribute__((used))

#define EMLITE_IMPORT(NAME)                                \
    __attribute__((                                        \
        import_module("env"), import_name(#NAME)           \
    ))

typedef uint32_t Handle;
typedef Handle (*Callback)(Handle);

// EM_JS and _EM_JS macros copied from
// https://github.com/emscripten-core/emscripten/blob/main/system/include/emscripten/em_js.h
// Copyright 2018 The Emscripten Authors.
// Licensed under MIT and the University of Illinois/NCSA
// Open Source License
#define _EM_JS(ret, c_name, js_name, params, code)         \
    ret c_name params EMLITE_IMPORT(js_name);              \
    __attribute__((visibility("hidden"))                   \
    ) void *__em_js_ref_##c_name = (void *)&c_name;        \
    EMLITE_USED                                            \
    __attribute__((section("em_js"), aligned(1))           \
    ) char __em_js__##js_name[] = #params "<::>" code;

#define EM_JS(ret, name, params, ...)                      \
    _EM_JS(ret, name, name, params, #__VA_ARGS__)


// clang-format off
EM_JS(Handle, emlite_val_new_array_impl, (), {
    return EMLITE_VALMAP.add([]);
});

EM_JS(Handle, emlite_val_new_object_impl, (), {
    return EMLITE_VALMAP.add({});
});

EM_JS(char *, emlite_val_typeof_impl, (Handle n), {
    const str = (typeof EMLITE_VALMAP.get(n)) + "\0";
    const len = Module.lengthBytesUTF8(str);
    const buf = _malloc(len);
    stringToUTF8(str, buf, len);
    return buf;
});

EM_JS(
    Handle,
    emlite_val_construct_new_impl,
    (Handle objRef, Handle argv),
    {
        const target = EMLITE_VALMAP.get(objRef);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        return EMLITE_VALMAP.add(
            Reflect.construct(target, args)
        );
    }
);

EM_JS(
    Handle,
    emlite_val_func_call_impl,
    (Handle func, Handle argv),
    {
        const target = EMLITE_VALMAP.get(func);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        return EMLITE_VALMAP.add(
            Reflect.apply(target, undefined, args)
        );
    }
);

EM_JS(void, emlite_val_push_impl, (Handle arr, Handle v), {
    EMLITE_VALMAP.get(arr).push(v);
});

EM_JS(Handle, emlite_val_make_int_impl, (int t), {
    return EMLITE_VALMAP.add(t | 0);
});

EM_JS(Handle, emlite_val_make_double_impl, (double t), {
    return EMLITE_VALMAP.add(t);
});

EM_JS(
    Handle,
    emlite_val_make_str_impl,
    (const char *str, size_t len),
    { return EMLITE_VALMAP.add(UTF8ToString(str, len)); }
);

EM_JS(int, emlite_val_get_value_int_impl, (Handle n), {
    return EMLITE_VALMAP.get(n);
});

EM_JS(double, emlite_val_get_value_double_impl, (Handle n), {
    return EMLITE_VALMAP.get(n);
});

EM_JS(char *, emlite_val_get_value_string_impl, (Handle n), {
    const str = EMLITE_VALMAP.get(n) + "\0";
    const len = Module.lengthBytesUTF8(str);
    const buf = _malloc(len);
    stringToUTF8(str, buf, len);
    return buf;
});

EM_JS(Handle, emlite_val_get_impl, (Handle n, Handle idx), {
    return EMLITE_VALMAP.add(EMLITE_VALMAP.get(n)[EMLITE_VALMAP.get(idx)]);
});

EM_JS(void, emlite_val_set_impl, (Handle n, Handle idx, Handle val), {
    EMLITE_VALMAP.get(n)[EMLITE_VALMAP.get(idx)] = EMLITE_VALMAP.get(val);
});

EM_JS(bool, emlite_val_has_impl, (Handle n, Handle idx), {
    return Reflect.has(EMLITE_VALMAP.get(n), EMLITE_VALMAP.get(idx));
});

EM_JS(bool, emlite_val_is_string_impl, (Handle h), {
    const obj            = EMLITE_VALMAP.get(h);
    return typeof obj === "string" || obj instanceof
        String;
});

EM_JS(bool, emlite_val_is_number_impl, (Handle arg), {
    return typeof EMLITE_VALMAP.get(arg) === "number";
});

EM_JS(bool, emlite_val_not_impl, (Handle h), {
    return !EMLITE_VALMAP.get(h);
});

EM_JS(bool, emlite_val_gt_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) > EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_gte_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) >= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lt_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) < EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lte_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) <= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_equals_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) == EMLITE_VALMAP.get(b);
});

EM_JS(
    bool,
    emlite_val_strictly_equals_impl,
    (Handle a, Handle b),
    { return EMLITE_VALMAP.get(a) === EMLITE_VALMAP.get(b); }
);

EM_JS(bool, emlite_val_instanceof_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) instanceof EMLITE_VALMAP.get(b);
});

EM_JS(void, emlite_val_throw_impl, (Handle arg), { throw arg; });

EM_JS(
    Handle,
    emlite_val_obj_call_impl,
    (Handle obj, const char *name, size_t len, Handle argv),
    {
        const target = EMLITE_VALMAP.get(obj);
        const method = UTF8ToString(name, len);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        return EMLITE_VALMAP.add(
            Reflect.apply(target[method], target, args)
        );
    }
);

EM_JS(
    bool,
    emlite_val_obj_has_own_prop_impl,
    (Handle obj, const char *prop, size_t len),
    {
        const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        return Object.prototype.hasOwnProperty.call(
            target, p
        );
    }
);

EM_JS(Handle, emlite_val_make_callback_impl, (Handle fidx, Handle data), {
    const jsFn = (... args) => {
        const arrHandle =
            EMLITE_VALMAP.add(args.map(v => v));
        return Module.wasmTable.get(fidx)(arrHandle, data);
    };
    return EMLITE_VALMAP.add(jsFn);
});

EM_JS(void, emlite_print_object_map_impl, (), {
    console.log(EMLITE_VALMAP);
});

EM_JS(void, emlite_reset_object_map_impl, (), {
    for (const h of[... EMLITE_VALMAP._h2e.keys()]) {
        if (h > 4) {
            const value = EMLITE_VALMAP._h2e.get(h).value;

            EMLITE_VALMAP._h2e.delete(h);
            EMLITE_VALMAP._v2h.delete(value);
        }
    }
});

EM_JS(void, emlite_val_inc_ref_impl, (Handle h), {
    EMLITE_VALMAP.incRef(h);
});

EM_JS(void, emlite_val_dec_ref_impl, (Handle h), {
    if (h > 4) EMLITE_VALMAP.decRef(h);
});
// clang-format on
EMLITE_USED
Handle emlite_val_new_array() {
    return emlite_val_new_array_impl();
}

EMLITE_USED
Handle emlite_val_new_object() {
    return emlite_val_new_object_impl();
}

EMLITE_USED
char *emlite_val_typeof(Handle n) {
    return emlite_val_typeof_impl(n);
}

EMLITE_USED
Handle emlite_val_construct_new(
    Handle objRef, Handle argv
) {
    return emlite_val_construct_new_impl(objRef, argv);
}

EMLITE_USED
Handle emlite_val_func_call(Handle func, Handle argv) {
    return emlite_val_func_call_impl(func, argv);
}

EMLITE_USED
void emlite_val_push(Handle arr, Handle v) {
    emlite_val_push_impl(arr, v);
}

EMLITE_USED
Handle emlite_val_make_int(int t) {
    return emlite_val_make_int_impl(t);
}

EMLITE_USED
Handle emlite_val_make_double(double t) {
    return emlite_val_make_double_impl(t);
}

EMLITE_USED
Handle emlite_val_make_str(const char *str, size_t len) {
    return emlite_val_make_str_impl(str, len);
}

EMLITE_USED
int emlite_val_get_value_int (Handle n) {
    return emlite_val_get_value_int_impl(n);
}

EMLITE_USED
double emlite_val_get_value_double(Handle n) {
    return emlite_val_get_value_double_impl(n);
}

EMLITE_USED
char *emlite_val_get_value_string(Handle n) {
    return emlite_val_get_value_string_impl(n);
}

EMLITE_USED
Handle emlite_val_get(Handle n, Handle idx) {
    return emlite_val_get_impl(n, idx);
}

EMLITE_USED
void emlite_val_set(Handle n, Handle idx, Handle val) {
    return emlite_val_set_impl(n, idx, val);
}

EMLITE_USED
bool emlite_val_has(Handle n, Handle idx) {
    return emlite_val_has_impl(n, idx);
}

EMLITE_USED
bool emlite_val_is_string(Handle h) {
    return emlite_val_is_string_impl(h);
}

EMLITE_USED
bool emlite_val_is_number(Handle h) {
    return emlite_val_is_number_impl(h);
}

EMLITE_USED
bool emlite_val_not(Handle h) {
    return emlite_val_not_impl(h);
}

EMLITE_USED
bool emlite_val_gt(Handle a, Handle b) {
    return emlite_val_gt_impl(a, b);
}

EMLITE_USED
bool emlite_val_gte(Handle a, Handle b) {
    return emlite_val_gte_impl(a, b);
}

EMLITE_USED
bool emlite_val_lt(Handle a, Handle b) {
    return emlite_val_lt_impl(a, b);
}

EMLITE_USED
bool emlite_val_lte(Handle a, Handle b) {
    return emlite_val_lte_impl(a, b);
}

EMLITE_USED
bool emlite_val_equals(Handle a, Handle b) {
    return emlite_val_equals_impl(a, b);
}

EMLITE_USED
bool emlite_val_strictly_equals(Handle a, Handle b) {
    return emlite_val_strictly_equals_impl(a, b);
}

EMLITE_USED
bool emlite_val_instanceof(Handle a, Handle b) {
    return emlite_val_instanceof_impl(a, b);
}

EMLITE_USED
void emlite_val_throw(Handle arg) {
    emlite_val_throw_impl(arg);
}

EMLITE_USED
Handle emlite_val_obj_call(
    Handle obj, const char *name, size_t len, Handle argv
) {
    return emlite_val_obj_call_impl(obj, name, len, argv);
}

EMLITE_USED
bool emlite_val_obj_has_own_prop(
    Handle obj, const char *prop, size_t len
) {
    return emlite_val_obj_has_own_prop_impl(obj, prop, len);
}

EMLITE_USED
Handle emlite_val_make_callback(Handle fidx, Handle data) {
    return emlite_val_make_callback_impl(fidx, data);
}

EMLITE_USED
void emlite_print_object_map() {
    emlite_print_object_map_impl();
}

EMLITE_USED
void emlite_reset_object_map() {
    emlite_reset_object_map_impl();
}

EMLITE_USED
void emlite_val_inc_ref(Handle h) {
    emlite_val_inc_ref_impl(h);
}

EMLITE_USED
void emlite_val_dec_ref(Handle h) {
    emlite_val_dec_ref_impl(h);
}