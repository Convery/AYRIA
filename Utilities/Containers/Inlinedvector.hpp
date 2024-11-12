/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-20
    License: MIT

    Very simple vector for when we have an expected amount of elements.
*/

#pragma once
#include <array>
#include <memory>
#include <ranges>
#include <cstdint>
#include "../Constexpr.hpp"

template <typename T, uint32_t Fixedsize> requires (Fixedsize > 0)
struct Inlinedvector
{
    uint32_t Capacity = Fixedsize, Size{};
    std::array<T, Fixedsize> Static{};
    T *Dynamic{};

    // The operator[] already does selection for us.
    struct Iterator_t
    {
        using difference_type = std::ptrdiff_t;
        using reference = const T &;
        using pointer = const T *;
        using value_type = T;

        uint32_t Index, Count;
        const Inlinedvector &Parent;

        const T &operator*() const noexcept { return Parent[Index]; }
        const T *operator->() const noexcept { return &Parent[Index]; }

        bool operator==(const std::default_sentinel_t) const noexcept { return Count == 0; }
        bool operator==(const Iterator_t &Right) const noexcept { return Count == Right.Count; }

        Iterator_t &operator++() noexcept { ++Index; --Count; return *this; }
        Iterator_t &operator--() noexcept { --Index; ++Count; return *this; }

        Iterator_t operator++(int) noexcept { auto Copy{ *this }; operator++(); return Copy; }
        Iterator_t operator--(int) noexcept { auto Copy{ *this }; operator--(); return Copy; }
    };

    // General information.
    [[nodiscard]] bool empty() const noexcept
    {
        return Size == 0;
    }
    [[nodiscard]] uint32_t size() const noexcept
    {
        return Size;
    }

    // New memory is not initialized.
    void reserve(uint32_t Newsize) noexcept
    {
        if (Newsize > Capacity)
        {
            // If realloc fails, we have bigger problems than a segfault..
            Dynamic = (T *)std::realloc(Dynamic, Newsize - Fixedsize);
            Capacity = Newsize;
        }
    }
    void resize(uint32_t Newsize) noexcept
    {
        if (Newsize <= Fixedsize && Dynamic)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                for (uint32_t i = 0; i < (Size - Fixedsize); ++i)
                    Dynamic[i].~T();

            std::free(std::exchange(Dynamic, nullptr));
        }
        if (Newsize < Fixedsize) std::fill(Static.begin() + Newsize, Static.end(), T{});
        if (Newsize > Capacity) reserve(Newsize);

        Size = Newsize;
        Capacity = cmp::max(Newsize, Fixedsize);
    }

    // STL-like modifiers.
    template <typename... Args> T &emplace_back(Args&&... args) noexcept
    {
        if (Size < Fixedsize)
        {
            return Static[Size++] = { args... };
        }
        if (Size < Capacity)
        {
            return Dynamic[Size++ - Fixedsize] = { args... };
        }

        reserve(Capacity + 1);
        return Dynamic[Size++ - Fixedsize] = { args... };
    }
    void push_back(const T &Value) noexcept
    {
        emplace_back(Value);
    }
    void pop_back() noexcept
    {
        resize(Size - 1);
    }

    // Simple access to elements.
    [[nodiscard]] const T &back() const noexcept
    {
        return operator[](Size - 1);
    }
    [[nodiscard]] const T &front() const noexcept
    {
        return operator[](0);
    }
    [[nodiscard]] T &operator[](uint32_t Index) const noexcept
    {
        assert(Size > Index);
        if (Index < Fixedsize) return Static[Index];
        else return Dynamic[1 + Index - Fixedsize];
    }

    // STL compatibility.
    [[nodiscard]] static auto end() noexcept { return std::default_sentinel_t{}; }
    [[nodiscard]] auto begin() const noexcept { return std::counted_iterator{ Iterator_t{ 0, Size, *this }, Size }; }

    // Provide some common vector members incase someone wants them.
    [[nodiscard]] auto insert(const std::counted_iterator<Iterator_t> &Position, const T &Value) noexcept
    {
        const auto Index = Position.base().Index;
        operator[](Index) = Value;
        return Position;
    }
    [[nodiscard]] auto insert(std::counted_iterator<Iterator_t> &Position, const T &Value) noexcept
    {
        const auto Index = Position.base().Index;
        operator[](Index) = Value;
        return Position;
    }
    void assign(uint32_t Newsize, const T &Value) noexcept
    {
        resize(Newsize);
        std::fill_n(Static.data(), cmp::min(Newsize, Fixedsize), Value);
        if (Newsize > Fixedsize) std::fill_n(Dynamic, Newsize - Fixedsize, Value);
    }
    void clear() noexcept
    {
        resize(0);
    }

    // Simplify constructing from any valid range (T.begin(), T.end()).
    template <cmp::Range_t U> requires (std::is_same_v<typename U::value_type, T>) Inlinedvector(const U &Range) noexcept
    {
        const auto PartA = cmp::min(Range.size(), Fixedsize);
        const auto PartB = Range.size() - PartA;

        resize(Range.size());
        std::ranges::copy_n(Range, PartA, Static.data());
        std::ranges::copy_n(Range.begin() + PartA, PartB, Dynamic);
    }
    Inlinedvector(uint32_t Newsize) noexcept
    {
        const auto PartA = cmp::min(Newsize, Fixedsize);
        const auto PartB = Newsize - PartA;

        resize(Newsize);
        std::fill_n(Static, PartA, T{});
        std::fill_n(Dynamic, PartB, T{});
    }
    ~Inlinedvector() noexcept { clear(); }
    Inlinedvector() = default;
};
