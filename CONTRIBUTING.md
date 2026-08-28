# Contributing to Quaverato Firmware

Thanks for being here. This is firmware for a physical guitar pedal that people have already
bought, built and gigged with, which shapes how we work more than anything else in this document.

## Where to put things

Start at the [Zeppelin Design Labs support page](https://zeppelindesignlabs.com/pages/support)
(docs, contact, forums). Questions, ideas, feature requests and open-ended discussion belong on
the [forums](https://forums.zeppelindesignlabs.com), not in GitHub Issues or Discussions.

| You want to | Go here |
|---|---|
| Ask how to build, flash or use something | [Forums](https://forums.zeppelindesignlabs.com) |
| Float an idea or request a feature | [Forums](https://forums.zeppelindesignlabs.com) |
| Report a firmware bug (LFO, MIDI, presets, bypass, boot) | [Open a bug report](../../issues/new?template=bug_report.yml) |
| Docs, contact, assembly, warranty, orders | [Support](https://zeppelindesignlabs.com/pages/support) |

GitHub Issues are a work queue for firmware defects we intend to fix. A forum thread that gains
traction can be promoted to an issue. That is the normal path, not a demotion.

## The constraint that shapes everything

The ATmega328P does not touch the audio. It drives two optocoupler LEDs with PWM, and the entire
signal path is analog. Anything requiring the firmware to *hear* the guitar is impossible on this
hardware, no matter how it is written. [`ROADMAP.md`](ROADMAP.md) spells out the specifics.

The second constraint is that **people flash this onto pedals they already own.** A change that
works beautifully on your bench but silently reinterprets someone's saved presets, or changes what
a knob does without warning, is a regression even if the code is correct. Backward compatibility is
a feature.

## Setting up

First-time Arduino IDE, VS Code / Cursor, and flash steps: [`docs/getting-started.md`](docs/getting-started.md).
Toolchain versions are pinned in [`firmware/Quaverato/sketch.yaml`](firmware/Quaverato/sketch.yaml) and
[`firmware/platformio.ini`](firmware/platformio.ini). Read
[`docs/build-environment.md`](docs/build-environment.md) for why historical `.hex`
files are not byte-reproducible.

```bash
cd firmware
arduino-cli compile --profile minicore --output-dir dist Quaverato   # or: pio run -d firmware
```

CI compiles every pull request. A build that fails CI will not be reviewed until it is green.

## Flashing, and how to not brick your pedal

There is **no bootloader**. The chip is programmed over ISP with a USBtiny or equivalent. Two
things are worth knowing before your first flash:

- **Do not touch the fuse bytes.** The board runs from an external 16 MHz crystal. Setting the
  fuses to an internal oscillator will make the chip unresponsive to further ISP programming
  until you supply an external clock. This is the single most common way to lose an afternoon.
- **Back up your EEPROM before flashing** if you have presets you care about:
  `avrdude -c usbtiny -p m328p -U eeprom:r:my-presets.hex:i`

[`docs/boot-and-flashing.md`](docs/boot-and-flashing.md) has the full procedure.

## Testing

CI can only prove the firmware compiles. It cannot prove the pedal sounds right, and there is no
emulator that models an LDR. So:

- **Anything touching the LFO, the relay sequence or the boot gestures must be tested on real
  hardware** before it is merged. Say so in the pull request, and say what you tested with:
  MIDI mod fitted or not, which board revision, which controller.
- Pure integer maths (depth curves, phase arithmetic, EEPROM packing) should be written so it can
  be compiled and tested on a host machine without an AVR. Prefer that structure where you can.
  It is the only part of this codebase that can be tested automatically.
- If you cannot test on hardware, say so plainly in the PR. That is fine. It just means someone
  else has to before it lands, and it is much better than a silent assumption.

## Code style

Match the file you are editing. The codebase is mid-migration from a single `.ino` to modules
under `firmware/Quaverato/src/`, so consistency within a file matters more than global uniformity.

A few things specific to this project:

- This is a 32 KB part with 2 KB of RAM. Keep wavetables and constant data in `PROGMEM`.
- Anything shared with an ISR must be `volatile`, and multi-byte reads of it need to be atomic.
  At least one bug fixed in 2.5.1 came from getting this wrong.
- Avoid floating point in anything that runs per-LFO-step. It is slow on AVR and it is the reason
  several timing paths are more expensive than they look.
- Comment *why*, not *what*. The existing code has very few comments, and the ones worth adding
  explain hardware quirks and timing constraints that the code cannot express on its own.

## Agentic AI coding tools

Cursor, Copilot, Claude, Kiro and similar tools are welcome on this repo. Treat them as
contributors that still need a human in the loop: you own the PR, the hardware test, and any
mistake the model makes.

[`AGENTS.md`](AGENTS.md) is the contract those tools are supposed to follow. If you change
anything an agent would get wrong later — FQBN, libraries, flash flags, doc locations, pin or
EEPROM maps, “do not” rules — update `AGENTS.md` in the same change. A getting-started fix that
never reaches `AGENTS.md` will be re-broken by the next session.

Read `AGENTS.md` before you let a tool edit the tree. Do not ask it to rewrite `master`, write
fuses, or invent a second flash procedure.

## Pull requests

Keep them focused, one behavioural change per PR. A bug fix and a refactor in the same diff is
hard to review and harder to revert.

In the description, cover what changed, why, and how you tested it on hardware. If the change
alters documented behaviour, update the relevant file in `docs/` in the same PR. If it changes
anything a player can hear or feel, add a `CHANGELOG.md` entry under an `## [Unreleased]` heading.

Do not bump `src/version.h` in a feature PR. Version bumps happen at release time, and CI checks
that `version.h` matches the git tag.

## Licensing of contributions

This project is **GPL-3.0**. By opening a pull request you agree that your contribution is
licensed under GPL-3.0 as well.

Older product manuals said Creative Commons BY-NC-SA. That is superseded. The `LICENSE` file in
this repository is authoritative.

If you are contributing code you did not write, or code derived from another project, say so in
the PR and name the source and its license. Wavetable headers from Robert W. Gallup are MIT and
their notices must stay intact. See [`NOTICE`](NOTICE).

## Reporting something security-sensitive

This is an offline audio pedal with no network stack, so genuine security issues are unlikely.
If you find something that could damage hardware, such as a code path that could misdrive the
relay or the optocouplers, please raise it privately with
[Zeppelin Design Labs support](https://zeppelindesignlabs.com/pages/support) rather than opening a public issue.
