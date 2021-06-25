
/*
 *
 * file.c - File I/O
 *
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely used stuff from Windows headers

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "file.h"

// -----------------------------------------------------------------------
// load a file
// -----------------------------------------------------------------------

int fileLoad(char *name, unsigned char *data) {

	FILE *f;
	int i;

	f = fopen(name, "rb");
	if (f == NULL)
		return -1;

	fseek(f, 0, SEEK_END);
	i = ftell(f);
	fseek(f, 0, SEEK_SET);

	fread(data, 1, i, f);
	fclose(f);

	return i;
}

// -----------------------------------------------------------------------
// save a file
// -----------------------------------------------------------------------

int fileSave(char *name, unsigned char *data, int length) {

	FILE *f;

	f = fopen(name, "wb");
	if (f == NULL)
		return -1;

	fwrite(data, 1, length, f);
	fclose(f);

	return length;
}
