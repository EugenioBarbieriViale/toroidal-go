#include <stdio.h>
#include <stdlib.h>

#define N_LINES 19
#define N_INTERS (N_LINES * N_LINES)

#define BLACK 'O'
#define WHITE 'X'
#define EMPTY '.'

#define INIT_STACK_SIZE 8

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

void get_neighbors(int fc, int out[]) {
  Coord2 c = unflatten(fc);

  Coord2 ns[4];
  int n = 0;
  ns[n++] = (Coord2){(c.row - 1 + N_LINES) % N_LINES, c.col};
  ns[n++] = (Coord2){(c.row + 1) % N_LINES, c.col};
  ns[n++] = (Coord2){c.row, (c.col - 1 + N_LINES) % N_LINES};
  ns[n++] = (Coord2){c.row, (c.col + 1) % N_LINES};

  for (int i = 0; i < 4; i++) {
    out[i] = flatten(ns[i]);
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
  int *buffer;
} Stack;

void construct(Stack *s) {
  s->top = -1;
  s->length = INIT_STACK_SIZE;
  s->buffer = (int *)malloc(INIT_STACK_SIZE * sizeof(int));
  if (!s->buffer) {
    perror("Error allocating memory");
    abort();
  }
}

void reallocate(int size, Stack *s) {
  s->length += size;
  s->buffer = realloc(s->buffer, s->length * sizeof(int));
  if (!s->buffer) {
    perror("Error reallocating memory");
    abort();
  }
}

void push(int val, Stack *s) {
  if (s->top >= s->length - 1) {
    reallocate(INIT_STACK_SIZE, s);
    // printf("Adding memory to stack, lenght now is %d\n", s->length);
  }

  s->top++;
  s->buffer[s->top] = val;
}

int pop(Stack *s) {
  if (s->top <= -1) {
    perror("Stack underflow, something is wrong\n");
    abort();
  }

  int len_diff = s->length - 2 * INIT_STACK_SIZE;
  if (len_diff > 0 && s->top <= len_diff) {
    reallocate(-1 * INIT_STACK_SIZE, s);
    // printf("Removing unused memory from stack, lenght now is %d\n",
    // s->length);
  }

  int n = s->buffer[s->top--];
  return n;
}

int get_idx_int(int elem, int len, int arr[]) {
  for (int i = 0; i < len; i++) {
    if (elem == arr[i])
      return i;
  }
  return -1;
}

int get_idx_char(char elem, int len, char arr[]) {
  for (int i = 0; i < len; i++) {
    if (elem == arr[i])
      return i;
  }
  return -1;
}

// useless, make board full int
#define get_idx(elem, len, arr)                                                \
  _Generic((arr), int *: get_idx_int, char *: get_idx_char)(elem, len, arr)

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

      int fn_in_chain = get_idx(fn, chain->top + 1, chain->buffer) != -1;
      int fn_in_frontier = get_idx(fn, frontier.top + 1, frontier.buffer) != -1;
      if (fn_in_chain || fn_in_frontier)
        continue;

      if (board[fn] == color)
        push(fn, &frontier);
      else
        push(fn, reached);
    }
  }

  free(frontier.buffer);
}

int unsigned get_liberties(int fc, char board[], int all_neighbors[][4],
                           Stack *reached, Stack *chain) {
  reached->top = -1;
  chain->top = -1;
  flood_fill(fc, board, all_neighbors, reached, chain);

  int unsigned liberties = 0;
  for (int i = 0; i <= reached->top; i++) {
    int fc = reached->buffer[i];
    if (board[fc] == EMPTY)
      liberties++;
  }

  return liberties;
}

int maybe_capture(unsigned int libs, char board[], Stack *chain) {
  if (libs == 0) {
    for (int i = 0; i <= chain->top; i++) {
      int fc = chain->buffer[i];
      board[fc] = EMPTY;
    }
    return 1;
  }
  return 0;
}

int is_on_board(Coord2 c) {
  return c.row >= 0 && c.row < N_LINES && c.col >= 0 && c.col < N_LINES;
}

void move(Coord2 c, char board[], int all_neighbors[][4], Stack *reached,
          Stack *chain, char color) {
  if (!is_on_board(c)) {
    printf("ILLEGAL MOVE: out of bounds (%d, %d)\n", c.row, c.col);
    return;
  }

  int fc = flatten(c);

  if (board[fc] != EMPTY) {
    printf("ILLEGAL MOVE: (%d, %d) already occupied\n", c.row, c.col);
    return;
  }

  if (fc == ko_point) {
    printf("ILLEGAL MOVE: ko violation at (%d, %d)\n", c.row, c.col);
    return;
  }

  ko_point = -1;

  board[fc] = color;

  Stack opp_stones;
  construct(&opp_stones);

  for (int i = 0; i < 4; i++) {
    int fn = all_neighbors[fc][i];
    if (board[fn] != EMPTY && board[fn] != color)
      push(fn, &opp_stones);
  }

  int unsigned captured_stone_count = 0;
  int single_capture_point = -1;

  for (int i = 0; i <= opp_stones.top; i++) {
    int fn = opp_stones.buffer[i];
    if (board[fn] == EMPTY)
      continue;

    int unsigned fn_libs =
        get_liberties(fn, board, all_neighbors, reached, chain);

    // remove opponent's stones
    if (maybe_capture(fn_libs, board, chain)) {
      captured_stone_count += chain->top + 1;

      if (chain->top == 0)
        single_capture_point = chain->buffer[0];
    }
  }

  int unsigned fc_libs =
      get_liberties(fc, board, all_neighbors, reached, chain);

  if (fc_libs == 0) {
    board[fc] = EMPTY;
    printf("ILLEGAL MOVE: suicide at (%d, %d)\n", c.row, c.col);
    free(opp_stones.buffer);
    return;
  }

  int fc_group_size = chain->top + 1;
  if (captured_stone_count == 1 && fc_libs == 1 && fc_group_size == 1)
    ko_point = single_capture_point;
  else
    ko_point = -1;

  free(opp_stones.buffer);
}

void clone_arr(char from[], char to[], int len) {
  for (int i = 0; i < len; i++)
    to[i] = from[i];
}

void score(char board[], int all_neighbors[][4], int *black_score,
           int *white_score) {
  char score_board[N_INTERS];
  clone_arr(board, score_board, N_INTERS);

  Stack empties;
  construct(&empties);

  Stack borders;
  construct(&borders);

  int fempty = 0;

  while (1) {
    fempty = get_idx(EMPTY, N_INTERS, score_board);
    if (fempty == -1)
      break;

    empties.top = -1;
    borders.top = -1;
    flood_fill(fempty, score_board, all_neighbors, &borders, &empties);

    if (borders.top == -1)
      break;

    char maybe_border_color = score_board[borders.buffer[0]];
    int all_same_color = 1;

    for (int i = 0; i <= borders.top; i++) {
      int fb = borders.buffer[i];
      if (score_board[fb] != maybe_border_color) {
        all_same_color = 0;
        break;
      }
    }

    if (all_same_color) {
      for (int i = 0; i <= empties.top; i++) {
        int fb = empties.buffer[i];
        score_board[fb] = maybe_border_color;

        if (maybe_border_color == BLACK)
          (*black_score)++;
        else if (maybe_border_color == WHITE)
          (*white_score)++;
      }
    } else {
      for (int i = 0; i <= empties.top; i++) {
        int fb = empties.buffer[i];
        score_board[fb] = '?';
      }
    }
  }

  free(empties.buffer);
  free(borders.buffer);
}

void show_board(char board[]) {
  printf("*---------------------------------------------------------*\n");
  for (int i = 0; i < N_LINES; i++) {
    printf("|");
    for (int j = 0; j < N_LINES; j++) {
      int fc = flatten((Coord2){i, j});
      printf(" %c ", board[fc]);
    }
    printf("|\n");
  }
  printf("*---------------------------------------------------------*\n");
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

  return 0;
}
