/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once

// Fixup some Visual Studio builds not defining this.
#if !defined (_DEBUG) && !defined (NDEBUG)
    #define NDEBUG
#endif

// MS-STL does asserts in constexpr operators.
#if !defined (NDEBUG)
#define _CONTAINER_DEBUG_LEVEL 0
#define _ITERATOR_DEBUG_LEVEL 0
#endif

// Platform identification.
#if defined (_MSC_VER)
#define EXPORT_ATTR __declspec(dllexport)
#define IMPORT_ATTR __declspec(dllimport)
#define INLINE_ATTR __forceinline
#define NOINLINE_ATTR __declspec(noinline)

#elif defined (__GNUC__) || defined (__clang__)
#define EXPORT_ATTR __attribute__((visibility("default")))
#define IMPORT_ATTR
#define INLINE_ATTR __attribute__((always_inline))
#define NOINLINE_ATTR __attribute__((noinline))
#else
#error Unknown compiler..
#endif

// Remove some Windows annoyance.
#if defined (_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S
#define NOMINMAX
#endif

// Build information helpers to avoid the preprocessor.
namespace Build
{
    constexpr bool is64bit = sizeof(void *) == 8;

    #if defined (_WIN32)
    constexpr bool isWindows = true;
    #else
    constexpr bool isWindows = false;
    #endif

    #if defined (__linux__)
    constexpr bool isLinux = true;
    #else
    constexpr bool isLinux = false;
    #endif

    #if defined (NDEBUG)
    constexpr bool isDebug = false;
    #else
    constexpr bool isDebug = true;
    #endif
}

// Information logging.
//namespace Logging { template <typename T> extern void Print(char Prefix, const T &Message); }
//#define Debugprint(string) if constexpr (Build::isDebug) Logging::Print('D', string)
//#define Traceprint() if constexpr (Build::isDebug) Logging::Print('>', __FUNCTION__)
//#define Warningprint(string) Logging::Print('W', string)
//#define Errorprint(string) Logging::Print('E', string)
//#define Infoprint(string) Logging::Print('I', string)

#define Debugprint(string)
#define Traceprint()
#define Warningprint(string)
#define Errorprint(string)
#define Infoprint(string)

// Asserts should indicate an unreachable state.
#define ASSERT(x) { if (!std::is_constant_evaluated()) assert(x); if (!Build::isDebug && !(x)) std::unreachable(); }

// Ignore ANSI compatibility for structs.
#pragma warning(disable: 4200)
#pragma warning(disable: 4201)

// Some (STL) objects may leak in case of an exception.
#pragma warning(disable: 4530)

// Ignore warnings about casting float to int.
#pragma warning(disable: 4244)

// Conditional expression is constant (currently).
#pragma warning(disable: 4127)

// Disable warnings about unused parameters (as we often interface with other APIs).
#pragma warning(disable: 4100)

// Elevate [[nodiscard]] to an error.
#pragma warning(error: 4834)
