#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)

#define INIT_STACK_SIZE 10

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

bool is_on_board(Coord2 c) {
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
  int length;
  int *data;
} Stack;

void construct(int length, Stack *s) {
  s->top = -1;
  s->length = length;
  s->data = (int *)malloc(length * sizeof(int));
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
    perror("Error allocating memory");
    abort();
  }
}

void push(int val, Stack *s) {
  if (s->top == s->length - 1) {
    printf("Stack overflow, reallocating + INIT_STACK_SIZE");
    reallocate(s);
  }

  s->top++;
  s->data[s->top] = val;
}

int pop(Stack *s) {
  int n = s->data[s->top--];
  return n;
}

int contains(int val, Stack s) {
  for (int i = 0; i < s.length; i++) {
    if (val == s.data[i]) {
      return i;
    }
  }
  return -1;
}

void flood_fill(int fc, char board[], int all_neighbors[][4], Stack *chain,
                Stack *reached) {
  char color = board[fc];

  Stack frontier;
  construct(INIT_STACK_SIZE, &frontier);

  push(fc, &frontier);

  while (frontier.top != -1) {
    int current_fc = pop(&frontier);
    push(current_fc, chain);

    for (int i = 0; i < 4; i++) {
      int fn = all_neighbors[fc][i];
      if (board[fn] == color && !contains(fn, *chain))
        push(fn, &frontier);
      else if (board[fn] != color)
        push(fn, reached);
    }
  }

  free(frontier.data);
  // return chain and reached
}

void show_board(char board[N_LINES]) {
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

void move(char board[], Coord2 c, char color) { board[flatten(c)] = color; }

int main() {
  char board[N_INTERS];
  init_board(board);

  move(board, (Coord2){5, 5}, 'O');
  move(board, (Coord2){4, 5}, 'X');
  show_board(board);

  int all_neighbors[N_INTERS][4];
  get_all_neighbors(all_neighbors);

  Stack chain;
  construct(INIT_STACK_SIZE, &chain);

  Stack reached;
  construct(INIT_STACK_SIZE, &reached);

  flood_fill(20, board, all_neighbors, &reached, &chain);

  return 0;
}

// for (int fc = 0; fc < N_INTERS; fc++) {
//   printf("%d, %d, %d, %d\n", all_neighbors[fc][0], all_neighbors[fc][1],
//          all_neighbors[fc][2], all_neighbors[fc][3]);
// }
