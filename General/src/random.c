
/*
 *
 * random.c - The random number generator
 *
 */

#include "common.h"
#include "random.h"

// -----------------------------------------------------------------------
// variables
// -----------------------------------------------------------------------

static long long randomSeed;

// -----------------------------------------------------------------------
// set the seed
// -----------------------------------------------------------------------

void randomSetSeed(long long seed) {

  randomSeed = seed;
}

// -----------------------------------------------------------------------
// return a random number
// -----------------------------------------------------------------------

int  randomNextInt(void) {

  randomSeed = (randomSeed * 0x5DEECE66DLL + 0xB) & ((1LL << 48) - 1);
  return (int)(randomSeed >> (48 - 32));
}
