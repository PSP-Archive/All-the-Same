
/*
 *
 * game.c - The game main loop
 *
 */

#include <stdio.h>

#include "common.h"
#include "random.h"
#include "game.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "network.h"
#include "sprites.h"
#include "system.h"
#include "sfx.h"
#include "bgm.h"

// -----------------------------------------------------------------------
// variables
// -----------------------------------------------------------------------

// the game time
float gameTime = 0.0f;

// the run time
float runTime = 0.0f;

// the key bits during previous iteration
int gameKeyBitsOld = 0;

// the face zoomer
float faceZoom = 6.0f;

// -----------------------------------------------------------------------
// variables (networking)
// -----------------------------------------------------------------------

// the send timer
float gameNetworkSendTimer = 0.0f;

// the server connection status
int gameNetworkServerConnectionStatus = NETWORK_SERVER_NO_CONNECTION;

// my ID in the game
int gameNetworkMyID;

// -----------------------------------------------------------------------
// resources (COMMON)
// -----------------------------------------------------------------------

// the menu sprites
static int spritesMenu[] = {
	1, 1, 118, 152 // SPRITE_FACE
};

// -----------------------------------------------------------------------
// resources (PSP)
// -----------------------------------------------------------------------

#ifdef PSP

// atlas textures
extern unsigned char atlasMenu_start[];

// the atlases
static struct videoTextureAtlas videoTextureAtlases[] = {
	// MENU
	{
		"atlasMenu.dtt",
		atlasMenu_start,
		512,
		"atlasMenu.pal",
		NULL,
		256,
		spritesMenu,
		1
	}
};

extern unsigned char sfxMenuClick_start[];

// the SFXs
static struct sfx sfxs[] = {
	{
		NULL,
		sfxMenuClick_start,
		13824/2
	}
};

// the BGMs
static char *bgms[] = {
	"bgmMenu.mp3"
};

#endif

// -----------------------------------------------------------------------
// resources (!PSP)
// -----------------------------------------------------------------------

#ifndef PSP

// the texture atlases
static struct videoTextureAtlas videoTextureAtlases[] = {
	// MENU
	{
		"gfx/atlasMenu.dtt",
		NULL,
		512,
		"gfx/atlasMenu.pal",
		NULL,
		256,
		spritesMenu,
		1
	}
};

// the SFXs
static struct sfx sfxs[] = {
	{
		"sfx/sfxMenuClick.wav",
		NULL,
		0
	}
};

// the BGMs
static char *bgms[] = {
	"bgm/bgmMenu.ogg"
};

#endif

// -----------------------------------------------------------------------
// init the game
// -----------------------------------------------------------------------

void gameInit(void) {

	// overclock the systems that support overclocking (i.e., PSP)
	systemOverclock(YES);

	// set the BGM, SFX and texture lists
	audioSetBGMList(bgms, 1);
	audioSetSFXList(sfxs, 1);
	videoSetTextureList(videoTextureAtlases, 1);

	// start the BGM
	audioPlayBGM(BGM_MENU);

	gameTime = 0.0f;
}

// -----------------------------------------------------------------------
// game iteration
// -----------------------------------------------------------------------

void gameIteration(float deltaTime) {

	int keyBits;

	if (deltaTime < 0.01f)
		deltaTime = 0.01f;

	// advance the run time
	runTime += deltaTime;

	// advance main timer
	gameTime += deltaTime;

	// read the key bits
	keyBits = inputKeyBits;




	// TODO: add your game here

	// press A to play the SFX
	if ((keyBits & INPUT_KEY_BIT_A) != 0 && (gameKeyBitsOld & INPUT_KEY_BIT_A) == 0)
		audioPlaySFX(SFX_MENU_CLICK);

	// zoom the face with UP and DOWN
	if ((keyBits & INPUT_KEY_BIT_UP) != 0)
		faceZoom += deltaTime * 0.02f;
	if ((keyBits & INPUT_KEY_BIT_DOWN) != 0)
		faceZoom -= deltaTime * 0.02f;
	if (faceZoom < 1.0f)
		faceZoom = 1.0f;



	// remember the keys
	gameKeyBitsOld = keyBits;

	// network (doesn't work on PSP)

	/*
	// read packets from the network
	networkRead();

	// send our status update to the server, every 100ms
	gameNetworkServerSendUpdate(deltaTime, 100.0f);
	*/
}

// -----------------------------------------------------------------------
// draw the game
// -----------------------------------------------------------------------

void gameDraw(void) {

	// clear the screen
	videoClearScreen(16, 0, 64, 255);

	// set the projection
	videoSetFrustum(-16.0f, 16.0f, -10.0f, 10.0f, 0.1f, 3000.0f);

	// bind the texture
	videoTextureAtlasBind(VIDEO_TEXTURE_ATLAS_MENU);

	// draw the face
	videoEnable(VIDEO_FEATURE_BLENDING);

	videoSetColor(255, 255, 255, 255);
	videoLoadIdentity();
	videoTranslate3f(0.0f, 0.0f, -11.7f);
	videoRotate3f(0.0f, 0.0f, runTime * 0.03f);
	videoDrawSprite(0.0f, 0.0f, 0.0f, faceZoom, faceZoom, VIDEO_SPRITE_MENU_FACE);
}

// -----------------------------------------------------------------------
// parse a packet that we got from the server
// -----------------------------------------------------------------------

void gameNetworkParsePacket(unsigned char *buffer, int size, int packetNumber) {

	int i = 2, j;

	// again, skip the first two bytes as the packet size is placed there

	switch (buffer[i++]) {
	case 'D':
		// DEATH OF A CLIENT

		// client ID
		j = buffer[i++];

		break;

	case 'B':
		// BEGIN THE MATCH

		gameNetworkServerConnectionStatus = NETWORK_SERVER_GAME;

		break;

	case 'p':
		// PING

		// ping back
		buffer[2] = 'p';
		networkServerSend(buffer, 3);

		break;

	case 'W':
		// WELCOME

		// get my ID
		gameNetworkMyID = buffer[i++];

		// update the server status
		gameNetworkServerConnectionStatus = NETWORK_SERVER_CONNECTION;

		break;
	}
}

// -----------------------------------------------------------------------
// send start command to server
// -----------------------------------------------------------------------

void gameNetworkServerSendStart(void) {

	networkBuffer[2] = 'B';
  networkServerSend(networkBuffer, 3);
}

// -----------------------------------------------------------------------
// send join command to server
// -----------------------------------------------------------------------

void gameNetworkServerSendJoin(void) {

	networkBuffer[2] = 'J';
  networkServerSend(networkBuffer, 3);
}

// -----------------------------------------------------------------------
// create a status update packet for the server
// -----------------------------------------------------------------------

int  gameNetworkCreateUpdatePacket(unsigned char *buffer) {

	int i = 2;

	// note that we must leave the two first bytes untouched as there the
	// network subsystem will store the packet size

	// fill the buffer with update information

	// return the packet size
	return i;
}

// -----------------------------------------------------------------------
// send data to the server at fixed intervals (do this when the game
// is running)
// -----------------------------------------------------------------------

void gameNetworkServerSendUpdate(float deltaTime, float updateInterval) {

	int i;

	if (gameNetworkServerConnectionStatus != NETWORK_SERVER_GAME)
		return;

	gameNetworkSendTimer += deltaTime;

	if (gameNetworkSendTimer < updateInterval)
		return;

	gameNetworkSendTimer -= updateInterval;

	// create an update packet
	i = gameNetworkCreateUpdatePacket(networkBuffer);

	// ... and send it to the server
	networkServerSend(networkBuffer, i);
}
