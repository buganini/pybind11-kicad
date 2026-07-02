cmake_minimum_required(VERSION 3.20)

project(pybind11_kicad_native LANGUAGES CXX)

set(PYBIND11_KICAD_TARGET_KICAD_MAJOR "10" CACHE STRING "Target KiCad major version")
set(PYBIND11_KICAD_BUNDLED_KICAD_VERSION "10.0.x" CACHE STRING "Pinned bundled KiCad version")

option(PYBIND11_KICAD_ENABLE_PCBNEW_COMPAT "Build pcbnew compatibility support" ON)
option(PYBIND11_KICAD_ENABLE_GUI "Enable KiCad GUI/editor integration" OFF)

set(PYBIND11_FINDPYTHON ON)

add_library(pybind11_kicad_core STATIC
    src/board.cpp
)
target_include_directories(pybind11_kicad_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(pybind11_kicad_core PUBLIC cxx_std_20)

find_package(pybind11 CONFIG QUIET)
if(pybind11_FOUND)
    pybind11_add_module(pybind11_kicad_native
        src/bindings.cpp
    )
    target_link_libraries(pybind11_kicad_native
        PRIVATE
            pybind11_kicad_core
    )
else()
    message(STATUS "pybind11 was not found; only the C++ facade target will be configured")
endif()
