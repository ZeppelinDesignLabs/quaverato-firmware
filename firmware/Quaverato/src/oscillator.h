#ifndef QUAVERATO_OSCILLATOR_H
#define QUAVERATO_OSCILLATOR_H

#include <Arduino.h>

void setupOscillator();
void enableOscillator();
void disableOscillator();
void stepWaveform();
void light(byte lightIntensity);
byte adjustLight(int lightIntensity, int mix);

unsigned long applyTapDivision(unsigned long rate);
void setTempo();
void splitDutyCycle(double duty, unsigned long rate);

#endif
