
/*
 * File:      videoCommon.c
 * Purpose:   The common video routines
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely-used stuff from windows headers

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "video.h"

// ---------------------------------------------------------------------------
// variables
// ---------------------------------------------------------------------------

// the current atlas information
int videoTextureAtlasName;

// the atlas list
struct videoTextureAtlas *videoTextureAtlasList;
int videoTextureAtlasListN;

// ---------------------------------------------------------------------------
// get sprite DX and DY
// ---------------------------------------------------------------------------

void videoGetSpriteDimensions(int spriteID, int *dX, int *dY) {

	int *sprites, sX, eX, sY, eY;

	sprites = videoTextureAtlasList[videoTextureAtlasName].spriteAttributes;

	spriteID <<= 2;

	// TODO: precalculate these
	sX = sprites[spriteID++];
	sY = sprites[spriteID++];
	eX = sprites[spriteID++];
	eY = sprites[spriteID];

	if (dX != NULL)
		*dX = eX - sX;
	if (dY != NULL)
		*dY = eY - sY;
}
