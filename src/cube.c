#include "cube.h"
#include "item.h"
#include "matrix.h"
#include "player.h"
#include "util.h"
#include <math.h>


// Make a complete cube model with between 0 and 6 faces
void make_cube_faces(             // Writes specific values to the output pointer
        float *data,              // output pointer
        const float ao[6][4],
        const float light[6][4],
        int left,                 // whether to generate a left face
        int right,                // whether to generate a right face
        int top,                  // whether to generate a top face
        int bottom,               // whether to generate a bottom face
        int front,                // whether to generate a front face
        int back,                 // whether to generate a back face
        int wleft,                // id for tile texture for left face
        int wright,               // id for tile texture for right face
        int wtop,                 // id for tile texture for top face
        int wbottom,              // id for tile texture for bottom face
        int wfront,               // id for tile texture for front face
        int wback,                // id for tile texture for back face
        float x,                  // block center position
        float y,
        float z,
        float n)                  // cube scale (extent from center)
{
    // 6 faces each with 4 points, each of which are 3-vectors
    static const float positions[6][4][3] = {
        {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
        {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
        {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
        {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}
    };
    // 6 faces each with a 3-vector normal direction
    static const float normals[6][3] = {
        {-1, 0, 0},
        {+1, 0, 0},
        {0, +1, 0},
        {0, -1, 0},
        {0, 0, -1},
        {0, 0, +1}
    };
    // 6 faces each with 4 points, each of which are 2-vectors
    static const float uvs[6][4][2] = {
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 1}, {0, 0}, {1, 1}, {1, 0}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{1, 0}, {1, 1}, {0, 0}, {0, 1}}
    };
    static const float indices[6][6] = {
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3}
    };
    // flipped indices
    static const float flipped[6][6] = {
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1}
    };
    float *d = data;
    // Scale the texture atlas so that a 16x16 tile takes up the whole block
    float s = 0.0625; // 1/16th
    float a = 0 + 1 / 2048.0;
    float b = s - 1 / 2048.0;
    int faces[6] = {left, right, top, bottom, front, back};
    int tiles[6] = {wleft, wright, wtop, wbottom, wfront, wback};
    for (int i = 0; i < 6; i++) {
        // Do not write to faces which were not specified.
        if (faces[i] == 0) {
            continue;
        }
        // Convert texture tile number to texture pixel coordinates
        float du = (tiles[i] % 16) * s;
        float dv = (tiles[i] / 16) * s;
        int flip = ao[i][0] + ao[i][3] > ao[i][1] + ao[i][2];
        for (int v = 0; v < 6; v++) {
            // j is index into the points
            int j = flip ? flipped[i][v] : indices[i][v];
            // Write the position 3-vector
            *(d++) = x + n * positions[i][j][0];
            *(d++) = y + n * positions[i][j][1];
            *(d++) = z + n * positions[i][j][2];
            // Write the normal 3-vector
            *(d++) = normals[i][0];
            *(d++) = normals[i][1];
            *(d++) = normals[i][2];
            // Write the texture UV 2-vector
            *(d++) = du + (uvs[i][j][0] ? b : a);
            *(d++) = dv + (uvs[i][j][1] ? b : a);
            *(d++) = ao[i][j];
            *(d++) = light[i][j];
        }
    }
}


// Make a cube model for a block
void make_cube(                   // writes specific values to the data pointer
        float *data,              // output pointer
        const float ao[6][4],
        const float light[6][4],
        int left,                 // whether to generate the faces
        int right,
        int top, 
        int bottom,
        int front,
        int back,
        float x,                  // cube center
        float y,
        float z,
        float n,                  // cube scale (aka extent)
        int w)                    // cube tile ID
{
    // Get blocks texture faces (the blocks lookup table is defined in item.c)
    int wleft = blocks[w][0];
    int wright = blocks[w][1];
    int wtop = blocks[w][2];
    int wbottom = blocks[w][3];
    int wfront = blocks[w][4];
    int wback = blocks[w][5];
    make_cube_faces(
            data,
            ao,
            light,
            left,
            right,
            top,
            bottom,
            front,
            back,
            wleft,
            wright,
            wtop,
            wbottom,
            wfront,
            wback,
            x,
            y,
            z,
            n);
}


// Make a plant model
void make_plant(       // writes specific values to the data pointer
        float *data,   // output pointer
        float ao,
        float light,
        float px,
        float py,
        float pz,
        float n,
        int w,
        float rotation)
{
    static const float positions[4][4][3] = {
        {{ 0, -1, -1}, { 0, -1, +1}, { 0, +1, -1}, { 0, +1, +1}},
        {{ 0, -1, -1}, { 0, -1, +1}, { 0, +1, -1}, { 0, +1, +1}},
        {{-1, -1,  0}, {-1, +1,  0}, {+1, -1,  0}, {+1, +1,  0}},
        {{-1, -1,  0}, {-1, +1,  0}, {+1, -1,  0}, {+1, +1,  0}}
    };
    static const float normals[4][3] = {
        {-1, 0, 0},
        {+1, 0, 0},
        {0, 0, -1},
        {0, 0, +1}
    };
    static const float uvs[4][4][2] = {
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{1, 0}, {1, 1}, {0, 0}, {0, 1}}
    };
    static const float indices[4][6] = {
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3}
    };
    float *d = data;
    // Scale the texture atlas so that a 16x16 tile takes up the whole block
    float s = 0.0625;
    float a = 0;
    float b = s;
    // Convert texture tile number to texture pixel coordinates
    float du = (plants[w] % 16) * s;
    float dv = (plants[w] / 16) * s;
    for (int i = 0; i < 4; i++) {
        for (int v = 0; v < 6; v++) {
            int j = indices[i][v];
            // Write the position 3-vector
            *(d++) = n * positions[i][j][0];
            *(d++) = n * positions[i][j][1];
            *(d++) = n * positions[i][j][2];
            // Write the normal 3-vector
            *(d++) = normals[i][0];
            *(d++) = normals[i][1];
            *(d++) = normals[i][2];
            // Write the UV 2-vector
            *(d++) = du + (uvs[i][j][0] ? b : a);
            *(d++) = dv + (uvs[i][j][1] ? b : a);
            // Write the ao value
            *(d++) = ao;
            // Write the light value
            *(d++) = light;
        }
    }
    // matrix "ma" and matrix "mb"
    float ma[16];
    float mb[16];
    mat_identity(ma);
    mat_rotate(mb, 0, 1, 0, RADIANS(rotation));
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 24, 3, 10);
    mat_translate(mb, px, py, pz);
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 24, 0, 10);
}


static void make_player_head(
        float *data,
        unsigned count,
        unsigned offset,
        unsigned stride,
        float x,      // player position x
        float y,      // player position y
        float z,      // player position z
        float rx,     // player (head) rotation
        float ry,
        const float ao[6][4],
        const float light[6][4])
{
    // Make a player head with specific texture tiles
    // and a scale smaller than a normal block
    make_cube_faces(
            data, ao, light,
            1, 1, 1, 1, 1, 1,
            226, 224, 241, 209, 225, 227,
            0, 0, 0, 0.35);

    float ma[16];
    float mb[16];
    mat_identity(ma);

    mat_rotate(mb, 0, 1, 0, rx);
    mat_multiply(ma, mb, ma);

    mat_rotate(mb, cosf(rx), 0, sinf(rx), -ry);
    mat_multiply(ma, mb, ma);

    mat_scale(mb, PLAYER_HEAD_SCALE, PLAYER_HEAD_SCALE, PLAYER_HEAD_SCALE);
    mat_multiply(ma, mb, ma);

    mat_translate(mb, x, player_eye_y(y), z);
    mat_multiply(ma, mb, ma);

    mat_apply(data, ma, count, offset, stride);
}


static void make_player_body(
        float *data,
        unsigned count,
        unsigned offset,
        unsigned stride,
        float x,
        float y,
        float z,
        float brx,
        const float ao[6][4],
        const float light[6][4])
{
    float ma[16];
    float mb[16];
    make_cube_faces(
            data + offset, ao, light,
            1, 1, 1, 1, 1, 1,
            230, 228, 245, 213, 229, 231,
            0, 0, 0, 1);
    mat_identity(ma);
    mat_translate(mb, x, y + PLAYER_BODY_Y, z);
    mat_multiply(ma, ma, mb);
    mat_rotate(mb, 0, 1, 0, brx);
    mat_multiply(ma, ma, mb);
    mat_scale(mb, PLAYER_BODY_EX, PLAYER_BODY_EY, PLAYER_BODY_EZ);
    mat_multiply(ma, ma, mb);
    mat_apply(data, ma, count, offset, stride);
}


static void make_player_limb(
        float *data, 
        int count,
        int offset,
        int stride,
        float x, 
        float y_top, 
        float z,
        float brx,
        float angle_z,
        const float ao[6][4],
        const float light[6][4])
{
    make_cube_faces(
            data + offset, ao, light,
            1, 1, 1, 1, 1, 1,
            230, 228, 245, 213, 229, 231,
            0, 0, 0, 1);

    float ma[16], mb[16];
    mat_identity(ma);

    mat_scale(mb, PLAYER_LIMB_EX, PLAYER_LIMB_EY, PLAYER_LIMB_EX);
    mat_multiply(ma, mb, ma);

    mat_rotate(mb, 0, 1, 0, brx);
    mat_multiply(ma, mb, ma);

    mat_translate(mb, x, y_top - PLAYER_LIMB_EX, z);
    mat_multiply(ma, mb, ma);

    mat_apply(data, ma, count, offset, stride);
}


// Make a player model
void make_player(     // writes specific values to the data pointer
        float *data,  // output pointer
        float x,      // player position x
        float y,      // player position y
        float z,      // player position z
        float rx,     // player (head) rotation
        float ry,
        float brx)    // player body rotation
{
    const int count = 36;
    const int stride = 10;
    const int offset = count * stride;

    float ao[6][4] = {0};
    float light[6][4] = {
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8}
    };

    make_player_head(data, count, offset*0, stride,
            x, y, z, rx, ry, ao, light);

    make_player_body(data, count, offset*1, stride,
            x, y, z, brx, ao, light);

    // arm
    make_player_limb(data, count, offset*4, stride,
            x + (PLAYER_BODY_EX + PLAYER_LIMB_EX), y + PLAYER_BODY_EY, z, brx, 0.0f, ao, light);

    // arm
    make_player_limb(data, count, offset*5, stride,
            x - (PLAYER_BODY_EX + PLAYER_LIMB_EX), y + PLAYER_BODY_EY, z, brx, 0.0f, ao, light);

    // leg
    make_player_limb(data, count, offset*2, stride,
            x + PLAYER_LIMB_EX, y - PLAYER_BODY_EY, z, brx, 0.0f, ao, light);

    // leg
    make_player_limb(data, count, offset*3, stride,
            x - PLAYER_LIMB_EX, y - PLAYER_BODY_EY, z, brx, 0.0f, ao, light);
}


// Make a 3D box wireframe model
void make_box_wireframe(  // writes specific values to the data pointer
        float *data,      // output pointer (must have room for 24*3 floats)
        float x,          // center
        float y,
        float z,
        float ex,         // extent
        float ey,
        float ez)
{
    // Make a unit cube at the origin because we will translate and scale it
    // with matrix math.
    make_cube_wireframe(data, 0, 0, 0, 1);
    float ma[16];
    float mb[16];
    mat_translate(ma, x, y, z);
    mat_scale(mb, ex, ey, ez);
    mat_multiply(ma, ma, mb);
    const int count = 24;
    const int offset = 0;
    const int stride = 3;
    mat_apply(data, ma, count, offset, stride);
}


// Make a cube wireframe model
void make_cube_wireframe(  // writes specific values to the data pointer
        float *data,       // data: output pointer (must have room for 24*3 floats)
        float x,           // block/cube position
        float y,
        float z,
        float n)           // cube scale
{
    // 8 points, each of which are 3-vectors
    static const float positions[8][3] = {
        {-1, -1, -1},
        {-1, -1, +1},
        {-1, +1, -1},
        {-1, +1, +1},
        {+1, -1, -1},
        {+1, -1, +1},
        {+1, +1, -1},
        {+1, +1, +1}
    };
    // The correct ordering of the points to form a nice cube
    static const int indices[24] = {
        0, 1, 0, 2, 0, 4, 1, 3,
        1, 5, 2, 3, 2, 6, 3, 7,
        4, 5, 4, 6, 5, 7, 6, 7
    };
    float *d = data;
    for (int i = 0; i < 24; i++) {
        int j = indices[i];
        // Write the position 3-vector
        *(d++) = x + n * positions[j][0];
        *(d++) = y + n * positions[j][1];
        *(d++) = z + n * positions[j][2];
    }
}


// Make a rectangle for a 2D text character.
void make_character(
        float *data,  // output pointer
        float x,      // center x
        float y,      // center y
        float n,      // width (extent from center to edge)
        float m,      // height (extent from center to edge)
        char c)       // ascii character
{
    float *d = data;
    float s = 0.0625;
    float a = s;
    float b = s * 2;
    int w = c - 32;
    float du = (w % 16) * a;
    float dv = 1 - (w / 16) * b - b;
    *(d++) = x - n; *(d++) = y - m;
    *(d++) = du + 0; *(d++) = dv;
    *(d++) = x + n; *(d++) = y - m;
    *(d++) = du + a; *(d++) = dv;
    *(d++) = x + n; *(d++) = y + m;
    *(d++) = du + a; *(d++) = dv + b;
    *(d++) = x - n; *(d++) = y - m;
    *(d++) = du + 0; *(d++) = dv;
    *(d++) = x + n; *(d++) = y + m;
    *(d++) = du + a; *(d++) = dv + b;
    *(d++) = x - n; *(d++) = y + m;
    *(d++) = du + 0; *(d++) = dv + b;
}


// Make a rectangle for a 3D text character (for rendering signs).
void make_character_3d(
        float *data,     // output pointer
        float x,         // character center x
        float y,         // character center y
        float z,         // character center z
        float n,         // scale
        int face,        // which cube face the text is on
        char c)          // ASCII character value
{
    static const float positions[8][6][3] = {
        {{0, -2, -1}, {0, +2, +1}, {0, +2, -1},
         {0, -2, -1}, {0, -2, +1}, {0, +2, +1}},
        {{0, -2, -1}, {0, +2, +1}, {0, -2, +1},
         {0, -2, -1}, {0, +2, -1}, {0, +2, +1}},
        {{-1, -2, 0}, {+1, +2, 0}, {+1, -2, 0},
         {-1, -2, 0}, {-1, +2, 0}, {+1, +2, 0}},
        {{-1, -2, 0}, {+1, -2, 0}, {+1, +2, 0},
         {-1, -2, 0}, {+1, +2, 0}, {-1, +2, 0}},
        {{-1, 0, +2}, {+1, 0, +2}, {+1, 0, -2},
         {-1, 0, +2}, {+1, 0, -2}, {-1, 0, -2}},
        {{-2, 0, +1}, {+2, 0, -1}, {-2, 0, -1},
         {-2, 0, +1}, {+2, 0, +1}, {+2, 0, -1}},
        {{+1, 0, +2}, {-1, 0, -2}, {-1, 0, +2},
         {+1, 0, +2}, {+1, 0, -2}, {-1, 0, -2}},
        {{+2, 0, -1}, {-2, 0, +1}, {+2, 0, +1},
         {+2, 0, -1}, {-2, 0, -1}, {-2, 0, +1}}
    };
    static const float uvs[8][6][2] = {
        {{0, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}},
        {{1, 0}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}},
        {{1, 0}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}},
        {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}},
        {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}}
    };
    static const float offsets[8][3] = {
        {-1, 0, 0}, {+1, 0, 0}, {0, 0, -1}, {0, 0, +1},
        {0, +1, 0}, {0, +1, 0}, {0, +1, 0}, {0, +1, 0},
    };
    float *d = data;
    float s = 0.0625;
    float pu = s / 5;
    float pv = s / 2.5;
    float u1 = pu;
    float v1 = pv;
    float u2 = s - pu;
    float v2 = s * 2 - pv;
    float p = 0.5;
    int w = c - 32;
    float du = (w % 16) * s;
    float dv = 1 - (w / 16 + 1) * s * 2;
    x += p * offsets[face][0];
    y += p * offsets[face][1];
    z += p * offsets[face][2];
    for (int i = 0; i < 6; i++) {
        *(d++) = x + n * positions[face][i][0];
        *(d++) = y + n * positions[face][i][1];
        *(d++) = z + n * positions[face][i][2];
        *(d++) = du + (uvs[face][i][0] ? u2 : u1);
        *(d++) = dv + (uvs[face][i][1] ? v2 : v1);
    }
}


// Make a sphere with radius r and the given level of detail.
// NOTE: Meant to be called recursively and by make_sphere().
int _make_sphere(
        float *data,
        float r,
        int detail,
        float *a,
        float *b,
        float *c,
        float *ta,
        float *tb,
        float *tc)
{
    if (detail == 0) {
        float *d = data;
        *(d++) = a[0] * r; *(d++) = a[1] * r; *(d++) = a[2] * r;
        *(d++) = a[0]; *(d++) = a[1]; *(d++) = a[2];
        *(d++) = ta[0]; *(d++) = ta[1];
        *(d++) = b[0] * r; *(d++) = b[1] * r; *(d++) = b[2] * r;
        *(d++) = b[0]; *(d++) = b[1]; *(d++) = b[2];
        *(d++) = tb[0]; *(d++) = tb[1];
        *(d++) = c[0] * r; *(d++) = c[1] * r; *(d++) = c[2] * r;
        *(d++) = c[0]; *(d++) = c[1]; *(d++) = c[2];
        *(d++) = tc[0]; *(d++) = tc[1];
        return 1;
    }
    else {
        float ab[3], ac[3], bc[3];
        for (int i = 0; i < 3; i++) {
            ab[i] = (a[i] + b[i]) / 2;
            ac[i] = (a[i] + c[i]) / 2;
            bc[i] = (b[i] + c[i]) / 2;
        }
        normalize(ab + 0, ab + 1, ab + 2);
        normalize(ac + 0, ac + 1, ac + 2);
        normalize(bc + 0, bc + 1, bc + 2);
        float tab[2], tac[2], tbc[2];
        tab[0] = 0; tab[1] = 1 - acosf(ab[1]) / PI;
        tac[0] = 0; tac[1] = 1 - acosf(ac[1]) / PI;
        tbc[0] = 0; tbc[1] = 1 - acosf(bc[1]) / PI;
        int total = 0;
        int n;
        n = _make_sphere(data, r, detail - 1, a, ab, ac, ta, tab, tac);
        total += n; data += n * 24;
        n = _make_sphere(data, r, detail - 1, b, bc, ab, tb, tbc, tab);
        total += n; data += n * 24;
        n = _make_sphere(data, r, detail - 1, c, ac, bc, tc, tac, tbc);
        total += n; data += n * 24;
        n = _make_sphere(data, r, detail - 1, ab, bc, ac, tab, tbc, tac);
        total += n; data += n * 24;
        return total;
    }
}


// Make a sphere with radius r and the given level of detail.
// Arguments:
// - data: output pointer
// - r: sphere radius
// - detail: level of detail (see note below)
// Returns:
// - data: output pointer
// NOTE: Table of resources needed for each level of detail:
//   detail, triangles,  floats
//        0,         8,     192
//        1,        32,     768
//        2,       128,    3072
//        3,       512,   12288
//        4,      2048,   49152
//        5,      8192,  196608
//        6,     32768,  786432
//        7,    131072, 3145728
void make_sphere(
        float *data,
        float r,
        int detail)
{
    static int indices[8][3] = {
        {4, 3, 0}, {1, 4, 0},
        {3, 4, 5}, {4, 1, 5},
        {0, 3, 2}, {0, 2, 1},
        {5, 2, 3}, {5, 1, 2}
    };
    static float positions[6][3] = {
        { 0, 0,-1}, { 1, 0, 0},
        { 0,-1, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0, 0, 1}
    };
    static float uvs[6][3] = {
        {0, 0.5}, {0, 0.5},
        {0, 0}, {0, 0.5},
        {0, 1}, {0, 0.5}
    };
    int total = 0;
    for (int i = 0; i < 8; i++) {
        int n = _make_sphere(
                data,
                r,
                detail,
                positions[indices[i][0]],
                positions[indices[i][1]],
                positions[indices[i][2]],
                uvs[indices[i][0]],
                uvs[indices[i][1]],
                uvs[indices[i][2]]);
        total += n; data += n * 24;
    }
}

