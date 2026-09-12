#ifndef MATH_HPP
#define MATH_HPP

#include <cmath>
#include <cstddef>

namespace Math
{
class Constants
{
  public:
    static const double Ln2;
    static const double Pi;
};

template <class T> T Abs(const T x)
{
    return x >= 0 ? x : -x;
}

template <class T> T Min(const T x, const T y)
{
    return x < y ? x : y;
}

template <class T> T Max(const T x, const T y)
{
    return x > y ? x : y;
}

// Delegate to std (issue #88): the hand-rolled Taylor series were slower and
// less accurate than <cmath>. Overloads for the arithmetic types the engine
// passes (float/double) resolve to std::sin/cos/exp/pow etc.
template <class T> T Sine(const T &x)
{
    return static_cast<T>(std::sin(x));
}

template <class T> T Cosine(const T &x)
{
    return static_cast<T>(std::cos(x));
}

template <class T> T Tangent(const T &x)
{
    return static_cast<T>(std::tan(x));
}

template <class T> T Exponent(const T &x)
{
    return static_cast<T>(std::exp(x));
}

template <class T> T NaturalLog(const T &x)
{
    return static_cast<T>(std::log(x));
}

template <class T> T Sinh(const T &x)
{
    return static_cast<T>(std::sinh(x));
}

template <class T> T Cosh(const T &x)
{
    return static_cast<T>(std::cosh(x));
}

template <class T> T Tanh(const T &x)
{
    return static_cast<T>(std::tanh(x));
}

template <class T> T _PowerLong(const T &scaler, long n)
{
    if (scaler == 0 || scaler == 1)
        return scaler;
    else if (n == 0)
        return 1;
    // Fast integer-exponent path via squaring (kept from the original).
    auto p = n > 0 ? n : -n;
    T result = 1;
    T base = scaler;
    while (p > 0)
    {
        if (p & 1)
            result *= base;
        base *= base;
        p >>= 1;
    }
    return n > 0 ? result : T(1) / result;
}

template <class T, class PowerType> T Power(const T &scaler, PowerType n)
{
    if (scaler == 0)
        return 0;
    else if (n == 0)
        return 1;
    else if (n == 1)
        return scaler;
    else if ((long)n == n)
        return _PowerLong<T>(scaler, (long)n);
    return static_cast<T>(std::pow(scaler, n));
}

template <class T> T ReLU(const T &x)
{
    if (x < 0)
        return 0;
    return x;
}

template <class T> T Sigmoid(const T &x)
{
    return 1 / (1 + Exponent(-x));
}

template <class T> T Gauss(const T &x, const T &mu, const T &sigma)
{
    const T normalization = (x - mu) / sigma;
    return 1 / (sigma * Power(2 * Constants::Pi, 0.5)) * Exponent(-0.5 * normalization * normalization);
}

template <class T> float ToRadians(T degrees)
{
    return degrees / 180.f * Constants::Pi;
}

} // namespace Math

#endif