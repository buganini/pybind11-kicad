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
    chmod +x "$WRAPPER_DIR/kikit" "$WRAPPER_DIR/kikit-info"
    export PATH="$WRAPPER_DIR:$PATH"
fi

if [ -n "${PYTHONPATH:-}" ]; then
    export PYTHONPATH="$ROOT_DIR/python:$KIKIT_DIR:$PYTHONPATH"
else
    export PYTHONPATH="$ROOT_DIR/python:$KIKIT_DIR"
fi

if [ -n "${PYTEST_ADDOPTS:-}" ]; then
    export PYTEST_ADDOPTS="-o cache_dir=$PYTEST_CACHE_DIR $PYTEST_ADDOPTS"
else
    export PYTEST_ADDOPTS="-o cache_dir=$PYTEST_CACHE_DIR"
fi

case "$MODE" in
    all)
        exec make -C "$KIKIT_DIR" test
        ;;
    unit)
        exec make -C "$KIKIT_DIR" test-unit
        ;;
    system)
        exec make -C "$KIKIT_DIR" test-system
        ;;
esac
