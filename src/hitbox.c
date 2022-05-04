#include <assert.h>
#include <math.h>
#include <limits.h>
#include "util.h"
#include "hitbox.h"

// Extent of a unit cube
#define CUBE_EXTENT 0.5

/* Collision functions for boxes.
 * A "box" is a 3-dimensional axis-aligned bounding box, which is described by
 * two 3-vectors: a center and an extent. The center is a point, and the extent
 * is the distance from the center any of the 8 corners of the box. Each
 * separate extent value represents the distance from the center to an edge
 * along the corresponding axis.
 */

// Round bounding box to nearest block position start and end
// Arguments:
// - x, y, z: box center
// - ex, ey, ez: box extent
// - x0, y0, z0: minimum block position output
// - x1, y1, z1: maximum block position output
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

// Swept collision of a moving box A with a static box B.
// Arguments:
// - ax, ay, az: center position for box A
// - aex, aey, aez: extent of box A
// - bx, by, bz: center position for box B
// - bex, bey, bez: extent of box B
// - vx, vy, vz: velocity of box A
// - nx, ny, nz: normal vector output
// Returns:
// - returns the normal vector direction of the box face that was collided with
// - value between 0.0 and 1.0 to indicate the collision time
//   (1.0 means no collision)
float box_sweep_box(
        float ax, float ay, float az, float aex, float aey, float aez,
        float bx, float by, float bz, float bex, float bey, float bez,
        float vx, float vy, float vz,
        float *nx, float *ny, float *nz)
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

// Get the swept collision info for a moving box intersecting a block
// Arguments:
// - x, y, z: box center
// - ex, ey, ez: box extents
// - bx, by, bz: block center
// - vx, vy, vz: box velocity
// - nx, ny, nz: collision normal output
// Returns:
// - collision time between 0.0 and 1.0 (1.0 means no collision)
float box_sweep_block(
        float x, float y, float z, float ex, float ey, float ez,
        int bx, int by, int bz, float vx, float vy, float vz,
        float *nx, float *ny, float *nz)
{
    const float n = CUBE_EXTENT;
    return box_sweep_box(
        x, y, z, ex, ey, ez, bx, by, bz, n, n, n, vx, vy, vz, nx, ny, nz);
}

// Get the broadphase box for a moving mox
// Arguments:
// - x, y, z: input center
// - ex, ey, ez: input extent
// - vx, vy, vz: velocity
// - bx, by, bz: output box center
// - bex, bey, bez: output box extent
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
    *bex = ex + ABS(vx)/2;
    *bey = ey + ABS(vy)/2;
    *bez = ez + ABS(vz)/2;
}

// Check if the two boxes A and B currently intersect.
// Arguments:
// - ax, ay, az: box A center
// - aex, aey, aez: box A extent
// - bx, by, bz: box B center
// - bex, bey, bez: box B extent
// Returns:
// - boolean of whether they intersect
int box_intersect_box(
        float ax, float ay, float az, float aex, float aey, float aez,
        float bx, float by, float bz, float bex, float bey, float bez)
{
    return !((ax + aex < bx - bex) || (ax - aex > bx + bex) ||
             (ay + aey < by - bey) || (ay - aey > by + bey) ||
             (az + aez < bz - bez) || (az - aez > bz + bez));
}

// Check whether a bounding box interects a block position
// Arguments:
// - x, y, z: box center
// - ex, ey, ez: box extent
// - bx, by, bz: block position
int box_intersect_block(
        float x, float y, float z, float ex, float ey, float ez,
        int bx, int by, int bz)
{
    const float n = CUBE_EXTENT;
    return box_intersect_box(x, y, z, ex, ey, ez, bx, by, bz, n, n, n);
}

