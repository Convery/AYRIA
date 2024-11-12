/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <concepts>

namespace cmp
{
    // Concepts.
    template <typename T> concept Byte_t = sizeof(T) == 1;
    template <typename T> concept Range_t = requires (T t) { t.begin(); t.end(); typename T::value_type; };
    template <typename T> concept Sequential_t = requires (T t) { t.data(); t.size(); typename T::value_type; };
    template <typename T> concept Char_t = std::is_same_v<std::remove_const_t<T>, char> || std::is_same_v<std::remove_const_t<T>, char8_t>;
    template <typename T> concept WChar_t = std::is_same_v<std::remove_const_t<T>, wchar_t> || std::is_same_v<std::remove_const_t<T>, char16_t>;

    // Fully typed instantiation.
    template <typename Type, template <typename ...> class Template> constexpr bool isDerived = false;
    template <template <typename ...> class Template, typename... Types> constexpr bool isDerived<Template<Types...>, Template> = true;

    // Partially typed instantiation.
    template <typename Type, template <typename, auto> class Template> constexpr bool isDerivedEx = false;
    template <template <typename, auto> class Template, typename T, auto A> constexpr bool isDerivedEx<Template<T, A>, Template> = true;

    // Until we have P2593.
    template <typename ...> constexpr bool always_true = true;
    template <typename ...> constexpr bool always_false = false;

    // Helper to avoid nested conditionals in templates.
    template <bool Conditional, typename T> struct Case_t : std::bool_constant<Conditional> { using type = T; };
    using Defaultcase_t = Case_t<false, void>;

    // Get the smallest type that can hold our value.
    template <uint64_t Maxvalue> using Smallint_t = typename std::disjunction<
        Case_t<(std::bit_width(Maxvalue) > 32), uint64_t>,
        Case_t<(std::bit_width(Maxvalue) > 16), uint32_t>,
        Case_t<(std::bit_width(Maxvalue) >  8), uint16_t>,
        Case_t<(std::bit_width(Maxvalue) <= 8), uint8_t> >::type;

    // Struct to create an overload set for use with std::visit and similar functions.
    template<class... Types> struct Overload : Types... { using Types::operator()...; };

    // For serialization, call a visitor with each property of a struct.
    decltype(auto) Visitmembers(const auto &Object, const auto &Visitor)
    {
        using Type = std::remove_cvref_t<decltype(Object)>;

             if constexpr (requires { [](Type &This) { const auto &[a1] = This; }; }) { const auto &[a1] = Object; return Visitor(a1); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2] = This; }; }) { const auto &[a1, a2] = Object; return Visitor(a1, a2); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3] = This; }; }) { const auto &[a1, a2, a3] = Object; return Visitor(a1, a2, a3); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4] = This; }; }) { const auto &[a1, a2, a3, a4] = Object; return Visitor(a1, a2, a3, a4); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5] = This; }; }) { const auto &[a1, a2, a3, a4, a5] = Object; return Visitor(a1, a2, a3, a4, a5); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5, a6] = This; }; }) { const auto &[a1, a2, a3, a4, a5, a6] = Object; return Visitor(a1, a2, a3, a4, a5, a6); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5, a6, a7] = This; }; }) { const auto &[a1, a2, a3, a4, a5, a6, a7] = Object; return Visitor(a1, a2, a3, a4, a5, a6, a7); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8] = This; }; }) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8] = Object; return Visitor(a1, a2, a3, a4, a5, a6, a7, a8); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8, a9] = This; }; }) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8, a9] = Object; return Visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9); }
        else if constexpr (requires { [](Type &This) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = This; }; }) { const auto &[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = Object; return Visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }

        else
        {
            static_assert(always_false<Type>, "We need a bigger boat..");
        }
    }
}
