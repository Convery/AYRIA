/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once
#include "Threading/Debugmutex.hpp"
#include "Threading/Spinlock.hpp"

// Helper to switch between debug and release mutex's.
#if defined (NDEBUG)
    #define Defaultmutex_t Spinlock_t
#else
    #define Defaultmutex_t Debugmutex_t
#endif
