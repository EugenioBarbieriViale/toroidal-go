// TODO
// - nearest intersection must also be in view field
// - iterate through sorted intersections

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

#define EPS (10e-3)

#define DIST(v, w) Vector3Length(Vector3Subtract(v, w))
#define IS_ZERO(v) (Vector3Length(v) < (float)EPS)
#define IS_EQUALF(a, b) (fabs(a - b) < EPS)

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

const Vector3 ORIGIN = {0.f, 0.f, 0.f};

typedef struct {
  Camera camera;

  Model torus;
  Model black;
  Model white;

  Vector3 *stones;
  Vector3 *intersections;
  Vector3 *sorted_inters;

  int sorted_count;
  int stone_skips;
  int count;
} MainLoopArg;

void compute_inters(Vector3[]);
void sort_inters(Vector3[], Vector3);
void draw_inters(int, Vector3[]);

int Vector3cmp(Vector3 *, Vector3 *);
int get_stone_idx(Vector3 *, Vector3[]);

void place_stone(MainLoopArg *);
void UpdateDrawFrame(void *);

int main() {
  InitWindow(W, H, "Toroidal Go");
  DisableCursor();

  Camera camera = {0};
  camera.position = (Vector3){10.f, 10.f, 10.f};
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

  Vector3 sorted_inters[N_INTERS];
  compute_inters(sorted_inters);

  MainLoopArg *main_loop_arg = (MainLoopArg *)malloc(sizeof(MainLoopArg));
  main_loop_arg->camera = camera;

  main_loop_arg->torus = torus;
  main_loop_arg->black = black;
  main_loop_arg->white = white;

  main_loop_arg->stones = stones;
  main_loop_arg->intersections = intersections;
  main_loop_arg->sorted_inters = sorted_inters;

  main_loop_arg->stone_skips = 0;
  main_loop_arg->sorted_count = 0;
  main_loop_arg->count = 0;

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

  free(main_loop_arg);
  CloseWindow();

  return 0;
}

void UpdateDrawFrame(void *arg_) {
  MainLoopArg *arg = arg_;

  UpdateCameraPro(
      &arg->camera,
      (Vector3){
          (!IsKeyDown(KEY_SPACE) && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))) *
                  0.2f - // Move forward-backward
              (!IsKeyDown(KEY_SPACE) &&
               (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) *
                  0.2f,
          (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * 0.2f - // Move right-left
              (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * 0.2f,
          (IsKeyDown(KEY_SPACE) && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))) *
                  0.2f - // Move up-down
              (IsKeyDown(KEY_SPACE) &&
               (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) *
                  0.2f},
      (Vector3){
          GetMouseDelta().x * 0.05f, // Rotation: yaw
          GetMouseDelta().y * 0.05f, // Rotation: pitch
          0.0f                       // Rotation: roll
      },
      GetMouseWheelMove() * 2.0f);

  sort_inters(arg->sorted_inters, arg->camera.position);
  place_stone(arg);

  BeginDrawing();
  ClearBackground(GRAY);

  BeginMode3D(arg->camera);

  DrawModel(arg->torus, ORIGIN, 1, BEIGE);

  draw_inters(arg->sorted_count, arg->sorted_inters);

  for (int i = 0; i < N_INTERS; i++) {
    if (IS_ZERO(arg->stones[i]))
      continue;

    if (i % 2 == 0) {
      DrawModel(arg->black, arg->stones[i], 1, GRAY);
    } else {
      DrawModel(arg->white, arg->stones[i], 1, WHITE);
    }
  }

  EndMode3D();
  EndDrawing();
}

int Vector3cmp(Vector3 *v, Vector3 *w) {
  return (IS_EQUALF(v->x, w->x) && IS_EQUALF(v->y, w->y) &&
          IS_EQUALF(v->z, w->z));
}

int get_stone_idx(Vector3 *s, Vector3 stones[]) {
  for (int i = 0; i < N_INTERS; i++) {
    if (Vector3cmp(&stones[i], s))
      return i;
  }
  return -1;
}

void place_stone(MainLoopArg *arg) {
  // check if another stone has already been placed in that position
  arg->sorted_count = 0;
  while (get_stone_idx(&arg->sorted_inters[arg->sorted_count], arg->stones) !=
         -1) {
    if (arg->sorted_count >= N_INTERS)
      break;
    arg->sorted_count++;
  }

  if (IsKeyPressed(KEY_E) && IsKeyDown(KEY_LEFT_SHIFT))
    arg->sorted_count--;
  else if (IsKeyPressed(KEY_E))
    arg->sorted_count++;

  if (IsKeyPressed(KEY_ENTER)) {
    arg->stones[arg->count++] = arg->sorted_inters[arg->sorted_count];
    arg->sorted_count = 0;
  }
}

void compute_inters(Vector3 intersections[]) {
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

void sort_inters(Vector3 intersections[], Vector3 cam_pos) {
  float distances[N_INTERS];
  for (int i = 0; i < N_INTERS; i++) {
    distances[i] = DIST(cam_pos, intersections[i]);
  }

  for (int i = 1; i < N_INTERS; i++) {
    Vector3 key_vec = intersections[i];
    float key_dst = distances[i];
    int j = i - 1;

    while (j >= 0 && distances[j] > key_dst) {
      intersections[j + 1] = intersections[j];
      distances[j + 1] = distances[j];
      j--;
    }

    intersections[j + 1] = key_vec;
    distances[j + 1] = key_dst;
  }
}

void draw_inters(int sorted_count, Vector3 sorted_inters[]) {
  for (int i = 0; i < N_INTERS; i++) {
    Vector3 v = sorted_inters[i];

    if (IS_ZERO(v))
      continue;

    Color color = RED;
    if (i == sorted_count)
      color = GREEN;

    DrawCubeV(v, (Vector3){0.2f, 0.2f, 0.2f}, color);
    DrawCubeWiresV(v, (Vector3){0.2f, 0.2f, 0.2f}, BLACK);
  }
}
