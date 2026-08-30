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
  light(waveFormStep);
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

void light(byte step) {
  // THROW AWAY: true delay on the high band, not 255-x inversion.
  // Low reads table[step]; high reads table[step + phaseOffset].
  const byte *table = waveTable[currentWaveTable];
  byte highSample = pgm_read_byte_near(table + (byte)(step + phaseOffset));
  byte lowSample = pgm_read_byte_near(table + step);
  if (tableShift) {
    highSample = 255 - highSample;
    lowSample = 255 - lowSample;
  }
  byte lightOne = adjustLight(highSample, floorOne);
  byte lightTwo = adjustLight(lowSample, floorTwo);
  analogWrite(led_pin_TempoLed, lightOne);
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
  // THROW AWAY: do not double the period in "harmonic" — PHASE is unused;
  // the mix knob is the only inter-band relationship.
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
