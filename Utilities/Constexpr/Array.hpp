/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <array>

namespace cmp
{
    // Concatenate arrays.
    template <typename T, size_t N, size_t M>
    constexpr auto combineArrays(const std::array<T, N> &Left, const std::array<T, M> &Right)
    {
        return[]<size_t... LIndex, size_t... RIndex>(const std::array<T, N> &Left, const std::array<T, M> &Right, std::index_sequence<LIndex...>, std::index_sequence<RIndex...>)
        {
            return std::array<T, N + M>{{Left[LIndex]..., Right[RIndex]...}};
        }(Left, Right, std::make_index_sequence<N>(), std::make_index_sequence<M>());
    }

    // Split at index.
    template <size_t SplitIndex, typename T, size_t N>
    constexpr auto splitArray(const std::array<T, N> &Array)
    {
        return[]<size_t... LIndex, size_t... RIndex>(const std::array<T, N> &Array, std::index_sequence<LIndex...>, std::index_sequence<RIndex...>)
        {
            return std::make_pair(std::array<T, sizeof...(LIndex)>{Array[LIndex]...}, std::array<T, sizeof...(RIndex)>{Array[sizeof...(LIndex) + RIndex]...});
        }(Array, std::make_index_sequence<SplitIndex>{}, std::make_index_sequence<N - SplitIndex>{});
    }

    // Expand with default-initialized elements, or truncate and shrink.
    template <size_t Newsize, typename T, size_t Oldsize>
    constexpr std::array<T, Newsize> resizeArray(const std::array<T, Oldsize> &Input)
    {
        constexpr auto Min = std::min(Oldsize, Newsize);

        return[]<size_t ...Index>(const std::array<T, Oldsize> &Input, std::index_sequence<Index...>)
        {
            return std::array<T, Newsize>{ { Input[Index]... }};
        }(Input, std::make_index_sequence<Min>());
    }

    // Helper for dealing with string-literals.
    template <typename T, size_t N> constexpr auto stripNullchar(const T(&Input)[N])
    {
        return resizeArray<N - 1>(std::to_array(Input));
    }
}
