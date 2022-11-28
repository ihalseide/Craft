#ifndef _game_h_
#define _game_h_


#include "config.h"
#include "cube.h"
#include "hitbox.h"
#include "item.h"
#include "map.h"
#include "player.h"
#include "player.h"
#include "sign.h"
#include "util.h"
#include "world.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tinycthread.h>


#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256


enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2,
};


enum {
    MODE_OFFLINE = 0,
    MODE_ONLINE = 1,
};


enum {
    WORKER_IDLE = 0,
    WORKER_BUSY = 1,
    WORKER_DONE = 2,
};


// Physiscs settings
typedef struct {
    float grav;                   // gravity
    float walksp;                 // walking speed
    float flysp;                  // flying speed
    float jumpaccel;              // jump vertical power or acceleration
    float flyr;                   // flying movement resistance
    float groundr;                // ground horizontal resistance factor
    float airhr;                  // air horizontal resistance factor
    float airvr;                  // air vertical resistance factor
    float jumpcool;               // jump cool-down time
    float blockcool;              // block placing cool-down time
    float dblockcool;             // block destroying cool-down time
    float min_impulse_damage;     // minimum velocity change required to take damage
    float impulse_damage_min;     // base damage dealt with velocity change exceeding the limit above
    float impulse_damage_scale;   // damage multiplier for velocity change exceeding the limit above
} PhysicsConfig;


// World chunk data (big area of blocks)
typedef struct {
    Map map;         // block types
    Map lights;      // block lights
    Map damage;      // block damage
    SignList signs;  // signs in the chunk
    int p;           // chunk X
    int q;           // chunk Z
    int faces;       // number of block faces
    int sign_faces;  // number of sign faces
    int dirty;       // flag
    int miny;        // minimum Y value held by any block
    int maxy;        // maximum Y value held by any block
    GLuint buffer;
    GLuint sign_buffer;
} Chunk;


// A single item that a Worker can work on
typedef struct {
    int p;                   // chunked X
    int q;                   // chunked Z
    int load;
    Map *block_maps[3][3];
    Map *light_maps[3][3];
    Map *damage_maps[3][3];
    int miny;
    int maxy;
    int faces;
    GLfloat *data;
} WorkerItem;


typedef struct {
    int index;
    int state;
    thrd_t thrd;      // thread
    mtx_t mtx;        // mutex
    cnd_t cnd;        // condition
    WorkerItem item;
} Worker;


// Block data
// - x: x position
// - y: y position
// - z: z position
// - w: block id / value
typedef struct {
    int x;
    int y;
    int z;
    int w;
} Block;


// OpenGL attribute data
// - program:
// - position:
// - normal:
// - uv:
// - matrix:
// - sampler:
// - camera:
// - timer:
// - extra1:
// - extra2:
// - extra3:
// - extra4:
typedef struct {
    GLuint program;
    GLuint position;
    GLuint normal;
    GLuint uv;
    GLuint matrix;
    GLuint sampler;
    GLuint camera;
    GLuint timer;
    GLuint extra1;
    GLuint extra2;
    GLuint extra3;
    GLuint extra4;
} Attrib;


// - active: flag for whether the box is active
// - buffer: opengl points model
typedef struct
{
    int active;
    GLuint buffer;
    Box box;
} DebugBox;


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


// Game model pointer
extern Model *g;


int _gen_sign_buffer(
        GLfloat *data,
        float x,
        float y,
        float z,
        int face,
        const char *text);

int _hit_test(
        Map *map,
        float max_distance,
        int previous,
        float x,
        float y,
        float z,
        float vx,
        float vy,
        float vz,
        int *hx,
        int *hy,
        int *hz);

void _set_block(int p,
        int q,
        int x,
        int y,
        int z,
        int w,
        int dirty);

void _set_sign( int p,
        int q,
        int x,
        int y,
        int z,
        int face,
        const char *text,
        int dirty);

int add_block_damage(int x,
        int y,
        int z,
        int damage);

void add_message(
        const char *text);

void array(
        Block *b1,
        Block *b2,
        int xc,
        int yc,
        int zc);

int box_intersect_world( float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        int *cx,
        int *cy,
        int *cz);

float box_sweep_world( float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        float vx,
        float vy,
        float vz,
        float *nx,
        float *ny,
        float *nz);

int break_block();

void builder_block(
        int x,
        int y,
        int z,
        int w);

float calc_damage_from_impulse(
        float d_vel);

void check_workers();

int chunk_distance(
        Chunk *chunk,
        int p,
        int q);

int chunk_visible(
        float planes[6][4],
        int p,
        int q,
        int miny,
        int maxy);

int chunked(
        float x);

void compute_chunk(
        WorkerItem *item);

void copy();

void create_chunk(
        Chunk *chunk,
        int p,
        int q);

void create_ghost(
        Player *p);

void create_window();

void cube(
        Block *b1,
        Block *b2,
        int fill);

void cylinder(
        Block *b1,
        Block *b2,
        int radius,
        int fill);

void delete_all_chunks();

void delete_all_players();

void delete_chunks();

void delete_ghost(
        Player *p);

void delete_player(
        int id);

void dirty_chunk(
        Chunk *chunk);

void draw_chunk(
        Attrib *attrib,
        Chunk *chunk);

void draw_cube(
        Attrib *attrib,
        GLuint buffer);

void draw_cube_offset(
        Attrib *attrib,
        GLuint buffer,
        int offset);

void draw_item(
        Attrib *attrib,
        GLuint buffer,
        int count);

void draw_lines(
        Attrib *attrib,
        GLuint buffer,
        int components,
        int count);

void draw_plant(
        Attrib *attrib,
        GLuint buffer);

void draw_player(
        Attrib *attrib,
        Player *player);

void draw_sign(
        Attrib *attrib,
        GLuint buffer,
        int length);

void draw_signs(
        Attrib *attrib,
        Chunk *chunk);

void draw_text(
        Attrib *attrib,
        GLuint buffer,
        int length);

void draw_triangles_2d(
        Attrib *attrib,
        GLuint buffer,
        int count);

void draw_triangles_3d(
        Attrib *attrib,
        GLuint buffer,
        int count);

void draw_triangles_3d_ao(
        Attrib *attrib,
        GLuint buffer,
        int count);

void draw_triangles_3d_text(
        Attrib *attrib,
        GLuint buffer,
        int count);

void ensure_chunks(
        Player *player);

void ensure_chunks_worker(
        Player *player,
        Worker *worker);

Chunk *find_chunk(
        int p,
        int q);

Player *find_player(
        int id);

void force_chunks(
        Player *player);

void gen_chunk_buffer(
        Chunk *chunk);

GLuint gen_crosshair_buffer();

GLuint gen_cube_buffer(
        float x,
        float y,
        float z,
        float n,
        int w);

GLuint gen_plant_buffer(
        float x,
        float y,
        float z,
        float n,
        int w);

void gen_sign_buffer(
        Chunk *chunk);

GLuint gen_sky_buffer();

GLuint gen_text_buffer(
        float x,
        float y,
        float n,
        char *text);

GLuint gen_wireframe_buffer(
        float x,
        float y,
        float z,
        float n);

void generate_chunk(
        Chunk *chunk,
        WorkerItem *item);

int get_block(
        int x,
        int y,
        int z);

int get_block_and_damage(
        int x,
        int y,
        int z,
        int *w,
        int *damage);

int get_block_damage(
        int x,
        int y,
        int z);

float get_daylight();

void get_motion_vector(
        int flying,
        int sz,
        int sx,
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz);

int get_scale_factor();

void get_sight_vector(
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz);

int ghost_id(
        int pid);

void handle_mouse_input();

void handle_movement(
        double dt);

int has_lights(
        Chunk *chunk);

int highest_block(
        float x,
        float z);

int hit_test(
        int previous,
        float x,
        float y,
        float z,
        float rx,
        float ry,
        int *bx,
        int *by,
        int *bz);

int hit_test_face(
        Player *player,
        int *x,
        int *y,
        int *z,
        int *face);

void init_chunk(
        Chunk *chunk,
        int p,
        int q);

int is_block_face_covered(
        int x,
        int y,
        int z,
        float nx,
        float ny,
        float nz);

void light_fill(
        char *opaque,
        char *light,
        int x,
        int y,
        int z,
        int w,
        int force);

void load_chunk(
        WorkerItem *item);

void login();

void map_set_func(
        int x,
        int y,
        int z,
        int w,
        void *arg);

void occlusion(
        char neighbors[27],
        char lights[27],
        float shades[27],
        float ao[6][4],
        float light[6][4]);

void on_char(
        GLFWwindow *window,
        unsigned int u);

void on_key(
        GLFWwindow *window,
        int key,
        int scancode,
        int action,
        int mods);

void on_left_click();

void on_light();

void on_middle_click();

void on_mouse_button(
        GLFWwindow *window,
        int button,
        int action,
        int mods);

void on_right_click();

void on_scroll(
        GLFWwindow *window,
        double xdelta,
        double ydelta);

void parse_buffer(
        char *buffer);

void parse_command(
        const char *buffer,
        int forward);

void paste();

int place_block();

Player *player_crosshair(
        Player *player);

float player_crosshair_distance(
        Player *p1,
        Player *p2);

int player_intersects_block(
        float x,
        float y,
        float z,
        float vx,
        float vy,
        float vz,
        int hx,
        int hy,
        int hz);

float player_player_distance(
        Player *p1,
        Player *p2);

void record_block(
        int x,
        int y,
        int z,
        int w);

void render_box_wireframe(
        Attrib *attrib,
        DebugBox *box,
        Player *p);

int render_chunks(
        Attrib *attrib,
        Player *player);

void render_crosshairs(
        Attrib *attrib);

void render_item(
        Attrib *attrib);

void render_players(
        Attrib *attrib,
        Player *player);

void render_players_hitboxes(
        Attrib *attrib,
        Player *p);

void render_sign(
        Attrib *attrib,
        Player *player);

void render_signs(
        Attrib *attrib,
        Player *player);

void render_sky(
        Attrib *attrib,
        Player *player,
        GLuint buffer);

void render_text(
        Attrib *attrib,
        int justify,
        float x,
        float y,
        float n,
        char *text);

void render_wireframe(
        Attrib *attrib,
        Player *player);

void request_chunk(
        int p,
        int q);

void reset_model();

void set_block(
        int x,
        int y,
        int z,
        int w);

void set_light(
        int p,
        int q,
        int x,
        int y,
        int z,
        int w);

void set_matrix_3d_state_view(
        float matrix[16],
        const State *s);

void set_sign(
        int x,
        int y,
        int z,
        int face,
        const char *text);

void sphere(
        Block *center,
        int radius,
        int fill,
        int fx,
        int fy,
        int fz);

float time_of_day();

void toggle_light(
        int x,
        int y,
        int z);

void tree(
        Block *block);

void unset_sign(
        int x,
        int y,
        int z);

void unset_sign_face(
        int x,
        int y,
        int z,
        int face);

int worker_run(
        void *arg);

void input_get_keys_view();

void input_get_keys_look(
        State *s,
        float dt);

void input_get_keys_walk(
        int *sx,
        int *sz);


#endif /*_game_h_*/
