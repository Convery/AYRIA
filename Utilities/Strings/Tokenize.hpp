/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <vector>
#include <string_view>

namespace String
{
    // Standard commandline parsing.
    constexpr std::vector<std::string_view> Tokenize(std::string_view Input)
    {
        std::vector<std::string_view> Tokens{};
        bool Quoted{};

        while (!Input.empty())
        {
            const auto P1 = Input.find('\"');
            const auto P2 = Input.find(' ');

            if (Quoted)
            {
                // Malformed quote-sequence, stop parsing.
                if (P1 == std::string_view::npos) [[unlikely]]
                    return Tokens;

                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = false;
            }
            else
            {
                if (P2 < P1)
                {
                    if (P2) Tokens.emplace_back(Input.substr(0, P2));
                    Input.remove_prefix(P2 + 1);
                }
                else if (P1 != std::string_view::npos)
                {
                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = true;
                }
                else
                {
                    break;
                }
            }
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }
    constexpr std::vector<std::wstring_view> Tokenize(std::wstring_view Input)
    {
        std::vector<std::wstring_view> Tokens{};
        bool Quoted{};

        while (!Input.empty())
        {
            const auto P1 = Input.find(L'\"');
            const auto P2 = Input.find(L' ');

            if (Quoted)
            {
                // Malformed quote-sequence, stop parsing.
                if (P1 == std::wstring_view::npos) [[unlikely]]
                    return Tokens;

                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = false;
            }
            else
            {
                if (P2 < P1)
                {
                    if (P2) Tokens.emplace_back(Input.substr(0, P2));
                    Input.remove_prefix(P2 + 1);
                }
                else if (P1 != std::wstring_view::npos)
                {
                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = true;
                }
                else
                {
                    break;
                }
            }
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }
    constexpr std::vector<std::u8string_view> Tokenize(std::u8string_view Input)
    {
        std::vector<std::u8string_view> Tokens{};
        bool Quoted{};

        while (!Input.empty())
        {
            const auto P1 = Input.find('\"');
            const auto P2 = Input.find(' ');

            if (Quoted)
            {
                // Malformed quote-sequence, stop parsing.
                if (P1 == std::string_view::npos) [[unlikely]]
                    return Tokens;

                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = false;
            }
            else
            {
                if (P2 < P1)
                {
                    if (P2) Tokens.emplace_back(Input.substr(0, P2));
                    Input.remove_prefix(P2 + 1);
                }
                else if (P1 != std::string_view::npos)
                {
                    if (P1) Tokens.emplace_back(Input.substr(0, P1));
                    Input.remove_prefix(P1 + 1);
                    Quoted = true;
                }
                else
                {
                    break;
                }
            }
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }

    // Tokens of length 0 are dropped unless PreserveNULL = true.
    constexpr std::vector<std::string_view> Split(std::string_view Input, std::string_view Needle, bool PreserveNULL = false)
    {
        std::vector<std::string_view> Tokens{};

        while (!Input.empty())
        {
            const auto Length = Input.find(Needle);
            if (Length == std::string_view::npos) break;

            if (PreserveNULL || Length) Tokens.emplace_back(Input.substr(0, Length));
            Input.remove_prefix(Length + Needle.size());
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }
    constexpr std::vector<std::wstring_view> Split(std::wstring_view Input, std::wstring_view Needle, bool PreserveNULL = false)
    {
        std::vector<std::wstring_view> Tokens{};

        while (!Input.empty())
        {
            const auto Length = Input.find(Needle);
            if (Length == std::wstring_view::npos) break;

            if (PreserveNULL || Length) Tokens.emplace_back(Input.substr(0, Length));
            Input.remove_prefix(Length + Needle.size());
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }
    constexpr std::vector<std::u8string_view> Split(std::u8string_view Input, std::u8string_view Needle, bool PreserveNULL = false)
    {
        std::vector<std::u8string_view> Tokens{};

        while (!Input.empty())
        {
            const auto Length = Input.find(Needle);
            if (Length == std::string_view::npos) break;

            if (PreserveNULL || Length) Tokens.emplace_back(Input.substr(0, Length));
            Input.remove_prefix(Length + Needle.size());
        }

        // Any remaining.
        if (!Input.empty())
            Tokens.emplace_back(Input);

        return Tokens;
    }

    // Common overloads.
    constexpr std::vector<std::string_view> Split(std::string_view Input, char Needle, bool PreserveNULL = false)
    {
        return Split(Input, std::string_view{ &Needle, 1 }, PreserveNULL);
    }
    constexpr std::vector<std::wstring_view> Split(std::wstring_view Input, wchar_t Needle, bool PreserveNULL = false)
    {
        return Split(Input, std::wstring_view{ &Needle, 1 }, PreserveNULL);
    }
    constexpr std::vector<std::u8string_view> Split(std::u8string_view Input, char8_t Needle, bool PreserveNULL = false)
    {
        return Split(Input, std::u8string_view{ &Needle, 1 }, PreserveNULL);
    }
}

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    // Without NULL tokens. (..., false)
    static_assert(4 == String::Split("ab,c,,,,,d,e", ',').size(), "BROKEN: String::Split(A)");
    static_assert(4 == String::Split(L"ab,c,,,,,d,e", L',').size(), "BROKEN: String::Split(W)");
    static_assert(4 == String::Split(u8"ab,c,,,,,d,e", u8',').size(), "BROKEN: String::Split(U8)");

    // Preserve NULL tokens.
    static_assert(8 == String::Split("ab,c,,,,,d,e", ',', true).size(), "BROKEN: String::Split(A)");
    static_assert(8 == String::Split(L"ab,c,,,,,d,e", L',', true).size(), "BROKEN: String::Split(W)");
    static_assert(8 == String::Split(u8"ab,c,,,,,d,e", u8',', true).size(), "BROKEN: String::Split(U8)");

    // Splits on ' ' and ", dropping anything in between. = { "a", "b c ", "d" }
    static_assert(3 == String::Tokenize(R"(a "b c "    "" d)").size(), "BROKEN: String::Tokenize(A)");
    static_assert(3 == String::Tokenize(LR"(a "b c "    "" d)").size(), "BROKEN: String::Tokenize(W)");
    static_assert(3 == String::Tokenize(u8R"(a "b c "    "" d)").size(), "BROKEN: String::Tokenize(U8)");
}
#endif
