
/*
 *
 * main.c - The SDL main loop
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
#include "SDL_opengl.h"

#include "common.h"
#include "random.h"
#include "main.h"
#include "audio.h"
#include "video.h"
#include "input.h"
#include "game.h"
#include "network.h"

// -----------------------------------------------------------------------
// variables
// -----------------------------------------------------------------------

// exit the main loop?
int mainExit = NO;

// the screen dimensions
int screenDX = 480*2, screenDY = 272*2;

// -----------------------------------------------------------------------
// reshape the window
// -----------------------------------------------------------------------

static void mainReshape(int width, int height) {

  glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
	glOrtho(-4.0f, 4.0f, -6.0f, 6.0f, 1.0f, 400.0f);

	glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	// free the atlas texture
	videoTextureAtlasBind(-1);
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

#ifdef WIN32
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
#endif

#ifdef LINUX
int main(int argc, char *argv[]) {
#endif

	int thisTime, lastTime;
  SDL_Surface *screen;
	SDL_Event event;
	float deltaTime;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); 

	// init the screen
  screen = SDL_SetVideoMode(screenDX, screenDY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_HWSURFACE | SDL_HWACCEL);
  if (screen == NULL) {
    fprintf(stderr, "main(): SDL_SetVideoMode() failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

	SDL_WM_SetCaption("Robots of Kill SDL OpenGL", NULL);

  mainReshape(screen->w, screen->h);

	// init the subsystems
	audioInit();
	videoInit();
	inputInit();
	randomSetSeed((long long)time(NULL));
	networkInit();
	gameInit();

	// load the configuration file
	//saveLoad();

	lastTime = 0;

	while (mainExit == NO) {
		// read input
		inputRead();

		// exit?
		if (inputKeyBits & INPUT_KEY_BIT_QUIT)
			mainExit = YES;

		// calculate the deltaTime (ms)
		thisTime = SDL_GetTicks();
    deltaTime = (float)(thisTime - lastTime);
		if (deltaTime < 1.0f)
			deltaTime = 1.0f;
    lastTime = thisTime;

		while (SDL_PollEvent(&event)) {
      if (event.type == SDL_VIDEORESIZE) {
				// NOTE: this "reloads" the textures
				screenDX = event.resize.w;
				screenDY = event.resize.h;
				screen = SDL_SetVideoMode(screenDX, screenDY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_HWSURFACE | SDL_HWACCEL);
				if (screen != NULL)
					mainReshape(screen->w, screen->h);
				else {
					// SDL_SetVideoMode() failed!
				}
			}
			else if (event.type == SDL_QUIT)
				mainExit = YES;
		}

		// run one iteration
		gameIteration(deltaTime);

		// draw the screen
		gameDraw();

		// swap the draw buffers
		videoSwapBuffers();

		// wait a little
		SDL_Delay(1);
	}

  SDL_Quit();

	return 0;
}
