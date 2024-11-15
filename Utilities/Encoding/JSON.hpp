/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <Utilities.hpp>
#include <Strings.hpp>

// Temp
#include <map>
#include <unordered_map>

namespace JSON
{
    using Object_t = std::unordered_map<std::u8string, struct Value_t>;
    using Array_t = std::vector<struct Value_t>;
    using String_t = std::u8string;
    using Null_t = std::monostate;
    using Unsigned_t = uint64_t;
    using Signed_t = int64_t;
    using Number_t = double;
    using Boolean_t = bool;

    template <typename T, typename... U> concept Any_t = (std::same_as<T, U> || ...);
    template <typename T> concept Supported_t = Any_t<T, Null_t, Boolean_t, Number_t, Signed_t, Unsigned_t, Object_t, Array_t, String_t>;

    // Encode strings in the format the user wants.
    namespace Internal
    {
        template <typename T> constexpr T toString(const String_t &Value)
        {
            if constexpr (std::is_same_v<typename T::value_type, wchar_t>) return Encoding::toUNICODE(Value);
            else if constexpr (std::is_same_v<typename T::value_type, char>) return Encoding::toASCII(Value);
            else if constexpr (std::is_same_v<typename T::value_type, char8_t>) return Value;

            else static_assert(cmp::always_false<T>, "Invalid string type, Blob_t should be handled elsewhere.");

            std::unreachable();
        }
    }

    // Generic JSON value.
    struct Value_t
    {
        std::variant<Null_t, Boolean_t, Number_t, Signed_t, Unsigned_t, Object_t, Array_t, String_t> Storage{ Null_t{} };
        template <Supported_t T> bool isType() const { return std::holds_alternative<T>(Storage); }

        // Direct access to the stored value (if available), else to a default-constructed element.
        template <Supported_t T> operator const T&() const { if (isType<T>()) return std::get<T>(Storage); static T Dummy{}; return Dummy; }
        template <Supported_t T> operator T &() { if (isType<T>()) return std::get<T>(Storage); static T Dummy{}; return Dummy; }

        // Try to convert to compatible types, Get function for explicit access.
        template <typename T> requires(!Supported_t<T>) operator T() const
        {
            // POD
                 if constexpr (std::is_same_v<T, bool>) { if (isType<Boolean_t>()) return T(std::get<Boolean_t>(Storage)); return T{}; }
            else if constexpr (std::is_floating_point_v<T>) { if (isType<Number_t>()) return T(std::get<Number_t>(Storage)); return T{}; }
            else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) { if (isType<Signed_t>()) return T(std::get<Signed_t>(Storage)); return T{}; }
            else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) { if (isType<Unsigned_t>()) return T(std::get<Unsigned_t>(Storage)); return T{}; }

            // Blob_t derives from basic_string, so it needs to be checked first.
            else if constexpr (std::is_same_v<T, Blob_t> || cmp::isDerived<T, std::unordered_set> || cmp::isDerived<T, std::vector> || cmp::isDerived<T, std::set>)
            {
                if (!isType<Array_t>()) return T{};
                return T(std::get<Array_t>(Storage).begin(), std::get<Array_t>(Storage).end());
            }
            else if constexpr (cmp::isDerived<T, std::basic_string> || cmp::isDerived<T, std::pmr::basic_string>)
            {
                if (!isType<String_t>()) return T{};
                return Internal::toString<T>(std::get<String_t>(Storage));
            }

            // Maps need to be checked last due to typename T::key_type always being considered.
            else if constexpr ((cmp::isDerived<T, std::unordered_map> || cmp::isDerived<T, std::map>) && cmp::isDerived<typename T::key_type, std::basic_string>)
            {
                if (!isType<Object_t>()) return T{};
                T Output{};

                for (const auto &[Key, Value] : std::get<Object_t>(Storage))
                    Output.emplace(Internal::toString<typename T::key_type>(Key), Value);

                return Output;
            }

            else static_assert(cmp::always_false<T>, "Could not convert JSON value to T");

            std::unreachable();
        }
        template <typename T> T Get() const { return static_cast<T>(*this); }

        // Convert to a supported type.
        Value_t() = default;
        template <cmp::Byte_t U, size_t N> Value_t(const U(&Value)[N]) : Storage(String_t{ Encoding::toUTF8(Value) }) {}
        template <Supported_t T> Value_t(const T &Value) : Storage(Value) {}
        template <typename T> Value_t(const T &Value)
        {
            if constexpr (cmp::isDerived<T, std::optional>)
            {
                if (Value) *this = Value_t(*Value);
                else Storage = Null_t{};
            }
            else if constexpr (std::is_same_v<T, Value_t>)
            {
                Storage = Value.Storage;
            }
            else
            {
                Storage = [](const auto &Value)
                {
                    // Generally POD.
                         if constexpr (std::is_floating_point_v<T>) return Number_t{ Value };
                    else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) return Signed_t{ Value };
                    else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) return Unsigned_t{ Value };

                    // Blob_t derives from basic_string, so it needs to be checked first.
                    else if constexpr (std::is_same_v<T, Blob_t> || cmp::isDerived<T, std::unordered_set> || cmp::isDerived<T, std::vector> || cmp::isDerived<T, std::set>)
                    {
                        Array_t Output{};
                        for (const auto &Item : Value)
                            Output.emplace_back(Item);
                        return Output;
                    }
                    else if constexpr (cmp::isDerived<T, std::basic_string> || cmp::isDerived<T, std::basic_string_view>)
                    {
                        return String_t{ Encoding::toUTF8(Value) };
                    }

                    // Maps need to be checked last due to typename T::key_type always being considered.
                    else if constexpr ((cmp::isDerived<T, std::unordered_map> || cmp::isDerived<T, std::map>) && cmp::isDerived<typename T::key_type, std::basic_string>)
                    {
                        Object_t Output{};
                        for (const auto &[Key, Item] : Value)
                            Output.emplace(Encoding::toUTF8(Key), Item);
                        return Output;
                    }
                    else static_assert(cmp::always_false<T>, "Could not convert T to JSON value");

                    std::unreachable();
                }(Value);
            }
        }

        // Provides a default constructed value on error.
        Value_t &operator[](size_t i)
        {
            if (isType<Array_t>()) return std::get<Array_t>(Storage)[i];
            else { static Value_t Dummy{}; return Dummy; }
        }
        const Value_t &operator[](size_t i) const
        {
            if (isType<Array_t>()) return std::get<Array_t>(Storage)[i];
            else { static Value_t Dummy{}; return Dummy; }
        }
        template <cmp::Byte_t U> Value_t &operator[](const std::basic_string_view<U> Key)
        {
            if (isType<Object_t>()) return std::get<Object_t>(Storage)[Encoding::toUTF8(Key)];
            else { static Value_t Dummy{}; return Dummy; }
        }
        template <cmp::Byte_t U> const Value_t &operator[](const std::basic_string_view<U> Key) const
        {
            if (isType<Object_t>())
            {
                const auto &Object = std::get<Object_t>(Storage);
                if (Object.contains(Encoding::toUTF8(Key)))
                    return Object.at(Encoding::toUTF8(Key));
            }

            static Value_t Dummy{}; return Dummy;
        }
        template <cmp::Byte_t U, size_t N> Value_t &operator[](const U(&Key)[N])
        {
            if (isType<Object_t>()) return std::get<Object_t>(Storage)[Encoding::toUTF8(Key)];
            else { static Value_t Dummy{}; return Dummy; }
        }
        template <cmp::Byte_t U, size_t N> const Value_t &operator[](const U(&Key)[N]) const
        {
            if (isType<Object_t>())
            {
                const auto &Object = std::get<Object_t>(Storage);
                if (Object.contains(Encoding::toUTF8(Key)))
                    return Object.at(Encoding::toUTF8(Key));
            }

            static Value_t Dummy{}; return Dummy;
        }

        // Safer access the the storage.
        template <typename T = Value_t, cmp::Byte_t U, size_t N> requires(std::is_convertible_v<Value_t, T>)
        T value(const U(&Key)[N], const T &Defaultvalue = {}) const
        {
            if (const auto Value = operator[](Key))
                return Value;
            return Defaultvalue;
        }
        template <typename T = Value_t, cmp::Byte_t U> requires(std::is_convertible_v<Value_t, T>)
        T value(const std::basic_string_view<U> Key, const T &Defaultvalue = {}) const
        {
            if (const auto Value = operator[](Encoding::toUTF8(Key)))
                return Value;
            return Defaultvalue;
        }
        template <typename T = Value_t> requires(std::is_convertible_v<Value_t, T>)
        T value(const T &Defaultvalue = {}) const
        {
            if (isType<T>()) return Get<T>();
            return Defaultvalue;
        }

        // Helpers for objects.
        bool empty() const
        {
            if (isType<Object_t>()) return std::get<Object_t>(Storage).empty();
            if (isType<String_t>()) return std::get<String_t>(Storage).empty();
            if (isType<Array_t>()) return std::get<Array_t>(Storage).empty();

            return true;
        }
        bool contains(const String_t &Key) const
        {
            if (!isType<Object_t>()) return false;
            return std::get<Object_t>(Storage).contains(Key);
        }
        bool contains(const std::string &Key) const
        {
            if (!isType<Object_t>()) return false;
            return std::get<Object_t>(Storage).contains(Encoding::toUTF8(Key));
        }
        template <typename ...Args> bool contains_all(Args&&... va) const
        {
            return (contains(va) && ...);
        }
        template <typename ...Args> bool contains_any(Args&&... va) const
        {
            return (contains(va) || ...);
        }

        // Serialize to string.
        std::string dump() const
        {
            if (isType<Null_t>()) return "null";
            if (isType<Number_t>()) return va("%f", Get<Number_t>());
            if (isType<Signed_t>()) return va("%lli", Get<Signed_t>());
            if (isType<Unsigned_t>()) return va("%llu", Get<Unsigned_t>());
            if (isType<Boolean_t>()) return Get<Boolean_t>() ? "true" : "false";
            if (isType<String_t>()) return va("\"%s\"", Encoding::toASCII(Get<String_t>()));

            if (isType<Array_t>())
            {
                std::string Result{ "[" };
                for (const auto &Item : Get<Array_t>())
                {
                    Result.append(Item.dump());
                    Result.append(" ,");
                }

                if (!Get<Array_t>().empty()) Result.pop_back();
                Result.append("]");
                return Result;
            }
            if (isType<Object_t>())
            {
                std::string Result{ "{" };
                for (const auto &[Key, Value] : Get<Object_t>())
                {
                    Result.append(va("\"%s\" : ", Encoding::toASCII(Key)));
                    Result.append(Value.dump());
                    Result.append(" ,");
                }

                if (!Get<Object_t>().empty()) Result.pop_back();
                Result.append("}");
                return Result;
            }

            std::unreachable();
        }
    };

    // Naive parsing.
    namespace Parsing
    {
        constexpr std::u8string_view Skip(std::u8string_view Input)
        {
            // Should never happen..
            if (Input.empty()) [[unlikely]]
                return Input;

            constexpr auto isWhitespace = [](char8_t Char)
            {
                constexpr auto Charset = std::u8string_view(u8" \t\n\v\f\r");

                #if defined(__cpp_lib_string_contains)
                return Charset.contains(Char);
                #else
                return Charset.find(Char) != Charset.end();
                #endif
            };

            auto View = Input | std::views::drop_while([&](auto Char) { return isWhitespace(Char); });
            return std::u8string_view(View.begin(), View.end());
        }

        inline std::optional<Value_t> Parsevalue(std::u8string_view &Input);
        inline std::optional<Array_t> Parsearray(std::u8string_view &Input)
        {
            Array_t Result{};

            // How did we even get here?
            if (Input.empty() || Input.front() != u8'[')
                return {};

            Input.remove_prefix(1); Input = Skip(Input);
            while (!Input.empty() && Input.front() != u8']')
            {
                if (const auto Value = Parsevalue(Input))
                    Result.emplace_back(*Value);
                else return {};

                Input = Skip(Input);
                if (Input.empty() || (Input.front() != u8',' && Input.front() != u8']'))
                    return {};

                if (Input.front() == u8',') Input.remove_prefix(1);
                Input = Skip(Input);
            }

            if (Input.empty() || Input.front() != u8']')
                return {};

            Input.remove_prefix(1);
            return Result;
        }
        inline std::optional<String_t> Parsestring(std::u8string_view &Input)
        {
            String_t Result{};

            // How did we even get here?
            if (Input.empty() || Input.front() != u8'"')
                return {};

            Input.remove_prefix(1);
            while (!Input.empty() && Input.front() != u8'"')
            {
                if (Input.front() == u8'\\')
                    Input.remove_prefix(1);

                Result.push_back(Input.front());
                Input.remove_prefix(1);
            }

            // No endtoken.
            if (Input.empty() || Input.front() != u8'"')
                return {};

            Input.remove_prefix(1);
            return Result;
        }
        inline std::optional<Object_t> Parseobject(std::u8string_view &Input)
        {
            Object_t Result{};

            // How did we even get here?
            if (Input.empty() || Input.front() != u8'{')
                return {};

            Input.remove_prefix(1); Input = Skip(Input);
            while (!Input.empty() && Input.front() != u8'}')
            {
                const auto Key = Parsestring(Input);
                if (!Key) return {};

                Input = Skip(Input);
                if (Input.empty() || Input.front() != u8':')
                    return {};

                Input.remove_prefix(1);
                if (const auto Value = Parsevalue(Input))
                    Result.emplace(*Key, *Value);
                else return {};

                Input = Skip(Input);
                if (Input.empty() || (Input.front() != u8',' && Input.front() != u8'}'))
                    return {};

                if (Input.front() == u8',') Input.remove_prefix(1);
                Input = Skip(Input);
            }

            if (Input.empty() || Input.front() != u8'}')
                return {};

            Input.remove_prefix(1);
            return Result;
        }

        // Only function that can be called safely from usercode.
        inline std::optional<Value_t> Parsevalue(std::u8string_view &Input)
        {
            Value_t Result{};
            Input = Skip(Input);

            // How did we even get here?
            if (Input.empty()) return {};

            if (Input.starts_with(u8"null"))
            {
                Result = Null_t{};
                Input.remove_prefix(4);
            }
            else if (Input.front() == u8'"')
            {
                const auto Value = Parsestring(Input);
                if (!Value) return {};
                Result = *Value;
            }
            else if (Input.front() == u8'{')
            {
                const auto Value = Parseobject(Input);
                if (!Value) return {};
                Result = *Value;
            }
            else if (Input.front() == u8'[')
            {
                const auto Value = Parsearray(Input);
                if (!Value) return {};
                Result = *Value;
            }
            else if (Input.starts_with(u8"true"))
            {
                Result = Boolean_t{ true };
                Input.remove_prefix(4);
            }
            else if (Input.starts_with(u8"false"))
            {
                Result = Boolean_t(false);
                Input.remove_prefix(5);
            }
            else if ((Input.front() >= '0' && Input.front() <= '9') || Input.front() == u8'-')
            {
                Unsigned_t Unsigned = 0; Signed_t Signed = 0; Number_t Number = 0;

                const auto Substring = Encoding::toASCII(Input.substr(0, 32));

                if (const auto [Ptr, ec] = std::from_chars(Substring.data(), Substring.data() + Substring.size(), Unsigned); ec == std::errc())
                { Result = Unsigned; Input.remove_prefix(Ptr - Substring.data()); }
                else if (const auto [Ptr, ec] = std::from_chars(Substring.data(), Substring.data() + Substring.size(), Signed); ec == std::errc())
                { Result = Signed; Input.remove_prefix(Ptr - Substring.data()); }
                else if (const auto [Ptr, ec] = std::from_chars(Substring.data(), Substring.data() + Substring.size(), Number); ec == std::errc())
                { Result = Number; Input.remove_prefix(Ptr - Substring.data()); }
                else return {};
            }
            else
                return {};

            return Result;
        }
    }

    // Pretty basic safety checks.
    inline Value_t Parse(std::u8string_view JSONString)
    {
        // To simplify other operations, accept a null string.
        if (JSONString.empty()) [[unlikely]] return {};

        // Malformed string check. Missing brackets, null-chars messing up C-string parsing, etc..
        const auto C1 = std::ranges::count(JSONString, '{') != std::ranges::count(JSONString, '}');
        const auto C2 = std::ranges::count(JSONString, '[') != std::ranges::count(JSONString, ']');
        const auto C3 = std::ranges::count(JSONString, '\0') > 1;
        if (C1 || C2 || C3) [[unlikely]]
        {
            if (C1) Errorprint("Trying to parse invalid JSON string, missing }");
            if (C2) Errorprint("Trying to parse invalid JSON string, missing ]");
            if (C3) Errorprint("Trying to parse invalid JSON string, null-chars in string");

            assert(false);
            return {};
        }

        [[maybe_unused]] const auto Original = JSONString;
        const auto Value = Parsing::Parsevalue(JSONString);
        if (!Value)
        {
            Errorprint(va("JSON Parsing failed at position: %zu", JSONString.data() - Original.data()));
            if (!std::is_constant_evaluated()) assert(false);
            return {};
        }

        return Value;
    }
    inline Value_t Parse(std::string_view JSONString)
    {
        return Parse(std::u8string_view((const char8_t *)JSONString.data(), JSONString.size()));
    }

    // Mainly for use with objects and arrays.
    inline std::string Dump(Value_t &&Value)
    {
        return Value.dump();
    }
    inline std::string Dump(const Value_t &Value)
    {
        return Value.dump();
    }
}

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    inline void JSONtest()
    {
        constexpr auto Input = u8R"({ "Object" : { "Key" : 42 }, "Array" : [ 0, 1, 2, "mixed" ] })";
        const auto Parsed = JSON::Parse(Input);
        const auto int42 = Parsed[u8"Object"][u8"Key"];
        const auto int2 = Parsed[u8"Array"][2];
        const auto Mix = Parsed[u8"Array"][3];

        const auto Test0 = uint32_t(42) == int42.Get<uint32_t>();
        const auto Test1 = uint64_t(42) == (uint64_t)int42;
        const auto Test2 = uint64_t(2) == (uint64_t)int2;
        const auto Test3 = u8"mixed"s == Mix.Get<std::u8string>();

        if(!Test0 || !Test1 || !Test2 || !Test3)
            std::printf("BROKEN: JSON Parsing\n");

        const auto Dump = Parsed.dump();
        if (Dump != JSON::Parse(Dump).dump())
            std::printf("BROKEN: JSON Dumping\n");
    };
}
#endif
