# Roadmap

Where the Quaverato firmware is going. This is a statement of intent, not a promise. Dates are
deliberately absent, and priorities move based on what people actually ask for.

**Want to influence it?** Comment on the linked issue, or start a thread on the
[Zeppelin Design Labs forums](https://forums.zeppelindesignlabs.com). Feedback before something is
built is worth far more than feedback after. See also the [support page](https://zeppelindesignlabs.com/pages/support).

---

## Where we are

**v2.5.1** re-opensources the Quaverato firmware after a stretch when the repository was private
(sources were still sent to anyone who asked). The original firmware was open source; this is
that project brought back with a modular tree, GPL-3.0, CI, and a more usable codebase so
development can continue. It also fixes a batch of long-standing bugs: `log10(0)` on the
HARMONIC MIX extremes, an unregistered MIDI Note Off handler, non-atomic tap-ISR reads, a
silent zero in the version blink, MIDI channel learned by Program Change not persisting,
unbounded `clockCount`, tap LED PWM at 490 Hz, and an EEPROM schema byte. See
[`CHANGELOG.md`](CHANGELOG.md).

---

## Next: the engine

This is a rewrite of how the LFO works. Nothing here is a user-visible feature on its own. It is
the foundation that makes the features after it cheap instead of impossible.

- [ ] **Replace the scheduler-driven wavetable stepper with a DDS phase accumulator in a timer
      interrupt.** Today the LFO advances one step per TaskScheduler callback, and `analogRead()`
      blocks for roughly 104 microseconds from that same cooperative scheduler. At the fastest
      rates the oscillator wants to step every 196 microseconds, so control polling lands
      unpredictably inside a large fraction of a step period. A 32-bit phase accumulator driven by
      hardware makes the rate exact and the timing jitter-free.

      *Open question: the interrupt cost needs measuring on real hardware before we commit to an
      update rate. If it is too tight, updating every 2nd or 4th PWM overflow is the tuning knob.*

- [ ] **Widen the modulation maths to fixed point.** `depth`, `floorOne` and `floorTwo` are each
      quantized to 33 levels (0 to 32), and `adjustLight()` compounds rounding through two
      successive divides by 32. Sweeping depth with an expression pedal zippers audibly. This is
      the single most impactful change for perceived smoothness, more so than PWM resolution.

- [ ] **Move the optocoupler LEDs to 9-bit PWM.** Timer1 is 16-bit, so this doubles resolution
      while keeping the carrier at 31.25 kHz. 10-bit is available but drops the carrier to
      15.6 kHz, inside the audible band. The LDR would filter it acoustically, but it invites
      electrical coupling for a marginal gain.

- [ ] **Host-side unit tests.** The phase accumulator, depth curve and phase-warp maths are pure
      integer arithmetic with no hardware dependency, so they can be compiled and tested natively.
      Since this release changes the audio engine, that is worth the setup cost.

---

## Then: the features

- [ ] **Continuous phase offset between the high and low bands.**

      Harmonic mode currently does `lightIntensity = 255 - lightIntensity`. That is amplitude
      inversion, not a phase shift. For a sine the two are identical; for a saw or ramp they are
      not, because the low band gets a *reversed* ramp rather than a *delayed* saw, which is not
      what the manual describes. Reading the second channel from the wavetable at an offset index
      gives a true 180 degrees, fixes the saw and ramp behaviour, and makes every angle in between
      available. Around 90 degrees the two bands sweep through each other instead of alternating,
      which is a sound this pedal cannot currently make.

      *Compatibility decision needed:* `splitDutyCycle()` doubles the period in harmonic mode,
      which is why the manual tells you to set MULTIPLIER to double-time. With a continuous offset
      there is no single correct compensation. Keep it, drop it and fix the documented wart, or
      scale it with the offset?

- [ ] **LDR response linearization.** An LDR's light-to-resistance curve is steeply nonlinear, so
      the shape you select is not the shape you hear. The square is probably soft-edged and the
      sine lopsided. A 256-byte lookup table in `PROGMEM` mapping desired attenuation to PWM value
      fixes this for one table read. It needs real characterization data first, plus a check on how
      much the optocouplers vary part to part. If the spread is wide, this becomes two or three
      selectable curves rather than one.

- [ ] **A shift layer on the TAP footswitch.** There is no spare knob. Holding TAP for over a
      second would put the six knobs onto a second set of parameters, with the bypass LED
      indicating the mode. BYPASS is already taken, since a five-second hold saves a preset, so TAP
      is the free control. This needs care so that a long hold does not corrupt tap timing, and so
      the primary parameters do not jump when the layer is released.

      Proposed initial mapping, kept deliberately sparse: HARMONIC MIX sets phase offset,
      WAVE SHAPE selects a second wave bank, RATE sets inter-band drift.

- [ ] **A second bank of wave shapes.** There is roughly 17 KB of flash free and each table costs
      256 bytes, so the ceiling is somewhere around sixty additional shapes. Which ones are worth
      having is a good topic for the forums.

---

## Later: not scheduled, not rejected

Waveform morphing, crossfading between adjacent shapes rather than stepping. Sample-and-hold and
random LFO shapes. Expanded preset storage over MIDI, since program change supports 0 to 127 and
EEPROM has room for around 47 more presets, though the constraint is the save gesture rather than
the storage. Soft-takeover on knobs after a preset recall. Expression-pedal morphing between two
presets. Leslie-style speed ramping on a footswitch hold.

---

## Not possible on this hardware

Worth stating plainly, because these come up often and the answer will never change without a new
pedal.

The ATmega328P **never sees the audio signal**. It only drives two optocoupler LEDs, and the entire
signal path is analog. So there is no envelope following or dynamics response, no pitch tracking or
shifting, and no delay, reverb or anything else needing an audio buffer.

There is **one output jack**, so no true stereo. Two Quaveratos can be run in stereo using CC 35 to
invert one of them, and that already works.

There is **no MIDI out or thru**. The UART transmit pin is wired to the bypass footswitch, so the
pedal can receive MIDI but has no way to send it. This also means the firmware cannot report its
version, state or presets back over MIDI.
