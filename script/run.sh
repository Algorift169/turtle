#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_PATH="$ROOT_DIR/build/bin/turtle-wm"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Building Turtle window manager first..."
    make -C "$ROOT_DIR"
fi

if [[ -z "${DISPLAY:-}" ]]; then
    echo "Launching Turtle inside Xephyr..."
    Xephyr :2 -screen 1280x720 &
    XEYPHR_PID=$!
    export DISPLAY=:2
    trap 'kill "$XEYPHR_PID"' EXIT
fi

exec "$BIN_PATH"
