
// present in freestanding environments
#include <stdarg.h>

#if __has_include(<errno.h>)
#include <errno.h>
#else
extern _Thread_local int __errno_tls;
#define errno __errno_tls
#define ENOMEM 12
#endif

#if __has_include(<string.h>)
#include <string.h>
#else
size_t strlen(const char *s) {
    const char *p = s;
    while (*p)
        ++p;
    return (size_t)(p - s);
}
// needed for dlmalloc
void *memset(void *dest, int ch, size_t count) {
    return __builtin_memset(dest, ch, count);
}
void *memcpy(void *dest, const void *src, size_t n) {
    return __builtin_memcpy(dest, src, n);
}
#endif

#if __has_include(<stdlib.h>)
#include <stdlib.h>
#else
void abort(void) { __builtin_unreachable(); }

// If we don't have the above, we most likely don't have
// sbrk. If we link dlmalloc, it expects sbrk:
#define WASM_PAGESIZE 65536u

extern unsigned char __heap_base;
static uintptr_t heap_top = (uintptr_t)&__heap_base;

/* Round p up to the next multiple of n (n must be a
 * power-of-two). */
static inline uintptr_t align_up(uintptr_t p, uintptr_t n) {
    return (p + n - 1u) & ~(n - 1u);
}
// Code copied from wasi-libc implementation:
// https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/sources/sbrk.c
// Licensed by wasi-libc under MIT, Apache and Apache-LLVM
void *sbrk(intptr_t increment) {
    // sbrk(0) returns the current memory size.
    if (increment == 0) {
        // The wasm spec doesn't guarantee that memory.grow
        // of 0 always succeeds.
        return (void *)(__builtin_wasm_memory_size(0) *
                        WASM_PAGESIZE);
    }

    // We only support page-size increments.
    if (increment % WASM_PAGESIZE != 0) {
        abort();
    }

    // WebAssembly doesn't support shrinking linear memory.
    if (increment < 0) {
        abort();
    }

    uintptr_t old = __builtin_wasm_memory_grow(
        0, (uintptr_t)increment / WASM_PAGESIZE
    );

    if (old == SIZE_MAX) {
        errno = ENOMEM;
        return (void *)-1;
    }

    return (void *)(old * WASM_PAGESIZE);
}

void *emlite_malloc(size_t size) {
    uintptr_t aligned_top = align_up(heap_top, 8u);
    uintptr_t new_top     = aligned_top + (uintptr_t)size;

    uintptr_t cur_brk = (uintptr_t)sbrk(0);

    if (new_top > cur_brk) {
        uintptr_t diff      = new_top - cur_brk;
        uintptr_t increment = align_up(diff, WASM_PAGESIZE);
        if (sbrk((intptr_t)increment) == (void *)-1)
            return NULL;
    }
    heap_top = new_top;
    return (void *)aligned_top;
}
void emlite_free(void *ptr) { (void)ptr; }
void *emlite_realloc(void *old, size_t sz) {
    (void)old;
    return emlite_malloc(sz);
}

#ifndef HAVE_DLMALLOC
void *malloc(size_t s) { return emlite_malloc(s); }
void free(void *ptr) { emlite_free(ptr); }
void *realloc(void *ptr, size_t s) {
    return emlite_realloc(ptr, s);
}
#endif
#endif

#if __has_include(<stdio.h>)
#include <stdio.h>
#else
static size_t emlite_utoa(
    char *buf, unsigned long long value, int base, int upper
) {
    static const char digits_low[] = "0123456789abcdef";
    static const char digits_up[]  = "0123456789ABCDEF";
    const char *digits = upper ? digits_up : digits_low;

    size_t i = 0;
    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value) {
            buf[i++] = digits[value % base];
            value /= base;
        }

        for (size_t j = 0; j < i / 2; ++j) {
            char t         = buf[j];
            buf[j]         = buf[i - 1 - j];
            buf[i - 1 - j] = t;
        }
    }
    return i;
}

static inline __attribute__((__always_inline__)) int
vsnprintf(
    char *out, size_t n, const char *fmt, va_list ap
) {
    size_t pos = 0;

    while (*fmt) {
        if (*fmt != '%') {
            if (pos + 1 < n)
                out[pos] = *fmt;
            ++pos;
            ++fmt;
            continue;
        }

        ++fmt;

        if (*fmt == '%') {
            if (pos + 1 < n)
                out[pos] = '%';
            ++pos;
            ++fmt;
            continue;
        }

        int longflag = 0;
        while (*fmt == 'l') {
            ++longflag;
            ++fmt;
        }

        char tmp[32];
        const char *chunk = tmp;
        size_t chunk_len  = 0;
        int negative      = 0;

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            chunk     = s;
            chunk_len = strlen(s);
            break;
        }
        case 'c': {
            tmp[0]    = (char)va_arg(ap, int);
            chunk     = tmp;
            chunk_len = 1;
            break;
        }
        case 'd':
        case 'i': {
            long long v = longflag ? va_arg(ap, long long)
                                   : va_arg(ap, int);
            if (v < 0) {
                negative = 1;
                v        = -v;
            }
            chunk_len = emlite_utoa(
                tmp, (unsigned long long)v, 10, 0
            );
            break;
        }
        case 'u': {
            unsigned long long v =
                longflag ? va_arg(ap, unsigned long long)
                         : va_arg(ap, unsigned int);
            chunk_len = emlite_utoa(tmp, v, 10, 0);
            break;
        }
        case 'x':
        case 'X': {
            unsigned long long v =
                longflag ? va_arg(ap, unsigned long long)
                         : va_arg(ap, unsigned int);
            chunk_len =
                emlite_utoa(tmp, v, 16, (*fmt == 'X'));
            break;
        }
        default:
            tmp[0]    = '%';
            tmp[1]    = *fmt;
            chunk     = tmp;
            chunk_len = 2;
            break;
        }

        if (negative) {
            if (pos + 1 < n)
                out[pos] = '-';
            ++pos;
        }

        for (size_t i = 0; i < chunk_len; ++i) {
            if (pos + 1 < n)
                out[pos] = chunk[i];
            ++pos;
        }

        ++fmt;
    }

    if (n)
        out[(pos < n) ? pos : (n - 1)] = '\0';

    return (int)pos;
}

static inline __attribute__((__always_inline__)) int
snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return r;
}
#endif

em_Val em_Val_from_int(int i) {
    return (em_Val){.h = emlite_val_make_int(i)};
}

em_Val em_Val_from_double(double i) {
    return (em_Val){.h = emlite_val_make_double(i)};
}
em_Val em_Val_from_string(const char *s) {
    return (em_Val){.h = emlite_val_make_str(s, strlen(s))};
}

em_Val em_Val_from_handle(Handle v) {
    return (em_Val){.h = v};
}

em_Val em_Val_global(const char *name) {
    Handle global = emlite_val_global_this();
    return em_Val_from_handle(
        emlite_val_obj_prop(global, name, strlen(name))
    );
}

em_Val em_Val_global_this() {
    return em_Val_from_handle(emlite_val_global_this());
}

em_Val em_Val_null() { return em_Val_from_handle(0); }

em_Val em_Val_undefined() { return em_Val_from_handle(1); }

em_Val em_Val_object() {
    return em_Val_from_handle(emlite_val_new_object());
}

em_Val em_Val_array() {
    return em_Val_from_handle(emlite_val_new_array());
}

em_Val em_Val_make_fn(Callback f) {
    uint32_t fidx = (uint32_t)f;
    return em_Val_from_handle(emlite_val_make_callback(fidx)
    );
}

void em_Val_delete(em_Val v) { emlite_val_dec_ref(v.h); }

void em_Val_throw(em_Val v) { emlite_val_throw(v.h); }

Handle em_Val_as_handle(em_Val self) { return self.h; }

em_Val em_Val_get(em_Val self, const char *prop) {
    return em_Val_from_handle(
        emlite_val_obj_prop(self.h, prop, strlen(prop))
    );
}

void em_Val_set(em_Val self, const char *prop, em_Val val) {
    emlite_val_obj_set_prop(
        self.h, prop, strlen(prop), val.h
    );
}

bool em_Val_has(em_Val self, const char *prop) {
    return emlite_val_obj_has_prop(
        self.h, prop, strlen(prop)
    );
}

bool em_Val_has_own_property(
    em_Val self, const char *prop
) {
    return emlite_val_obj_has_own_prop(
        self.h, prop, strlen(prop)
    );
}

char *em_Val_typeof(em_Val self) {
    return emlite_val_typeof(self.h);
}

em_Val em_Val_at(em_Val self, size_t idx) {
    return em_Val_from_handle(
        emlite_val_get_elem(self.h, idx)
    );
}

em_Val em_Val_await(em_Val self) {
    return emlite_eval_v(
        "(async() => { let obj = EMLITE_VALMAP.toValue(%d); let "
        "ret = await obj; "
        "return EMLITE_VALMAP.toHandle(ret); })()",
        self.h
    );
}

bool em_Val_is_number(em_Val self) {
    return emlite_val_is_number(self.h);
}

bool em_Val_is_string(em_Val self) {
    return emlite_val_is_string(self.h);
}

bool em_Val_instanceof(em_Val self, em_Val v) {
    return emlite_val_instanceof(self.h, v.h);
}

bool em_Val_not(em_Val self) {
    return emlite_val_not(self.h);
}

bool em_Val_seq(em_Val self, em_Val other) {
    return emlite_val_strictly_equals(self.h, other.h);
}

bool em_Val_eq(em_Val self, em_Val other) {
    return emlite_val_equals(self.h, other.h);
}

bool em_Val_neq(em_Val self, em_Val other) {
    return !emlite_val_strictly_equals(self.h, other.h);
}

bool em_Val_gt(em_Val self, em_Val other) {
    return emlite_val_gt(self.h, other.h);
}

bool em_Val_gte(em_Val self, em_Val other) {
    return emlite_val_gte(self.h, other.h);
}

bool em_Val_lt(em_Val self, em_Val other) {
    return emlite_val_lt(self.h, other.h);
}

bool em_Val_lte(em_Val self, em_Val other) {
    return emlite_val_lte(self.h, other.h);
}

int em_Val_as_int(em_Val self) {
    return emlite_val_get_value_int(self.h);
}

bool em_Val_as_bool(em_Val self) { return self.h > 3; }

double em_Val_as_double(em_Val self) {
    return emlite_val_get_value_double(self.h);
}

char *em_Val_as_string(em_Val self) {
    return emlite_val_get_value_string(self.h);
}

em_Val em_Val_call(
    em_Val self, const char *method, int n, ...
) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    em_Val ret = em_Val_from_handle(emlite_val_obj_call(
        self.h, method, strlen(method), arr
    ));
    emlite_val_dec_ref(arr);
    return ret; 
}

em_Val em_Val_new(em_Val self, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    em_Val ret = em_Val_from_handle(
        emlite_val_construct_new(self.h, arr)
    );
    emlite_val_dec_ref(arr);
    return ret;
}

em_Val em_Val_invoke(em_Val self, int n, ...) {
    Handle arr = emlite_val_new_array();
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        em_Val c = va_arg(args, em_Val);
        emlite_val_push(arr, em_Val_as_handle(c));
    }
    va_end(args);
    em_Val ret = em_Val_from_handle(
        emlite_val_func_call(self.h, arr)
    );
    emlite_val_dec_ref(arr);
    return ret;
}

em_Val emlite_eval(const char *src) {
    em_Val eval = em_Val_global("eval");
    em_Val js_src = em_Val_from_string(src);
    em_Val ret = em_Val_invoke(eval, 1, js_src);
    em_Val_delete(js_src);
    em_Val_delete(eval);
    return ret;
}

em_Val emlite_eval_v(const char *src, ...) {
    va_list args;
    va_start(args, src);
    va_list args_len;
    va_copy(args_len, args);
    size_t len = vsnprintf(NULL, 0, src, args_len);
    va_end(args_len);
    char *ptr = (char *)malloc(len + 1);
    if (!ptr) {
        va_end(args);
        return em_Val_null();
    }
    (void)vsnprintf(ptr, len + 1, src, args);
    va_end(args);
    em_Val ret = emlite_eval(ptr);
    free(ptr);
    return ret;
}