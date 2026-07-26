#include "gxf.h"
#include <stdlib.h>
#include <time.h>

void control_camera(Camera *, Vector3 *, int *);

int get_stone_idx(Vector3 *, Vector3 *);
void pop_stone(int, int, Vector3 **);

void place_stone_keyboard(MainLoopArg *);
void place_stone_mouse(Vector3 *, MainLoopArg *);

void compute_inters(Vector3 *);
void sort_inters(int, Vector3 **, Vector3);
void draw_inters(Vector3 *, Vector3 *);

const float COLLISION_RADIUS = 0.2f;
const Vector3 ORIGIN = {0.f, 0.f, 0.f};

static inline int is_moving(void) {
  return (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) ||
          IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) ||
          IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT));
}

static inline Color get_rnd_color(void) {
  Color colors[9] = {BLACK, YELLOW, RED,    PURPLE, BLUE,
                     GREEN, BROWN,  ORANGE, PINK};
  int ci = rand() % 9;
  return colors[ci];
}

MainLoopArg *gxf_init() {
  MainLoopArg *arg = (MainLoopArg *)malloc(sizeof(MainLoopArg));

  srand(time(NULL));

  InitWindow(W, H, "Toroidal Go");
  DisableCursor();

  arg->camera = (Camera){0};
  arg->camera.position = (Vector3){0.f, 0.f, 25.f};
  arg->camera.target = ORIGIN;
  arg->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  arg->camera.fovy = 45.0f;
  arg->camera.projection = CAMERA_PERSPECTIVE;

  arg->torus = LoadModelFromMesh(GenMeshTorus(NORM_RADIUS, SIZE, 32, 64));
  arg->black = LoadModelFromMesh(GenMeshSphere(1, 32, 64));
  arg->white = LoadModelFromMesh(GenMeshSphere(1, 32, 64));

  Texture2D texture = LoadTexture("./assets/board.png");
  arg->torus.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/black_stone.png");
  arg->black.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  texture = LoadTexture("./assets/white_marble.png");
  arg->white.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  for (int i = 0; i < N_INTERS; i++) {
    arg->stones[i] = ORIGIN;
    arg->board_state[i] = EMPTY;
  }

  compute_inters(arg->intersections);

  arg->sorted_inters = (Vector3 *)malloc(N_INTERS * sizeof(Vector3));
  compute_inters(arg->sorted_inters);

  arg->focused_stone = arg->sorted_inters[0];
  arg->count_from_closest = 0;
  arg->count = 0;

  arg->camera_mode = CAMERA_FIRST_PERSON;
  arg->camera_color = get_rnd_color();

  return arg;
}

void gxf_update(MainLoopArg *arg) {
  int new_len = N_INTERS - arg->count;
  if (arg->camera_mode == CAMERA_THIRD_PERSON)
    sort_inters(new_len, &arg->sorted_inters, arg->camera.target);
  else
    sort_inters(new_len, &arg->sorted_inters, arg->camera.position);

  Vector3 mouse_delta;
  control_camera(&arg->camera, &mouse_delta, &arg->camera_mode);

  place_stone_keyboard(arg);
  place_stone_mouse(&mouse_delta, arg);
}

void gxf_draw(MainLoopArg *arg) {
  BeginDrawing();
  ClearBackground(GRAY);

  BeginMode3D(arg->camera);

  DrawModel(arg->torus, (Vector3){0.f, 0.f, 0.f}, 1, BEIGE);
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
    DrawCube(arg->camera.target, 1.0f, 1.0f, 1.0f, arg->camera_color);
    DrawCubeWires(arg->camera.target, 1.0f, 1.0f, 1.0f, BLACK);
  }

  EndMode3D();

  DrawText(TextFormat("%.2f", 1.f / GetFrameTime()), 10, 10, 20, WHITE);

  EndDrawing();
}

void gxf_cleanup(MainLoopArg *arg) {
  UnloadModel(arg->torus);
  UnloadModel(arg->black);
  UnloadModel(arg->white);

  free(arg->sorted_inters);
  free(arg);

  CloseWindow();
}

void control_camera(Camera *camera, Vector3 *mouse_delta, int *camera_mode) {
  if (IsKeyPressed(KEY_ONE)) {
    *camera_mode = CAMERA_FIRST_PERSON;
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
  }

  if (IsKeyPressed(KEY_TWO)) {
    *camera_mode = CAMERA_THIRD_PERSON;
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
  }

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
      // printf("REALLOCATING, LENGTH IS NOW %d\n", last_idx + 1);
      *sorted_inters = tmp_ptr;
    } else {
      // perror("Error reallocating memory");
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
    arg->board_state[arg->count_from_closest] = 1;
    // send to server game state, if return is positive make let place stone
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
      arg->board_state[i] = 1;
      // send to server game state, if return is positive make let place stone
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
