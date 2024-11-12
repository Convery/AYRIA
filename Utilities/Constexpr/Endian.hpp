/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <bit>
#include <concepts>

namespace cmp
{
    // Convert to a different endian (half are NOP on each system).
    template <std::integral T> constexpr T toLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <std::integral T> constexpr T toBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <std::integral T> constexpr T fromLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(Value);
        else
            return Value;
    }
    template <std::integral T> constexpr T fromBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(Value);
        else
            return Value;
    }

    // Helper to handle floats as raw bytes.
    template <std::floating_point T> constexpr auto toInt(T Value)
    {
        if constexpr (sizeof(T) == 8) return std::bit_cast<uint64_t>(Value);
        if constexpr (sizeof(T) == 4) return std::bit_cast<uint32_t>(Value);
        if constexpr (sizeof(T) == 2) return std::bit_cast<uint16_t>(Value);

        static_assert(sizeof(T) < 2 || sizeof(T) > 8, "Invalid float size");
        std::unreachable();
    }

    template <std::floating_point T> constexpr T toLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::bit_cast<T>(std::byteswap(toInt(Value)));
        else
            return Value;
    }
    template <std::floating_point T> constexpr T toBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::bit_cast<T>(std::byteswap(toInt(Value)));
        else
            return Value;
    }
    template <std::floating_point T> constexpr T fromLittle(T Value)
    {
        if constexpr (std::endian::native == std::endian::big)
            return std::bit_cast<T>(std::byteswap(toInt(Value)));
        else
            return Value;
    }
    template <std::floating_point T> constexpr T fromBig(T Value)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::bit_cast<T>(std::byteswap(toInt(Value)));
        else
            return Value;
    }
}
