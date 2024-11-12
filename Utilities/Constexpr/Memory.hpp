/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <bit>
#include <array>
#include <Datatypes.hpp>
#include "Metaprogramming.hpp"

namespace cmp
{
    // Since we can't cast (char *) <-> (void *) in constexpr.
    template <Byte_t T, Byte_t U> constexpr bool memcmp(const T *A, const U *B, size_t Size)
    {
        if (!std::is_constant_evaluated())
        {
            return 0 == std::memcmp(A, B, Size);
        }
        else
        {
            while (Size--)
            {
                if ((U(*A++) ^ T(*B++)) != T{}) [[unlikely]]
                    return false;
            }

            return true;
        }
    }

    // Memcpy accepts the size in bytes in-case we want partial copies.
    template <typename T, typename U> constexpr void memcpy(T *Dst, const U *Src, size_t Sizebytes)
    {
        if (!std::is_constant_evaluated())
        {
            std::memcpy(Dst, Src, Sizebytes);
        }
        else
        {
            if constexpr (sizeof(T) == sizeof(U))
            {
                while (Sizebytes)
                {
                    *Dst++ = std::bit_cast<T>(*Src++);
                    Sizebytes -= sizeof(T);
                }
            }
            else if constexpr (sizeof(T) == 1)
            {
                const auto Temp = std::bit_cast<std::array<uint8_t, sizeof(U)>>(*Src);
                const auto Step = std::min(sizeof(U), Sizebytes);

                for (size_t i = 0; i < Step; ++i)
                    *Dst++ = Temp[i];

                if (Sizebytes -= Step) return memcpy(Dst, ++Src, Sizebytes);
            }
            else if constexpr (sizeof(U) == 1)
            {
                const auto Step = std::min(sizeof(T), Sizebytes);
                std::array<uint8_t, sizeof(T)> Temp{};

                for (size_t i = 0; i < Step; ++i)
                    Temp[i] = uint8_t(*Src++);

                *Dst++ = std::bit_cast<T>(Temp);
                if (Sizebytes -= Step) return memcpy(Dst, Src, Sizebytes);
            }
            else
            {
                static_assert(always_false<T>, "memcpy needs the types to be the same size, or one being 1 byte aligned");
            }
        }
    }
    template <typename T, typename U> constexpr void memcpy(T *Dst, const U &Src)
    {
        if constexpr (sizeof(T) == sizeof(U))
        {
            *Dst = std::bit_cast<T>(Src);
        }
        else if constexpr (sizeof(T) == 1 || sizeof(U) == 1)
        {
            return memcpy(Dst, &Src, sizeof(U) >= sizeof(T) ? sizeof(U) : sizeof(T));
        }
        else
        {
            const auto A = std::bit_cast<std::array<uint8_t, sizeof(U)>>(Src);
            auto B = std::bit_cast<std::array<uint8_t, sizeof(T)>>(*Dst);

            const auto Min = sizeof(U) <= sizeof(T) ? sizeof(U) : sizeof(T);
            for (size_t i = 0; i < Min; ++i) B[i] = A[i];

            *Dst = std::bit_cast<T>(B);
        }


    }

    // Get the underlying bytes of an object / range in constexpr mode.
    template <typename T> requires(!Range_t<T>) constexpr auto getBytes(const T &Value)
    {
        return std::bit_cast<std::array<uint8_t, sizeof(T)>>(Value);
    }
    template <Byte_t T, size_t N> constexpr auto getBytes(const std::array<T, N> &Value)
    {
        return[]<size_t ...Index>(const std::array<T, N> &Input, std::index_sequence<Index...>)
        {
            return std::array<uint8_t, N>{ { uint8_t(Input[Index])... }};
        }(Value, std::make_index_sequence<N>());
    }
    template <typename T, size_t N> constexpr auto getBytes(const std::array<T, N> &Value)
    {
        return std::bit_cast<std::array<uint8_t, sizeof(T) * N>>(Value);
    }
    template <size_t N = std::dynamic_extent> constexpr auto getBytes(std::span<const uint8_t, N> Input) { return Input; }
    template <Range_t T> requires(std::extent<T>() == 0 && std::is_constant_evaluated()) constexpr auto getBytes(const T &Range)
    {
        Blob_t Buffer{}; Buffer.reserve(Range.size() * sizeof(typename T::value_type));

        for (const auto &Item : Range)
        {
            const auto Local = std::bit_cast<std::array<uint8_t, sizeof(typename T::value_type)>>(Item);
            Buffer.append(Local.data(), sizeof(typename T::value_type));
        }

        return Buffer;
    }
    template <Range_t T> requires(std::extent<T>() == 0 && !std::is_constant_evaluated()) constexpr auto getBytes(const T &Range)
    {
        return { reinterpret_cast<const uint8_t *>(Range.data()), std::extent<T>() * sizeof(typename T::value_type) };
    }
    template <Range_t T> requires(std::is_same_v<std::decay<typename T::value_type>, uint8_t>) constexpr auto getBytes(const T &Range)
    {
        return Range;
    }
    template <Range_t T> requires(std::extent<T>() != 0 && sizeof(typename T::value_type) != 1) constexpr auto getBytes(const T &Range)
    {
        std::array<uint8_t, std::extent<T>() * sizeof(typename T::value_type)> Buffer{};
        cmp::memcpy(Buffer.data(), Range.data(), std::extent<T>() * sizeof(typename T::value_type));

        return Buffer;
    }
}
