#ifndef __GEOMETRY__H__
#define __GEOMETRY__H__

#include <stdint.h>
#include <math.h>

typedef struct __attribute__((packed)) {
	int16_t x;
	int16_t y;
} ScreenPoint;

typedef struct __attribute__((packed)) {
	float x;
	float y;
	float z;
} Vec3;

typedef struct __attribute__((packed)) {
    Vec3 pos;
    Vec3 forward;
    Vec3 up;
    Vec3 right;
} Camera;

ScreenPoint project(Vec3 v, Camera cam);

Camera camera_look_at(Vec3 pos, Vec3 target);

Vec3 rotate_x(Vec3 v, float angle_rad);

Vec3 rotate_y(Vec3 v, float angle_rad);

Vec3 rotate_z(Vec3 v, float angle_rad);

Vec3 rotate_xyz(Vec3 v, float rx, float ry, float rz);

float to_deg(float rad);

float to_rad(float deg);

void camera_rotate(Camera *cam, float dyaw, float dpitch);

Vec3 vec3_sub(Vec3 a, Vec3 b);

Vec3 vec3_add(Vec3 a, Vec3 b);

#endif