/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT

    Python-esque utilities.
*/

#pragma once
#include <ranges>

// for (const auto &x : Slice({1, 2, 3, 4, 5}, 1, 3)) = { 2, 3, 4 }
template <typename T, typename Container> constexpr auto Slice(const Container &Args, int Begin, int End)
{
    const auto Range = std::span(Args);
    const auto First = std::next(Range.begin(), Begin);
    const auto Last = End ? std::next(Range.begin(), End) : std::prev(Range.end(), -End);

    return std::ranges::subrange(First, Last);
}

// for (const auto &x : Range(1, 100, 2)) = { 1, 3, 5 ... }
template <typename T, typename Steptype = int> constexpr auto Range(T Start, T Stop, Steptype Stepsize = 1)
{
    return std::views::iota(Start, Stop) | std::views::stride(Stepsize);
}

// for (const auto &[Index, Value] : Enumerate({ 1, 2, 3, 4 })) = { {0, 1}, {1, 2}, ... }
template <typename T, typename Container> constexpr auto Enumerate(const Container &Args, size_t Start = 0)
{
    const auto Indicies = std::views::iota(Start, Start + Args.size());
    return std::views::zip(Indicies, Args);
}

// MSVC deduction needs help..
template <typename Range> requires std::ranges::range<Range> constexpr auto Enumerate(const Range &Args, size_t Start = 0)
{
    const auto Indicies = std::views::iota(Start, Start + std::ranges::distance(Args));
    return std::views::zip(Indicies, Args);
}

// Need to extend the lifetime of the list.
template <typename T> constexpr auto Enumerate(const std::initializer_list<T> &Args, size_t Start = 0)
{
    std::vector<std::pair<size_t, T>> Vector;
    Vector.reserve(Args.size());

    for (const auto &[Index, Item] : Enumerate<T, std::initializer_list<T>>(Args, Start))
        Vector.emplace_back(Index, Item);

    return Vector;
}
template <typename T> constexpr auto Slice(const std::initializer_list<T> &Args, int Begin, int End)
{
    std::vector<T> Vector;
    Vector.reserve(Args.size());

    for (const auto &Item : Slice<T, std::initializer_list<T>>(Args, Begin, End))
        Vector.emplace_back(Item);

    return Vector;
}

namespace Unittests
{
    constexpr bool Enumtest = []()
    {
        for (const auto &[Index, Item] : Enumerate({ 1U, 2U, 3U }, 1))
            if (Index != Item)
                return false;

        for (const auto &[Index, Item] : Enumerate({ 1U, 2U, 3U }))
            if (Index != (Item - 1))
                return false;

        return true;
    }();
    static_assert(Enumtest, "BROKEN: Enum utility");

    constexpr bool Rangetest = []()
    {
        size_t Counter = 0;  // = { 0, 2, 4 }
        for (const auto Int : Range(0, 6, 2))
            Counter += Int;

        return Counter == 6;
    }();
    static_assert(Rangetest, "BROKEN: Range utility");

    constexpr bool Slicetest = []()
    {
        const auto Subrange = Slice({ 1, 2, 3, 4, 5 }, 1, 4);
        return Subrange[0] == 2 && Subrange[1] == 3 && Subrange[2] == 4;
    }();
    static_assert(Slicetest, "BROKEN: Slice utility");
}
