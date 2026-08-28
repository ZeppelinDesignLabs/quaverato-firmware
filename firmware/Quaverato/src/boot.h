#ifndef QUAVERATO_BOOT_H
#define QUAVERATO_BOOT_H

bool scanForCallibrationMode();
void enterCallibrationMode();
bool scanForModeChange();
void changeMode();
bool scanForVersionMode();
void runVersionMode();
void versionBlink(int numberOfBlinks);
int loadMidiChannelFromEeprom();

#endif
