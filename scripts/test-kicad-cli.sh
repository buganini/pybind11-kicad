#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
EXPECTED_VERSION="${PYBIND11_KICAD_EXPECTED_KICAD_CLI_VERSION:-${PYBIND11_KICAD_TARGET_KICAD_VERSION:-10.0.4}}"
ERROR_LOG="${TMPDIR:-/tmp}/pybind11-kicad-kicad-cli-test-$$.log"
TEST_BOARD="${PYBIND11_KICAD_KICAD_CLI_TEST_BOARD:-$ROOT_DIR/tests/golden/simple_board.kicad_pcb}"
TEST_BOARD_PRL="${TEST_BOARD%.*}.kicad_prl"
TEST_PDF="${PYBIND11_KICAD_KICAD_CLI_TEST_PDF:-$ROOT_DIR/tmp/kicad-cli-test-$$.pdf}"
KEEP_TEST_PDF=0
REMOVE_TEST_BOARD_PRL=0

if [ -n "${PYBIND11_KICAD_KICAD_CLI_TEST_PDF:-}" ]; then
    KEEP_TEST_PDF=1
fi

cleanup() {
    rm -f "$ERROR_LOG"

    if [ "$REMOVE_TEST_BOARD_PRL" -eq 1 ]; then
        rm -f "$TEST_BOARD_PRL"
    fi

    if [ "$KEEP_TEST_PDF" -eq 0 ]; then
        rm -f "$TEST_PDF"
    fi
}
trap cleanup EXIT HUP INT TERM

if [ ! -f "$TEST_BOARD" ]; then
    echo "KiCad CLI test board not found: $TEST_BOARD" >&2
    exit 2
fi

if [ ! -e "$TEST_BOARD_PRL" ]; then
    REMOVE_TEST_BOARD_PRL=1
fi

mkdir -p "$(dirname -- "$TEST_PDF")"

set +e
VERSION_OUTPUT=$("$ROOT_DIR/scripts/run.sh" kicad-cli version 2>"$ERROR_LOG")
STATUS=$?
set -e

if [ "$STATUS" -ne 0 ]; then
    echo "Self-built kicad-cli failed: scripts/run.sh kicad-cli version" >&2
    echo "Exit status: $STATUS" >&2

    if [ -s "$ERROR_LOG" ]; then
        echo "stderr:" >&2
        sed 's/^/  /' "$ERROR_LOG" >&2
    fi

    exit "$STATUS"
fi

if [ "$VERSION_OUTPUT" != "$EXPECTED_VERSION" ]; then
    echo "Self-built kicad-cli reported unexpected version." >&2
    echo "  expected: $EXPECTED_VERSION" >&2
    echo "  actual:   $VERSION_OUTPUT" >&2
    exit 1
fi

echo "Self-built kicad-cli OK: $VERSION_OUTPUT"

rm -f "$TEST_PDF"

set +e
"$ROOT_DIR/scripts/run.sh" kicad-cli pcb export pdf \
    --mode-single \
    --layers F.Cu \
    --output "$TEST_PDF" \
    "$TEST_BOARD" \
    2>"$ERROR_LOG"
STATUS=$?
set -e

if [ "$STATUS" -ne 0 ]; then
    echo "Self-built kicad-cli failed: scripts/run.sh kicad-cli pcb export pdf" >&2
    echo "Exit status: $STATUS" >&2

    if [ -s "$ERROR_LOG" ]; then
        echo "stderr:" >&2
        sed 's/^/  /' "$ERROR_LOG" >&2
    fi

    exit "$STATUS"
fi

if [ ! -s "$TEST_PDF" ]; then
    echo "Self-built kicad-cli did not create a non-empty PDF: $TEST_PDF" >&2
    exit 1
fi

PDF_HEADER=$(dd if="$TEST_PDF" bs=4 count=1 2>/dev/null || true)

if [ "$PDF_HEADER" != "%PDF" ]; then
    echo "Self-built kicad-cli output is not a PDF: $TEST_PDF" >&2
    exit 1
fi

echo "Self-built kicad-cli PDF export OK"
