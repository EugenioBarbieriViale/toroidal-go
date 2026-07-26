#ifndef RULES_H
#define RULES_H

#include "stack.h"

#define N_LINES 19
#define BOARD_SIZE (N_LINES * N_LINES)

#define BLACK 1
#define WHITE 2
#define EMPTY 0
#define UNDEF 3

typedef struct {
  int row;
  int col;
} Coord2;

void get_neighbors(int, int *);
int move(int, int, int *, const int[][4], Stack *, Stack *, char *, int *);
void score(int *, const int[][4], int *, int *);

#endif
