cmake_minimum_required(VERSION 3.20)

project(pybind11_kicad_native LANGUAGES CXX)

set(PYBIND11_KICAD_TARGET_KICAD_MAJOR "10" CACHE STRING "Target KiCad major version")
set(PYBIND11_KICAD_TARGET_KICAD_VERSION "10.0.4" CACHE STRING "Target KiCad version")
set(PYBIND11_KICAD_KICAD_GIT_REPOSITORY "https://gitlab.com/kicad/code/kicad.git" CACHE STRING "KiCad source Git repository")
set(PYBIND11_KICAD_KICAD_GIT_TAG "${PYBIND11_KICAD_TARGET_KICAD_VERSION}" CACHE STRING "KiCad source Git tag")
set(PYBIND11_KICAD_KICAD_GIT_COMMIT "f7414d419cae5df2d00e7eaacb16fc0e803799bc" CACHE STRING "Expected KiCad source commit")
set(PYBIND11_KICAD_KICAD_SOURCE_DIR "" CACHE PATH "Existing KiCad source checkout")
set(PYBIND11_KICAD_KICAD_BUILD_DIR "" CACHE PATH "Configured KiCad build tree")
set(PYBIND11_KICAD_KICAD_LINK_LIBRARIES "" CACHE STRING "Semicolon-separated KiCad libraries or targets for board IO")

option(PYBIND11_KICAD_FETCH_KICAD_SOURCE "Download the pinned KiCad source when no source directory is provided" ON)
option(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO "Compile the KiCad-backed board IO adapter" OFF)
option(PYBIND11_KICAD_ENABLE_PCBNEW_COMPAT "Build pcbnew compatibility support" ON)
option(PYBIND11_KICAD_ENABLE_GUI "Enable KiCad GUI/editor integration" OFF)

set(PYBIND11_FINDPYTHON ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(KiCadSource)
pybind11_kicad_resolve_kicad_source(PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR)

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Official Windows KiCad and CPython extension targets use MSVC; configure with a compatible Visual Studio/MSVC toolchain.")
endif()

if(APPLE AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(FATAL_ERROR "Official macOS KiCad targets and CPython extensions should use the Apple Clang/Xcode toolchain family.")
endif()

if(UNIX AND NOT APPLE AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Linux KiCad targets and CPython extensions are GCC/libstdc++ oriented; configure with the distro-compatible GCC toolchain.")
endif()

include(Pybind11KiCadNativeTargets)
pybind11_kicad_add_native_targets("${CMAKE_CURRENT_SOURCE_DIR}")
