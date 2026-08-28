#!/bin/bash
# Rebuild an archived pre-2.5.0 sketch with the current toolchain and compare it
# to the shipped hex. This reproduces the flash-size evidence in
# docs/build-environment.md for the claim that historical binaries are NOT
# byte-reproducible.
#
# Usage: tools/baseline-compile.sh [version]   (default 2.4.2)
#
# Archived sketches predate the sketch profile, so they need the pinned core and
# libraries installed globally first: tools/setup-arduino.sh
set -euo pipefail
export PATH="/usr/local/bin:$HOME/bin:$PATH"

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER="${1:-2.4.2}"
SRC="$REPO/archive/v$VER/Quaverato_$VER.ino"
SHIPPED="$REPO/archive/v$VER/Quaverato_$VER.ino.hex"

if [ ! -f "$SRC" ]; then
  echo "No archived source for $VER at $SRC" >&2
  echo "Available:" >&2
  ls -1 "$REPO/archive" >&2
  exit 1
fi

BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT

# Arduino requires the sketch folder name to match the .ino basename, and these
# sketches include the wavetables with angle brackets, so the headers have to sit
# on the include path next to the sketch.
mkdir -p "$BUILD/Quaverato"
cp "$SRC" "$BUILD/Quaverato/Quaverato.ino"
cp "$REPO/firmware/Quaverato/src/wavetables/"*.h "$BUILD/Quaverato/"

# 2.x archived sketches still include <EEPROMex.h>. Current firmware does not.
if grep -q 'EEPROMex.h' "$SRC"; then
  arduino-cli lib install "EEPROMEx@1.0.0" >/dev/null
fi

FQBN="MiniCore:avr:328:clock=16MHz_external,BOD=2v7,LTO=Os_flto,variant=modelP,bootloader=no_bootloader,eeprom=keep"

echo "=== Rebuilding archived $VER with FQBN=$FQBN ==="
arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD/out" \
  --warnings none \
  "$BUILD/Quaverato"

echo ""
python3 - "$BUILD/out" "$SHIPPED" "$VER" <<'PY'
import os
import sys

build, shipped, ver = sys.argv[1], sys.argv[2], sys.argv[3]


def flash_bytes(path):
    mem = {}
    with open(path) as handle:
        for line in handle:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            count, addr, typ = raw[0], (raw[1] << 8) | raw[2], raw[3]
            if typ == 0:
                for i in range(count):
                    mem[addr + i] = raw[4 + i]
    return len(mem)


rebuilt = None
for name in sorted(os.listdir(build)):
    if name.endswith(".hex") and "bootloader" not in name:
        rebuilt = flash_bytes(os.path.join(build, name))
        break

original = flash_bytes(shipped)
print("=== Flash size ===")
print("shipped %s:  %d bytes" % (ver, original))
if rebuilt is None:
    print("rebuild:        no hex produced")
    sys.exit(1)
print("rebuilt %s:  %d bytes" % (ver, rebuilt))
print("delta:          %+d bytes" % (rebuilt - original))
print()
print("A non-zero delta is expected. The original toolchain is unknown, so")
print("archived .hex files remain the source of truth for field images.")
PY
