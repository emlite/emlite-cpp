#pragma once

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

#define IS_INTEGRAL(x)                                     \
    template <>                                            \
    struct is_integral<x> {                                \
        static constexpr bool value = true;                \
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

