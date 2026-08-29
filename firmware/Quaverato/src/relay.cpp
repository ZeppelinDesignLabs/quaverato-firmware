#include "relay.h"

#include <Arduino.h>
#include "pins.h"
#include "state.h"

static unsigned long relayTwoAt = 0;
static unsigned long relayCleanupAt = 0;
static unsigned long relayIsolatorAt = 0;
static unsigned long midiMomentAt = 0;
static bool relayTwoArmed = false;
static bool relayCleanupArmed = false;
static bool relayIsolatorArmed = false;
static bool midiMomentArmed = false;

static bool deadlineReached(unsigned long at) {
  return (long)(micros() - at) >= 0;
}

void flipRelay() {
  static unsigned long localButtonTimer = 0;
  noInterrupts();

  if (micros() - localButtonTimer > 25000) {
    digitalWrite(relay_pin_Isolator, LOW);
    relayTwoAt = micros() + 25000;
    relayTwoArmed = true;
  }
  localButtonTimer = micros();
  interrupts();
}

void flipRelayPartTwo() {
  relayON = !relayON;
  digitalWrite(led_pin_Bypass, relayON);
  digitalWrite(relay_pin_Lock, LOW);
  digitalWrite(relay_pin_Switch, LOW);
  digitalWrite(relay_pin_Lock, !relayON);
  digitalWrite(relay_pin_Switch, relayON);
  relayCleanupAt = micros() + 8000;
  relayCleanupArmed = true;
  relayTwoArmed = false;
}

void resetRelay() {
  digitalWrite(relay_pin_Lock, LOW);
  digitalWrite(relay_pin_Switch, LOW);
  relayIsolatorAt = micros() + 5000;
  relayIsolatorArmed = true;
  relayCleanupArmed = false;
}

void disableIsolator() {
  digitalWrite(relay_pin_Isolator, HIGH);
  relayIsolatorArmed = false;
}

void armMidiMomentary() {
  midiMomentAt = micros() + 20000;
  midiMomentArmed = true;
}

void midiMomentRelay() {
  if (!momentMode) {
    flipRelay();
  }
  midiMomentArmed = false;
}

void serviceRelay() {
  if (relayTwoArmed && deadlineReached(relayTwoAt)) {
    flipRelayPartTwo();
  }
  if (relayCleanupArmed && deadlineReached(relayCleanupAt)) {
    resetRelay();
  }
  if (relayIsolatorArmed && deadlineReached(relayIsolatorAt)) {
    disableIsolator();
  }
  if (midiMomentArmed && deadlineReached(midiMomentAt)) {
    midiMomentRelay();
  }
}
