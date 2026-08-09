/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2019 Pavel Kovalenko
 */

#pragma once

#include <algorithm> // std::min, std::max
#include <limits> // std::numeric_limits

template <typename T>
struct Vector2T;

template <typename S>
struct Box2T
{
    using Scalar = S;
    using Vec2 = Vector2T<Scalar>;
    using Box2 = Box2T<Scalar>;

    Vec2 Min, Max;

    Box2T() = default;

    constexpr Box2T(Vec2 size) :
        Min(size/2),
        Max(-size/2)
    {}

    constexpr Box2T(Vec2 min, Vec2 max) :
        Min(min),
        Max(max)
    {}

    constexpr Box2T(Vec2 center, Scalar radius) :
        Min(center - Vec2(radius, radius)),
        Max(center + Vec2(radius, radius))
    {}

    constexpr Scalar Width() const
    { return Max.X-Min.X; }

    constexpr Scalar Height() const
    { return Max.Y-Min.Y; }

    constexpr Vec2 Center() const
    { return Min + (Max-Min)/2; }

    constexpr Vec2 Size() const
    { return Vec2(Width(), Height()); }

    constexpr bool Contains(Vec2 v) const
    { return Min.X<=v.X && v.X<=Max.X && Min.Y<=v.Y && v.Y<=Max.Y; }

    bool Contains(Scalar x, Scalar y) const
    { return Contains({x, y}); }

    bool Contains(Box2T const &b) const
    { return Contains(b.Min) && Contains(b.Max); }

    Box2T &Merge(Vec2 p)
    {
        Min = {std::min(Min.X, p.X), std::min(Min.Y, p.Y)};
        Max = {std::max(Max.X, p.X), std::max(Max.Y, p.Y)};
        return *this;
    }

    Box2T &Merge(Box2T const &b)
    {
        Merge(b.Min);
        Merge(b.Max);
        return *this;
    }

    Box2T &Shrink(Scalar s)
    {
        Min += s;
        Max -= s;
        return *this;
    }

    Box2T &Grow(Scalar s)
    {
        Min -= s;
        Max += s;
        return *this;
    }

    bool operator==(Box2T const &rhs) const
    { return Min==rhs.Min && Max==rhs.Max; }
    
    bool operator!=(Box2T const &rhs) const
    { return Min!=rhs.Min || Max!=rhs.Max; }

    bool IsEmpty() const
    { return Min.X>Max.X || Min.Y>Max.Y; }

    Box2T &operator+=(Vec2 offset)
    {
        Min += offset;
        Max += offset;
        return *this;
    }

    Box2T &operator-=(Vec2 offset)
    {
        Min -= offset;
        Max -= offset;
        return *this;
    }

    Box2T operator+(Vec2 offset) const
    { return {Min+offset, Max+offset}; }

    Box2T operator-(Vec2 offset) const
    { return {Min-offset, Max-offset}; }

    template <typename T>
    operator Box2T<T>() const { return {Vector2T<T>(Min), Vector2T<T>(Max)}; }

private:
    using Limits = std::numeric_limits<Scalar>;

public:
    static constexpr Scalar ScalarEps{Limits::epsilon()};
    static const Box2T Empty;
};

using Box2 = Box2T<float>;
using Box2f = Box2T<float>;
using Box2d = Box2T<double>;
using Box2i = Box2T<int32_t>;

template <typename Scalar>
CONSTEXPR_DEF Box2T<Scalar> Box2T<Scalar>::Empty
    {Vector2T<Scalar>::MaxValue, Vector2T<Scalar>::MinValue};
