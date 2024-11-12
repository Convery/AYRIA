/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-21
    License: MIT

    Constexpr implementation used as fallback if OpenSSL is not available.
*/

#pragma once
#include <Utilities.hpp>

#if __has_include(<openssl/evp.h>)
#include <openssl/evp.h>
#endif

namespace Hash
{
    namespace SHAData
    {
        constexpr std::array<uint32_t, 64> kSHA256 =
        {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
        constexpr std::array<uint64_t, 80> kSHA512 =
        {
            0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
            0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
            0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
            0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
            0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
            0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
            0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
            0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
            0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
            0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
            0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
            0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
            0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
            0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
            0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
            0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
            0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
            0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
            0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
            0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
        };

        constexpr std::array<uint32_t, 8> sSHA256 =
        {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        constexpr std::array<uint64_t, 8> sSHA512 =
        {
            0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
            0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
        };
    }
    namespace Compiletime
    {
        using namespace SHAData;

        constexpr void Transform256(std::array<uint32_t, 8> &State, std::span<const uint8_t> Input)
        {
            std::array<uint32_t, 8> Copy{ State };
            std::array<uint32_t, 64> Scratch{};

            // Can't just cast in constexpr.
            cmp::memcpy(Scratch.data(), Input.data(), 16 * sizeof(uint32_t));
            for (size_t i = 0; i < 16; ++i) Scratch[i] = cmp::toBig(Scratch[i]);

            for (size_t i = 16; i < 64; ++i)
            {
                const uint32_t Sigma0 = std::rotr(Scratch[i - 15], 7) ^ std::rotr(Scratch[i - 15], 18) ^ Scratch[i - 15] >> 3;
                const uint32_t Sigma1 = std::rotr(Scratch[i - 2], 17) ^ std::rotr(Scratch[i - 2], 19) ^ Scratch[i - 2] >> 10;
                Scratch[i] = (Scratch[i - 16] + Sigma0 + Scratch[i - 7] + Sigma1);
            }

            for (size_t i = 0; i < 64; ++i)
            {
                const uint32_t Sigma0 = std::rotr(Copy[0], 2) ^ std::rotr(Copy[0], 13) ^ std::rotr(Copy[0], 22);
                const uint32_t Sigma1 = std::rotr(Copy[4], 6) ^ std::rotr(Copy[4], 11) ^ std::rotr(Copy[4], 25);
                const uint32_t Maj = (Copy[0] & Copy[1]) ^ (Copy[0] & Copy[2]) ^ (Copy[1] & Copy[2]);
                const uint32_t Ch = (Copy[4] & Copy[5]) ^ ((~Copy[4]) & Copy[6]);
                const uint32_t t1 = Copy[7] + Sigma1 + Ch + kSHA256[i] + Scratch[i];
                const uint32_t t2 = Sigma0 + Maj;

                Copy[7] = Copy[6];
                Copy[6] = Copy[5];
                Copy[5] = Copy[4];
                Copy[4] = (Copy[3] + t1);
                Copy[3] = Copy[2];
                Copy[2] = Copy[1];
                Copy[1] = Copy[0];
                Copy[0] = (t1 + t2);
            }

            for (size_t i = 0; i < 8; ++i)
            {
                State[i] += Copy[i];
            }
        }
        constexpr void Transform512(std::array<uint64_t, 8> &State, std::span<const uint8_t> Input)
        {
            std::array<uint64_t, 8> Copy{ State };
            std::array<uint64_t, 80> Scratch{};

            // Can't just cast in constexpr.
            cmp::memcpy(Scratch.data(), Input.data(), 16 * sizeof(uint64_t));
            for (size_t i = 0; i < 16; ++i) Scratch[i] = cmp::toBig(Scratch[i]);

            for (size_t i = 16; i < 80; ++i)
            {
                const uint64_t Sigma0 = std::rotr(Scratch[i - 15], 1) ^ std::rotr(Scratch[i - 15], 8) ^ Scratch[i - 15] >> 7;
                const uint64_t Sigma1 = std::rotr(Scratch[i - 2], 19) ^ std::rotr(Scratch[i - 2], 61) ^ Scratch[i - 2] >> 6;
                Scratch[i] = (Scratch[i - 16] + Sigma0 + Scratch[i - 7] + Sigma1);
            }

            for (size_t i = 0; i < 80; ++i)
            {
                const uint64_t Sigma0 = std::rotr(Copy[0], 28) ^ std::rotr(Copy[0], 34) ^ std::rotr(Copy[0], 39);
                const uint64_t Sigma1 = std::rotr(Copy[4], 14) ^ std::rotr(Copy[4], 18) ^ std::rotr(Copy[4], 41);

                const uint64_t Maj = (Copy[0] & Copy[1]) ^ (Copy[0] & Copy[2]) ^ (Copy[1] & Copy[2]);
                const uint64_t Ch = (Copy[4] & Copy[5]) ^ ((~Copy[4]) & Copy[6]);
                const uint64_t t1 = Copy[7] + Sigma1 + Ch + kSHA512[i] + Scratch[i];
                const uint64_t t2 = Sigma0 + Maj;

                Copy[7] = Copy[6];
                Copy[6] = Copy[5];
                Copy[5] = Copy[4];
                Copy[4] = (Copy[3] + t1);
                Copy[3] = Copy[2];
                Copy[2] = Copy[1];
                Copy[1] = Copy[0];
                Copy[0] = (t1 + t2);
            }

            for (size_t i = 0; i < 8; ++i)
            {
                State[i] += Copy[i];
            }
        }

        constexpr std::array<uint8_t, 32> SHA256(std::span<const uint8_t> Input)
        {
            std::array<uint32_t, 8> State = { sSHA256 };
            const auto Remaining = Input.size() & 63;
            const size_t Count = Input.size() / 64;

            for (size_t i = 0; i < Count; ++i)
            {
                Transform256(State, Input.subspan(i * 64, 64));
            }

            std::array<uint8_t, 64> Lastblock{};
            auto Span = Input.subspan(Input.size() - Remaining);
            cmp::memcpy(Lastblock.data(), Span.data(), Span.size());

            Lastblock[Remaining] = uint8_t(0x80);
            if (Remaining >= 56)
            {
                Transform256(State, std::span<const uint8_t>(Lastblock));
                Lastblock = {};
            }

            cmp::memcpy(&Lastblock[56], cmp::toBig<uint64_t>(Input.size() << 3));
            Transform256(State, std::span<const uint8_t>(Lastblock));

            for (size_t i = 0; i < 8; ++i) State[i] = cmp::toBig(State[i]);
            return std::bit_cast<std::array<uint8_t, 32>>(State);
        }
        constexpr std::array<uint8_t, 64> SHA512(std::span<const uint8_t> Input)
        {
            std::array<uint64_t, 8> State = { sSHA512 };
            const auto Remaining = Input.size() & 127;
            const size_t Count = Input.size() / 128;

            for (size_t i = 0; i < Count; ++i)
            {
                Transform512(State, Input.subspan(i * 128, 128));
            }

            std::array<uint8_t, 128> Lastblock{};
            auto Span = Input.subspan(Input.size() - Remaining);
            cmp::memcpy(Lastblock.data(), Span.data(), Remaining);

            Lastblock[Remaining] = uint8_t(0x80);
            if (Remaining >= 112)
            {
                Transform512(State, std::span<const uint8_t>(Lastblock));
                Lastblock = {};
            }

            cmp::memcpy(&Lastblock[120], cmp::toBig<uint64_t>(Input.size() << 3));
            Transform512(State, std::span<const uint8_t>(Lastblock));

            for (size_t i = 0; i < 8; ++i) State[i] = cmp::toBig(State[i]);
            return std::bit_cast<std::array<uint8_t, 64>>(State);
        }
    }
    namespace Runtime
    {
        // If built with OpenSSL, use the (possibly) hardware accelerated engine.
        inline std::array<uint8_t, 32> SHA256(std::span<const uint8_t> Input)
        {
            #if __has_include(<openssl/evp.h>)
            const auto Context = EVP_MD_CTX_create();
            std::array<uint8_t, 32> Result{};

            EVP_DigestInit_ex(Context, EVP_sha256(), nullptr);
            EVP_DigestUpdate(Context, Input.data(), Input.size());
            EVP_DigestFinal_ex(Context, (unsigned char *)Result.data(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return Result;
            #else
            return Compiletime::SHA256(Input);
            #endif
        }
        inline std::array<uint8_t, 64> SHA512(std::span<const uint8_t> Input)
        {
            #if __has_include(<openssl/evp.h>)
            const auto Context = EVP_MD_CTX_create();
            std::array<uint8_t, 64> Result{};

            EVP_DigestInit_ex(Context, EVP_sha512(), nullptr);
            EVP_DigestUpdate(Context, Input.data(), Input.size());
            EVP_DigestFinal_ex(Context, (unsigned char *)Result.data(), nullptr);
            EVP_MD_CTX_destroy(Context);

            return Result;
            #else
            return Compiletime::SHA512(Input);
            #endif
        }
    }

    // Any range.
    template <cmp::Range_t T> constexpr std::array<uint8_t, 32> SHA256(const T &Input)
    {
        if (!std::is_constant_evaluated()) return Runtime::SHA256(cmp::getBytes(Input));
        else return Compiletime::SHA256(cmp::getBytes(Input));
    }
    template <cmp::Range_t T> constexpr std::array<uint8_t, 64> SHA512(const T &Input)
    {
        if (!std::is_constant_evaluated()) return Runtime::SHA512(cmp::getBytes(Input));
        else return Compiletime::SHA512(cmp::getBytes(Input));
    }

    // Any other typed value.
    template <typename T> requires (!cmp::Range_t<T>) constexpr std::array<uint8_t, 32> SHA256(const T &Input)
    {
        return SHA256(cmp::getBytes(Input));
    }
    template <typename T> requires (!cmp::Range_t<T>) constexpr std::array<uint8_t, 64> SHA512(const T &Input)
    {
        return SHA512(cmp::getBytes(Input));
    }

    // String literals.
    template <cmp::Char_t T, size_t N> constexpr std::array<uint8_t, 32> SHA256(const T(&Input)[N])
    {
        return SHA256(cmp::getBytes(cmp::stripNullchar(Input)));
    }
    template <cmp::Char_t T, size_t N> constexpr std::array<uint8_t, 64> SHA512(const T(&Input)[N])
    {
        return SHA512(cmp::getBytes(cmp::stripNullchar(Input)));
    }
}

#if defined(ENABLE_UNITTESTS)
namespace Unittests
{
    static_assert("5994471abb01112afcc18159f6cc74b4f511b99806da59b3caf5a9c173cacfc5" == String::toHex(Hash::SHA256("12345")), "BROKEN: SHA256 hashing");
    static_assert("3627909a29c31381a071ec27f7c9ca97726182aed29a7ddd2e54353322cfb30abb9e3a6df2ac2c20fe23436311d678564d0c8d305930575f60e2d3d048184d79" == String::toHex(Hash::SHA512("12345")), "BROKEN: SHA512 hashing");
}
#endif
