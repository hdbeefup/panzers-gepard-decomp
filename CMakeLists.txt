# Codename Panzers: Phase One — decomp / recompile ("Gepard" engine)
#
# The engine base under src/ was imported from the S.W.I.N.E. HD runtime
# decomp (swine-portable @ 95bbcc4); see docs/IMPORT_NOTES.md. Baseline:
# MSVC, 32-bit x86, DirectX 9 (June 2010 SDK). x86 ONLY: the decompiled code
# stores pointers in 32-bit fields and has x86 layout static_asserts.
cmake_minimum_required(VERSION 3.10)
project(PanzersGepard LANGUAGES CXX C)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
        "PanzersGepard is x86 (32-bit) only, but this configure targets a "
        "${CMAKE_SIZEOF_VOID_P}-byte pointer architecture. "
        "Reconfigure a clean build dir with: cmake -S . -B build -A Win32")
endif()

set(PANZERS_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Match original compiler settings (same flags as swine-portable)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(
        /W3
        /wd4302
        /wd4996
        /GR
        /EHsc
        /GS-
        /Zc:wchar_t
        /Zc:forScope
        /fp:precise
        /D_CRT_SECURE_NO_WARNINGS
        /DWIN32
        /D_WINDOWS
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X86")
    # Generate PDB even in Release for debugging
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options("$<$<CONFIG:Release>:/DEBUG>")
    add_link_options("$<$<CONFIG:Release>:/OPT:REF>")
    add_link_options("$<$<CONFIG:Release>:/OPT:ICF>")
endif()

# DirectX 9 SDK (June 2010)
if(DEFINED ENV{DXSDK_DIR})
    set(DXSDK_DIR "$ENV{DXSDK_DIR}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../dxsdk")
    set(DXSDK_DIR "${CMAKE_SOURCE_DIR}/../dxsdk")
elseif(EXISTS "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
    set(DXSDK_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
else()
    message(WARNING "DirectX SDK not found. Set DXSDK_DIR environment variable.")
    set(DXSDK_DIR "")
endif()

if(DXSDK_DIR)
    set(DXSDK_INCLUDE_DIR "${DXSDK_DIR}/Include")
    set(DXSDK_LIB_DIR "${DXSDK_DIR}/Lib/x86")
    message(STATUS "DirectX SDK: ${DXSDK_DIR} (libs: ${DXSDK_LIB_DIR})")
endif()

# Common include directories
include_directories(
    ${PANZERS_SRC_DIR}/core
    ${PANZERS_SRC_DIR}/core/chaos
    ${PANZERS_SRC_DIR}/common
    ${PANZERS_SRC_DIR}/3dengine
)

if(DXSDK_DIR)
    # IMPORTANT: DX SDK include must come AFTER system includes to avoid
    # conflicts with newer MSVC headers. Use SYSTEM to lower priority.
    include_directories(AFTER SYSTEM ${DXSDK_INCLUDE_DIR})
    link_directories(${DXSDK_LIB_DIR})
endif()

# Audio backend. dsound = SWINE's DirectSound concert (+ in-tree mdec MP3
# decoder); miniaudio is kept selectable as in SWINE (unverified).
# Panzers HD uses Miles (SMilesConcert, src/sound/milesconcert.cpp) when
# options.ini has "Use Miles = 1", the shipped default; that is the default here.
set(SWINE_AUDIO_BACKEND "miles" CACHE STRING
    "Audio backend: miles (Panzers default), dsound (SWINE) or miniaudio (unverified)")
set_property(CACHE SWINE_AUDIO_BACKEND PROPERTY STRINGS miles dsound miniaudio)
if(NOT SWINE_AUDIO_BACKEND MATCHES "^(miles|dsound|miniaudio)$")
    message(FATAL_ERROR "SWINE_AUDIO_BACKEND must be 'miles', 'dsound' or 'miniaudio' (got '${SWINE_AUDIO_BACKEND}')")
endif()
message(STATUS "Audio backend: ${SWINE_AUDIO_BACKEND}")

# zstd is NOT built: its only engine user (ANI2 animations in
# 3dengine/animation.cpp, a SWINE-HD-only format) is stubbed in src/stubs/.

# Engine libraries, in dependency order
add_subdirectory(src/core)
add_subdirectory(src/3dengine)
add_subdirectory(src/window)
add_subdirectory(third_party/rad)   # mss32/binkw32 import libs from .def (no SDK)
add_subdirectory(src/sound)
if(SWINE_AUDIO_BACKEND STREQUAL "dsound")
    add_subdirectory(src/mdec)
endif()
add_subdirectory(src/platform)
add_subdirectory(src/common)
add_subdirectory(src/stubs)

# Bink intro player (src/panzers/bink.*), lifted from SSuperWindow. A separate
# library so SSuperWindow (src/panzers) and tools can link it.
add_library(panzers_bink STATIC src/panzers/bink.cpp src/panzers/bink.h)
target_include_directories(panzers_bink PUBLIC ${PANZERS_SRC_DIR}/panzers)
target_link_libraries(panzers_bink PUBLIC core rad_binkw32 rad_mss32 netapi32)
if(SWINE_AUDIO_BACKEND STREQUAL "miles")
    add_subdirectory(src/tools/soundtest)
endif()

# Tripwire census: printed at configure time here, and POST_BUILD by the
# panzers target (src/main). Python must be found before src/main is added.
find_package(Python3 COMPONENTS Interpreter)

# Executable
add_subdirectory(src/main)

# Test tools
add_subdirectory(src/tools/paktest)
add_subdirectory(src/tools/rendertest)

if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/census.py ${PANZERS_SRC_DIR}
        OUTPUT_VARIABLE _panzers_census
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "${_panzers_census}")
else()
    message(WARNING "Python 3 not found: tripwire census (tools/census.py) will not run")
endif()
