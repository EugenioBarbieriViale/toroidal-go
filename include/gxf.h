#ifndef GXF_H
#define GXF_H

#include "rules.h"

#if defined(PLATFORM_WEB)
#include "./external/raylib/src/raylib.h"
#include "./external/raylib/src/raymath.h"
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

#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

#define SBLACK 1
#define SWHITE 2
#define EMPTY 0
#define UNDEF 3

#define COLLISION_RADIUS (0.2f)
#define ORIGIN ((Vector3){0.f, 0.f, 0.f})

typedef struct {
  Vector3 pos;
  int idx;
} AvailablePoint;

typedef struct {
  Camera camera;

  Model torus;
  Model black;
  Model white;

  Font font;

  Vector3 intersections[BOARD_SIZE];
  BoardState *bs;

  AvailablePoint available_points[BOARD_SIZE];
  Vector3 focused_stone;
  int count;

  int camera_mode;
  Color camera_color;
} MainLoopArg;

void show_welcome_screen(Font);

MainLoopArg *gxf_init(int);
Vector3 gxf_update(MainLoopArg *);
void gxf_draw(MainLoopArg *);
void gxf_cleanup(MainLoopArg *);

int try_place_stone(Vector3, MainLoopArg *);
void update_available(MainLoopArg *);

#endif
