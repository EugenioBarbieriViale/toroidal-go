#include "gxf.h"
#include "net.h"
#include "rules.h"
#include <stdio.h> // for debug, then remove

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int init_buf_len = 3 * INIT_BUF_LEN;

void UpdateDrawFrame(void *);

int main() {
  // int server_fd = init_connection();
  // const int color = get_color_from_server(server_fd);
  // printf("RECEIVED COLOR :%d\n", color);

  MainLoopArg *main_loop_arg = gxf_init(1);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop_arg(UpdateDrawFrame, main_loop_arg, 0, 1);
#else
  SetTargetFPS(FPS);

  while (!WindowShouldClose()) {
    UpdateDrawFrame(main_loop_arg);
  }
#endif

  gxf_cleanup(main_loop_arg);
  // close_connection(server_fd);

  return 0;
}

void UpdateDrawFrame(void *arg_) {
  MainLoopArg *arg = arg_;

  if (arg->count % 2 == 0)
    arg->bs->color = SBLACK;
  else
    arg->bs->color = SWHITE;

  // you need to update first, so that the intersections are sorted first
  Vector3 mouse_delta = gxf_update(arg);
  arg->bs->fc = try_place_stone(mouse_delta, arg);

  if (arg->bs->fc != -1) {
    char msg[init_buf_len];
    int ans = move(arg->bs, msg, &init_buf_len);
    printf("\n%s\n", msg);

    printf("-------------------------------------------------------\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
      if (i != 0 && i % N_LINES == 0)
        printf("\n");
      printf(" % d", arg->bs->board[i]);
    }
    printf("\n-------------------------------------------------------\n");

    if (ans != 1)
      update_board(arg);
  }

  gxf_draw(arg);
}
