
/*
 *
 * main.c - The PSP main loop
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psptypes.h>
#include <pspiofilemgr.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>
#include <psprtc.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <string.h>

#include "common.h"
#include "main.h"
#include "audio.h"
#include "video.h"
#include "input.h"
#include "random.h"
#include "game.h"
#include "network.h"

// ---------------------------------------------------------------------------
// PSP info
// ---------------------------------------------------------------------------

PSP_MODULE_INFO("All the Same Test PSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

// ---------------------------------------------------------------------------
// variables (INTERNAL)
// ---------------------------------------------------------------------------

// did we get a request to exit?
static int mainExit = NO;

// ---------------------------------------------------------------------------
// the callbacks
// ---------------------------------------------------------------------------

int mainCallbackExit(int arg1, int arg2, void *common) {

	mainExit = YES;

	return 0;
}

int mainCallbackThread(SceSize args, void *argp) {

	int callbackID;

	callbackID = sceKernelCreateCallback("Exit Callback", mainCallbackExit, NULL);
	sceKernelRegisterExitCallback(callbackID);
	sceKernelSleepThreadCB();

	return 0;
}

int mainSetupCallbacks(void) {

	int threadID;

	threadID = sceKernelCreateThread("update_thread", mainCallbackThread, 0x11, 0xFA0, 0, 0);
	if (threadID >= 0)
		sceKernelStartThread(threadID, 0, NULL);

	return threadID;
}

// ---------------------------------------------------------------------------
// the main loop
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	float deltaTime, tickResolution;
	u64 tickLast, tickCurrent;

	mainSetupCallbacks();

	// init the clock
	tickResolution = sceRtcGetTickResolution() / 1000.0f;
	sceRtcGetCurrentTick(&tickLast);

	// init the subsystems
	audioInit();
	videoInit();
	inputInit();
	randomSetSeed((long long)time(NULL));
	gameInit();
	networkInit();

	// load the configuration file
	//saveLoad();

	// main loop
	while (mainExit == NO) {
		// calculate deltaTime
		sceRtcGetCurrentTick(&tickCurrent);
		deltaTime = (tickCurrent - tickLast) / tickResolution;
		tickLast = tickCurrent;

		// read input
		inputRead();

		// run one iteration
		gameIteration(deltaTime);

		// draw the screen
		gameDraw();

		// swap the draw buffers
		videoSwapBuffers();
	}

	sceGuTerm();
	sceKernelExitGame();

	return 0;
}
