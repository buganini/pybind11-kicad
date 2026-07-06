#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
KIKIT_LOCK_FILE="$ROOT_DIR/compat/kikit.lock"

if [ -f "$KIKIT_LOCK_FILE" ]; then
    . "$KIKIT_LOCK_FILE"
fi

CACHE_ROOT="${PYBIND11_KICAD_KIKAKUKA_CACHE_DIR:-$ROOT_DIR/.cache/kikakuka}"
HEADLESS_PUI_PATH="$ROOT_DIR/tests/support/kikakuka_headless"
KIKIT_CACHE_ROOT="${PYBIND11_KICAD_COMPAT_DIR:-$ROOT_DIR/.cache/kikit}"
KIKIT_DIR="${KIKIT_SOURCE:-$KIKIT_CACHE_ROOT/KiKit}"
KIKAKUKA_DIR="${PYBIND11_KICAD_KIKAKUKA_SOURCE:-${KIKAKUKA_SOURCE:-$ROOT_DIR/../kikakuka}}"
KIKAKUKA_SCRIPT="${PYBIND11_KICAD_KIKAKUKA_SCRIPT:-$KIKAKUKA_DIR/kikakuka.py}"
KIKAKUKA_SAMPLES_DIR="${PYBIND11_KICAD_KIKAKUKA_SAMPLES_DIR:-$KIKAKUKA_DIR/samples}"
KIKAKUKA_GERBER_EXPORT="${PYBIND11_KICAD_KIKAKUKA_GERBER_EXPORT:-$KIKAKUKA_SAMPLES_DIR/gerber/export}"
KIKAKUKA_LOCAL_RUNNER="${PYBIND11_KICAD_KIKAKUKA_LOCAL_RUNNER:-$ROOT_DIR/scripts/run.sh}"
KIKAKUKA_LOCAL_PYTHON_ENV_DIR="${PYBIND11_KICAD_KIKAKUKA_VENV:-${PYBIND11_KICAD_PYTHON_ENV_DIR:-}}"
KIKAKUKA_LOCAL_PYTHON="${PYBIND11_KICAD_KIKAKUKA_PYTHON:-}"
KIKAKUKA_GOLDEN_PYTHON="${PYBIND11_KICAD_KIKAKUKA_GOLDEN_PYTHON:-$ROOT_DIR/..//kikakuka/env/bin/python}"
KIKAKUKA_GOLDEN_PYTHONPATH="${PYBIND11_KICAD_KIKAKUKA_GOLDEN_PYTHONPATH:-}"
KIKAKUKA_REQUIREMENTS="${PYBIND11_KICAD_KIKAKUKA_REQUIREMENTS:-$KIKAKUKA_DIR/requirements.txt}"
KIKAKUKA_EXTRA_REQUIREMENT="${PYBIND11_KICAD_KIKAKUKA_EXTRA_REQUIREMENT-wxPython==4.2.5}"
KIKAKUKA_INSTALL_DEPS="${PYBIND11_KICAD_KIKAKUKA_INSTALL_DEPS:-1}"
COMPARE_SCRIPT="${PYBIND11_KICAD_KIKAKUKA_COMPARE_SCRIPT:-$ROOT_DIR/scripts/compare-kikakuka-boards.py}"
DEFAULT_NATIVE_BUILD_DIR="$ROOT_DIR/tmp/pybind11-kicad-10-build/pybind11-kicad-native"
NATIVE_BUILD_DIR="${PYBIND11_KICAD_NATIVE_BUILD_DIR:-}"
if [ -z "$NATIVE_BUILD_DIR" ] && [ -d "$DEFAULT_NATIVE_BUILD_DIR" ]; then
    NATIVE_BUILD_DIR="$DEFAULT_NATIVE_BUILD_DIR"
fi

if [ ! -x "$KIKAKUKA_LOCAL_RUNNER" ]; then
    echo "Local pybind11-kicad runner not found or not executable: $KIKAKUKA_LOCAL_RUNNER" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_LOCAL_RUNNER." >&2
    exit 2
fi

if [ -n "$KIKAKUKA_LOCAL_PYTHON_ENV_DIR" ]; then
    if [ ! -x "$KIKAKUKA_LOCAL_PYTHON_ENV_DIR/bin/python" ] && [ ! -x "$KIKAKUKA_LOCAL_PYTHON_ENV_DIR/Scripts/python.exe" ]; then
        echo "Configured local Python env has no Python executable: $KIKAKUKA_LOCAL_PYTHON_ENV_DIR" >&2
        echo "Set PYBIND11_KICAD_KIKAKUKA_VENV or PYBIND11_KICAD_PYTHON_ENV_DIR." >&2
        exit 2
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

if [ "$KIKAKUKA_INSTALL_DEPS" != "0" ] && [ ! -f "$KIKAKUKA_REQUIREMENTS" ]; then
    echo "Kikakuka requirements file not found: $KIKAKUKA_REQUIREMENTS" >&2
    echo "Set PYBIND11_KICAD_KIKAKUKA_REQUIREMENTS, or set PYBIND11_KICAD_KIKAKUKA_INSTALL_DEPS=0." >&2
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

if [ -d "$KIKIT_DIR/kikit" ]; then
    KIKIT_PYTHONPATH="$KIKIT_DIR"
else
    KIKIT_PYTHONPATH=""
fi

mkdir -p "$CACHE_ROOT/golden" "$CACHE_ROOT/local" "$CACHE_ROOT/logs" "$CACHE_ROOT/diffs"

LOCAL_PYTHONPATH="$HEADLESS_PUI_PATH"
if [ -n "$KIKIT_PYTHONPATH" ]; then
    LOCAL_PYTHONPATH="$LOCAL_PYTHONPATH:$KIKIT_PYTHONPATH"
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
echo "  local runner:  $KIKAKUKA_LOCAL_RUNNER python" >&2
if [ -n "$KIKAKUKA_LOCAL_PYTHON_ENV_DIR" ]; then
    echo "  local env:     $KIKAKUKA_LOCAL_PYTHON_ENV_DIR" >&2
fi
if [ -n "$KIKAKUKA_LOCAL_PYTHON" ]; then
    echo "  local python:  $KIKAKUKA_LOCAL_PYTHON" >&2
fi
echo "  golden python: $KIKAKUKA_GOLDEN_PYTHON" >&2
echo "  script:        $KIKAKUKA_SCRIPT" >&2
echo "  samples:       $KIKAKUKA_SAMPLES_DIR" >&2
echo "  gerber export: $KIKAKUKA_GERBER_EXPORT" >&2
echo "  cache:         $CACHE_ROOT" >&2
echo "  compare:       $COMPARE_SCRIPT" >&2
if [ "$KIKAKUKA_INSTALL_DEPS" != "0" ]; then
    echo "  requirements:  $KIKAKUKA_REQUIREMENTS" >&2
    if [ -n "$KIKAKUKA_EXTRA_REQUIREMENT" ]; then
        echo "  extra:         $KIKAKUKA_EXTRA_REQUIREMENT" >&2
    fi
fi
if [ -n "$KIKIT_PYTHONPATH" ]; then
    echo "  kikit:         $KIKIT_PYTHONPATH" >&2
fi
if [ -n "$NATIVE_BUILD_DIR" ]; then
    echo "  native:        $NATIVE_BUILD_DIR" >&2
fi

TOTAL=0
FAILED=0

run_local_python_base() (
    PYTHONPATH="$TEST_PYTHONPATH"
    export PYTHONPATH

    if [ -n "$NATIVE_BUILD_DIR" ]; then
        PYBIND11_KICAD_NATIVE_BUILD_DIR="$NATIVE_BUILD_DIR"
        export PYBIND11_KICAD_NATIVE_BUILD_DIR
    fi

    if [ -n "$KIKAKUKA_LOCAL_PYTHON_ENV_DIR" ]; then
        PYBIND11_KICAD_PYTHON_ENV_DIR="$KIKAKUKA_LOCAL_PYTHON_ENV_DIR"
        export PYBIND11_KICAD_PYTHON_ENV_DIR
    fi

    if [ -n "$KIKAKUKA_LOCAL_PYTHON" ]; then
        PYBIND11_KICAD_PYTHON="$KIKAKUKA_LOCAL_PYTHON"
        export PYBIND11_KICAD_PYTHON
    fi

    exec "$KIKAKUKA_LOCAL_RUNNER" python "$@"
)

run_local_python() (
    PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI=1
    export PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI

    run_local_python_base "$@"
)

install_local_requirements() {
    if [ "$KIKAKUKA_INSTALL_DEPS" = "0" ]; then
        return
    fi

    echo "Installing Kikakuka local test dependencies:" >&2
    echo "  $KIKAKUKA_REQUIREMENTS" >&2
    run_local_python_base -m pip install -r "$KIKAKUKA_REQUIREMENTS" >&2

    if [ -n "$KIKAKUKA_EXTRA_REQUIREMENT" ]; then
        echo "Installing Kikakuka extra test dependency:" >&2
        echo "  $KIKAKUKA_EXTRA_REQUIREMENT" >&2
        run_local_python_base -m pip install "$KIKAKUKA_EXTRA_REQUIREMENT" >&2
    fi
}

install_local_requirements

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

    if run_local_python "$KIKAKUKA_SCRIPT" "$CASE_INPUT" "$LOCAL_OUTPUT" >"$LOCAL_LOG" 2>&1; then
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

    if run_local_python "$COMPARE_SCRIPT" "$GOLDEN_OUTPUT" "$LOCAL_OUTPUT" >"$DIFF_FILE" 2>&1; then
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
