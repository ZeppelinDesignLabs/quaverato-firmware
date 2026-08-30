#include "controls.h"

#include <Arduino.h>
#include <util/atomic.h>
#include <math.h>

#include "pins.h"
#include "state.h"
#include "oscillator.h"
#include "relay.h"
#include "presets.h"

void serviceControls() {
  static unsigned long last = 0;
  const unsigned long now = micros();
  if (now - last < 4000) {
    return;
  }
  last = now;
  functionSwitch();
}

static byte safeLogDepth(int value) {
  if (value <= 0) {
    return 0;
  }
  int d = (int)(log10((double)value) * 16);
  return (byte)min(d, 32);
}

void functionSwitch() {
  static int function = 0;

  function++;
  if (function > 8) {
    function = 0;
  }

  switch (function) {
    case 0:
      ratePot();
      break;
    case 1:
      waveformPot();
      break;
    case 2:
      depthPot();
      break;
    case 3:
      divisionPot();
      break;
    case 4:
      harmonicMixPot();
      break;
    case 5:
      dutyCyclePot();
      break;
    case 6:
      phaseSwitch();
      break;
    case 7:
      modeSwitch();
      bypassSwitch();
      break;
    case 8:
      tapped();
      break;
  }
}

void phaseSwitch() {
  static bool oldReading = !digitalRead(switch_pin_Phase);
  bool reading = digitalRead(switch_pin_Phase);
  if (reading != oldReading) {
    synchronize = !reading;
    oldReading = reading;
  }
}

void modeSwitch() {
  static bool oldReading;
  bool reading = digitalRead(switch_pin_Mode);
  if (reading != oldReading && !presetModeFlag) {
    noInterrupts();
    if (reading == LOW) {
      if (!switchMode) {
        if (momentMode) {
          relayON = true;
          flipRelay();
          momentMode = false;
        }
        switchMode = true;
      }
    } else {
      momentMode = true;
      switchMode = false;
    }
    interrupts();
    oldReading = reading;
  }
}

void bypassSwitch() {
  static bool pushed = false;
  if (switchMode && !presetModeFlag) {
    if (digitalRead(switch_pin_Bypass) == LOW && !pushed) {
      pushed = true;
      armPresetStore();
      flipRelay();
    } else if (digitalRead(switch_pin_Bypass) == HIGH && pushed) {
      pushed = false;
      if (!presetModeFlag) {
        cancelPresetStore();
      }
    }
  } else if (momentMode) {
    if (digitalRead(switch_pin_Bypass) == relayON) {
      flipRelay();
    }
  }
}

void ratePot() {
  static int oldReading = -10;
  static int oldStepRate = -1;
  int reading = analogRead(pot_pin_Rate);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    stepRate = map(reading, 0, 1023, 125, 16);
    stepRate *= stepRate;
    if (stepRate != oldStepRate) {
      setTempo();
      oldStepRate = stepRate;
    }
    expressionSelect = 24;
    oldReading = reading;
  }
}

void waveformPot() {
  static int oldReading = -10;
  int reading = analogRead(pot_pin_Shape);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    currentWaveTable = map(reading, -100, 900, 0, 4);
    currentWaveTable = constrain(currentWaveTable, 0, 4);
    expressionSelect = 22;
    oldReading = reading;
  }
}

void depthPot() {
  static int oldReading = -10;

  int reading = analogRead(pot_pin_Depth);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    int mapped = map(reading, 0, 1023, 1, 102);
    depth = safeLogDepth(mapped);
    expressionSelect = 20;
    oldReading = reading;
  }
}

void divisionPot() {
  static int oldReading = -10;
  static int timeDivisionOld = -10;

  int reading = analogRead(pot_pin_TimeDivision);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    timeDivision = map(reading, -150, 900, 1, 6);
    timeDivision = constrain(timeDivision, 1, 6);
    if (timeDivision != timeDivisionOld && !presetModeFlag) {
      switch (timeDivision) {
        case 1:
          tapDivisor = 0.5;
          break;
        case 2:
          tapDivisor = 1.0;
          break;
        case 3:
          tapDivisor = 1.5;
          break;
        case 4:
          tapDivisor = 2.0;
          break;
        case 5:
          tapDivisor = 3.0;
          break;
        case 6:
          tapDivisor = 4.0;
          break;
      }
      setTempo();
      oldReading = reading;
      timeDivisionOld = timeDivision;
    }
    expressionSelect = 21;
    oldReading = reading;
  }
}

void harmonicMixPot() {
  static int oldReading = -10;

  int reading = analogRead(pot_pin_HarmonicMix);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    // THROW AWAY: both bands stay full; the knob is 0–360° of high-side offset.
    floorOne = 32;
    floorTwo = 32;
    zeroCutoff = 0;
    phaseOffset = (byte)map(reading, 0, 1023, 0, 255);
    expressionSelect = 26;
    oldReading = reading;
  }
}

void dutyCyclePot() {
  static int oldReading = -10;

  int reading = analogRead(pot_pin_DutyCycle);
  if (reading >= oldReading + 10 || reading <= oldReading - 10) {
    dutyCycle = (float)reading / 1023;
    dutyCycle = constrain(dutyCycle, 0.0625, 0.9375);
    setTempo();
    expressionSelect = 25;
    oldReading = reading;
  }
}

void tap() {
  if (micros() - debounceTimer > 10000) {
    if (micros() - buttonTimer < 3000000) {
      tempoTapped = micros() - buttonTimer;
    }
    buttonTimer = micros();
    debounceTimer = micros();
    attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), debounceTap, RISING);
  }
}

void debounceTap() {
  if (micros() - debounceTimer > 10000) {
    debounceTimer = micros();
    attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), tap, FALLING);
  }
}

void tapped() {
  unsigned long tappedValue = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    tappedValue = tempoTapped;
    if (tappedValue > 0) {
      tempoTapped = 0;
    }
  }
  if (tappedValue > 0) {
    stepRate = constrain(tappedValue / 256, minTapRate, maxTapRate);
    setTempo();
  }
}
