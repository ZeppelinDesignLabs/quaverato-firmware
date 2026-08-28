# Build environment

## Target hardware

| Item | Value |
|------|-------|
| MCU | ATmega328P (28-pin DIP, MiniCore `variant=modelP`) |
| Clock | 16 MHz external crystal (BOM `CR-10-10` / Y1, 22 pF at C5/C6) |
| Bootloader | None (ISP flash via USBtiny) |
| Fuses (typical MiniCore 16 MHz ext, BOD 2.7V, no bootloader) | Set by MiniCore when burning bootloader/fuses; shipping units are ISP-flashed |

Cross-check: Owner's Manual minimum LFO rate ≈ 0.125 Hz equals `15625 µs × 256 steps × 2`, which requires true `micros()` at 16 MHz.

## Baseline compile (unmodified v2.4.2 source)

Environment used for the Phase 1 baseline (2026-08-23, WSL2 Ubuntu):

| Component | Version |
|-----------|---------|
| arduino-cli | 1.5.1 |
| MiniCore | 3.1.3 |
| avr-gcc (bundled) | 7.3.0-atmel3.6.1-arduino7 |
| TaskScheduler | 4.0.8 |
| MIDI Library | 5.0.2 |
| EEPROMEx | 1.0.0 (archived 2.x sketches only; current firmware uses the AVR core `EEPROM` library) |

FQBN:

```text
MiniCore:avr:328:clock=16MHz_external,BOD=2v7,LTO=Os_flto,variant=modelP,bootloader=no_bootloader,eeprom=keep
```

### Flash size comparison

Rebuilding each archived sketch unmodified, with the toolchain above:

| Version | Shipped hex | Rebuilt | Delta |
|---------|-------------|---------|-------|
| 1.1.3 | 10,028 | 10,036 | **+8** |
| 2.3.6 | 15,884 | 16,376 | **+492** |
| 2.4.2 | 15,642 | 16,430 | **+788** |

Reproduce any row with `tools/baseline-compile.sh <version>`. Modular v2.5.1 (this tree) is what CI builds.

Historical binaries are **not** byte-reproducible from current tooling, and the spread of those deltas is the evidence. Pre-MIDI 1.1.3 lands within 8 bytes of its shipped image, so it was built with something very close to the current toolchain. The 2.x images diverge by hundreds of bytes, and they diverge by *different* amounts, so 2.3.6 and 2.4.2 were not built with the same toolchain as each other either. Corroborating that: 2.4.2 adds the relay-isolator logic yet its shipped hex is **242 bytes smaller** than 2.3.6, while our rebuilds put 2.4.2 **54 bytes larger** than 2.3.6, which is the direction the added code predicts.

Treat archived `.hex` files as the source of truth for field-verified images.

## Last shipped version before 2.5.1

**2.4.2** was the newest firmware in the field before this repository's 2.5.1 re-opensource. Both the legacy repository tip and the public Troubleshooting Guide agreed on it. A 2025 forum mention of a `2-4-3` blink was unreliable (a ChatGPT diagnosis); no 2.4.3 source or hex exists in this archive. Manufacturing / ZDL Updater bundles are still worth spot-checking offline when convenient.

## Accepted policy

- Publish original `.hex` assets on GitHub Releases for v1.1.3 / v2.3.6 / v2.4.2.
- Build modern releases (v2.5.1+) from this tree with the pinned profile in `firmware/Quaverato/sketch.yaml` / `firmware/platformio.ini`.
- Do not claim byte-identical reproduction of pre-2.5.1 shipping images.
