#include "relay.h"

#include <Arduino.h>
#include "pins.h"
#include "state.h"
#include "tasks.h"

void flipRelay() {
  static unsigned long localButtonTimer = 0;
  noInterrupts();

  if (micros() - localButtonTimer > 25000) {
    digitalWrite(relay_pin_Isolator, LOW);
    relayPartTwo.enableDelayed(25000);
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
  relayCleanup.enableDelayed(8000);
  relayPartTwo.disable();
}

void resetRelay() {
  digitalWrite(relay_pin_Lock, LOW);
  digitalWrite(relay_pin_Switch, LOW);
  relayDisableIsolator.enableDelayed(5000);
  relayCleanup.disable();
}

void disableIsolator() {
  digitalWrite(relay_pin_Isolator, HIGH);
  relayDisableIsolator.disable();
}

void midiMomentRelay() {
  if (!momentMode) {
    flipRelay();
  }
  midiMomentaryMode.disable();
}
