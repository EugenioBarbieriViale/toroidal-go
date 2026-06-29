// TODO
// - nearest intersection must also be in view field

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h> // remove

#define W 800
#define H 600
#define FPS 30

#define NORM_RADIUS 0.6f
#define SIZE 22.f

#define CENTER_RADIUS (SIZE / 2.0f)
#define TUBE_RADIUS (NORM_RADIUS * SIZE / 2.0f)

#define EPS (10e-3)

#define DIST(v, w) Vector3Length(Vector3Subtract(v, w))
#define IS_ZERO(v) (Vector3Length(v) < EPS)
#define IS_EQUALF(a, b) (fabs(a - b) < EPS)

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

#define STONE_OFFSET 0.5f

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

void print_board(Coord2 coords[]) {
  char grid[N_LINES][N_LINES + 1];
  for (int i = 0; i < N_LINES; i++) {
    memset(grid[i], ' ', N_LINES);
    grid[i][N_LINES] = '\0';
  }

  for (int i = 0; i < N_INTERS; i++) {
    int r = coords[i].row;
    int c = coords[i].col;
    if (c >= 0 && c < N_LINES && r >= 0 && r < N_LINES)
      grid[r][c] = '.';
  }

  for (int i = N_LINES - 1; i >= 0; i--)
    printf("%s\n", grid[i]);
}

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

  SetTargetFPS(FPS);
  DisableCursor();

  int count = 0;
  while (!WindowShouldClose()) {
    UpdateCameraPro(
        &camera,
        (Vector3){
            (!IsKeyDown(KEY_SPACE) && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))) *
                    0.2f - // Move forward-backward
                (!IsKeyDown(KEY_SPACE) &&
                 (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) *
                    0.2f,
            (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) *
                    0.2f - // Move right-left
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

    sort_inters(sorted_inters, camera.position);

    if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ENTER)) {
      stones[count] = sorted_inters[1];
      count++;
    } else if (IsKeyPressed(KEY_ENTER)) {
      stones[count] = sorted_inters[0];
      count++;
    }

    BeginDrawing();
    ClearBackground(GRAY);

    BeginMode3D(camera);

    DrawModel(torus, ORIGIN, 1, BEIGE);

    draw_inters(sorted_inters);

    for (int i = 0; i < N_INTERS; i++) {
      if (Vector3Length(stones[i]) < 10e-3)
        continue;

      if (i % 2 == 0)
        DrawModel(black, stones[i], 1, GRAY);
      else
        DrawModel(white, stones[i], 1, WHITE);
    }

    EndMode3D();
    EndDrawing();

    // temporary for visualization in 2d
    print_board(coords);
  }

  UnloadModel(torus);
  UnloadModel(black);
  UnloadModel(white);

  UnloadTexture(texture);
  CloseWindow();

  return 0;
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
