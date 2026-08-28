# Quaverato Harmonic Tremolo Pedal — Firmware

Open firmware for the [Quaverato](https://zeppelindesignlabs.com/products/quaverato-harmonic-tremolo-pedal-diy-kit) harmonic tremolo pedal from [Zeppelin Design Labs](https://zeppelindesignlabs.com/products/quaverato-harmonic-tremolo-pedal-diy-kit) (ATmega328P).

The original firmware was open source. The repository later went private; sources were still
sent to anyone who asked. **v2.5.1** puts a revised, modular tree back in public so the
firmware is easier to build, flash, and keep developing.

Building the kit? Use the step-by-step [assembly guide](https://docs.zeppelindesignlabs.com/quaverato/) (board V8.2, serial ZD3672+).

Help, docs and contact: [support](https://zeppelindesignlabs.com/pages/support). Questions, ideas and feature requests: [forums](https://forums.zeppelindesignlabs.com).

## Status

- Branch **`main`** — modular source, CI builds, current development (**v2.5.1**).
- Branch **`master`** — frozen archive of the original versioned `.ino` / `.hex` uploads. Not rewritten.

## License

**GNU GPL v3.0** — see [`LICENSE`](LICENSE).

Older product manuals stated Creative Commons BY-NC-SA. That claim is **superseded**; the license committed in this repository (GPL-3.0) is authoritative. Product documentation should be updated to match.

Wavetable headers are **MIT** (Robert W. Gallup, 2013) — see [`NOTICE`](NOTICE).

## Build and flash

First-time setup (Arduino IDE, VS Code / Cursor / Kiro, or a release `.hex`) is in **[`docs/getting-started.md`](docs/getting-started.md)**.

```bash
cd firmware
arduino-cli compile --profile minicore --output-dir dist Quaverato
# hex: firmware/dist/Quaverato.ino.hex
# or
pio run -d firmware
```

See [`docs/build-environment.md`](docs/build-environment.md) for toolchain pins and why historical `.hex` files are not byte-reproduced.

Archived field images live in [`archive/`](archive/) and on GitHub Releases (`v1.1.3`, `v2.3.6`, `v2.4.2`). Checksums: [`docs/releases.md`](docs/releases.md).

## Documentation

| Doc | Topic |
|-----|--------|
| [Assembly guide](https://docs.zeppelindesignlabs.com/quaverato/) | Kit build (board V8.2 / ZD3672+) |
| [`docs/getting-started.md`](docs/getting-started.md) | First-time Arduino IDE / editor setup, build, and flash |
| [`docs/midi.md`](docs/midi.md) | MIDI CC/PC map |
| [`docs/eeprom.md`](docs/eeprom.md) | EEPROM layout & factory presets |
| [`docs/pinout.md`](docs/pinout.md) | MCU pin map (incl. UART/bypass overlap) |
| [`docs/hardware.md`](docs/hardware.md) | Board revisions |
| [`docs/boot-and-flashing.md`](docs/boot-and-flashing.md) | Boot holds & avrdude |
| [`CHANGELOG.md`](CHANGELOG.md) | Release history |
| [`ROADMAP.md`](ROADMAP.md) | What is planned next, and what the hardware rules out |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Where to post, PRs, and agentic AI tools |

```bash
python3 tools/decode-eeprom.py eeprom/quaverato-default-presets.hex
```

## Layout

```text
firmware/Quaverato/     Arduino sketch + src/ modules
eeprom/                 Default presets Intel HEX
archive/                Original 1.1.3 / 2.3.6 / 2.4.2 sources & hexes
docs/                   Technical documentation
tools/                  Helpers (EEPROM decode, local build scripts)
```

Agent / AI contributors: see [`AGENTS.md`](AGENTS.md).
