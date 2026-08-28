# EEPROM map

ATmega328P EEPROM (1024 bytes). Addresses from `firmware/Quaverato/src/eeprom_map.h`.

| Address | Name | Meaning |
|---------|------|---------|
| 0 | Schema version | `EEPROM_SCHEMA_VERSION` (currently **1**). `0x00` and `0xFF` mean "legacy, same 19-byte preset layout" and are rewritten to 1 on first boot of firmware that knows the byte. Any other mismatch restores the six factory presets and then writes 1. |
| 1 | Bypass startup flag | Loaded then **toggled** by `flipRelay()` at boot. Truthy → ends **bypassed**; falsy → ends **engaged**. Factory image uses `0xFF`. |
| 2 | MIDI channel | 1–16 when omni is off. Invalid/`0xFF` → treated as channel 1 (v2.5.1+). |
| 3 | MIDI omni | Non-zero → `MIDI_CHANNEL_OMNI`. |
| 10+ | Presets | Six packed `Preset` structs |

## Preset struct (19 bytes, little-endian)

```c
struct Preset {
  byte depth;              // 0–32
  double multiplier;       // tap divisor 0.5–4.0 (4 bytes on AVR)
  byte waveShape;          // 0–4
  unsigned long rate;      // step interval (µs-ish squared domain)
  float spacing;           // duty 0.0625–0.9375
  int harmonicMixHigh;     // floorOne 0–32
  int harmonicMixLow;      // floorTwo 0–32
  bool phase;              // synchronize
};
```

Root = 10; preset *n* at `10 + n * 19` for *n* = 0…5.

There is still no factory-reset footswitch gesture. Restoring defaults means flashing
`eeprom/quaverato-default-presets.hex`, or a schema mismatch (which writes the same six
slots from `PROGMEM`). Existing pedals keep their presets: the factory image and field
units leave address 0 as `0xFF`, which is treated as the current layout.

## Factory defaults (`eeprom/quaverato-default-presets.hex`)

Decoded with `tools/decode-eeprom.py`:

| # | depth | mult | wave | rate | spacing | hi | lo | phase byte |
|---|-------|------|------|------|---------|----|----|------------|
| 0 | 32 | 0.50 | 0 | 2704 | 0.475 | 32 | 32 | 0xFF |
| 1 | 32 | 1.00 | 0 | 1296 | 0.475 | 32 | 32 | 0xFF |
| 2 | 32 | 1.50 | 1 | 3025 | 0.475 | 32 | 32 | 0xFF |
| 3 | 32 | 2.00 | 2 | 2500 | 0.475 | 32 | 32 | 0xFF |
| 4 | 32 | 2.00 | 4 | 4489 | 0.243 | 32 | 32 | 0xFF |
| 5 | 32 | 4.00 | 0 | 1681 | 0.497 | 32 | 32 | 0xFF |

Rates are perfect squares (52², 36², …), consistent with `stepRate *= stepRate`. Phase bytes are `0xFF` (truthy) — this image was hand-authored, not dumped from a unit after `writePreset`.

Flash EEPROM:

```bash
avrdude -c usbtiny -p m328p -U eeprom:w:eeprom/quaverato-default-presets.hex:i
```
