#include "rules.h"
#include <stdio.h>
#include <stdlib.h>

int ko_point = -1;

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

int get_idx(int elem, int len, int arr[]) {
  for (int i = 0; i < len; i++) {
    if (elem == arr[i])
      return i;
  }
  return -1;
}

void flood_fill(int fc, int board[], int all_neighbors[][4], Stack *reached,
                Stack *chain) {
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

int unsigned get_liberties(int fc, int board[], int all_neighbors[][4],
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

int maybe_capture(unsigned int libs, int board[], Stack *chain) {
  if (libs == 0) {
    for (int i = 0; i <= chain->top; i++) {
      int fc = chain->buffer[i];
      board[fc] = EMPTY;
    }
    return 1;
  }
  return 0;
}

void move(Coord2 c, int board[], int all_neighbors[][4], Stack *reached,
          Stack *chain, int color) {
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

void score(int board[], int all_neighbors[][4], int *black_score,
           int *white_score) {
  int score_board[N_INTERS];
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
        score_board[fb] = '?';
      }
    }
  }

  free(empties.buffer);
  free(borders.buffer);
}
