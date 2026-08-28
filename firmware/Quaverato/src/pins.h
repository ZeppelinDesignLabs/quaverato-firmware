#ifndef QUAVERATO_PINS_H
#define QUAVERATO_PINS_H

#include <Arduino.h>

const byte led_pin_TempoLed = 3;
const byte led_pin_Low = 10;
const byte led_pin_High = 9;
const byte led_pin_Bypass = 4;

const byte midi_pin_Input = 0;

const byte switch_pin_TapTempo = 2;
const byte switch_pin_Bypass = 1;
const byte switch_pin_Mode = 5;
const byte switch_pin_Phase = 6;

const byte relay_pin_Switch = 8;
const byte relay_pin_Lock = 7;
const byte relay_pin_Isolator = 11;

// Analog pin numbers
const byte pot_pin_Rate = 1;
const byte pot_pin_Shape = 4;
const byte pot_pin_Depth = 0;
const byte pot_pin_DutyCycle = 3;
const byte pot_pin_TimeDivision = 2;
const byte pot_pin_HarmonicMix = 5;

#endif
