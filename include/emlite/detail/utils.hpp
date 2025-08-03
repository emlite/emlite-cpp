#pragma once

#include "tiny_traits.hpp"

/// Null type for Option
struct NullOpt {
    explicit constexpr NullOpt() = default;
};
constexpr NullOpt nullopt{};

// Forward declaration for error handling
namespace emlite {
class Val;
}

/// Custom Option class (no std library dependency)
template <typename T>
class Option {
  public:
    using value_type = T;

  private:
    bool has_value_;
    union {
        T value_;
        char dummy_;
    };

  public:
    Option() noexcept : has_value_(false), dummy_(0) {}

    Option(NullOpt) noexcept : has_value_(false), dummy_(0) {}

    Option(T value) noexcept : has_value_(true), value_(move(value)) {}

    Option(const Option &other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(other.value_);
        } else {
            dummy_ = 0;
        }
    }

    Option &operator=(const Option &other) noexcept {
        if (this == &other)
            return *this;

        if (has_value_) {
            value_.~T();
        }

        has_value_ = other.has_value_;
        if (has_value_) {
            new (&value_) T(other.value_);
        } else {
            dummy_ = 0;
        }
        return *this;
    }

    Option(Option &&other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(move(other.value_));
            other.value_.~T();
        } else {
            dummy_ = 0;
        }
        other.has_value_ = false;
        other.dummy_     = 0;
    }

    Option &operator=(Option &&other) noexcept {
        if (this == &other)
            return *this;

        if (has_value_) {
            value_.~T();
        }

        has_value_ = other.has_value_;
        if (has_value_) {
            new (&value_) T(move(other.value_));
            other.value_.~T();
        } else {
            dummy_ = 0;
        }
        other.has_value_ = false;
        other.dummy_     = 0;
        return *this;
    }

    ~Option() {
        if (has_value_) {
            value_.~T();
        }
    }

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    explicit operator bool() const noexcept { return has_value_; }

    const T &value() const;
    T &value();

    const T &operator*() const { return value(); }
    T &operator*() { return value(); }

    const T *operator->() const { return &value(); }
    T *operator->() { return &value(); }

    T value_or(const T &default_value) const { return has_value_ ? value_ : default_value; }

    T unwrap() const { return value(); }

    T expect(const char *message) const;

    void reset() {
        if (has_value_) {
            value_.~T();
            has_value_ = false;
            dummy_     = 0;
        }
    }

    template <typename F>
    auto map(F &&func) const -> Option<decltype(func(value_))> {
        using U = decltype(func(value_));
        if (has_value_) {
            return Option<U>(forward<F>(func)(value_));
        }
        return Option<U>();
    }
};

/// Create Option with value
template <typename T>
Option<T> some(T &&value) {
    return Option<T>(forward<T>(value));
}

/// Create empty Option
template <typename T>
Option<T> none() {
    return Option<T>();
}

/// Custom Result class (no std library dependency)
template <typename T, typename E = emlite::Val>
class Result {
  public:
    using value_type = T;
    using error_type = E;

  private:
    bool has_value_;
    bool has_error_;
    union {
        T value_;
        E error_;
        char dummy_{};
    };

  public:
    Result(T value) noexcept : has_value_(true), has_error_(false), value_(move(value)) {}

    Result(E error) noexcept : has_value_(false), has_error_(true), error_(move(error)) {}

    Result(const Result &other) noexcept
        : has_value_(other.has_value_), has_error_(other.has_error_) {
        if (has_value_) {
            new (&value_) T(other.value_);
        } else if (has_error_) {
            new (&error_) E(other.error_);
        } else {
            dummy_ = 0;
        }
    }

    Result &operator=(const Result &other) noexcept {
        if (this == &other)
            return *this;

        if (has_value_) {
            value_.~T();
        } else if (has_error_) {
            error_.~E();
        }

        has_value_ = other.has_value_;
        has_error_ = other.has_error_;

        if (has_value_) {
            new (&value_) T(other.value_);
        } else if (has_error_) {
            new (&error_) E(other.error_);
        } else {
            dummy_ = 0;
        }
        return *this;
    }

    Result(Result &&other) noexcept : has_value_(other.has_value_), has_error_(other.has_error_) {
        if (has_value_) {
            new (&value_) T(move(other.value_));
            other.value_.~T();
        } else if (has_error_) {
            new (&error_) E(move(other.error_));
            other.error_.~E();
        } else {
            dummy_ = 0;
        }
        other.has_value_ = false;
        other.has_error_ = false;
        other.dummy_     = 0;
    }

    Result &operator=(Result &&other) noexcept {
        if (this == &other)
            return *this;

        if (has_value_) {
            value_.~T();
        } else if (has_error_) {
            error_.~E();
        }

        has_value_ = other.has_value_;
        has_error_ = other.has_error_;

        if (has_value_) {
            new (&value_) T(move(other.value_));
            other.value_.~T();
        } else if (has_error_) {
            new (&error_) E(move(other.error_));
            other.error_.~E();
        } else {
            dummy_ = 0;
        }
        other.has_value_ = false;
        other.has_error_ = false;
        other.dummy_     = 0;
        return *this;
    }

    ~Result() {
        if (has_value_) {
            value_.~T();
        } else if (has_error_) {
            error_.~E();
        }
    }

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] bool is_error() const noexcept { return has_error_; }
    explicit operator bool() const noexcept { return has_value_; }

    const T &value() const;
    T &value();

    const T &operator*() const { return value(); }
    T &operator*() { return value(); }

    const T *operator->() const { return &value(); }
    T *operator->() { return &value(); }

    T value_or(const T &default_value) const { return has_value_ ? value_ : default_value; }

    T unwrap() const { return value(); }

    [[nodiscard]] const E &error() const;

    template <typename F>
    auto map(F &&func) const -> Result<decltype(func(value_)), E> {
        using U = decltype(func(value_));
        if (has_value_) {
            return Result<U, E>(forward<F>(func)(value_));
        } else if (has_error_) {
            return Result<U, E>(error_);
        } else {
            return Result<U, E>(E{});
        }
    }

    template <typename F>
    auto and_then(F &&func) const -> decltype(func(value_)) {
        if (has_value_) {
            return forward<F>(func)(value_);
        } else if (has_error_) {
            return decltype(func(value_))(error_);
        } else {
            return decltype(func(value_))(E{});
        }
    }
};

/// Create Result with success value
template <typename T, typename E>
Result<T, E> ok(T &&value) {
    return Result<T, E>(forward<T>(value));
}

/// Create Result with error value
template <typename T, typename E>
Result<T, E> err(E &&error) {
    return Result<T, E>(forward<E>(error));
}