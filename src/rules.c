#include <stdio.h>
#include <stdlib.h>

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)

#define BLACK 'O'
#define WHITE 'X'
#define EMPTY '.'

#define INIT_STACK_SIZE 8

#define BLACK_OR_WHITE(c) c == 'O' ? "BLACK" : "WHITE"

int ko_point = -1;

typedef struct {
  int row;
  int col;
} Coord2;

int flatten(Coord2 c) { return N_LINES * c.row + c.col; }
Coord2 unflatten(int fc) { return (Coord2){fc / N_LINES, fc % N_LINES}; }

void init_board(char board[]) {
  for (int i = 0; i < N_INTERS; i++) {
    board[i] = '.';
  }
}

int is_on_board(Coord2 c) {
  return c.row >= 0 && c.row < N_LINES && c.col >= 0 && c.col < N_LINES;
}

void get_neighbors(int fc, int out[]) {
  Coord2 c = unflatten(fc);

  Coord2 ns[4];
  int n = 0;
  ns[n++] = (Coord2){c.row - 1, c.col};
  ns[n++] = (Coord2){c.row + 1, c.col};
  ns[n++] = (Coord2){c.row, c.col - 1};
  ns[n++] = (Coord2){c.row, c.col + 1};

  for (int i = 0; i < 4; i++) {
    if (is_on_board(ns[i]))
      out[i] = flatten(ns[i]);
    else
      out[i] = -1;
  }
}

void get_all_neighbors(int all_neighbors[][4]) {
  for (int fc = 0; fc < N_INTERS; fc++) {
    get_neighbors(fc, all_neighbors[fc]);
  }
}

typedef struct {
  int top;
  unsigned int length;
  int *data;
} Stack;

void construct(Stack *s) {
  s->top = -1;
  s->length = INIT_STACK_SIZE;
  s->data = (int *)malloc(INIT_STACK_SIZE * sizeof(int));
  if (!s->data) {
    perror("Error allocating memory");
    abort();
  }
}

void reallocate(Stack *s) {
  s->length += INIT_STACK_SIZE;
  s->data = realloc(s->data, s->length * sizeof(int));
  if (!s->data) {
    perror("Error reallocating memory");
    abort();
  }
}

void push(int val, Stack *s) {
  if (s->top >= s->length - 1) {
    printf("Reallocating stack\n");
    reallocate(s);
  }

  s->top++;
  s->data[s->top] = val;
}

int pop(Stack *s) {
  if (s->top <= -1) {
    perror("Stack underflow, something is wrong\n");
    abort();
  }
  int n = s->data[s->top--];
  return n;
}

int contains(int val, Stack *s) {
  for (int i = 0; i <= s->top; i++) {
    if (val == s->data[i]) {
      return 1;
    }
  }
  return 0;
}

void flood_fill(int fc, char board[], int all_neighbors[][4], Stack *reached,
                Stack *chain) {
  char color = board[fc];

  Stack frontier;
  construct(&frontier);

  push(fc, &frontier);

  while (frontier.top != -1) {
    int current_fc = pop(&frontier);
    push(current_fc, chain);

    for (int i = 0; i < 4; i++) {
      int fn = all_neighbors[current_fc][i];
      if (fn != -1 && !contains(fn, chain)) {
        if (board[fn] == color)
          push(fn, &frontier);
        else
          push(fn, reached);
      }
    }
  }

  free(frontier.data);
}

int unsigned get_liberties(int fc, char board[], int all_neighbors[][4],
                           Stack *reached, Stack *chain) {
  reached->top = -1;
  chain->top = -1;
  flood_fill(fc, board, all_neighbors, reached, chain);

  int unsigned liberties = 0;
  for (int i = 0; i <= reached->top; i++) {
    int fc = reached->data[i];
    if (board[fc] == EMPTY)
      liberties++;
  }

  return liberties;
}

int maybe_capture(unsigned int libs, char board[], Stack *chain) {
  if (libs == 0) {
    for (int i = 0; i <= chain->top; i++) {
      int fc = chain->data[i];
      board[fc] = EMPTY;
    }
    return 1;
  }
  return 0;
}

void move(Coord2 c, char board[], int all_neighbors[][4], Stack *reached,
          Stack *chain, char color) {
  int fc = flatten(c);

  if (board[fc] != EMPTY) {
    printf("ILLEGAL MOVE: (%d, %d) already occupied\n", c.row, c.col);
    return;
  }

  if (fc != -1 && fc == ko_point) {
    printf("ILLEGAL MOVE: Ko violation at (%d, %d)\n", c.row, c.col);
    return;
  }

  ko_point = -1;

  board[fc] = color;

  Stack opp_stones;
  construct(&opp_stones);

  for (int i = 0; i < 4; i++) {
    int fn = all_neighbors[fc][i];
    if (fn != -1) {
      if (board[fn] != EMPTY && board[fn] != color)
        push(fn, &opp_stones);
    }
  }

  int unsigned captured_stone_count = 0;
  int single_capture_point = -1;

  for (int i = 0; i <= opp_stones.top; i++) {
    int fn = opp_stones.data[i];
    if (board[fn] == EMPTY)
      continue;

    int unsigned fn_libs =
        get_liberties(fn, board, all_neighbors, reached, chain);

    // remove opponent's stones
    if (maybe_capture(fn_libs, board, chain)) {
      captured_stone_count += chain->top + 1;

      if (chain->top == 0)
        single_capture_point = chain->data[0];
    }
  }

  int unsigned fc_libs =
      get_liberties(fc, board, all_neighbors, reached, chain);

  if (fc_libs == 0) {
    board[fc] = EMPTY;
    printf("ILLEGAL MOVE: Suicide at (%d, %d)\n", c.row, c.col);
    free(opp_stones.data);
    return;
  }

  int fc_group_size = chain->top + 1;
  if (captured_stone_count == 1 && fc_libs == 1 && fc_group_size == 1)
    ko_point = single_capture_point;
  else
    ko_point = -1;

  free(opp_stones.data);
}

void score(char board[]) {}

void show_board(char board[]) {
  printf("*------------------------------------------------------*\n");
  for (int i = 0; i < N_LINES; i++) {
    printf("|");
    for (int j = 0; j < N_LINES; j++) {
      int fc = flatten((Coord2){i, j});
      printf(" %c ", board[fc]);
    }
    printf("|\n");
  }
  printf("*------------------------------------------------------*\n");
}

int main() {
  char board[N_INTERS];
  init_board(board);

  int all_neighbors[N_INTERS][4];
  get_all_neighbors(all_neighbors);

  Stack chain;
  construct(&chain);

  Stack reached;
  construct(&reached);

  // move((Coord2){1, 3}, board, all_neighbors, &reached, &chain, BLACK);
  // show_board(board);
  //
  // move((Coord2){0, 3}, board, all_neighbors, &reached, &chain, WHITE);
  // show_board(board);
  //
  // move((Coord2){0, 2}, board, all_neighbors, &reached, &chain, BLACK);
  // show_board(board);
  //
  // move((Coord2){0, 4}, board, all_neighbors, &reached, &chain, WHITE);
  // show_board(board);
  //
  // move((Coord2){0, 5}, board, all_neighbors, &reached, &chain, BLACK);
  // show_board(board);
  //
  // // filler White move elsewhere, unrelated
  // move((Coord2){5, 5}, board, all_neighbors, &reached, &chain, WHITE);
  // show_board(board);
  //
  // // This fills the last liberty at (1,4) -> should capture the 2-stone edge
  // // group
  // move((Coord2){1, 4}, board, all_neighbors, &reached, &chain, BLACK);
  // show_board(board);

  // --- Ko test setup (same as before) ---

  move((Coord2){2, 4}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){3, 6}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){4, 4}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){2, 5}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){3, 3}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){4, 5}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){7, 7}, board, all_neighbors, &reached, &chain,
       BLACK); // filler, irrelevant
  show_board(board);

  move((Coord2){3, 4}, board, all_neighbors, &reached, &chain,
       WHITE); // stone to be captured; liberty only at (3,5)
  show_board(board);

  // --- Ko violation test ---

  // Black captures the lone White stone at (3,4).
  // Ko point should now be set to (3,4).
  move((Coord2){3, 5}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // ILLEGAL: White immediately tries to recapture at (3,4).
  // This recreates the exact position from before Black's last move
  // (White stone at (3,4), Black stone at (3,5) removed).
  // move() should detect this violates ko and reject the move —
  // board state after this call should be UNCHANGED from the previous
  // show_board() output.
  move((Coord2){3, 4}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  free(chain.data);
  free(reached.data);

  return 0;
}
