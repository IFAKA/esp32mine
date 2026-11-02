#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IDF_DIR="${IDF_PATH:-/Users/faka/.espressif/esp-idf}"

if [[ ! -f "$IDF_DIR/export.sh" ]]; then
    echo "ESP-IDF was not found at $IDF_DIR. Set IDF_PATH to an ESP-IDF checkout." >&2
    exit 1
fi

# Some macOS ESP-IDF installations have more than one Python environment.
# Prefer the known-good environment when it exists, while still allowing an
# explicitly selected IDF_PYTHON_ENV_PATH to win.
if [[ -z "${IDF_PYTHON_ENV_PATH:-}" && -d /private/tmp/esp-idf-venv ]]; then
    export IDF_PYTHON_ENV_PATH=/private/tmp/esp-idf-venv
fi
export IDF_PATH="$IDF_DIR"
source "$IDF_PATH/export.sh" >/dev/null

echo "[1/3] Building the SDL input bridge..."
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" >/dev/null
cmake --build "$ROOT_DIR/build" --target qemu_input qemu_activate --parallel >/dev/null

# Give QEMU a real macOS application identity. Without a bundle, SDL's QEMU
# window is owned by an unbundled child of Terminal and cannot reliably be
# selected, activated, or switched to with Command-Tab.
REAL_QEMU_BINARY="$(type -P qemu-system-xtensa)"
QEMU_PREFIX="$(cd "$(dirname "$REAL_QEMU_BINARY")/.." && pwd)"
QEMU_APP_DIR="$ROOT_DIR/build/ESP32-S3-QEMU.app"
mkdir -p "$QEMU_APP_DIR/Contents/MacOS" "$QEMU_APP_DIR/Contents/share"
cp "$REAL_QEMU_BINARY" "$QEMU_APP_DIR/Contents/MacOS/ESP32-S3-QEMU"
cp "$ROOT_DIR/tools/qemu-app-Info.plist" "$QEMU_APP_DIR/Contents/Info.plist"
cp -R "$QEMU_PREFIX/share/qemu" "$QEMU_APP_DIR/Contents/share/"
chmod +x "$QEMU_APP_DIR/Contents/MacOS/ESP32-S3-QEMU" "$ROOT_DIR/tools/qemu-system-xtensa"
export ESP32MINE_QEMU_APP_EXEC="$QEMU_APP_DIR/Contents/MacOS/ESP32-S3-QEMU"
export PATH="$ROOT_DIR/tools:$PATH"

echo "[2/3] Building the actual ESP32-S3 firmware..."
idf.py -C "$ROOT_DIR/firmware" build >/dev/null

INPUT_PID=""
QEMU_PID=""
cleanup() {
    if [[ -n "$QEMU_PID" ]] && kill -0 "$QEMU_PID" 2>/dev/null; then
        kill "$QEMU_PID" 2>/dev/null || true
        wait "$QEMU_PID" 2>/dev/null || true
    fi
    if [[ -n "$INPUT_PID" ]] && kill -0 "$INPUT_PID" 2>/dev/null; then
        kill "$INPUT_PID" 2>/dev/null || true
        wait "$INPUT_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

echo "[3/3] Starting ESP32-S3 firmware in QEMU..."
echo "The green 'ESP32-S3 QEMU controls' window owns input; keep it focused while playing."
echo "QEMU display: actual firmware framebuffer (3x view) | Controls: WASD, mouse, Space, LMB, RMB, 1-4 | Esc quits"
cd "$ROOT_DIR/firmware"
# ESP32-S3 QEMU uses -m for the emulated PSRAM capacity.  idf.py supplies its
# default 32M machine size; this trailing argument selects the product's 8MB
# octal-PSRAM configuration while retaining idf.py's flash/efuse setup.
idf.py qemu --graphics --qemu-extra-args="-m 8M -name ESP32-S3-QEMU -serial tcp::5556,server,nowait" &
QEMU_PID=$!

# Launch the controls after QEMU so macOS leaves the input window frontmost.
# The bridge retries TCP connection while the firmware finishes booting.
"$ROOT_DIR/build/qemu_input" &
INPUT_PID=$!
sleep 1
if ! kill -0 "$INPUT_PID" 2>/dev/null; then
    echo "The QEMU input bridge could not open a macOS display." >&2
    echo "Run this command from a logged-in macOS desktop session." >&2
    exit 1
fi

# Launch Services can return from the QEMU wrapper even while the GUI process
# remains alive (notably when an older QEMU app instance exists). The controls
# process is the authoritative session lifetime: keeping this shell attached
# prevents the EXIT trap from killing the bridge while the game is visible.
wait "$INPUT_PID"
