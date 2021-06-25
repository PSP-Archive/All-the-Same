
/*
 *
 * video.c - draw graphics
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "video.h"
#include "inflate.h"

// ---------------------------------------------------------------------------
// definitions
// ---------------------------------------------------------------------------

// display buffer, and screen size
#define BUFFER_DX 512
#define SCREEN_DX 480
#define SCREEN_DY 272

// ---------------------------------------------------------------------------
// variables
// ---------------------------------------------------------------------------

// the current video quality
int videoQuality = VIDEO_QUALITY_HIGH;

// display list
static unsigned int __attribute__((aligned(16))) displayList[262144];

// offset for memory allocation
static unsigned int staticOffset = 0;

// room for the atlas texture
static unsigned char __attribute__((aligned(16))) videoTextureAtlasData[512*512*4];

// general vectors
static volatile ScePspFVector3 videoVectorA = { 0.0f, 0.0f, 0.0f };

// unit square (plain)
struct videoVertexV __attribute__((aligned(16))) videoVerticesUnitSquarePlain[4] = {
	{ -1.0f, -1.0f, 0.0f },
	{  1.0f, -1.0f, 0.0f },
	{ -1.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f, 0.0f }
};

// the maximum number of textured vertices we can draw per frame
#define VERTICES_TEXTURED_MAX 1024

// textured vertices
static struct videoVertexTV __attribute__((aligned(16))) videoVerticesTV[VERTICES_TEXTURED_MAX];

// the textured vertex index
static int videoVertexTVNext = 0;

// the feature translation table
static int videoFeatureTranslationTable[] = {
	GU_TEXTURE_2D,
	GU_BLEND
};

// ---------------------------------------------------------------------------
// memory allocation
// ---------------------------------------------------------------------------

static unsigned int getMemorySize(unsigned int width, unsigned int height, unsigned int psm) {

	switch (psm) {
	case GU_PSM_T4:
		return (width * height) >> 1;

	case GU_PSM_T8:
		return width * height;

	case GU_PSM_5650:
	case GU_PSM_5551:
	case GU_PSM_4444:
	case GU_PSM_T16:
		return 2 * width * height;

	case GU_PSM_8888:
	case GU_PSM_T32:
		return 4 * width * height;

	default:
		return 0;
	}
}

void *getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm) {

	unsigned int memSize = getMemorySize(width, height, psm);
	void *result = (void *)staticOffset;
	staticOffset += memSize;

	return result;
}

void *getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm) {

	void *result = getStaticVramBuffer(width, height, psm);

	return (void *)(((unsigned int)result) + ((unsigned int)sceGeEdramGetAddr()));
}

// ---------------------------------------------------------------------------
// init the video subsystem
// ---------------------------------------------------------------------------

void videoInit(void) {

	// init the hardware
	void *fbp0 = getStaticVramBuffer(BUFFER_DX, SCREEN_DY, GU_PSM_8888);
	void *fbp1 = getStaticVramBuffer(BUFFER_DX, SCREEN_DY, GU_PSM_8888);
	void *zbp = getStaticVramBuffer(BUFFER_DX, SCREEN_DY, GU_PSM_8888);

	sceGuInit();

	sceGuStart(GU_DIRECT, displayList);
	sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFFER_DX);
	sceGuDispBuffer(SCREEN_DX, SCREEN_DY, fbp1, BUFFER_DX);
	sceGuDepthBuffer(zbp, BUFFER_DX);
	sceGuOffset(2048 - (SCREEN_DX/2), 2048 - (SCREEN_DY/2));
	sceGuViewport(2048, 2048, SCREEN_DX, SCREEN_DY);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, SCREEN_DX, SCREEN_DY);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	//sceGuEnable(GU_DITHER);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	// reset the atlas texture
	videoTextureAtlasName = -1;

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
// set the video quality
// ---------------------------------------------------------------------------

void videoSetQuality(int quality) {

	videoQuality = quality;
}

// ---------------------------------------------------------------------------
// clear the screen
// ---------------------------------------------------------------------------

void videoClearScreen(int r, int g, int b, int a) {

	sceGuStart(GU_DIRECT, displayList);

	// clear screen (ABGR)
	sceGuClearColor((a << 24) | (b << 16) | (g << 8) | r);

	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

// ---------------------------------------------------------------------------
// set perspective projection
// ---------------------------------------------------------------------------

void videoSetFrustum(float left, float right, float top, float bottom, float n, float f) {

	ScePspFMatrix4 m;

	m.x.x = (2.0f * n) / (right - left);
	m.y.x = 0.0f;
	m.z.x = (right + left) / (right - left);
	m.w.x = 0.0f;

	m.x.y = 0.0f;
	m.y.y = (2.0f * n) / (top - bottom);
	m.z.y = (top + bottom) / (top - bottom);
	m.w.y = 0.0f;

	m.x.z = 0.0f;
	m.y.z = 0.0f;
	m.z.z = -(f + n) / (f - n);
	m.w.z = -(2.0f * f * n) / (f - n);

	m.x.w = 0.0f;
	m.y.w = 0.0f;
	m.z.w = -1.0f;
	m.w.w = 0.0f;

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumMultMatrix(&m);

	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
}

// ---------------------------------------------------------------------------
// set the drawing color
// ---------------------------------------------------------------------------

void videoSetColor(int r, int g, int b, int a) {

	sceGuColor((a << 24) | (b << 16) | (g << 8) | r);
}

// ---------------------------------------------------------------------------
// enable a feature
// ---------------------------------------------------------------------------

void videoEnable(int feature) {

	sceGuEnable(videoFeatureTranslationTable[feature]);
}

// ---------------------------------------------------------------------------
// disable a feature
// ---------------------------------------------------------------------------

void videoDisable(int feature) {

	sceGuDisable(videoFeatureTranslationTable[feature]);
}

// ---------------------------------------------------------------------------
// draw a single colored rectangle 2D
// ---------------------------------------------------------------------------

void videoDrawRectangle2D(float dX, float dY) {

	sceGuDisable(GU_TEXTURE_2D);

	sceGumPushMatrix();

	videoVectorA.x = dX * 0.5f;
	videoVectorA.y = dY * 0.5f;
	videoVectorA.z = 1.0f;
	sceGumScale((ScePspFVector3 *)&videoVectorA);

	// draw the rectangle
	sceGumDrawArray(GU_TRIANGLE_STRIP, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, NULL, videoVerticesUnitSquarePlain);

	sceGumPopMatrix();

	sceGuEnable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
// draw a textured rectangle
// ---------------------------------------------------------------------------

void videoDrawSprite(float cX, float cY, float cZ, float scaleX, float scaleY, int spriteID) {

	float dXAtlas, dX, dY, sX, sY, eX, eY;
	int *sprites, i;

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

	i = videoVertexTVNext;
	if (i > VERTICES_TEXTURED_MAX - 4)
		return;

	videoVerticesTV[i].u = sX;
	videoVerticesTV[i].v = sY;
	videoVerticesTV[i].x = -1.0f;
	videoVerticesTV[i].y = -1.0f;
	videoVerticesTV[i].z =  0.0f;
	i++;

	videoVerticesTV[i].u = eX;
	videoVerticesTV[i].v = sY;
	videoVerticesTV[i].x =  1.0f;
	videoVerticesTV[i].y = -1.0f;
	videoVerticesTV[i].z =  0.0f;
	i++;

	videoVerticesTV[i].u = sX;
	videoVerticesTV[i].v = eY;
	videoVerticesTV[i].x = -1.0f;
	videoVerticesTV[i].y =  1.0f;
	videoVerticesTV[i].z =  0.0f;
	i++;

	videoVerticesTV[i].u = eX;
	videoVerticesTV[i].v = eY;
	videoVerticesTV[i].x =  1.0f;
	videoVerticesTV[i].y =  1.0f;
	videoVerticesTV[i].z =  0.0f;
	i++;

	videoVertexTVNext = i;

	// translate and scale
	sceGumPushMatrix();

	videoVectorA.x = cX;
	videoVectorA.y = cY;
	videoVectorA.z = cZ;
	sceGumTranslate((ScePspFVector3 *)&videoVectorA);

	videoVectorA.x = dX * 0.5f * scaleX;
	videoVectorA.y = dY * 0.5f * scaleY;
	videoVectorA.z = 1.0f;
	sceGumScale((ScePspFVector3 *)&videoVectorA);

	// TODO: can we avoid this?
	// flush the data cache
	sceKernelDcacheWritebackAll();

	// draw the rectangle
	sceGumDrawArray(GU_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, NULL, &videoVerticesTV[i-4]);

	sceGumPopMatrix();
}

// ---------------------------------------------------------------------------
// load an identity matrix
// ---------------------------------------------------------------------------

void videoLoadIdentity(void) {

	sceGumLoadIdentity();
}

// ---------------------------------------------------------------------------
// push matrix
// ---------------------------------------------------------------------------

void videoMatrixPush(void) {

	sceGumPushMatrix();
}

// ---------------------------------------------------------------------------
// pop matrix
// ---------------------------------------------------------------------------

void videoMatrixPop(void) {

	sceGumPopMatrix();
}

// ---------------------------------------------------------------------------
// translate
// ---------------------------------------------------------------------------

void videoTranslate3f(float tX, float tY, float tZ) {

	videoVectorA.x = tX;
	videoVectorA.y = tY;
	videoVectorA.z = tZ;
	sceGumTranslate((ScePspFVector3 *)&videoVectorA);
}

// ---------------------------------------------------------------------------
// scale
// ---------------------------------------------------------------------------

void videoScale3f(float sX, float sY, float sZ) {

	videoVectorA.x = sX;
	videoVectorA.y = sY;
	videoVectorA.z = sZ;
	sceGumScale((ScePspFVector3 *)&videoVectorA);
}

// ---------------------------------------------------------------------------
// rotate (degrees)
// ---------------------------------------------------------------------------

void videoRotate3f(float rX, float rY, float rZ) {

	videoVectorA.x = (rX * GU_PI) / 180.0f;
	videoVectorA.y = (rY * GU_PI) / 180.0f;
	videoVectorA.z = (rZ * GU_PI) / 180.0f;
	sceGumRotateXYZ((ScePspFVector3 *)&videoVectorA);
}

// ---------------------------------------------------------------------------
// draw triangles (TNV)
// ---------------------------------------------------------------------------

void videoDrawTrianglesTNV(struct videoVertexTNV *vertices, int triangles) {

	// draw the mesh
	sceGumDrawArray(GU_TRIANGLES, GU_NORMAL_32BITF | GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, triangles*3, NULL, (ScePspFVector3 *)vertices);
}

// ---------------------------------------------------------------------------
// draw triangles (NV)
// ---------------------------------------------------------------------------

void videoDrawTrianglesNV(struct videoVertexNV *vertices, int triangles) {

	// draw the mesh
	sceGumDrawArray(GU_TRIANGLES, GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, triangles*3, NULL, (ScePspFVector3 *)vertices);
}

// ---------------------------------------------------------------------------
// bind an atlas texture
// ---------------------------------------------------------------------------

void videoTextureAtlasBind(int name) {

	int dX;

	if (videoTextureAtlasName == name)
		return;

	// uncompress the texture
	inflate(videoTextureAtlasList[name].dataTexture, videoTextureAtlasData);

	// remember it
	videoTextureAtlasName = name;

	// setup texture
	dX = videoTextureAtlasList[name].edgeLength;

	// every time we specify the texture, it'll be copied to the texture slot

	// flush the data cache
	sceKernelDcacheWritebackAll();

	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, dX, dX, dX, videoTextureAtlasData);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0xffff00);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexOffset(0.0f, 0.0f);

	sceGuAmbientColor(0xffffffff);
}

// ---------------------------------------------------------------------------
// swap the draw buffers
// ---------------------------------------------------------------------------

void videoSwapBuffers(void) {

	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();

	videoVertexTVNext = 0;
}
