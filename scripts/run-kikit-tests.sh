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

CACHE_ROOT="${PYBIND11_KICAD_COMPAT_DIR:-${TMPDIR:-/tmp}/pybind11-kicad-compat}"
KIKIT_DIR="${KIKIT_SOURCE:-$CACHE_ROOT/KiKit}"

if [ -n "${KIKIT_SOURCE:-}" ]; then
    if [ "$(git -C "$KIKIT_DIR" rev-parse HEAD)" != "$KIKIT_COMMIT" ]; then
        echo "KIKIT_SOURCE is not at pinned KiKit commit $KIKIT_COMMIT" >&2
        exit 2
    fi
elif [ ! -d "$KIKIT_DIR/.git" ]; then
    mkdir -p "$CACHE_ROOT"
    git clone "$KIKIT_URL" "$KIKIT_DIR"
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
elif ! git -C "$KIKIT_DIR" cat-file -e "$KIKIT_COMMIT^{commit}" 2>/dev/null; then
    git -C "$KIKIT_DIR" fetch origin "$KIKIT_COMMIT"
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
elif [ "$(git -C "$KIKIT_DIR" rev-parse HEAD)" != "$KIKIT_COMMIT" ]; then
    if [ -n "$(git -C "$KIKIT_DIR" status --porcelain)" ]; then
        echo "Cached KiKit checkout is dirty: $KIKIT_DIR" >&2
        echo "Clean it or set PYBIND11_KICAD_COMPAT_DIR to a fresh cache." >&2
        exit 2
    fi
    git -C "$KIKIT_DIR" checkout --detach "$KIKIT_COMMIT"
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
