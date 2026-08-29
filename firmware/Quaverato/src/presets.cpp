#include "presets.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#include "eeprom_map.h"
#include "pins.h"
#include "state.h"
#include "oscillator.h"

static unsigned long presetArmAt = 0;
static unsigned long presetBlinkAt = 0;
static bool presetArmed = false;
static bool presetBlinking = false;

// Matches eeprom/quaverato-default-presets.hex. Used only when the schema byte
// is neither current nor the legacy 0x00 / 0xFF "same layout" markers.
static const Preset kFactoryPresets[EEPROM_PRESET_COUNT] PROGMEM = {
  {32, 0.5, 0, 2704UL, 0.475f, 32, 32, true},
  {32, 1.0, 0, 1296UL, 0.475f, 32, 32, true},
  {32, 1.5, 1, 3025UL, 0.475f, 32, 32, true},
  {32, 2.0, 2, 2500UL, 0.475f, 32, 32, true},
  {32, 2.0, 4, 4489UL, 0.243f, 32, 32, true},
  {32, 4.0, 0, 1681UL, 0.497f, 32, 32, true},
};

void restoreFactoryPresets() {
  for (int i = 0; i < EEPROM_PRESET_COUNT; i++) {
    Preset preset;
    memcpy_P(&preset, &kFactoryPresets[i], sizeof(preset));
    EEPROM.put(EEPROM_ADDR_PRESET_ROOT + (i * sizeof(preset)), preset);
  }
}

void ensureEepromSchema() {
  const byte schema = EEPROM.read(EEPROM_ADDR_SCHEMA);
  if (schema == EEPROM_SCHEMA_VERSION) {
    return;
  }
  // Virgin EEPROM and every image shipped so far leave address 0 erased (0xFF).
  // 0x00 was also never a schema we wrote. Both mean "current 19-byte layout".
  if (schema != 0x00 && schema != 0xFF) {
    restoreFactoryPresets();
  }
  EEPROM.update(EEPROM_ADDR_SCHEMA, EEPROM_SCHEMA_VERSION);
}

void armPresetStore() {
  presetArmed = true;
  presetBlinking = false;
  presetArmAt = micros() + 5000000UL;
}

void cancelPresetStore() {
  presetArmed = false;
  presetBlinking = false;
}

void servicePreset() {
  if (presetArmed && (long)(micros() - presetArmAt) >= 0) {
    presetArmed = false;
    presetBlinking = true;
    presetBlinkAt = micros();
  }
  if (presetBlinking && (long)(micros() - presetBlinkAt) >= 20000) {
    presetBlinkAt = micros();
    presetMode();
    if (!presetModeFlag) {
      presetBlinking = false;
    }
  }
}

void presetMode() {
  static bool blinkLight = false;
  presetModeFlag = true;
  digitalWrite(led_pin_Bypass, blinkLight);
  blinkLight = !blinkLight;
  if (digitalRead(switch_pin_Bypass) == HIGH) {
    writePreset(timeDivision - 1);
    presetModeFlag = false;
    cancelPresetStore();
  }
}

void readPreset(int presetNumber) {
  EEPROM.get(presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
  depth = constrain(currentPreset.depth, 0, 32);
  tapDivisor = constrain(currentPreset.multiplier, 0.5, 4.0);
  currentWaveTable = constrain(currentPreset.waveShape, 0, 4);
  stepRate = constrain(currentPreset.rate, minTapRate, maxTapRate);
  dutyCycle = constrain(currentPreset.spacing, 0.0625, 0.9375);
  floorOne = constrain(currentPreset.harmonicMixHigh, 0, 32);
  floorTwo = constrain(currentPreset.harmonicMixLow, 0, 32);
  synchronize = currentPreset.phase;
  splitDutyCycle(dutyCycle, applyTapDivision(stepRate));
}

void writePreset(int presetNumber) {
  digitalWrite(led_pin_Bypass, LOW);
  delay(1000);
  currentPreset = {depth, tapDivisor, currentWaveTable, stepRate, dutyCycle, floorOne, floorTwo, synchronize};
  EEPROM.put(presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
  for (int i = 0; i <= presetNumber; i++) {
    digitalWrite(led_pin_Bypass, HIGH);
    delay(400);
    digitalWrite(led_pin_Bypass, LOW);
    delay(400);
  }
}
