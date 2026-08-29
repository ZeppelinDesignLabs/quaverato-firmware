#ifndef QUAVERATO_PRESETS_H
#define QUAVERATO_PRESETS_H

void presetMode();
void readPreset(int presetNumber);
void writePreset(int presetNumber);
void ensureEepromSchema();
void restoreFactoryPresets();
void armPresetStore();
void cancelPresetStore();
void servicePreset();

#endif
