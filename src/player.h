#ifndef _player_h_
#define _player_h_


#include "config.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>


// Player hitbox extent
#define PLAYER_WIDTH 0.4f
#define PLAYER_HEIGHT 1.2f
#define PLAYER_BLOCKHEIGHT 3

// Difference between player's position and player's head/eye level
#define PLAYER_HEAD_SCALE 0.69f
#define PLAYER_HEAD_Y (PLAYER_HEIGHT - PLAYER_HEAD_SCALE/2.0f + 0.1f)

#define PLAYER_BODY_EY 0.40f
#define PLAYER_BODY_EX 0.33f
#define PLAYER_BODY_EZ 0.15f
#define PLAYER_BODY_Y (PLAYER_HEAD_Y - PLAYER_HEAD_SCALE/2.0f - PLAYER_BODY_EY + 0.11f)
#define PLAYER_LIMB_EY PLAYER_BODY_EY
#define PLAYER_LIMB_EX PLAYER_BODY_EZ


// State for a player
// - x, y, z: position
// - rx, ry: rotation x and y
// - t: keep track of time, for interpolation
// - is_grounded: flag for if on the ground and can jump
// - is_blocked: flag for if stuck in a block
// - flying: flag for if flying
// - brx: body rotation x
// - jumpt: last jump time (for limiting jump rate)
// - blockt: last block placement time (for limiting rate)
// - dblockt: last block break time (for limiting rate)
// - autot: last automatic block break/placement
typedef struct {
    float x; 
    float y;
    float z;
    float rx;
    float ry;
    float vx;
    float vy;
    float vz;
    float t; 
    float brx;
} State;


typedef struct {
    int is_grounded;
    int is_blocked;
    int flying;
    float jumpt;
    float blockt;
    float dblockt;
    float autot;
    int taken_damage;  // amount of damage this player has taken
    int attack_damage; // amount of damage this player attacks with
    float reach;       // reach for place/destroy/attack (in blocks)
} PlayerAttributes;


// Player
// - id
// - name: name string buffer
// - state: current player position state
// - state1: another state, for interpolation
// - state2: another state, for interpolation
// - buffer: some GL buffer id (?)
typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    State state;
    State state1;
    State state2;
    GLuint buffer;
    PlayerAttributes attrs;
} Player;


void player_hitbox_extent(
        float *ex,
        float *ey,
        float *ez);


GLuint gen_player_buffer(
        float x,
        float y,
        float z,
        float rx,
        float ry,
        float brx);

void interpolate_player(
        Player *player);

void update_player(
        Player *player,
        float x,
        float y,
        float z,
        float rx,
        float ry,
        int interpolate);


void set_matrix_3d_player_camera(
        float matrix[16],
        const Player *p);

float player_eye_y(
        float y);


#endif /* _player_h_ */
