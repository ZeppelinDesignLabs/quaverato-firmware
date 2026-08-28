#include "wavetables.h"

#include "wavetables/sine256.h"
#include "wavetables/saw256.h"
#include "wavetables/ramp256.h"
#include "wavetables/tri256.h"
#include "wavetables/sq256.h"

const byte *const waveTable[WAVE_TABLE_COUNT] = {
  &sine256[0],
  &saw256[0],
  &ramp256[0],
  &tri256[0],
  &sq256[0]
};
