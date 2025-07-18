#pragma once

#include "tiny_traits.hpp"
#include <stddef.h>

template <class>
class Closure;

template <class R, class... Args>
class Closure<R(Args...)> {
    static constexpr size_t SBO_SIZE  = 3 * sizeof(void *);
    static constexpr size_t SBO_ALIGN = alignof(void *);

    struct vtable_t {
        R (*invoke)(const void *, Args &&...);
        void (*copy)(void *dest, const void *src);
        void (*move)(void *dest, void *src) noexcept;
        void (*destroy)(void *data) noexcept;
    };

    const vtable_t *_vt = nullptr;
    void *_ptr          = nullptr;
    alignas(SBO_ALIGN) unsigned char _sbo[SBO_SIZE];

    void *sbo_addr() noexcept {
        return static_cast<void *>(_sbo);
    }
    const void *sbo_addr() const noexcept {
        return static_cast<const void *>(_sbo);
    }
    bool is_sbo() const noexcept {
        return _ptr == sbo_addr();
    }

    template <class Fn>
    static const vtable_t *make_vtable() {
        static const vtable_t vt = {
            // invoke
            [](const void *data, Args &&...as) -> R {
                return (*static_cast<const Fn *>(data))(
                    static_cast<Args &&>(as)...
                );
            },
            // copy
            [](void *dest, const void *src) {
                new (dest)
                    Fn(*static_cast<const Fn *>(src));
            },
            // move
            [](void *dest, void *src) noexcept {
                new (dest) Fn(static_cast<Fn &&>(
                    *static_cast<Fn *>(src)
                ));
                static_cast<Fn *>(src)->~Fn();
            },
            // destroy
            [](void *data) noexcept {
                static_cast<Fn *>(data)->~Fn();
            }
        };
        return &vt;
    }

    template <class Fn>
    void assign(Fn f) {
        _vt = make_vtable<Fn>();

        if (sizeof(Fn) <= SBO_SIZE &&
            alignof(Fn) <= SBO_ALIGN) {
            _ptr = sbo_addr();
            new (_ptr) Fn(static_cast<Fn &&>(f));
        } else {
            static_assert(
                alignof(Fn) <= alignof(void *),
                "Closure: heap-allocated callable requires "
                "over-aligned new, which is not available."
            );
            _ptr = operator new(sizeof(Fn));
            new (_ptr) Fn(static_cast<Fn &&>(f));
        }
    }

  public:
    Closure() noexcept = default;

    Closure(decltype(nullptr)) noexcept {}

    Closure(const Closure &other) {
        if (!other)
            return;
        _vt = other._vt;
        if (other.is_sbo()) {
            _ptr = sbo_addr();
            _vt->copy(_ptr, other._ptr);
        } else {
            _ptr = operator new(SBO_SIZE);
            _vt->copy(_ptr, other._ptr);
        }
    }

    Closure(Closure &&other) noexcept {
        if (!other)
            return;
        _vt = other._vt;
        if (other.is_sbo()) {
            _ptr = sbo_addr();
            _vt->move(_ptr, other._ptr);
        } else {
            _ptr       = other._ptr;
            other._ptr = nullptr;
            other._vt  = nullptr;
        }
    }

    template <
        class Fn,
        enable_if_t<
            !is_same_v<remove_reference_t<Fn>, Closure> &&
                is_convertible_v<
                    decltype(declval<Fn &>()(declval<Args>(
                    )...)),
                    R>,
            int> = 0>
    Closure(Fn &&fn) {
        assign(static_cast<Fn &&>(fn));
    }

    ~Closure() { clear(); }

    Closure &operator=(decltype(nullptr)) noexcept {
        clear();
        return *this;
    }

    Closure &operator=(const Closure &rhs) {
        if (this == &rhs)
            return *this;
        clear();
        if (rhs) {
            _vt = rhs._vt;
            if (rhs.is_sbo()) {
                _ptr = sbo_addr();
                _vt->copy(_ptr, rhs._ptr);
            } else {
                _ptr = operator new(SBO_SIZE);
                _vt->copy(_ptr, rhs._ptr);
            }
        }
        return *this;
    }

    Closure &operator=(Closure &&rhs) noexcept {
        if (this == &rhs)
            return *this;
        clear();
        _vt = rhs._vt;
        if (rhs.is_sbo()) {
            _ptr = sbo_addr();
            _vt->move(_ptr, rhs._ptr);
        } else {
            _ptr     = rhs._ptr;
            rhs._ptr = nullptr;
            rhs._vt  = nullptr;
        }
        return *this;
    }

    template <class Fn>
    enable_if_t<
        !is_same_v<remove_reference_t<Fn>, Closure>,
        Closure &>
    operator=(Fn &&fn) {
        clear();
        assign(static_cast<Fn &&>(fn));
        return *this;
    }

    void clear() noexcept {
        if (!_vt)
            return;
        _vt->destroy(_ptr);
        if (!is_sbo())
            operator delete(_ptr);
        _vt  = nullptr;
        _ptr = nullptr;
    }

    explicit operator bool() const noexcept {
        return _vt != nullptr;
    }

    R operator()(Args... as) const {
        return _vt->invoke(
            _ptr, static_cast<Args &&>(as)...
        );
    }
};