#ifndef _game_h_
#define _game_h_


#include "GameModel.h"
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


enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2,
};


enum {
    MODE_OFFLINE = 0,
    MODE_ONLINE = 1,
};


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


int
_gen_sign_buffer(
        GLfloat *data,
        float x,
        float y,
        float z,
        int face,
        const char *text);

int
_hit_test(
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

void
_set_block(
        Model *g,
        int p,
        int q,
        int x,
        int y,
        int z,
        int w,
        int dirty);

void
_set_sign(
        Model *g,
        int p,
        int q,
        int x,
        int y,
        int z,
        int face,
        const char *text,
        int dirty);

int
add_block_damage(
        Model *g,
        int x,
        int y,
        int z,
        int damage);

void
add_message(
        Model *g,
        const char *text);

void
array(
        Model *g,
        Block *b1,
        Block *b2,
        int xc,
        int yc,
        int zc);

int
box_intersect_world(
        Model *g,
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez,
        int *cx,
        int *cy,
        int *cz);

float
box_sweep_world(
        Model *g,
        float x,
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

int
break_block(
        Model *g);

void
builder_block(
        Model *g,
        int x,
        int y,
        int z,
        int w);

float
calc_damage_from_impulse(
        Model *g,
        float d_vel);

void
check_workers(
        Model *g);

int
chunk_distance(
        Chunk *chunk,
        int p,
        int q);

int 
chunk_visible(
        const Model *g,
        float planes[6][4],
        int p,
        int q,
        int miny,
        int maxy);

int
chunked(
        float x);

void
compute_chunk(
        Model *g,
        WorkerItem *item);

void
copy(
        Model *g);

void
create_chunk(
        Model *g,
        Chunk *chunk,
        int p,
        int q);

void
cube(
        Model *g,
        Block *b1,
        Block *b2,
        int fill);

void
cylinder(
        Model *g,
        Block *b1,
        Block *b2,
        int radius,
        int fill);

void
delete_all_chunks(
        Model *g);

void
delete_all_players(
        Model *g);

void
delete_chunks(
        Model *g);

void
delete_player(
        Model *g,
        int id);

void
dirty_chunk(
        Model *g,
        Chunk *chunk);

void
draw_chunk(
        Attrib *attrib,
        Chunk *chunk);

void
draw_cube(
        Attrib *attrib,
        GLuint buffer);

void
draw_cube_offset(
        Attrib *attrib,
        GLuint buffer,
        int offset);

void
draw_item(
        Attrib *attrib,
        GLuint buffer,
        int count);

void
draw_lines(
        Attrib *attrib,
        GLuint buffer,
        int components,
        int count);

void
draw_plant(
        Attrib *attrib,
        GLuint buffer);

void
draw_player(
        Attrib *attrib,
        Player *player);

void
draw_sign(
        Attrib *attrib,
        GLuint buffer,
        int length);

void
draw_signs(
        Attrib *attrib,
        Chunk *chunk);

void
draw_text(
        Attrib *attrib,
        GLuint buffer,
        int length);

void
draw_triangles_2d(
        Attrib *attrib,
        GLuint buffer,
        int count);

void
draw_triangles_3d(
        Attrib *attrib,
        GLuint buffer,
        int count);

void
draw_triangles_3d_ao(
        Attrib *attrib,
        GLuint buffer,
        int count);

void
draw_triangles_3d_text(
        Attrib *attrib,
        GLuint buffer,
        int count);

void
ensure_chunks(
        Model *g,
        Player *player);

void
ensure_chunks_worker(
        Model *g,
        Player *player,
        Worker *worker);

Chunk *
find_chunk(
        Model *g,
        int p,
        int q);

Chunk *
find_chunk_xyz(
        Model *g,
        int x,
        int z);

Player *
find_player(
        Model *g,
        int id);

void
force_chunks(
        Model *g,
        Player *player);

void
gen_chunk_buffer(
        Model *g,
        Chunk *chunk);

GLuint
gen_crosshair_buffer();

GLuint
gen_cube_buffer(
        const Model *g,
        float x,
        float y,
        float z,
        float n,
        int w);

GLuint
gen_plant_buffer(
        float x,
        float y,
        float z,
        float n,
        int w);

void
gen_sign_buffer(
        Chunk *chunk);

GLuint
gen_sky_buffer();

GLuint
gen_text_buffer(
        float x,
        float y,
        float n,
        char *text);

GLuint
gen_wireframe_buffer(
        float x,
        float y,
        float z,
        float n);

void
generate_chunk(
        Chunk *chunk,
        WorkerItem *item);

int
get_block(
        Model *g,
        int x,
        int y,
        int z);

int
get_block_and_damage(
        Model *g,
        int x,
        int y,
        int z,
        int *w,
        int *damage);

int
get_block_damage(
        Model *g,
        int x,
        int y,
        int z);

float
get_daylight();

void
get_motion_vector(
        int flying,
        int sz,
        int sx,
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz);

int
get_scale_factor(
        Model *g);

void
get_sight_vector(
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz);

void
handle_mouse_input(
        Model *g);

void
handle_movement(
        Model *g,
        double dt);

int
has_lights(
        Model *g,
        Chunk *chunk);

int
highest_block(
        Model *g,
        float x,
        float z);

int
hit_test(
        Model *g,
        int previous,
        float x,
        float y,
        float z,
        float rx,
        float ry,
        int *bx,
        int *by,
        int *bz);

int
hit_test_face(
        Model *g,
        Player *player,
        int *x,
        int *y,
        int *z,
        int *face);

void
init_chunk(
        Model *g,
        Chunk *chunk,
        int p,
        int q);

int
is_block_face_covered(
        Model *g,
        int x,
        int y,
        int z,
        float nx,
        float ny,
        float nz);

void
light_fill(
        char *opaque,
        char *light,
        int x,
        int y,
        int z,
        int w,
        int force);

void
load_chunk(
        WorkerItem *item);

void
login();

void
map_set_func(
        int x,
        int y,
        int z,
        int w,
        void *arg);

void
occlusion(
        char neighbors[27],
        char lights[27],
        float shades[27],
        float ao[6][4],
        float light[6][4]);

void
on_left_click(
        Model *g);

void
on_light(
        Model *g);

void
on_middle_click(
        Model *g);

void
on_right_click();

void
parse_buffer(
        Model *g,
        char *buffer);

void
parse_command(
        Model *g,
        const char *buffer,
        int forward);

void
paste(
        Model *g);

int
place_block(
        Model *g);

Player *
player_crosshair(
        Model *g,
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

float
player_player_distance(
        Player *p1,
        Player *p2);

void
record_block(
        Model *g,
        int x,
        int y,
        int z,
        int w);

void
render_box_wireframe(
        Model *g,
        Attrib *attrib,
        DebugBox *box,
        Player *p);

int
render_chunks(
        Model *g,
        Attrib *attrib,
        Player *player);

void
render_crosshairs(
        Model *g,
        Attrib *attrib);

void
render_item(
        Model *g,
        Attrib *attrib);

void
render_players(
        Model *g,
        Attrib *attrib,
        Player *player);

void
render_players_hitboxes(
        Model *g,
        Attrib *attrib,
        Player *p);

void
render_sign(
        Model *g,
        Attrib *attrib,
        Player *player);

void
render_signs(
        Model *g,
        Attrib *attrib,
        Player *player);

void
render_sky(
        Model *g,
        Attrib *attrib,
        Player *player,
        GLuint buffer);

void
render_text(
        Model *g,
        Attrib *attrib,
        int justify,
        float x,
        float y,
        float n,
        char *text);

void
render_wireframe(
        Model *g,
        Attrib *attrib,
        Player *player);

void
request_chunk(
        int p,
        int q);

void
reset_model(
        Model *g);

void
set_block(
        Model *g,
        int x,
        int y,
        int z,
        int w);

void
set_light(
        Model *g,
        int p,
        int q,
        int x,
        int y,
        int z,
        int w);

void
set_matrix_3d_state_view(
        float matrix[16],
        const State *s);

void
set_sign(
        Model *g,
        int x,
        int y,
        int z,
        int face,
        const char *text);

void
sphere(
        Model *g,
        Block *center,
        int radius,
        int fill,
        int fx,
        int fy,
        int fz);

float
time_of_day(
        Model *g);

void
toggle_light(
        Model *g,
        int x,
        int y,
        int z);

void
tree(
        Model *g,
        Block *block);

void
unset_sign(
        Model *g,
        int x,
        int y,
        int z);

void
unset_sign_face(
        Model *g,
        int x,
        int y,
        int z,
        int face);

int
worker_run(
        void *arg);

void
input_get_keys_view(
        Model *g);

void
input_get_keys_look(
        Model *g,
        State *s,
        float dt);

void
input_get_keys_walk(
        Model *g,
        int *sx,
        int *sz);

float
input_player_fly(
        Model *g);

float
input_player_jump_or_fly(
        Model *g,
        Player *p);

void
set_matrix_3d_player_camera(
        Model *g,
        float matrix[16],
        const Player *p);


#endif /*_game_h_*/
