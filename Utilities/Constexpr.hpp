/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include "Constexpr/Array.hpp"
#include "Constexpr/Endian.hpp"
#include "Constexpr/Math.hpp"
#include "Constexpr/Memory.hpp"
#include "Constexpr/Metaprogramming.hpp"

// Lookups do not work properly in MSVC, copy this into each module that needs it.
template <typename T, size_t N, size_t M> constexpr auto operator+(const std::array<T, N> &Left, const std::array<T, M> &Right)
{
    return[]<size_t... LIndex, size_t... RIndex>(const std::array<T, N> &Left, const std::array<T, M> &Right, std::index_sequence<LIndex...>, std::index_sequence<RIndex...>)
    {
        return std::array<T, N + M>{{Left[LIndex]..., Right[RIndex]...}};
    }(Left, Right, std::make_index_sequence<N>(), std::make_index_sequence<M>());
}
