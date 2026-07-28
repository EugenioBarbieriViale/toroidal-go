#include "gxf.h"
#include "rules.h"
#include <stdio.h> // for debug, then remove

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int init_buf_len = 3 * 256;

void UpdateDrawFrame(void *);

int main() {
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

  return 0;
}

void UpdateDrawFrame(void *arg_) {
  MainLoopArg *arg = arg_;

  if (arg->count % 2 == 0)
    arg->bs->color = SBLACK;
  else
    arg->bs->color = SWHITE;

  Vector3 mouse_delta = gxf_update(arg);
  arg->bs->fc = try_place_stone(mouse_delta, arg);

  if (arg->bs->fc != -1) {
    char msg[init_buf_len];
    int ans = move(arg->bs, msg, &init_buf_len);
    printf("\n%s\n", msg);

    // printf("-------------------------------------------------------\n");
    // for (int i = 0; i < BOARD_SIZE; i++) {
    //   if (i != 0 && i % N_LINES == 0)
    //     printf("\n");
    //   printf(" % d", arg->bs->board[i]);
    // }
    // printf("\n-------------------------------------------------------\n");

    if (ans == 0)
      update_available(arg);
  }

  gxf_draw(arg);
}
