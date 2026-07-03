#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

TARGET_KICAD_MAJOR="${PYBIND11_KICAD_TARGET_KICAD_MAJOR:-10}"
TARGET_KICAD_VERSION="${PYBIND11_KICAD_TARGET_KICAD_VERSION:-10.0.4}"
KICAD_GIT_COMMIT="${PYBIND11_KICAD_KICAD_GIT_COMMIT:-f7414d419cae5df2d00e7eaacb16fc0e803799bc}"
SOURCE_DIR="${PYBIND11_KICAD_KICAD_SOURCE_DIR:-$ROOT_DIR/tmp/kicad}"
OUTPUT_DIR="${PYBIND11_KICAD_DEPS_OUTPUT_DIR:-$ROOT_DIR/deps/kicad-$TARGET_KICAD_VERSION}"
PYTHON_BIN="${PYBIND11_KICAD_DEPS_PYTHON:-python3}"

if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    echo "Python executable not found: $PYTHON_BIN" >&2
    exit 127
fi

if [ ! -d "$SOURCE_DIR" ]; then
    echo "KiCad source checkout not found: $SOURCE_DIR" >&2
    echo "Run scripts/build.sh configure first, or set PYBIND11_KICAD_KICAD_SOURCE_DIR." >&2
    exit 2
fi

exec "$PYTHON_BIN" "$ROOT_DIR/scripts/convert-kicad-deps.py" \
    --kicad-source "$SOURCE_DIR" \
    --output-dir "$OUTPUT_DIR" \
    --target-version "$TARGET_KICAD_VERSION" \
    --target-major "$TARGET_KICAD_MAJOR" \
    --commit "$KICAD_GIT_COMMIT"
