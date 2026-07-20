#ifndef RULES_H
#define RULES_H

#include "stack.h"

#define N_LINES 19
#define N_INTERS (N_LINES * N_LINES)

#define BLACK 1
#define WHITE 2
#define EMPTY 0

typedef struct {
  int row;
  int col;
} Coord2;

// static inline int flatten(Coord2);
// static inline Coord2 unflatten(int);
// static inline void clone_arr(int *, int *, int);

static inline int flatten(Coord2 c) { return N_LINES * c.row + c.col; }
static inline Coord2 unflatten(int fc) {
  return (Coord2){fc / N_LINES, fc % N_LINES};
}

static inline void clone_arr(int from[], int to[], int len) {
  for (int i = 0; i < len; i++)
    to[i] = from[i];
}

void get_neighbors(int, int *);
int get_idx(int, int, int *);

void flood_fill(int, int *, int[][4], Stack *, Stack *);
int unsigned get_liberties(int, int *, int[][4], Stack *, Stack *);

int maybe_capture(unsigned int, int *, Stack *);
void move(Coord2, int *, int[][4], Stack *, Stack *, int);

void score(int *, int[][4], int *, int *);

#endif
