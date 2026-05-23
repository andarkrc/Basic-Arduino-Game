#include "geometry.h"

#define FOV 90.0f
#define DIST 10.0f

#define display_width 128
#define display_height 160

#define NEAR 0.1f
#define FOV_DEG 90.0f

/*
    Transparency disclaimer:
    most of this code is vibe coded cus i couldn't be bothered with doing the math.
*/

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

static Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (len < 0.0001f) return v;

    return (Vec3){v.x / len, v.y / len, v.z / len};
}

static Vec3 world_to_camera(Vec3 point, Camera cam) {
    Vec3 d = vec3_sub(point, cam.pos);
    return (Vec3){
        vec3_dot(d, cam.right),
        vec3_dot(d, cam.up),
        vec3_dot(d, cam.forward)
    };
}

ScreenPoint project(Vec3 world_pos, Camera cam) {
    Vec3 v = world_to_camera(world_pos, cam);

    // behind camera
    if (v.z < NEAR) {
        return (ScreenPoint){-1, -1};  // marker invalid
    }

    float f = (display_width / 2.0f);
    float aspect = (float)display_width / (float)display_height;

    ScreenPoint p;
    p.x = (int16_t)( v.x * f / v.z + display_width  / 2.0f);
    p.y = (int16_t)(-v.y * f * aspect / v.z + display_height / 2.0f);
    // -v.y pentru ca Y creste in jos pe ecran
    return p;
}

// --- Initializare camera ---
Camera camera_look_at(Vec3 pos, Vec3 target) {
    Camera cam;
    cam.pos     = pos;
    cam.forward = vec3_normalize(vec3_sub(target, pos));
    // world up = (0,1,0)
    Vec3 world_up = {0, 1, 0};
    cam.right   = vec3_normalize(vec3_cross(cam.forward, world_up));
    cam.up      = vec3_normalize(vec3_cross(cam.right, cam.forward));
    return cam;
}

// Rotatie pe axa X
Vec3 rotate_x(Vec3 v, float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    return (Vec3){
        v.x,
        v.y * c - v.z * s,
        v.y * s + v.z * c
    };
}

// Rotatie pe axa Y
Vec3 rotate_y(Vec3 v, float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    return (Vec3){
        v.x * c + v.z * s,
        v.y,
       -v.x * s + v.z * c
    };
}

// Rotatie pe axa Z
Vec3 rotate_z(Vec3 v, float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    return (Vec3){
        v.x * c - v.y * s,
        v.x * s + v.y * c,
        v.z
    };
}

// Rotatie pe toate 3 axele deodata (pitch, yaw, roll)
Vec3 rotate_xyz(Vec3 v, float rx, float ry, float rz) {
    v = rotate_x(v, rx);
    v = rotate_y(v, ry);
    v = rotate_z(v, rz);
    return v;
}

float to_rad(float deg)
{
    return deg * 3.14159f / 180;
}

float to_deg(float rad)
{
    return rad * 180 / 3.14159f;
}

void camera_rotate(Camera *cam, float dyaw, float dpitch)
{
    // Rotatie yaw in jurul axei Y globale (0,1,0) - mai stabil decat up local
    Vec3 world_up = {0, 1, 0};

    // --- YAW (in jurul Y global) ---
    float cy = cosf(dyaw);
    float sy = sinf(dyaw);

    Vec3 new_forward;
    new_forward.x = cy * cam->forward.x + sy * cam->forward.z;
    new_forward.y = cam->forward.y;
    new_forward.z = -sy * cam->forward.x + cy * cam->forward.z;
    cam->forward = vec3_normalize(new_forward);

    // --- PITCH (in jurul right vector local) ---
    float cp = cosf(dpitch);
    float sp = sinf(dpitch);

    // Rodrigues: v' = v*cos + (r x v)*sin + r*(r·v)*(1-cos)
    Vec3 r = cam->right;
    Vec3 f = cam->forward;
    float dot = r.x*f.x + r.y*f.y + r.z*f.z;
    Vec3 cross = vec3_cross(r, f);

    new_forward.x = f.x*cp + cross.x*sp + r.x*dot*(1.f - cp);
    new_forward.y = f.y*cp + cross.y*sp + r.y*dot*(1.f - cp);
    new_forward.z = f.z*cp + cross.z*sp + r.z*dot*(1.f - cp);
    cam->forward = vec3_normalize(new_forward);

    // --- Recalculeaza right si up ---
    cam->right = vec3_normalize(vec3_cross(cam->forward, world_up));
    cam->up    = vec3_normalize(vec3_cross(cam->right, cam->forward));
}