#include "boot.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <MIDI.h>

#include "version.h"
#include "pins.h"
#include "eeprom_map.h"
#include "state.h"
#include "midi_handlers.h"

bool scanForCallibrationMode() {
  int counter = 0;
  while (!digitalRead(switch_pin_TapTempo) && !digitalRead(switch_pin_Bypass)) {
    delay(100);
    counter++;
    if (counter > 30) {
      return true;
    }
  }
  return false;
}

void enterCallibrationMode() {
  bool lightOn = false;
  callibrationMode = true;
  setMidiChannel = true;
  midiChannel = MIDI_CHANNEL_OMNI;
  EEPROM.update(EEPROM_ADDR_MIDI_OMNI, true);
  setMidiInputChannel(midiChannel);
  mixTarget = 0;
  for (int i = 0; i < 15; i++) {
    lightOn = !lightOn;
    digitalWrite(led_pin_Bypass, lightOn);
    digitalWrite(led_pin_TempoLed, lightOn);
    digitalWrite(led_pin_High, lightOn);
    digitalWrite(led_pin_Low, lightOn);
    delay(100);
  }
}

bool scanForModeChange() {
  int counter = 0;
  while (!digitalRead(switch_pin_Bypass)) {
    delay(100);
    counter++;
    if (counter >= 30) {
      return true;
    }
  }
  return false;
}

void changeMode() {
  // setup() loads this flag then calls flipRelay(), which toggles relayON.
  // So EEPROM true -> ends bypassed; EEPROM false -> ends engaged.
  bool eepromValue = !EEPROM.read(EEPROM_ADDR_BYPASS_STATE);
  EEPROM.update(EEPROM_ADDR_BYPASS_STATE, eepromValue);
  const bool engagedOnStartup = !eepromValue;
  // Manual: 3 blinks = engaged on startup, 5 blinks = bypassed on startup
  const int blinkCount = engagedOnStartup ? 3 : 5;
  for (int i = 0; i < blinkCount; i++) {
    digitalWrite(led_pin_Bypass, HIGH);
    delay(100);
    digitalWrite(led_pin_Bypass, LOW);
    delay(100);
  }
}

bool scanForVersionMode() {
  int counter = 0;
  while (!digitalRead(switch_pin_TapTempo)) {
    delay(100);
    counter++;
    if (counter >= 30) {
      return true;
    }
  }
  return false;
}

void runVersionMode() {
  // Blink version a few times then continue boot (avoid brick on shorted tap)
  for (int cycle = 0; cycle < 3; cycle++) {
    versionBlink(VERSION_MAJOR);
    versionBlink(VERSION_MINOR);
    versionBlink(VERSION_PATCH);
    digitalWrite(led_pin_TempoLed, LOW);
    delay(4000);
  }
}

void versionBlink(int numberOfBlinks) {
  // A zero digit is one long pulse. Blinking it exactly would be silence, so
  // 2.5.0 read as "2, 5" with an unexplained gap where the patch should be.
  // Every other digit stays an exact count, as the Troubleshooting Guide
  // documents ("2-4-2").
  const int onTime = (numberOfBlinks == 0) ? 600 : 150;
  const int pulses = (numberOfBlinks == 0) ? 1 : numberOfBlinks;
  for (int i = pulses; i > 0; i--) {
    digitalWrite(led_pin_TempoLed, LOW);
    delay(150);
    digitalWrite(led_pin_TempoLed, HIGH);
    delay(onTime);
  }
  digitalWrite(led_pin_TempoLed, LOW);
  delay(800);
}

int loadMidiChannelFromEeprom() {
  midiOmni = EEPROM.read(EEPROM_ADDR_MIDI_OMNI);
  if (midiOmni) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = EEPROM.read(EEPROM_ADDR_MIDI_CHANNEL);
    if (midiChannel == 0 || midiChannel > 16 || midiChannel == 0xFF) {
      midiChannel = 1;
    }
  }
  return midiChannel;
}
