# Pinout (Arduino digital / analog numbering)

From `firmware/Quaverato/src/pins.h`, board rev **8.2** (serial ZD3672+).

## Digital

| Pin | Net | Notes |
|-----|-----|-------|
| 0 | MIDI RX | Hardware UART RX via 6N138 opto (MIDI mod) |
| 1 | Bypass footswitch | **Also UART TX.** Safe only because `MIDI.turnThruOff()` ensures TX never drives. Do not enable MIDI thru. |
| 2 | Tap tempo footswitch | INT0 |
| 3 | Tempo LED (PWM) | Timer2 fast PWM, prescaler 1 → 62.5 kHz (was 490 Hz Arduino default) |
| 4 | Bypass LED | |
| 5 | Mode toggle | TOG / MOM |
| 6 | Phase toggle | IN / OUT |
| 7 | Relay lock coil | |
| 8 | Relay switch coil | |
| 9 | High-band opto LED (PWM) | Timer1 after `TCCR1B` divisor=1 |
| 10 | Low-band opto LED (PWM) | |
| 11 | Relay/audio isolator | PB3 / MOSI — shared with ISP header. Gates Q3 (J176) on board 8.2. Driven LOW around the relay transition, HIGH when idle. |

## Analog

| Pin | Net |
|-----|-----|
| A0 | Depth |
| A1 | Rate |
| A2 | Time division / multiplier |
| A3 | Duty cycle / spacing |
| A4 | Wave shape |
| A5 | Harmonic mix |

## ISP

2×3 header: standard AVR ISP. Flashing requires USBtiny (or compatible). Pin 11 (MOSI) is shared with the isolator output — expect isolator activity during ISP.
