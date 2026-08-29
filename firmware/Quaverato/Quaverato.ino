#include <EEPROM.h>

#include "src/version.h"
#include "src/pins.h"
#include "src/eeprom_map.h"
#include "src/state.h"
#include "src/wavetables.h"
#include "src/oscillator.h"
#include "src/controls.h"
#include "src/midi_handlers.h"
#include "src/relay.h"
#include "src/presets.h"
#include "src/boot.h"

void setup() {
  // Timer1 (pins 9/10 opto LEDs): phase-correct 8-bit, prescaler 1 → ~31.25 kHz.
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;
  // Timer2 (pin 3 tap LED): Arduino defaults are phase-correct, prescaler 64 →
  // 490 Hz flicker. Fast PWM + prescaler 1 → 62.5 kHz. Pin 11 (OC2A / isolator)
  // is digitalWrite only, so the waveform change does not drive it.
  TCCR2A = (TCCR2A & 0b11111100) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;

  pinMode(led_pin_TempoLed, OUTPUT);
  pinMode(led_pin_Bypass, OUTPUT);
  pinMode(relay_pin_Lock, OUTPUT);
  pinMode(relay_pin_Switch, OUTPUT);
  pinMode(relay_pin_Isolator, OUTPUT);
  pinMode(switch_pin_TapTempo, INPUT_PULLUP);
  pinMode(switch_pin_Mode, INPUT_PULLUP);
  pinMode(switch_pin_Phase, INPUT_PULLUP);
  pinMode(switch_pin_Bypass, INPUT_PULLUP);
  pinMode(midi_pin_Input, INPUT);

  digitalWrite(switch_pin_Bypass, HIGH);

  ensureEepromSchema();

  if (scanForCallibrationMode()) {
    enterCallibrationMode();
  }
  if (scanForVersionMode()) {
    runVersionMode();
  }
  if (scanForModeChange()) {
    changeMode();
  }
  delay(200);
  relayON = EEPROM.read(EEPROM_ADDR_BYPASS_STATE);
  flipRelay();

  switchMode = !digitalRead(switch_pin_Mode);
  momentMode = !switchMode;
  synchronize = digitalRead(switch_pin_Phase);

  attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), tap, FALLING);

  loadMidiChannelFromEeprom();
  setupMidi(midiChannel);

  setupOscillator();
  enableOscillator();
}

void loop() {
  handleMIDI();
  serviceControls();
  serviceRelay();
  servicePreset();
}
