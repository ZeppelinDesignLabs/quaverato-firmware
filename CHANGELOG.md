# Changelog

All notable firmware changes are documented here.

## [Unreleased]

### Changed
- Removed TaskScheduler. The LFO steps from the Timer1 overflow interrupt (same one-step-per-interval
  behaviour, not DDS yet). Controls, MIDI, relay delays and preset-save live in `loop()` with
  `micros()` deadlines. `analogRead()` no longer shares a cooperative queue with the oscillator.

## [2.5.1] — re-opensourced modular tree

Quaverato firmware was open source, then the repository went private (sources still went out
on request). 2.5.1 puts an upgraded, modular codebase back in public: GPL-3.0, CI, Arduino
IDE / editor getting-started docs, and the fixes below.

### Changed
- Restructured as hybrid Arduino sketch + modular C++ under `firmware/Quaverato/`.
- Version numbers live in `src/version.h` (CI checks tag match).
- Formal GitHub Releases replace versioned filenames in the repo root.
- A zero in the version blink readout is now a single long pulse instead of nothing at all.
  Non-zero digits are unchanged and still blink an exact count.
- Tap LED PWM is now Timer2 fast PWM at 62.5 kHz. Arduino left that timer at phase-correct
  PWM with prescaler 64, so the tap LED flickered at 490 Hz. The optocoupler LEDs on Timer1
  were already at ~31.25 kHz and are unchanged.
- EEPROM access uses the AVR core `EEPROM` library. `EEPROMEx` is no longer a dependency.
  Byte writes use `update` and preset slots use `get` / `put`, so identical values are not
  rewritten.

### Fixed
- Guard `log10(0)` on depth / harmonic-mix MIDI and pot paths.
- Register MIDI Note Off handler (momentary mode).
- Atomic reads of tap ISR shared state (`buttonTimer` / `tempoTapped`).
- Validate EEPROM MIDI channel (`0xFF` / out of range → channel 1).
- CC 26 updates `zeroCutoff` and calibration clamp like the knob.
- CC 24 rate clamped to tap min/max.
- CC 35 LFO invert polarity and continuous stereo invert (no MIDI-clock requirement).
- Version mode no longer infinite-loops (shorted tap no longer bricks boot).
- Startup-mode LED blink counts match documentation (3 = engaged, 5 = bypassed).
- Setting the MIDI receive channel with a Program Change message now persists. Calibration mode
  leaves the omni flag set and only the CC learn path cleared it, so a channel learned from a PC
  message reverted to omni on the next power-up, contrary to `docs/midi.md`.
- `clockCount` no longer increments without bound when MIDI clock follow (CC 51) is off. It counted
  every incoming clock byte until the `int` overflowed. Enabling CC 51 mid-stream now also starts
  from a whole beat rather than a partial one.
- The in-RAM `midiOmni` flag is kept in step with EEPROM at every site that changes omni state
  (channel learn, CC 124, CC 125), so it no longer goes stale after the flag is changed at runtime.
- EEPROM address 0 is now a schema version. `0x00` and `0xFF` (every image shipped so far) are
  treated as the current 19-byte preset layout and rewritten to version 1, so existing presets
  survive. Any other mismatch restores the six factory slots instead of silently rereading them
  as the wrong struct.

### Notes
- CC 28 Mode polarity left as shipped (64–127 = momentary); documentation updated to match firmware.

## [2.4.2] — archived

Four-stage true-bypass relay sequence with isolator on pin 11 (quieter switching).
Field hex: `archive/v2.4.2/Quaverato_2.4.2.ino.hex` (15,642 bytes).

## [2.3.6] — 2018-12-11 (archived)

Full MIDI implementation; significant RATE knob improvement.
Field hex: `archive/v2.3.6/Quaverato_2.3.6.ino.hex` (15,884 bytes).

## [1.1.3] — 2018-08-13 (archived)

Initial public release (no MIDI).
Field hex: `archive/v1.1.3/Quaverato_1.1.3.ino.hex` (10,028 bytes).
