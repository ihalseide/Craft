#include "config.h"
#include "config.h"
#include "cube.h"
#include "player.h"
#include "util.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string.h>


// Get a player's hitbox (center and extents)
// Arguments:
// - px, py, pz: the player's position
// - x, y, z: pointers to output hitbox center to
// - ex, ey, ez: pointer to output hitbox extents to
// Returns:
// - modifies values pointed to by x, y, z, ex, ey, and ez
void player_hitbox(
        float px,
        float py,
        float pz,
        float *x,
        float *y,
        float *z,
        float *ex,
        float *ey,
        float *ez)
{
    *x = px;
    *y = py;
    *z = pz;
    *ex = PLAYER_WIDTH;
    *ey = PLAYER_HEIGHT;
    *ez = PLAYER_WIDTH;
}


// Player position inverse. Set player position from hitbox center point
// (inverse of player_hitbox()).
// Arguments:
// - x, y, z: hitbox position to convert into player position
// - px, py, pz: pointer to player position to modify
// Returns:
// - modifies values pointed to by px, py, and pz
void player_pos_inv(
        float x,
        float y,
        float z,
        float *px,
        float *py,
        float *pz)
{
    *px = x;
    *py = y + PLAYER_HEADY + (PLAYER_HEIGHT / 2.0);
    *pz = z;
}


// Set the current player state to interpolate between the previous known states
void interpolate_player(Player *player) {
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
// Arguments:
// - player: player to modify
// - x, y, z: new position
// - rx, ry: new rotation
// - interpolate: whether to interpolate position
void update_player(
        Player *player,
        float x,
        float y,
        float z,
        float rx,
        float ry,
        int interpolate)
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
// Arguments:
// - x, y, z: player position
// - rx, ry: player head rotation x and rotation y
// Returns:
// - OpenGL buffer handle
GLuint gen_player_buffer(
        float x,
        float y,
        float z,
        float rx,
        float ry,
        float brx) 
{
    GLfloat *data = malloc_faces(10, 2*2*6);
    make_player(data, x, y, z, rx, ry, brx);
    return gen_faces(10, 2*6, data);
}

