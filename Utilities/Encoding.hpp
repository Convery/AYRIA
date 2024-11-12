/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <span>
#include <string_view>

namespace Encoding
{
    // Helper for early exits.
    template <typename T, size_t N> constexpr bool isASCII(std::span<T, N> Input)
    {
        // Covers char and char8_t; at runtime.
        if (sizeof(T) == 1 && !std::is_constant_evaluated())
        {
            const auto Intptr = (const uint32_t *)Input.data();
            const auto Remaining = Input.size() & 3;
            const auto Count32 = Input.size() / 4;

            for (size_t i = 0; i < Count32; ++i)
                if (Intptr[i] & 0x80808080)
                    return false;

            for (size_t i = 0; i < Remaining; ++i)
                if (Input[(Count32 * 4) + i] & 0x80)
                    return false;
        }

        // Covers wchar_t and char16_t; and others at compiletime.
        else
        {
            for (const auto &Item : Input)
                if (Item >= 0x80)
                    return false;
        }

        return true;
    }
    template <typename T> constexpr bool isASCII(const std::basic_string<T> &Input)
    {
        return isASCII(std::span(Input));
    }
}

#include "Encoding/UTF8.hpp"
#include "Encoding/JSON.hpp"
