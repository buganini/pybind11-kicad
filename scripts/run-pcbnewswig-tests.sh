#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
UPSTREAM_QA_DIR="${PYBIND11_KICAD_PCBNEWSWIG_QA_DIR:-$ROOT_DIR/tests/upstream}"
TEST_DIR="${PYBIND11_KICAD_PCBNEWSWIG_TEST_DIR:-$UPSTREAM_QA_DIR/pcbnewswig}"
TEST_PATH="${PYBIND11_KICAD_PCBNEWSWIG_TEST_PATH:-.}"

if ! "$ROOT_DIR/scripts/build.sh" python -c "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('pytest') else 1)"; then
    echo "pytest is required for the vendored KiCad pcbnewswig suite." >&2
    echo "Install it in the project Python environment with:" >&2
    echo "  $ROOT_DIR/scripts/build.sh python -m pip install -r $ROOT_DIR/compat/pcbnewswig-test-requirements.txt" >&2
    exit 127
fi

cd "$TEST_DIR"
exec "$ROOT_DIR/scripts/build.sh" python -m pytest "$TEST_PATH" "$@"
