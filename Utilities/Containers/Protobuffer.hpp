/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-20
    License: MIT

    A simple protobuf implementation.
*/

#pragma once
#include "Bytebuffer.hpp"

// Might as well reuse some plumbing for basic protobuffer support.
struct Protobuffer_t : Bytebuffer_t
{
    enum class Wiretype_t : uint8_t { VARINT, I64, STRING /* LEN */, I32 = 5, INVALID = 255 };
    uint32_t CurrentID{}; Wiretype_t Currenttype{};

    // Inherit constructors.
    using Bytebuffer_t::Bytebuffer_t;

    // Copying a buffer takes a read-only view of it, prefer moving.
    explicit Protobuffer_t(const Protobuffer_t &Other) noexcept : Bytebuffer_t(Other)
    {
        Currenttype = Other.Currenttype;
        CurrentID = Other.CurrentID;
    }
    Protobuffer_t(Protobuffer_t &&Other) noexcept : Bytebuffer_t(Other)
    {
        Currenttype = Other.Currenttype;
        CurrentID = Other.CurrentID;
    }

    // Encode as little endian.
    void EncodeI64(uint64_t Input)
    {
        Bytebuffer_t::Write<uint64_t>(cmp::toLittle(Input), false);
    }
    void EncodeI32(uint32_t Input)
    {
        Bytebuffer_t::Write<uint32_t>(cmp::toLittle(Input), false);
    }
    template <std::integral T> void EncodeVARINT(T Input)
    {
        std::array<uint8_t, 10> Buffer{};
        uint8_t Size = 0;

        // Little endian needed.
        Input = cmp::toLittle(Input);

        for (; Size < 10; Size++)
        {
            if (!Input) break;

            // MSB = continuation.
            Buffer[Size] = (Input & 0x7F) | 0x80;
            Input >>= 7;
        }

        // Need at least 1 byte.
        if (Size == 0) Size = 1;

        // Clear MSB.
        Buffer[Size - 1] &= 0x7F;

        rawWrite(Size, Buffer.data());
    }
    void EncodeSTRING(const std::u8string &Input)
    {
        EncodeVARINT(Input.size());
        rawWrite(Input.size(), Input.data());
    }

    // Decode to host endian.
    uint64_t DecodeI64()
    {
        const auto I64 = Bytebuffer_t::Read<uint64_t>(false);
        return cmp::fromLittle(I64);
    }
    uint32_t DecodeI32()
    {
        const auto I32 = Bytebuffer_t::Read<uint32_t>(false);
        return cmp::fromLittle(I32);
    }
    uint64_t DecodeVARINT()
    {
        uint64_t Value{};

        for (uint8_t i = 0; i < 10; ++i)
        {
            Value <<= 7;

            const auto Byte = Bytebuffer_t::Read<uint8_t>(false);
            Value |= (Byte & 0x7F);

            // No continuation bit.
            if (!(Byte & 0x80))
                break;
        }

        return cmp::fromLittle(Value);
    }
    std::u8string DecodeSTRING()
    {
        const auto Length = DecodeVARINT();
        ASSERT(Length < UINT32_MAX);

        std::u8string Result(Length, 0);
        rawRead(uint32_t(Length), Result.data());
        return Result;
    }

    // Sometimes the protocol wants ZigZag encoding over 2's compliment.
    template <std::integral T> T toZigZag(T Input)
    {
        return (Input >> 1) ^ -(Input & 1);
    }
    template <std::integral T> T fromZigZag(T Input)
    {
        constexpr size_t Bits = sizeof(T) * 8 - 1;
        return (Input >> Bits) ^ (Input << 1);
    }

    // Tags are silly things.
    void EncodeTAG(uint32_t ID, Wiretype_t Type)
    {
        const uint64_t Tag = (uint64_t(ID) << 3) | uint8_t(Type);
        EncodeVARINT(Tag);
    }
    std::pair<uint32_t, Wiretype_t> DecodeTAG()
    {
        const auto Tag = DecodeVARINT();

        // EOF
        if (Tag == 0) [[unlikely]]
        {
            Bytebuffer_t::Seek(0, SEEK_SET);
            return { 0, Wiretype_t::INVALID };
        }

        return { uint32_t(Tag >> 3), Wiretype_t(Tag & 7) };
    }

    // Seek tags.
    bool Seek(uint32_t ID)
    {
        // Early exit.
        if (ID == CurrentID && ID != 0)
            return true;

        // Seek from begining.
        if (ID < CurrentID)
        {
            CurrentID = 0;

            Bytebuffer_t::Seek(0, SEEK_SET);
            return Seek(ID);
        }

        // Seek forward.
        while(true)
        {
            std::tie(CurrentID, Currenttype) = DecodeTAG();

            if (Wiretype_t::INVALID == Currenttype) [[unlikely]] return false;
            if (ID == CurrentID) [[unlikely]] return true;

            // Skip the data.
            switch (Currenttype)
            {
                case Wiretype_t::VARINT: { (void)DecodeVARINT(); break; }
                case Wiretype_t::STRING: { (void)DecodeSTRING(); break; }
                case Wiretype_t::I64:    { (void)DecodeI64(); break; }
                case Wiretype_t::I32:    { (void)DecodeI32(); break; }
            }
        }
    }

    // Typed IO, need explicit type when writing, tries to convert when reading.
    template <typename T> void Write(T Value, Wiretype_t Type, uint32_t ID)
    {
        // User-specified serialization.
        if constexpr (requires { Value.Serialize(*this); }) return Value.Serialize(*this);

        // Special case of merging bytebuffers.
        if constexpr (std::is_same_v<T, Bytebuffer_t> || std::is_same_v<T, Protobuffer_t>)
        {
            // In-case we get a non-owning buffer, the caller needs to update the iterator.
            assert(Value.Internaliterator);

            rawWrite(std::min(Value.Internaliterator, Value.Internalsize), Value.data());
            return;
        }

        EncodeTAG(ID, Type);
        if (Type == Wiretype_t::VARINT) EncodeVARINT(Value);
        if (Type == Wiretype_t::STRING) EncodeSTRING(Value);
        if (Type == Wiretype_t::I64) EncodeI64(Value);
        if (Type == Wiretype_t::I32) EncodeI32(Value);
    }
    template <typename Type> bool Read(Type &Buffer, uint32_t ID)
    {
        // Lookup ID.
        if (!Seek(ID))
        {
            Errorprint(va("Protobuf tag %u not found", ID));
            return false;
        }

        if (Currenttype == Wiretype_t::VARINT)
        {
                 if constexpr (std::is_integral_v<Type>) Buffer = (Type)DecodeVARINT();
            else if constexpr (std::is_floating_point_v<Type>) Buffer = (Type)DecodeVARINT();
            else
            {
                Debugprint(va("Error: Protobuf tag %u type is VARINT", ID));
                return false;
            }

            return true;
        }

        if (Currenttype == Wiretype_t::I64)
        {
            if constexpr (std::is_same_v<Type, uint64_t>)
            {
                Buffer = DecodeI64();
                return true;
            }
            else
            {
                Debugprint(va("Error: Protobuf tag %u type is I64", ID));
                return false;
            }
        }

        if (Currenttype == Wiretype_t::I32)
        {
            if constexpr (std::is_same_v<Type, uint32_t>)
            {
                Buffer = DecodeI32();
                return true;
            }
            else
            {
                Debugprint(va("Error: Protobuf tag %u type is I32", ID));
                return false;
            }
        }

        if (Currenttype == Wiretype_t::STRING)
        {
            if constexpr (cmp::isDerived<Type, std::basic_string>)
            {
                const auto String = DecodeSTRING();
                     if constexpr (std::is_same_v<Type, std::u8string>) Buffer = String;
                else if constexpr (std::is_same_v<Type, std::string>) Buffer = Encoding::toASCII(String);
                else if constexpr (std::is_same_v<Type, std::wstring>) Buffer = Encoding::toUNICODE(String);
                else if constexpr (std::is_same_v<Type, Blob_t>) Buffer = { String.begin(), String.end() };
                else
                {
                    Debugprint(va("Error: Protobuf tag %u type is LEN, but could not convert it?", ID));
                    return false;
                }

                return true;
            }
            else
            {
                Debugprint(va("Error: Protobuf tag %u type is LEN", ID));
                return false;
            }
        }

        // WTF?
        ASSERT(false);
        return false;
    }
    template <typename Type> Type Read(uint32_t ID)
    {
        Type Result{};

        // User-specified serialization.
        if constexpr (requires { Result.Deserialize(*this); }) return Result.Deserialize(*this);

        Read(Result, ID);
        return Result;
    }
};
