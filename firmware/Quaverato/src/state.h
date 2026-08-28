#ifndef QUAVERATO_STATE_H
#define QUAVERATO_STATE_H

#include <Arduino.h>
#include "eeprom_map.h"

extern const int minTapRate;
extern const int maxTapRate;

extern unsigned long stepRate;
extern unsigned long firstHalfStepRate;
extern unsigned long secondHalfStepRate;
extern float dutyCycle;

extern volatile bool relayON;

extern volatile unsigned long debounceTimer;
extern volatile unsigned long buttonTimer;
extern volatile unsigned long tempoTapped;

extern byte currentWaveTable;
extern byte waveFormStep;

extern byte depth;
extern double tapDivisor;
extern int floorOne;
extern int floorTwo;

extern int timeDivision;

extern bool synchronize;

extern bool callibrationMode;
extern byte mixTarget;

extern byte zeroCutoff;

extern bool tableShift;

extern bool setMidiChannel;
extern bool midiOmni;
extern int midiChannel;
extern bool followMidiClock;
extern int clockCount;

extern bool momentMode;
extern bool switchMode;
extern bool presetModeFlag;

extern int expressionSelect;

extern int presetOffset;
extern const int presetRoot;

extern Preset currentPreset;

#endif
