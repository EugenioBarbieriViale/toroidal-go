#include "gxf.h"
#include "net.h"
#include "rules.h"
#include <stdio.h> // for debug, then remove

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

void UpdateDrawFrame(void *);

int main() {
  printf("Connecting to server...\n");
  int server_fd = init_connection();
  printf("Connected to server\n");

  printf("Awaiting color decision from server...\n");
  int player_color = get_color_from_server(server_fd);
  printf("Color received from server: %d\n", player_color);

  MainLoopArg *main_loop_arg = gxf_init(server_fd, player_color);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop_arg(UpdateDrawFrame, main_loop_arg, 0, 1);
#else
  SetTargetFPS(FPS);

  while (!WindowShouldClose()) {
    UpdateDrawFrame(main_loop_arg);
  }
#endif

  gxf_cleanup(main_loop_arg);
  close_connection(server_fd);

  return 0;
}

void UpdateDrawFrame(void *arg_) {
  MainLoopArg *arg = arg_;

  static int start_game = 0, turn_count = 0;

  if (IsKeyPressed(KEY_SPACE)) {
    start_game = 1;
  }

  if (!start_game) {
    show_welcome_screen(arg->font);
    return;
  }

  if (turn_count % 2 == 0)
    arg->bs->color = SBLACK;
  else
    arg->bs->color = SWHITE;

  Vector3 mouse_delta = gxf_update(arg);
  arg->bs->fc = try_place_stone(mouse_delta, arg);

  if (arg->bs->fc != -1) {
    int init_buf_len = 256;
    char msg[init_buf_len];
    int move_success = move(arg->bs, msg, init_buf_len);
    printf("\n%s\n", msg);

    if (move_success) {
      int status = send_board_to_server(arg->server_fd, MOVE, arg->bs->fc,
                                        arg->bs->board);

      // and server says ok
      if (status > 0) {
        update_available(arg);
        turn_count++;
      }
    }

    // if (move_success) {
    //   update_available(arg);
    //   turn_count++;
    // }
  }

  gxf_draw(arg);
}

void show_ascii_board(int board[BOARD_SIZE]) {
  printf("-------------------------------------------------------\n");
  for (int i = 0; i < BOARD_SIZE; i++) {
    if (i != 0 && i % N_LINES == 0)
      printf("\n");
    printf(" % d", board[i]);
  }
  printf("\n-------------------------------------------------------\n");
}
