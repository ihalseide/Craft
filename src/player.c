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
GLuint 
gen_player_buffer(  // returns OpenGL buffer handle
        float x,           // player x
        float y,           // player y
        float z,           // player z
        float rx,          // player rotation x
        float ry,          // player rotation y
        float brx)         // player body rotation x
{
    const unsigned faces = 6 * 4;  // TODO: change to 6 * 6 for all body parts
    GLfloat *data = malloc_faces(10, faces);
    make_player(data, x, y, z, rx, ry, brx);
    return gen_faces(10, faces, data);
}


void 
set_matrix_3d_player_camera(   // Everything except the player pointer is output
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


static
void
make_player_head(
        float *data,               // data out
        unsigned count,
        unsigned offset,
        unsigned stride,
        float x,                   // player position x
        float y,                   // player position y
        float z,                   // player position z
        float rx,                  // player (head) rotation
        float ry,
        const float ao[6][4],
        const float light[6][4])
{
    // Make a player head with specific texture tiles
    // and a scale smaller than a normal block
    const int head_size = 10;
    static const TexturedBox head_box = 
    {
        .left    = (PointInt2){ .x = 24, .y = 18 },
        .right   = (PointInt2){ .x = 0,  .y = 18 },
        .top     = (PointInt2){ .x = 12, .y = 6  },
        .bottom  = (PointInt2){ .x = 12, .y = 30 },
        .front   = (PointInt2){ .x = 12, .y = 18 },
        .back    = (PointInt2){ .x = 0,  .y = 6  },
        .x_width  = head_size,
        .y_height = head_size,
        .z_depth  = head_size,
    };
    make_box(data, ao, light, &head_box, 0, 0, 0);

    float ma[16];
    float mb[16];
    mat_identity(ma);

    mat_rotate(mb, 0, 1, 0, rx);
    mat_multiply(ma, mb, ma);

    mat_rotate(mb, cosf(rx), 0, sinf(rx), -ry);
    mat_multiply(ma, mb, ma);

    mat_translate(mb, x, y + PLAYER_HEAD_Y, z);
    mat_multiply(ma, mb, ma);

    mat_apply(data, ma, count, offset, stride);
}


static
void
make_player_body(
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
    static const TexturedBox body_box = 
    {
        .left    = (PointInt2){ .x = 62, .y = 14 },
        .right   = (PointInt2){ .x = 36, .y = 14 },
        .top     = (PointInt2){ .x = 46, .y = 4  },
        .bottom  = (PointInt2){ .x = 46, .y = 34 },
        .front   = (PointInt2){ .x = 46, .y = 14 },
        .back    = (PointInt2){ .x = 72, .y = 14 },
        .x_width  = 12,
        .y_height = 14,
        .z_depth  = 7,
    };
    make_box(data + offset, ao, light, &body_box, 0, 0, 0);

    float ma[16];
    float mb[16];
    mat_identity(ma);

    mat_translate(mb, x, y + PLAYER_BODY_Y, z);
    mat_multiply(ma, ma, mb);

    mat_rotate(mb, 0, 1, 0, brx);
    mat_multiply(ma, ma, mb);

    mat_apply(data, ma, count, offset, stride);
}


static
void
make_player_leg(
        float *data,
        unsigned count,
        unsigned offset,
        unsigned stride,
        float x,
        float y,
        float z,
        float brx,
        int is_left,              // flag for if this is the left leg
        const float ao[6][4],
        const float light[6][4])
{
    static const TexturedBox left_leg_box = 
    {
        .left    = (PointInt2){ .x = 100, .y = 24 },
        .right   = (PointInt2){ .x = 88,  .y = 24 },
        .top     = (PointInt2){ .x = 112, .y = 24 },
        .bottom  = (PointInt2){ .x = 82,  .y = 38 },
        .front   = (PointInt2){ .x = 94,  .y = 24 },
        .back    = (PointInt2){ .x = 106, .y = 24 },
        .x_width  = 6,
        .y_height = 15,
        .z_depth  = 6,
    };
    static const TexturedBox right_leg_box = 
    {
        .left    = (PointInt2){ .x = 30 + 100, .y = 24 },
        .right   = (PointInt2){ .x = 30 + 88,  .y = 24 },
        .top     = (PointInt2){ .x = 30 + 112, .y = 24 },
        .bottom  = (PointInt2){ .x = 30 + 82,  .y = 38 },
        .front   = (PointInt2){ .x = 30 + 94,  .y = 24 },
        .back    = (PointInt2){ .x = 30 + 106, .y = 24 },
        .x_width  = 6,
        .y_height = 15,
        .z_depth  = 6,
    };
    const TexturedBox *leg_box = is_left ? &left_leg_box : &right_leg_box;
    make_box(data + offset, ao, light, leg_box, 0, 0, 0);

    float ma[16];
    float mb[16];
    mat_identity(ma);

    mat_translate(mb, x, y - 4.0/16.0 - 0.45, z);
    mat_multiply(ma, ma, mb);

    mat_rotate(mb, 0, 1, 0, brx);
    mat_multiply(ma, ma, mb);

    float leg_offset = (is_left ? -1 : 1) * 3.0 / 16.0; // +/-3 pixels
    //leg_offset*=2; // to see the inside
    mat_translate(mb, leg_offset, 0, 0);
    mat_multiply(ma, ma, mb);

    mat_apply(data, ma, count, offset, stride);
}



static
void
make_player_arm(
        float *data,
        unsigned count,
        unsigned offset,
        unsigned stride,
        float x,
        float y,
        float z,
        float brx,
        int is_left,              // flag for if this is the left leg
        const float ao[6][4],
        const float light[6][4])
{
    static const TexturedBox left_arm_box = 
    {
        .left    = (PointInt2){ .x = 88,  .y = 4 },
        .right   = (PointInt2){ .x = 98,  .y = 4 },
        .top     = (PointInt2){ .x = 112, .y = 4 },
        .bottom  = (PointInt2){ .x = 82,  .y = 19 },
        .front   = (PointInt2){ .x = 93,  .y = 4 },
        .back    = (PointInt2){ .x = 103, .y = 4 },
        .x_width  = 5,
        .y_height = 15,
        .z_depth  = 5,
    };
    static const TexturedBox right_arm_box = 
    {
        .left    = (PointInt2){ .x = 30 + 100, .y = 24 },
        .right   = (PointInt2){ .x = 30 + 88,  .y = 24 },
        .top     = (PointInt2){ .x = 30 + 112, .y = 24 },
        .bottom  = (PointInt2){ .x = 30 + 82,  .y = 38 },
        .front   = (PointInt2){ .x = 30 + 94,  .y = 24 },
        .back    = (PointInt2){ .x = 30 + 106, .y = 24 },
        .x_width  = 5,
        .y_height = 15,
        .z_depth  = 5,
    };
    const TexturedBox *arm_box = is_left ? &left_arm_box : &right_arm_box;
    make_box(data + offset, ao, light, arm_box, 0, 0, 0);

    float ma[16];
    float mb[16];
    mat_identity(ma);

    mat_translate(mb, x, y + 1.5 / 16.0, z);
    mat_multiply(ma, ma, mb);

    mat_rotate(mb, 0, 1, 0, brx);
    mat_multiply(ma, ma, mb);

    float arm_offset = (is_left ? -1 : 1) * 8.0 / 16.0; // +/-8 pixels
    //arm_offset*=2; // to see the inside
    mat_translate(mb, arm_offset, 0, 0);
    mat_multiply(ma, ma, mb);

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

    // left leg
    make_player_arm(data, count, offset*2, stride,
            x, y, z, brx, 1, ao, light);

    // right leg
    make_player_arm(data, count, offset*3, stride,
            x, y, z, brx, 0, ao, light);
}


