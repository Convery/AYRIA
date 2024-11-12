cmake_minimum_required(VERSION 3.20)

# Future-proofing VCPKG..
list(APPEND VCPKG_FEATURE_FLAGS "versions")

# Overide the submodule install with VCPKG_ROOT
if (DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
else()
    set(CMAKE_TOOLCHAIN_FILE "./vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

# Override VCPKG's dynamic linking
if (DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}-static")
endif()

# Optional customization through third-party lubraries.
option(BETTER_CONTAINERS "Enable abseil containers" OFF)
option(BETTER_HOOKS "Enable third-party hooking" OFF)
if (BETTER_CONTAINERS)
    list(APPEND VCPKG_MANIFEST_FEATURES "abseil-containers")
endif()
if (BETTER_HOOKS)
    list(APPEND VCPKG_MANIFEST_FEATURES "better-hooks")
endif()
set(VCPKG_MANIFEST_FEATURES ${VCPKG_MANIFEST_FEATURES} CACHE INTERNAL "")

# Convery (tcn@ayria.se)
project(AYRIA)

# Use the latest standard at this time.
set(CMAKE_CXX_STANDARD 23)
enable_language(ASM_MASM)

# Needed for static analysis.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Export to the a gitignored directory.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)

# Separate all release and debug-builds.
mark_as_advanced(CMAKE_IMPORT_LIBRARY_SUFFIX CMAKE_DEBUG_POSTFIX)
set(CMAKE_DEBUG_POSTFIX "d")

# Compiler presets.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	# Global for x86, enable SSE2 auto-vectorization.
	if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /arch:SSE2")
    endif()

	# Release build.
	if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /Zc:__cplusplus /permissive- /EHc-s- /Oiysyb3 /GF /Gy /Gw /GL /GR- /utf-8 /Brepro /fp:except- /Zc:throwingNew /openmp:llvm /arch:AVX")
        set(EXTRA_LNKFLAGS "${EXTRA_LNKFLAGS} /time /LTCG /debug:full /opt:ref /opt:icf=8 /opt:lbr /incremental:no /Brepro /cgthreads:8")

	# Debug build.
	else()
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /Zc:__cplusplus /permissive- /EHc-s- /Gy /GR- /utf-8 /fp:except- /Zc:throwingNew /fsanitize=address") # /d2TPReport /openmp:llvm
        set(EXTRA_LNKFLAGS "${EXTRA_LNKFLAGS} /time /LTCG /debug:full /opt:ref /opt:lbr /incremental:no /cgthreads:8")
    endif()

	# Force static runtime, warnings, and override default flags.
    foreach(Flag CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO)
        string(REGEX REPLACE "/W[0-4]" "/W4" ${Flag} "${${Flag}}")
        string(REPLACE "/EHsc" "/EHc-s-" ${Flag} "${${Flag}}")
		string(REPLACE "/GR " "/GR- " ${Flag} "${${Flag}}")
        string(REPLACE "/Zi " "/Z7 " ${Flag} "${${Flag}}")
        string(REPLACE "/Ob2" "/Ob3" ${Flag} "${${Flag}}")
        string(REPLACE "/Ob1" "/Ob3" ${Flag} "${${Flag}}")
        string(REPLACE "/MD" "/MT" ${Flag} "${${Flag}}")
    endforeach()

else()

	# Assume GCC/CLANG compatible compiler.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")

	# For some reason my CMAKE fails to differentiate between x86 and x64.
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m32")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m32 -static-libgcc -static-libstdc++")
    else()
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m64")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m64 -static-libgcc -static-libstdc++")
    endif()
endif()

# We use VCPKG for third-party libraries, so find the include path through a required package.
find_library(STD_LIB_RAW libssl.lib HINTS ${PROJECT_SOURCE_DIR})
find_path(STD_INC openssl/ssl.h  HINTS ${PROJECT_SOURCE_DIR})
get_filename_component(STD_LIB ${STD_LIB_RAW} DIRECTORY)

# Let the sub-modules access the root and third-party directories easier.
include_directories("${PROJECT_SOURCE_DIR}/Utilities")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories(${STD_INC})
link_directories(${STD_LIB})

# Project modules.
#add_subdirectory(Core)
#add_subdirectory(Plugins)
#add_subdirectory(Frontend)

# Temporary & local experiements.
set(LOCAL_BUILD_DIR "${CMAKE_SOURCE_DIR}/local_build")
if(EXISTS ${LOCAL_BUILD_DIR} AND IS_DIRECTORY ${LOCAL_BUILD_DIR})
    add_subdirectory(${LOCAL_BUILD_DIR})
endif()
