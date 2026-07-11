#include <raylib.h>
#include <stdio.h>

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)

typedef struct {
  int row;
  int col;
} Coord2;

void init_board(char grid[][N_LINES]) {
  for (int i = 0; i < N_LINES; i++) {
    for (int j = 0; j < N_LINES; j++) {
      grid[i][j] = '.';
    }
  }
}

void show_board(char grid[][N_LINES]) {
  printf("*------------------------------------------------------*\n");
  for (int i = 0; i < N_LINES; i++) {
    printf("|");
    for (int j = 0; j < N_LINES; j++) {
      printf(" %c ", grid[i][j]);
    }
    printf("|\n");
  }
  printf("*------------------------------------------------------*\n");
}

void move(char grid[][N_LINES], Coord2 coord, char c) {
  grid[coord.row][coord.col] = c;
}

int main() {
  char grid[N_LINES][N_LINES];
  init_board(grid);
  move(grid, (Coord2){5, 5}, 'O');
  move(grid, (Coord2){4, 5}, 'X');
  show_board(grid);
  return 0;
}
