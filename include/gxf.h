#ifndef GXF_H
#define GXF_H

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

#define N_LINES 18
#define N_INTERS (N_LINES * N_LINES)
#define UNIT_ANGLE (2.f * PI / (float)N_LINES)

#define SBLACK 1
#define SWHITE 2
#define EMPTY 0
#define UNDEF 3

typedef struct {
  Camera camera;

  Model torus;
  Model black;
  Model white;

  Vector3 stones[N_INTERS];
  int board_state[N_INTERS];

  Vector3 intersections[N_INTERS];
  Vector3 *sorted_inters;

  Vector3 focused_stone;
  int count_from_closest;
  int count;

  int camera_mode;
  Color camera_color;
} MainLoopArg;

MainLoopArg *gxf_init(void);
void gxf_update(MainLoopArg *);
void gxf_draw(MainLoopArg *);
void gxf_cleanup(MainLoopArg *);

#endif
