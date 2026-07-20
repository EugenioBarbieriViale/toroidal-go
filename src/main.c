// TODO
// - if not moving, show cursor and let click to place stones

#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_WEB)
#include "./external/raylib/src/raylib.h"
#include "./external/raylib/src/raymath.h"
#include <emscripten/emscripten.h>
#else
#include "raylib.h"
#include "raymath.h"
#endif

#define W 800
#define H 600
#define FPS 30

#define NORM_RADIUS 0.6f
#define SIZE 22.f

#define CENTER_RADIUS (SIZE / 2.0f)
#define TUBE_RADIUS (NORM_RADIUS * SIZE / 2.0f)

#define REALLOC_THRESHOLD 8
#define EPS ((float)(10e-3))

static inline float dist(Vector3 v, Vector3 w) {
  return Vector3Length(Vector3Subtract(v, w));
}

#define IS_ZERO(v) (Vector3Length(v) < EPS)
#define ARE_EQUAL(v, w) (dist(v, w) < EPS)

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

const float COLLISION_RADIUS = 0.2f;
const Vector3 ORIGIN = {0.f, 0.f, 0.f};

typedef struct {
  Camera camera;

  Model torus;
  Model black;
  Model white;

  Vector3 *stones;
  Vector3 *intersections;
  Vector3 *sorted_inters;

  Vector3 focused_stone;
  int count_from_closest;
  int count;

  int camera_mode;
} MainLoopArg;

void UpdateDrawFrame(void *);
void control_camera(Camera *, Vector3 *, int *);

int get_stone_idx(Vector3 *, Vector3 *);
void pop_stone(int, int, Vector3 **);
void place_stone_keyboard(MainLoopArg *);
void place_stone_mouse(Vector3 *, MainLoopArg *);

void compute_inters(Vector3 *);
void sort_inters(int, Vector3 **, Vector3);
void draw_inters(Vector3 *, Vector3 *);

int main() {
  InitWindow(W, H, "Toroidal Go");
  DisableCursor();

  Camera camera = {0};
  camera.position = (Vector3){0.f, 0.f, 25.f};
  camera.target = ORIGIN;
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  Model torus = LoadModelFromMesh(GenMeshTorus(NORM_RADIUS, SIZE, 32, 64));
  Model black = LoadModelFromMesh(GenMeshSphere(1, 32, 64));
  Model white = LoadModelFromMesh(GenMeshSphere(1, 32, 64));

  Texture2D texture = LoadTexture("./assets/board.png");
  torus.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/black_stone.png");
  black.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/white_marble.png");
  white.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  Vector3 stones[N_INTERS];
  for (int i = 0; i < N_INTERS; i++)
    stones[i] = ORIGIN;

  Vector3 intersections[N_INTERS];
  compute_inters(intersections);

  Vector3 *sorted_inters = (Vector3 *)malloc(N_INTERS * sizeof(Vector3));
  compute_inters(sorted_inters);

  MainLoopArg *main_loop_arg = (MainLoopArg *)malloc(sizeof(MainLoopArg));
  main_loop_arg->camera = camera;

  main_loop_arg->torus = torus;
  main_loop_arg->black = black;
  main_loop_arg->white = white;

  main_loop_arg->stones = stones;
  main_loop_arg->intersections = intersections;
  main_loop_arg->sorted_inters = sorted_inters;

  main_loop_arg->focused_stone = sorted_inters[0];
  main_loop_arg->count_from_closest = 0;
  main_loop_arg->count = 0;
  main_loop_arg->camera_mode = CAMERA_FIRST_PERSON;

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop_arg(UpdateDrawFrame, main_loop_arg, 0, 1);
#else
  SetTargetFPS(FPS);

  while (!WindowShouldClose()) {
    UpdateDrawFrame(main_loop_arg);
  }
#endif

  UnloadModel(torus);
  UnloadModel(black);
  UnloadModel(white);

  UnloadTexture(texture);

  free(main_loop_arg->sorted_inters);
  free(main_loop_arg);

  CloseWindow();

  return 0;
}

static inline int is_moving(void) {
  return (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) ||
          IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) ||
          IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT));
}

void control_camera(Camera *camera, Vector3 *mouse_delta, int *camera_mode) {
  static bool was_moving = false;
  bool moving = is_moving();
  if (moving && !was_moving)
    DisableCursor();
  if (*camera_mode != CAMERA_THIRD_PERSON && !moving && was_moving)
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

  if (*camera_mode == CAMERA_THIRD_PERSON) {
    Vector2 mouseDelta = GetMouseDelta();
    rotation.x = mouseDelta.x * 0.05f;
    rotation.y = mouseDelta.y * 0.05f;
  }

  UpdateCameraPro(camera, movement, rotation, zoom);
}

void UpdateDrawFrame(void *arg_) {
  MainLoopArg *arg = arg_;

  int new_len = N_INTERS - arg->count;
  sort_inters(new_len, &arg->sorted_inters, arg->camera.position);

  if (IsKeyPressed(KEY_ONE)) {
    arg->camera_mode = CAMERA_FIRST_PERSON;
    arg->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  }

  if (IsKeyPressed(KEY_TWO)) {
    arg->camera_mode = CAMERA_THIRD_PERSON;
    arg->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  }

  Vector3 mouse_delta;
  control_camera(&arg->camera, &mouse_delta, &arg->camera_mode);

  place_stone_keyboard(arg);
  place_stone_mouse(&mouse_delta, arg);

  BeginDrawing();
  ClearBackground(GRAY);

  BeginMode3D(arg->camera);

  DrawModel(arg->torus, ORIGIN, 1, BEIGE);
  draw_inters(&arg->focused_stone, arg->intersections);

  for (int i = 0; i < N_INTERS; i++) {
    if (IS_ZERO(arg->stones[i]))
      continue;

    if (i % 2 == 0) {
      DrawModel(arg->black, arg->stones[i], 1, GRAY);
    } else {
      DrawModel(arg->white, arg->stones[i], 1, WHITE);
    }
  }

  if (arg->camera_mode == CAMERA_THIRD_PERSON) {
    DrawCube(arg->camera.target, 0.5f, 0.5f, 0.5f, PURPLE);
    DrawCubeWires(arg->camera.target, 0.5f, 0.5f, 0.5f, DARKPURPLE);
  }

  EndMode3D();

  DrawText(TextFormat("%.2f", 1.f / GetFrameTime()), 10, 10, 20, WHITE);

  EndDrawing();
}

int get_stone_idx(Vector3 *s, Vector3 *stones) {
  for (int i = 0; i < N_INTERS; i++) {
    if (ARE_EQUAL(stones[i], *s))
      return i;
  }
  return -1;
}

void pop_stone(int idx, int last_idx, Vector3 **sorted_inters) {
  if (idx < 0 || idx > last_idx)
    abort();

  if (last_idx == 0)
    return;

  (*sorted_inters)[idx] = (*sorted_inters)[last_idx];

  if ((N_INTERS - last_idx) % REALLOC_THRESHOLD == 0) {
    Vector3 *tmp_ptr =
        realloc(*sorted_inters, (last_idx + 1) * sizeof(Vector3));
    if (tmp_ptr) {
      printf("REALLOCATING, LENGTH IS NOW %d\n", last_idx + 1);
      *sorted_inters = tmp_ptr;
    } else {
      perror("Error reallocating memory");
      abort();
    }
  }
}

void place_stone_keyboard(MainLoopArg *arg) {
  if (IsKeyPressed(KEY_E) && IsKeyDown(KEY_LEFT_SHIFT)) {
    if (arg->count_from_closest > 0)
      arg->count_from_closest--;
  } else if (IsKeyPressed(KEY_E) && arg->count_from_closest < N_INTERS)
    arg->count_from_closest++;

  // make the user choose (later implement)
  if (is_moving())
    arg->count_from_closest = 0;

  arg->focused_stone = arg->sorted_inters[arg->count_from_closest];

  if (IsKeyPressed(KEY_ENTER)) {
    arg->stones[arg->count++] = arg->focused_stone;

    int last_idx = N_INTERS - arg->count - 1;
    pop_stone(arg->count_from_closest, last_idx, &arg->sorted_inters);

    arg->count_from_closest = 0;
  }
}

void place_stone_mouse(Vector3 *mouse_delta, MainLoopArg *arg) {
  if (!ARE_EQUAL(*mouse_delta, ORIGIN))
    return;

  Ray ray = GetScreenToWorldRay(GetMousePosition(), arg->camera);

  int new_len = N_INTERS - arg->count;
  for (int i = 0; i < new_len; i++) {
    RayCollision collision =
        GetRayCollisionSphere(ray, arg->sorted_inters[i], COLLISION_RADIUS);

    if (collision.hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      arg->stones[arg->count++] = arg->sorted_inters[i];
      pop_stone(i, new_len - 1, &arg->sorted_inters);
      break;
    }
  }
}

void compute_inters(Vector3 *intersections) {
  for (int i = 0; i < N_LINES; i++) {
    float theta = (float)i * UNIT_ANGLE;

    for (int j = i * N_LINES; j < (i + 1) * N_LINES; j++) {
      float phi = (float)j * UNIT_ANGLE;
      intersections[j] =
          (Vector3){(CENTER_RADIUS + TUBE_RADIUS * cosf(phi)) * cosf(theta),
                    (CENTER_RADIUS + TUBE_RADIUS * cosf(phi)) * sinf(theta),
                    TUBE_RADIUS * sinf(phi)};
    }
  }
}

void sort_inters(int new_len, Vector3 **to_sort_inters, Vector3 cam_pos) {
  float distances[new_len];
  for (int i = 0; i < new_len; i++) {
    distances[i] = dist(cam_pos, (*to_sort_inters)[i]);
  }

  for (int i = 1; i < new_len; i++) {
    Vector3 key_vec = (*to_sort_inters)[i];
    float key_dst = distances[i];
    int j = i - 1;

    while (j >= 0 && distances[j] > key_dst) {
      (*to_sort_inters)[j + 1] = (*to_sort_inters)[j];
      distances[j + 1] = distances[j];
      j--;
    }

    (*to_sort_inters)[j + 1] = key_vec;
    distances[j + 1] = key_dst;
  }
}

void draw_inters(Vector3 *focused_stone, Vector3 *intersections) {
  for (int i = 0; i < N_INTERS; i++) {
    Vector3 v = intersections[i];

    if (IS_ZERO(v))
      continue;

    Color color = RED;
    if (ARE_EQUAL(v, *focused_stone))
      color = GREEN;

    DrawCubeV(v, (Vector3){0.2f, 0.2f, 0.2f}, color);
    DrawCubeWiresV(v, (Vector3){0.2f, 0.2f, 0.2f}, BLACK);
  }
}
