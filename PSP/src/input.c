
/*
 *
 * input.c - The input manager
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "input.h"

// ---------------------------------------------------------------------------
// variables
// ---------------------------------------------------------------------------

// the current key bits
int inputKeyBits;

// the mouse press status
int inputMousePressed;

// the mouse coordinates (on a 1080p virtual screen)
int inputMouseX;
int inputMouseY;

// ---------------------------------------------------------------------------
// init the input manager
// ---------------------------------------------------------------------------

void inputInit(void) {

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

	// reset the keys
	inputKeyBits = 0;

	// reset the mouse
	inputMousePressed = NO;
}

// ---------------------------------------------------------------------------
// read the input
// ---------------------------------------------------------------------------

void inputRead(void) {

	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);

	// parse the bits
	inputKeyBits = 0;

	if (pad.Buttons & PSP_CTRL_SELECT)
		inputKeyBits |= INPUT_KEY_BIT_SELECT;
	if (pad.Buttons & PSP_CTRL_START)
		inputKeyBits |= INPUT_KEY_BIT_START;
	if (pad.Buttons & PSP_CTRL_UP)
		inputKeyBits |= INPUT_KEY_BIT_UP;
	if (pad.Buttons & PSP_CTRL_DOWN)
		inputKeyBits |= INPUT_KEY_BIT_DOWN;
	if (pad.Buttons & PSP_CTRL_LEFT)
		inputKeyBits |= INPUT_KEY_BIT_LEFT;
	if (pad.Buttons & PSP_CTRL_RIGHT)
		inputKeyBits |= INPUT_KEY_BIT_RIGHT;
	if (pad.Buttons & PSP_CTRL_LTRIGGER)
		inputKeyBits |= INPUT_KEY_BIT_L;
	if (pad.Buttons & PSP_CTRL_RTRIGGER)
		inputKeyBits |= INPUT_KEY_BIT_R;
	if (pad.Buttons & PSP_CTRL_TRIANGLE)
		inputKeyBits |= INPUT_KEY_BIT_Y;
	if (pad.Buttons & PSP_CTRL_CIRCLE)
		inputKeyBits |= INPUT_KEY_BIT_B;
	if (pad.Buttons & PSP_CTRL_SQUARE)
		inputKeyBits |= INPUT_KEY_BIT_X;
	if (pad.Buttons & PSP_CTRL_CROSS)
		inputKeyBits |= INPUT_KEY_BIT_A;
}
