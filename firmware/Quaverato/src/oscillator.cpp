#include "oscillator.h"

#include "pins.h"
#include "state.h"
#include "tasks.h"
#include "wavetables.h"

void stepWaveform() {
  light(pgm_read_byte_near(waveTable[currentWaveTable] + waveFormStep));
  waveFormStep++;
  if (waveFormStep < 128) {
    if (!followMidiClock || waveFormStep == 0) {
      oscillator.setInterval(firstHalfStepRate);
    }
  }
  if (waveFormStep >= 128) {
    if (!followMidiClock || waveFormStep == 128) {
      oscillator.setInterval(secondHalfStepRate);
    }
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
  oscillator.enable();
}

void splitDutyCycle(double duty, unsigned long rate) {
  if (!synchronize) {
    rate *= 2;
  }
  firstHalfStepRate = 2 * rate * duty;
  secondHalfStepRate = 2 * rate * (1 - duty);
}
