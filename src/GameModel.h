#ifndef _GameModel_h
#define _GameModel_h


#include "config.h"
#include "map.h"
#include "Worker.h"
#include "Block.h"
#include "Chunk.h"
#include "Physics.h"
#include "player.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256


// Program state model
// - window:
// - workers:
// - chunks:
// - chunk_count:
// - create_radius:
// - render_radius:
// - delete_radius:
// - sign_radius:
// - players:
// - player_count:
// - typing:
// - typing_buffer:
// - message_index:
// - messages:
// - width:
// - height:
// - observe1:
// - observe2:
// - flying:
// - item_index: current selected block to place next
// - scale:
// - ortho:
// - fov:
// - suppress_char:
// - mode:
// - mode_changed:
// - db_path:
// - server_addr:
// - server_port:
// - day_length:
// - time_changed:
// - block0:
// - block1:
// - copy0:
// - copy1:
typedef struct {
    GLFWwindow *window;
    Worker workers[WORKERS];
    Chunk chunks[MAX_CHUNKS];
    int chunk_count;
    int create_radius;
    int render_radius;
    int delete_radius;
    int sign_radius;
    Player players[MAX_PLAYERS];
    int player_count;
    int typing;
    char typing_buffer[MAX_TEXT_LENGTH];
    int message_index;
    char messages[MAX_MESSAGES][MAX_TEXT_LENGTH];
    int width;
    int height;
    int observe1;
    int observe2;
    int item_index;
    int scale;
    int ortho;
    float fov;
    int suppress_char;
    int mode;
    int mode_changed;
    char db_path[MAX_PATH_LENGTH];
    char server_addr[MAX_ADDR_LENGTH];
    int server_port;
    int day_length;
    int time_changed;
    Block block0;
    Block block1;
    Block copy0;
    Block copy1;
    PhysicsConfig physics;
} Model;


#endif
