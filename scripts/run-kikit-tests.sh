#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LOCK_FILE="$ROOT_DIR/compat/kikit.lock"
MODE="${1:-all}"

case "$MODE" in
    all|unit|system)
        ;;
    *)
        echo "usage: $0 [all|unit|system]" >&2
        exit 2
        ;;
esac

if [ ! -f "$LOCK_FILE" ]; then
    echo "Missing KiKit lock file: $LOCK_FILE" >&2
    exit 2
fi

. "$LOCK_FILE"

CACHE_ROOT="${PYBIND11_KICAD_COMPAT_DIR:-$ROOT_DIR/.cache/kikit}"
KIKIT_DIR="${KIKIT_SOURCE:-$CACHE_ROOT/KiKit}"
PYTEST_CACHE_DIR="$CACHE_ROOT/pytest-cache"
WRAPPER_DIR="$CACHE_ROOT/bin"
KIKIT_TEST_PYTHON="${PYBIND11_KICAD_KIKIT_PYTHON:-}"
NATIVE_BUILD_DIR="${PYBIND11_KICAD_NATIVE_BUILD_DIR:-$ROOT_DIR/tmp/pybind11-kicad-10-build/pybind11-kicad-native}"
KICAD_BUILD_DIR="${PYBIND11_KICAD_BUILD_DIR:-$ROOT_DIR/tmp/pybind11-kicad-10-build}"

if [ -n "${KIKIT_SOURCE:-}" ]; then
    if [ "$(git -C "$KIKIT_DIR" rev-parse HEAD)" != "$KIKIT_COMMIT" ]; then
        echo "KIKIT_SOURCE is not at pinned KiKit commit $KIKIT_COMMIT" >&2
        exit 2
    fi
elif [ ! -d "$KIKIT_DIR/.git" ]; then
    mkdir -p "$CACHE_ROOT"
    echo "Fetching pinned KiKit $KIKIT_LABEL into $KIKIT_DIR" >&2
    git init "$KIKIT_DIR"
    git -C "$KIKIT_DIR" remote add origin "$KIKIT_URL"
    git -C "$KIKIT_DIR" fetch --depth 1 origin "$KIKIT_COMMIT"
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
elif ! git -C "$KIKIT_DIR" cat-file -e "$KIKIT_COMMIT^{commit}" 2>/dev/null; then
    git -C "$KIKIT_DIR" fetch --depth 1 origin "$KIKIT_COMMIT"
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
elif [ "$(git -C "$KIKIT_DIR" rev-parse HEAD)" != "$KIKIT_COMMIT" ]; then
    if [ -n "$(git -C "$KIKIT_DIR" status --porcelain)" ]; then
        echo "Cached KiKit checkout is dirty: $KIKIT_DIR" >&2
        echo "Clean it or set PYBIND11_KICAD_COMPAT_DIR to a fresh cache." >&2
        exit 2
    fi
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
fi

mkdir -p "$CACHE_ROOT" "$PYTEST_CACHE_DIR"

if [ -z "$KIKIT_TEST_PYTHON" ]; then
    if command -v python3.14 >/dev/null 2>&1; then
        KIKIT_TEST_PYTHON="$(command -v python3.14)"
    elif command -v python3 >/dev/null 2>&1; then
        KIKIT_TEST_PYTHON="$(command -v python3)"
    elif command -v python >/dev/null 2>&1; then
        KIKIT_TEST_PYTHON="$(command -v python)"
    else
        echo "No Python interpreter found for KiKit compatibility tests." >&2
        exit 127
    fi
fi

if [ "$MODE" = "unit" ] || [ "$MODE" = "all" ]; then
    if ! command -v pytest >/dev/null 2>&1; then
        echo "pytest was not found on PATH." >&2
        echo "Install KiKit test dependencies, for example:" >&2
        echo "  python3.14 -m pip install -r $ROOT_DIR/compat/kikit-test-requirements.txt" >&2
        exit 127
    fi
fi

if [ "$MODE" = "system" ] || [ "$MODE" = "all" ]; then
    if ! command -v bats >/dev/null 2>&1; then
        echo "bats was not found on PATH." >&2
        echo "Install bats-core with your system package manager before running KiKit system tests." >&2
        exit 127
    fi

    if [ ! -d "$NATIVE_BUILD_DIR" ]; then
        echo "KiKit system tests require the KiCad-backed native module directory." >&2
        echo "Build it with scripts/run.sh build-pybind11-kicad or set:" >&2
        echo "  PYBIND11_KICAD_NATIVE_BUILD_DIR=/path/to/pybind11-kicad-native" >&2
        exit 127
    fi

    if ! "$KIKIT_TEST_PYTHON" -c "import wx" >/dev/null 2>&1; then
        echo "wxPython was not found for $KIKIT_TEST_PYTHON." >&2
        echo "Install KiKit test dependencies, for example:" >&2
        echo "  python3.14 -m pip install -r $ROOT_DIR/compat/kikit-test-requirements.txt" >&2
        exit 127
    fi

    mkdir -p "$WRAPPER_DIR"
    cat > "$WRAPPER_DIR/kikit" <<EOF
#!/usr/bin/env sh
exec "$KIKIT_TEST_PYTHON" -c 'from kikit.ui import cli; cli()' "\$@"
EOF
    cat > "$WRAPPER_DIR/kikit-info" <<EOF
#!/usr/bin/env sh
exec "$KIKIT_TEST_PYTHON" -c 'from kikit.info import cli; cli()' "\$@"
EOF
    rm -f "$WRAPPER_DIR/openscad"
    chmod +x "$WRAPPER_DIR/kikit" "$WRAPPER_DIR/kikit-info"
    export PATH="$WRAPPER_DIR:$PATH"

    for candidate in \
        "$KICAD_BUILD_DIR/kicad" \
        "$KICAD_BUILD_DIR/kicad/KiCad.app/Contents/MacOS"
    do
        if [ -x "$candidate/kicad-cli" ] || [ -x "$candidate/kicad-cli.exe" ]; then
            if "$candidate/kicad-cli" version >/dev/null 2>&1; then
                export PATH="$candidate:$PATH"
            elif [ -x "$candidate/kicad-cli.exe" ] && "$candidate/kicad-cli.exe" version >/dev/null 2>&1; then
                export PATH="$candidate:$PATH"
            fi
        fi
    done
fi

PYBIND11_KICAD_TEST_PYTHONPATH="$ROOT_DIR/python:$KIKIT_DIR"

if [ -d "$NATIVE_BUILD_DIR" ]; then
    PYBIND11_KICAD_TEST_PYTHONPATH="$PYBIND11_KICAD_TEST_PYTHONPATH:$NATIVE_BUILD_DIR"
fi

if [ -n "${PYTHONPATH:-}" ]; then
    export PYTHONPATH="$PYBIND11_KICAD_TEST_PYTHONPATH:$PYTHONPATH"
else
    export PYTHONPATH="$PYBIND11_KICAD_TEST_PYTHONPATH"
fi

if [ -n "${PYTEST_ADDOPTS:-}" ]; then
    export PYTEST_ADDOPTS="-o cache_dir=$PYTEST_CACHE_DIR $PYTEST_ADDOPTS"
else
    export PYTEST_ADDOPTS="-o cache_dir=$PYTEST_CACHE_DIR"
fi

run_kikit_system_tests() {
    mkdir -p "$KIKIT_DIR/build/test"
    (
        cd "$KIKIT_DIR/build/test"
        set --

        for test_file in ../../test/system/*.bats; do
            case "$test_file" in
                */stencil.bats)
                    continue
                    ;;
            esac

            set -- "$@" "$test_file"
        done

        bats "$@"
    )
}

case "$MODE" in
    all)
        run_kikit_system_tests
        exec make -C "$KIKIT_DIR" test-unit
        ;;
    unit)
        exec make -C "$KIKIT_DIR" test-unit
        ;;
    system)
        run_kikit_system_tests
        ;;
esac
