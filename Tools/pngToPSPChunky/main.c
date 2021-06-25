
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "defines.h"
#include "png.h"


static void _image_squash(unsigned char *data, unsigned char *out, int dX, int dY, int type, int target) {

  int colors, cR, cG, cB, cA, i;

  if (type == GL_RGB)
    colors = 3;
  else
    colors = 4;

	fprintf(stderr, "_image_squash(): The image has %d components.\n", colors);

  for (i = 0; i < dX*dY; i++) {
    cR = data[i*colors + 0];
    cG = data[i*colors + 1];
    cB = data[i*colors + 2];

		if (colors == 3)
			cA = 0xFF;
		else
			cA = data[i*colors + 3];

		out[i*4 + 0] = cR;
		out[i*4 + 1] = cG;
		out[i*4 + 2] = cB;
		out[i*4 + 3] = cA;
  }
}


int main(int argc, char *argv[]) {

  int dX, dY, type, i, target;
  unsigned char *d, *o;
  FILE *f;

  if (argc != 4 || (strcmp(argv[1], "-1") != 0 && strcmp(argv[1], "-2") != 0 && strcmp(argv[1], "-3") != 0 && strcmp(argv[1], "-4") != 0)) {
    fprintf(stderr, "USAGE: %s -{1234} <PNG FILE> <OUT NAME>\n", argv[0]);
    fprintf(stderr, "COMMANDS:\n");
    fprintf(stderr, " 1  GU_PSM_5650\n");
    fprintf(stderr, " 2  GU_PSM_5551\n");
    fprintf(stderr, " 3  GU_PSM_4444\n");
    fprintf(stderr, " 4  GU_PSM_8888\n");
    return 1;
  }

  if (strcmp(argv[1], "-1") == 0)
		target = GU_PSM_5650;
  else if (strcmp(argv[1], "-2") == 0)
		target = GU_PSM_5551;
  else if (strcmp(argv[1], "-3") == 0)
		target = GU_PSM_4444;
  else if (strcmp(argv[1], "-4") == 0)
		target = GU_PSM_8888;

  if (png_load(argv[2], &dX, &dY, &type, &d) == FAILED)
    return 1;

	o = malloc(dX*dY*4);
	if (o == NULL) {
		fprintf(stderr, "main(): Out of memory error.\n");
		return 1;
	}

  /* squash the image -> 16bit */
  _image_squash(d, o, dX, dY, type, target);

  /* write the image data */
  f = fopen(argv[3], "wb");
  if (f == NULL) {
    fprintf(stderr, "main(): Could not open file \"%s\" for writing.\n", argv[3]);
    return 1;
  }

	/* write DX and DY */
	/*
	_write_u16(f, dX);
	_write_u16(f, dY);
  */

	if (target == GU_PSM_8888) {
    for (i = 0; i < dX*dY*4; i += 4) {
      fprintf(f, "%c", o[i+0]);
      fprintf(f, "%c", o[i+1]);
      fprintf(f, "%c", o[i+2]);
      fprintf(f, "%c", o[i+3]);
		}
	}
	else if (target == GU_PSM_4444) {
    for (i = 0; i < dX*dY*4; i += 4) {
      fprintf(f, "%c", ((o[i+0] >> 4) << 4) | (o[i+1] >> 4));
      fprintf(f, "%c", ((o[i+2] >> 4) << 4) | (o[i+3] >> 4));
		}
	}

  fclose(f);
  free(d);
	free(o);

  return 0;
}
