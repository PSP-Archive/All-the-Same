
/*
 *
 * system.c - The system functions
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>
#include <psprtc.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "system.h"

// ---------------------------------------------------------------------------
// overclock the system?
// ---------------------------------------------------------------------------

void systemOverclock(int overclock) {

	if (overclock == YES)
		scePowerSetClockFrequency(333, 333, 166);
	else
		scePowerSetClockFrequency(222, 222, 111);
}
