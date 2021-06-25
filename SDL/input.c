
/*
 *
 * input.c - The input manager
 *
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely used stuff from Windows headers

#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#endif

#ifdef LINUX
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#endif

#include "SDL.h"

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

	// reset the keys
	inputKeyBits = 0;

	// reset the mouse
	inputMousePressed = NO;
}

// ---------------------------------------------------------------------------
// read the input
// ---------------------------------------------------------------------------

void inputRead(void) {

	unsigned char *keys = SDL_GetKeyState(NULL);
	int i;

	// parse the key bits
	inputKeyBits = 0;

	if (keys[SDLK_l])
		inputKeyBits |= INPUT_KEY_BIT_L;
	if (keys[SDLK_r])
		inputKeyBits |= INPUT_KEY_BIT_R;
	if (keys[SDLK_a])
		inputKeyBits |= INPUT_KEY_BIT_A;
	if (keys[SDLK_b])
		inputKeyBits |= INPUT_KEY_BIT_B;
	if (keys[SDLK_x])
		inputKeyBits |= INPUT_KEY_BIT_X;
	if (keys[SDLK_y])
		inputKeyBits |= INPUT_KEY_BIT_Y;
	if (keys[SDLK_ESCAPE])
		inputKeyBits |= INPUT_KEY_BIT_QUIT;
	if (keys[SDLK_RETURN])
		inputKeyBits |= INPUT_KEY_BIT_START;
	if (keys[SDLK_SPACE])
		inputKeyBits |= INPUT_KEY_BIT_SELECT;
	if (keys[SDLK_LEFT])
		inputKeyBits |= INPUT_KEY_BIT_LEFT;
	if (keys[SDLK_RIGHT])
		inputKeyBits |= INPUT_KEY_BIT_RIGHT;
	if (keys[SDLK_DOWN])
		inputKeyBits |= INPUT_KEY_BIT_DOWN;
	if (keys[SDLK_UP])
		inputKeyBits |= INPUT_KEY_BIT_UP;

	// mouse
	i = SDL_GetMouseState(&inputMouseX, &inputMouseY);
	if ((i & SDL_BUTTON(1)) != 0)
		inputMousePressed = YES;
	else
		inputMousePressed = NO;

	// convert x and y (emulate a 1080p screen)

}
