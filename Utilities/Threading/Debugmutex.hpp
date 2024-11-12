/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include <mutex>
#include <thread>
#include <cstdio>
#include <format>

// Timed mutex that checks for cyclic locking and long tasks.
struct Debugmutex_t
{
    std::thread::id Currentowner{};
    std::timed_mutex Mutex{};

    // Unified error func.
    [[noreturn]] static void Break(const std::string &Message)
    {
        (void)std::fprintf(stderr, "%s\n", Message.c_str());

        #if defined (_MSC_VER)
        __debugbreak();
        #elif defined (__clang__)
        __builtin_debugtrap();
        #elif defined (__GNUC__)
        __builtin_trap();
        #endif

        // Incase someone included this in release mode.
        volatile size_t Intentional_nullderef = 0xDEAD;
        *(size_t *)Intentional_nullderef = 0xDEAD;

        std::unreachable();
    }

    void lock()
    {
        if (Currentowner == std::this_thread::get_id())
        {
            Break(std::format("Debugmutex: Recursive lock by thread {:X}", std::bit_cast<uint32_t>(Currentowner)));
        }

        if (!Mutex.try_lock_for(std::chrono::seconds(10)))
        {
            Break(std::format("Debugmutex: Timeout, locked by thread {:X}", std::bit_cast<uint32_t>(Currentowner)));
        }

        Currentowner = std::this_thread::get_id();
    }
    void unlock()
    {
        if (Currentowner != std::this_thread::get_id())
        {
            Break(std::format("Debugmutex: Thread {:X} tried to unlock a mutex owned by {:X}", std::bit_cast<uint32_t>(std::this_thread::get_id()), std::bit_cast<uint32_t>(Currentowner)));
        }

        Currentowner = {};
        Mutex.unlock();
    }
};
