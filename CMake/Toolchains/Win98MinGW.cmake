# Toolchain file for building MoonChildFE (Win98 Edition) with any i686-w64-mingw32
# GCC cross-toolchain - the MSYS2 "mingw32" environment on Windows, or a custom
# cross-built toolchain on Linux/macOS (e.g. mingw-builds targeting old Pentium CPUs).
#
# Usage: cmake --preset win98-debug
# Override the toolchain location with -DMOONCHILD_MINGW32_ROOT=/path/to/toolchain
# (the directory containing "bin/i686-w64-mingw32-gcc[.exe]") or the
# MOONCHILD_MINGW32_ROOT environment variable. If unset, it's auto-detected: on
# Windows this defaults to C:/msys64/mingw32, otherwise the prefixed
# i686-w64-mingw32-gcc/g++/windres executables are looked up on PATH.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR X86)

if(NOT DEFINED MOONCHILD_MINGW32_ROOT)
    if(DEFINED ENV{MOONCHILD_MINGW32_ROOT})
        set(MOONCHILD_MINGW32_ROOT "$ENV{MOONCHILD_MINGW32_ROOT}")
    elseif(CMAKE_HOST_WIN32)
        set(MOONCHILD_MINGW32_ROOT "C:/msys64/mingw32")
    else()
        set(MOONCHILD_MINGW32_ROOT "")
    endif()
endif()

# MSYS2's mingw32 environment ships its cross compiler under both names
# (plain gcc.exe within that environment, and the prefixed alias); a Linux/macOS
# cross-built toolchain (e.g. mingw-builds) typically only ships the prefixed
# name. Try the prefixed name everywhere (safe - it can't collide with a native
# host compiler), and only accept the unprefixed name when a root was given.
set(_moonchild_c_names   i686-w64-mingw32-gcc   i686-w64-mingw32-gcc.exe)
set(_moonchild_cxx_names i686-w64-mingw32-g++   i686-w64-mingw32-g++.exe)
set(_moonchild_rc_names  i686-w64-mingw32-windres i686-w64-mingw32-windres.exe)

if(NOT MOONCHILD_MINGW32_ROOT STREQUAL "")
    find_program(CMAKE_C_COMPILER
        NAMES ${_moonchild_c_names} gcc.exe gcc
        PATHS "${MOONCHILD_MINGW32_ROOT}/bin"
        NO_DEFAULT_PATH)
    find_program(CMAKE_CXX_COMPILER
        NAMES ${_moonchild_cxx_names} g++.exe g++
        PATHS "${MOONCHILD_MINGW32_ROOT}/bin"
        NO_DEFAULT_PATH)
    find_program(CMAKE_RC_COMPILER
        NAMES ${_moonchild_rc_names} windres.exe windres
        PATHS "${MOONCHILD_MINGW32_ROOT}/bin"
        NO_DEFAULT_PATH)
endif()

if(NOT CMAKE_C_COMPILER)
    find_program(CMAKE_C_COMPILER NAMES ${_moonchild_c_names})
endif()
if(NOT CMAKE_CXX_COMPILER)
    find_program(CMAKE_CXX_COMPILER NAMES ${_moonchild_cxx_names})
endif()
if(NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER NAMES ${_moonchild_rc_names})
endif()

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR
        "Could not find an i686-w64-mingw32 GCC cross-toolchain! Set "
        "MOONCHILD_MINGW32_ROOT to the directory containing its 'bin' folder "
        "(e.g. C:/msys64/mingw32, or ~/mingw/mingw-builds/install/cross), or make "
        "sure i686-w64-mingw32-gcc/g++ are on PATH.")
endif()

# Derive the sysroot from wherever the compiler was actually found, so
# find_package()/find_library() etc. look next to the compiler rather than on
# the host system, regardless of how MOONCHILD_MINGW32_ROOT was resolved above.
get_filename_component(_moonchild_bin_dir "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(_moonchild_sysroot "${_moonchild_bin_dir}" DIRECTORY)

set(CMAKE_FIND_ROOT_PATH "${_moonchild_sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# -march=i586 keeps the generated code compatible with the original Pentium-era
# machines that shipped with Windows 98; drop it (or override via
# CMAKE_C_FLAGS/CMAKE_CXX_FLAGS) if your toolchain already defaults to an old
# enough baseline, or if you only care about running under emulation/VMs.
set(CMAKE_C_FLAGS_INIT   "-march=i586")
set(CMAKE_CXX_FLAGS_INIT "-march=i586")
