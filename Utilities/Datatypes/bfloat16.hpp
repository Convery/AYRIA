/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT

    Integer-accurate until +/-256. e.g. 305.0f gets rounded to 304.0f.
*/

#pragma once
#include <Stdinclude.hpp>

// NOTE(tcn): MSVC does not have planned support for extended-precision floats (P1467R9).
#if __has_include (<stdfloat>)
#include <stdfloat>
#endif

#if defined(__STDCPP_BFLOAT16_T__)
using bfloat16_t = std::bfloat16_t;
#else
struct bfloat16_t
{
    static constexpr float Epsilon = 7.81250e-3;
    uint16_t Value{};

    // Currently prefers rounding to truncating, may change in the future.
    static constexpr uint16_t Truncate(const float Input)
    {
        // Quiet NaN
        if (Input != Input)
            return 0xFFC1;

        // Flush denormals to +0 or -0.
        if ((Input < 0 ? -Input : Input) < std::numeric_limits<float>::min())
            return (Input < 0) ? 0x8000U : 0;

        constexpr auto Offset = std::endian::native == std::endian::little;
        const auto Word = std::bit_cast<std::array<uint16_t, 2>>(Input);
        return Word[Offset];
    }
    static constexpr uint16_t Round(const float Input)
    {
        // Quiet NaN
        if (Input != Input)
            return 0xFFC1;

        // Flush denormals to +0 or -0.
        if (((Input < 0) ? -Input : Input) < std::numeric_limits<float>::min())
            return (Input < 0) ? 0x8000U : 0;

        // Constexpr does not like unions / normal casts.
        return static_cast<uint16_t>(uint32_t(std::bit_cast<uint32_t>(Input) + 0x00007FFFUL + ((std::bit_cast<uint32_t>(Input) >> 16) & 1)) >> 16);
    }

    // Fast conversion to IEEE 754
    static constexpr float toFloat(const uint16_t Input)
    {
        constexpr auto Offset = std::endian::native == std::endian::little;
        std::array<uint16_t, 2> Word{};
        Word[Offset] = Input;

        return std::bit_cast<float>(Word);
    }
    static constexpr float toFloat(const bfloat16_t &Input)
    {
        return toFloat(Input.Value);
    }

    constexpr operator float() const { return toFloat(Value); }
    constexpr operator int32_t() const { return int32_t(toFloat(Value)); }

    constexpr bfloat16_t() = default;
    constexpr bfloat16_t(const float Input) : Value(Round(Input)) {};
    explicit constexpr bfloat16_t(const uint16_t Input) : Value(Input) {}
    template <std::integral T> constexpr bfloat16_t(const T Input) : bfloat16_t(float(Input)) {}

    constexpr bool operator<(const bfloat16_t &Right)  const { return toFloat(Value) < toFloat(Right); }
    constexpr bool operator>(const bfloat16_t &Right)  const { return toFloat(Value) > toFloat(Right); }
    constexpr bool operator<=(const bfloat16_t &Right) const { return toFloat(Value) <= toFloat(Right); }
    constexpr bool operator>=(const bfloat16_t &Right) const { return toFloat(Value) >= toFloat(Right); }
    constexpr bool operator!=(const bfloat16_t &Right) const { return !operator==(Right); }
    constexpr bool operator==(const bfloat16_t &Right) const
    {
        if (Value == Right.Value) return true;

        const auto Temp = toFloat(Value) - toFloat(Right);
        const auto ABS = (Temp >= 0) ? Temp : -Temp;
        return ABS < Epsilon;
    }

    constexpr bfloat16_t &operator+=(const bfloat16_t &Right) { *this = (toFloat(*this) + toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator-=(const bfloat16_t &Right) { *this = (toFloat(*this) - toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator*=(const bfloat16_t &Right) { *this = (toFloat(*this) * toFloat(Right)); return *this; }
    constexpr bfloat16_t &operator/=(const bfloat16_t &Right) { *this = (toFloat(*this) / toFloat(Right)); return *this; }

    friend constexpr bfloat16_t operator+(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) + toFloat(Right); }
    friend constexpr bfloat16_t operator-(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) - toFloat(Right); }
    friend constexpr bfloat16_t operator*(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) * toFloat(Right); }
    friend constexpr bfloat16_t operator/(const bfloat16_t &Left, const bfloat16_t &Right) { return toFloat(Left) / toFloat(Right); }
};

namespace std
{
    // Partial specialization.
    template<> struct is_floating_point<bfloat16_t> : std::integral_constant<bool, std::is_same_v<bfloat16_t, typename std::remove_cv_t<bfloat16_t>>> {};
    template<> struct numeric_limits<bfloat16_t>
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
        static constexpr int digits = 8;
        static constexpr int digits10 = 2;
        static constexpr int max_digits10 = 9;
        static constexpr int radix = 2;
        static constexpr int min_exponent = -125;
        static constexpr int min_exponent10 = -37;
        static constexpr int max_exponent = 128;
        static constexpr int max_exponent10 = 38;
        static constexpr bool traps = true;
        static constexpr bool tinyness_before = false;

        static constexpr bfloat16_t min() noexcept { return uint16_t(0x0080); }
        static constexpr bfloat16_t max() noexcept { return uint16_t(0x7F7F); }
        static constexpr bfloat16_t lowest() noexcept { return uint16_t(0xFF7F); }
        static constexpr bfloat16_t epsilon() noexcept { return uint16_t(0x3C00); }
        static constexpr bfloat16_t round_error() noexcept { return 0.5; }

        static constexpr bfloat16_t infinity() noexcept { return uint16_t(0x7F80); }
        static constexpr bfloat16_t quiet_NaN() noexcept { return uint16_t(0xFFC1); }
        static constexpr bfloat16_t signaling_NaN() noexcept { return uint16_t(0xFF81); }
        static constexpr bfloat16_t denorm_min() noexcept { return uint16_t(1); }
    };

    template<> struct numeric_limits<const bfloat16_t> : numeric_limits<bfloat16_t> {};
    template<> struct numeric_limits<volatile bfloat16_t> : numeric_limits<bfloat16_t> {};
    template<> struct numeric_limits<const volatile bfloat16_t> : numeric_limits<bfloat16_t> {};
}
#endif
