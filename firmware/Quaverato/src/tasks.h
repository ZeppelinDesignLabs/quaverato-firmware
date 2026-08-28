#ifndef QUAVERATO_TASKS_H
#define QUAVERATO_TASKS_H

#ifndef _TASK_MICRO_RES
#define _TASK_MICRO_RES
#endif

#include <TaskSchedulerDeclarations.h>

extern Scheduler ts;
extern Task oscillator;
extern Task checkControls;
extern Task relayPartTwo;
extern Task relayCleanup;
extern Task relayDisableIsolator;
extern Task updateMidi;
extern Task storePreset;
extern Task midiMomentaryMode;

#endif
