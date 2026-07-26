#include "gxf.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

void UpdateDrawFrame(void *);

int main() {
  MainLoopArg *main_loop_arg = gxf_init();

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

  gxf_update(arg);
  gxf_draw(arg);
}
