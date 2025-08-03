#pragma once

#include "tiny_traits.hpp"
#include <stddef.h>

/// A smart pointer class with unique ownershipt
template <class T, typename = enable_if_t<!is_same_v<T, void>>>
class Uniq {
    T *ptr_ = nullptr;

  public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(decltype(nullptr)) noexcept : ptr_(nullptr) {}
    explicit Uniq(T *p) noexcept : ptr_(p) {}

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

    Uniq(Uniq &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    Uniq &operator=(Uniq &&other) noexcept {
        if (this != &other) {
            reset();
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq &other) noexcept {
        auto tmp   = ptr_;
        ptr_       = other.ptr_;
        other.ptr_ = tmp;
    }

    void reset(T *p = nullptr) noexcept {
        if (ptr_)
            delete ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T *release() noexcept {
        T *tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T *get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T &operator*() const noexcept { return *ptr_; }
    T *operator->() const noexcept { return ptr_; }
};

/// A specialization of Uniq, the smart pointer with unique
/// ownership, for array types
template <class T>
class Uniq<T[]> {
    T *ptr_ = nullptr;

  public:
    using element_type = T;

    constexpr Uniq() noexcept = default;
    constexpr Uniq(decltype(nullptr)) noexcept : ptr_(nullptr) {}
    explicit Uniq(T *p) noexcept : ptr_(p) {}

    Uniq(const Uniq &)            = delete;
    Uniq &operator=(const Uniq &) = delete;

    Uniq(Uniq &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    Uniq &operator=(Uniq &&other) noexcept {
        if (this != &other) {
            reset();
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~Uniq() { reset(); }

    void swap(Uniq &other) noexcept {
        auto tmp   = ptr_;
        ptr_       = other.ptr_;
        other.ptr_ = tmp;
    }

    void reset(T *p = nullptr) noexcept {
        if (ptr_)
            delete[] ptr_;
        ptr_ = p;
    }

    [[nodiscard]] T *release() noexcept {
        T *tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    [[nodiscard]] T *get() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T &operator[](size_t i) const noexcept { return ptr_[i]; }
};

/// A helper swap function for Uniq objects
template <class T>
inline void swap(Uniq<T> &a, Uniq<T> &b) noexcept {
    a.swap(b);
}

/// A helper swap function for Uniq objects
template <class T>
inline void swap(Uniq<T[]> &a, Uniq<T[]> &b) noexcept {
    a.swap(b);
}