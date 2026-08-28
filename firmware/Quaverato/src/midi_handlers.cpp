#include "midi_handlers.h"

#include <MIDI.h>
#include <EEPROM.h>
#include <math.h>

#include "eeprom_map.h"
#include "state.h"
#include "tasks.h"
#include "oscillator.h"
#include "relay.h"
#include "presets.h"
#include "controls.h"

MIDI_CREATE_DEFAULT_INSTANCE();

static byte safeLogDepth(int value) {
  if (value <= 0) {
    return 0;
  }
  int d = (int)(log10((double)value) * 16);
  return (byte)min(d, 32);
}

void handleMIDI() {
  MIDI.read();
}

void setMidiInputChannel(int channel) {
  MIDI.setInputChannel(channel);
}

void setupMidi(int channel) {
  MIDI.begin(channel);
  MIDI.turnThruOff();
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleContinue(handleContinue);
}

void handleControlChange(byte channel, byte number, byte value) {
  if (setMidiChannel) {
    midiChannel = channel;
    EEPROM.update(EEPROM_ADDR_MIDI_CHANNEL, channel);
    EEPROM.update(EEPROM_ADDR_MIDI_OMNI, false);
    midiOmni = false;
    MIDI.setInputChannel(midiChannel);
    setMidiChannel = false;
  }
  switch (number) {
    case 20:
      depth = safeLogDepth(value);
      break;
    case 21:
      if (value <= 21) {
        tapDivisor = 0.5;
      } else if (value <= 43) {
        tapDivisor = 1.0;
      } else if (value <= 65) {
        tapDivisor = 1.5;
      } else if (value <= 87) {
        tapDivisor = 2.0;
      } else if (value <= 109) {
        tapDivisor = 3.0;
      } else {
        tapDivisor = 4.0;
      }
      setTempo();
      break;
    case 22:
      if (value <= 25) {
        currentWaveTable = 0;
      } else if (value <= 50) {
        currentWaveTable = 1;
      } else if (value <= 75) {
        currentWaveTable = 2;
      } else if (value <= 100) {
        currentWaveTable = 3;
      } else {
        currentWaveTable = 4;
      }
      break;
    case 24:
      stepRate = 143 - value; // minimum = 16
      stepRate *= stepRate;
      stepRate = constrain(stepRate, minTapRate, maxTapRate);
      setTempo();
      break;
    case 25:
      dutyCycle = (float)value / 127;
      dutyCycle = constrain(dutyCycle, 0.0625, 0.9375);
      setTempo();
      break;
    case 26:
      if (value > 63) {
        floorOne = 32;
        floorTwo = map(value, 64, 127, 100, 0);
        floorTwo = safeLogDepth(floorTwo);
      } else {
        floorOne = map(value, 0, 63, 0, 100);
        floorOne = safeLogDepth(floorOne);
        floorTwo = 32;
      }
      zeroCutoff = abs(floorOne - floorTwo);
      if (callibrationMode) {
        if (floorOne != 32) {
          floorOne = 0;
        }
        if (floorTwo != 32) {
          floorTwo = 0;
        }
      }
      break;
    case 27:
      if (value > 63) {
        synchronize = true;
      } else {
        synchronize = false;
      }
      break;
    case 28:
      // Keep shipped polarity: value > 63 => momentMode
      if (value > 63) {
        momentMode = true;
        switchMode = false;
      } else {
        momentMode = false;
        switchMode = true;
      }
      break;
    case 29:
      flipRelay();
      break;
    case 30:
      handleControlChange(channel, expressionSelect, value);
      break;
    case 35:
      // Chart: 0-63 = INVERTED, 64-127 = FACTORY
      tableShift = (value <= 63);
      break;
    case 51:
      if (value > 63) {
        followMidiClock = true;
      } else {
        followMidiClock = false;
      }
      break;
    case 93:
      tap();
      break;
    case 124:
      midiChannel = EEPROM.read(EEPROM_ADDR_MIDI_CHANNEL);
      if (midiChannel == 0 || midiChannel > 16 || midiChannel == 0xFF) {
        midiChannel = 1;
      }
      EEPROM.update(EEPROM_ADDR_MIDI_OMNI, false);
      midiOmni = false;
      MIDI.setInputChannel(midiChannel);
      break;
    case 125:
      midiChannel = MIDI_CHANNEL_OMNI;
      EEPROM.update(EEPROM_ADDR_MIDI_OMNI, true);
      midiOmni = true;
      MIDI.setInputChannel(midiChannel);
      break;
    default:
      break;
  }
}

void handleProgramChange(byte channel, byte number) {
  const int minPreset = 0;
  const int maxPreset = EEPROM_PRESET_COUNT - 1;
  if (setMidiChannel) {
    midiChannel = channel;
    EEPROM.update(EEPROM_ADDR_MIDI_CHANNEL, channel);
    // Calibration mode leaves the omni flag set, so learning the channel has to
    // clear it or the next boot reverts to omni. Same as the CC learn path.
    EEPROM.update(EEPROM_ADDR_MIDI_OMNI, false);
    midiOmni = false;
    MIDI.setInputChannel(midiChannel);
    setMidiChannel = false;
  }
  if (number >= minPreset && number <= maxPreset) {
    readPreset(number);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  } else if (!relayON) {
    midiMomentaryMode.enableDelayed(20000);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (relayON) {
    midiMomentaryMode.enableDelayed(20000);
  }
}

void handleClock() {
  // Counting only while following keeps clockCount from running away (and
  // eventually overflowing) on a rig that streams clock we are ignoring, and
  // means enabling CC 51 mid-stream starts from a whole beat.
  if (!followMidiClock) {
    clockCount = 0;
    return;
  }
  clockCount++;
  if (clockCount >= 24) {
    if (micros() - buttonTimer < 3000000) {
      tempoTapped = micros() - buttonTimer;
    }
    buttonTimer = micros();
    clockCount = 0;
  }
}

void handleStart() {
  if (followMidiClock) {
    clockCount = 0;
    buttonTimer = micros();
    // Continuous tableShift invert is applied in light(); always start at phase 0
    waveFormStep = 0;
    oscillator.setInterval(firstHalfStepRate);
    oscillator.enable();
  }
}

void handleStop() {
  if (followMidiClock) {
    oscillator.disable();
  }
}

void handleContinue() {
  if (followMidiClock) {
    oscillator.enable();
  }
}
