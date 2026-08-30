#include "state.h"
#include "eeprom_map.h"

const int minTapRate = 784;   // actual minimum is 784/4 = 196
const int maxTapRate = 15625; // actual maximum is 15625*2 = 31250 microseconds

unsigned long stepRate = 196;
volatile unsigned long firstHalfStepRate = 196;
volatile unsigned long secondHalfStepRate = 196;
float dutyCycle = 0.5;

volatile bool relayON = false;

volatile unsigned long debounceTimer = 0;
volatile unsigned long buttonTimer = 0;
volatile unsigned long tempoTapped = 0;

volatile byte currentWaveTable = 0;
volatile byte waveFormStep = 0;

byte depth = 16;
double tapDivisor = 1;
int floorOne = 32;
int floorTwo = 32;
volatile byte phaseOffset = 0;

int timeDivision = 2;

bool synchronize = true;

bool callibrationMode = false;
byte mixTarget = 255;

byte zeroCutoff = 0;

bool tableShift = false;

bool setMidiChannel = false;
bool midiOmni = false;
int midiChannel = 0;
volatile bool followMidiClock = false;
int clockCount = 0;

bool momentMode = false;
bool switchMode = false;
bool presetModeFlag = false;

int expressionSelect = 20;

int presetOffset = 0;
const int presetRoot = EEPROM_ADDR_PRESET_ROOT;

Preset currentPreset;
