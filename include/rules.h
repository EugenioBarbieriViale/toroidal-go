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

typedef struct {
  int fc;
  int color;

  int board[BOARD_SIZE];
  int all_neighbors[BOARD_SIZE][4];

  Stack reached;
  Stack chain;

  int captured_stone_count;
} BoardState;

BoardState *init_bs(const int);

int move(BoardState *bs, char *, int *);
void score(int *, const int[][4], int *, int *);

#endif
