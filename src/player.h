#ifndef _player_h_
#define _player_h_

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "config.h"

// Player hitbox extent
#define PLAYER_WIDTH 0.4f
#define PLAYER_HEIGHT 1.2f
#define PLAYER_BLOCKHEIGHT 3

// Difference between player's position and player's head/eye level
#define PLAYER_HEADY 0.25f

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
    int is_grounded;
    int is_blocked;
    int flying;
    float brx;
    float jumpt;
    float blockt;
    float dblockt;
    float autot;
} State;

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
} Player;

void update_player(
        Player *player, float x, float y, float z, float rx, float ry,
        int interpolate);

void interpolate_player(Player *player);

void player_hitbox(
        float px, float py, float pz, float *x, float *y, float *z,
        float *ex, float *ey, float *ez);

void player_pos_inv(float x, float y, float z, float *px, float *py, float *pz);

GLuint gen_player_buffer(
        float x, float y, float z, float rx, float ry, float brx);

#endif /* _player_h_ */
