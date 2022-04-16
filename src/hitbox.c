#include <assert.h>
#include <math.h>
#include <limits.h>
#include "util.h"
#include "hitbox.h"

// Respond to a collision from swept collision.
// Allow the caller to apply the resulting velocity that was just written
// out as they see fit.
// Arguments:
// - t: time (0.0 to 1.0)
// - nx, ny, nz:
// - x, y, z: input and output position vector
// - vx, vy, vz: input and output velocity vector, (output is not multiplied by
//   the remaining time)
// Returns: none
void handle_collision(
        float t, float nx, float ny, float nz, float *x, float *y, float *z,
        float *vx, float *vy, float *vz)
{
    // Move up to the collision point
    *x += (*vx) * t;
    *y += (*vy) * t;
    *z += (*vz) * t;
    // remaining time
    float rt = 1.0 - t;
    // Respond by modifying the velocity vector
    if (nx != 0)
    {
        // In x direction
        *vx = 0;
        *vy = *vz * rt;
        *vz = *vy * rt;
    }
    else if (ny != 0)
    {
        // In y direction
        *vy = 0;
        *vx = *vz * rt;
        *vz = *vx * rt;
    }
    else if (nz != 0)
    {
        // In z direction
        *vz = 0;
        *vx = *vy * rt;
        *vy = *vx * rt;
    }
}

// Round bounding box to nearest block position start and end
void box_nearest_blocks(
        float x, float y, float z, float ex, float ey, float ez,
        int *x0, int *y0, int *z0, int *x1, int *y1, int *z1)
{
    *x0 = (int)floorf(x - ex);
    *y0 = (int)floorf(y - ey);
    *z0 = (int)floorf(z - ez);
    *x1 = (int)ceilf(x + ex);
    *y1 = (int)ceilf(y + ey);
    *z1 = (int)ceilf(z + ez);
}

// box_intersect_box()
void test_box_intersect_box(void)
{
    assert(box_intersect_box(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1));
    assert(box_intersect_box(0, 0, 0, 2, 0.5, 0.5, 1, 0, 0, 0.5, 10, 10));
    assert(box_intersect_box(0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1));
    assert(!box_intersect_box(10, 10, 10, 1, 1, 1, 1, 1, 1, 1, 1, 1));
}

// Swept collision of a moving box A with a static box B.
// Arguments:
// - ax: x position for box A
// - ay: y "
// - az: z "
// - aex: x extent of hitbox for box A
// - aey: y "
// - aez: z "
// - bx: x position for box B
// - by: y "
// - bz: z "
// - bex: x extent of hitbox for box B
// - bey: y "
// - bez: z "
// - vx: velocity x of box A
// - vy: "        y
// - vz: "        z
// - nx: pointer to normal x to output to
// - ny: "                 y
// - nz: "                 z
// Returns:
// - returns the normal vector of the box face that was collided with
// - value between 0.0 and 1.0 to indicate the collision time
//   (1.0 means no collision)
float box_sweep_box(
        float ax, float ay, float az, float aex, float aey, float aez,
        float bx, float by, float bz, float bex, float bey, float bez,
        float vx, float vy, float vz, float *nx, float *ny, float *nz)
{
    // Default normals
    *nx = 0;
    *ny = 0;
    *nz = 0;
    // No velocity -> no collision
    if (vx == 0 && vy == 0 && vz == 0)
    {
        return 1.0;
    }

    // xc is the x distance between the closest edges.
    // xf is the x distance between the furthest edges.
    float xc, xf;
    if (vx > 0.0)
    {
        xc = (bx - bex) - (ax + aex);
        xf = (bx + bex) - (ax - aex);
    }
    else
    {
        xc = (bx + bex) - (ax - aex);
        xf = (bx - bex) - (ax + aex);
    }

    // yc is the y distance between the closest edges.
    // yf is the y distance between the furthest edges.
    float yc, yf;
    if (vy > 0)
    {
        yc = (by - bey) - (ay + aey);
        yf = (by + bey) - (ay - aey);
    }
    else
    {
        yc = (by + bey) - (ay - aey);
        yf = (by - bey) - (ay + aey);
    }

    // zc is the z distance between the closest edges.
    // zf is the z distance between the furthest edges.
    float zc, zf;
    if (vz > 0)
    {
        zc = (bz - bez) - (az + aez);
        zf = (bz + bez) - (az - aez);
    }
    else
    {
        zc = (bz + bez) - (az - aez);
        zf = (bz - bez) - (az + aez);
    }

    // x entry time and x exit time
    float xEnterT, xExitT;
    if (vx == 0)
    {
        xEnterT = -INFINITY;
        xExitT = INFINITY;
    }
    else
    {
        xEnterT = xc / vx;
        xExitT = xf / vx;
    }

    // y entry time and y exit time
    float yEnterT, yExitT;
    if (vy == 0)
    {
        yEnterT = -INFINITY;
        yExitT = INFINITY;
    }
    else
    {
        yEnterT = yc / vy;
        yExitT = yf / vy;
    }

    // z entry time and z exit time
    float zEnterT, zExitT;
    if (vz == 0)
    {
        zEnterT = -INFINITY;
        zExitT = INFINITY;
    }
    else
    {
        zEnterT = zc / vz;
        zExitT = zf / vz;
    }

    // Find the min and max times
    float enterT, exitT;
    enterT = MAX(xEnterT, yEnterT);
    enterT = MAX(enterT, zEnterT);
    exitT = MIN(xExitT, yExitT);
    exitT = MIN(exitT, zExitT);
    if ((enterT > exitT)
            || (xEnterT < 0 && yEnterT < 0 && zEnterT < 0) 
            || (xEnterT > 1) 
            || (yEnterT > 1) 
            || (zEnterT > 1))
    {
        // No collision
        return 1.0;
    }

    // Calculate the normal of the collided surface
    if (xEnterT > yEnterT && xEnterT > zEnterT)
    {
        // x is closest
        *ny = 0;
        *nz = 0;
        if (xc < 0)
        {
            *nx = 1;
        }
        else
        {
            *nx = -1;
        }
    }
    else if (yEnterT > zEnterT)
    {
        // y is closest
        *nx = 0;
        *nz = 0;
        if (yc < 0)
        {
            *ny = 1;
        }
        else
        {
            *ny = -1;
        }
    }
    else
    {
        // z is closest
        *nx = 0;
        *ny = 0;
        if (zc < 0)
        {
            *nz = 1;
        }
        else
        {
            *nz = -1;
        }
    }

    // Collision time is the entry time
    return enterT;
}

// Get the swept collision info for a moving box intersecting a block in the
// world.
float box_sweep_block(
        float x, float y, float z, float ex, float ey, float ez,
        int bx, int by, int bz, float vx, float vy, float vz,
        float *nx, float *ny, float *nz)
{
    // n is the extent of a unit cube in all dimensions.
    const float n = 0.5;
    return box_sweep_box(
        x, y, z, ex, ey, ez, bx, by, bz, n, n, n, vx, vy, vz, nx, ny, nz);
}

// Arguments:
// - x: input center x
// - y: input center y
// - z: input center z
// - ex: input extent x
// - ey: input extent y
// - ez: input extent z
// - vx: velocity x
// - vy: velocity y
// - vz: velocity z
// - bx: output box center x
// - by: output box center y
// - bz: output box center z
// - bex: output box extent x
// - bey: output box extent y
// - bez: output box extent z
// Returns:
// - modifies bx, by, bz, bex, bey, and bez
void box_broadphase(
        float x, float y, float z, float ex, float ey, float ez,
        float vx, float vy, float vz, float *bx, float *by, float *bz,
        float *bex, float *bey, float *bez)
{
    *bx = x + vx/2;
    *by = y + vy/2;
    *bz = z + vz/2;
    vx = ABS(vx);
    vy = ABS(vy);
    vz = ABS(vz);
    *bex = ex + vx/2;
    *bey = ey + vy/2;
    *bez = ez + vz/2;
}

// Check if two axis-aligned-bounding-boxes, A and B, intersect.
// Arguments:
// - ax: bounding box A's center x position
// - ay: bounding box A's center y position
// - az: bounding box A's center z position
// - aex: bounding box A's x extent
// - aey: bounding box A's y extent
// - aez: bounding box B's z extent
// - bx: bounding box B's center x position
// - by: bounding box B's center y position
// - bz: bounding box B's center z position
// - bex: bounding box B's x extent
// - bey: bounding box B's y extent
// - bez: bounding box B's z extent
// Returns:
// - non-zero if they intersect
int box_intersect_box(
        float ax, float ay, float az, float aex, float aey, float aez,
        float bx, float by, float bz, float bex, float bey, float bez)
{
    return !((ax + aex < bx - bex) || (ax - aex > bx + bex) ||
             (ay + aey < by - bey) || (ay - aey > by + bey) ||
             (az + aez < bz - bez) || (az - aez > bz + bez));
}

// Return whether a bounding box interects a block
int box_intersect_block(
        float x, float y, float z, float ex, float ey, float ez,
        int bx, int by, int bz)
{
    // n is the extent of a unit cube in all dimensions.
    const float n = 0.5;
    return box_intersect_box(x, y, z, ex, ey, ez, bx, by, bz, n, n, n);
}

