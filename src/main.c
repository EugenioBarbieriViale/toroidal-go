// https://www.raylib.com/examples/core/loader.html?name=core_3d_camera_first_person

#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define FPS 30
#define W 800
#define H 600

#define NORM_RADIUS 0.6f
#define SIZE 22.f

// #define TUBE_RADIUS (SIZE * (1.0f - NORM_RADIUS))
// #define INNER_RADIUS (SIZE * (2 * NORM_RADIUS - 1.0f))
// #define INNER_RADIUS (SIZE * (1.0f - NORM_RADIUS))
// #define OUTER_RADIUS TUBE_RADIUS + (TUBE_RADIUS - INNER_RADIUS) * 2
// #define OUTER_RADIUS (SIZE * (1.0f + 2 * NORM_RADIUS))

#define INNER_RADIUS (SIZE * (2 * NORM_RADIUS - 1.0f))
#define OUTER_RADIUS (2 * (CENTER_RADIUS - INNER_RADIUS) + INNER_RADIUS)
#define CENTER_RADIUS (SIZE * NORM_RADIUS)
#define TUBE_RADIUS (CENTER_RADIUS - INNER_RADIUS)

#define N_LINES 18
#define UNIT_ANGLE (2.0f * PI / (float)N_LINES)

#define STONE_OFFSET 0.5f
#define COLLISION_OFFSET 5.f

#define MOUSE_SPEED 4

const Vector3 ORIGIN = {0.f, 0.f, 0.f};

void make_move(RayCollision, Vector3[], int *);
void compute_inters(Vector3[]);

Vector3 radial_offset(Vector3 v, float offset) {
  float len = Vector3Length(v);
  Vector3 offset_v = Vector3Scale(v, offset / len);
  return Vector3Subtract(v, offset_v);
}

typedef struct {
  float r;     // radial distance
  float theta; // polar angle with respect to y
  float phi;   // azimuthal angle with respect to z
} PolarCoords;

// https://en.wikipedia.org/wiki/Spherical_coordinate_system#/media/File:3D_Spherical.svg
PolarCoords to_polar(Vector3 v) {
  PolarCoords p;

  p.r = Vector3Length(Vector3Subtract(ORIGIN, v));
  p.theta = atanf(sqrtf(v.x * v.x + v.z * v.z) / (v.y));
  p.phi = atanf(v.x / v.z);

  return p;
}

Vector3 to_cartesian(PolarCoords p, Vector3 origin) {
  Vector3 v;
  v.y = p.r * cosf(p.theta);
  v.x = p.r * sinf(p.theta) * sinf(p.phi);
  v.z = p.r * sinf(p.theta) * cosf(p.phi);

  return Vector3Add(v, origin);
}

int main() {
  InitWindow(W, H, "Toroidal Go");
  printf("TUBE: %f, INNER: %f, OUTER: %f\n", TUBE_RADIUS, INNER_RADIUS,
         OUTER_RADIUS);

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

  Vector3 stones[361];
  Vector3 intersections[361];
  compute_inters(intersections);

  SetTargetFPS(FPS);
  DisableCursor();

  int count = 0;
  float phi = PI / 2.f;
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

    ray = GetScreenToWorldRay(GetMousePosition(), camera);

    RayCollision collision =
        GetRayCollisionMesh(ray, torus.meshes[0], torus.transform);

    make_move(collision, stones, &count);

    BeginDrawing();
    ClearBackground(GRAY);

    BeginMode3D(camera);

    for (int i = 0; i < count; i++) {
      if (i % 2 == 0)
        DrawModel(black, stones[i], 1, BLACK);
      else
        DrawModel(white, stones[i], 1, WHITE);
    }

    DrawModel(torus, ORIGIN, 1, BEIGE);

    for (int i = 0; i < 361; i++) {
      Vector3 v = intersections[i];
      DrawLine3D(ORIGIN, v, GREEN);
      DrawCubeV(v, (Vector3){0.2f, 0.2f, 0.2f}, RED);
      DrawCubeWiresV(v, (Vector3){0.2f, 0.2f, 0.2f}, BLACK);
    }

    EndMode3D();
    EndDrawing();
  }

  UnloadModel(torus);
  CloseWindow();

  return 0;
}

void make_move(RayCollision collision, Vector3 stones[], int *count) {
  if (collision.hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector3 s = radial_offset(collision.normal, STONE_OFFSET);

    stones[*count] = s;
    (*count)++;

    printf("[%f, %f, %f]\n", collision.point.x, collision.point.y,
           collision.point.z);
  }
}

void compute_inters(Vector3 intersections[]) {
  for (int i = 0; i < 361; i++) {
    intersections[i] = ORIGIN;
  }

  PolarCoords p;
  p.r = INNER_RADIUS;
  p.phi = PI / 2.f;

  for (int l = 0; l < N_LINES; l++) {
    p.theta = l * UNIT_ANGLE - UNIT_ANGLE / 2.f;
    intersections[l] = to_cartesian(p, ORIGIN);
  }

  p.r = TUBE_RADIUS;
  p.phi = 0.f;

  for (int l = N_LINES; l < 2 * N_LINES; l++) {
    p.theta = l * UNIT_ANGLE - UNIT_ANGLE / 2.f;
    intersections[l] = to_cartesian(p, (Vector3){CENTER_RADIUS, 0.f, 0.f});
  }
}
