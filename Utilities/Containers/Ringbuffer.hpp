/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-20
    License: MIT

    Fixed capacity container that overwrites the last element as needed.
*/

#pragma once
#include <array>
#include <ranges>
#include <cstdint>

#include "Python.hpp"
#include "../Constexpr.hpp"

template <typename T, size_t N> requires (N > 0)
struct Ringbuffer_t
{
    using size_type = cmp::Smallint_t<N>;

    size_type Head{}, Size{};
    std::array<T, N> Storage{};

    // For clearer code..
    static constexpr size_type Clamp(size_type Value) noexcept
    {
        return Value % N;
    }
    static constexpr size_type Advance(size_type Index, int Offset) noexcept
    {
        return Clamp(size_type(Index + Offset + N));
    }

    // Need a custom iterator to wrap around.
    struct Iterator_t
    {
        using difference_type = std::ptrdiff_t;
        using reference = const T &;
        using pointer = const T *;
        using value_type = T;

        size_type Index, Count;
        const T *Data;

        const T &operator*() const noexcept { return Data[Index]; }
        const T *operator->() const noexcept { return Data + Index; }

        bool operator==(const std::default_sentinel_t) const noexcept { return Count == 0; }
        bool operator==(const Iterator_t &Right) const noexcept { return Count == Right.Count; }

        Iterator_t &operator++() noexcept { Index = Advance(Index, -1); --Count; return *this; }
        Iterator_t &operator--() noexcept { Index = Advance(Index, 1); ++Count; return *this; }

        Iterator_t operator++(int) noexcept { auto Copy{ *this }; operator++(); return Copy; }
        Iterator_t operator--(int) noexcept { auto Copy{ *this }; operator--(); return Copy; }
    };

    // Not sure if useful for user-code.
    bool empty() const noexcept
    {
        return Size == 0;
    }
    bool full() const noexcept
    {
        return Size == N;
    }
    size_type size() const noexcept
    {
        return Size;
    }

    // Simple access to common elements.
    [[nodiscard]] const T &back() const noexcept
    {
        return Storage[Head];
    }
    [[nodiscard]] const T &front() const noexcept
    {
        // On Size == 0, this returns the same value as [0].
        return Storage[Advance(Head, -1)];
    }

    // STL compatibility.
    [[nodiscard]] static auto end() noexcept { return std::default_sentinel_t{}; }
    [[nodiscard]] auto begin() const noexcept { return std::counted_iterator{ Iterator_t{ Advance(Head, -1), Size, Storage.data() }, Size }; }

    // STL-like modifiers.
    void push_back(const T &Value) noexcept
    {
        Storage[Head] = Value;
        if (Size != N) ++Size;

        Head = Advance(Head, 1);
    }
    template <typename... Args> T &emplace_back(Args&&... args) noexcept
    {
        if (Size != N) ++Size;

        Storage[Head] = { std::forward<Args>(args)... };
        auto &Ret = Storage[Head];

        Head = Advance(Head, 1);
        return Ret;
    }
};

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    inline void Ringbuffertest()
    {
        // 3 element capacity.
        Ringbuffer_t<int, 3> Buffer;
        std::array<int, 6> Rangetest{};

        // Insert 4 elements, evicts the oldest.
        Buffer.emplace_back(1);
        Buffer.emplace_back(2);
        Buffer.push_back(3);
        Buffer.push_back(4);

        if (4 != Buffer.front() || 2 != Buffer.back() || 3 != Buffer.size())
            std::printf("BROKEN: Ringbuffer core\n");

        // Ensure that ranges are supported by copying the 3 elements.
        std::ranges::copy(Buffer | std::views::reverse, Rangetest.begin());
        for (const auto &[Index, Value] : Enumerate(Buffer, 3)) Rangetest[Index] = Value;

        if (Rangetest != std::array{ 2, 3, 4, 4, 3, 2 })
            std::printf("BROKEN: Ringbuffer ranges\n");
    }
}
#endif
