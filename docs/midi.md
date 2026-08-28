# MIDI implementation

Control Change (CC) and Program Change map for Quaverato firmware **v2.5.1+**.
Written from the source of truth in `firmware/Quaverato/src/midi_handlers.cpp` (not copied from copyrighted manuals).

## Program Change

| PC value | Preset |
|----------|--------|
| 0–5 | Factory/user presets 1–6 |

## Control Change

| CC | Function | Data |
|----|----------|------|
| 20 | Depth | 0–127 (log-scaled into internal depth 0–32; **0 is safe**, no `log10(0)`) |
| 21 | Multiplier | 0–21 → 0.5×; 22–43 → 1×; 44–65 → 1.5×; 66–87 → 2×; 88–109 → 3×; 110–127 → 4× |
| 22 | Wave shape | 0–25 sine; 26–50 saw; 51–75 ramp; 76–100 triangle; 101–127 square |
| 24 | Rate | 0–127 → squared step interval, **clamped** to tap min/max |
| 25 | Spacing (duty) | 0–127 → 6.25%–93.75% |
| 26 | Harmonic mix | 0–63 low emphasis; 64–127 high; updates `zeroCutoff` like the knob |
| 27 | Phase | 0–63 out-of-phase (harmonic); 64–127 in-phase (traditional) |
| 28 | Mode | **Shipped polarity:** 0–63 toggle; 64–127 momentary *(see note)* |
| 29 | Bypass | any value toggles relay |
| 30 | Expression | remaps to last knob touched (`expressionSelect`) |
| 35 | LFO invert | 0–63 inverted (stereo); 64–127 factory — applied continuously |
| 51 | Beat clock | 0–63 ignore; 64–127 follow MIDI clock |
| 93 | Tap | any value = tap press |
| 124 | Omni off | restore stored channel |
| 125 | Omni on | listen all channels |

## Channel learning

With both footswitches held at power-up (calibration mode), the next CC or PC sets the receive channel permanently (unless omni is later enabled).

## Notes / historical mismatches

Older Owner's Manual charts disagreed with shipping firmware in a few places. **v2.5.1 policy:**

| Item | Decision |
|------|----------|
| CC 28 Mode | **Firmware polarity kept** (64–127 = momentary). Docs follow the code so existing MIDI rigs keep working. |
| CC 35 LFO invert | **Firmware fixed** to match stereo docs: 0–63 invert, applied every LFO step (not only on MIDI Start). |
| CC 26 | **Firmware fixed** to update `zeroCutoff` / calibration clamp like the pot. |
| Note On/Off momentary | Implemented; Note Off is registered in v2.5.1 (was missing in 2.4.2). |
