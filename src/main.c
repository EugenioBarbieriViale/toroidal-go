// TODO
// - nearest intersection must also be in view field

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

#define EPS (10e-3)

#define DIST(v, w) Vector3Length(Vector3Subtract(v, w))
#define IS_ZERO(v) (Vector3Length(v) < (float)EPS)
#define IS_EQUALF(a, b) (fabs(a - b) < EPS)

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

const Vector3 ORIGIN = {0.f, 0.f, 0.f};

void compute_inters(Vector3[]);
void sort_inters(Vector3[], Vector3);
void draw_inters(Vector3[]);

typedef struct {
  int row;
  int col;
} Coord2;

void get_board_coords(Coord2[]);

typedef struct {
  Vector3 key;
  Coord2 value;
} HashItem;

void install_map(HashItem[], Vector3[], Coord2[]);
Coord2 *lookup(HashItem[], Vector3);

void UpdateDrawFrame(void *);

typedef struct {
  Camera camera;
  Model torus;
  Model black;
  Model white;
  Vector3 *stones;
  Vector3 *sorted_inters;
  Coord2 *coords;
  HashItem *hashmap;
  int count;
} MainLoopArg;

int main() {
  InitWindow(W, H, "Toroidal Go");

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

  // hmmmmm
  Vector3 sorted_inters[N_INTERS];
  compute_inters(sorted_inters);

  Coord2 coords[N_INTERS];
  get_board_coords(coords);

  HashItem hashmap[N_INTERS];
  install_map(hashmap, intersections, coords);

  for (int i = 0; i < N_INTERS; i++) {
    if (lookup(hashmap, intersections[i]) == NULL)
      printf("NO CORRESPONDING KEY OF ELEMENT %d\n", i);
  }

  MainLoopArg *main_loop_arg = (MainLoopArg *)malloc(sizeof(MainLoopArg));
  main_loop_arg->camera = camera;
  main_loop_arg->torus = torus;
  main_loop_arg->black = black;
  main_loop_arg->white = white;
  main_loop_arg->stones = stones;
  main_loop_arg->sorted_inters = sorted_inters;
  main_loop_arg->coords = coords;
  main_loop_arg->hashmap = hashmap;
  main_loop_arg->count = 0;

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop_arg(UpdateDrawFrame, main_loop_arg, 0, 1);
#else
  SetTargetFPS(FPS);
  DisableCursor();

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
#if defined(PLATFORM_WEB)
  DisableCursor();
#endif

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

  if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ENTER)) {
    arg->stones[arg->count] = arg->sorted_inters[1];
    arg->count++;
  } else if (IsKeyPressed(KEY_ENTER)) {
    arg->stones[arg->count] = arg->sorted_inters[0];
    arg->count++;
  }

  BeginDrawing();
  ClearBackground(GRAY);

  BeginMode3D(arg->camera);

  DrawModel(arg->torus, ORIGIN, 1, BEIGE);

  draw_inters(arg->sorted_inters);

  for (int i = 0; i < N_INTERS; i++) {
    if (IS_ZERO(arg->stones[i]))
      continue;

    Coord2 c = *lookup(arg->hashmap, arg->stones[i]);

    if (i % 2 == 0) {
      DrawModel(arg->black, arg->stones[i], 1, GRAY);
    } else {
      DrawModel(arg->white, arg->stones[i], 1, WHITE);
    }
  }

  EndMode3D();
  EndDrawing();
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

void draw_inters(Vector3 sorted_inters[]) {
  for (int i = 0; i < N_INTERS; i++) {
    Vector3 v = sorted_inters[i];

    if (IS_ZERO(v))
      continue;

    Color color = RED;
    if (i == 0)
      color = GREEN;
    else if (i == 1)
      color = YELLOW;

    DrawCubeV(v, (Vector3){0.2f, 0.2f, 0.2f}, color);
    DrawCubeWiresV(v, (Vector3){0.2f, 0.2f, 0.2f}, BLACK);
  }
}

void get_board_coords(Coord2 coords[]) {
  for (int i = 0; i < N_INTERS; i++) {
    coords[i].row = (int)(i / N_LINES);
    coords[i].col = i % N_LINES;
  }
}

void install_map(HashItem hashmap[], Vector3 intersections[], Coord2 coords[]) {
  HashItem h;
  for (int i = 0; i < N_INTERS; i++) {
    h.key = intersections[i];
    h.value = coords[i];
    hashmap[i] = h;
  }
}

int Vector3cmp(Vector3 v, Vector3 w) {
  return (IS_EQUALF(v.x, w.x) && IS_EQUALF(v.y, w.y) && IS_EQUALF(v.z, w.z));
}

Coord2 *lookup(HashItem hashmap[], Vector3 key) {
  for (int i = 0; i < N_INTERS; i++) {
    if (Vector3cmp(hashmap[i].key, key) == 0) {
      return &hashmap[i].value;
    }
  }
  return NULL;
}
