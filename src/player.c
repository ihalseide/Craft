#include "config.h"
#include "cube.h"
#include "game.h"
#include "matrix.h"
#include "player.h"
#include "util.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>


// compute player eye height
float player_eye_y(  // returns player eye Y position
        float y)     // player Y position
{
    return y + PLAYER_HEAD_Y;
}


void player_hitbox_extent(
        float *ex,
        float *ey,
        float *ez)
{
    *ex = PLAYER_WIDTH;
    *ey = PLAYER_HEIGHT;
    *ez = PLAYER_WIDTH;
}


// Set the current player state to interpolate between the previous known states
void interpolate_player(
        Player *player) 
{
    State *s1 = &player->state1;
    State *s2 = &player->state2;
    float t1 = s2->t - s1->t;
    float t2 = glfwGetTime() - s2->t;
    t1 = MIN(t1, 1);
    t1 = MAX(t1, 0.1);
    float p = MIN(t2 / t1, 1);
    update_player(
            player,
            s1->x + (s2->x - s1->x) * p,
            s1->y + (s2->y - s1->y) * p,
            s1->z + (s2->z - s1->z) * p,
            s1->rx + (s2->rx - s1->rx) * p,
            s1->ry + (s2->ry - s1->ry) * p,
            0);
}


// Update a player with a new position and rotation.
void update_player(
        Player *player,   // player to modify
        float x,          // position x
        float y,          // position y
        float z,          // position z
        float rx,         // rotation x
        float ry,         // rotation y
        int interpolate)  // flag to interpolate position?
{
    if (interpolate) {
        State *s1 = &player->state1;
        State *s2 = &player->state2;
        memcpy(s1, s2, sizeof(State));
        s2->x = x; s2->y = y; s2->z = z; s2->rx = rx; s2->ry = ry;
        s2->t = glfwGetTime();
        if (s2->rx - s1->rx > PI) {
            s1->rx += 2 * PI;
        }
        if (s1->rx - s2->rx > PI) {
            s1->rx -= 2 * PI;
        }
    }
    else {
        State *s = &player->state;
        s->x = x; s->y = y; s->z = z; s->rx = rx; s->ry = ry;
        del_buffer(player->buffer);
        player->buffer = gen_player_buffer(
                s->x, s->y, s->z, s->rx, s->ry, s->brx);
    }
}


// Create buffer for player model
GLuint gen_player_buffer(  // returns OpenGL buffer handle
        float x,           // player x
        float y,           // player y
        float z,           // player z
        float rx,          // player rotation x
        float ry,          // player rotation y
        float brx)         // player body rotation x
{
    const unsigned faces = 6 * 2 * 6;
    GLfloat *data = malloc_faces(10, faces);
    make_player(data, x, y, z, rx, ry, brx);
    return gen_faces(10, faces, data);
}


void set_matrix_3d_player_camera(   // Everything except the player pointer is output
        float matrix[16],           // [output]
        const Player *p)            // Player to get camera for
{
    set_matrix_3d(
            matrix,
            g->width,
            g->height,
            p->state.x,
            player_eye_y(p->state.y),
            p->state.z,
            p->state.rx,
            p->state.ry,
            g->fov,
            g->ortho,
            g->render_radius);
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


