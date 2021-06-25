
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"


void _fileSeek(FILE *f, char *str) {

	char tmp[256];

	while (1) {
		fscanf(f, "%s", tmp);
		if (strcmp(tmp, str) == 0)
			return;
	}
}


struct vertex {
	float x, y, z;
};

struct vertex vertices[64*1024];

struct face {
	int v1, v2, v3;
	int uv1, uv2, uv3;
	float nX, nY, nZ;
};

struct face faces[64*1024];

struct uv {
	float u, v;
};

struct uv uvs[64*1024];


int main(int argc, char *argv[]) {

	int verticesN, facesN, uvN, i, mode;
  FILE *f;

  if (argc != 3) {
    fprintf(stderr, "USAGE: %s -{n/t} <ASCII DAE FILE>\n", argv[0]);
		fprintf(stderr, "COMMANDS:\n");
		fprintf(stderr, " n Normal conversion\n");
		fprintf(stderr, " t Strip UVs\n");
    return 1;
  }

	if (strcmp(argv[1], "-n") == 0)
		mode = MODE_NORMAL;
	else if (strcmp(argv[1], "-t") == 0)
		mode = MODE_STRIP_UVS;
	else
		return 1;

	f = fopen(argv[2], "rb");
	if (f == NULL) {
		fprintf(stderr, "main(): Couldn't open file %s for reading.\n", argv[2]);
		return 1;
	}

	_fileSeek(f, "*MESH_NUMVERTEX");
	fscanf(f, "%d", &verticesN);
	_fileSeek(f, "*MESH_NUMFACES");
	fscanf(f, "%d", &facesN);

	fprintf(stderr, "main(): The mesh has %d vertices and %d faces.\n", verticesN, facesN);

	/* read vertices */
	_fileSeek(f, "*MESH_VERTEX_LIST");
	fscanf(f, "%*s");

	for (i = 0; i < verticesN; i++) {
		fscanf(f, "%*s %*d");
		fscanf(f, "%f %f %f", &vertices[i].x, &vertices[i].y, &vertices[i].z);
		/*
		fprintf(stderr, "x = %f\n", vertices[i].x);
		*/
	}

	/* read faces */
	_fileSeek(f, "*MESH_FACE_LIST");
	fscanf(f, "%*s");

	for (i = 0; i < facesN; i++) {
		_fileSeek(f, "*MESH_FACE");
		fscanf(f, "%*s %*s");
		fscanf(f, "%d", &faces[i].v1);
		fscanf(f, "%*s");
		fscanf(f, "%d", &faces[i].v2);
		fscanf(f, "%*s");
		fscanf(f, "%d", &faces[i].v3);
	}

	/* read uvs */
	_fileSeek(f, "*MESH_NUMTVERTEX");
	fscanf(f, "%d", &uvN);

	for (i = 0; i < uvN; i++) {
		_fileSeek(f, "*MESH_TVERT");
		fscanf(f, "%*d");
		fscanf(f, "%f %f", &uvs[i].u, &uvs[i].v);
	}

	/* read the textured faces */
	_fileSeek(f, "*MESH_NUMTVFACES");
	fscanf(f, "%d", &i);

	if (i != facesN) {
		fprintf(stderr, "main(): Faces = %d, textured faces = %d\n", facesN, i);
		return -1;
	}

	for (i = 0; i < facesN; i++) {
		_fileSeek(f, "*MESH_TFACE");
		fscanf(f, "%*d");
		fscanf(f, "%d", &faces[i].uv1);
		fscanf(f, "%d", &faces[i].uv2);
		fscanf(f, "%d", &faces[i].uv3);
	}

	/* read face normals */
	for (i = 0; i < facesN; i++) {
		_fileSeek(f, "*MESH_FACENORMAL");
		fscanf(f, "%*d");
		fscanf(f, "%f", &faces[i].nX);
		fscanf(f, "%f", &faces[i].nY);
		fscanf(f, "%f", &faces[i].nZ);
	}

	/* output the mesh in C */
	if (mode == MODE_NORMAL)
		fprintf(stdout, "struct videoVertexTNV mesh[3*%d] = {\n", facesN);
	else
		fprintf(stdout, "struct videoVertexNV mesh[3*%d] = {\n", facesN);

	if (mode == MODE_NORMAL) {
		for (i = 0; i < facesN; i++) {
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f, %f, %f },\n", uvs[faces[i].uv1].u, 1.0f - uvs[faces[i].uv1].v, faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v1].x, vertices[faces[i].v1].y, vertices[faces[i].v1].z);
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f, %f, %f },\n", uvs[faces[i].uv2].u, 1.0f - uvs[faces[i].uv2].v, faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v2].x, vertices[faces[i].v2].y, vertices[faces[i].v2].z);
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f, %f, %f },\n", uvs[faces[i].uv3].u, 1.0f - uvs[faces[i].uv3].v, faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v3].x, vertices[faces[i].v3].y, vertices[faces[i].v3].z);
		}
	}
	else {
		for (i = 0; i < facesN; i++) {
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f },\n", faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v1].x, vertices[faces[i].v1].y, vertices[faces[i].v1].z);
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f },\n", faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v2].x, vertices[faces[i].v2].y, vertices[faces[i].v2].z);
			fprintf(stdout, "  { %f, %f, %f, %f, %f, %f },\n", faces[i].nX, faces[i].nY, faces[i].nZ, vertices[faces[i].v3].x, vertices[faces[i].v3].y, vertices[faces[i].v3].z);
		}
	}

	fprintf(stdout, "};\n");

  return 0;
}
