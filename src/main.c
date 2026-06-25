// TODO
// - nearest intersection must also be in view field

#include <raylib.h>
#include <raymath.h>

#define FPS 30
#define W 800
#define H 600

#define NORM_RADIUS 0.6f
#define SIZE 22.f

#define CENTER_RADIUS (SIZE / 2.0f)
#define TUBE_RADIUS (NORM_RADIUS * SIZE / 2.0f)
#define INNER_RADIUS (CENTER_RADIUS - TUBE_RADIUS)
#define OUTER_RADIUS (CENTER_RADIUS + TUBE_RADIUS)

#define DIST(v, w) Vector3Length(Vector3Subtract(v, w))

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

#define STONE_OFFSET 0.5f

const Vector3 ORIGIN = {0.f, 0.f, 0.f};

void compute_inters(Vector3[]);
void sort_inters(Vector3[], Vector3);
void draw_inters(Vector3[]);

Vector3 radial_offset(Vector3 v, float offset) {
  float len = Vector3Length(v);
  Vector3 offset_v = Vector3Scale(v, offset / len);
  return Vector3Subtract(v, offset_v);
}

int main() {
  InitWindow(W, H, "Toroidal Go");

  Camera camera = {0};
  camera.position = (Vector3){10.f, 10.f, 10.f};
  camera.target = ORIGIN;
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  Ray ray = {0};

  Model torus = LoadModelFromMesh(GenMeshTorus(NORM_RADIUS, SIZE, 32, 64));
  Model black = LoadModelFromMesh(GenMeshSphere(1, 32, 64));
  Model white = LoadModelFromMesh(GenMeshSphere(1, 32, 64));

  Texture2D texture = LoadTexture("./assets/board.png");
  torus.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  Vector3 stones[N_INTERS];
  for (int i = 0; i < N_INTERS; i++)
    stones[i] = ORIGIN;

  Vector3 intersections[N_INTERS];
  compute_inters(intersections);

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

    // ray = GetScreenToWorldRay(GetMousePosition(), camera);
    //
    // RayCollision collision =
    //     GetRayCollisionMesh(ray, torus.meshes[0], torus.transform);

    if (IsKeyPressed(KEY_ENTER)) {
      stones[count] = intersections[0];
      count++;
    }

    BeginDrawing();
    ClearBackground(GRAY);

    BeginMode3D(camera);

    DrawModel(torus, ORIGIN, 1, BEIGE);

    sort_inters(intersections, camera.position);
    draw_inters(intersections);

    for (int i = 0; i < N_INTERS; i++) {
      if (Vector3Length(stones[i]) == 0)
        continue;

      if (i % 2 == 0)
        DrawModel(black, stones[i], 1, BLACK);
      else
        DrawModel(white, stones[i], 1, WHITE);
    }

    EndMode3D();
    EndDrawing();
  }

  UnloadModel(torus);
  CloseWindow();

  return 0;
}

void compute_inters(Vector3 intersections[]) {
  for (int i = 0; i < N_LINES; i++) {
    float theta = i * UNIT_ANGLE;

    for (int j = i * N_LINES; j < (i + 1) * N_LINES; j++) {
      float phi = j * UNIT_ANGLE;
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

void draw_inters(Vector3 intersections[]) {
  for (int i = 0; i < N_INTERS; i++) {
    Vector3 v = intersections[i];

    if (Vector3Length(v) == 0)
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
