#include "rules.h"
#include <stdio.h>
#include <stdlib.h>

int ko_point = -1;

static void get_neighbors(int, int *);

static int get_idx(int, int, int *);
static void flood_fill(int, int *, const int[][4], Stack *, Stack *);

static int unsigned get_liberties(int, int *, const int[][4], Stack *, Stack *);
static int maybe_capture(unsigned int, int *, Stack *);

static inline int flatten(Coord2 c) { return N_LINES * c.row + c.col; }

static inline Coord2 unflatten(int fc) {
  return (Coord2){fc / N_LINES, fc % N_LINES};
}

static inline void clone_arr(int from[], int to[], int len) {
  for (int i = 0; i < len; i++)
    to[i] = from[i];
}

BoardState *init_bs(const int player_color) {
  BoardState *bs = malloc(sizeof(BoardState));

  bs->fc = -1;
  bs->color = player_color;

  for (int i = 0; i < BOARD_SIZE; i++) {
    bs->board[i] = EMPTY;
    get_neighbors(i, bs->all_neighbors[i]);
  }

  construct(&bs->reached);
  construct(&bs->chain);

  return bs;
}

static void get_neighbors(int fc, int out[]) {
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

static int get_idx(int elem, int len, int arr[]) {
  for (int i = 0; i < len; i++) {
    if (elem == arr[i])
      return i;
  }
  return -1;
}

static void flood_fill(int fc, int board[], const int all_neighbors[][4],
                       Stack *reached, Stack *chain) {
  int color = board[fc];

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

static int unsigned get_liberties(int fc, int board[],
                                  const int all_neighbors[][4], Stack *reached,
                                  Stack *chain) {
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

static int maybe_capture(unsigned int libs, int board[], Stack *chain) {
  if (libs == 0) {
    for (int i = 0; i <= chain->top; i++) {
      int fc = chain->buffer[i];
      board[fc] = EMPTY;
    }
    return 1;
  }
  return 0;
}

int move(BoardState *bs, char *return_buf, int *buf_len) {
  Coord2 c = unflatten(bs->fc);

  if (bs->fc < 0 || bs->fc > BOARD_SIZE) {
    *buf_len = snprintf(return_buf, *buf_len,
                        "ILLEGAL MOVE: position (%d, %d) is out of bounds",
                        c.row, c.col);
    return 1;
  }

  if (bs->board[bs->fc] != EMPTY) {
    *buf_len =
        snprintf(return_buf, *buf_len,
                 "ILLEGAL MOVE: (%d, %d) already occupied", c.row, c.col);
    // printf("ILLEGAL MOVE: (%d, %d) already occupied\n", c.row, c.col);
    return 1;
  }

  if (bs->fc == ko_point) {
    *buf_len = snprintf(return_buf, *buf_len,
                        "ILLEGAL MOVE: ko violation at (%d, %d)", c.row, c.col);
    // printf("ILLEGAL MOVE: ko violation at (%d, %d)\n", c.row, c.col);
    return 1;
  }

  ko_point = -1;

  bs->board[bs->fc] = bs->color;

  Stack opp_stones;
  construct(&opp_stones);

  for (int i = 0; i < 4; i++) {
    int fn = bs->all_neighbors[bs->fc][i];
    if (bs->board[fn] != EMPTY && bs->board[fn] != bs->color)
      push(fn, &opp_stones);
  }

  int unsigned captured_stone_count = 0;
  int single_capture_point = -1;

  for (int i = 0; i <= opp_stones.top; i++) {
    int fn = opp_stones.buffer[i];
    if (bs->board[fn] == EMPTY)
      continue;

    int unsigned fn_libs = get_liberties(fn, bs->board, bs->all_neighbors,
                                         &bs->reached, &bs->chain);

    // remove opponent's stones
    if (maybe_capture(fn_libs, bs->board, &bs->chain)) {
      captured_stone_count += bs->chain.top + 1;

      if (bs->chain.top == 0)
        single_capture_point = bs->chain.buffer[0];
    }
  }

  int unsigned fc_libs = get_liberties(bs->fc, bs->board, bs->all_neighbors,
                                       &bs->reached, &bs->chain);

  if (fc_libs == 0) {
    bs->board[bs->fc] = EMPTY;
    *buf_len = snprintf(return_buf, *buf_len,
                        "ILLEGAL MOVE: suicide at (%d, %d)", c.row, c.col);
    // printf("ILLEGAL MOVE: suicide at (%d, %d)\n", c.row, c.col);
    free(opp_stones.buffer);
    return 1;
  }

  int fc_group_size = bs->chain.top + 1;
  if (captured_stone_count == 1 && fc_libs == 1 && fc_group_size == 1)
    ko_point = single_capture_point;
  else
    ko_point = -1;

  free(opp_stones.buffer);

  *buf_len = snprintf(return_buf, *buf_len,
                      "Successfully placed stone at (%d, %d)", c.row, c.col);
  return 0;
}

void score(int board[], const int all_neighbors[][4], int *black_score,
           int *white_score) {
  int score_board[BOARD_SIZE];
  clone_arr(board, score_board, BOARD_SIZE);

  Stack empties;
  construct(&empties);

  Stack borders;
  construct(&borders);

  int fempty = 0;

  while (1) {
    fempty = get_idx(EMPTY, BOARD_SIZE, score_board);
    if (fempty == -1)
      break;

    empties.top = -1;
    borders.top = -1;
    flood_fill(fempty, score_board, all_neighbors, &borders, &empties);

    if (borders.top == -1)
      break;

    int maybe_border_color = score_board[borders.buffer[0]];
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
        score_board[fb] = UNDEF;
      }
    }
  }

  free(empties.buffer);
  free(borders.buffer);
}
