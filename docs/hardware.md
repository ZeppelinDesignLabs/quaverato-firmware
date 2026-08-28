# Hardware revisions

| Board | Serial | Notes |
|-------|--------|-------|
| under 8.2 | ZD3644 and below | Use older assembly manual; MIDI-ready status varies |
| **8.2+** | **ZD3672+** | Current kit/manual; ships with MIDI-ready firmware (2.3.6+) |

## Firmware vs hardware

| Firmware | Hardware expectation |
|----------|----------------------|
| 1.1.3 | Pre-MIDI; two-coil relay only |
| 2.3.6 | MIDI; two-coil relay timing |
| 2.4.2 / 2.5.1+ | Four-stage relay including **isolator on pin 11** |

v2.4.2 introduced `relay_pin_Isolator` (D11 / PB3) and a delayed isolate → flip → release → un-isolate sequence for quieter true-bypass switching. On board **8.2**, D11 gates **Q3**, a **J176** P-channel JFET that mutes the audio path around the relay transition. Firmware drives D11 **LOW** before the coils move (JFET on / muted), then **HIGH** after they settle (JFET off / audio passes). See `relay.cpp`. Units without that net should not run 2.4.2+ without verification.

D11 is also ISP **MOSI**. Expect isolator activity while programming. See [pinout.md](pinout.md).
