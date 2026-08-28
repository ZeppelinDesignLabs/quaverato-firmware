#ifndef QUAVERATO_WAVETABLES_H
#define QUAVERATO_WAVETABLES_H

#include <Arduino.h>
#include <avr/pgmspace.h>

extern PROGMEM const byte sine256[];
extern PROGMEM const byte saw256[];
extern PROGMEM const byte ramp256[];
extern PROGMEM const byte tri256[];
extern PROGMEM const byte sq256[];

enum {
  WAVE_TABLE_COUNT = 5
};

extern const byte *const waveTable[WAVE_TABLE_COUNT];

#endif
