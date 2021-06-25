
#ifndef VIDEO_H
#define VIDEO_H

// the vertex formats
struct videoVertexV {
	float x, y, z;
};

struct videoVertexTV {
	float u, v;
	float x, y, z;
};

struct videoVertexTNV {
	float u, v;
	float nx, ny, nz;
	float x, y, z;
};

struct videoVertexNV {
	float nx, ny, nz;
	float x, y, z;
};

// the video quality levels
#define VIDEO_QUALITY_LOW  0
#define VIDEO_QUALITY_HIGH 1

// the current video quality
extern int videoQuality;

// the atlas texture structure. note that not all elements are used on every platform.
struct videoTextureAtlas {
	char fileNameTexture[256];
	unsigned char *dataTexture;
	int edgeLength;
	char fileNamePalette[256];
	unsigned char *dataPalette;
	int colors;
	int *spriteAttributes;
	int sprites;
};

// features for videoEnable() and videoDisable
#define VIDEO_FEATURE_TEXTURING 0
#define VIDEO_FEATURE_BLENDING  1

// the current atlas information
extern int videoTextureAtlasName;

// the atlas list
extern struct videoTextureAtlas *videoTextureAtlasList;
extern int videoTextureAtlasListN;

void videoInit(void);
void videoSetTextureList(struct videoTextureAtlas *atlasList, int atlasListN);
void videoClearScreen(int r, int g, int b, int a);
void videoSwapBuffers(void);
void videoTextureAtlasBind(int name);
void videoSetColor(int r, int g, int b, int a);
void videoSetQuality(int quality);

void videoDrawRectangle2D(float dX, float dY);
void videoDrawSprite(float cX, float cY, float cZ, float scaleX, float scaleY, int spriteID);
void videoGetSpriteDimensions(int spriteID, int *dX, int *dY);

void videoSetFrustum(float left, float right, float top, float bottom, float n, float f);
void videoLoadIdentity(void);
void videoTranslate3f(float tX, float tY, float tZ);
void videoScale3f(float sX, float sY, float sZ);
void videoRotate3f(float rX, float rY, float rZ);
void videoMatrixPush(void);
void videoMatrixPop(void);
void videoDrawTrianglesTNV(struct videoVertexTNV *vertices, int triangles);
void videoDrawTrianglesNV(struct videoVertexNV *vertices, int triangles);
void videoEnable(int feature);
void videoDisable(int feature);

#endif
