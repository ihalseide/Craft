#ifndef _hitbox_h
#define _hitbox_h


// Axis Aligned Bounding Box
typedef struct {
    float x;   // center x
    float y;   // center y
    float z;   // center z
    float ex;  // extent x
    float ey;  // extent y
    float ez;  // extent z
} Box;


void box_nearest_blocks(
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        int *x0,
        int *y0,
        int *z0,
        int *x1,
        int *y1, int *z1);

float box_sweep_box(
        float ax,
        float ay,
        float az,
        float aex,
        float aey,
        float aez,
        float bx,
        float by,
        float bz,
        float bex,
        float bey, float bez, float vx, float vy, float vz, float *nx, float *ny, float *nz);

float box_sweep_block(
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        int bx,
        int by,
        int bz,
        float vx,
        float vy, float vz, float *nx, float *ny, float *nz);

void box_broadphase(
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        float vx,
        float vy,
        float vz,
        float *bx,
        float *by, float *bz, float *bex, float *bey, float *bez);

int box_intersect_box(
        float ax,
        float ay,
        float az,
        float aex,
        float aey,
        float aez,
        float bx,
        float by,
        float bz,
        float bex,
        float bey, float bez);

int box_intersect_block(
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        int bx,
        int by,
        int bz);


#endif /* _hitbox_h */

