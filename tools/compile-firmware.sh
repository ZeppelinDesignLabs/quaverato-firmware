#!/bin/bash
# Compile the current firmware tree and report flash usage.
set -euo pipefail
export PATH="/usr/local/bin:$HOME/bin:$PATH"

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${1:-$REPO/firmware/dist}"
WORK="$REPO/firmware/build"

rm -rf "$DIST" "$WORK"
mkdir -p "$DIST" "$WORK"

# The profile in firmware/Quaverato/sketch.yaml pins the core and libraries and
# installs anything missing, so this needs no prior arduino-cli setup.
# --build-path is compiler junk (firmware/build/). --output-dir is the hex
# you flash (firmware/dist/Quaverato.ino.hex).
arduino-cli compile \
  --profile minicore \
  --build-path "$WORK" \
  --output-dir "$DIST" \
  "$REPO/firmware/Quaverato"

python3 - "$DIST" <<'PY'
import os
import sys

build = sys.argv[1]
for name in sorted(os.listdir(build)):
    if not name.endswith(".hex") or "bootloader" in name:
        continue
    mem = {}
    with open(os.path.join(build, name)) as handle:
        for line in handle:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            count, addr, typ = raw[0], (raw[1] << 8) | raw[2], raw[3]
            if typ == 0:
                for i in range(count):
                    mem[addr + i] = raw[4 + i]
    if mem:
        print("%s: %d bytes flash" % (name, len(mem)))
PY
