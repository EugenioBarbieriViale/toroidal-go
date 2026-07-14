#include <stdio.h>
#include <stdlib.h>

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)

#define BLACK 'O'
#define WHITE 'X'
#define EMPTY '.'

#define INIT_STACK_SIZE 8

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
  int *re_data = realloc(s->data, s->length * sizeof(int));
  if (re_data)
    s->data = re_data;
  else {
    perror("Error reallocating memory");
    abort();
  }
}

void push(int val, Stack *s) {
  if (s->top == s->length - 1) {
    printf("Reallocating stack\n");
    reallocate(s);
  }

  s->top++;
  s->data[s->top] = val;
}

int pop(Stack *s) {
  if (s->top == -1) {
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
      // if (fn != -1 && board[fn] == color && !contains(fn, chain))
      //   push(fn, &frontier);
      // else if (board[fn] != color)
      //   push(fn, reached);

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

// memcpy captured stones
int unsigned maybe_capture(int fc, char board[], int all_neighbors[][4],
                           Stack *reached, Stack *chain) {
  reached->top = -1;
  chain->top = -1;
  flood_fill(fc, board, all_neighbors, reached, chain);

  int liberties = 0;
  for (int i = 0; i <= reached->top; i++) {
    int fc = reached->data[i];
    if (board[fc] == EMPTY)
      liberties++;
  }

  if (liberties == 0) {
    for (int i = 0; i <= chain->top; i++) {
      int fc = chain->data[i];
      board[fc] = EMPTY;
    }
  }

  return liberties;
}

void move(Coord2 c, char board[], int all_neighbors[][4], Stack *reached,
          Stack *chain, char color) {
  int fc = flatten(c);

  if (board[fc] != EMPTY) {
    printf("Illegal move\n");
    return;
  }

  board[fc] = color;

  Stack opp_stones;
  construct(&opp_stones);

  Stack my_stones;
  construct(&my_stones);

  for (int i = 0; i < 4; i++) {
    int fn = all_neighbors[fc][i];
    if (board[fn] == color)
      push(fn, &my_stones);
    else if (board[fn] != EMPTY && board[fn] != color)
      push(fn, &opp_stones);
  }

  for (int i = 0; i <= opp_stones.top; i++) {
    int fc = opp_stones.data[i];
    int lib = maybe_capture(fc, board, all_neighbors, reached, chain);
    printf("Opp libs: %d\n", lib);
  }

  for (int i = 0; i <= my_stones.top; i++) {
    int fc = my_stones.data[i];
    int lib = maybe_capture(fc, board, all_neighbors, reached, chain);
    printf("My libs: %d\n", lib);
  }

  free(opp_stones.data);
  free(my_stones.data);
}

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

  move((Coord2){1, 2}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){2, 2}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){1, 3}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){2, 3}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){2, 1}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){3, 2}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){3, 1}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  move((Coord2){3, 3}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){2, 4}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // filler White move elsewhere on the board, unrelated to the group
  move((Coord2){7, 7}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){3, 4}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // filler White move
  move((Coord2){7, 8}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  move((Coord2){4, 2}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  // filler White move
  move((Coord2){7, 6}, board, all_neighbors, &reached, &chain, WHITE);
  show_board(board);

  // This fills the LAST liberty of the White group -> should trigger capture
  move((Coord2){4, 3}, board, all_neighbors, &reached, &chain, BLACK);
  show_board(board);

  free(chain.data);
  free(reached.data);

  return 0;
}
