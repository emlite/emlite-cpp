#pragma once

#include <stddef.h>

template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T &> {
    using type = T;
};

template <typename T>
struct remove_reference<T &&> {
    using type = T;
};

template <class T>
using remove_reference_t =
    typename remove_reference<T>::type;

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &t
) noexcept {
    return static_cast<T &&>(t);
}

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &&t
) noexcept {
    static_assert(
        !__is_lvalue_reference(T),
        "forwarding an rvalue as an lvalue"
    );
    return static_cast<T &&>(t);
}

template <class T, T V>
struct integral_constant {
    static constexpr T value = V;
    using value_type         = T;
    using type               = integral_constant;
    constexpr operator value_type() const noexcept {
        return value;
    }
};

using true_type  = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template <class B, class D>
using convert_test =
    decltype(static_cast<const volatile B *>(
        static_cast<D *>(nullptr)
    ));

template <bool B, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> {
    using type = T;
};

template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <typename T, typename U>
struct is_same {
    static constexpr bool value = false;
};

template <typename T>
struct is_same<T, T> {
    static constexpr bool value = true;
};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

template <typename T>
struct is_integral {
    static constexpr bool value = false;
};

template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;

template <typename T>
struct is_floating_point {
    static constexpr bool value = false;
};

template <typename T>
constexpr bool is_floating_point_v =
    is_floating_point<T>::value;

template <typename T>
struct is_signed {
    static constexpr bool value = false;
};

template <typename T>
constexpr bool is_signed_v = is_signed<T>::value;

#define IS_INTEGRAL(x)                                     \
    template <>                                            \
    struct is_integral<x> {                                \
        static constexpr bool value = true;                \
    };

#define IS_SIGNED(x)                                       \
    template <>                                            \
    struct is_signed<x> {                                  \
        static constexpr bool value = true;                \
    };

#define IS_UNSIGNED(x)                                     \
    template <>                                            \
    struct is_signed<x> {                                  \
        static constexpr bool value = false;               \
    };

#define IS_FLOATING(x)                                     \
    template <>                                            \
    struct is_floating_point<x> {                          \
        static constexpr bool value = true;                \
    };

IS_INTEGRAL(bool)
IS_INTEGRAL(char)
IS_INTEGRAL(signed char)
IS_INTEGRAL(unsigned char)
IS_INTEGRAL(wchar_t)
IS_INTEGRAL(char16_t)
IS_INTEGRAL(char32_t)
IS_INTEGRAL(short)
IS_INTEGRAL(unsigned short)
IS_INTEGRAL(int)
IS_INTEGRAL(unsigned int)
IS_INTEGRAL(long)
IS_INTEGRAL(unsigned long)
IS_INTEGRAL(long long)
IS_INTEGRAL(unsigned long long)

// Signed/unsigned type traits
IS_SIGNED(signed char)
IS_SIGNED(short)
IS_SIGNED(int)
IS_SIGNED(long)
IS_SIGNED(long long)
IS_UNSIGNED(bool)
IS_UNSIGNED(char
) // char signedness is implementation-defined, assuming
  // unsigned
IS_UNSIGNED(unsigned char)
IS_UNSIGNED(wchar_t) // Usually unsigned
IS_UNSIGNED(char16_t)
IS_UNSIGNED(char32_t)
IS_UNSIGNED(unsigned short)
IS_UNSIGNED(unsigned int)
IS_UNSIGNED(unsigned long)
IS_UNSIGNED(unsigned long long)

IS_FLOATING(float)
IS_FLOATING(double)
IS_FLOATING(long double)

template <class Base, class Derived, class = void>
struct is_base_of : false_type {};

template <class...>
using void_t = void;

template <class Base, class Derived>
struct is_base_of<
    Base,
    Derived,
    void_t<decltype(static_cast<const volatile Base *>(
        static_cast<Derived *>(nullptr)
    ))>> : true_type {};

template <class Base, class Derived>
inline constexpr bool is_base_of_v =
    is_base_of<Base, Derived>::value;

template <class T>
typename remove_reference<T>::type &&declval() noexcept;

template <class, class>
struct is_convertible;

namespace detail {
template <class F, class T>
static auto test(int
) -> decltype(static_cast<T>(*static_cast<F *>(nullptr)), char{});

template <class, class>
static auto test(...) -> long;
} // namespace detail

template <class F, class T>
struct is_convertible {
    enum { value = sizeof(detail::test<F, T>(0)) == 1 };
};

template <class F>
struct is_convertible<F, void> {
    enum { value = 1 };
};

template <class T>
struct is_convertible<void, T> {
    enum { value = 0 };
};

template <>
struct is_convertible<void, void> {
    enum { value = 1 };
};

template <class F, class T>
constexpr bool is_convertible_v =
    is_convertible<F, T>::value;

template <size_t... I>
struct index_sequence {};
template <size_t N, size_t... I>
struct make_index_sequence_impl
    : make_index_sequence_impl<N - 1, N - 1, I...> {};
template <size_t... I>
struct make_index_sequence_impl<0, I...> {
    using type = index_sequence<I...>;
};
template <size_t N>
using make_index_sequence =
    typename make_index_sequence_impl<N>::type;

template <class T>
constexpr remove_reference_t<T> &&move(T &&t) noexcept {
    return static_cast<remove_reference_t<T> &&>(t);
}

// Forward declarations for Option and Result detection
namespace emlite {
    template<typename T> class Option;
    template<typename T, typename E> class Result;
}

// Type traits for Option detection
template<typename T>
struct is_option : false_type {};

template<typename T>
struct is_option<emlite::Option<T>> : true_type {};

template<typename T>
constexpr bool is_option_v = is_option<T>::value;

// Type traits for Result detection  
template<typename T>
struct is_result : false_type {};

template<typename T, typename E>
struct is_result<emlite::Result<T, E>> : true_type {};

template<typename T>
constexpr bool is_result_v = is_result<T>::value;