#ifndef QUAVERATO_MIDI_HANDLERS_H
#define QUAVERATO_MIDI_HANDLERS_H

#include <Arduino.h>

void handleMIDI();
void setupMidi(int channel);
void setMidiInputChannel(int channel);

void handleControlChange(byte channel, byte number, byte value);
void handleProgramChange(byte channel, byte number);
void handleNoteOn(byte channel, byte note, byte velocity);
void handleNoteOff(byte channel, byte note, byte velocity);
void handleClock();
void handleStart();
void handleStop();
void handleContinue();

#endif
