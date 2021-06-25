
#ifndef INFLATE_H
#define INFLATE_H

struct node {
	struct node *left;
	struct node *right;
	int literal;
};

void inflate(unsigned char *data, unsigned char *output);

#endif
