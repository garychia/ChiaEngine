#ifndef TYPES_HPP
#define TYPES_HPP

namespace Types
{
#define BOOL_TYPE_DECLARATION(name, value)                                                                             \
    template <class T> struct name                                                                                     \
    {                                                                                                                  \
        static constexpr bool Value = value;                                                                           \
    }

#define BOOL_TYPE_SPECIALIZATION(type, name, value)                                                                    \
    template <> struct name<type>                                                                                      \
    {                                                                                                                  \
        static constexpr bool Value = value;                                                                           \
    }

BOOL_TYPE_DECLARATION(IsInteger, false);

BOOL_TYPE_SPECIALIZATION(char, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(short, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(int, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(long, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(long long, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(unsigned char, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(unsigned short, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(unsigned int, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(unsigned long, IsInteger, true);
BOOL_TYPE_SPECIALIZATION(unsigned long long, IsInteger, true);

BOOL_TYPE_DECLARATION(IsFloat, false);

BOOL_TYPE_SPECIALIZATION(float, IsFloat, true);
BOOL_TYPE_SPECIALIZATION(double, IsFloat, true);

BOOL_TYPE_DECLARATION(IsChar, false);

BOOL_TYPE_SPECIALIZATION(char, IsChar, true);
BOOL_TYPE_SPECIALIZATION(wchar_t, IsChar, true);
BOOL_TYPE_SPECIALIZATION(char16_t, IsChar, true);
BOOL_TYPE_SPECIALIZATION(char32_t, IsChar, true);

BOOL_TYPE_DECLARATION(IsConst, false);
BOOL_TYPE_DECLARATION(IsConst<const T>, true);

BOOL_TYPE_DECLARATION(IsVolatile, false);
BOOL_TYPE_DECLARATION(IsVolatile<volatile T>, true);

template <class T> struct RemoveConst
{
    using Type = T;
};

template <class T> struct RemoveConst<const T>
{
    using Type = T;
};

template <class T> struct RemoveReference
{
    using Type = T;
};

template <class T> struct RemoveReference<T &>
{
    using Type = T;
};

template <class T> struct RemoveReference<T &&>
{
    using Type = T;
};

template <class T> constexpr typename RemoveReference<T>::Type &&Move(T &&arg) noexcept
{
    return static_cast<typename RemoveReference<T>::Type &&>(arg);
}

template <class T> constexpr T &&Forward(typename RemoveReference<T>::Type &arg) noexcept
{
    return static_cast<T &&>(arg);
}

template <class T> constexpr T &&Forward(typename RemoveReference<T>::Type &&arg) noexcept
{
    return static_cast<T &&>(arg);
}
} // namespace Types

#endif // TYPES_HPP
