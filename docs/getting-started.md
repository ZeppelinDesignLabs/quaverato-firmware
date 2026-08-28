# Getting started — build and flash

This is the first-time path for putting firmware on a Quaverato. The pedal has **no bootloader**. You program the ATmega328P over the 6-pin ISP header with a USBtiny-compatible programmer.

| You want to | Go here |
|---|---|
| Install the latest published firmware | [Flash a release hex](#flash-a-release-hex) |
| Edit and upload from Arduino IDE | [Arduino IDE](#arduino-ide) |
| Edit and upload from VS Code, Cursor, or Kiro | [VS Code, Cursor, and other VS Code forks](#vs-code-cursor-and-other-vs-code-forks) |
| Check that the pedal booted the new firmware | [Verify on the pedal](#verify-on-the-pedal) |

Board settings, library versions, and the `avrdude` command below must stay in step with [`firmware/Quaverato/sketch.yaml`](../firmware/Quaverato/sketch.yaml).

## What you need

- A Quaverato with the 6-pin ISP header populated (kit and production boards have this).
- A **USBtinyISP** or compatible (SparkFun Pocket AVR Programmer, Adafruit USBtinyISP, and many clones). Zeppelin Design Labs [sells one](https://zeppelindesignlabs.com/products/parts-tools/usbtiny-programmer/) as well. USBasp also works if you pass `-c usbasp` instead of `-c usbtiny`.
- A USB cable for the programmer.

Board **8.2+** / serial **ZD3672+** is the current target. Firmware 2.4.2 and later drive the quiet-switching isolator on pin 11. Do not put 2.4.2+ on an older board unless you have confirmed that net exists. See [hardware.md](hardware.md).

## Two rules

**Do not burn a bootloader and do not write fuses.** The chip runs from an external 16 MHz crystal. “Burn Bootloader” in Arduino IDE, `pio run -t fuses`, and `pio run -t bootloader` all write fuse bytes. The wrong clock fuse makes the chip ignore further ISP until you feed it an external clock.

**Back up EEPROM before you flash** if you have presets you care about. A flash-only write does not erase EEPROM, but a bad click or a factory-preset restore will.

```bash
avrdude -c usbtiny -p m328p -U eeprom:r:my-presets.hex:i
```

Restore that file later with `eeprom:w:my-presets.hex:i`.

Pin 11 is both the isolator and ISP MOSI. The isolator may click or glitch while you program. That is expected.

## Programmer and Windows driver

1. Plug the USBtiny into USB and onto the 2×3 ISP header. Pin 1 is marked; do not reverse the connector. The programmer powers the MCU through that header, so a 9 V supply is not required for flashing.
2. If you do have 9 V connected and the programmer has a **target-power jumper**, turn the jumper **off** so the two supplies do not fight.

The USBtiny is **not a COM port**. Arduino IDE’s **Tools → Port** (often **COM1**) is unused for this programmer. Leave it. Upload Using Programmer talks to USB VID `1781` / PID `0C9F` directly.

On **Windows**, Device Manager can show **USBtiny** under **libusb-win32 devices** as “working properly” and MiniCore’s avrdude will still fail with `Permission denied` / `cannot find USBtiny device (0x1781/0xc9f)`. That driver is the wrong one for MiniCore. Replace it with **libusbK**:

1. Download [Zadig](https://zadig.akeo.ie/).
2. Plug in the programmer. **Options → List All Devices**, then select **USBtiny** / **USBtinyISP**.
3. Check the USB IDs **before** you click anything: they must be VID `1781`, PID `0C9F`. Other devices can have “tiny” in the name. **Replace Driver** cannot be cancelled once it starts.
4. Set the target driver to **libusbK** and click **Replace Driver**.
5. Device Manager should now list it under **libusbK USB Devices**. Close and reopen Arduino IDE, then **Upload Using Programmer** again.

If you replace the driver on the wrong device, let Zadig finish, then in Device Manager uninstall that device (check **Delete the driver software**), unplug it, and plug it back in so Windows restores the original driver.

macOS and Linux normally need no driver. On Linux, add your user to the `plugdev` group or install an avrdude udev rule if you get a permission error.

If you run `avrdude` inside **WSL**, the programmer is a Windows USB device. Attach it with [usbipd-win](https://github.com/dorssel/usbipd-win) (`usbipd list`, `usbipd bind`, `usbipd attach --wsl`) before the WSL `avrdude` command. Arduino IDE on Windows does not need that.

## Flash a release hex

Use this when you only want the published firmware, not a local build.

1. Open the latest [GitHub Release](https://github.com/ZeppelinDesignLabs/quaverato-firmware/releases) and download `Quaverato-x.y.z.hex` (the version is in the filename).
2. Back up EEPROM if you need to.
3. Flash. Do not add fuse flags.

```bash
avrdude -c usbtiny -p m328p -U flash:w:Quaverato-x.y.z.hex:i
```

Optional — replace presets with the factory image from this repo:

```bash
avrdude -c usbtiny -p m328p -U eeprom:w:eeprom/quaverato-default-presets.hex:i
```

`avrdude` is bundled with MiniCore once Arduino IDE or `arduino-cli` has installed the core. You can also install it with Homebrew (`brew install avrdude`) or your Linux package manager.

On **Windows**, `avrdude` is almost never on your PowerShell PATH. After MiniCore is installed, call it by full path (version folder may differ):

```powershell
& "$env:LOCALAPPDATA\Arduino15\packages\MiniCore\tools\avrdude\8.0-arduino.1\bin\avrdude.exe" -c usbtiny -p m328p -U flash:w:dist\Quaverato.ino.hex:i
```

If that path 404s:

```powershell
Get-ChildItem "$env:LOCALAPPDATA\Arduino15\packages\MiniCore\tools\avrdude" -Recurse -Filter avrdude.exe
```

PlatformIO also ships a copy at `%USERPROFILE%\.platformio\packages\tool-avrdude\avrdude.exe`. Prefer the MiniCore one; it is what Arduino IDE uses. You do not need to add either to PATH.

Then [verify on the pedal](#verify-on-the-pedal).

## Arduino IDE

Use Arduino IDE 1.8 or 2.x. These steps are written for 2.x; 1.8 uses the same **Tools** menus.

### Install the board package and libraries

1. **File → Preferences → Additional boards manager URLs** and add:

   ```text
   https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
   ```

2. **Tools → Board → Boards Manager**, search **MiniCore**, install **3.1.3**.
3. **Tools → Manage Libraries** and install:
   - **TaskScheduler** by Anatoli Arkhipenko, version **4.0.8**
   - **MIDI Library** by Francois Best / Forty Seven Effects, version **5.0.2**

Do not install **EEPROMEx**. This firmware uses the AVR core `EEPROM` library.

### Open the sketch and set the board

1. **File → Open** [`firmware/Quaverato/Quaverato.ino`](../firmware/Quaverato/Quaverato.ino). The sketch folder must stay named `Quaverato`.
2. **Tools** menus. **Bootloader** is a setting in the *middle* of this menu (with Clock, BOD, Variant). It is not the **Burn Bootloader** command at the very bottom — that last item is easy to see first and skip past the real setting.

   | Menu | Setting |
   |---|---|
   | Board | **MiniCore → ATmega328** |
   | Clock | **External 16 MHz** |
   | BOD | **BOD 2.7v** |
   | EEPROM | **EEPROM retained** |
   | Compiler LTO | **LTO enabled** |
   | Variant | **328P / 328PA** |
   | Bootloader | **No bootloader** (compile setting — not the command below) |
   | Programmer | **USBtinyISP** |
   | Burn Bootloader | Do not click. Writes fuses even when the setting above is **No bootloader**. |

Those match the compile profile in `firmware/Quaverato/sketch.yaml`. A wrong clock or variant will either fail to talk to the chip or run at the wrong rate.

### Build and upload

- **Sketch → Verify/Compile** to build only.
- **Sketch → Upload Using Programmer** to flash over the USBtiny.

Do **not** use the ordinary **Upload** button. That talks to a serial bootloader, which this pedal does not have.

**Sketch → Export Compiled Binary** writes a `.hex` next to the sketch if you want to archive or `avrdude` it later.

## VS Code, Cursor, and other VS Code forks

Kiro and other VS Code-compatible editors use the same extensions and the same terminal commands.

You can edit the repo from the project root. For **PlatformIO’s** upload button, open the [`firmware`](../firmware) folder so it sees `platformio.ini`.

### Option A — PlatformIO (editor UI)

1. Install the **PlatformIO IDE** extension.
2. **File → Open Folder** on `firmware/` (the directory that contains `platformio.ini`). The sketch lives in `firmware/Quaverato/`; `platformio.ini` points `src_dir` there. If a build says “put your source code files in `firmware/src`”, the `src_dir` line was ignored — it must be under `[platformio]`, not under `[env:quaverato]`.
3. Let PlatformIO install the `atmelavr` platform and the libraries pinned in that file.
4. **Build**, then **Upload**. Both live in three places:
   - Bottom **status bar**: the checkmark is Build, the right-arrow is Upload.
   - Left **PlatformIO** sidebar (alien-head icon) → **Project Tasks** → `quaverato` → **Build** / **Upload**.
   - Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`) → **PlatformIO: Build** and **PlatformIO: Upload**.

   Upload uses the USBtiny (`upload_protocol = usbtiny`). `platformio.ini` passes `-e` so flash is erased before write. PIO’s default Upload skips that erase and will leave a dead pedal. Do not run the **fuses** or **bootloader** tasks. Those write fuse bytes.

### Option B — Arduino CLI in the integrated terminal

This is the same path CI uses. It reads versions from `firmware/Quaverato/sketch.yaml` and installs anything missing.

1. Install [arduino-cli](https://arduino.github.io/arduino-cli/latest/installation/).
2. From the repo root, compile and write the hex into `firmware/dist/` (a bare `compile` leaves it in a hidden cache and does not print the path):

```bash
cd firmware
arduino-cli compile --profile minicore --output-dir dist Quaverato
```

`Quaverato` is the sketch folder and it appears **once**, last. Putting it both before and after `--output-dir` makes the CLI say `accepts at most 1 arg(s), received 2`.

The file you flash is `firmware/dist/Quaverato.ino.hex`. That folder is gitignored.

3. Flash it. From `firmware/`. On macOS/Linux, `avrdude` is often on PATH. On Windows PowerShell it is not — use the MiniCore path from [Flash a release hex](#flash-a-release-hex):

```powershell
& "$env:LOCALAPPDATA\Arduino15\packages\MiniCore\tools\avrdude\8.0-arduino.1\bin\avrdude.exe" -c usbtiny -p m328p -U flash:w:dist\Quaverato.ino.hex:i
```

```bash
avrdude -c usbtiny -p m328p -U flash:w:dist/Quaverato.ino.hex:i
```

A compile without `--output-dir` still succeeds; the hex is under your user cache (Windows: `%LOCALAPPDATA%\arduino\sketches\<hash>\Quaverato.ino.hex`). Re-run with `--output-dir dist` rather than hunting that folder.

`tools/compile-firmware.sh` does the same thing (hex in `firmware/dist/`):

```bash
bash tools/compile-firmware.sh
avrdude -c usbtiny -p m328p -U flash:w:firmware/dist/Quaverato.ino.hex:i
```

### Option C — Arduino IDE extension

The community Arduino extension can work if it is pointed at the same MiniCore FQBN and **USBtinyISP**, and you choose **Upload Using Programmer**. It does not read `sketch.yaml`. Prefer PlatformIO or `arduino-cli` if the extension fights you on board options.

## Verify on the pedal

1. Unplug the programmer.
2. Power the pedal while holding **Tap** for about three seconds, then release.
3. The tempo LED blinks **major, minor, patch**. Each non-zero digit is that many short pulses. A **zero** is one long pulse.

So 2.5.1 is two short, five short, one short. A zero digit (for example a future 2.6.0) is two short, six short, one long.

Then play through it: engage, tap tempo, sweep Rate / Depth / Harmonic Mix. Calibration and MIDI details are in [boot-and-flashing.md](boot-and-flashing.md) and [midi.md](midi.md).

## Troubleshooting

| Symptom | What to check |
|---|---|
| Only **COM1** in **Tools → Port**, or `Permission denied` / `cannot find USBtiny device (0x1781/0xc9f)` | Port is unused. The USBtiny is not a COM device. Windows: Device Manager **libusb-win32** is not enough — Zadig **libusbK**, then restart the IDE. Linux: udev / `plugdev`. WSL: `usbipd attach --wsl`. |
| Ordinary **Upload** times out | There is no bootloader. Use **Upload Using Programmer**. |
| avrdude talks, then the pedal is dead | You probably wrote fuses (Burn Bootloader). Stop and recover with an external clock on XTAL; do not keep retrying random fuse values. |
| Isolator clicks during flash | Expected. Pin 11 is MOSI. |
| Version blink does not match the hex you flashed | Confirm the file name and that avrdude reported `avrdude done. Thank you.` without a verify error. |
| Lost presets | Restore your `my-presets.hex` backup, or flash `eeprom/quaverato-default-presets.hex`. |
| `Warning: no eeprom data found in Intel Hex file … Quaverato.ino.eep` | Harmless. The sketch has no EEPROM image to burn, so avrdude skips that file and leaves existing presets alone. |
| `_TASK_MICRO_RES` redefined | Harmless PlatformIO warning if a stale `-D_TASK_MICRO_RES` is in `platformio.ini`. The sketch already defines it. The build still succeeds. |
| PlatformIO **Upload** says success (or verify mismatch) and the pedal is dead | PIO’s Upload adds avrdude `-D` (no erase). Leftover bits from the previous firmware stay. `platformio.ini` must pass `-e`. Recover with Arduino IDE **Upload Using Programmer**. Do not add `-V` to hide a verify failure. |
