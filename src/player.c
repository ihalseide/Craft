#include "config.h"
#include "config.h"
#include "cube.h"
#include "game.h"
#include "matrix.h"
#include "player.h"
#include "util.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
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

