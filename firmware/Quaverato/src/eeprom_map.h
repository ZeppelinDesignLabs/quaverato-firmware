#ifndef QUAVERATO_EEPROM_MAP_H
#define QUAVERATO_EEPROM_MAP_H

#include <Arduino.h>

// Bump this when Preset (or any other persisted layout) changes. ensureEepromSchema()
// treats 0x00 and 0xFF as "legacy, same 19-byte layout" so existing pedals keep their
// presets; any other mismatch restores the factory slots.
const byte EEPROM_SCHEMA_VERSION = 1;

const int EEPROM_ADDR_SCHEMA = 0;
const int EEPROM_ADDR_BYPASS_STATE = 1;
const int EEPROM_ADDR_MIDI_CHANNEL = 2;
const int EEPROM_ADDR_MIDI_OMNI = 3;
const int EEPROM_ADDR_PRESET_ROOT = 10;
const int EEPROM_PRESET_COUNT = 6;

struct Preset {
  byte depth;
  double multiplier;
  byte waveShape;
  unsigned long rate;
  float spacing;
  int harmonicMixHigh;
  int harmonicMixLow;
  bool phase;
};

static_assert(sizeof(Preset) == 19, "Preset is packed 19 bytes; bump EEPROM_SCHEMA_VERSION if this changes");

#endif
