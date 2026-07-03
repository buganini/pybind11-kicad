#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

CACHE_ROOT="${PYBIND11_KICAD_KIKAKUKA_CACHE_DIR:-$ROOT_DIR/.cache/kikakuka}"
HEADLESS_PUI_PATH="$ROOT_DIR/tests/support/kikakuka_headless"
KIKAKUKA_DIR="${PYBIND11_KICAD_KIKAKUKA_SOURCE:-${KIKAKUKA_SOURCE:-$ROOT_DIR/../kikakuka}}"
KIKAKUKA_SCRIPT="${PYBIND11_KICAD_KIKAKUKA_SCRIPT:-$KIKAKUKA_DIR/kikakuka.py}"
KIKAKUKA_SAMPLES_DIR="${PYBIND11_KICAD_KIKAKUKA_SAMPLES_DIR:-$KIKAKUKA_DIR/samples}"
KIKAKUKA_GERBER_EXPORT="${PYBIND11_KICAD_KIKAKUKA_GERBER_EXPORT:-$KIKAKUKA_SAMPLES_DIR/gerber/export}"
KIKAKUKA_VENV="${PYBIND11_KICAD_KIKAKUKA_VENV:-}"
KIKAKUKA_PYTHON="${PYBIND11_KICAD_KIKAKUKA_PYTHON:-}"
KIKAKUKA_GOLDEN_PYTHON="${PYBIND11_KICAD_KIKAKUKA_GOLDEN_PYTHON:-$ROOT_DIR/..//kikakuka/env/bin/python}"
KIKAKUKA_GOLDEN_PYTHONPATH="${PYBIND11_KICAD_KIKAKUKA_GOLDEN_PYTHONPATH:-}"
COMPARE_SCRIPT="${PYBIND11_KICAD_KIKAKUKA_COMPARE_SCRIPT:-$ROOT_DIR/scripts/compare-kikakuka-boards.py}"
DEFAULT_NATIVE_BUILD_DIR="$ROOT_DIR/tmp/pybind11-kicad-10-build/pybind11-kicad-native"
NATIVE_BUILD_DIR="${PYBIND11_KICAD_NATIVE_BUILD_DIR:-}"
if [ -z "$NATIVE_BUILD_DIR" ] && [ -d "$DEFAULT_NATIVE_BUILD_DIR" ]; then
    NATIVE_BUILD_DIR="$DEFAULT_NATIVE_BUILD_DIR"
fi

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
    if [ -x "$ROOT_DIR/env/bin/python" ]; then
        KIKAKUKA_PYTHON="$ROOT_DIR/env/bin/python"
    elif command -v python3.14 >/dev/null 2>&1; then
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

if [ ! -d "$KIKAKUKA_SAMPLES_DIR" ]; then
    echo "Kikakuka samples directory not found: $KIKAKUKA_SAMPLES_DIR" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_SAMPLES_DIR." >&2
    exit 2
fi

if [ ! -d "$KIKAKUKA_GERBER_EXPORT" ]; then
    echo "Kikakuka Gerber export directory not found: $KIKAKUKA_GERBER_EXPORT" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_GERBER_EXPORT." >&2
    exit 2
fi

if [ ! -x "$KIKAKUKA_GOLDEN_PYTHON" ]; then
    echo "Golden Kikakuka Python runner not found or not executable: $KIKAKUKA_GOLDEN_PYTHON" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_GOLDEN_PYTHON." >&2
    exit 2
fi

if [ -n "$NATIVE_BUILD_DIR" ] && [ ! -d "$NATIVE_BUILD_DIR" ]; then
    echo "Native build directory not found: $NATIVE_BUILD_DIR" >&2
    echo "Set PYBIND11_KICAD_NATIVE_BUILD_DIR to a build directory containing pybind11_kicad_native." >&2
    exit 2
fi

if [ ! -f "$COMPARE_SCRIPT" ]; then
    echo "Kikakuka board comparison script not found: $COMPARE_SCRIPT" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_COMPARE_SCRIPT." >&2
    exit 2
fi

mkdir -p "$CACHE_ROOT/golden" "$CACHE_ROOT/local" "$CACHE_ROOT/logs" "$CACHE_ROOT/diffs"

if [ -n "$NATIVE_BUILD_DIR" ]; then
    LOCAL_PYTHONPATH="$HEADLESS_PUI_PATH:$ROOT_DIR/python:$NATIVE_BUILD_DIR"
else
    LOCAL_PYTHONPATH="$HEADLESS_PUI_PATH:$ROOT_DIR/python"
fi

ORIGINAL_PYTHONPATH="${PYTHONPATH:-}"
if [ -n "$ORIGINAL_PYTHONPATH" ]; then
    TEST_PYTHONPATH="$LOCAL_PYTHONPATH:$ORIGINAL_PYTHONPATH"
else
    TEST_PYTHONPATH="$LOCAL_PYTHONPATH"
fi

if [ -n "$KIKAKUKA_GOLDEN_PYTHONPATH" ]; then
    GOLDEN_TEST_PYTHONPATH="$HEADLESS_PUI_PATH:$KIKAKUKA_GOLDEN_PYTHONPATH"
else
    GOLDEN_TEST_PYTHONPATH="$HEADLESS_PUI_PATH"
fi

PANEL_LIST="$CACHE_ROOT/panels.txt"
find "$KIKAKUKA_SAMPLES_DIR" -type f -name '*.kikit_pnl' | sort > "$PANEL_LIST"

if [ ! -s "$PANEL_LIST" ]; then
    echo "No .kikit_pnl files found under: $KIKAKUKA_SAMPLES_DIR" >&2
    exit 2
fi

echo "Running Kikakuka comparison tests:" >&2
echo "  local python:  $KIKAKUKA_PYTHON" >&2
echo "  golden python: $KIKAKUKA_GOLDEN_PYTHON" >&2
echo "  script:        $KIKAKUKA_SCRIPT" >&2
echo "  samples:       $KIKAKUKA_SAMPLES_DIR" >&2
echo "  gerber export: $KIKAKUKA_GERBER_EXPORT" >&2
echo "  cache:         $CACHE_ROOT" >&2
echo "  compare:       $COMPARE_SCRIPT" >&2
if [ -n "$NATIVE_BUILD_DIR" ]; then
    echo "  native:        $NATIVE_BUILD_DIR" >&2
fi

TOTAL=0
FAILED=0

run_case() {
    CASE_KIND="$1"
    CASE_NAME="$2"
    CASE_INPUT="$3"

    TOTAL=$((TOTAL + 1))
    GOLDEN_OUTPUT="$CACHE_ROOT/golden/$CASE_NAME.kicad_pcb"
    LOCAL_OUTPUT="$CACHE_ROOT/local/$CASE_NAME.kicad_pcb"
    GOLDEN_LOG="$CACHE_ROOT/logs/$CASE_NAME.golden.log"
    LOCAL_LOG="$CACHE_ROOT/logs/$CASE_NAME.local.log"
    DIFF_FILE="$CACHE_ROOT/diffs/$CASE_NAME.diff"

    echo "[$TOTAL] $CASE_NAME ($CASE_KIND)" >&2
    rm -f "$GOLDEN_OUTPUT" "$LOCAL_OUTPUT" "$DIFF_FILE"

    if PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI=1 PYTHONPATH="$GOLDEN_TEST_PYTHONPATH" "$KIKAKUKA_GOLDEN_PYTHON" "$KIKAKUKA_SCRIPT" "$CASE_INPUT" "$GOLDEN_OUTPUT" >"$GOLDEN_LOG" 2>&1; then
        :
    else
        echo "  golden failed: $GOLDEN_LOG" >&2
        FAILED=$((FAILED + 1))
        return
    fi
    if [ ! -f "$GOLDEN_OUTPUT" ]; then
        echo "  golden produced no output: $GOLDEN_LOG" >&2
        FAILED=$((FAILED + 1))
        return
    fi

    if PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI=1 PYTHONPATH="$TEST_PYTHONPATH" "$KIKAKUKA_PYTHON" "$KIKAKUKA_SCRIPT" "$CASE_INPUT" "$LOCAL_OUTPUT" >"$LOCAL_LOG" 2>&1; then
        :
    else
        echo "  local failed:  $LOCAL_LOG" >&2
        FAILED=$((FAILED + 1))
        return
    fi
    if [ ! -f "$LOCAL_OUTPUT" ]; then
        echo "  local produced no output:  $LOCAL_LOG" >&2
        FAILED=$((FAILED + 1))
        return
    fi

    if PYTHONPATH="$TEST_PYTHONPATH" "$KIKAKUKA_PYTHON" "$COMPARE_SCRIPT" "$GOLDEN_OUTPUT" "$LOCAL_OUTPUT" >"$DIFF_FILE" 2>&1; then
        rm -f "$DIFF_FILE"
        echo "  ok" >&2
    else
        echo "  board content differs: $DIFF_FILE" >&2
        FAILED=$((FAILED + 1))
    fi
}

while IFS= read -r PANEL; do
    NAME=$(basename -- "$PANEL" .kikit_pnl)
    run_case "panel" "$NAME" "$PANEL"
done < "$PANEL_LIST"

run_case "gerber conversion" "gerber_export" "$KIKAKUKA_GERBER_EXPORT"

if [ "$FAILED" -ne 0 ]; then
    echo "Kikakuka comparison failed: $FAILED of $TOTAL sample(s)." >&2
    exit 1
fi

echo "Kikakuka comparison passed: $TOTAL sample(s)." >&2
