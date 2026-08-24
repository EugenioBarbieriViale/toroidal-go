#include "gxf.h"
#include "rules.h"
#include <stdlib.h>
#include <time.h>

void control_camera(Camera *, Vector3 *, int);

int try_place_stone_keyboard(MainLoopArg *);
int try_place_stone_mouse(Vector3, MainLoopArg *);

void compute_inters(Vector3 *);
void sort_points(int, AvailablePoint[], Vector3);
void draw_inters(Vector3, Vector3 *);

static inline int is_moving(void) {
  return (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) ||
          IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) ||
          IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT));
}

static inline Color get_rnd_color(void) {
  Color colors[9] = {SKYBLUE, YELLOW, RED,    PURPLE, BLUE,
                     GREEN,   BEIGE,  ORANGE, PINK};
  int ci = rand() % 9;
  return colors[ci];
}

MainLoopArg *gxf_init(const int server_fd, const int player_color) {
  MainLoopArg *arg = (MainLoopArg *)malloc(sizeof(MainLoopArg));

  srand(time(NULL));

  SetWindowState(FLAG_WINDOW_RESIZABLE);
  InitWindow(W, H, "Toroidal Go");

  arg->camera = (Camera){0};
  arg->camera.position = (Vector3){0.f, 0.f, 25.f};
  arg->camera.target = ORIGIN;
  arg->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  arg->camera.fovy = 45.0f;
  arg->camera.projection = CAMERA_PERSPECTIVE;

  arg->torus = LoadModelFromMesh(GenMeshTorus(NORM_RADIUS, SIZE, 32, 64));
  arg->black = LoadModelFromMesh(GenMeshSphere(1, 32, 64));
  arg->white = LoadModelFromMesh(GenMeshSphere(1, 32, 64));

  Texture2D texture = LoadTexture("./assets/board1.png");
  arg->torus.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/black_stone.png");
  arg->black.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/white_marble.png");
  arg->white.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  // arg->font = LoadFont("./assets/IosevkaNerdFont-Regular.ttf");
  arg->font = LoadFont("./assets/IosevkaNerdFont-Bold.ttf");

  compute_inters(arg->intersections);
  arg->bs = init_bs(player_color);

  update_available(arg);

  arg->focused_stone = arg->available_points[0].pos;

#if defined(PLATFORM_WEB)
  DisableCursor();
  arg->camera_mode = CAMERA_THIRD_PERSON;
#else
  arg->camera_mode = CAMERA_FIRST_PERSON;
#endif

  arg->camera_color = get_rnd_color();

  return arg;
}

Vector3 gxf_update(MainLoopArg *arg) {
  if (arg->camera_mode == CAMERA_THIRD_PERSON)
    sort_points(arg->av_count, arg->available_points, arg->camera.target);
  else
    sort_points(arg->av_count, arg->available_points, arg->camera.position);

  if (IsKeyPressed(KEY_X)) {
    arg->camera.position = Vector3Scale(arg->camera.position, -1.f);
    arg->camera.target = Vector3Scale(arg->camera.target, -1.f);
  }

#if defined(PLATFORM_WEB)
  static int was_pressed_r = 1;
#else
  static int was_pressed_r = 0;
#endif

  if (IsKeyPressed(KEY_R)) {
    if (was_pressed_r % 2 == 0) {
      DisableCursor();
      arg->camera_mode = CAMERA_THIRD_PERSON;
    } else {
      if (!is_moving())
        EnableCursor();
      arg->camera_mode = CAMERA_FIRST_PERSON;
    }

    arg->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    was_pressed_r++;
  }

  Vector3 mouse_delta;
  control_camera(&arg->camera, &mouse_delta, arg->camera_mode);

  return mouse_delta;
}

void gxf_draw(MainLoopArg *arg) {
  BeginDrawing();
  ClearBackground(GRAY);

  BeginMode3D(arg->camera);

  DrawModel(arg->torus, (Vector3){0.f, 0.f, 0.f}, 1, BEIGE);
  draw_inters(arg->focused_stone, arg->intersections);

  for (int i = 0; i < BOARD_SIZE; i++) {
    switch (arg->bs->board[i]) {
    case SBLACK:
      DrawModel(arg->black, arg->intersections[i], 1, GRAY);
      break;
    case SWHITE:
      DrawModel(arg->white, arg->intersections[i], 1, WHITE);
    }
  }

  if (arg->camera_mode == CAMERA_THIRD_PERSON) {
    DrawCube(arg->camera.target, 1.0f, 1.0f, 1.0f, arg->camera_color);
    DrawCubeWires(arg->camera.target, 1.0f, 1.0f, 1.0f, BLACK);
  }

  EndMode3D();

  DrawText(TextFormat("BLACK CAPTURES: %d", arg->bs->black_captured_stones), 10,
           30, 30, WHITE);
  DrawText(TextFormat("WHITE CAPTURES: %d", arg->bs->white_captured_stones), 10,
           60, 30, WHITE);

#if defined(PLATFORM_WEB)
#else
  DrawText(TextFormat("%.2f FPS", 1.f / GetFrameTime()), 10, 10, 20, WHITE);
#endif

  EndDrawing();
}

void gxf_cleanup(MainLoopArg *arg) {
  UnloadModel(arg->torus);
  UnloadModel(arg->black);
  UnloadModel(arg->white);

  UnloadFont(arg->font);

  free(arg->bs->reached.buffer);
  free(arg->bs->chain.buffer);
  free(arg->bs);

  free(arg);
  CloseWindow();
}

void static inline center_text_to_h(float fh, const char *text, int w, int h,
                                    Font f, Color color) {
  Vector2 text_dims = MeasureTextEx(f, text, (float)f.baseSize, 2);
  DrawTextEx(f, text, (Vector2){(w - text_dims.x) / 2.f, fh * h},
             (float)f.baseSize, 2, color);
}

void show_welcome_screen(Font f) {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  int w = GetScreenWidth(), h = GetScreenHeight();
  float font_size = f.baseSize;
  float spacing = 8.f;

  int count = 1;

  center_text_to_h((float)(count++) / (spacing + 1.f),
                   "Welcome to 3D toroidal go!", w, h, f, BLACK);

  center_text_to_h((float)count / spacing,
                   "Use W,S,A,D or the ARROW KEYS to move.", w, h, f, RED);
  center_text_to_h(((float)(count++) / spacing + (float)font_size / h),
                   "Hold SPACE+W to move up, and SPACE+S to move down", w, h, f,
                   RED);

  center_text_to_h((float)count / spacing,
                   "Press ENTER to place a stone on the closest free spot "
                   "(marked in green)",
                   w, h, f, BLUE);
  center_text_to_h(((float)(count++) / spacing + (float)font_size / h),
                   "or CLICK on any free spot (marked in red)", w, h, f, BLUE);

  center_text_to_h((float)count / spacing,
                   "Press E to focus your next closest free spot", w, h, f,
                   ORANGE);
  center_text_to_h(((float)(count++) / spacing + (float)font_size / h),
                   "or SHIFT+E to go to the previous closest one", w, h, f,
                   ORANGE);

  center_text_to_h((float)(count++) / spacing,
                   "Press R to change view mode (first or third person)", w, h,
                   f, GREEN);

  center_text_to_h((float)(count++) / spacing,
                   "Press X to go to your symmetrical position", w, h, f,
                   PURPLE);

  center_text_to_h((float)(++count) / (spacing + 1.f),
                   "Press SPACE to start playing!", w, h, f, BLACK);

  EndDrawing();
}

void control_camera(Camera *camera, Vector3 *mouse_delta, int camera_mode) {
  static bool was_moving = false;
  bool moving = is_moving();
  if (moving && !was_moving) {
    DisableCursor();
  }
  if (camera_mode != CAMERA_THIRD_PERSON && !moving && was_moving)
    EnableCursor();
  was_moving = moving;

  if (moving) {
    *mouse_delta =
        (Vector3){GetMouseDelta().x * 0.05f, GetMouseDelta().y * 0.05f, 0.0f};
  } else {
    *mouse_delta = ORIGIN;
  }

  Vector3 movement = ORIGIN;
  Vector3 rotation = *mouse_delta;
  float zoom = -GetMouseWheelMove() * 2.0f;

  if (IsKeyDown(KEY_SPACE) && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)))
    movement.z += 0.3f;
  else if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    movement.x += 0.3f;
  if (IsKeyDown(KEY_SPACE) && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)))
    movement.z -= 0.3f;
  else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    movement.x -= 0.3f;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    movement.y += 0.3f;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    movement.y -= 0.3f;

  if (camera_mode == CAMERA_THIRD_PERSON) {
    Vector2 mouseDelta = GetMouseDelta();
    rotation.x = mouseDelta.x * 0.05f;
    rotation.y = mouseDelta.y * 0.05f;
  }

  UpdateCameraPro(camera, movement, rotation, zoom);
}

void update_available(MainLoopArg *arg) {
  arg->av_count = 0;
  for (int i = 0; i < BOARD_SIZE; i++) {
    int stone = arg->bs->board[i];
    if (stone == EMPTY || stone == UNDEF) {
      arg->available_points[arg->av_count++] = (AvailablePoint){
          .idx = i,
          .pos = arg->intersections[i],
      };
    }
  }
}

int try_place_stone(Vector3 mouse_delta, MainLoopArg *arg) {
  int keyboard_fc = try_place_stone_keyboard(arg);
  int mouse_fc = try_place_stone_mouse(mouse_delta, arg);

  if (keyboard_fc == -1 && mouse_fc == -1)
    return -1;

  if (keyboard_fc == -1)
    return mouse_fc;

  return keyboard_fc;
}

int try_place_stone_keyboard(MainLoopArg *arg) {
  static int count_from_closest = 0;

  if (IsKeyPressed(KEY_E) && IsKeyDown(KEY_LEFT_SHIFT)) {
    if (count_from_closest > 0)
      count_from_closest--;
  } else if (IsKeyPressed(KEY_E) && count_from_closest < BOARD_SIZE)
    count_from_closest++;

  if (is_moving())
    count_from_closest = 0;

  arg->focused_stone = arg->available_points[count_from_closest].pos;

  int virtual_cursor_click = (arg->camera_mode == CAMERA_THIRD_PERSON &&
                              IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

  if (IsKeyPressed(KEY_ENTER) || virtual_cursor_click) {
    int temp = count_from_closest;
    count_from_closest = 0;
    return arg->available_points[temp].idx;
  }

  return -1;
}

int try_place_stone_mouse(Vector3 mouse_delta, MainLoopArg *arg) {
  if (!ARE_EQUAL(mouse_delta, ORIGIN))
    return -1;

  Ray ray = GetScreenToWorldRay(GetMousePosition(), arg->camera);

  for (int i = 0; i < arg->av_count; i++) {
    RayCollision collision = GetRayCollisionSphere(
        ray, arg->available_points[i].pos, COLLISION_RADIUS);

    if (collision.hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      return arg->available_points[i].idx;
    }
  }

  return -1;
}

void compute_inters(Vector3 *intersections) {
  for (int i = 0; i < N_LINES; i++) {
    float theta = (float)i * UNIT_ANGLE;

    for (int j = i * N_LINES; j < (i + 1) * N_LINES; j++) {
      int k = j - i * N_LINES;
      float phi = (float)k * UNIT_ANGLE;

      intersections[j] =
          (Vector3){(CENTER_RADIUS + TUBE_RADIUS * cosf(phi)) * cosf(theta),
                    (CENTER_RADIUS + TUBE_RADIUS * cosf(phi)) * sinf(theta),
                    TUBE_RADIUS * sinf(phi)};
    }
  }
}

void sort_points(int count, AvailablePoint available_points[],
                 Vector3 cam_pos) {
  float distances[count];
  for (int i = 0; i < count; i++) {
    distances[i] = dist(cam_pos, available_points[i].pos);
  }

  for (int i = 1; i < count; i++) {
    AvailablePoint key_point = available_points[i];
    float key_dst = distances[i];
    int j = i - 1;

    while (j >= 0 && distances[j] > key_dst) {
      available_points[j + 1] = available_points[j];
      distances[j + 1] = distances[j];
      j--;
    }

    available_points[j + 1] = key_point;
    distances[j + 1] = key_dst;
  }
}

void draw_inters(Vector3 focused_stone, Vector3 *intersections) {
  Color color = RED;
  for (int i = 0; i < BOARD_SIZE; i++) {
    Vector3 v = intersections[i];
    color = RED;

    if (ARE_EQUAL(v, focused_stone))
      color = GREEN;

    DrawCubeV(v, (Vector3){0.2f, 0.2f, 0.2f}, color);
    DrawCubeWiresV(v, (Vector3){0.2f, 0.2f, 0.2f}, BLACK);
  }
}
