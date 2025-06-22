// EM_JS and _EM_JS macros copied from https://github.com/emscripten-core/emscripten/blob/main/system/include/emscripten/em_js.h
// Copyright 2018 The Emscripten Authors.
// Licensed under MIT and the University of Illinois/NCSA Open Source License
#define _EM_JS(ret, c_name, js_name, params, code)         \
        ret c_name params EMLITE_IMPORT(js_name);          \
        __attribute__((visibility("hidden"))               \
        ) void *__em_js_ref_##c_name = (void *)&c_name;    \
        EMLITE_USED                                        \
        __attribute__((section("em_js"), aligned(1))       \
        ) char __em_js__##js_name[] = #params "<::>" code; 

#define EM_JS(ret, name, params, ...)                      \
    _EM_JS(ret, name, name, params, #__VA_ARGS__)          \


// clang-format off
EM_JS(Handle, emlite_val_null, (), { return 0; });

EM_JS(Handle, emlite_val_undefined, (), { return 1; });

EM_JS(Handle, emlite_val_false, (), { return 2; });

EM_JS(Handle, emlite_val_true, (), { return 3; });

EM_JS(Handle, emlite_val_global_this, (), { return 4; });

EM_JS(Handle, emlite_val_new_array, (), {
    return EMLITE_VALMAP.add([]);
});

EM_JS(Handle, emlite_val_new_object, (), {
    return EMLITE_VALMAP.add({});
});

EM_JS(char *, emlite_val_typeof, (Handle n), {
    const str = (typeof EMLITE_VALMAP.get(n)) + "\0";
    const len = Module.lengthBytesUTF8(str);
    const buf = _malloc(len);
    stringToUTF8(str, buf, len);
    return buf;
});

EM_JS(
    Handle,
    emlite_val_construct_new,
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
    emlite_val_func_call,
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

EM_JS(void, emlite_val_push, (Handle arr, Handle v), {
    EMLITE_VALMAP.get(arr).push(v);
});

EM_JS(Handle, emlite_val_make_int, (int t), {
    return EMLITE_VALMAP.add(t | 0);
});

EM_JS(Handle, emlite_val_make_double, (double t), {
    return EMLITE_VALMAP.add(t);
});

EM_JS(
    Handle,
    emlite_val_make_str,
    (const char *str, size_t len),
    { return EMLITE_VALMAP.add(UTF8ToString(str, len)); }
);

EM_JS(int, emlite_val_get_value_int, (Handle n), {
    return EMLITE_VALMAP.get(n);
});

EM_JS(double, emlite_val_get_value_double, (Handle n), {
    return EMLITE_VALMAP.get(n);
});

EM_JS(char *, emlite_val_get_value_string, (Handle n), {
    const str = EMLITE_VALMAP.get(n) + "\0";
    const len = Module.lengthBytesUTF8(str);
    const buf = _malloc(len);
    stringToUTF8(str, buf, len);
    return buf;
});

EM_JS(Handle, emlite_val_get_elem, (Handle n, size_t idx), {
    return EMLITE_VALMAP.add(EMLITE_VALMAP.get(n)[idx]);
});

EM_JS(bool, emlite_val_is_string, (Handle h), {
    const obj            = EMLITE_VALMAP.get(h);
    return typeof obj === "string" || obj instanceof
        String;
});

EM_JS(bool, emlite_val_is_number, (Handle arg), {
    return typeof EMLITE_VALMAP.get(arg) === "number";
});
EM_JS(bool, emlite_val_not, (Handle h), {
    return !EMLITE_VALMAP.get(h);
});

EM_JS(bool, emlite_val_gt, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) > EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_gte, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) >= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lt, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) < EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lte, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) <= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_equals, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) == EMLITE_VALMAP.get(b);
});

EM_JS(
    bool,
    emlite_val_strictly_equals,
    (Handle a, Handle b),
    { return EMLITE_VALMAP.get(a) === EMLITE_VALMAP.get(b); }
);

EM_JS(bool, emlite_val_instanceof, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) instanceof EMLITE_VALMAP.get(b);
});

EM_JS(void, emlite_val_throw, (Handle arg), { throw arg; });

EM_JS(
    Handle,
    emlite_val_obj_call,
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
    Handle,
    emlite_val_obj_prop,
    (Handle obj, const char *prop, size_t len),
    {
        const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        return EMLITE_VALMAP.add(target[p]);
    }
);

EM_JS(
    void,
    emlite_val_obj_set_prop,
    (Handle obj, const char *prop, size_t len, Handle val),
    {
        const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        target[p]    = EMLITE_VALMAP.get(val);
    }
);

EM_JS(
    bool,
    emlite_val_obj_has_prop,
    (Handle obj, const char *prop, size_t len),
    {
        const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        return Reflect.has(target, p);
    }
);

EM_JS(
    bool,
    emlite_val_obj_has_own_prop,
    (Handle obj, const char *prop, size_t len),
    {
        const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        return Object.prototype.hasOwnProperty.call(
            target, p
        );
    }
);

EM_JS(Handle, emlite_val_make_callback, (Handle fidx), {
    const id = emliteNextCbId++;
    EMLITE_CB_STORE.set(id, fidx);

    const jsFn = (... args) => {
        const arrHandle =
            EMLITE_VALMAP.add(args.map(v => EMLITE_VALMAP.add(v)));
        const ret = wasmTable.get(fidx)(arrHandle);

        return ret;
    };
    return EMLITE_VALMAP.add(jsFn);
});

EM_JS(void, emlite_print_object_map, (), {
    console.log(EMLITE_VALMAP);
});

EM_JS(void, emlite_reset_object_map, (), {
    for (const h of[... EMLITE_VALMAP._h2e.keys()]) {
        if (h > 4) {
            const value = EMLITE_VALMAP._h2e.get(h).value;

            EMLITE_VALMAP._h2e.delete(h);
            EMLITE_VALMAP._v2h.delete(value);
        }
    }
});

EM_JS(void, emlite_val_inc_ref, (Handle h), {
    EMLITE_VALMAP.incRef(h);
});

EM_JS(void, emlite_val_dec_ref, (Handle h), {
    if (h > 4) EMLITE_VALMAP.decRef(h);
});
// clang-format on