#include "oscillator.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "pins.h"
#include "state.h"
#include "wavetables.h"

// Timer1 phase-correct 8-bit, prescaler 1: overflow period is 510 / 16 MHz =
// 31.875 µs. Tick units are eighths of a microsecond so that period is exact
// (255/8). The LFO still advances one wavetable step at a time; DDS replaces
// this interval logic next.
static const uint16_t TICKS_PER_OVF = 255;
static volatile bool lfoRunning = false;
static volatile uint32_t tickAccum = 0;
static volatile uint32_t intervalTicks = 784UL * 8;

static uint32_t intervalToTicks(unsigned long us) {
  if (us == 0) {
    us = 1;
  }
  return us * 8UL;
}

static void latchIntervalFromStep() {
  const unsigned long us =
      (waveFormStep < 128) ? firstHalfStepRate : secondHalfStepRate;
  intervalTicks = intervalToTicks(us);
}

void setupOscillator() {
  latchIntervalFromStep();
  TIMSK1 |= _BV(TOIE1);
}

void enableOscillator() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    latchIntervalFromStep();
    lfoRunning = true;
  }
}

void disableOscillator() {
  lfoRunning = false;
}

void stepWaveform() {
  light(pgm_read_byte_near(waveTable[currentWaveTable] + waveFormStep));
  waveFormStep++;
  if (!followMidiClock || waveFormStep == 0 || waveFormStep == 128) {
    latchIntervalFromStep();
  }
}

ISR(TIMER1_OVF_vect) {
  if (!lfoRunning) {
    return;
  }
  tickAccum += TICKS_PER_OVF;
  uint8_t steps = 0;
  while (tickAccum >= intervalTicks && steps < 4) {
    tickAccum -= intervalTicks;
    stepWaveform();
    steps++;
  }
}

void light(byte lightIntensity) {
  // Continuous LFO invert for stereo (CC35); works without MIDI clock
  if (tableShift) {
    lightIntensity = 255 - lightIntensity;
  }

  lightIntensity = lightIntensity < zeroCutoff ? 0 : lightIntensity;
  byte lightOne = adjustLight(lightIntensity, floorOne);
  if (!synchronize) {
    lightIntensity = 255 - lightIntensity;
  }
  byte lightTwo = adjustLight(lightIntensity, floorTwo);

  if (floorOne >= floorTwo) {
    analogWrite(led_pin_TempoLed, lightOne);
    lightTwo = callibrationMode ? 0 : lightTwo;
  } else {
    analogWrite(led_pin_TempoLed, lightTwo);
    lightOne = callibrationMode ? 0 : lightOne;
  }
  analogWrite(led_pin_High, lightOne);
  analogWrite(led_pin_Low, lightTwo);
}

byte adjustLight(int lightIntensity, int mix) {
  lightIntensity = (lightIntensity * mix) / 32;
  lightIntensity = lightIntensity + ((mixTarget * (32 - mix)) / 32);
  lightIntensity = (lightIntensity * depth) / 32;
  lightIntensity = lightIntensity + ((255 * (32 - depth)) / 32);
  return (byte)lightIntensity;
}

unsigned long applyTapDivision(unsigned long rate) {
  return (unsigned long)(rate / tapDivisor);
}

void setTempo() {
  splitDutyCycle(dutyCycle, applyTapDivision(stepRate));
  enableOscillator();
}

void splitDutyCycle(double duty, unsigned long rate) {
  if (!synchronize) {
    rate *= 2;
  }
  const unsigned long first = 2 * rate * duty;
  const unsigned long second = 2 * rate * (1 - duty);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    firstHalfStepRate = first;
    secondHalfStepRate = second;
    if (!followMidiClock) {
      latchIntervalFromStep();
    }
  }
}
