/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2019 Pavel Kovalenko
 */

#pragma once

#include "Common.hpp"
#include <limits> // std::numeric_limits
#include <cmath> // std::abs
#include <type_traits> // enable_if, is_floating_point

template <typename Scalar>
struct Vector2T
{
    using Vec2 = Vector2T<Scalar>;

    Scalar X, Y;

    Vector2T() = default;

    constexpr Vector2T(Scalar x, Scalar y) :
        X(x),
        Y(y)
    {}

    constexpr Scalar &operator[](size_t i)
    {
        switch (i)
        {
        case 0: return X;
        case 1: return Y;
        default: R_ASSERT(!"Index out of range");
        }
    }

    constexpr Scalar const &operator[](size_t i) const
    { return const_cast<Vector2T &>(*this)[i]; }

    constexpr Vec2 operator+(Vec2 v) const
    { return Vec2(X+v.X, Y+v.Y); }

    constexpr Vec2 operator-(Vec2 v) const
    { return Vec2(X-v.X, Y-v.Y); }

    constexpr Vec2 operator+(Scalar s) const
    { return Vec2(X+s, Y+s); }

    constexpr Vec2 operator-(Scalar s) const
    { return Vec2(X-s, Y-s); }

    constexpr Vec2 operator*(Scalar s) const
    { return Vec2(X*s, Y*s); }

    constexpr Vec2 operator/(Scalar s) const
    { return Vec2(X/s, Y/s); }

    constexpr Vec2 &operator+=(Scalar s)
    { return *this = *this + s; }

    constexpr Vec2 &operator-=(Scalar s)
    { return *this = *this - s; }

    constexpr Vec2 &operator+=(Vec2 v)
    { return *this = *this + v; }

    constexpr Vec2 &operator-=(Vec2 v)
    { return *this = *this - v; }

    constexpr Vec2 operator-() const
    { return Vec2(-X, -Y); }

    constexpr Vec2 operator+() const
    { return *this; }

    constexpr Scalar SqrLength() const
    { return X*X + Y*Y; }

    constexpr bool operator<(Vec2 v2) const
    { return SqrLength() < v2.SqrLength(); }

    constexpr bool operator>(Vec2 v2) const
    { return SqrLength() > v2.SqrLength(); }

    constexpr bool operator<=(Vec2 v2) const
    { return SqrLength() <= v2.SqrLength(); }

    constexpr bool operator>=(Vec2 v2) const
    { return SqrLength() >= v2.SqrLength(); }

    bool operator==(Vec2 v2) const
    { return std::abs(X-v2.X) <= ScalarEps && std::abs(Y-v2.Y) <= ScalarEps; }

    bool operator!=(Vec2 v2) const
    { return !(*this==v2); }

    Scalar Length() const
    { return std::sqrt(SqrLength()); }

    Scalar Magnitude() const
    { return Length(); }

    Vec2 Normalize() const
    { return *this / Length(); }

private:
    template <typename T>
    static constexpr bool IsFP = std::is_floating_point<T>::value;

public:
    // non-fp to anything, fp to other fp
    template <typename T,
        typename S = Scalar,
        typename = std::enable_if_t<!IsFP<S> || IsFP<S> && IsFP<T>>
    >
    constexpr operator Vector2T<T>() const
    { return {T(X), T(Y)}; }

    // fp to non-fp
    template <typename T,
        typename = void,
        typename S = Scalar,
        typename = std::enable_if_t<IsFP<S> && !IsFP<T>>
    >
    operator Vector2T<T>() const
    { return {T(std::round(X)), T(std::round(Y))}; }

private:
    using Limits = std::numeric_limits<Scalar>;

public:
    static constexpr Scalar ScalarEps{Limits::epsilon()};
    static const Vec2 MinValue;
    static const Vec2 MaxValue;
    static const Vec2 Epsilon;
    static const Vec2 Origin;
};

using Vector2 = Vector2T<float>;
using Vector2f = Vector2T<float>;
using Vector2d = Vector2T<double>;
using Vector2i = Vector2T<int32_t>;

template <typename Scalar>
CONSTEXPR_DEF Vector2T<Scalar> Vector2T<Scalar>::MinValue{Limits::min(), Limits::min()};
template <typename Scalar>
CONSTEXPR_DEF Vector2T<Scalar> Vector2T<Scalar>::MaxValue{Limits::max(), Limits::max()};
template <typename Scalar>
CONSTEXPR_DEF Vector2T<Scalar> Vector2T<Scalar>::Epsilon{ScalarEps, ScalarEps};
template <typename Scalar>
CONSTEXPR_DEF Vector2T<Scalar> Vector2T<Scalar>::Origin{0, 0};
