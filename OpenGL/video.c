
/*
 * File:      video.c
 * Purpose:   The video subsystem
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely-used stuff from windows headers

#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
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
#include "video.h"
#include "inflate.h"

// ---------------------------------------------------------------------------
// variables
// ---------------------------------------------------------------------------

// room for the atlas texture
static unsigned char videoTextureAtlasData[512*512*4];

// the current video quality
int videoQuality = VIDEO_QUALITY_HIGH;

// the feature translation table
static int videoFeatureTranslationTable[] = {
	GL_TEXTURE_2D,
	GL_BLEND
};

// ---------------------------------------------------------------------------
// init the video subsystem
// ---------------------------------------------------------------------------

void videoInit(void) {

	unsigned int texID;

	// reset the atlas texture
	videoTextureAtlasName = -1;

	// generate one texture ID for the atlas texture
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	// set the default quality level
	videoSetQuality(VIDEO_QUALITY_HIGH);
}

// ---------------------------------------------------------------------------
// specify the atlas textures and sprites
// ---------------------------------------------------------------------------

void videoSetTextureList(struct videoTextureAtlas *atlasList, int atlasListN) {

	// reset the atlas texture
	videoTextureAtlasName = -1;

	// store the atlas list
	videoTextureAtlasList = atlasList;
	videoTextureAtlasListN = atlasListN;
}

// ---------------------------------------------------------------------------
// clear the screen
// ---------------------------------------------------------------------------

void videoClearScreen(int r, int g, int b, int a) {

  glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ---------------------------------------------------------------------------
// set perspective projection
// ---------------------------------------------------------------------------

void videoSetFrustum(float left, float right, float top, float bottom, float n, float f) {

	float m[4*4];

	m[4*0+0] = (2.0f * n) / (right - left);
	m[4*1+0] = 0.0f;
	m[4*2+0] = (right + left) / (right - left);
	m[4*3+0] = 0.0f;

	m[4*0+1] = 0.0f;
	m[4*1+1] = (2.0f * n) / (top - bottom);
	m[4*2+1] = (top + bottom) / (top - bottom);
	m[4*3+1] = 0.0f;

	m[4*0+2] = 0.0f;
	m[4*1+2] = 0.0f;
	m[4*2+2] = -(f + n) / (f - n);
	m[4*3+2] = -(2.0f * f * n) / (f - n);

	m[4*0+3] = 0.0f;
	m[4*1+3] = 0.0f;
	m[4*2+3] = -1.0f;
	m[4*3+3] = 0.0f;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
	glMultMatrixf(m);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

// ---------------------------------------------------------------------------
// bind the texture
// ---------------------------------------------------------------------------

void videoTextureAtlasBind(int name) {

	int dX;

	if (videoTextureAtlasName == name)
		return;

	if (name >= 0) {
		unsigned char *data;
		FILE *f;
		int size;

		f = fopen(videoTextureAtlasList[name].fileNameTexture, "rb");
		if (f == NULL) {
			fprintf(stderr, "videoTextureAtlasBind(): Couldn't open file \"%s\" for reading.\n", videoTextureAtlasList[name].fileNameTexture);
			return;
		}

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);

		data = malloc(size);
		if (data == NULL) {
			fprintf(stderr, "videoTextureAtlasBind(): Out of memory error.\n");
			fclose(f);
			return;
		}

		fread(data, 1, size, f);
		fclose(f);

		// uncompress the texture
		inflate(data, videoTextureAtlasData);
		free(data);
	}

	// remember it
	videoTextureAtlasName = name;

	if (name < 0)
		return;

	// setup texture
	dX = videoTextureAtlasList[name].edgeLength;

	// bind it
	if (videoQuality == VIDEO_QUALITY_LOW) {
		// no mipmaps
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dX, dX, 0, GL_RGBA, GL_UNSIGNED_BYTE, videoTextureAtlasData);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	else {
		// mipmaps
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, dX, dX, GL_RGBA, GL_UNSIGNED_BYTE,  videoTextureAtlasData);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

// ---------------------------------------------------------------------------
// set the video quality
// ---------------------------------------------------------------------------

void videoSetQuality(int quality) {

	videoQuality = quality;

	if (quality == VIDEO_QUALITY_LOW) {
		glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	}
	else {
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}
}

// ---------------------------------------------------------------------------
// set the drawing color
// ---------------------------------------------------------------------------

void videoSetColor(int r, int g, int b, int a) {

	glColor4ub(r, g, b, a);
}

// ---------------------------------------------------------------------------
// enable a feature
// ---------------------------------------------------------------------------

void videoEnable(int feature) {

	glEnable(videoFeatureTranslationTable[feature]);
}

// ---------------------------------------------------------------------------
// disable a feature
// ---------------------------------------------------------------------------

void videoDisable(int feature) {

	glDisable(videoFeatureTranslationTable[feature]);
}

// ---------------------------------------------------------------------------
// load an identity matrix
// ---------------------------------------------------------------------------

void videoLoadIdentity(void) {

	glLoadIdentity();
}

// ---------------------------------------------------------------------------
// push matrix
// ---------------------------------------------------------------------------

void videoMatrixPush(void) {

	glPushMatrix();
}

// ---------------------------------------------------------------------------
// pop matrix
// ---------------------------------------------------------------------------

void videoMatrixPop(void) {

	glPopMatrix();
}

// ---------------------------------------------------------------------------
// translate
// ---------------------------------------------------------------------------

void videoTranslate3f(float tX, float tY, float tZ) {

	glTranslatef(tX, tY, tZ);
}

// ---------------------------------------------------------------------------
// scale
// ---------------------------------------------------------------------------

void videoScale3f(float sX, float sY, float sZ) {

	glScalef(sX, sY, sZ);
}

// ---------------------------------------------------------------------------
// rotate (degrees)
// ---------------------------------------------------------------------------

void videoRotate3f(float rX, float rY, float rZ) {

	if (rZ != 0.0f)
		glRotatef(rZ, 0.0f, 0.0f, 1.0f);

	if (rY != 0.0f)
		glRotatef(rY, 0.0f, 1.0f, 0.0f);

	if (rX != 0.0f)
		glRotatef(rX, 1.0f, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// draw triangles (TNV)
// ---------------------------------------------------------------------------

void videoDrawTrianglesTNV(struct videoVertexTNV *vertices, int triangles) {

	struct videoVertexTNV *v;
	int i;

	// TODO: optimize

	glBegin(GL_TRIANGLES);
	for (i = 0; i < triangles*3; i++) {
		v = &vertices[i];
		glTexCoord2f(v->u, v->v);
		glNormal3f(v->nx, v->ny, v->nz);
		glVertex3f(v->x, v->y, v->z);
	}
	glEnd();
}

// ---------------------------------------------------------------------------
// draw triangles (NV)
// ---------------------------------------------------------------------------

void videoDrawTrianglesNV(struct videoVertexNV *vertices, int triangles) {

	struct videoVertexNV *v;
	int i;

	// TODO: optimize
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLES);
	for (i = 0; i < triangles*3; i++) {
		v = &vertices[i];
		glNormal3f(v->nx, v->ny, v->nz);
		glVertex3f(v->x, v->y, v->z);
	}
	glEnd();

	glEnable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
// draw a single colored rectangle
// ---------------------------------------------------------------------------

void videoDrawRectangle2D(float dX, float dY) {

	glDisable(GL_TEXTURE_2D);

	glPushMatrix();

	glScalef(dX * 0.5f, dY * 0.5f, 1.0f);

	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f( 1.0f, -1.0f, 0.0f);
	glVertex3f(-1.0f,  1.0f, 0.0f);
	glVertex3f( 1.0f,  1.0f, 0.0f);
	glEnd();

	glPopMatrix();

	glEnable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
// draw a sprite
// ---------------------------------------------------------------------------

void videoDrawSprite(float cX, float cY, float cZ, float scaleX, float scaleY, int spriteID) {

	float dXAtlas, dX, dY, sX, sY, eX, eY;
	int *sprites;

	dXAtlas = 1.0f / videoTextureAtlasList[videoTextureAtlasName].edgeLength;
	sprites = videoTextureAtlasList[videoTextureAtlasName].spriteAttributes;

	spriteID <<= 2;

	// TODO: precalculate these
	sX = sprites[spriteID++];
	sY = sprites[spriteID++];
	eX = sprites[spriteID++];
	eY = sprites[spriteID];

	dX = eX - sX;
	dY = eY - sY;

	sX *= dXAtlas;
	sY *= dXAtlas;
	eX *= dXAtlas;
	eY *= dXAtlas;

	glPushMatrix();

	glTranslatef(cX, cY, cZ);
	glScalef(dX * 0.5f * scaleX, dY * 0.5f * scaleY, 1.0f);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(sX, sY);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glTexCoord2f(eX, sY);
	glVertex3f( 1.0f, -1.0f, 0.0f);
	glTexCoord2f(sX, eY);
	glVertex3f(-1.0f,  1.0f, 0.0f);
	glTexCoord2f(eX, eY);
	glVertex3f( 1.0f,  1.0f, 0.0f);
	glEnd();

	glPopMatrix();
}

// ---------------------------------------------------------------------------
// swap the draw buffers
// ---------------------------------------------------------------------------

void videoSwapBuffers(void) {

	SDL_GL_SwapBuffers();
}
