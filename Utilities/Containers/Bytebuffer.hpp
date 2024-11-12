/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-20
    License: MIT

    A simple container that prefixes values with a type-ID.
    Data is stored/serialized as little-endian.
*/

#pragma once
#include <Utilities/Utilities.hpp>
#include "../Encoding.hpp"
#include "../Strings.hpp"

namespace Bytebuffer
{
    // Type identifiers for the stream.
    enum Datatypes_t : uint8_t
    {
        BB_NONE = 0,        // Padding / NOP.

        BB_BLOB,            // uint8_t
        BB_ASCIISTRING,     // char
        BB_UTF8STRING,      // char8_t
        BB_UNICODESTRING,   // wchar_t

        // POD.
        BB_BOOL,
        BB_SINT8,
        BB_UINT8,
        BB_SINT16,
        BB_UINT16,
        BB_SINT32,
        BB_UINT32,
        BB_SINT64,
        BB_UINT64,
        BB_FLOAT32,
        BB_FLOAT64,
        BB_LAST_POD,

        BB_ARRAY = 100, // Arraytypes, e.g. 100 + BB_UINT16 = vector<uint16_t>

        BB_MAX
    };

    // Deduce the ID from the datatype.
    template <typename Type> static constexpr uint8_t toID()
    {
        // If it's an enumeration, get the base type.
        if constexpr (std::is_enum_v<Type>) return toID<std::underlying_type_t<Type>>();

        // Vectors are either arrays or blobs of data.
        if constexpr (cmp::isDerived<Type, std::vector> || cmp::isDerivedEx<Type, std::array> || cmp::isDerivedEx<Type, std::span> )
        {
            // Do not use spans for strings, they are a headache to deal with.
            if constexpr (cmp::isDerivedEx<Type, std::span>)
            {
                if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, char>) ASSERT(false);
                if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, char8_t>) ASSERT(false);
                if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, wchar_t>) ASSERT(false);
            }

            if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, uint8_t>) return BB_BLOB;
            return BB_ARRAY + toID<std::decay_t<typename Type::value_type>>();
        }

        // Strings of std::span is not supported because they are a pain.
        if constexpr (cmp::isDerived<Type, std::basic_string> || cmp::isDerived<Type, std::basic_string_view>)
        {
            if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, uint8_t>) return BB_BLOB;
            if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, char>) return BB_ASCIISTRING;
            if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, char8_t>) return BB_UTF8STRING;
            if constexpr (std::is_same_v<std::decay_t<typename Type::value_type>, wchar_t>) return BB_UNICODESTRING;
        }

        // POD.
        if constexpr (std::is_same_v<std::decay_t<Type>, bool>)             return BB_BOOL;
        if constexpr (std::is_same_v<std::decay_t<Type>, int8_t>)           return BB_SINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint8_t>)          return BB_UINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, int16_t>)          return BB_SINT16;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint16_t>)         return BB_UINT16;
        if constexpr (std::is_same_v<std::decay_t<Type>, int32_t>)          return BB_SINT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint32_t>)         return BB_UINT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, int64_t>)          return BB_SINT64;
        if constexpr (std::is_same_v<std::decay_t<Type>, uint64_t>)         return BB_UINT64;
        if constexpr (std::is_same_v<std::decay_t<Type>, float>)            return BB_FLOAT32;
        if constexpr (std::is_same_v<std::decay_t<Type>, double>)           return BB_FLOAT64;

        // Extensions for POD where the user uses an aliased type.
        if constexpr (std::is_same_v<std::decay_t<Type>, char>)             return BB_SINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, char8_t>)          return BB_UINT8;
        if constexpr (std::is_same_v<std::decay_t<Type>, char16_t>)         return BB_UINT16;
        if constexpr (std::is_same_v<std::decay_t<Type>, wchar_t>)
        {
            // *nix and Windows disagree on how large a wchar_t is..
            if constexpr (sizeof(wchar_t) == 4)                             return BB_UINT32;
            else                                                            return BB_UINT16;
        }

        return BB_NONE;
    }
}

struct Bytebuffer_t
{
    // Default to owning mode if nothing else is specified..
    std::variant<const uint8_t *, uint8_t *> Internalbuffer{ (uint8_t *)nullptr };
    uint32_t Internaliterator{};
    uint32_t Internalsize{};

    // Utility access.
    void Rewind() noexcept
    {
        Internaliterator = 0;
    }
    [[nodiscard]] uint8_t Peek() const noexcept
    {
        const auto pByte = data(true);
        if (!pByte) return Bytebuffer::BB_NONE;
        return uint8_t(*pByte);
    }
    [[nodiscard]] bool isOwning() const noexcept
    {
        return std::holds_alternative<uint8_t *>(Internalbuffer);
    }
    void Seek(int Offset, int Origin) noexcept
    {
        switch (Origin)
        {
            case SEEK_SET:
                Internaliterator = cmp::clamp(uint32_t(Offset), uint32_t(0), Internalsize);
                break;

            case SEEK_CUR:
                Internaliterator = cmp::clamp(uint32_t(Internaliterator + Offset), uint32_t(0), Internalsize);
                break;

            // A negative offset is undefined in C, so just take the absolute value.
            case SEEK_END:
                Internaliterator = cmp::clamp(uint32_t(Internalsize - cmp::abs(Offset)), uint32_t(0), Internalsize);
                break;

            default:
                // NOP
                ASSERT(false);
        }
    }
    void Expandbuffer(size_t Extracapacity)
    {
        // Need to switch to owning mode.
        if (!isOwning())
        {
            const auto Originalbuffer = std::get<const uint8_t *>(Internalbuffer);
            const auto Originalsize = Internalsize;

            // Original buffer should never be nullptr, but better safe than sorry..
            const auto Newbuffer = (uint8_t *)malloc(Originalsize + Extracapacity);
            if (Originalbuffer) std::memcpy(Newbuffer, Originalbuffer, Originalsize);
            std::memset(Newbuffer + Originalsize, 0, Extracapacity);

            Internalbuffer = Newbuffer;
            Internalsize = Originalsize + Extracapacity;
        }

        // Else we just alloc/realloc.
        else
        {
            const auto Originalbuffer = std::get<uint8_t *>(Internalbuffer);
            const auto Originalsize = Internalsize;

            if (Originalbuffer)
            {
                const auto Newbuffer = (uint8_t *)realloc(Originalbuffer, Originalsize + Extracapacity);
                std::memset(Newbuffer + Originalsize, 0, Extracapacity);

                Internalbuffer = Newbuffer;
                Internalsize = Originalsize + Extracapacity;
            }
            else
            {
                const auto Newbuffer = (uint8_t *)malloc(Originalsize + Extracapacity);
                std::memset(Newbuffer + Originalsize, 0, Extracapacity);

                Internalbuffer = Newbuffer;
                Internalsize = Originalsize + Extracapacity;
            }
        }
    }
    [[nodiscard]] size_t size(bool Remainder = false) const noexcept
    {
        return Internalsize - (Remainder ? Internaliterator : 0);
    }
    [[nodiscard]] const uint8_t *data(bool atOffset = false) const noexcept
    {
        // Trying to get a pointer past our allocation.
        if (atOffset && (Internaliterator >= Internalsize)) return nullptr;

        // Assume that the iterator is at a valid position.
        if (isOwning()) return std::get<uint8_t *>(Internalbuffer) + (atOffset ? Internaliterator : 0);
        else return std::get<const uint8_t *>(Internalbuffer) + (atOffset ? Internaliterator : 0);
    }
    [[nodiscard]] std::span<const uint8_t> as_span() const
    {
        return { data(), size() };
    }

    // Direct IO on the buffer.
    bool rawRead(size_t Size, void *Buffer = nullptr) noexcept
    {
        // If there's not enough data left we can't do much.
        if ((Internaliterator + Size) > Internalsize) [[unlikely]]
            return false;

        // If there's a buffer, just memcpy into it.
        if (Buffer) if (const auto Data = data(true); Data) std::memcpy(Buffer, Data, Size);

        Internaliterator += Size;
        return true;
    }
    void rawWrite(size_t Size, const void *Buffer = nullptr)
    {
        // If we were created as non-owning we need to do a realloc regardless.
        if (!isOwning() || ((Internaliterator + Size) > Internalsize))
            Expandbuffer((Internaliterator + Size) - Internalsize);

        // A null-buffer is just a memset.
        const auto pBuffer = std::get<uint8_t *>(Internalbuffer) + Internaliterator;
        if (Buffer) std::memcpy(pBuffer, Buffer, Size);
        else std::memset(pBuffer, 0, Size);

        Internaliterator += Size;
    }

    // Typed IO, prefix the type with the ID.
    template <typename Type> bool Read(Type &Buffer, bool Typechecked = true)
    {
        constexpr auto ExpectedID = Bytebuffer::toID<Type>();
        const auto StoredID = Peek();

        // Can't read into const storage.
        static_assert(!cmp::isDerived<Type, std::basic_string_view>, "Can not write to a string view.");

        // Verify that the requested type is compatible.
        if (Typechecked || ExpectedID > Bytebuffer::BB_ARRAY) [[likely]]
        {
            // Special case: padding / NAN / whatever.
            if (StoredID == Bytebuffer::BB_NONE) [[unlikely]]
            {
                Buffer = {};
                return rawRead(sizeof(StoredID));
            }

            // If the ID is not the same, do not modify.
            if (StoredID != ExpectedID) [[unlikely]]
                return false;

            // Advance past the type prefix.
            rawRead(sizeof(StoredID));
        }

        // Array of values.
        if constexpr (cmp::isDerived<Type, std::vector>)
        {
            const auto Size = Read<uint32_t>();
            const auto Count = Read<uint32_t>(false);

            // Can't trust anyone these days..
            if (Size == 0) [[unlikely]] return false;
            if (Count == 0) [[unlikely]] return false;
            if ((Count * sizeof(typename Type::value_type)) != Size) [[unlikely]] return false;

            // STL implements vector<bool> as a bitset, so we can't really memcpy.
            if constexpr (std::is_same_v<typename Type::value_type, bool>)
            {
                const auto Old = Internaliterator;
                Buffer.resize(Count);

                for (size_t i = 0; i < Count; ++i)
                    Buffer[i] = Read<bool>(false);

                return (Internaliterator - Old) == Size;
            }
            else
            {
                Buffer.resize(Count);
                const auto Result = rawRead(Size, Buffer.data());

                // Ensure endian-ness, NOP on little-endian systems.
                if constexpr (std::is_integral_v<Type> || std::is_floating_point_v<Type>)
                {
                    for (size_t i = 0; i < Count; ++i)
                        Buffer[i] = cmp::fromLittle(Buffer[i]);
                }

                return Result;
            }
        }

        // Strings and blobs.
        if constexpr (cmp::isDerived<Type, std::basic_string>)
        {
            if constexpr (std::is_same_v<Type, Blob_t>)
            {
                // Just a size and a block of bytes..
                const auto Size = Read<uint32_t>(Typechecked);
                Buffer.resize(Size);

                return rawRead(Size, Buffer.data());
            }
            else
            {
                // Strings are null-terminated, so just initialize from the pointer.
                Buffer = Type((const typename Type::value_type *)data(true));

                // Sanity checking.
                if constexpr (std::is_same_v<typename Type::value_type, char>) ASSERT(std::isalnum(Buffer.front()));
                if constexpr (std::is_same_v<typename Type::value_type, char8_t>) ASSERT(std::isalnum(Buffer.front()));
                if constexpr (std::is_same_v<typename Type::value_type, wchar_t>) ASSERT(std::isalnum(Buffer.front()));

                return rawRead((Buffer.size() + 1) * sizeof(typename Type::value_type));
            }
        }

        // POD. Ensure endian-ness, NOP on little-endian systems.
        const auto Result = rawRead(sizeof(Type), &Buffer);
        if constexpr (std::is_integral_v<Type> || std::is_floating_point_v<Type>)
        {
            Buffer = cmp::fromLittle(Buffer);
        }
        return Result;
    }
    template <typename Type> void Write(const Type &Value, bool Typechecked = true)
    {
        // User-specified serialization.
        if constexpr (requires { Value.Serialize(*this); }) return Value.Serialize(*this);

        // Special case of merging bytebuffers.
        else if constexpr (std::is_same_v<Type, Bytebuffer_t>)
        {
            rawWrite(cmp::max(Value.Internaliterator, Value.Internalsize), Value.data());
            return;
        }

        // If we encounter an optional, process it instead.
        else if constexpr (cmp::isDerived<Type, std::optional>)
        {
            if (Value) return Write(*Value, Typechecked);
            else return WriteNULL();
        }
        else
        {
            static_assert(Bytebuffer::toID<Type>() != Bytebuffer::BB_NONE, "Invalid datatype.");
        }

        // A byte prefix identifier for the type.
        constexpr auto TypeID = Bytebuffer::toID<Type>();
        if (Typechecked || TypeID > Bytebuffer::BB_ARRAY) [[likely]]
            rawWrite(sizeof(TypeID), &TypeID);

        // Array of values.
        if constexpr (cmp::isDerived<Type, std::vector> || cmp::isDerivedEx<Type, std::array> || cmp::isDerivedEx<Type, std::span>)
        {
            // Elements of the array can only be POD.
            constexpr auto Internaltype = Bytebuffer::toID<typename Type::value_type>();
            static_assert(Internaltype < Bytebuffer::BB_LAST_POD && Internaltype != Bytebuffer::BB_NONE, "Invalid datatype");

            // Array layout, total size and element count.
            Write<uint32_t>(uint32_t(sizeof(typename Type::value_type) * Value.size()));
            Write<uint32_t>((uint32_t)Value.size(), false);

            // No need for a type ID with each element.
            for (const auto &Item : Value)
                Write(Item, false);

            return;
        }

        // Strings and blobs.
        if constexpr (cmp::isDerived<Type, std::basic_string> || cmp::isDerived<Type, std::basic_string_view>)
        {
            if constexpr (std::is_same_v<Type, Blob_t>)
            {
                Write<uint32_t>((uint32_t)Value.size(), Typechecked);
                rawWrite(Value.size(), Value.data());
            }
            else if constexpr (cmp::isDerived<Type, std::basic_string>)
            {
                // Write as null-terminated string.
                rawWrite((Value.size() + 1) * sizeof(typename Type::value_type), Value.c_str());
            }
            else
            {
                // Write as null-terminated string.
                rawWrite((Value.size()) * sizeof(typename Type::value_type), Value.data());
                WriteNULL();
            }

            return;
        }

        // POD.
        if constexpr (std::is_integral_v<Type> || std::is_floating_point_v<Type>)
        {
            const auto Temp = cmp::toLittle(Value);
            rawWrite(sizeof(Type), &Temp);
        }
        else rawWrite(sizeof(Type), &Value);
    }
    template <typename Type> Type Read(bool Typechecked = true)
    {
        Type Result{};

        // User-specified serialization.
        if constexpr (requires { Result.Deserialize(*this); }) return Result.Deserialize(*this);

        Read(Result, Typechecked);
        return Result;
    }
    void WriteNULL()
    {
        return rawWrite(1);
    }

    // Helper for reading and writing.
    template <cmp::Char_t T, size_t N> Bytebuffer_t &operator<<(const T(&Input)[N])
    {
        Write(std::basic_string_view<T>(Input));
        return *this;
    }
    template <typename Type> Bytebuffer_t &operator<<(Type Value)
    {
        Write(Value);
        return *this;
    }
    template <typename Type> void operator>>(Type &Buffer)
    {
        Read(Buffer);
    }

    // Helper for serializing the contents.
    [[nodiscard]] std::string to_hex(bool fromOffset = false) const
    {
        return String::toHEX(std::span(data(fromOffset), size(fromOffset)), true);
    }
    [[nodiscard]] std::string to_string() const
    {
        // Take a copy so we don't have to mess with the iterator.
        Bytebuffer_t Reader(data(), size());
        std::stringstream Output;
        uint8_t TypeID;

        Output << "{\n";
        while ((TypeID = Reader.Peek()) != Bytebuffer::BB_NONE)
        {
            Output << "    ";

            // Reflection would be nice to have..
            #define POD(x, y, ...) case x: Output << #y << " = " << __VA_ARGS__ Reader.Read<y>() << '\n'; break
            #define String(x, y) case x: Output << #y << " = " << Encoding::toASCII(Reader.Read<y>()) << '\n'; break

            #define Array_POD(x, y, ...) case Bytebuffer::BB_ARRAY + x: Output << #y << "[] = "; \
            for (const auto &Item : Reader.Read<std::vector<y>>()) Output << __VA_ARGS__ Item << ", "; Output << '\n'; break

            #define Array_String(x, y) case Bytebuffer::BB_ARRAY + x: Output << #y << "[] = "; \
            for (const auto &Item : Reader.Read<std::vector<y>>()) Output << Encoding::toASCII(Item) << ", "; Output << '\n'; break

            switch (TypeID)
            {
                POD(Bytebuffer::BB_BOOL, bool);
                POD(Bytebuffer::BB_SINT8, int8_t, (int));
                POD(Bytebuffer::BB_UINT8, uint8_t, (unsigned));
                POD(Bytebuffer::BB_SINT16, int16_t);
                POD(Bytebuffer::BB_UINT16, uint16_t);
                POD(Bytebuffer::BB_SINT32, int32_t);
                POD(Bytebuffer::BB_UINT32, uint32_t);
                POD(Bytebuffer::BB_SINT64, int64_t);
                POD(Bytebuffer::BB_UINT64, uint64_t);
                POD(Bytebuffer::BB_FLOAT32, float);
                POD(Bytebuffer::BB_FLOAT64, double);

                Array_POD(Bytebuffer::BB_BOOL, bool);
                Array_POD(Bytebuffer::BB_SINT8, int8_t, (int));
                Array_POD(Bytebuffer::BB_UINT8, uint8_t, (unsigned));
                Array_POD(Bytebuffer::BB_SINT16, int16_t);
                Array_POD(Bytebuffer::BB_UINT16, uint16_t);
                Array_POD(Bytebuffer::BB_SINT32, int32_t);
                Array_POD(Bytebuffer::BB_UINT32, uint32_t);
                Array_POD(Bytebuffer::BB_SINT64, int64_t);
                Array_POD(Bytebuffer::BB_UINT64, uint64_t);
                Array_POD(Bytebuffer::BB_FLOAT32, float);
                Array_POD(Bytebuffer::BB_FLOAT64, double);

                String(Bytebuffer::BB_ASCIISTRING, std::string);
                String(Bytebuffer::BB_UTF8STRING, std::u8string);
                Array_String(Bytebuffer::BB_ASCIISTRING, std::string);
                Array_String(Bytebuffer::BB_UTF8STRING, std::u8string);

                // No sane dev should use an array of blobs..
                case Bytebuffer::BB_BLOB:
                {
                    Output << "Blob_t = { " << String::toHEX(Reader.Read<Blob_t>(), true) << "}\n";
                    break;
                }

                default:
                    Output << ">>> Deserialization failed for typeID: " << TypeID << '\n';
                    goto LABEL_EXIT;
            }

            #undef POD
            #undef String
            #undef Array_POD
            #undef Array_String
        }

        LABEL_EXIT:
        Output << '}';
        return std::string(Output.str());
    }

    // Construct a non-owning buffer from common containers.
    Bytebuffer_t(const void *Buffer, size_t Size)
    {
        Internalbuffer = (const uint8_t *)Buffer;
        Internaliterator = 0;
        Internalsize = Size;
    }
    template <cmp::Range_t Range> Bytebuffer_t(const Range &Input)
    {
        const auto Validated = cmp::getBytes(Input);
        Internalbuffer = Validated.data();
        Internalsize = Validated.size();
        Internaliterator = 0;
    }

    // Construct a owning buffer.
    Bytebuffer_t() = default;
    explicit Bytebuffer_t(size_t Size)
    {
        const auto Newbuffer = (uint8_t *)malloc(Size);
        std::memset(Newbuffer, 0, Size);

        Internalbuffer = Newbuffer;
        Internaliterator = 0;
        Internalsize = Size;
    }

    // Copying a buffer takes a read-only view of it, prefer moving.
    explicit Bytebuffer_t(const Bytebuffer_t &Other) noexcept
    {
        Internalbuffer = Other.data();
        Internalsize = Other.Internalsize;
        Internaliterator = Other.Internaliterator;
    }
    Bytebuffer_t(Bytebuffer_t &&Other) noexcept
    {
        Internalbuffer.swap(Other.Internalbuffer);
        Internaliterator = Other.Internaliterator;
        Internalsize = Other.Internalsize;
    }

    // Cleanup for owning mode.
    ~Bytebuffer_t() noexcept
    {
        if (isOwning())
        {
            if (const auto Buffer = std::get<uint8_t *>(Internalbuffer))
            {
                free(Buffer);
            }
        }
    }
};

// Helper to serialize structs until we get reflection..
namespace Bytebuffer
{
    static Bytebuffer_t fromStruct(const auto &Object, bool Typechecked)
    {
        Bytebuffer_t Buffer{};

        // Visit all properties of the struct.
        cmp::Visitmembers(Object, [&](auto && ...Items) -> void
        {
            ((Buffer.Write(Items, Typechecked)), ...);
        });

        return Buffer;
    }
}

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    inline void Bytebuffertest()
    {
        Bytebuffer_t Buffer{};

        Buffer << uint32_t(0x2A);
        Buffer.Write(uint8_t(2));
        Buffer.Write(uint8_t(3), false);
        Buffer << "Hello";

        const auto Dump = Buffer.to_hex();
        if ("0B 2A 00 00 00 07 02 03 02 48 65 6C 6C 6F 00"s != Dump)
            std::printf("BROKEN: Bytebuffer writing\n");

        // Reset the read/write iterator.
        Buffer.Rewind();

        if (42 != Buffer.Read<uint32_t>()) std::printf("BROKEN: Bytebuffer reading\n");
        if (2 != Buffer.Read<uint8_t>()) std::printf("BROKEN: Bytebuffer reading\n");
        if (3 != Buffer.Read<uint8_t>(false)) std::printf("BROKEN: Bytebuffer reading\n");
        if ("Hello"s != Buffer.Read<std::string>()) std::printf("BROKEN: Bytebuffer reading\n");
    }
}
#endif
