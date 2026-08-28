#!/bin/bash
# Optional: install the pinned core and libraries globally.
#
# Not required to build. `tools/compile-firmware.sh` and the `--profile minicore`
# builds resolve everything from firmware/Quaverato/sketch.yaml on their own.
# This is only useful for the Arduino IDE, which does not read sketch profiles.
set -euo pipefail
export PATH="/usr/local/bin:$HOME/bin:$PATH"

if ! command -v arduino-cli >/dev/null 2>&1; then
  mkdir -p "$HOME/bin"
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/bin" sh
  export PATH="$HOME/bin:$PATH"
fi

arduino-cli version
arduino-cli config init --overwrite
arduino-cli config add board_manager.additional_urls https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
arduino-cli core update-index

# Versions must stay in step with firmware/Quaverato/sketch.yaml.
arduino-cli core install MiniCore:avr@3.1.3
arduino-cli lib install "TaskScheduler@4.0.8" "MIDI Library@5.0.2"

echo "=== cores ==="
arduino-cli core list
echo "=== libs ==="
arduino-cli lib list
