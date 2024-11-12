/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT

    Standard vectors for our small floats.
*/

#pragma once
#include "bfloat16.hpp"
#include "float16.hpp"

// Prefer 16-bit alignment over natural alignment (possible performance-hit for vec3_t in some cases).
#pragma pack(push, 2)

// 32-bit.
template <typename T> struct vec2_t
{
    T x{}, y{};

    constexpr vec2_t() = default;
    constexpr operator bool() const { return !!(x + y); }
    constexpr vec2_t(const vec2_t &Other) { x = Other.x; y = Other.y; }
    template <typename U> constexpr operator U() const { return { x, y }; }
    template <typename A, typename B> constexpr vec2_t(A X, B Y) : x(X), y(Y) {}

    // For handling POINT and similar structs.
    template <typename U> requires (requires { [](U &This) { auto &[a1, a2] = This; }; })
    constexpr vec2_t(const U &Object)
    {
         const auto &[a1, a2] = Object;
         *this = vec2_t{ a1, a2 };
    }

    constexpr T operator[](size_t i) const
    {
        ASSERT(i > 1);

        if (i == 0) return x;
        if (i == 1) return y;

        std::unreachable();
    }
    T &operator[](size_t i)
    {
        ASSERT(i > 1);

        if (i == 0) return x;
        if (i == 1) return y;

        std::unreachable();
    }

    constexpr bool operator!=(const vec2_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec2_t &Right) const { return x == Right.x && y == Right.y; }
    constexpr bool operator<(const vec2_t &Right)  const { return (x < Right.x) || (y < Right.y); }
    constexpr bool operator>(const vec2_t &Right)  const { return (x > Right.x) || (y > Right.y); }
    constexpr bool operator<=(const vec2_t &Right) const { return (x <= Right.x) || (y <= Right.y); }
    constexpr bool operator>=(const vec2_t &Right) const { return (x >= Right.x) || (y >= Right.y); }

    constexpr vec2_t &operator*=(const T &Right) { x *= Right; y *= Right; return *this; }
    constexpr vec2_t &operator/=(const T &Right) { x /= Right; y /= Right; return *this; }
    constexpr vec2_t &operator+=(const vec2_t &Right) { x += Right.x; y += Right.y; return *this; }
    constexpr vec2_t &operator-=(const vec2_t &Right) { x -= Right.x; y -= Right.y; return *this; }

    constexpr friend vec2_t operator*(vec2_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec2_t operator/(vec2_t Left, const T &Right) { Left /= Right; return Left; }
    constexpr friend vec2_t operator+(vec2_t Left, const vec2_t &Right) { Left += Right; return Left; }
    constexpr friend vec2_t operator-(vec2_t Left, const vec2_t &Right) { Left -= Right; return Left; }
};
using vec2f = vec2_t<float16_t>;
using vec2u = vec2_t<uint16_t>;
using vec2i = vec2_t<int16_t>;

// 48-bit.
template <typename T> struct vec3_t
{
    T x{}, y{}, z{};

    constexpr vec3_t() = default;
    constexpr operator bool() const { return !!(x + y + z); }
    template <typename U> constexpr operator U() const { return { x, y, z }; }
    constexpr vec3_t(const vec3_t &Other) { x = Other.x; y = Other.y; z = Other.z; }
    template <typename A, typename B, typename C> constexpr vec3_t(A X, B Y, C Z) : x(X), y(Y), z(Z) {}

    // For handling RGBTRIPPLE and similar structs.
    template <typename U> requires (requires { [](U &This) { auto &[a1, a2, a3] = This; }; })
    constexpr vec3_t(const U &Object)
    {
         const auto &[a1, a2, a3] = Object;
         *this = vec3_t{ a1, a2, a3 };
    }

    constexpr T operator[](size_t i) const
    {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;

        std::unreachable();
    }
    T &operator[](size_t i)
    {
        ASSERT(i > 2);

        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;

        std::unreachable();
    }

    constexpr bool operator!=(const vec3_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec3_t &Right) const { return x == Right.x && y == Right.y && z == Right.z; }
    constexpr bool operator<(const vec3_t &Right)  const { return (x < Right.x) || (y < Right.y) || (z < Right.z); }
    constexpr bool operator>(const vec3_t &Right)  const { return (x > Right.x) || (y > Right.y) || (z > Right.z); }
    constexpr bool operator<=(const vec3_t &Right) const { return (x <= Right.x) || (y <= Right.y) || (z <= Right.z); }
    constexpr bool operator>=(const vec3_t &Right) const { return (x >= Right.x) || (y >= Right.y) || (z >= Right.z); }

    constexpr vec3_t &operator*=(const T &Right) { x *= Right; y *= Right; z *= Right; return *this; }
    constexpr vec3_t &operator+=(const vec3_t &Right) { x += Right.x; y += Right.y; z += Right.z; return *this; }
    constexpr vec3_t &operator-=(const vec3_t &Right) { x -= Right.x; y -= Right.y; z -= Right.z; return *this; }

    constexpr friend vec3_t operator*(vec3_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec3_t operator+(vec3_t Left, const vec3_t &Right) { Left += Right; return Left; }
    constexpr friend vec3_t operator-(vec3_t Left, const vec3_t &Right) { Left -= Right; return Left; }
};
using vec3f = vec3_t<float16_t>;
using vec3u = vec3_t<uint16_t>;
using vec3i = vec3_t<int16_t>;

// 64-bit.
template <typename T> struct vec4_t
{
    union
    {
        #pragma warning (suppress: 4201)
        struct { vec2_t<T> ab, cd; };
        #pragma warning (suppress: 4201)
        struct { T x, y, z, w; };
    };

    constexpr vec4_t() { x = y = z = w = 0; }
    constexpr vec4_t(vec2_t<T> X, vec2_t<T> Y) : ab(X), cd(Y) {}
    template <typename U> constexpr operator U() const { return { x, y, z, w }; }
    constexpr vec4_t(const vec4_t &Other) { x = Other.x; y = Other.y; z = Other.z; w = Other.w; }
    template <typename A, typename B, typename C, typename D> constexpr vec4_t(A X, B Y, C Z, D W) : x(X), y(Y), z(Z), w(W) {}

    // For handling RECT and similar structs.
    template <typename U> requires (requires { [](U &This) { auto &[a1, a2, a3, a4] = This; }; })
    constexpr vec4_t(const U &Object)
    {
         const auto &[a1, a2, a3, a4] = Object;
         *this = vec4_t{ a1, a2, a3, a4 };
    }

    constexpr T operator[](size_t i) const
    {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        if (i == 3) return w;

        std::unreachable();
    }
    T &operator[](size_t i)
    {
        ASSERT(i > 3);

        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        if (i == 3) return w;

        std::unreachable();
    }

    constexpr operator bool() const { return !!(x + y + z + w); }
    constexpr bool operator!=(const vec4_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const vec4_t &Right) const { return ab == Right.ab && cd == Right.cd; }
    constexpr bool operator<(const vec4_t &Right)  const { return (ab < Right.ab) || (cd < Right.cd); }
    constexpr bool operator>(const vec4_t &Right)  const { return (ab > Right.ab) || (cd > Right.cd); }
    constexpr bool operator<=(const vec4_t &Right) const { return (ab <= Right.ab) || (cd <= Right.cd); }
    constexpr bool operator>=(const vec4_t &Right) const { return (ab >= Right.ab) || (cd >= Right.cd); }

    constexpr vec4_t &operator*=(const T &Right) { ab *= Right; cd *= Right; return *this; }
    constexpr vec4_t &operator+=(const vec4_t &Right) { ab += Right.ab; cd += Right.cd; return *this; }
    constexpr vec4_t &operator-=(const vec4_t &Right) { ab -= Right.ab; cd -= Right.cd; return *this; }

    constexpr friend vec4_t operator*(vec4_t Left, const T &Right) { Left *= Right; return Left; }
    constexpr friend vec4_t operator+(vec4_t Left, const vec4_t &Right) { Left += Right; return Left; }
    constexpr friend vec4_t operator-(vec4_t Left, const vec4_t &Right) { Left -= Right; return Left; }
};
using vec4f = vec4_t<float16_t>;
using vec4u = vec4_t<uint16_t>;
using vec4i = vec4_t<int16_t>;

#pragma pack(pop)
