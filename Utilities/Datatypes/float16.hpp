/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT

    Integer-accurate until +/-2048. e.g. 2051.0f gets rounded to 2052.0f.
*/

#pragma once
#include <Stdinclude.hpp>

// NOTE(tcn): MSVC does not have planned support for extended-precision floats (P1467R9).
#if __has_include (<stdfloat>)
#include <stdfloat>
#endif

#if defined (__STDCPP_FLOAT16_T__)
using float16_t = std::float16_t;
#else
struct float16_t
{
    static constexpr float Epsilon = 9.765625e-4;
    uint16_t Value{};

    // AVX provides intrinsics for doing this, but seems slower for a single conversion.
    static constexpr float toFloat(const uint16_t Input)
    {
        const auto Words = uint32_t(Input) << 16;
        const auto Sign = Words & 0x80000000U;
        const auto Mantissa = Words + Words;

        // Denormal.
        if (Mantissa < 0x8000000U)
        {
            const auto Denormalized = std::bit_cast<float>((Mantissa >> 17) | 0x3F000000U) - 0.5f;
            return std::bit_cast<float>(Sign | std::bit_cast<uint32_t>(Denormalized));
        }
        else
        {
            const auto Scale = std::bit_cast<float>(0x7800000U);
            const auto Normalized = std::bit_cast<float>((Mantissa >> 4) + 0x70000000U) * Scale;
            return std::bit_cast<float>(Sign | std::bit_cast<uint32_t>(Normalized));
        }
    }
    static constexpr float toFloat(const float16_t &Input)
    {
        return toFloat(Input.Value);
    }
    static constexpr uint16_t fromFloat(const float Input)
    {
        constexpr auto zeroScale = std::bit_cast<float>(0x08800000U);
        constexpr auto infScale = std::bit_cast<float>(0x77800000U);

        const auto Words = std::bit_cast<uint32_t>(Input);
        const auto Sign = Words & 0x80000000U;
        const auto Mantissa = Words + Words;

        // Out of range.
        if (Mantissa > 0xFF000000U)
            return (Sign >> 16) | 0x7E00;

        const auto ABS = (Input >= 0) ? Input : -Input;
        const auto Normalized = ABS * (infScale * zeroScale);
        const auto Bias = std::max(Mantissa & 0xFF000000U, 0x71000000U);
        const auto Bits = std::bit_cast<uint32_t>(std::bit_cast<float>((Bias >> 1) + 0x07800000U) + Normalized);

        return (Sign >> 16) | (((Bits >> 13) & 0x00007C00U) + (Bits & 0x00000FFFU));
    }

    constexpr operator float() const { return toFloat(Value); }
    constexpr operator int32_t() const { return static_cast<int32_t>(toFloat(Value)); }

    constexpr float16_t() = default;
    constexpr float16_t(const float Input) : Value(fromFloat(Input)) {};
    explicit constexpr float16_t(const uint16_t Input) : Value(Input) {}
    template <std::integral T> constexpr float16_t(const T Input) : float16_t(static_cast<float>(Input)) {}

    constexpr bool operator<(const float16_t &Right)  const { return toFloat(Value) < toFloat(Right); }
    constexpr bool operator>(const float16_t &Right)  const { return toFloat(Value) > toFloat(Right); }
    constexpr bool operator<=(const float16_t &Right) const { return toFloat(Value) <= toFloat(Right); }
    constexpr bool operator>=(const float16_t &Right) const { return toFloat(Value) >= toFloat(Right); }
    constexpr bool operator!=(const float16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const float16_t &Right) const
    {
        if (Value == Right.Value) return true;

        const auto Temp = toFloat(Value) - toFloat(Right);
        const auto ABS = (Temp >= 0) ? Temp : -Temp;
        return ABS < Epsilon;
    }

    constexpr float16_t &operator+=(const float16_t &Right) { *this = (toFloat(*this) + toFloat(Right)); return *this; }
    constexpr float16_t &operator-=(const float16_t &Right) { *this = (toFloat(*this) - toFloat(Right)); return *this; }
    constexpr float16_t &operator*=(const float16_t &Right) { *this = (toFloat(*this) * toFloat(Right)); return *this; }
    constexpr float16_t &operator/=(const float16_t &Right) { *this = (toFloat(*this) / toFloat(Right)); return *this; }

    friend constexpr float16_t operator+(const float16_t &Left, const float16_t &Right) { return toFloat(Left) + toFloat(Right); }
    friend constexpr float16_t operator-(const float16_t &Left, const float16_t &Right) { return toFloat(Left) - toFloat(Right); }
    friend constexpr float16_t operator*(const float16_t &Left, const float16_t &Right) { return toFloat(Left) * toFloat(Right); }
    friend constexpr float16_t operator/(const float16_t &Left, const float16_t &Right) { return toFloat(Left) / toFloat(Right); }
};

namespace std
{
    // Partial specialization.
    template<> struct is_floating_point<float16_t> : std::integral_constant<bool, std::is_same<float16_t, typename std::remove_cv<float16_t>::type>::value> {};
    template<> struct numeric_limits<float16_t>
    {
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
        static constexpr bool is_exact = false;
        static constexpr bool has_infinity = true;
        static constexpr bool has_quiet_NaN = true;
        static constexpr bool has_signaling_NaN = true;
        static constexpr float_denorm_style has_denorm = denorm_present;
        static constexpr bool has_denorm_loss = false;
        static constexpr std::float_round_style round_style = std::round_to_nearest;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = false;
        static constexpr bool is_modulo = false;
        static constexpr int digits = 11;
        static constexpr int digits10 = 3;
        static constexpr int max_digits10 = 5;
        static constexpr int radix = 2;
        static constexpr int min_exponent = -13;
        static constexpr int min_exponent10 = -4;
        static constexpr int max_exponent = 16;
        static constexpr int max_exponent10 = 4;
        static constexpr bool traps = true;
        static constexpr bool tinyness_before = false;

        static constexpr float16_t min() noexcept { return uint16_t(0x0400); }
        static constexpr float16_t max() noexcept { return uint16_t(0x7BFF); }
        static constexpr float16_t lowest() noexcept { return uint16_t(0xFBFF); }
        static constexpr float16_t epsilon() noexcept { return uint16_t(0x1400); }
        static constexpr float16_t round_error() noexcept { return 0.5; }

        static constexpr float16_t infinity() noexcept { return uint16_t(0x7C00); }
        static constexpr float16_t quiet_NaN() noexcept { return uint16_t(0x7E00); }
        static constexpr float16_t signaling_NaN() noexcept { return uint16_t(0x7E00); }
        static constexpr float16_t denorm_min() noexcept { return uint16_t(1); }
    };

    template<> struct numeric_limits<const float16_t> : numeric_limits<float16_t> {};
    template<> struct numeric_limits<volatile float16_t> : numeric_limits<float16_t> {};
    template<> struct numeric_limits<const volatile float16_t> : numeric_limits<float16_t> {};
}
#endif
