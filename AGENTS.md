# Agent guide — Quaverato firmware

Instructions for AI agents and humans making automated changes to this repository.

## Product

Firmware for the Zeppelin Design Labs **Quaverato** harmonic tremolo pedal: ATmega328P, 16 MHz external crystal, ISP-flashed (no bootloader), USBtiny programmer on the 6-pin header.

Canonical remote: `git@github.com:ZeppelinDesignLabs/quaverato-firmware.git`
License: **GPL-3.0** (`LICENSE`). Wavetable data is **MIT** (see `NOTICE`). Do not reintroduce CC BY-NC-SA.

## Do not

- Rewrite or force-push **`master`** (frozen historical archive). Do not rewrite **`main`** unless the user explicitly asks.
- Edit files under **`archive/`** except to add newly verified field hexes with checksums in `docs/releases.md`.
- Claim byte-identical reproduction of pre-2.5.1 shipping `.hex` files (different original toolchains).
- Enable MIDI thru or drive UART TX — digital pin **1** is the bypass footswitch *and* UART TX; `MIDI.turnThruOff()` is required.
- Include `wavetables/*.h` data headers from more than one translation unit (`wavetables.cpp` only).
- Drive the LFO from the Timer1 overflow ISR only. Do not put `analogRead()` or other blocking work in that ISR.
- Paste copyrighted Owner’s Manual / Assembly Instructions prose into `docs/`. Document interface facts from source instead.
- Tell anyone to **Burn Bootloader**, run `pio run -t fuses`, or run `pio run -t bootloader`. Those write fuse bytes. The wrong clock fuse bricks ISP until an external clock is supplied.
- Reintroduce **EEPROMEx**. Use the AVR core `EEPROM` library (`update` / `get` / `put`).
- Invent a second first-time flash procedure. Human-facing setup lives in `docs/getting-started.md`. Point there; do not fork the steps in chat or in other docs.
- Claim Arduino IDE reads `sketch.yaml`. It does not. The IDE path is the **Tools** table in `docs/getting-started.md`, which must match the FQBN below.

## Layout

| Path | Role |
|------|------|
| `firmware/Quaverato/Quaverato.ino` | Thin sketch: `setup()`, `loop()` |
| `firmware/Quaverato/src/` | Modules (oscillator, controls, MIDI, relay, presets, boot, state) |
| `firmware/Quaverato/sketch.yaml` | Pinned MiniCore + libraries for `arduino-cli --profile` and CI |
| `firmware/platformio.ini` | Same pins for PlatformIO (open the `firmware/` folder, not the repo root) |
| `.vscode/extensions.json` | Recommends PlatformIO for VS Code / Cursor / Kiro |
| `eeprom/` | Default presets Intel HEX |
| `archive/` | Original 1.1.3 / 2.3.6 / 2.4.2 field builds |
| `docs/getting-started.md` | First-time Arduino IDE, editor, hex, and flash |
| `docs/` | Other technical docs written for this repo |
| `tools/` | Local build / EEPROM helpers (WSL-friendly scripts) |

## Build and flash

Target FQBN (must match CI, `sketch.yaml`, Arduino IDE **Tools**, and `docs/getting-started.md`):

```text
MiniCore:avr:328:clock=16MHz_external,BOD=2v7,LTO=Os_flto,variant=modelP,bootloader=no_bootloader,eeprom=keep
```

Pinned libs: MIDI Library **5.0.2**, MiniCore **3.1.3**. EEPROM is the AVR core library.

`sketch.yaml` must sit next to `Quaverato.ino`, not in `firmware/`, or `--profile` is not found. Profile library names are case-sensitive even though `arduino-cli lib install` is not.

```bash
# Prefer WSL on this machine
cd firmware
arduino-cli compile --profile minicore --output-dir dist Quaverato
# hex: firmware/dist/Quaverato.ino.hex  (gitignored; or: bash tools/compile-firmware.sh)
# or: pio run -d firmware
```

Flash (ISP only — never fuses):

```bash
avrdude -c usbtiny -p m328p -U flash:w:<hex>:i
```

When changing the FQBN, library versions, programmer, or `avrdude` flags, update **all** of: `firmware/Quaverato/sketch.yaml`, `firmware/platformio.ini`, `docs/getting-started.md`, and this file. `platformio.ini` must keep `upload_protocol = usbtiny` and `upload_flags` must include `-e` (PIO’s Upload passes `-D` and will brick-until-reflash without an erase). Never add `-V` to hide a verify mismatch. Do not add a default fuses/bootloader environment.

Arduino IDE: **Sketch → Upload Using Programmer**, programmer **USBtinyISP**. Ordinary **Upload** talks to a bootloader that is not on this pedal. **Tools → Bootloader → No bootloader** is a compile setting (required). **Tools → Burn Bootloader** is a fuse-write action (never). They are not the same item.

Windows USBtiny usually needs Zadig **libusbK**. `avrdude` inside WSL needs [usbipd-win](https://github.com/dorssel/usbipd-win) to attach the programmer. Details: `docs/getting-started.md`.

## Versioning & releases

- Source of truth: `firmware/Quaverato/src/version.h` (`VERSION_MAJOR` / `MINOR` / `PATCH`).
- Release tags are `vMAJOR.MINOR.PATCH` and **must match** `version.h` (CI enforces this for modern tags).
- Do **not** bump `version.h` in a feature or docs change. Version bumps happen when cutting a release, in the same commit as the `CHANGELOG.md` section and any version strings in README / getting-started / boot docs.
- Historical tags `v1.1.3`, `v2.3.6`, `v2.4.2` ship **archived** hexes, not CI rebuilds.

## Code conventions

- Keep `Quaverato.ino` thin; put logic in `src/*.cpp`.
- Shared runtime state lives in `state.h` / `state.cpp`.
- Prefer behavior-preserving refactors unless the task is explicitly a bugfix or feature.
- **CC 28 (Mode):** shipped polarity is intentional (`value > 63` → momentary). Fix docs, not firmware, unless a breaking release is explicitly requested.
- Guard `log10` inputs; never pass 0.
- ISR-shared multi-byte values (`buttonTimer`, `tempoTapped`): use `ATOMIC_BLOCK` on the main-thread read path.
- Pin / EEPROM constants: `pins.h`, `eeprom_map.h` — no new magic numbers.
- EEPROM address 0 is a schema version (`EEPROM_SCHEMA_VERSION`). `0x00` / `0xFF` mean the current 19-byte preset layout. Do not change the `Preset` struct or map without bumping the schema and updating `docs/eeprom.md`.
- Version blink: a zero digit is one long pulse (`docs/boot-and-flashing.md`). Do not restore the old “blink nothing” or the v1.1.3 `+1` off-by-one.

## Hardware constraints agents must respect

- Board **8.2+** / serial **ZD3672+** is the current target; `relay_pin_Isolator` (D11) is required for 2.4.2+ quiet switching.
- D11 is also ISP **MOSI** — isolator may glitch during programming; that is expected.
- Do not assume USB DFU or a bootloader exists.
- The MCU never sees audio. It PWM-drives two optocoupler LEDs. Do not propose envelope, pitch, or delay features on this hardware (`ROADMAP.md`).

## Docs map

Read the matching doc and keep it in sync with code.

| Topic | Doc |
|-------|-----|
| First-time IDE / editor / hex / flash | `docs/getting-started.md` |
| Boot holds, version blink | `docs/boot-and-flashing.md` |
| MIDI CC/PC | `docs/midi.md` |
| EEPROM layout & factory presets | `docs/eeprom.md` |
| MCU pin map | `docs/pinout.md` |
| Board revisions | `docs/hardware.md` |
| Toolchain pins, historical hex deltas | `docs/build-environment.md` |
| Archived checksums | `docs/releases.md` |
| Release history | `CHANGELOG.md` |
| Planned work / hardware limits | `ROADMAP.md` |
| Human contributors | `CONTRIBUTING.md` |
| Questions / ideas / features (humans) | https://forums.zeppelindesignlabs.com |
| Support hub | https://zeppelindesignlabs.com/pages/support |

When you change conventions, flash steps, FQBN, or doc locations, update this file in the same change. `CONTRIBUTING.md` tells humans that `AGENTS.md` is the contract for agentic tools.

Hardware-facing changes (LFO, relay, boot gestures) need a real-pedal test note in the PR. CI only proves compile.

## Preferred environment

Developers on Windows may use Windows Git or WSL2 on the same checkout. `.gitattributes` forces LF for text (`* text=auto eol=lf`) so `git status` matches on both sides. `.editorconfig` and `.vscode/settings.json` keep the editor on LF too. Do not set `core.autocrlf` to paper over a dirty tree. Archived hexes under `archive/` stay byte-identical. Shell scripts in `tools/` must stay LF.

End-user and editor setup (Arduino IDE, VS Code / Cursor / Kiro, PlatformIO, release hex) is specified in `docs/getting-started.md`, not here.
