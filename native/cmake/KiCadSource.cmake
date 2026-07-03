include_guard(GLOBAL)

function(pybind11_kicad_resolve_kicad_source out_var)
    if(PYBIND11_KICAD_KICAD_SOURCE_DIR)
        set(_kicad_source_dir "${PYBIND11_KICAD_KICAD_SOURCE_DIR}")
    elseif(PYBIND11_KICAD_FETCH_KICAD_SOURCE)
        find_package(Git REQUIRED)

        set(_kicad_source_dir "${CMAKE_BINARY_DIR}/_deps/kicad-src")

        if(NOT EXISTS "${_kicad_source_dir}/.git")
            file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")

            if(EXISTS "${_kicad_source_dir}")
                file(GLOB _kicad_source_dir_entries
                    LIST_DIRECTORIES true
                    "${_kicad_source_dir}/*"
                    "${_kicad_source_dir}/.[!.]*"
                    "${_kicad_source_dir}/..?*"
                )

                if(_kicad_source_dir_entries)
                    message(FATAL_ERROR
                        "KiCad source download directory exists but is not a Git checkout: "
                        "${_kicad_source_dir}"
                    )
                endif()
            endif()

            message(STATUS
                "Downloading KiCad ${PYBIND11_KICAD_TARGET_KICAD_VERSION} source into "
                "${_kicad_source_dir}"
            )
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" clone
                    --depth 1
                    --branch "${PYBIND11_KICAD_KICAD_GIT_TAG}"
                    --filter=blob:none
                    --progress
                    "${PYBIND11_KICAD_KICAD_GIT_REPOSITORY}"
                    "${_kicad_source_dir}"
                RESULT_VARIABLE _kicad_clone_result
            )

            if(_kicad_clone_result)
                message(FATAL_ERROR
                    "Could not download KiCad source from "
                    "${PYBIND11_KICAD_KICAD_GIT_REPOSITORY}"
                )
            endif()
        endif()
    else()
        message(STATUS "KiCad source fetch is disabled and no source directory was provided")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(NOT EXISTS "${_kicad_source_dir}/pcbnew/pcb_io/pcb_io_mgr.h")
        message(FATAL_ERROR
            "KiCad source directory does not look like KiCad ${PYBIND11_KICAD_TARGET_KICAD_VERSION}: "
            "${_kicad_source_dir}"
        )
    endif()

    find_package(Git QUIET)

    if(GIT_FOUND AND EXISTS "${_kicad_source_dir}/.git" AND PYBIND11_KICAD_KICAD_GIT_COMMIT)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${_kicad_source_dir}" rev-parse HEAD
            OUTPUT_VARIABLE _kicad_source_head
            ERROR_VARIABLE _kicad_source_head_error
            RESULT_VARIABLE _kicad_source_head_result
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(_kicad_source_head_result)
            message(FATAL_ERROR
                "Could not validate KiCad source commit in ${_kicad_source_dir}: "
                "${_kicad_source_head_error}"
            )
        endif()

        if(NOT _kicad_source_head STREQUAL PYBIND11_KICAD_KICAD_GIT_COMMIT)
            message(FATAL_ERROR
                "KiCad source commit mismatch. Expected "
                "${PYBIND11_KICAD_KICAD_GIT_COMMIT}, got ${_kicad_source_head}."
            )
        endif()
    endif()

    set(${out_var} "${_kicad_source_dir}" PARENT_SCOPE)
endfunction()
