/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT

    Compiletime mathematics.
*/

#pragma once
#include <numeric>
#include <cstdint>
#include <cmath>

namespace cmp
{
    // Opt for branchless versions.
    template <typename T> constexpr T abs(const T &Value)
    {
        if constexpr (std::signed_integral<T>)
        {
            const auto Mask = Value >> (sizeof(T) * 8 - 1);
            return (Value + Mask) ^ Mask;
        }
        else
        {
            return Value * ((Value > 0) - (Value < 0));
        }
    }
    template <typename T> constexpr T min(const T &A, const T &B)
    {
        if constexpr (std::signed_integral<T>)
        {
            return A + ((B - A) & ((B - A) >> (sizeof(T) * 8 - 1)));
        }
        else
        {
            return (A < B) * A + (B <= A) * B;
        }
    }
    template <typename T> constexpr T max(const T &A, const T &B)
    {
        if constexpr (std::signed_integral<T>)
        {
            return A - ((A - B) & ((A - B) >> (sizeof(T) * 8 - 1)));
        }
        else
        {
            return (A > B) * A + (B >= A) * B;
        }
    }
    template <typename T> constexpr T min(std::initializer_list<T> &&Items)
    {
        T Min = 0;
        for (const auto &Item : Items)
            Min = min(Min, Item);
        return Min;
    }
    template <typename T> constexpr T max(std::initializer_list<T> &&Items)
    {
        T Max = 0;
        for (const auto &Item : Items)
            Max = max(Max, Item);
        return Max;
    }
    template <typename T> constexpr T clamp(const T &Value, const T &Min, const T &Max)
    {
        return cmp::max(Min, cmp::min(Value, Max));
    }

    // Plumbing, recursion makes MSVC upset..
    template <typename T, std::integral U> constexpr T pow_int(T Base, U Exponent)
    {
        if (Exponent == 0) return 1;
        if (Exponent < 0) return pow_int(1 / Base, -Exponent);

        long double Result{ 1.0 };
        while (Exponent)
        {
            if (Exponent & 1) Result *= Base;
            Exponent >>= 1;
            Base *= Base;
        }

        return T(Result);
    }

    // As we are only doing this in constexpr, we can use a series.
    constexpr size_t Taylorsteps = 512;
    template <std::floating_point T = double> constexpr T log(const T &Value)
    {
        // Prefer optimized implementations.
        if (!std::is_constant_evaluated())
        {
            return std::log(Value);
        }

        // Undefined for negative values.
        if (Value < T{ 0.0 }) return std::numeric_limits<T>::quiet_NaN();

        // Taylor series.
        long double Sum{}, Term{ (Value - 1) / (Value + 1) };
        const long double Squared{ Term * Term };
        for (size_t i = 0; i < Taylorsteps; ++i)
        {
            Sum += Term / (2 * i + 1);
            Term *= Squared;
        }

        return T(2 * Sum);
    }
    template <std::floating_point T = double> constexpr T exp(const T &Value)
    {
        // Prefer optimized implementations.
        if (!std::is_constant_evaluated())
        {
            return std::exp(Value);
        }

        // Check if an integer got promoted.
        if (Value == T(int64_t(Value)))
        {
            constexpr auto Euler = 2.7182818284590452353602874713527L;
            return pow_int(Euler, int64_t(Value));
        }

        // Taylor series.
        long double Sum{ 1.0 }, Term{ 1.0 };
        for (size_t i = 1; i < Taylorsteps; ++i)
        {
            Term *= Value / i;
            Sum += Term;
        }
        return T(Sum);
    }
    template <std::floating_point T = double> constexpr T pow(const T &Base, const double &Exponent)
    {
        // Prefer optimized implementations.
        if (!std::is_constant_evaluated())
        {
            return std::pow(Base, Exponent);
        }

        // Check if an integer got promoted.
        if (Exponent == T(int64_t(Exponent)))
        {
            return pow_int(Base, int64_t(Exponent));
        }
        else
        {
            return exp(Exponent * log(Base));
        }
    }
}

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    constexpr bool Mathtest = []()
    {
        constexpr auto fPI = std::numbers::pi_v<double>;
        constexpr auto fE = std::numbers::e_v<double>;
        constexpr auto i32 = 256;

        // Compare our implementation to std::*
        constexpr double Constexpr[12] =
        {
            cmp::log(fPI), cmp::log(fE), cmp::log((double)i32),
            cmp::exp(fPI), cmp::exp(fE), cmp::exp((double)i32),
            cmp::pow(fPI, 2), cmp::pow(fE, 2), cmp::pow((double)i32, 2),
            cmp::pow(fPI, 2.2), cmp::pow(fE, 2.2), cmp::pow((double)i32, 2.2)
        };
        constexpr double MSSTL[12] =
        {
            1.144730, 1.000000, 5.545177,
            23.140693, 15.154262, 1511427665004103527714100498092829891603482697174374415092350456743517150826614334359230562343706299625849749504.0,
            9.869604, 7.389056, 65536.0,
            12.408798, 9.025013, 198668.001806
        };

        // Max error in %
        constexpr double Threshold = 0.01;
        for (size_t i = 0; i < 12; ++i)
        {
            const auto Ratio = cmp::max(Constexpr[i], MSSTL[i]) / cmp::min(Constexpr[i], MSSTL[i]);
            const auto Percent = cmp::abs(1.0 - Ratio) * 100.0;

            if (Percent > Threshold)
                return false;
        }

        return true;
    }();
    static_assert(Mathtest, "BROKEN: Constexpr math");
}
#endif
