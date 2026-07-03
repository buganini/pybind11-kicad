#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

TARGET_KICAD_MAJOR="${PYBIND11_KICAD_TARGET_KICAD_MAJOR:-10}"
TARGET_KICAD_VERSION="${PYBIND11_KICAD_TARGET_KICAD_VERSION:-10.0.4}"
KICAD_GIT_REPOSITORY="${PYBIND11_KICAD_KICAD_GIT_REPOSITORY:-https://gitlab.com/kicad/code/kicad.git}"
KICAD_GIT_TAG="${PYBIND11_KICAD_KICAD_GIT_TAG:-$TARGET_KICAD_VERSION}"
KICAD_GIT_COMMIT="${PYBIND11_KICAD_KICAD_GIT_COMMIT:-f7414d419cae5df2d00e7eaacb16fc0e803799bc}"

BUILD_DIR="${PYBIND11_KICAD_BUILD_DIR:-$ROOT_DIR/tmp/pybind11-kicad-$TARGET_KICAD_MAJOR-build}"
SOURCE_DIR="${PYBIND11_KICAD_KICAD_SOURCE_DIR:-$ROOT_DIR/tmp/kicad}"
PYTHON_ENV_DIR="${PYBIND11_KICAD_PYTHON_ENV_DIR:-$ROOT_DIR/env}"
MODE="${1:-all}"
if [ "$#" -gt 0 ]; then
    shift
fi

case "$MODE" in
    configure)
        BUILD_TARGETS=""
        BUILD_SCOPE="configure only"
        ;;
    kicad)
        BUILD_TARGETS="kicad kicad-cli"
        BUILD_SCOPE="KiCad executable and kicad-cli"
        ;;
    pybind11-kicad|native)
        BUILD_TARGETS="pybind11_kicad_native"
        BUILD_SCOPE="pybind11-kicad native module"
        ;;
    python)
        BUILD_TARGETS=""
        BUILD_SCOPE="Python environment"
        ;;
    all|build)
        BUILD_TARGETS="kicad kicad-cli pybind11_kicad_native"
        BUILD_SCOPE="KiCad executable, kicad-cli, and pybind11-kicad native module"
        ;;
    *)
        echo "usage: $0 [configure|kicad|pybind11-kicad|python|all] [python-args...]" >&2
        echo "       $0 python [-c command | -m module | script.py] [args...]" >&2
        exit 2
        ;;
esac

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command not found: $1" >&2
        exit 127
    fi
}

resolve_python_executable() {
    PYTHON_BIN="${PYBIND11_KICAD_PYTHON:-python3.14}"
    if command -v "$PYTHON_BIN" >/dev/null 2>&1; then
        PYTHON_EXECUTABLE=$(command -v "$PYTHON_BIN")
    elif [ -x "$PYTHON_BIN" ]; then
        PYTHON_EXECUTABLE="$PYTHON_BIN"
    else
        echo "Python 3.14 executable not found: $PYTHON_BIN" >&2
        exit 127
    fi

    PYTHON_VERSION=$("$PYTHON_EXECUTABLE" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    if [ "$PYTHON_VERSION" != "3.14" ]; then
        echo "Expected Python 3.14, got $PYTHON_VERSION from $PYTHON_EXECUTABLE" >&2
        exit 2
    fi
}

run_python_environment() {
    NATIVE_DIR="${PYBIND11_KICAD_NATIVE_BUILD_DIR:-$BUILD_DIR/pybind11-kicad-native}"

    if [ ! -d "$NATIVE_DIR" ]; then
        echo "Native module directory not found: $NATIVE_DIR" >&2
        echo "Run scripts/build.sh pybind11-kicad first, or set PYBIND11_KICAD_NATIVE_BUILD_DIR." >&2
        exit 2
    fi

    if [ ! -f "$PYTHON_ENV_DIR/pyvenv.cfg" ]; then
        mkdir -p "$(dirname -- "$PYTHON_ENV_DIR")"
        "$PYTHON_EXECUTABLE" -m venv "$PYTHON_ENV_DIR"
    fi

    if [ -x "$PYTHON_ENV_DIR/bin/python" ]; then
        PYTHON_EXECUTABLE="$PYTHON_ENV_DIR/bin/python"
    elif [ -x "$PYTHON_ENV_DIR/Scripts/python.exe" ]; then
        PYTHON_EXECUTABLE="$PYTHON_ENV_DIR/Scripts/python.exe"
    else
        echo "Python virtual environment is missing a Python executable: $PYTHON_ENV_DIR" >&2
        exit 2
    fi

    PYTHON_VERSION=$("$PYTHON_EXECUTABLE" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    if [ "$PYTHON_VERSION" != "3.14" ]; then
        echo "Expected $PYTHON_ENV_DIR to use Python 3.14, got $PYTHON_VERSION" >&2
        echo "Recreate the environment with Python 3.14 or set PYBIND11_KICAD_PYTHON_ENV_DIR." >&2
        exit 2
    fi

    VIRTUAL_ENV="$PYTHON_ENV_DIR"
    export VIRTUAL_ENV
    PATH="$(dirname -- "$PYTHON_EXECUTABLE"):$PATH"
    export PATH

    if [ -n "${PYTHONPATH:-}" ]; then
        export PYTHONPATH="$ROOT_DIR/python:$NATIVE_DIR:$PYTHONPATH"
    else
        export PYTHONPATH="$ROOT_DIR/python:$NATIVE_DIR"
    fi

    exec "$PYTHON_EXECUTABLE" "$@"
}

if [ "$MODE" = "python" ]; then
    resolve_python_executable
    run_python_environment "$@"
fi

if [ "$#" -gt 0 ]; then
    echo "Unexpected arguments for '$MODE'." >&2
    echo "usage: $0 [configure|kicad|pybind11-kicad|python|all] [python-args...]" >&2
    exit 2
fi

require_command git
require_command cmake

if [ ! -d "$SOURCE_DIR/.git" ]; then
    if [ "${PYBIND11_KICAD_FETCH_KICAD_SOURCE:-1}" = "0" ]; then
        echo "KiCad source checkout not found: $SOURCE_DIR" >&2
        exit 2
    fi

    mkdir -p "$(dirname -- "$SOURCE_DIR")"
    git clone --depth 1 --branch "$KICAD_GIT_TAG" --filter=blob:none \
        "$KICAD_GIT_REPOSITORY" "$SOURCE_DIR"
fi

ACTUAL_COMMIT=$(git -C "$SOURCE_DIR" rev-parse HEAD)
if [ "$ACTUAL_COMMIT" != "$KICAD_GIT_COMMIT" ]; then
    echo "KiCad source commit mismatch in $SOURCE_DIR" >&2
    echo "  expected: $KICAD_GIT_COMMIT" >&2
    echo "  actual:   $ACTUAL_COMMIT" >&2
    exit 2
fi

resolve_python_executable

PYTHON_INCLUDE_DIR=$("$PYTHON_EXECUTABLE" -c 'import sysconfig; print(sysconfig.get_config_var("INCLUDEPY") or "")')
PYTHON_LIBRARY=$("$PYTHON_EXECUTABLE" - <<'PY'
import os
import sysconfig

candidates = []
libdir = sysconfig.get_config_var("LIBDIR")
ldlibrary = sysconfig.get_config_var("LDLIBRARY")
if libdir and ldlibrary:
    candidates.append(os.path.join(libdir, ldlibrary))

fw = sysconfig.get_config_var("PYTHONFRAMEWORK")
fw_prefix = sysconfig.get_config_var("PYTHONFRAMEWORKPREFIX")
version = sysconfig.get_config_var("VERSION")
if fw and fw_prefix and version:
    candidates.append(os.path.join(fw_prefix, f"{fw}.framework", "Versions", version, fw))

for path in candidates:
    if path and os.path.exists(path):
        print(path)
        break
PY
)

if [ -z "$PYTHON_INCLUDE_DIR" ] || [ ! -d "$PYTHON_INCLUDE_DIR" ]; then
    echo "Could not resolve Python include directory for $PYTHON_EXECUTABLE" >&2
    exit 2
fi

if [ -z "$PYTHON_LIBRARY" ] || [ ! -e "$PYTHON_LIBRARY" ]; then
    echo "Could not resolve Python library for $PYTHON_EXECUTABLE" >&2
    exit 2
fi

PYTHON_FRAMEWORK=$("$PYTHON_EXECUTABLE" - <<'PY'
import os
import sysconfig

fw = sysconfig.get_config_var("PYTHONFRAMEWORK")
fw_prefix = sysconfig.get_config_var("PYTHONFRAMEWORKPREFIX")
if fw and fw_prefix:
    path = os.path.join(fw_prefix, f"{fw}.framework")
    if os.path.isdir(path):
        print(path)
PY
)

mkdir -p "$BUILD_DIR"
HOOK_FILE="$BUILD_DIR/pybind11-kicad-top-level-hook.cmake"
ADD_NATIVE_FILE="$BUILD_DIR/pybind11-kicad-add-native.cmake"

cat > "$HOOK_FILE" <<EOF
cmake_language(DEFER DIRECTORY "\${CMAKE_SOURCE_DIR}" CALL include "$ADD_NATIVE_FILE")
EOF

cat > "$ADD_NATIVE_FILE" <<EOF
set(PYBIND11_KICAD_TARGET_KICAD_MAJOR "$TARGET_KICAD_MAJOR" CACHE STRING "" FORCE)
set(PYBIND11_KICAD_TARGET_KICAD_VERSION "$TARGET_KICAD_VERSION" CACHE STRING "" FORCE)
set(PYBIND11_KICAD_KICAD_GIT_REPOSITORY "$KICAD_GIT_REPOSITORY" CACHE STRING "" FORCE)
set(PYBIND11_KICAD_KICAD_GIT_TAG "$KICAD_GIT_TAG" CACHE STRING "" FORCE)
set(PYBIND11_KICAD_KICAD_GIT_COMMIT "$KICAD_GIT_COMMIT" CACHE STRING "" FORCE)
set(PYBIND11_KICAD_KICAD_SOURCE_DIR "\${CMAKE_SOURCE_DIR}" CACHE PATH "" FORCE)
set(PYBIND11_KICAD_KICAD_BUILD_DIR "\${CMAKE_BINARY_DIR}" CACHE PATH "" FORCE)
set(PYBIND11_KICAD_FETCH_KICAD_SOURCE OFF CACHE BOOL "" FORCE)
set(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO ON CACHE BOOL "" FORCE)
set(PYBIND11_KICAD_NATIVE_OUTPUT_DIR "\${CMAKE_BINARY_DIR}/pybind11-kicad-native" CACHE PATH "" FORCE)
list(APPEND CMAKE_MODULE_PATH "$ROOT_DIR/native/cmake")
include(KiCadSource)
pybind11_kicad_resolve_kicad_source(PYBIND11_KICAD_RESOLVED_KICAD_SOURCE_DIR)
include(Pybind11KiCadNativeTargets)
pybind11_kicad_add_native_targets("$ROOT_DIR/native")
EOF

CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-RelWithDebInfo}"

set -- \
    -S "$SOURCE_DIR" \
    -B "$BUILD_DIR" \
    -G "$CMAKE_GENERATOR" \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$HOOK_FILE" \
    -DPYTHON_EXECUTABLE="$PYTHON_EXECUTABLE" \
    -DPYTHON_INCLUDE_DIR="$PYTHON_INCLUDE_DIR" \
    -DPYTHON_LIBRARY="$PYTHON_LIBRARY" \
    -DKICAD_SCRIPTING_WXPYTHON=OFF \
    -DKICAD_BUILD_QA_TESTS=OFF \
    -DKICAD_BUILD_I18N=OFF \
    -DKICAD_USE_SENTRY=OFF \
    -DKICAD_IPC_API=OFF \
    -DKICAD_BUILD_FLATPAK=OFF \
    -DKICAD_USE_PCH=OFF

if [ "$(uname -s)" = "Darwin" ]; then
    HOMEBREW_PREFIX="${PYBIND11_KICAD_HOMEBREW_PREFIX:-}"
    if [ -z "$HOMEBREW_PREFIX" ] && command -v brew >/dev/null 2>&1; then
        HOMEBREW_PREFIX=$(brew --prefix)
    fi

    if [ -n "$HOMEBREW_PREFIX" ]; then
        set -- "$@" -DCMAKE_PREFIX_PATH="$HOMEBREW_PREFIX"

        if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists ngspice; then
            NGSPICE_INCLUDE_DIR="${NGSPICE_INCLUDE_DIR:-$(pkg-config --variable=includedir ngspice)}"
            NGSPICE_LIBRARY="${NGSPICE_LIBRARY:-$HOMEBREW_PREFIX/lib/libngspice.dylib}"
            set -- "$@" \
                -DNGSPICE_INCLUDE_DIR="$NGSPICE_INCLUDE_DIR" \
                -DNGSPICE_LIBRARY="$NGSPICE_LIBRARY" \
                -DNGSPICE_DLL="$NGSPICE_LIBRARY" \
                -DNGSPICE_LIB_NAME=ngspice
        fi

        OCC_PREFIX="${OCC_PREFIX:-}"
        if [ -z "$OCC_PREFIX" ] && command -v brew >/dev/null 2>&1; then
            OCC_PREFIX=$(brew --prefix opencascade 2>/dev/null || true)
        fi

        if [ -n "$OCC_PREFIX" ]; then
            set -- "$@" \
                -DOCC_INCLUDE_DIR="$OCC_PREFIX/include/opencascade" \
                -DOCC_LIBRARY_DIR="$OCC_PREFIX/lib"
        fi
    fi

    if [ -n "$PYTHON_FRAMEWORK" ]; then
        set -- "$@" -DPYTHON_FRAMEWORK="$PYTHON_FRAMEWORK"
    fi

    set -- "$@" -DCMAKE_OSX_ARCHITECTURES="${CMAKE_OSX_ARCHITECTURES:-$(uname -m)}"
fi

echo "pybind11-kicad KiCad build:" >&2
echo "  KiCad version: $TARGET_KICAD_VERSION" >&2
echo "  KiCad major:   $TARGET_KICAD_MAJOR" >&2
echo "  KiCad tag:     $KICAD_GIT_TAG" >&2
echo "  KiCad commit:  $KICAD_GIT_COMMIT" >&2
echo "  Python:        $PYTHON_EXECUTABLE" >&2
echo "  Source dir:    $SOURCE_DIR" >&2
echo "  Build dir:     $BUILD_DIR" >&2
echo "  Build scope:   $BUILD_SCOPE" >&2

cmake "$@"

if [ -n "$BUILD_TARGETS" ]; then
    # Word splitting is intentional: subcommands select one or more CMake targets.
    # shellcheck disable=SC2086
    cmake --build "$BUILD_DIR" --target $BUILD_TARGETS --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-8}"
fi

echo "KiCad/native build dir: $BUILD_DIR" >&2
echo "KiCad CLI path: $BUILD_DIR/kicad/kicad-cli" >&2
echo "Native module dir: $BUILD_DIR/pybind11-kicad-native" >&2
