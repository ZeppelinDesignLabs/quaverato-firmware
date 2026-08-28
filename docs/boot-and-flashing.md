# Boot modes and special holds

Hold the listed footswitch(es), apply power, keep held ~3 s (firmware counts 100 ms ticks).

| Hold | Mode |
|------|------|
| Tap + Bypass | Calibration mode: isolate high/low bands via Harmonic Mix; also arms MIDI channel learn |
| Tap only | Version blink on tempo LED (major–minor–patch). **v2.5.1+** blinks three cycles then continues boot (avoids permanent lockup if tap is shorted) |
| Bypass only | Toggle bypass-at-startup EEPROM flag; LED blinks 3× (engaged) or 5× (bypassed) |

### Reading the version blink

Each digit is blinked as that many short pulses, separated by a longer pause. A **zero** digit is
a single long pulse. **v2.5.1** is two short, five short, one short.

Firmware before the long-pulse-for-zero change counted zero as no pulses at all. v1.1.3 went the
other way and added one to every digit, so it reported itself as `2, 2, 4`.

## Preset save (runtime)

1. Mode = TOG  
2. Hold Bypass ~5 s until bypass LED blinks  
3. Set Multiplier knob to preset slot  
4. Release Bypass — LED blinks slot number  

## Flashing

First-time Arduino IDE, editor, and release-hex steps: [getting-started.md](getting-started.md). Do not write fuses.

```bash
# Firmware (ISP, no bootloader)
avrdude -c usbtiny -p m328p -U flash:w:Quaverato-x.y.z.hex:i

# Optional factory presets
avrdude -c usbtiny -p m328p -U eeprom:w:eeprom/quaverato-default-presets.hex:i
```
