#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

CACHE_ROOT="${PYBIND11_KICAD_KIKAKUKA_CACHE_DIR:-$ROOT_DIR/.cache/kikakuka}"
KIKAKUKA_DIR="${PYBIND11_KICAD_KIKAKUKA_SOURCE:-${KIKAKUKA_SOURCE:-$ROOT_DIR/../kikakuka}}"
KIKAKUKA_SCRIPT="${PYBIND11_KICAD_KIKAKUKA_SCRIPT:-$KIKAKUKA_DIR/kikakuka.py}"
KIKAKUKA_PANEL="${PYBIND11_KICAD_KIKAKUKA_PANEL:-$KIKAKUKA_DIR/samples/L7.kikit_pnl}"
KIKAKUKA_OUTPUT="${PYBIND11_KICAD_KIKAKUKA_OUTPUT:-$CACHE_ROOT/out.kicad_pcb}"
KIKAKUKA_VENV="${PYBIND11_KICAD_KIKAKUKA_VENV:-}"
KIKAKUKA_PYTHON="${PYBIND11_KICAD_KIKAKUKA_PYTHON:-}"
NATIVE_BUILD_DIR="${PYBIND11_KICAD_NATIVE_BUILD_DIR:-}"

if [ -n "$KIKAKUKA_VENV" ]; then
    if [ ! -x "$KIKAKUKA_VENV/bin/python" ]; then
        echo "PYBIND11_KICAD_KIKAKUKA_VENV does not contain bin/python: $KIKAKUKA_VENV" >&2
        exit 2
    fi
    PATH="$KIKAKUKA_VENV/bin:$PATH"
    export PATH
    if [ -z "$KIKAKUKA_PYTHON" ]; then
        KIKAKUKA_PYTHON="$KIKAKUKA_VENV/bin/python"
    fi
fi

if [ -z "$KIKAKUKA_PYTHON" ]; then
    if command -v python3.14 >/dev/null 2>&1; then
        KIKAKUKA_PYTHON="$(command -v python3.14)"
    elif command -v python3 >/dev/null 2>&1; then
        KIKAKUKA_PYTHON="$(command -v python3)"
    elif command -v python >/dev/null 2>&1; then
        KIKAKUKA_PYTHON="$(command -v python)"
    else
        echo "No Python interpreter found for Kikakuka test." >&2
        exit 127
    fi
fi

if [ ! -f "$KIKAKUKA_SCRIPT" ]; then
    echo "Kikakuka script not found: $KIKAKUKA_SCRIPT" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_SOURCE or PYBIND11_KICAD_KIKAKUKA_SCRIPT." >&2
    exit 2
fi

if [ ! -f "$KIKAKUKA_PANEL" ]; then
    echo "Kikakuka panel fixture not found: $KIKAKUKA_PANEL" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_PANEL." >&2
    exit 2
fi

if [ -n "$NATIVE_BUILD_DIR" ] && [ ! -d "$NATIVE_BUILD_DIR" ]; then
    echo "Native build directory not found: $NATIVE_BUILD_DIR" >&2
    echo "Set PYBIND11_KICAD_NATIVE_BUILD_DIR to a build directory containing pybind11_kicad_native." >&2
    exit 2
fi

mkdir -p "$CACHE_ROOT" "$(dirname -- "$KIKAKUKA_OUTPUT")"

if [ -n "$NATIVE_BUILD_DIR" ]; then
    LOCAL_PYTHONPATH="$ROOT_DIR/python:$NATIVE_BUILD_DIR"
else
    LOCAL_PYTHONPATH="$ROOT_DIR/python"
fi

if [ -n "${PYTHONPATH:-}" ]; then
    export PYTHONPATH="$LOCAL_PYTHONPATH:$PYTHONPATH"
else
    export PYTHONPATH="$LOCAL_PYTHONPATH"
fi

echo "Running Kikakuka panel smoke test:" >&2
echo "  python:  $KIKAKUKA_PYTHON" >&2
echo "  script:  $KIKAKUKA_SCRIPT" >&2
echo "  panel:   $KIKAKUKA_PANEL" >&2
echo "  output:  $KIKAKUKA_OUTPUT" >&2
if [ -n "$NATIVE_BUILD_DIR" ]; then
    echo "  native:  $NATIVE_BUILD_DIR" >&2
fi

exec "$KIKAKUKA_PYTHON" "$KIKAKUKA_SCRIPT" "$KIKAKUKA_PANEL" "$KIKAKUKA_OUTPUT"
