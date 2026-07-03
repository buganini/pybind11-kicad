include_guard(GLOBAL)

function(_pybind11_kicad_configure_native_module target_name)
    set_target_properties("${target_name}"
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${PYBIND11_KICAD_NATIVE_OUTPUT_DIR}"
    )

    if(PYBIND11_KICAD_KICAD_BUILD_DIR)
        set(_pybind11_kicad_build_rpaths
            "${PYBIND11_KICAD_KICAD_BUILD_DIR}"
            "${PYBIND11_KICAD_KICAD_BUILD_DIR}/kicad"
            "${PYBIND11_KICAD_KICAD_BUILD_DIR}/kicad/KiCad.app/Contents/Frameworks"
        )
        set_property(TARGET "${target_name}" APPEND
            PROPERTY BUILD_RPATH ${_pybind11_kicad_build_rpaths}
        )
    endif()

    if(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
        target_compile_definitions("${target_name}"
            PRIVATE
                PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO
        )
    endif()
endfunction()

function(pybind11_kicad_add_native_targets native_source_dir)
    if(TARGET pybind11_kicad_core)
        return()
    endif()

    if(NOT PYBIND11_KICAD_NATIVE_OUTPUT_DIR)
        set(PYBIND11_KICAD_NATIVE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    add_library(pybind11_kicad_core STATIC
        "${native_source_dir}/src/board.cpp"
    )
    set_target_properties(pybind11_kicad_core
        PROPERTIES
            EXCLUDE_FROM_ALL ON
            POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(pybind11_kicad_core
        PUBLIC
            "${native_source_dir}/include"
    )
    target_compile_features(pybind11_kicad_core PUBLIC cxx_std_20)

    if(PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR)
        add_library(pybind11_kicad_kicad_source INTERFACE)
        target_include_directories(pybind11_kicad_kicad_source
            INTERFACE
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/include"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/common"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/pcbnew"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/pcbnew/board_stackup_manager"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/3d-viewer"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/3d-viewer/3d_canvas"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/3d-viewer/3d_viewer"
        )

        if(PYBIND11_KICAD_KICAD_BUILD_DIR)
            target_include_directories(pybind11_kicad_kicad_source
                INTERFACE
                    "${PYBIND11_KICAD_KICAD_BUILD_DIR}"
            )
        endif()

        target_compile_definitions(pybind11_kicad_kicad_source
            INTERFACE
                PYBIND11_KICAD_TARGET_KICAD_MAJOR=${PYBIND11_KICAD_TARGET_KICAD_MAJOR}
                PYBIND11_KICAD_TARGET_KICAD_VERSION="${PYBIND11_KICAD_TARGET_KICAD_VERSION}"
        )
        target_link_libraries(pybind11_kicad_core PRIVATE pybind11_kicad_kicad_source)
        message(STATUS "Using KiCad source: ${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}")
    endif()

    if(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
        if(NOT PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR)
            message(FATAL_ERROR "KiCad board IO requires a resolved KiCad source tree")
        endif()

        if(NOT PYBIND11_KICAD_KICAD_BUILD_DIR)
            message(FATAL_ERROR "KiCad board IO requires PYBIND11_KICAD_KICAD_BUILD_DIR")
        endif()

        if(NOT EXISTS "${PYBIND11_KICAD_KICAD_BUILD_DIR}/config.h")
            message(FATAL_ERROR
                "KiCad board IO requires a configured KiCad build tree with config.h: "
                "${PYBIND11_KICAD_KICAD_BUILD_DIR}"
            )
        endif()

        if(NOT PYBIND11_KICAD_KICAD_LINK_LIBRARIES AND TARGET pcbcommon)
            set(PYBIND11_KICAD_KICAD_LINK_LIBRARIES
                pcbcommon
                ${PCBNEW_IO_LIBRARIES}
                ${wxWidgets_LIBRARIES}
                ${PYTHON_LIBRARIES}
                Boost::headers
                ${PCBNEW_EXTRA_LIBS}
            )
        endif()

        if(NOT PYBIND11_KICAD_KICAD_LINK_LIBRARIES)
            message(FATAL_ERROR
                "KiCad board IO requires PYBIND11_KICAD_KICAD_LINK_LIBRARIES. "
                "KiCad does not provide a small exported board-IO SDK target; pass the "
                "KiCad build-tree libraries/targets needed for PCB_IO_MGR and pcbnew, "
                "or add native targets from the end of a configured KiCad CMake build graph."
            )
        endif()

        target_compile_definitions(pybind11_kicad_core
            PRIVATE
                PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO
        )
        target_sources(pybind11_kicad_core
            PRIVATE
                "${native_source_dir}/src/headless_kiface.cpp"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/pcbnew/board_stackup_manager/stackup_predefined_prms.cpp"
                "${PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR}/3d-viewer/3d_viewer/eda_3d_viewer_settings.cpp"
        )

        if(TARGET markdown_lib)
            list(APPEND PYBIND11_KICAD_KICAD_LINK_LIBRARIES markdown_lib)
        endif()

        target_link_libraries(pybind11_kicad_core
            PRIVATE
                ${PYBIND11_KICAD_KICAD_LINK_LIBRARIES}
        )
    endif()

    if(NOT COMMAND pybind11_add_module)
        find_package(pybind11 CONFIG QUIET)
    endif()

    if(COMMAND pybind11_add_module)
        pybind11_add_module(pybind11_kicad_native
            "${native_source_dir}/src/bindings.cpp"
        )
        set_target_properties(pybind11_kicad_native
            PROPERTIES
                EXCLUDE_FROM_ALL ON
        )
        _pybind11_kicad_configure_native_module(pybind11_kicad_native)
        target_link_libraries(pybind11_kicad_native
            PRIVATE
                pybind11_kicad_core
        )
    elseif(PYBIND11_INCLUDE_DIR OR TARGET pybind11::headers)
        add_library(pybind11_kicad_native MODULE
            "${native_source_dir}/src/bindings.cpp"
        )
        set_target_properties(pybind11_kicad_native
            PROPERTIES
                EXCLUDE_FROM_ALL ON
                PREFIX ""
                LIBRARY_OUTPUT_DIRECTORY "${PYBIND11_KICAD_NATIVE_OUTPUT_DIR}"
                CXX_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN ON
        )
        _pybind11_kicad_configure_native_module(pybind11_kicad_native)

        if(PYBIND11_INCLUDE_DIR)
            target_include_directories(pybind11_kicad_native
                PRIVATE
                    "${PYBIND11_INCLUDE_DIR}"
            )
        endif()

        if(PYTHON_INCLUDE_DIRS)
            target_include_directories(pybind11_kicad_native
                PRIVATE
                    ${PYTHON_INCLUDE_DIRS}
            )
        endif()

        target_link_libraries(pybind11_kicad_native
            PRIVATE
                pybind11_kicad_core
                ${PYTHON_LIBRARIES}
        )

        if(TARGET pybind11::headers)
            target_link_libraries(pybind11_kicad_native
                PRIVATE
                    pybind11::headers
            )
        endif()
    else()
        message(STATUS "pybind11 was not found; only the C++ facade target will be configured")
    endif()
endfunction()
