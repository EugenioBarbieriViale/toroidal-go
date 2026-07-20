#include "rules.h"
#include <stdio.h>
#include <stdlib.h>

#define BLACK_CHAR 'O'
#define WHITE_CHAR 'X'
#define EMPTY_CHAR '.'

void show_board(int *);
void debug_rules(void);

int main() {
  // server code here (?)
  return 0;
}

void debug_rules(void) {
  int board[N_INTERS];
  for (int i = 0; i < N_INTERS; i++) {
    board[i] = 0;
  }

  int all_neighbors[N_INTERS][4];
  for (int fc = 0; fc < N_INTERS; fc++) {
    get_neighbors(fc, all_neighbors[fc]);
  }

  Stack chain;
  construct(&chain);

  Stack reached;
  construct(&reached);

  // Claude generated test
  // --- Closed ring test: hollow 5x5 Black square enclosing 3x3 territory ---

  // Top edge of the ring
  move((Coord2){5, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){5, 6}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){5, 7}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){5, 8}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){5, 9}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // Right edge of the ring
  move((Coord2){6, 9}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){7, 9}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){8, 9}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){9, 9}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // Bottom edge of the ring
  move((Coord2){9, 8}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){9, 7}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){9, 6}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){9, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // Left edge of the ring (closing the loop)
  move((Coord2){8, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){7, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);
  move((Coord2){6, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){18, 5}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  int black_score, white_score = 0;
  score(board, all_neighbors, &black_score, &white_score);

  printf("BLACK SCORE: %d, WHITE SCORE: %d\n", black_score, white_score);

  free(chain.buffer);
  free(reached.buffer);
}

void show_board(int board[]) {
  printf("*---------------------------------------------------------*\n");
  for (int i = 0; i < N_LINES; i++) {
    printf("|");
    for (int j = 0; j < N_LINES; j++) {
      int fc = flatten((Coord2){i, j});
      int c;
      switch (board[fc]) {
      default:
        c = '@';
        break;
      case 0:
        c = EMPTY_CHAR;
        break;
      case 1:
        c = BLACK_CHAR;
        break;
      case 2:
        c = WHITE_CHAR;
        break;
      }
      printf(" %c ", c);
    }
    printf("|\n");
  }
  printf("*---------------------------------------------------------*\n");
}
