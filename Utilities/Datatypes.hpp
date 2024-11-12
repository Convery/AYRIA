/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-19
    License: MIT
*/

#pragma once

// Generic datatype for dynamic arrays, std::byte leads to bad codegen on MSVC.
using Blob_view_t = std::basic_string_view<uint8_t>;
using Blob_t = std::basic_string<uint8_t>;

#include "Datatypes/bfloat16.hpp"
#include "Datatypes/float16.hpp"
#include "Datatypes/vec16.hpp"