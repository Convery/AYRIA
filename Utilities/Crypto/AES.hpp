/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-21
    License: MIT

    Constexpr implementation used as fallback if OpenSSL is not available.
    Defaults to CBC mode, other modes available as namespaces.
*/

#pragma once
#include <Utilities.hpp>
#include <intrin.h>

#if __has_include(<openssl/evp.h>)
#include <openssl/evp.h>
#endif

namespace AES
{
    enum Mode_t : uint8_t
    {
        AES_ECB,
        AES_CBC,
        AES_CFB,

        AES_XEX,
        AES_XTS,

        AES_CTR_32BE,
        AES_CTR_32LE,
        AES_CTR_64BE,
        AES_CTR_64LE,
        AES_CTR_128BE,
        AES_CTR_128LE,

        // Last unique mode.
        AES_MAX,

        // Aliases for default modes.
        AES_CTR = AES_CTR_32BE
    };
}

namespace AES::Implementation
{
    // Core algorithm.
    namespace HW
    {
        using Block_t = __m128i;

        // _mm_aeskeygenassist_si128 requires the RCON to be resolved at compiletime and MSVC has issues figuring it out.
        constexpr std::array<uint8_t, 10> Roundconstants = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };
        inline Block_t Keygenassist(const Block_t &Input, uint8_t i)
        {
            switch (i)
            {
                case 0: return _mm_aeskeygenassist_si128(Input, Roundconstants[0]);
                case 1: return _mm_aeskeygenassist_si128(Input, Roundconstants[1]);
                case 2: return _mm_aeskeygenassist_si128(Input, Roundconstants[2]);
                case 3: return _mm_aeskeygenassist_si128(Input, Roundconstants[3]);
                case 4: return _mm_aeskeygenassist_si128(Input, Roundconstants[4]);
                case 5: return _mm_aeskeygenassist_si128(Input, Roundconstants[5]);
                case 6: return _mm_aeskeygenassist_si128(Input, Roundconstants[6]);
                case 7: return _mm_aeskeygenassist_si128(Input, Roundconstants[7]);
                case 8: return _mm_aeskeygenassist_si128(Input, Roundconstants[8]);
                case 9: return _mm_aeskeygenassist_si128(Input, Roundconstants[9]);
            }

            std::unreachable();
        }

        // Derive the keys for the rounds.
        template <uint8_t Keysize, uint8_t Rounds> std::array<Block_t, Rounds + 1> Keyexpansion(const std::array<uint8_t, Keysize * 4> &Key)
        {
            std::array<Block_t, Rounds + 1> Schedule{};
            cmp::memcpy(Schedule.data(), Key.data(), Keysize * 4);

            // AES-128
            if constexpr (Keysize == 4 && Rounds == 10)
            {
                for (uint8_t i = 0; i < Rounds; ++i)
                {
                    const auto A = _mm_shuffle_epi32(Keygenassist(Schedule[i], i), _MM_SHUFFLE(3, 3, 3, 3));

                    auto T = Schedule[i];
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(A, T);

                    Schedule[i + 1] = T;
                }
            }

            // AES-192
            if constexpr (Keysize == 6 && Rounds == 12)
            {
                for (uint8_t i = 1; i < Rounds; ++i)
                {
                    const auto A = _mm_shuffle_epi32(Keygenassist(Schedule[i], i / 2), _MM_SHUFFLE(1, 1, 1, 1));

                    auto T = Schedule[i - 1];
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(A, T);

                    const auto B = _mm_shuffle_epi32(T, _MM_SHUFFLE(3, 3, 3, 3));

                    auto U = Schedule[i];
                    U = _mm_xor_si128(U, _mm_slli_si128(U, 4));
                    U = _mm_xor_si128(B, U);

                    if (i == 12)
                    {
                        Schedule[i] = T;
                    }
                    else
                    {
                        Schedule[i] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Schedule[i]), _mm_castsi128_pd(T), 0));
                        Schedule[i + 1] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(T), _mm_castsi128_pd(U), 1));
                    }
                }
            }

            // AES-256
            if constexpr (Keysize == 8 && Rounds == 14)
            {
                for (uint8_t i = 1; i < Rounds; ++i)
                {
                    const auto A = (i & 1) ?
                        _mm_shuffle_epi32(Keygenassist(Schedule[i], i / 2), _MM_SHUFFLE(3, 3, 3, 3)) :
                        _mm_shuffle_epi32(_mm_aeskeygenassist_si128(Schedule[i], 0), _MM_SHUFFLE(2, 2, 2, 2));

                    auto T = Schedule[i - 1];
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(T, _mm_slli_si128(T, 4));
                    T = _mm_xor_si128(A, T);

                    Schedule[i + 1] = T;
                }
            }

            return Schedule;
        }
        template <uint8_t Keysize, uint8_t Rounds> std::array<Block_t, Rounds + 1> INVKeyexpansion(const std::array<uint8_t, Keysize * 4> &Key)
        {
            const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
            std::array<Block_t, Rounds + 1> Result{};

            for (uint8_t i = 1; i < Rounds + 1; ++i)
                Result[i] = _mm_aesimc_si128(Keys[Rounds - i]);

            Result[0] = Keys[Rounds];
            Result[Rounds] = Keys[0];
            return Result;
        }

        // The actual algorithm, note that decryption uses inv-mixed keys.
        template <uint8_t Rounds> Block_t Encryptblock(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // Add round key.
            auto Block = _mm_xor_si128(Input, Keys[0]);

            for (uint8_t i = 1; i < Rounds; ++i)
                Block = _mm_aesenc_si128(Block, Keys[i]);

            return _mm_aesenclast_si128(Block, Keys[Rounds]);
        }
        template <uint8_t Rounds> Block_t Decryptblock(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // Add round key.
            auto Block = _mm_xor_si128(Input, Keys[0]);

            for (uint8_t i = 1; i < Rounds; ++i)
                Block = _mm_aesdec_si128(Block, Keys[i]);

            return _mm_aesdeclast_si128(Block, Keys[Rounds]);
        }
    }
    namespace Portable
    {
        using Block_t = std::array<uint32_t, 4>;

        // Multiplication over GF(2^8)
        constexpr uint8_t Mul2(uint8_t Input)
        {
            return (Input << 1) ^ ((Input >> 7) * uint8_t(0x1B));
        }
        constexpr uint8_t Mul3(uint8_t Input)
        {
            return Mul2(Input) ^ Input;
        }
        constexpr uint8_t Div3(uint8_t Input)
        {
            // Equivalent to Mul246
            Input ^= Input << 1;
            Input ^= Input << 2;
            Input ^= Input << 4;
            return Input ^ ((Input >> 7) * uint8_t(0x09));
        }
        constexpr uint32_t Mul2(uint32_t Input)
        {
            return ((Input & 0x7F7F7F7F) << 1) ^ ((Input & 0x80808080) >> 7) * 0x1B;
        }
        constexpr uint32_t Mul3(uint32_t Input)
        {
            return ((Input & 0x3F3F3F3F) << 2) ^ ((Input & 0x80808080) >> 7) * 0x36 ^ ((Input & 0x40404040) >> 6) * 0x1B;
        }

        // Plumbing for 128-bit blocks.
        constexpr Block_t XOR(const Block_t &A, const Block_t &B)
        {
            // _mm_xor_si128(A, B)
            return Block_t{ A[0] ^ B[0], A[1] ^ B[1], A[2] ^ B[2], A[3] ^ B[3] };
        }
        constexpr Block_t Shift4(const Block_t &Input)
        {
            // _mm_slli_si128(Input, 4)
            return Block_t { 0, Input[0], Input[1], Input[2] };
        }
        constexpr Block_t Shuffle4(const Block_t &Input, uint8_t Control)
        {
            // _mm_shuffle_epi32(Input, Control)
            return Block_t
            {
                Input[(Control & 0x03) >> 0],
                Input[(Control & 0x0C) >> 2],
                Input[(Control & 0x30) >> 4],
                Input[(Control & 0xC0) >> 6]
            };
        }

        // Generate the substitution-boxes at compiletime.
        constexpr std::array<uint8_t, 256> SBox = []()
        {
            std::array<uint8_t, 256> Buffer{};
            uint8_t P = 1, Q = 1;

            do
            {
                // P * Q == 1
                P = Mul3(P); Q = Div3(Q);

                // Affine transform.
                Buffer[P] = 0x63 ^ (Q ^ std::rotl(Q, 1) ^ std::rotl(Q, 2) ^ std::rotl(Q, 3) ^ std::rotl(Q, 4));

            } while (P != 1);

            // First element has no inverse.
            Buffer[0] = 0x63;

            return Buffer;
        }();
        constexpr std::array<uint8_t, 256> INVSBox = []()
        {
            std::array<uint8_t, 256> Buffer{};

            for (uint32_t i = 0; i < 256; ++i)
                Buffer[SBox[i]] = uint8_t(i);

            return Buffer;
        }();

        // Substitute bytes from the box.
        constexpr uint32_t Substitute(uint32_t Word)
        {
            auto Temp = std::bit_cast<std::array<uint8_t, 4>>(Word);
            Temp[0] = SBox[Temp[0]]; Temp[1] = SBox[Temp[1]];
            Temp[2] = SBox[Temp[2]]; Temp[3] = SBox[Temp[3]];
            return std::bit_cast<uint32_t>(Temp);
        }
        constexpr uint32_t INVSubstitute(uint32_t Word)
        {
            auto Temp = std::bit_cast<std::array<uint8_t, 4>>(Word);
            Temp[0] = INVSBox[Temp[0]]; Temp[1] = INVSBox[Temp[1]];
            Temp[2] = INVSBox[Temp[2]]; Temp[3] = INVSBox[Temp[3]];
            return std::bit_cast<uint32_t>(Temp);
        }
        constexpr Block_t Substitute(const Block_t &Input)
        {
            return { Substitute(Input[0]), Substitute(Input[1]), Substitute(Input[2]), Substitute(Input[3]) };
        }
        constexpr Block_t INVSubstitute(const Block_t &Input)
        {
            return { INVSubstitute(Input[0]), INVSubstitute(Input[1]), INVSubstitute(Input[2]), INVSubstitute(Input[3]) };
        }

        // Shiftrow the rows.
        constexpr Block_t Shiftrow(const Block_t &Input)
        {
            const auto asBytes = std::bit_cast<std::array<std::array<uint8_t, 4>, 4>>(Input);
            const std::array<uint8_t, 16> Result =
            {
                asBytes[0][0], asBytes[1][1], asBytes[2][2], asBytes[3][3],
                asBytes[1][0], asBytes[2][1], asBytes[3][2], asBytes[0][3],
                asBytes[2][0], asBytes[3][1], asBytes[0][2], asBytes[1][3],
                asBytes[3][0], asBytes[0][1], asBytes[1][2], asBytes[2][3]
            };

            return std::bit_cast<Block_t>(Result);
        }
        constexpr Block_t INVShiftrow(const Block_t &Input)
        {
            const auto asBytes = std::bit_cast<std::array<std::array<uint8_t, 4>, 4>>(Input);
            const std::array<uint8_t, 16> Result =
            {
                asBytes[0][0], asBytes[3][1], asBytes[2][2], asBytes[1][3],
                asBytes[1][0], asBytes[0][1], asBytes[3][2], asBytes[2][3],
                asBytes[2][0], asBytes[1][1], asBytes[0][2], asBytes[3][3],
                asBytes[3][0], asBytes[2][1], asBytes[1][2], asBytes[0][3]
            };

            return std::bit_cast<Block_t>(Result);
        }

        // Mix the columns.
        constexpr uint32_t Mix(uint32_t Word)
        {
            const auto Temp = Mul2(Word) ^ std::rotr(Word, 16);
            return Temp ^ std::rotr(Word ^ Temp, 8);
        }
        constexpr uint32_t INVMix(uint32_t Word)
        {
            const auto Temp = Mul3(Word);
            return Mix(Word ^ Temp ^ std::rotr(Temp, 16));
        }
        constexpr Block_t Mix(const Block_t &Input)
        {
            return { Mix(Input[0]), Mix(Input[1]), Mix(Input[2]), Mix(Input[3]) };
        }
        constexpr Block_t INVMix(const Block_t &Input)
        {
            return { INVMix(Input[0]), INVMix(Input[1]), INVMix(Input[2]), INVMix(Input[3]) };
        }

        // Replacement for _mm_aeskeygenassist_si128
        constexpr std::array<uint8_t, 10> Roundconstants = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };
        constexpr Block_t Keygenassist(const Block_t &Input, uint8_t i)
        {
            const auto RCON = uint32_t(Roundconstants[i]);

            return Block_t
            {
                Substitute(Input[1]),
                std::rotr(Substitute(Input[1]), 8) ^ RCON,
                Substitute(Input[3]),
                std::rotr(Substitute(Input[3]), 8) ^ RCON
            };
        }

        // Derive the keys for the rounds.
        template <uint8_t Keysize, uint8_t Rounds> std::array<Block_t, Rounds + 1> constexpr Keyexpansion(const std::array<uint8_t, Keysize * 4> &Key)
        {
            std::array<Block_t, Rounds + 1> Schedule{};
            cmp::memcpy(Schedule.data(), Key.data(), Keysize * 4);

            // AES-128
            if constexpr (Keysize == 4 && Rounds == 10)
            {
                for (uint8_t i = 0; i < Rounds; ++i)
                {
                    const auto A = Shuffle4(Keygenassist(Schedule[i], i), 0xFF);

                    auto T = Schedule[i];
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(A, T);

                    Schedule[i + 1] = T;
                }
            }

            // AES-192
            if constexpr (Keysize == 6 && Rounds == 12)
            {
                for (uint8_t i = 1; i < Rounds; ++i)
                {
                    const auto A = Shuffle4(Keygenassist(Schedule[i], i / 2), 0x55);

                    auto T = Schedule[i - 1];
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(A, T);

                    const auto B = Shuffle4(T, 0xFF);

                    auto U = Schedule[i];
                    U = XOR(U, Shift4(U));
                    U = XOR(B, U);

                    if (i == 12)
                    {
                        Schedule[i] = T;
                    }
                    else
                    {
                        // Schedule[i] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Schedule[i]), _mm_castsi128_pd(T), 0));
                        Schedule[i][0] = Schedule[i][0];
                        Schedule[i][1] = Schedule[i][1];
                        Schedule[i][2] = T[0];
                        Schedule[i][3] = T[1];

                        // Schedule[i + 1] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(T), _mm_castsi128_pd(U), 1));
                        Schedule[i + 1][0] = T[2];
                        Schedule[i + 1][1] = T[3];
                        Schedule[i + 1][2] = U[0];
                        Schedule[i + 1][3] = U[1];
                    }
                }
            }

            // AES-256
            if constexpr (Keysize == 8 && Rounds == 14)
            {
                for (uint8_t i = 1; i < Rounds; ++i)
                {
                    const auto A = (i & 1) ?
                        Shuffle4(Keygenassist(Schedule[i], i / 2), 0xFF) :
                        Shuffle4(Keygenassist(Schedule[i], 0), 0xAA);

                    auto T = Schedule[i - 1];
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(T, Shift4(T));
                    T = XOR(A, T);

                    Schedule[i + 1] = T;
                }
            }

            return Schedule;
        }
        template <uint8_t Keysize, uint8_t Rounds> std::array<Block_t, Rounds + 1> constexpr INVKeyexpansion(const std::array<uint8_t, Keysize * 4> &Key)
        {
            const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
            std::array<Block_t, Rounds + 1> Result{};

            for (uint8_t i = 1; i < Rounds + 1; ++i)
                Result[i] = INVMix(Keys[Rounds - i]);

            Result[0] = Keys[Rounds];
            Result[Rounds] = Keys[0];
            return Result;
        }

        // The actual algorithm, note that decryption uses inv-mixed keys.
        template <uint8_t Rounds> constexpr Block_t Encryptblock(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // Add round key.
            auto Block = XOR(Input, Keys[0]);

            for (uint8_t i = 1; i < Rounds; ++i)
            {
                Block = Mix(Substitute(Shiftrow(Block)));
                Block = XOR(Block, Keys[i]);
            }

            Block = Substitute(Shiftrow(Block));
            Block = XOR(Block, Keys[Rounds]);

            return Block;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // Add round key.
            auto Block = XOR(Input, Keys[0]);

            for (uint8_t i = 1; i < Rounds; ++i)
            {
                Block = INVMix(INVSubstitute(INVShiftrow(Block)));
                Block = XOR(Block, Keys[i]);
            }

            Block = INVSubstitute(INVShiftrow(Block));
            Block = XOR(Block, Keys[Rounds]);

            return Block;
        }
    }

    // Modes.
    namespace HW
    {
        inline Block_t XOR(const Block_t &A, const Block_t &B)
        {
            return _mm_xor_si128(A, B);
        }
        inline Block_t Mul128(const Block_t &Value)
        {
            Block_t Result{ Value };
            uint8_t Carry{};

            for (uint8_t i = 0; i < 16; ++i)
            {
                const auto Temp = uint8_t(Result.m128i_i8[i] & 0x80);
                Result.m128i_i8[i] = uint8_t(Result.m128i_i8[i] << 1);
                Result.m128i_i8[i] |= Carry;
                Carry = Temp >> 7;
            }

            Carry = uint8_t(0 - Carry);
            Result.m128i_i8[0] ^= uint8_t(0x87 & Carry);

            return Result;
        }

        // No initial vector, unsafe.
        template <uint8_t Rounds> constexpr Block_t Encryptblock_ECB(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            return Encryptblock<Rounds>(Input, Keys);
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_ECB(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            return Decryptblock<Rounds>(Input, Keys);
        }

        // State = Initialvector
        template <uint8_t Rounds> constexpr Block_t Encryptblock_CBC(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = Encryptblock<Rounds>(XOR(State, Input), Keys);
            State = Temp;
            return Temp;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_CBC(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(State, Decryptblock<Rounds>(Input, Keys));
            State = Input;
            return Temp;
        }

        // State = Initialvector
        template <uint8_t Rounds> constexpr Block_t Encryptblock_CFB(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            State = Temp;
            return Temp;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_CFB(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            State = Input;
            return Temp;
        }

        // State = None | Counter
        template <uint8_t Rounds, size_t Countersize, bool Bigendian> constexpr Block_t Encryptblock_CTR(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            auto NC = std::bit_cast<std::array<uint8_t, 16>>(State);

            // Increment the counter for the nex round.
            if constexpr (Bigendian)
            {
                NC[15] += 1;
                for (uint8_t i = 0; i < (Countersize / 8); ++i)
                    if (NC[15 - i] == 0) NC[14 - i] += 1;
            }
            else
            {
                NC[0] += 1;
                for (uint8_t i = 0; i < (Countersize / 8); ++i)
                    if (NC[i] == 0) NC[i + 1] += 1;
            }

            State = std::bit_cast<Block_t>(NC);
            return Temp;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_CTR(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // CTR just runs the same operation again to decrypt.
            return Encryptblock_CTR<Rounds>(State, Input, Keys);
        }

        // State = Block
        template <uint8_t Rounds> constexpr Block_t Encryptblock_XEX(uint64_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys, const std::array<Block_t, Rounds + 1> &Tweakkeys)
        {
            const auto Tweak = Encryptblock<Rounds>(State, Tweakkeys);
            const auto Temp = Encryptblock<Rounds>(XOR(Input, Tweak), Keys);
            const auto Result = XOR(Temp, Tweak);
            State = Mul128(Tweak);
            return Result;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_XEX(uint64_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys, const std::array<Block_t, Rounds + 1> &Tweakkeys)
        {
            const auto Tweak = Encryptblock<Rounds>(State, Tweakkeys);
            const auto Temp = Decryptblock<Rounds>(XOR(Input, Tweak), Keys);
            const auto Result = XOR(Temp, Tweak);
            State = Mul128(Tweak);
            return Result;
        }
    }
    namespace Portable
    {
        // GF(2^128) multiplication by alpha.
        constexpr Block_t Mul128(const Block_t &Value)
        {
            Block_t Result{ Value };
            uint8_t Carry{};

            for (int8_t i = 15; i >= 0; --i)
            {
                const auto Temp = uint8_t(Result[i] & 0x80);
                Result[i] = uint8_t(Result[i] << 1) | Carry;
                Carry = Temp >> 7;
            }

            if (Carry)
                Result[0] ^= 0x87;

            return Result;
        }

        // No initial vector, unsafe.
        template <uint8_t Rounds> constexpr Block_t Encryptblock_ECB(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            return Encryptblock<Rounds>(Input, Keys);
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_ECB(const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            return Decryptblock<Rounds>(Input, Keys);
        }

        // State = Initialvector
        template <uint8_t Rounds> constexpr Block_t Encryptblock_CBC(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = Encryptblock<Rounds>(XOR(State, Input), Keys);
            State = Temp;
            return Temp;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_CBC(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(State, Decryptblock<Rounds>(Input, Keys));
            State = Input;
            return Temp;
        }

        // State = Initialvector
        template <uint8_t Rounds> constexpr Block_t Encryptblock_CFB(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            State = Temp;
            return Temp;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_CFB(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            State = Input;
            return Temp;
        }

        // State = None | Counter
        template <uint8_t Rounds, size_t Countersize, bool Bigendian> constexpr Block_t Encryptblock_CTR(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = XOR(Input, Encryptblock<Rounds>(State, Keys));
            auto NC = std::bit_cast<std::array<uint8_t, 16>>(State);

            // Increment the counter for the nex round.
            if constexpr (Bigendian)
            {
                NC[15] += 1;
                for (uint8_t i = 0; i < (Countersize / 8); ++i)
                    if (NC[15 - i] == 0) NC[14 - i] += 1;
            }
            else
            {
                NC[0] += 1;
                for (uint8_t i = 0; i < (Countersize / 8); ++i)
                    if (NC[i] == 0) NC[i + 1] += 1;
            }

            State = std::bit_cast<Block_t>(NC);
            return Temp;
        }
        template <uint8_t Rounds, size_t Countersize, bool Bigendian> constexpr Block_t Decryptblock_CTR(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            // CTR just runs the same operation again to decrypt.
            return Encryptblock_CTR<Rounds, Countersize, Bigendian>(State, Input, Keys);
        }

        // State = Block
        template <uint8_t Rounds> constexpr Block_t Encryptblock_XEX(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = Encryptblock<Rounds>(XOR(Input, State), Keys);
            const auto Result = XOR(Temp, State);
            State = Mul128(State);
            return Result;
        }
        template <uint8_t Rounds> constexpr Block_t Decryptblock_XEX(Block_t &State, const Block_t &Input, const std::array<Block_t, Rounds + 1> &Keys)
        {
            const auto Temp = Decryptblock<Rounds>(XOR(Input, State), Keys);
            const auto Result = XOR(Temp, State);
            State = Mul128(State);
            return Result;
        }
    }

    // Select implementation at runtime.
    inline bool hasIntrinsics()
    {
        #if defined(_MSC_VER)
        std::array<int, 4> CPUID{};
        __cpuid(CPUID.data(), 1);
        return (CPUID[2] & (1 << 25)) != 0;

        #else

        std::array<unsigned int, 4> CPUID{};
        __get_cpuid(1, &CPUID[0], &CPUID[1], &CPUID[2], &CPUID[3]);
        return (CPUID[2] & (1 << 25)) != 0;
        #endif
    }
}

namespace AES::Modes
{
    using namespace AES::Implementation;

    // GF(2^128) multiplication by alpha (2).
    constexpr Portable::Block_t MUL128(const Portable::Block_t &Input)
    {
        auto Result = std::bit_cast<std::array<uint8_t, 16>>(Input);
        const uint8_t Carry{ uint8_t(Result[15] >> 7) };

        for (int8_t i = 15; i > 0; --i)
            Result[i] = (Result[i] << 1) | (Result[i - 1] >> 7);

        Result[0] = (Result[0] << 1) ^ (Carry * 0x87);
        return std::bit_cast<Portable::Block_t>(Result);
    }
    constexpr HW::Block_t MUL128(const HW::Block_t &Input)
    {
        const uint8_t Carry{ uint8_t(Input.m128i_i8[15] >> 7) };
        auto Result = Input;

        for (int8_t i = 15; i > 0; --i)
            Result.m128i_i8[i] = (Result.m128i_i8[i] << 1) | (Result.m128i_i8[i - 1] >> 7);

        Result.m128i_i8[0] = (Result.m128i_i8[0] << 1) ^ (Carry * 0x87);
        return Result;
    }
}

// WIP
#if 0
namespace AES::Modes
{
    using namespace AES::Implementation;

    // GF(2^128) multiplication by alpha (2).

    constexpr HW::Block_t Mul128(const HW::Block_t &Input)
    {
        HW::Block_t Result{ Input };
        uint8_t Carry{};

        for (int8_t i = 15; i >= 0; --i)
        {
            const auto Temp = uint8_t(Result.m128i_i8[i] & 0x80);
            Result.m128i_i8[i] = uint8_t(Result.m128i_i8[i] << 1) | Carry;
            Carry = Temp >> 7;
        }

        if (Carry)
            Result.m128i_i8[0] ^= 0x87;

        return Result;
    }

    // Increment the counter for the next round.
    template <size_t Countersize, bool Bigendian> constexpr Portable::Block_t Increment(const Portable::Block_t &Input)
    {
        auto NC = std::bit_cast<std::array<uint8_t, 16>>(Input);

        // Increment the counter for the nex round.
        if constexpr (Bigendian)
        {
            NC[15] += 1;
            for (uint8_t i = 0; i < (Countersize / 8); ++i)
                if (NC[15 - i] == 0) NC[14 - i] += 1;
        }
        else
        {
            NC[0] += 1;
            for (uint8_t i = 0; i < (Countersize / 8); ++i)
                if (NC[i] == 0) NC[i + 1] += 1;
        }

        return std::bit_cast<Portable::Block_t>(NC);
    }
    template <size_t Countersize, bool Bigendian> constexpr HW::Block_t Increment(const HW::Block_t &Input)
    {
        auto NC = std::bit_cast<std::array<uint8_t, 16>>(Input);

        // Increment the counter for the nex round.
        if constexpr (Bigendian)
        {
            NC[15] += 1;
            for (uint8_t i = 0; i < (Countersize / 8); ++i)
                if (NC[15 - i] == 0) NC[14 - i] += 1;
        }
        else
        {
            NC[0] += 1;
            for (uint8_t i = 0; i < (Countersize / 8); ++i)
                if (NC[i] == 0) NC[i + 1] += 1;
        }

        return std::bit_cast<HW::Block_t>(NC);
    }

    // Input-size must be a multiple of 16.
    namespace Unpadded
    {
        // No initial vector, unsafe.
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_ECB)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Encryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Encryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_ECB)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Decryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Decryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
        }

        // Stream-block encryption.
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_CBC)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = Encryptblock<Rounds>(XOR(State, Block), Keys);
                    cmp::memcpy(&Buffer[i], State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = Encryptblock<Rounds>(XOR(State, Block), Keys);
                    cmp::memcpy(&Buffer[i], State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_CFB)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = XOR(Block, Encryptblock<Rounds>(State, Keys));
                    cmp::memcpy(&Buffer[i], State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = XOR(Block, Encryptblock<Rounds>(State, Keys));
                    cmp::memcpy(&Buffer[i], State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_CBC)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(State, Decryptblock<Rounds>(Block, Keys)));
                    State = Block;
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(State, Decryptblock<Rounds>(Block, Keys)));
                    State = Block;
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_CFB)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                    State = Block;
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                    State = Block;
                }

                return Buffer;
            }
        }

        // Parallelizable encryption, implemented sequentially.

        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode >= AES_CTR_32BE && Mode <= AES_CTR_128LE)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                return Buffer;
            }
        }

        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode >= AES_CTR_32BE && Mode <= AES_CTR_128LE)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::array<uint8_t, N> Buffer{};

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                return Buffer;
            }
        }





        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_XEX)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, uint64_t SectorID, uint64_t BlockID = 1)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_XTS)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, Keysize * 4> &Tweakkey, uint64_t SectorID, uint64_t BlockID = 0)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }

        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_XEX)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, uint64_t SectorID, uint64_t BlockID = 1)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (N % 16 == 0 && N != std::dynamic_extent && Mode == AES_XTS)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, Keysize * 4> &Tweakkey, uint64_t SectorID, uint64_t BlockID = 0)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::array<uint8_t, N> Buffer{};
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
    }

    // PKCS#7 padding.
    namespace Padded
    {
        // No initial vector, unsafe.
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_ECB)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Encryptblock<Rounds>(Block, Keys));
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], Encryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Encryptblock<Rounds>(Block, Keys));
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], Encryptblock<Rounds>(Block, Keys));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_ECB)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key)
        {
            // Can't decrypt the last block properly.
            assert((Input.size() & 15) == 0);

            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Decryptblock<Rounds>(Block, Keys));
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], Decryptblock<Rounds>(Block, Keys));
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
        }

        // Stream-block encryption.
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_CBC)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = Encryptblock<Rounds>(XOR(State, Block), Keys);
                    cmp::memcpy(&Buffer[i], State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], Encryptblock<Rounds>(XOR(State, Block), Keys));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = Encryptblock<Rounds>(XOR(State, Block), Keys);
                    cmp::memcpy(&Buffer[i], State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], Encryptblock<Rounds>(XOR(State, Block), Keys));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_CFB)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = XOR(Block, Encryptblock<Rounds>(State, Keys));
                    cmp::memcpy(&Buffer[i], State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    State = XOR(Block, Encryptblock<Rounds>(State, Keys));
                    cmp::memcpy(&Buffer[i], State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_CBC)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            // Can't decrypt the last block properly.
            assert((Input.size() & 15) == 0);

            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(State, Decryptblock<Rounds>(Block, Keys)));
                    State = Block;
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(State, Decryptblock<Rounds>(Block, Keys)));
                    State = Block;
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_CFB)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            // Can't decrypt the last block properly.
            assert((Input.size() & 15) == 0);

            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                    State = Block;
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                    State = Block;
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
        }

        // Parallelizable encryption, implemented sequentially.
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_XEX)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, uint64_t SectorID, uint64_t BlockID = 1)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Encryptblock<Rounds>(XOR(Block, State), Keys), State));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Encryptblock<Rounds>(XOR(Block, State), Keys), State));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode >= AES_CTR_32BE && Mode <= AES_CTR_128LE)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size() + Padding);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                // Last block.
                {
                    auto Block = std::bit_cast<Block_t>(std::array<uint8_t, 16>
                    {
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding,
                        Padding, Padding, Padding, Padding
                    });
                    cmp::memcpy(&Block, &Input[Input.size() - (Input.size() & 15)], (Input.size() & 15));
                    cmp::memcpy(&Buffer[Input.size() - (Input.size() & 15)], XOR(Block, Encryptblock<Rounds>(State, Keys)));
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_XEX)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, uint64_t SectorID, uint64_t BlockID = 1)
        {
            // Can't decrypt the last block properly.
            assert((Input.size() & 15) == 0);

            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Keys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode >= AES_CTR_32BE && Mode <= AES_CTR_128LE)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, 16> &IV)
        {
            // Can't decrypt the last block properly.
            assert((Input.size() & 15) == 0);

            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                Block_t State = std::bit_cast<Block_t>(IV);
                std::vector<uint8_t> Buffer(Input.size());

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));
                    cmp::memcpy(&Buffer[i], XOR(Block, Encryptblock<Rounds>(State, Keys)));

                    if constexpr (Mode == AES_CTR_32BE) State = Increment<32, true>(State);
                    if constexpr (Mode == AES_CTR_64BE) State = Increment<64, true>(State);
                    if constexpr (Mode == AES_CTR_128BE) State = Increment<128, true>(State);

                    if constexpr (Mode == AES_CTR_32LE) State = Increment<32, false>(State);
                    if constexpr (Mode == AES_CTR_64LE) State = Increment<64, false>(State);
                    if constexpr (Mode == AES_CTR_128LE) State = Increment<128, false>(State);
                }

                // Remove padding.
                Buffer.resize(Buffer.size() - Buffer.back());

                return Buffer;
            }
        }


        /*
            XTS standard enc.

            // SingleblockEnc
            T = ENC(K2, SectionID) * for 0..BlockID *= Alpha
            PP = Plain ^ T
            CC = ENC(K1, PP)
            C = CC ^ T

            // Full
            for block 0..N-1
                C = SingleblockEnc ..
            b = 16 - sizeof(block N)

            if (b == 0)
                C_N-1 = SingleblockEnc N-1
                C_N = 0
            else
                CC = SingleblockEnc N-1
                C_N = first b bits of CC
                CP = last 128-b bits of CC
                PP = N | CP
                C_N-1 = SingleblockEnc PP

            ret C | C_N-1 | C_N



            XTS standard dec.

            // SingleblockDec
            T = ENC(K2, SectionID) * for 0..BlockID *= Alpha
            CC = Cipher ^ T
            PP = DEC(K1, PP)
            P = PP ^ T

            // Full
            for block 0..N-1
                P = SingleblockDec ..

            b = 16 - sizeof(block N)
            if (b == 0)
                P_N-1 = SingleblockDec N-1
                P_N = 0
            else
                PP = SingleblockDec N
                P_N = first b bits of PP
                CP = last 128-b bits of PP
                CC = N | CP
                P_N-1 = SingleblockDec CC

        */


        // XEX takes a section and counter as IV. Sector << 32 | Counter.
        // XTS takes a section and then multiplies in GF



        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_XTS)
        constexpr std::array<uint8_t, N> Encrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, Keysize * 4> &Tweakkey, uint64_t SectorID, uint64_t BlockID = 0)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;
				const uint8_t Padding = 16 - (Input.size() & 15);

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size() + Padding);
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Encryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
        template <Mode_t Mode, uint8_t Keysize, uint8_t Rounds, template <typename, auto> typename U, cmp::Byte_t T, auto N> requires (Mode == AES_XTS)
        constexpr std::array<uint8_t, N> Decrypt(const U<T, N> &Input, const std::array<uint8_t, Keysize * 4> &Key, const std::array<uint8_t, Keysize * 4> &Tweakkey, uint64_t SectorID, uint64_t BlockID = 0)
        {
            if (std::is_constant_evaluated() || !hasIntrinsics())
            {
                using namespace Portable;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State[0], SectorID);
                else cmp::memcpy(&State[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
            else
            {
                using namespace HW;

                const auto Tweakkeys = Keyexpansion<Keysize, Rounds>(Tweakkey);
                const auto Keys = Keyexpansion<Keysize, Rounds>(Key);
                std::vector<uint8_t> Buffer(Input.size());
                Block_t State{};

                if constexpr (std::endian::native == std::endian::little) cmp::memcpy(&State.m128i_i8[0], SectorID);
                else cmp::memcpy(&State.m128i_i8[7], SectorID);

                State = Encryptblock<Rounds>(State, Tweakkeys);
                for(; BlockID; BlockID--) State = Mul128(State);

                // Regular blocks.
				for (size_t i = 0; i < Input.size(); i += 16)
                {
                    Block_t Block{}; cmp::memcpy(&Block, &Input[i], sizeof(Block_t));

                    const auto Temp = Decryptblock<Rounds>(XOR(Block, State), Keys);
                    cmp::memcpy(&Buffer[i], XOR(Temp, State));
                    State = Mul128(State);
                }

                return Buffer;
            }
        }
    }

    // Default mode, unpadded CBC.
}


#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    // Random data.
    constexpr std::array<uint8_t, 16> AESData{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    constexpr std::array<uint8_t, 16> AESIV{ 0x16, 0x15, 0x7e, 0x2b, 0xa6, 0xd2, 0xae, 0x28, 0x88, 0x15, 0xf7, 0xab, 0x3c, 0x4f, 0xcf, 0x09 };
    constexpr std::array<uint8_t, 16> AES128Key{ 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    constexpr std::array<uint8_t, 24> AES192Key{ 0xe8, 0x69, 0xc2, 0x0c, 0xdc, 0xf7, 0x78, 0xa6, 0x44, 0x41, 0x44, 0x62, 0x81, 0x1b, 0xd4, 0xb1, 0x1d, 0xd0, 0x03, 0xa1, 0x32, 0x7f, 0xc2, 0x30 };
    constexpr std::array<uint8_t, 32> AES256Key{ 0x88, 0x05, 0x79, 0xbd, 0x7c, 0x1f, 0x39, 0x5b, 0xad, 0x10, 0x85, 0xac, 0xc1, 0xbf, 0x4f, 0x35, 0xb8, 0xa7, 0xc1, 0x7f, 0x1a, 0x00, 0x99, 0xa5, 0x49, 0x3b, 0x3a, 0x1, 0x81, 0xb0, 0x26, 0xdc };

    constexpr auto ENC_AES128 = AES::Unpadded::Encrypt128(AESData, AES128Key, AESIV);
    constexpr auto ENC_AES192 = AES::Unpadded::Encrypt192(AESData, AES192Key, AESIV);
    constexpr auto ENC_AES256 = AES::Unpadded::Encrypt256(AESData, AES256Key, AESIV);

    constexpr auto DEC_AES128 = AES::Unpadded::Decrypt128(ENC_AES128, AES128Key, AESIV);
    constexpr auto DEC_AES192 = AES::Unpadded::Decrypt192(ENC_AES192, AES192Key, AESIV);
    constexpr auto DEC_AES256 = AES::Unpadded::Decrypt256(ENC_AES256, AES256Key, AESIV);

    static_assert(AESData == DEC_AES128, "BROKEN: AES128 (CBC)");
    static_assert(AESData == DEC_AES192, "BROKEN: AES192 (CBC)");
    static_assert(AESData == DEC_AES256, "BROKEN: AES256 (CBC)");

    inline void AEStest()
    {
        // Random data.
        constexpr std::array<uint8_t, 16> AESData{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        constexpr std::array<uint8_t, 16> AESIV{ 0x16, 0x15, 0x7e, 0x2b, 0xa6, 0xd2, 0xae, 0x28, 0x88, 0x15, 0xf7, 0xab, 0x3c, 0x4f, 0xcf, 0x09 };
        constexpr std::array<uint8_t, 16> AES128Key{ 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
        constexpr std::array<uint8_t, 24> AES192Key{ 0xe8, 0x69, 0xc2, 0x0c, 0xdc, 0xf7, 0x78, 0xa6, 0x44, 0x41, 0x44, 0x62, 0x81, 0x1b, 0xd4, 0xb1, 0x1d, 0xd0, 0x03, 0xa1, 0x32, 0x7f, 0xc2, 0x30 };
        constexpr std::array<uint8_t, 32> AES256Key{ 0x88, 0x05, 0x79, 0xbd, 0x7c, 0x1f, 0x39, 0x5b, 0xad, 0x10, 0x85, 0xac, 0xc1, 0xbf, 0x4f, 0x35, 0xb8, 0xa7, 0xc1, 0x7f, 0x1a, 0x00, 0x99, 0xa5, 0x49, 0x3b, 0x3a, 0x1, 0x81, 0xb0, 0x26, 0xdc };

        // Padded mode (PKCS#7)
        {
            auto ENC_AES128 = AES::Compiletime::Padded::Encrypt<AES::AES_CBC, 4, 10>(AESData, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto ENC_AES192 = AES::Compiletime::Padded::Encrypt<AES::AES_CBC, 6, 12>(AESData, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto ENC_AES256 = AES::Compiletime::Padded::Encrypt<AES::AES_CBC, 8, 14>(AESData, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES128 = AES::Runtime::Padded::Encrypt<AES::AES_CBC, 4, 10>(AESData, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES192 = AES::Runtime::Padded::Encrypt<AES::AES_CBC, 6, 12>(AESData, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES256 = AES::Runtime::Padded::Encrypt<AES::AES_CBC, 8, 14>(AESData, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));

            auto DEC_AES128 = AES::Compiletime::Padded::Decrypt<AES::AES_CBC, 4, 10>(ENC_AES128, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto DEC_AES192 = AES::Compiletime::Padded::Decrypt<AES::AES_CBC, 6, 12>(ENC_AES192, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto DEC_AES256 = AES::Compiletime::Padded::Decrypt<AES::AES_CBC, 8, 14>(ENC_AES256, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES128 = AES::Runtime::Padded::Decrypt<AES::AES_CBC, 4, 10>(SSLENC_AES128, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES192 = AES::Runtime::Padded::Decrypt<AES::AES_CBC, 6, 12>(SSLENC_AES192, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES256 = AES::Runtime::Padded::Decrypt<AES::AES_CBC, 8, 14>(SSLENC_AES256, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));

            if (ENC_AES128 != SSLENC_AES128) printf("BROKEN: AES128 (CBC, Padded)\n");
            if (ENC_AES192 != SSLENC_AES192) printf("BROKEN: AES192 (CBC, Padded)\n");
            if (ENC_AES256 != SSLENC_AES256) printf("BROKEN: AES256 (CBC, Padded)\n");

            if (DEC_AES128 != SSLDEC_AES128) printf("BROKEN: AES128 (CBC, Padded)\n");
            if (DEC_AES192 != SSLDEC_AES192) printf("BROKEN: AES192 (CBC, Padded)\n");
            if (DEC_AES256 != SSLDEC_AES256) printf("BROKEN: AES256 (CBC, Padded)\n");
        }

        // Plain.
        {
            auto ENC_AES128 = AES::Compiletime::Unpadded::Encrypt<AES::AES_CBC, 4, 10>(AESData, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto ENC_AES192 = AES::Compiletime::Unpadded::Encrypt<AES::AES_CBC, 6, 12>(AESData, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto ENC_AES256 = AES::Compiletime::Unpadded::Encrypt<AES::AES_CBC, 8, 14>(AESData, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES128 = AES::Runtime::Unpadded::Encrypt<AES::AES_CBC, 4, 10>(AESData, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES192 = AES::Runtime::Unpadded::Encrypt<AES::AES_CBC, 6, 12>(AESData, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLENC_AES256 = AES::Runtime::Unpadded::Encrypt<AES::AES_CBC, 8, 14>(AESData, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));

            auto DEC_AES128 = AES::Compiletime::Unpadded::Decrypt<AES::AES_CBC, 4, 10>(ENC_AES128, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto DEC_AES192 = AES::Compiletime::Unpadded::Decrypt<AES::AES_CBC, 6, 12>(ENC_AES192, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto DEC_AES256 = AES::Compiletime::Unpadded::Decrypt<AES::AES_CBC, 8, 14>(ENC_AES256, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES128 = AES::Runtime::Unpadded::Decrypt<AES::AES_CBC, 4, 10>(SSLENC_AES128, AES128Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES192 = AES::Runtime::Unpadded::Decrypt<AES::AES_CBC, 6, 12>(SSLENC_AES192, AES192Key, std::bit_cast<AES::Internal::Block_t>(AESIV));
            auto SSLDEC_AES256 = AES::Runtime::Unpadded::Decrypt<AES::AES_CBC, 8, 14>(SSLENC_AES256, AES256Key, std::bit_cast<AES::Internal::Block_t>(AESIV));

            if (ENC_AES128 != SSLENC_AES128) printf("BROKEN: AES128 (CBC, Unpadded)\n");
            if (ENC_AES192 != SSLENC_AES192) printf("BROKEN: AES192 (CBC, Unpadded)\n");
            if (ENC_AES256 != SSLENC_AES256) printf("BROKEN: AES256 (CBC, Unpadded)\n");

            if (DEC_AES128 != SSLDEC_AES128) printf("BROKEN: AES128 (CBC, Unpadded)\n");
            if (DEC_AES192 != SSLDEC_AES192) printf("BROKEN: AES192 (CBC, Unpadded)\n");
            if (DEC_AES256 != SSLDEC_AES256) printf("BROKEN: AES256 (CBC, Unpadded)\n");
        }
    }
}
#endif

#endif