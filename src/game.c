#include "auth.h"
#include "client.h"
#include "config.h"
#include "cube.h"
#include "db.h"
#include "game.h"
#include "hitbox.h"
#include "blocks.h"
#include "map.h"
#include "matrix.h"
#include "noise.h"
#include "player.h"
#include "sign.h"
#include "texturedBox.h"
#include "tinycthread.h"
#include "util.h"
#include "world.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Convert a value in block space to chunk space
// Arguments:
// - x: block coordinate
// Returns:
// - chunk coordinate the block coordinate is in
int chunked(
        float x)
{
    return floorf(roundf(x) / CHUNK_SIZE);
}


// Get the current time of day
// Depends on glfwGetTime()
// Arguments: none
// Returns:
// - time value between 0.0 and 1.0
float time_of_day(
        Model *g)
{
    if (g->day_length <= 0) {
        return 0.5;
    }
    float t;
    t = glfwGetTime();
    t = t / g->day_length;
    t = t - (int)t;
    return t;
}


// Arguments: none
// Returns:
// - daylight value
float get_daylight(
        Model *g)
{
    float timer = time_of_day(g);
    if (timer < 0.5) {
        float t = (timer - 0.25) * 100;
        return 1 / (1 + powf(2, -t));
    }
    else {
        float t = (timer - 0.85) * 100;
        return 1 - 1 / (1 + powf(2, -t));
    }
}


// Note: depends on window size and frame buffer size
// Arguments: none
// Returns:
// - scale factor
int get_scale_factor(
        Model *g)
{
    int window_width, window_height;
    int buffer_width, buffer_height;
    glfwGetWindowSize(g->window, &window_width, &window_height);
    glfwGetFramebufferSize(g->window, &buffer_width, &buffer_height);
    int result = buffer_width / window_width;
    result = MAX(1, result);
    result = MIN(2, result);
    return result;
}


// Arguments:
// - rx: rotation x
// - ry: rotation y
// Returns:
// - vx: vector x
// - vy: vector y
// - vz: vector z
void get_sight_vector(
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz)
{
    float m = cosf(ry);
    *vx = cosf(rx - RADIANS(90)) * m;
    *vy = sinf(ry);
    *vz = sinf(rx - RADIANS(90)) * m;
}


// Get the motion vector for a player's state
// Arguments:
// - flying: whether the player is flying
// - sz: strafe z
// - sx: strafe x
// - rx: rotation x
// - ry: rotation y
// Returns:
// - vx: vector x
// - vy: vector y
// - vz: vector z
void get_motion_vector(
        int flying,
        int sz,
        int sx,
        float rx,
        float ry,
        float *vx,
        float *vy,
        float *vz)
{
    *vx = 0; *vy = 0; *vz = 0;
    if (!sz && !sx) {
        return;
    }
    float strafe = atan2f(sz, sx);
    if (flying) {
        // Flying motion
        float m = cosf(ry);
        float y = sinf(ry);
        if (sx) {
            if (!sz) {
                y = 0;
            }
            m = 1;
        }
        if (sz > 0) {
            y = -y;
        }
        *vx = cosf(rx + strafe) * m;
        *vy = y;
        *vz = sinf(rx + strafe) * m;
    }
    else {
        // Walking motion
        *vx = cosf(rx + strafe);
        *vy = 0;
        *vz = sinf(rx + strafe);
    }
}


// Generate the position buffer for the crosshairs in the middle of the screen.
// Arguments: none
// Returns:
// - OpenGL buffer handle
GLuint gen_crosshair_buffer(
        Model *g)
{
    int x = g->width / 2;
    int y = g->height / 2;
    int p = 10 * g->scale;
    float data[] = {
        x, y - p, x, y + p,
        x - p, y, x + p, y
    };
    return gen_buffer(sizeof(data), data);
}


// Create a new cube wireframe buffer
// Arguments:
// - x: cube x position
// - y: cube y position
// - z: cube z position
// - n: cube scale, distance from center to faces
// Returns:
// - OpenGL buffer handle
GLuint gen_wireframe_buffer(
        float x,
        float y,
        float z,
        float n)
{
    // (6 faces)*(4 points)*(3 dimensions) = 72 floats
    float data[72];
    make_cube_wireframe(data, x, y, z, n);
    return gen_buffer(sizeof(data), data);
}


// Arguments:
// - x: box center x position
// - y: box center y position
// - z: box center z position
// - ex: box x extent
// - ey: box y extent
// - ez: box z extent
// Returns:
// - OpenGL buffer handle
GLuint gen_box_wireframe_buffer(
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez)
{
    // (6 faces)*(4 points)*(3 dimensions) = 72 floats
    float data[72];
    make_box_wireframe(data, x, y, z, ex, ey, ez);
    return gen_buffer(sizeof(data), data);
}


// Create the sky buffer (sphere shape)
// Arguments: none
// Returns: OpenGL buffer handle
GLuint gen_sky_buffer()
{
    // The size of this data array should match the detail parameter in make_sphere()
    float data[12288];
    make_sphere(data, 1, 3);
    return gen_buffer(sizeof(data), data);
}


// Create a new cube buffer
// Arguments:
// - x: cube x position
// - y: cube y position
// - z: cube z position
// - n: cube scale, distance from center to faces
// - w: block id for textures
// Returns:
// - OpenGL buffer handle
GLuint gen_cube_buffer(
        float x,
        float y,
        float z,
        float n,
        int w)
{
    // Each face has 10 component float properties.
    // A cube model has 6 faces
    GLfloat *data = malloc_faces(10, 6);
    float ao[6][4] = {0};
    float light[6][4] = {
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5}
    };
    make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
    return gen_faces(10, 6, data);
}


// Generate a buffer for a plant block model at a given location
// Arguments:
// - x: block x position
// - y: block y position
// - z: block z position
// - n: scale, distance from center to rectangle edge
// - w: plant block type
// Returns:
// - OpenGL buffer handle
GLuint gen_plant_buffer(
        float x,
        float y,
        float z,
        float n,
        int w)
{
    // Each face has 10 component float properties.
    // A plant model has 4 faces because there are 2 squares each with 2 sides
    GLfloat *data = malloc_faces(10, 4);
    float ao = 0;
    float light = 1;
    make_plant(data, ao, light, x, y, z, n, w, 45);
    return gen_faces(10, 4, data);
}

// Create a 2D screen model for a text string
// Arguments:
// - x: screen x
// - y: screen y
// - n: scale
// - text: text data to be displayed
// Returns:
// - OpenGL buffer handle
GLuint gen_text_buffer(
        float x,
        float y,
        float n,
        char *text)
{
    int length = strlen(text);
    GLfloat *data = malloc_faces(4, length);
    for (int i = 0; i < length; i++) {
        // Multiply by 24 because there are 24 properties per character
        make_character(data + i * 24, x, y, n / 2, n, text[i]);
        x += n;
    }
    return gen_faces(4, length, data);
}

// Draws 3D triangle models
// Arguments:
// - attrib: attributes to be used for rendering the triangles
// - buffer: triangles data
// - count: number of triangles
// Returns: none
void draw_triangles_3d_ao(
        Attrib *attrib,
        GLuint buffer,
        int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, 0);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_cube_offset(
        Attrib *attrib,
        GLuint buffer,
        int offset)
{
    int count = 36;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, 0);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, offset, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw triangles for 3D text
// Arguments:
// - attrib: attributes to be used for rendering
// - buffer
// - count: number of triangles
// Returns: none
void draw_triangles_3d_text(
        Attrib *attrib,
        GLuint buffer,
        int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, (GLvoid *)(sizeof(GLfloat) * 3));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw 3D (textured) triangle models
// Arguments:
// - attrib: attributes to be used for rendering
// - buffer
// - count
// Returns: none
void draw_triangles_3d(
        Attrib *attrib,
        GLuint buffer,
        int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 8, 0);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw 2D (textured) triangle models
// Arguments:
// - attrib: attributes to be used for rendering
// - buffer
// - count: number of triangles
// Returns: none
void draw_triangles_2d(
        Attrib *attrib,
        GLuint buffer,
        int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, 0);
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, (GLvoid *)(sizeof(GLfloat) * 2));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw lines
// Arguments:
// - attrib: attributes to be used for rendering
// - buffer
// - components
// - count
// Returns: none
void draw_lines(
        Attrib *attrib,
        GLuint buffer,
        int components,
        int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glVertexAttribPointer(
            attrib->position, components, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw a game chunk (of blocks)
// Arguments:
// Returns: none
void draw_chunk(
        Attrib *attrib,
        Chunk *chunk)
{
    draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
}

// Draw a block (item), which can be a plant shape or a cube shape
// Arguments:
// Returns: none
void draw_item(
        Attrib *attrib,
        GLuint buffer,
        int count)
{
    draw_triangles_3d_ao(attrib, buffer, count);
}

// Draw 2D text
// Arguments:
// Returns: none
void draw_text(
        Attrib *attrib,
        GLuint buffer,
        int length)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_triangles_2d(attrib, buffer, length * 6);
    glDisable(GL_BLEND);
}

// Draw the signs in a given chunk
// Arguments:
// Returns: none
void draw_signs(
        Attrib *attrib,
        Chunk *chunk)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(attrib, chunk->sign_buffer, chunk->sign_faces * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

// Draw a single sign model
// Arguments:
// Returns: none
void draw_sign(
        Attrib *attrib,
        GLuint buffer,
        int length)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(attrib, buffer, length * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

// Draw a cube block model
// Arguments:
// Returns: none
void draw_cube(
        Attrib *attrib,
        GLuint buffer)
{
    draw_item(attrib, buffer, 36);
}

// Draw a plant block model
// Arguments:
// Returns: none
void draw_plant(
        Attrib *attrib,
        GLuint buffer)
{
    draw_item(attrib, buffer, 24);
}

// Draw a player model
// Arguments:
// - attrib: attributes to be used for rendering
// - player
// Returns: none
void draw_player(
        Attrib *attrib,
        Player *player)
{
    const int offset = 36;
    const int num_parts = 6;  // for player body
    for (unsigned int i = 0; i < num_parts; i++) {
        draw_cube_offset(attrib, player->buffer, i * offset);
    }
}

// Find a player with a certain id
// Arguments:
// - id: player id of the player to find
// Returns:
// - pointer to the player in the game model
Player *find_player(
        Model *g,
        int id)
{
    for (int i = 0; i < g->player_count; i++) {
        Player *player = g->players + i;
        if (player->id == id) {
            return player;
        }
    }
    return 0;
}

// Look for a player with the given id and remove that player's data
// Arguments:
// - id: id of the player to delete
// Returns: none
void delete_player(
        Model *g,
        int id)
{
    Player *player = find_player(g, id);
    if (!player) {
        return;
    }
    int count = g->player_count;
    del_buffer(player->buffer);
    Player *other = g->players + (--count);
    memcpy(player, other, sizeof(Player));
    g->player_count = count;
}

// Delete all of the current players
// Arguments: none
// Returns: none
void
delete_all_players(
        Model *g)
{
    for (int i = 0; i < g->player_count; i++) {
        Player *player = g->players + i;
        del_buffer(player->buffer);
    }
    g->player_count = 0;
}

// Get the distance between 2 players.
// Arguments:
// - p1: a player
// - p2: another player
// Returns:
// - distance
float player_player_distance(
        Player *p1,
        Player *p2)
{
    State *s1 = &p1->state;
    State *s2 = &p2->state;
    return v3_mag(s2->x - s1->x, s2->y - s1->y, s2->z - s1->z);
}

// Get the distance between where p1 is looking and p2's position.
// Arguments:
// - p1
// - p2
// Returns:
// - distance
float player_crosshair_distance(
        Player *p1,
        Player *p2)
{
    State *s1 = &p1->state;
    State *s2 = &p2->state;
    float d = player_player_distance(p1, p2);
    float vx, vy, vz;
    get_sight_vector(s1->rx, s1->ry, &vx, &vy, &vz);
    vx *= d; vy *= d; vz *= d;
    float px, py, pz;
    px = s1->x + vx; py = s1->y + vy; pz = s1->z + vz;
    float x = s2->x - px;
    float y = s2->y - py;
    float z = s2->z - pz;
    return sqrtf(x * x + y * y + z * z);
}

// Find the player that the given player is looking at.
// Arguments:
// - player: the given player
// Returns:
// - the closest player that is in range and that the given player is looking
//   near.
Player *
player_crosshair(
        Model *g,
        Player *player)
{
    Player *result = 0;
    float threshold = RADIANS(5);
    float best = 0;
    for (int i = 0; i < g->player_count; i++) {
        Player *other = g->players + i;
        if (other == player) {
            continue;
        }
        float p = player_crosshair_distance(player, other);
        float d = player_player_distance(player, other);
        if (d < 96 && p / d < threshold) {
            if (best == 0 || d < best) {
                best = d;
                result = other;
            }
        }
    }
    return result;
}

// Try to find a chunk with at specific chunk coordinates
// Arguments:
// - p: chunk x
// - q: chunk z
// Returns:
// - chunk pointer
Chunk *
find_chunk(
        Model *g,
        int p,
        int q)
{
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk->p == p && chunk->q == q) {
            return chunk;
        }
    }
    return NULL;
}

// Get the "distance" of a chunk from the given chunk coordinates.
// Arguments:
// - chunk: chunk to test distance the distance from (p, q)
// - p: chunk x
// - q: chunk z
// Returns:
// - distance in chunk space, where "distance" means the maximum value of the
//   change in p and the change in q.
int chunk_distance(
        Chunk *chunk,
        int p,
        int q)
{
    int dp = ABS(chunk->p - p);
    int dq = ABS(chunk->q - q);
    return MAX(dp, dq);
}

// Predicate function to determine if a chunk is visible within the given
// frustrum planes.
// Arguments:
// - planes: frustrum planes to check if the chunk is visible within
// - p: chunk x coordinate
// - q: chunk z coordinate
// - miny: minimum block y coordinate in the chunk
// - maxy: maximum block y coordinate in the chunk
// Returns:
// - non-zero if the chunk is visible
int chunk_visible(
        const Model *g,
        float planes[6][4],
        int p,
        int q,
        int miny,
        int maxy)
{
    int x = p * CHUNK_SIZE - 1;
    int z = q * CHUNK_SIZE - 1;
    int d = CHUNK_SIZE + 1;
    float points[8][3] = {
        {x + 0, miny, z + 0},
        {x + d, miny, z + 0},
        {x + 0, miny, z + d},
        {x + d, miny, z + d},
        {x + 0, maxy, z + 0},
        {x + d, maxy, z + 0},
        {x + 0, maxy, z + d},
        {x + d, maxy, z + d}
    };
    int n = g->ortho ? 4 : 6;
    for (int i = 0; i < n; i++) {
        int in = 0;
        int out = 0;
        for (int j = 0; j < 8; j++) {
            float d =
                planes[i][0] * points[j][0] +
                planes[i][1] * points[j][1] +
                planes[i][2] * points[j][2] +
                planes[i][3];
            if (d < 0) {
                out++;
            }
            else {
                in++;
            }
            if (in && out) {
                break;
            }
        }
        if (in == 0) {
            return 0;
        }
    }
    return 1;
}

// Find the highest y position of a block at a given (x,z) position.
// Arguments:
// - x
// - z
// Returns:
// - highest y value, or -1 if not found
int highest_block(
        Model *g,
        float x,
        float z)
{
    int result = -1;
    int nx = roundf(x);
    int nz = roundf(z);
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        Map *map = &chunk->map;
        MAP_FOR_EACH(map, ex, ey, ez, ew) {
            if (block_is_obstacle(g, ew) && ex == nx && ez == nz) {
                result = MAX(result, ey);
            }
        } END_MAP_FOR_EACH;
    }
    return result;
}

// Finds the closest block in a map found by casting a hit ray.
// Meant to be called by hit_test().
// Arguments:
// - map: map to search for hitting blocks in
// - max_distance
// - previous: boolean
// - x: ray start x
// - y: ray start y
// - z: ray start z
// - vx: ray x direction
// - vy: ray y direction
// - vz: ray z direction
// - hx: pointer to output hit x position
// - hy: pointer to output hit y position
// - hz: pointer to output hit z position
// Returns:
// - the block type that was hit, returns 0 if there was no block found
// - writes hit output to hx, hy, and hz
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
        int *hz)
{
    int m = 32;
    int px = 0;
    int py = 0;
    int pz = 0;
    for (int i = 0; i < max_distance * m; i++) {
        int nx = roundf(x);
        int ny = roundf(y);
        int nz = roundf(z);
        if (nx != px || ny != py || nz != pz) {
            int hw = map_get(map, nx, ny, nz);
            if (hw > 0) {
                if (previous) {
                    *hx = px; *hy = py; *hz = pz;
                }
                else {
                    *hx = nx; *hy = ny; *hz = nz;
                }
                return hw;
            }
            px = nx; py = ny; pz = nz;
        }
        x += vx / m; y += vy / m; z += vz / m;
    }
    return 0;
}

// Finds the closest block found by casting a hit ray.
// Arguments:
// - previous: boolean
// - x: ray start x
// - y: ray start y
// - z: ray start z
// - rx: ray x direction
// - ry: ray y direction
// - bx: pointer to output hit x position
// - by: pointer to output hit y position
// - bz: pointer to output hit z position
// Returns:
// - the block type that was hit, returns 0 if no block was found
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
        int *bz)
{
    //const int r = 8; // radius (blocks) (max length)
    const float r = g->players[0].attrs.reach; // radius (blocks) (max length)
    int result = 0;
    float best = 0;
    int p = chunked(x);
    int q = chunked(z);
    float vx, vy, vz;
    get_sight_vector(rx, ry, &vx, &vy, &vz);
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk_distance(chunk, p, q) > 1+chunked(r)) {
            continue;
        }
        int hx, hy, hz;
        int hw = _hit_test(&chunk->map, r, previous,
                x, y, z, vx, vy, vz, &hx, &hy, &hz);
        if (hw > 0) {
            float d = sqrtf(
                    powf(hx - x, 2) + powf(hy - y, 2) + powf(hz - z, 2));
            if (best == 0 || d < best) {
                best = d;
                *bx = hx; *by = hy; *bz = hz;
                result = hw;
            }
        }
    }
    return result;
}

// See which block face a player is looking at
// Arguments:
// - player
// - x: output pointer to the x position of the hit block
// - y: output pointer to the y position of the hit block
// - z: output pointer to the z position of the hit block
// - face: output pointer to the face number of the hit block
// Returns:
// - non-zero if a face was hit
int hit_test_face(
        Model *g,
        Player *player,
        int *x,
        int *y,
        int *z,
        int *face)
{
    State *s = &player->state;
    float eye_y = player_eye_y(s->y);
    int w = hit_test(g, 0, s->x, eye_y, s->z, s->rx, s->ry, x, y, z);
    if (block_is_obstacle(g, w)) {
        int hx, hy, hz;
        hit_test(g, 1, s->x, eye_y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        int dx = hx - *x;
        int dy = hy - *y;
        int dz = hz - *z;
        if (dx == -1 && dy == 0 && dz == 0) {
            *face = 0; return 1;
        }
        if (dx == 1 && dy == 0 && dz == 0) {
            *face = 1; return 1;
        }
        if (dx == 0 && dy == 0 && dz == -1) {
            *face = 2; return 1;
        }
        if (dx == 0 && dy == 0 && dz == 1) {
            *face = 3; return 1;
        }
        if (dx == 0 && dy == 1 && dz == 0) {
            int degrees = roundf(DEGREES(atan2f(s->x - hx, s->z - hz)));
            if (degrees < 0) {
                degrees += 360;
            }
            int top = ((degrees + 45) / 90) % 4;
            *face = 4 + top; return 1;
        }
    }
    return 0;
}


// Function to return whether a player position instersects the given
// block position.
int player_intersects_block(  // Returns non-zero if the player intersects the given block position
        float x,              // player x
        float y,              // player y
        float z,              // player z
        float vx,             // player velocity x
        float vy,             // player velocity y
        float vz,             // player velocity z
        int bx,               // block x
        int by,               // block y
        int bz)               // block z
{
    float ex, ey, ez;
    player_hitbox_extent(&ex, &ey, &ez);
    return box_intersect_block(x, y, z, ex, ey, ez, bx, by, bz);
}


// Generate the buffer data for a single sign model
// Arguments:
// - data: pointer to write the data to
// - x
// - y
// - z
// - face
// - text: sign ASCII text string (null-terminated)
// Returns:
// - number of character faces
int _gen_sign_buffer(
        GLfloat *data,
        float x,
        float y,
        float z,
        int face,
        const char *text)
{
    static const int glyph_dx[8] = { 0,  0, -1,  1, 1,  0, -1,  0 };
    static const int glyph_dz[8] = { 1, -1,  0,  0, 0, -1,  0,  1 };
    static const int line_dx[8] = { 0,  0,  0,  0, 0,  1,  0, -1 };
    static const int line_dy[8] = {-1, -1, -1, -1, 0,  0,  0,  0 };
    static const int line_dz[8] = { 0,  0,  0,  0, 1,  0, -1,  0 };
    if (face < 0 || face >= 8) {
        return 0;
    }
    int count = 0;
    float max_width = 64;
    float line_height = 1.25;
    char lines[1024];
    int rows = wrap(text, max_width, lines, 1024);
    rows = MIN(rows, 5);
    int dx = glyph_dx[face];
    int dz = glyph_dz[face];
    int ldx = line_dx[face];
    int ldy = line_dy[face];
    int ldz = line_dz[face];
    float n = 1.0 / (max_width / 10);
    float sx = x - n * (rows - 1) * (line_height / 2) * ldx;
    float sy = y - n * (rows - 1) * (line_height / 2) * ldy;
    float sz = z - n * (rows - 1) * (line_height / 2) * ldz;
    char *key;
    char *line = tokenize(lines, "\n", &key);
    while (line) {
        int length = strlen(line);
        int line_width = string_width(line);
        line_width = MIN(line_width, max_width);
        float rx = sx - dx * line_width / max_width / 2;
        float ry = sy;
        float rz = sz - dz * line_width / max_width / 2;
        for (int i = 0; i < length; i++) {
            int width = char_width(line[i]);
            line_width -= width;
            if (line_width < 0) {
                break;
            }
            rx += dx * width / max_width / 2;
            rz += dz * width / max_width / 2;
            if (line[i] != ' ') {
                make_character_3d(
                        data + count * 30, rx, ry, rz, n / 2, face, line[i]);
                count++;
            }
            rx += dx * width / max_width / 2;
            rz += dz * width / max_width / 2;
        }
        sx += n * line_height * ldx;
        sy += n * line_height * ldy;
        sz += n * line_height * ldz;
        line = tokenize(NULL, "\n", &key);
        rows--;
        if (rows <= 0) {
            break;
        }
    }
    return count;
}


// Create the game's sign buffer for a chunk
// Arguments:
// - chunk: the chunk to generate the sign models for
// Returns: none
void gen_sign_buffer(
        Chunk *chunk)
{
    SignList *signs = &chunk->signs;

    // first pass - count characters
    int max_faces = 0;
    for (unsigned i = 0; i < signs->size; i++) {
        Sign *e = signs->data + i;
        max_faces += strlen(e->text);
    }

    // second pass - generate geometry
    GLfloat *data = malloc_faces(5, max_faces);
    int faces = 0;
    for (unsigned i = 0; i < signs->size; i++) {
        Sign *e = signs->data + i;
        faces += _gen_sign_buffer(
                data + faces * 30, e->x, e->y, e->z, e->face, e->text);
    }

    del_buffer(chunk->sign_buffer);
    chunk->sign_buffer = gen_faces(5, faces, data);
    chunk->sign_faces = faces;
}


// Predicate function for whether a given chunk has any block light values
// Arguments:
// - chunk: the chunk to check
// Returns:
// - non-zero if the chunk has lights
int has_lights(
        Model *g,
        Chunk *chunk)
{
    if (!SHOW_LIGHTS) {
        return 0;
    }
    for (int dp = -1; dp <= 1; dp++) {
        for (int dq = -1; dq <= 1; dq++) {
            Chunk *other = chunk;
            if (dp || dq) {
                other = find_chunk(g, chunk->p + dp, chunk->q + dq);
            }
            if (!other) {
                continue;
            }
            Map *map = &other->lights;
            if (map->size) {
                return 1;
            }
        }
    }
    return 0;
}


// Mark a chunk as dirty and also potentially mark the neighboring chunks
// Arguments:
// - chunk: chunk to mark as dirty
void dirty_chunk(
        Model *g,
        Chunk *chunk)
{
    chunk->dirty = 1;
    if (has_lights(g, chunk)) {
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk *other = find_chunk(g, chunk->p + dp, chunk->q + dq);
                if (other) {
                    other->dirty = 1;
                }
            }
        }
    }
}


// Arguments:
// - neighbors
// - lights
// - shades
// - ao
// - light
// Returns: none
void occlusion(
        char neighbors[27],
        char lights[27],
        float shades[27],
        float ao[6][4],
        float light[6][4])
{
    static const int lookup3[6][4][3] = {
        {{0, 1, 3}, {2, 1, 5}, {6, 3, 7}, {8, 5, 7}},
        {{18, 19, 21}, {20, 19, 23}, {24, 21, 25}, {26, 23, 25}},
        {{6, 7, 15}, {8, 7, 17}, {24, 15, 25}, {26, 17, 25}},
        {{0, 1, 9}, {2, 1, 11}, {18, 9, 19}, {20, 11, 19}},
        {{0, 3, 9}, {6, 3, 15}, {18, 9, 21}, {24, 15, 21}},
        {{2, 5, 11}, {8, 5, 17}, {20, 11, 23}, {26, 17, 23}}
    };
    static const int lookup4[6][4][4] = {
        {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}, {4, 5, 7, 8}},
        {{18, 19, 21, 22}, {19, 20, 22, 23}, {21, 22, 24, 25}, {22, 23, 25, 26}},
        {{6, 7, 15, 16}, {7, 8, 16, 17}, {15, 16, 24, 25}, {16, 17, 25, 26}},
        {{0, 1, 9, 10}, {1, 2, 10, 11}, {9, 10, 18, 19}, {10, 11, 19, 20}},
        {{0, 3, 9, 12}, {3, 6, 12, 15}, {9, 12, 18, 21}, {12, 15, 21, 24}},
        {{2, 5, 11, 14}, {5, 8, 14, 17}, {11, 14, 20, 23}, {14, 17, 23, 26}}
    };
    static const float curve[4] = {0.0, 0.25, 0.5, 0.75};
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int corner = neighbors[lookup3[i][j][0]];
            int side1 = neighbors[lookup3[i][j][1]];
            int side2 = neighbors[lookup3[i][j][2]];
            int value = side1 && side2 ? 3 : corner + side1 + side2;
            float shade_sum = 0;
            float light_sum = 0;
            int is_light = lights[13] == 15;
            for (int k = 0; k < 4; k++) {
                shade_sum += shades[lookup4[i][j][k]];
                light_sum += lights[lookup4[i][j][k]];
            }
            if (is_light) {
                light_sum = 15 * 4 * 10;
            }
            float total = curve[value] + shade_sum / 4.0;
            ao[i][j] = MIN(total, 1.0);
            light[i][j] = light_sum / 15.0 / 4.0;
        }
    }
}


#define XZ_SIZE (CHUNK_SIZE * 3 + 2)
#define XZ_LO (CHUNK_SIZE)
#define XZ_HI (CHUNK_SIZE * 2 + 1)
#define Y_SIZE 258
#define XYZ(x, y, z) ((y) * XZ_SIZE * XZ_SIZE + (x) * XZ_SIZE + (z))
#define XZ(x, z) ((x) * XZ_SIZE + (z))


// Arguments:
// - opaque
// - light
// - x
// - y
// - z
// - w
// - force
// Returns: none
void light_fill(
        char *opaque,
        char *light,
        int x,
        int y,
        int z,
        int w,
        int force)
{
    if (x + w < XZ_LO || z + w < XZ_LO) {
        return;
    }
    if (x - w > XZ_HI || z - w > XZ_HI) {
        return;
    }
    if (y < 0 || y >= Y_SIZE) {
        return;
    }
    if (light[XYZ(x, y, z)] >= w) {
        return;
    }
    if (!force && opaque[XYZ(x, y, z)]) {
        return;
    }
    light[XYZ(x, y, z)] = w--;
    light_fill(opaque, light, x - 1, y, z, w, 0);
    light_fill(opaque, light, x + 1, y, z, w, 0);
    light_fill(opaque, light, x, y - 1, z, w, 0);
    light_fill(opaque, light, x, y + 1, z, w, 0);
    light_fill(opaque, light, x, y, z - 1, w, 0);
    light_fill(opaque, light, x, y, z + 1, w, 0);
}


// Arguments:
// - item
// Returns: none
void compute_chunk(
        WorkerItem *item)
{
    char *opaque = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char *light = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char *highest = (char *)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

    int ox = item->p * CHUNK_SIZE - CHUNK_SIZE - 1;
    int oy = -1;
    int oz = item->q * CHUNK_SIZE - CHUNK_SIZE - 1;

    // check for lights
    int has_light = 0;
    if (SHOW_LIGHTS) {
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                Map *map = item->light_maps[a][b];
                if (map && map->size) {
                    has_light = 1;
                }
            }
        }
    }

    // populate opaque array
    for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 3; b++) {
            Map *map = item->block_maps[a][b];
            if (!map) {
                continue;
            }
            MAP_FOR_EACH(map, ex, ey, ez, ew) {
                int x = ex - ox;
                int y = ey - oy;
                int z = ez - oz;
                int w = ew;
                // TODO: this should be unnecessary
                if (x < 0 || y < 0 || z < 0) {
                    continue;
                }
                if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE) {
                    continue;
                }
                // END TODO
                //opaque[XYZ(x, y, z)] = !block_is_transparent(g, w); // TODO actually check this
                opaque[XYZ(x, y, z)] = 1;
                if (opaque[XYZ(x, y, z)]) {
                    highest[XZ(x, z)] = MAX(highest[XZ(x, z)], y);
                }
            } END_MAP_FOR_EACH;
        }
    }

    // flood fill light intensities
    if (has_light) {
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                Map *map = item->light_maps[a][b];
                if (!map) {
                    continue;
                }
                MAP_FOR_EACH(map, ex, ey, ez, ew) {
                    int x = ex - ox;
                    int y = ey - oy;
                    int z = ez - oz;
                    light_fill(opaque, light, x, y, z, ew, 1);
                } END_MAP_FOR_EACH;
            }
        }
    }

    Map *map = item->block_maps[1][1];

    // count exposed faces
    int miny = 256;
    int maxy = 0;
    int faces = 0;
    MAP_FOR_EACH(map, ex, ey, ez, ew) {
        if (ew <= 0) {
            continue;
        }
        int x = ex - ox;
        int y = ey - oy;
        int z = ez - oz;
        int f1 = !opaque[XYZ(x - 1, y, z)];
        int f2 = !opaque[XYZ(x + 1, y, z)];
        int f3 = !opaque[XYZ(x, y + 1, z)];
        int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
        int f5 = !opaque[XYZ(x, y, z - 1)];
        int f6 = !opaque[XYZ(x, y, z + 1)];
        int total = f1 + f2 + f3 + f4 + f5 + f6;
        if (total == 0) {
            continue;
        }
        if (block_is_plant(item->game_model, ew)) {
            total = 4;
        }
        miny = MIN(miny, ey);
        maxy = MAX(maxy, ey);
        faces += total;
    } END_MAP_FOR_EACH;

    // generate geometry
    GLfloat *data = malloc_faces(10, faces);
    int offset = 0;
    MAP_FOR_EACH(map, ex, ey, ez, ew) {
        if (ew <= 0) {
            continue;
        }
        int x = ex - ox;
        int y = ey - oy;
        int z = ez - oz;
        int f1 = !opaque[XYZ(x - 1, y, z)];
        int f2 = !opaque[XYZ(x + 1, y, z)];
        int f3 = !opaque[XYZ(x, y + 1, z)];
        int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
        int f5 = !opaque[XYZ(x, y, z - 1)];
        int f6 = !opaque[XYZ(x, y, z + 1)];
        int total = f1 + f2 + f3 + f4 + f5 + f6;
        if (total == 0) {
            continue;
        }
        char neighbors[27] = {0};
        char lights[27] = {0};
        float shades[27] = {0};
        int index = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
                    lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
                    shades[index] = 0;
                    if (y + dy <= highest[XZ(x + dx, z + dz)]) {
                        for (int oy = 0; oy < 8; oy++) {
                            if (opaque[XYZ(x + dx, y + dy + oy, z + dz)]) {
                                shades[index] = 1.0 - oy * 0.125;
                                break;
                            }
                        }
                    }
                    index++;
                }
            }
        }
        float ao[6][4];
        float light[6][4];
        occlusion(neighbors, lights, shades, ao, light);
        if (block_is_plant(item->game_model, ew)) {
            total = 4;
            float min_ao = 1;
            float max_light = 0;
            for (int a = 0; a < 6; a++) {
                for (int b = 0; b < 4; b++) {
                    min_ao = MIN(min_ao, ao[a][b]);
                    max_light = MAX(max_light, light[a][b]);
                }
            }
            float rotation = simplex2(ex, ez, 4, 0.5, 2) * 360;
            make_plant(
                    data + offset, min_ao, max_light,
                    ex, ey, ez, 0.5, ew, rotation);
        }
        else {
            make_cube(
                    data + offset, ao, light,
                    f1, f2, f3, f4, f5, f6,
                    ex, ey, ez, 0.5, ew);
        }
        offset += total * 60;
    } END_MAP_FOR_EACH;

    free(opaque);
    free(light);
    free(highest);

    item->miny = miny;
    item->maxy = maxy;
    item->faces = faces;
    item->data = data;
}


// Arguments:
// - chunk
// - item
// Returns: none
void generate_chunk(
        Chunk *chunk,
        WorkerItem *item)
{
    chunk->miny = item->miny;
    chunk->maxy = item->maxy;
    chunk->faces = item->faces;
    del_buffer(chunk->buffer);
    chunk->buffer = gen_faces(10, item->faces, item->data);
    gen_sign_buffer(chunk);
}


// Arguments:
// - chunk
// Returns: none
void gen_chunk_buffer(
        Model *g,
        Chunk *chunk)
{
    WorkerItem _item;
    WorkerItem *item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    for (int dp = -1; dp <= 1; dp++) {
        for (int dq = -1; dq <= 1; dq++) {
            Chunk *other = chunk;
            if (dp || dq) {
                other = find_chunk(g, chunk->p + dp, chunk->q + dq);
            }
            if (other) {
                item->block_maps[dp + 1][dq + 1] = &other->map;
                item->light_maps[dp + 1][dq + 1] = &other->lights;
            }
            else {
                item->block_maps[dp + 1][dq + 1] = 0;
                item->light_maps[dp + 1][dq + 1] = 0;
            }
        }
    }
    compute_chunk(item);
    generate_chunk(chunk, item);
    chunk->dirty = 0;
}


// Callback function used for world generation
// Arguments:
// - x: x position
// - y: y position
// - z: z position
// - w: block type
// - arg: pointer to map (is void* so that world gen doesn't need to know the type)
// Returns:
// - no return value
// - modifies the Map pointed to by "arg"
void map_set_func(
        int x,
        int y,
        int z,
        int w,
        void *arg)
{
    Map *map = (Map *)arg;
    map_set(map, x, y, z, w);
}


// Arguments:
// - item
// Returns: none
void load_chunk(
        WorkerItem *item)
{
    int p = item->p;
    int q = item->q;

    Map *block_map = item->block_maps[1][1];
    create_world(p, q, map_set_func, block_map);
    db_load_blocks(block_map, p, q);

    Map *light_map = item->light_maps[1][1];
    db_load_lights(light_map, p, q);

    Map *dam_map = item->damage_maps[1][1];
    db_trim_block_damage(p, q);
    db_load_damage(dam_map, p, q);
}


// Arguments:
// - p
// - q
// Returns: none
void request_chunk(
        int p,
        int q)
{
    int key = db_get_key(p, q);
    client_chunk(p, q, key);
}


// Arguments:
// - chunk
// - p
// - q
// Returns: none
void init_chunk(
        Model *g,
        Chunk *chunk,
        int p,
        int q)
{
    chunk->p = p;
    chunk->q = q;
    chunk->faces = 0;
    chunk->sign_faces = 0;
    chunk->buffer = 0;
    chunk->sign_buffer = 0;
    dirty_chunk(g, chunk);
    SignList *signs = &chunk->signs;
    sign_list_alloc(signs, 16);
    db_load_signs(signs, p, q);
    Map *block_map = &chunk->map;
    Map *dam_map = &chunk->damage;
    Map *light_map = &chunk->lights;
    int dx = p * CHUNK_SIZE - 1;
    int dy = 0;
    int dz = q * CHUNK_SIZE - 1;
    map_alloc(block_map, dx, dy, dz, 0x7fff);
    map_alloc(dam_map, dx, dy, dz, 0x7fff);
    map_alloc(light_map, dx, dy, dz, 0xf);
}


// Arguments:
// - chunk
// - p
// - q
// Returns: none
void create_chunk(
        Model *g,
        Chunk *chunk,
        int p,
        int q)
{
    init_chunk(g, chunk, p, q);

    WorkerItem _item;
    WorkerItem *item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->block_maps[1][1] = &chunk->map;
    item->light_maps[1][1] = &chunk->lights;
    item->damage_maps[1][1] = &chunk->damage;
    load_chunk(item);

    request_chunk(p, q);
}


// Delete the chunks that should be deleted (because they are out of range)
// Arguments: none
// Returns: none
void delete_chunks(
        Model *g)
{
    int count = g->chunk_count;
    State *s1 = &g->players->state;
    State *s2 = &(g->players + g->observe1)->state;
    State *s3 = &(g->players + g->observe2)->state;
    State *states[3] = {s1, s2, s3};
    for (int i = 0; i < count; i++) {
        Chunk *chunk = g->chunks + i;
        int delete = 1;
        // Do not delete a chunk if the chunk is in range
        // for any of the player's position states.
        for (int j = 0; j < 3; j++) {
            State *s = states[j];
            int p = chunked(s->x);
            int q = chunked(s->z);
            if (chunk_distance(chunk, p, q) < g->delete_radius) {
                delete = 0;
                break;
            }
        }
        if (delete) {
            map_free(&chunk->map);
            map_free(&chunk->lights);
            map_free(&chunk->damage);
            sign_list_free(&chunk->signs);
            del_buffer(chunk->buffer);
            del_buffer(chunk->sign_buffer);
            Chunk *other = g->chunks + (--count);
            memcpy(chunk, other, sizeof(Chunk));
        }
    }
    g->chunk_count = count;
}


// Delete all chunk data
// Arguments: none
// Returns: none
void delete_all_chunks(
        Model *g)
{
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        map_free(&chunk->map);
        map_free(&chunk->lights);
        map_free(&chunk->damage);
        sign_list_free(&chunk->signs);
        del_buffer(chunk->buffer);
        del_buffer(chunk->sign_buffer);
    }
    g->chunk_count = 0;
}

// Arguments: none
// Returns: none
void check_workers(
        Model *g)
{
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = g->workers + i;
        mtx_lock(&worker->mtx);
        if (worker->state == WORKER_DONE) {
            WorkerItem *item = &worker->item;
            Chunk *chunk = find_chunk(g, item->p, item->q);
            if (chunk) {
                if (item->load) {
                    Map *block_map = item->block_maps[1][1];
                    map_free(&chunk->map);
                    map_copy(&chunk->map, block_map);

                    Map *light_map = item->light_maps[1][1];
                    map_free(&chunk->lights);
                    map_copy(&chunk->lights, light_map);

                    Map *dam_map = item->damage_maps[1][1];
                    map_free(&chunk->damage);
                    map_copy(&chunk->damage, dam_map);

                    request_chunk(item->p, item->q);
                }
                generate_chunk(chunk, item);
            }
            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) {
                    Map *block_map = item->block_maps[a][b];
                    if (block_map) {
                        map_free(block_map);
                        free(block_map);
                    }

                    Map *light_map = item->light_maps[a][b];
                    if (light_map) {
                        map_free(light_map);
                        free(light_map);
                    }

                    Map *dam_map = item->damage_maps[a][b];
                    if (dam_map) {
                        map_free(dam_map);
                        free(dam_map);
                    }
                }
            }
            worker->state = WORKER_IDLE;
        }
        mtx_unlock(&worker->mtx);
    }
}

// Force the chunks around the given player to generate on this main thread.
// Arguments:
// - player: player to generate chunks for
// Returns: none
void force_chunks(
        Model *g,
        Player *player)
{
    State *s = &player->state;
    int p = chunked(s->x);
    int q = chunked(s->z);
    int r = 1;
    for (int dp = -r; dp <= r; dp++) {
        for (int dq = -r; dq <= r; dq++) {
            int a = p + dp;
            int b = q + dq;
            Chunk *chunk = find_chunk(g, a, b);
            if (chunk) {
                if (chunk->dirty) {
                    gen_chunk_buffer(g, chunk);
                }
            }
            else if (g->chunk_count < MAX_CHUNKS) {
                chunk = g->chunks + g->chunk_count++;
                create_chunk(g, chunk, a, b);
                gen_chunk_buffer(g, chunk);
            }
        }
    }
}


// Arguments:
// - player
// - worker
// Returns: none
void ensure_chunks_worker(
        Model *g,
        Player *player,
        Worker *worker)
{
    State *s = &player->state;
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    float planes[6][4];
    frustum_planes(planes, g->render_radius, matrix);
    int p = chunked(s->x);
    int q = chunked(s->z);
    int r = g->create_radius;
    int start = 0x0fffffff;
    int best_score = start;
    int best_a = 0;
    int best_b = 0;
    for (int dp = -r; dp <= r; dp++) {
        for (int dq = -r; dq <= r; dq++) {
            int a = p + dp;
            int b = q + dq;
            int index = (ABS(a) ^ ABS(b)) % WORKERS;
            if (index != worker->index) {
                continue;
            }
            Chunk *chunk = find_chunk(g, a, b);
            if (chunk && !chunk->dirty) {
                continue;
            }
            int distance = MAX(ABS(dp), ABS(dq));
            int invisible = !chunk_visible(g, planes, a, b, 0, 256);
            int priority = 0;
            if (chunk) {
                priority = chunk->buffer && chunk->dirty;
            }
            int score = (invisible << 24) | (priority << 16) | distance;
            if (score < best_score) {
                best_score = score;
                best_a = a;
                best_b = b;
            }
        }
    }
    if (best_score == start) {
        return;
    }
    int a = best_a;
    int b = best_b;
    int load = 0;
    Chunk *chunk = find_chunk(g, a, b);
    if (!chunk) {
        load = 1;
        if (g->chunk_count < MAX_CHUNKS) {
            chunk = g->chunks + g->chunk_count++;
            init_chunk(g, chunk, a, b);
        }
        else {
            return;
        }
    }
    WorkerItem *item = &worker->item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->load = load;
    for (int dp = -1; dp <= 1; dp++) {
        for (int dq = -1; dq <= 1; dq++) {
            Chunk *other = chunk;
            if (dp || dq) {
                other = find_chunk(g, chunk->p + dp, chunk->q + dq);
            }
            if (other) {
                Map *block_map = malloc(sizeof(Map));
                map_copy(block_map, &other->map);
                item->block_maps[dp + 1][dq + 1] = block_map;

                Map *light_map = malloc(sizeof(Map));
                map_copy(light_map, &other->lights);
                item->light_maps[dp + 1][dq + 1] = light_map;

                Map *dam_map = malloc(sizeof(Map));
                map_copy(dam_map, &other->damage);
                item->damage_maps[dp + 1][dq + 1] = dam_map;
            }
            else {
                item->block_maps[dp + 1][dq + 1] = 0;
                item->light_maps[dp + 1][dq + 1] = 0;
                item->damage_maps[dp + 1][dq + 1] = 0;
            }
        }
    }
    chunk->dirty = 0;
    worker->state = WORKER_BUSY;
    cnd_signal(&worker->cnd);
}

// Arguments:
// - player
// Returns: none
void ensure_chunks(
        Model *g,
        Player *player)
{
    check_workers(g);
    force_chunks(g, player);
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = g->workers + i;
        mtx_lock(&worker->mtx);
        if (worker->state == WORKER_IDLE) {
            ensure_chunks_worker(g, player, worker);
        }
        mtx_unlock(&worker->mtx);
    }
}

// Arguments:
// - arg
// Returns:
// - integer
int worker_run(
        void *arg)
{
    Worker *worker = (Worker *)arg;
    int running = 1;
    while (running) {
        mtx_lock(&worker->mtx);
        while (worker->state != WORKER_BUSY) {
            cnd_wait(&worker->cnd, &worker->mtx);
        }
        mtx_unlock(&worker->mtx);
        WorkerItem *item = &worker->item;
        if (item->load) {
            load_chunk(item);
        }
        compute_chunk(item);
        mtx_lock(&worker->mtx);
        worker->state = WORKER_DONE;
        mtx_unlock(&worker->mtx);
    }
    return 0;
}

// Arguments:
// - x
// - y
// - z
// Returns: none
void unset_sign(
        Model *g,
        int x,
        int y,
        int z)
{
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        SignList *signs = &chunk->signs;
        if (sign_list_remove_all(signs, x, y, z)) {
            chunk->dirty = 1;
            db_delete_signs(x, y, z);
        }
    }
    else {
        db_delete_signs(x, y, z);
    }
}

// Arguments:
// - x
// - y
// - z
// - face
// Returns: none
void unset_sign_face(
        Model *g,
        int x,
        int y,
        int z,
        int face)
{
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        SignList *signs = &chunk->signs;
        if (sign_list_remove(signs, x, y, z, face)) {
            chunk->dirty = 1;
            db_delete_sign(x, y, z, face);
        }
    }
    else {
        db_delete_sign(x, y, z, face);
    }
}


// Arguments:
// - p, q
// - x, y, z
// - face
// - text
// - dirty
// Returns: none
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
        int dirty)
{
    if (strlen(text) == 0) {
        unset_sign_face(g, x, y, z, face);
        return;
    }
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        SignList *signs = &chunk->signs;
        sign_list_add(signs, x, y, z, face, text);
        if (dirty) {
            chunk->dirty = 1;
        }
    }
    db_insert_sign(p, q, x, y, z, face, text);
}


// Arguments:
// - x, y, z
// - face
// - text
// Returns: none
void set_sign(
        Model *g,
        int x,
        int y,
        int z,
        int face,
        const char *text)
{
    int p = chunked(x);
    int q = chunked(z);
    _set_sign(g, p, q, x, y, z, face, text, 1);
    client_sign(x, y, z, face, text);
}


// Arguments:
// - x, y, z
// Returns: none
void toggle_light(
        Model *g,
        int x,
        int y,
        int z)
{
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        Map *map = &chunk->lights;
        int w = map_get(map, x, y, z) ? 0 : 15;
        map_set(map, x, y, z, w);
        db_insert_light(p, q, x, y, z, w);
        client_light(x, y, z, w);
        dirty_chunk(g, chunk);
    }
}


// Arguments:
// - p, q
// - x, y, z
// - w
// Returns: none
void
set_light(
        Model *g,
        int p,
        int q,
        int x,
        int y,
        int z,
        int w)
{
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        Map *map = &chunk->lights;
        if (map_set(map, x, y, z, w)) {
            dirty_chunk(g, chunk);
            db_insert_light(p, q, x, y, z, w);
        }
    }
    else {
        db_insert_light(p, q, x, y, z, w);
    }
}


// Arguments:
// - p, q
// - x, y, z
// - w
// - dirty
// Returns: none
void _set_block(
        Model *g,
        int p,
        int q,
        int x,
        int y,
        int z,
        int w,
        int dirty)
{
    Chunk *chunk = find_chunk(g, p, q);
    if (chunk) {
        Map *map = &chunk->map;
        if (map_set(map, x, y, z, w)) {
            if (dirty) {
                dirty_chunk(g, chunk);
            }
            db_insert_block(p, q, x, y, z, w);
        }
    }
    else {
        db_insert_block(p, q, x, y, z, w);
    }
    // Reset damage for deleted blocks
    if (w == 0) {
        map_set(&chunk->damage, x, y, z, 0);
    }
    // If a block is removed, then remove any signs and light source from that block.
    if (w == 0 && chunked(x) == p && chunked(z) == q) {
        unset_sign(g, x, y, z);
        set_light(g, p, q, x, y, z, 0);
    }
}


// Arguments:
// - x, y, z
// - w
// Returns: none
void set_block(
        Model *g,
        int x,
        int y,
        int z,
        int w)
{
    int p = chunked(x);
    int q = chunked(z);
    _set_block(g, p, q, x, y, z, w, 1);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) { continue; }
            if (dx && chunked(x + dx) == p) { continue; }
            if (dz && chunked(z + dz) == q) { continue; }
            _set_block(g, p + dx, q + dz, x, y, z, -w, 1);
        }
    }
    client_block(x, y, z, w);
}


// Add the block to the (short) player's block history record
// Arguments:
// - x, y, z
// - w
// Returns: none
void record_block(
        Model *g,
        int x,
        int y,
        int z,
        int w)
{
    memcpy(&g->block1, &g->block0, sizeof(Block));
    g->block0.x = x;
    g->block0.y = y;
    g->block0.z = z;
    g->block0.w = w;
}


Chunk *
find_chunk_xyz(
        Model *g,
        int x,
        int z)
{
    int p = chunked(x);
    int q = chunked(z);
    return find_chunk(g, p, q);
}


// Arguments:
// - x, y, z
// Returns:
// - w (block type value)
int get_block(
        Model *g,
        int x,
        int y,
        int z)
{
    Chunk *chunk = find_chunk_xyz(g, x, z);
    if (!chunk) { return 0; }
    return map_get(&chunk->map, x, y, z);
}


int
get_block_damage(
        Model *g,
        int x,
        int y,
        int z)
{
    Chunk *chunk = find_chunk_xyz(g, x, z);
    if (!chunk) { return 0; }
    return map_get(&chunk->damage, x, y, z);
}


// Returns whether a block was found
// Output: 'w' and 'damage'
int
get_block_and_damage(
        Model *g,
        int x,
        int y,
        int z,
        int *w,
        int *damage)
{
    Chunk *chunk = find_chunk_xyz(g, x, z);
    if (!chunk) { return 0; }
    if (w) { *w = map_get(&chunk->map, x, y, z); }
    if (damage) { *damage = map_get(&chunk->damage, x, y, z); }
    return 1;
}


void
set_block_damage(
        Model *g,
        int x,
        int y,
        int z,
        int damage)
{
    Chunk *chunk = find_chunk_xyz(g, x, z);
    if (!chunk) { assert(0 && "no chunk!"); }
    map_set(&chunk->damage, x, y, z, damage);
    db_insert_block_damage(chunk->p, chunk->q, x, y, z, damage);
}


// Add damage to a block and destroy it if it exceeds the damage limit.
// Returns non-zero if the block should destroyed
int
add_block_damage(
        Model *g,
        int x,
        int y,
        int z,
        int damage)
{
    int w, initial_damage;
    if (!get_block_and_damage(g, x, y, z, &w, &initial_damage)) { return 0; }
    if (damage < block_get_min_damage_threshold(g, w)) { return 0; }
    int new_damage = initial_damage + damage;
    set_block_damage(g, x, y, z, new_damage);
    return new_damage >= block_get_max_damage(g, w);
}


// Arguments:
// - x, y, z
// - w
// Returns: none
void
builder_block(
        Model *g,
        int x,
        int y,
        int z,
        int w)
{
    if (y <= 0 || y >= 256) { return; }
    if (block_is_destructable(g, get_block(g, x, y, z))) {
        set_block(g, x, y, z, 0);
    }
    if (w) {
        set_block(g, x, y, z, w);
    }
}


// Arguments:
// - attrib
// - player
// Returns:
// - number of faces
int render_chunks(
        Model *g,
        Attrib *attrib,
        Player *player)
{
    int result = 0;
    State *s = &player->state;
    float eye_y = player_eye_y(s->y);
    ensure_chunks(g, player);
    int p = chunked(s->x);
    int q = chunked(s->z);
    float light = get_daylight(g);
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    float planes[6][4];
    frustum_planes(planes, g->render_radius, matrix);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, s->x, eye_y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1i(attrib->extra1, 2);
    glUniform1f(attrib->extra2, light);
    glUniform1f(attrib->extra3, g->render_radius * CHUNK_SIZE);
    glUniform1i(attrib->extra4, g->ortho);
    glUniform1f(attrib->timer, time_of_day(g));
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk_distance(chunk, p, q) > g->render_radius) {
            continue;
        }
        if (!chunk_visible(g, planes, chunk->p, chunk->q, chunk->miny, chunk->maxy)) {
            continue;
        }
        draw_chunk(attrib, chunk);
        result += chunk->faces;
    }
    return result;
}


// Arguments:
// - attrib
// - player
// Returns: none
void
render_signs(
        Model *g,
        Attrib *attrib,
        Player *player)
{
    State *s = &player->state;
    int p = chunked(s->x);
    int q = chunked(s->z);
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    float planes[6][4];
    frustum_planes(planes, g->render_radius, matrix);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 3);
    glUniform1i(attrib->extra1, 1);
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk_distance(chunk, p, q) > g->sign_radius) {
            continue;
        }
        if (!chunk_visible(
                    g, planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        {
            continue;
        }
        draw_signs(attrib, chunk);
    }
}


// Arguments:
// - attrib
// - player
// Returns: none
void
render_sign(
        Model *g,
        Attrib *attrib,
        Player *player)
{
    if (!g->typing || g->typing_buffer[0] != CRAFT_KEY_SIGN) {
        return;
    }
    int x, y, z, face;
    if (!hit_test_face(g, player, &x, &y, &z, &face)) {
        return;
    }
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 3);
    glUniform1i(attrib->extra1, 1);
    char text[MAX_SIGN_LENGTH];
    strncpy(text, g->typing_buffer + 1, MAX_SIGN_LENGTH);
    text[MAX_SIGN_LENGTH - 1] = '\0';
    GLfloat *data = malloc_faces(5, strlen(text));
    int length = _gen_sign_buffer(data, x, y, z, face, text);
    GLuint buffer = gen_faces(5, length, data);
    draw_sign(attrib, buffer, length);
    del_buffer(buffer);
}


// Render the other players for the given player
void
render_players(
        Model *g,
        Attrib *attrib,
        Player *player)
{
    State *s = &player->state;
    float eye_y = player_eye_y(s->y);
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, s->x, eye_y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day(g));
    for (int i = 0; i < g->player_count; i++) {
        Player *other = g->players + i;
        if (other == player) { continue; }
        draw_player(attrib, other);
    }
}


// Render the sky for the given player's perspective
void
render_sky(
        Model *g,
        Attrib *attrib,
        Player *player,
        GLuint buffer)
{
    State *s = &player->state;
    float matrix[16];
    set_matrix_3d(
            matrix, g->width, g->height,
            0, 0, 0, s->rx, s->ry, g->fov, 0, g->render_radius);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 2);
    glUniform1f(attrib->timer, time_of_day(g));
    draw_triangles_3d(attrib, buffer, 512 * 3);
}


// Render the wireframe of the selected block for the player
void
render_wireframe(
        Model *g,
        Attrib *attrib,
        Player *player)
{
    State *s = &player->state;
    float eye_y = player_eye_y(s->y);
    float matrix[16];
    set_matrix_3d_player_camera(g, matrix, player);
    int hx, hy, hz;
    int hw = hit_test(g, 0, s->x, eye_y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (block_is_obstacle(g, hw)) {
        glUseProgram(attrib->program);
        glLineWidth(1);
        glEnable(GL_COLOR_LOGIC_OP);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        GLuint wireframe_buffer = gen_wireframe_buffer(hx, hy, hz, 0.53);
        draw_lines(attrib, wireframe_buffer, 3, 24);
        del_buffer(wireframe_buffer);
        glDisable(GL_COLOR_LOGIC_OP);
    }
}


// Render the wireframe for a box
void
render_box_wireframe(
        Model *g,
        Attrib *attrib,
        DebugBox *box,
        Player *p)
{
    if (!box->active) { return; }
    glUseProgram(attrib->program);
    float matrix[16];
    glLineWidth(3);
    set_matrix_3d_player_camera(g, matrix, p);
    glUseProgram(attrib->program);
    //glEnable(GL_COLOR_LOGIC_OP);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    draw_lines(attrib, box->buffer, 3, 24);
    glLineWidth(1);
    //glDisable(GL_COLOR_LOGIC_OP);
}


// Render all of the players' hitboxes except for the given player
// Arguments:
// - attrib
// - p: given player
// Returns: none
void
render_players_hitboxes(
        Model *g,
        Attrib *attrib,
        Player *p)
{
    glUseProgram(attrib->program);
    glLineWidth(2);
    for (int i = 0; i < g->player_count; i++) {
        Player *other = g->players + i;
        if (other != p) {
            float matrix[16];
            set_matrix_3d_player_camera(g, matrix, p);
            glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
            State *os = &other->state;
            float ex, ey, ez;
            player_hitbox_extent(&ex, &ey, &ez);
            GLuint box_buffer = gen_box_wireframe_buffer(os->x, os->y, os->z, ex, ey, ez);
            draw_lines(attrib, box_buffer, 3, 24);
            del_buffer(box_buffer);
        }
    }
}

// Arguments:
// - attrib
// Returns: none
void
render_crosshairs(
        Model *g,
        Attrib *attrib)
{
    float matrix[16];
    set_matrix_2d(matrix, g->width, g->height);
    glUseProgram(attrib->program);
    glLineWidth(4 * g->scale);
    glEnable(GL_COLOR_LOGIC_OP);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    GLuint crosshair_buffer = gen_crosshair_buffer(g);
    draw_lines(attrib, crosshair_buffer, 2, 4);
    del_buffer(crosshair_buffer);
    glDisable(GL_COLOR_LOGIC_OP);
}

// Arguments:
// - attrib
// Returns: none
void
render_item(
        Model *g,
        Attrib *attrib) 
{
    float matrix[16];
    set_matrix_item(matrix, g->width, g->height, g->scale);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, 0, 0, 5);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day(g));
    //int w = items[g->item_index];  // TODO get this
    int w = 1;
    if (block_is_plant(g, w)) {
        GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
        draw_plant(attrib, buffer);
        del_buffer(buffer);
    }
    else {
        GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
        draw_cube(attrib, buffer);
        del_buffer(buffer);
    }
}

// Arguments:
// - attrib
// - justify
// - x
// - y
// - n
// - text
// Returns: none
void
render_text(
        Model *g,
        Attrib *attrib,
        int justify,
        float x,
        float y,
        float n,
        char *text)
{
    float matrix[16];
    set_matrix_2d(matrix, g->width, g->height);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 1);
    glUniform1i(attrib->extra1, 0);
    int length = strlen(text);
    x -= n * justify * (length - 1) / 2;
    GLuint buffer = gen_text_buffer(x, y, n, text);
    draw_text(attrib, buffer, length);
    del_buffer(buffer);
}

// Arguments:
// - text
// Returns: none
void
add_message(
        Model *g,
        const char *text) 
{
    printf("%s\n", text);
    snprintf(
            g->messages[g->message_index], MAX_TEXT_LENGTH, "%s", text);
    g->message_index = (g->message_index + 1) % MAX_MESSAGES;
}

// Arguments: none
// Returns:
void login() 
{
    char username[128] = {0};
    char identity_token[128] = {0};
    char access_token[128] = {0};
    if (db_auth_get_selected(username, 128, identity_token, 128)) {
        printf("Contacting login server for username: %s\n", username);
        if (get_access_token(
                    access_token, 128, username, identity_token))
        {
            printf("Successfully authenticated with the login server\n");
            client_login(username, access_token);
        }
        else {
            printf("Failed to authenticate with the login server\n");
            client_login("", "");
        }
    }
    else {
        printf("Logging in anonymously\n");
        client_login("", "");
    }
}


// Player copies block
void
copy(
        Model *g)
{
    memcpy(&g->copy0, &g->block0, sizeof(Block));
    memcpy(&g->copy1, &g->block1, sizeof(Block));
}


// Player pastes structure
void
paste(
        Model *g) 
{
    Block *c1 = &g->copy1;
    Block *c2 = &g->copy0;
    Block *p1 = &g->block1;
    Block *p2 = &g->block0;
    int scx = SIGN(c2->x - c1->x);
    int scz = SIGN(c2->z - c1->z);
    int spx = SIGN(p2->x - p1->x);
    int spz = SIGN(p2->z - p1->z);
    int oy = p1->y - c1->y;
    int dx = ABS(c2->x - c1->x);
    int dz = ABS(c2->z - c1->z);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x <= dx; x++) {
            for (int z = 0; z <= dz; z++) {
                int w = get_block(g, c1->x + x * scx, y, c1->z + z * scz);
                builder_block(g, p1->x + x * spx, y + oy, p1->z + z * spz, w);
            }
        }
    }
}

// (Used as a chat command).
// Arguments:
// - b1
// - b2
// - xc
// - yc
// - zc
// Returns: none
void
array(
        Model *g,
        Block *b1,
        Block *b2,
        int xc,
        int yc,
        int zc) 
{
    if (b1->w != b2->w) {
        return;
    }
    int w = b1->w;
    int dx = b2->x - b1->x;
    int dy = b2->y - b1->y;
    int dz = b2->z - b1->z;
    xc = dx ? xc : 1;
    yc = dy ? yc : 1;
    zc = dz ? zc : 1;
    for (int i = 0; i < xc; i++) {
        int x = b1->x + dx * i;
        for (int j = 0; j < yc; j++) {
            int y = b1->y + dy * j;
            for (int k = 0; k < zc; k++) {
                int z = b1->z + dz * k;
                builder_block(g, x, y, z, w);
            }
        }
    }
}

// Place a cube made out of blocks (Used as a chat command).
// Arguments:
// - b1
// - b2
// - fill
// Returns: none
void
cube(
        Model *g,
        Block *b1,
        Block *b2,
        int fill) 
{
    if (b1->w != b2->w) { return; }
    int w = b1->w;
    int x1 = MIN(b1->x, b2->x);
    int y1 = MIN(b1->y, b2->y);
    int z1 = MIN(b1->z, b2->z);
    int x2 = MAX(b1->x, b2->x);
    int y2 = MAX(b1->y, b2->y);
    int z2 = MAX(b1->z, b2->z);
    int a = (x1 == x2) + (y1 == y2) + (z1 == z2);
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            for (int z = z1; z <= z2; z++) {
                if (!fill) {
                    int n = 0;
                    n += x == x1 || x == x2;
                    n += y == y1 || y == y2;
                    n += z == z1 || z == z2;
                    if (n <= a) {
                        continue;
                    }
                }
                builder_block(g, x, y, z, w);
            }
        }
    }
}


// Place a sphere at a block location. (Used as a chat command)
// Arguments:
// - center: a block to take the center block position from
// - radius: sphere radius
// - fill
// - fx
// - fy
// - fz
// Returns: none
void
sphere(
        Model *g,
        Block *center,
        int radius,
        int fill,
        int fx,
        int fy,
        int fz) 
{
    static const float offsets[8][3] = {
        {-0.5, -0.5, -0.5},
        {-0.5, -0.5, 0.5},
        {-0.5, 0.5, -0.5},
        {-0.5, 0.5, 0.5},
        {0.5, -0.5, -0.5},
        {0.5, -0.5, 0.5},
        {0.5, 0.5, -0.5},
        {0.5, 0.5, 0.5}
    };
    int cx = center->x;
    int cy = center->y;
    int cz = center->z;
    int w = center->w;
    for (int x = cx - radius; x <= cx + radius; x++) {
        if (fx && x != cx) {
            continue;
        }
        for (int y = cy - radius; y <= cy + radius; y++) {
            if (fy && y != cy) {
                continue;
            }
            for (int z = cz - radius; z <= cz + radius; z++) {
                if (fz && z != cz) {
                    continue;
                }
                int inside = 0;
                int outside = fill;
                for (int i = 0; i < 8; i++) {
                    float dx = x + offsets[i][0] - cx;
                    float dy = y + offsets[i][1] - cy;
                    float dz = z + offsets[i][2] - cz;
                    float d = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (d < radius) {
                        inside = 1;
                    }
                    else {
                        outside = 1;
                    }
                }
                if (inside && outside) {
                    builder_block(g, x, y, z, w);
                }
            }
        }
    }
}


// Place a cylinder made out of blocks (Used as a chat command).
// Arguments:
// - b1
// - b2
// - radius
// - fill
// Returns: none
void
cylinder(
        Model *g,
        Block *b1,
        Block *b2,
        int radius,
        int fill) 
{
    if (b1->w != b2->w) {
        return;
    }
    int w = b1->w;
    int x1 = MIN(b1->x, b2->x);
    int y1 = MIN(b1->y, b2->y);
    int z1 = MIN(b1->z, b2->z);
    int x2 = MAX(b1->x, b2->x);
    int y2 = MAX(b1->y, b2->y);
    int z2 = MAX(b1->z, b2->z);
    int fx = x1 != x2;
    int fy = y1 != y2;
    int fz = z1 != z2;
    if (fx + fy + fz != 1) {
        return;
    }
    Block block = {x1, y1, z1, w};
    if (fx) {
        for (int x = x1; x <= x2; x++) {
            block.x = x;
            sphere(g, &block, radius, fill, 1, 0, 0);
        }
    }
    if (fy) {
        for (int y = y1; y <= y2; y++) {
            block.y = y;
            sphere(g, &block, radius, fill, 0, 1, 0);
        }
    }
    if (fz) {
        for (int z = z1; z <= z2; z++) {
            block.z = z;
            sphere(g, &block, radius, fill, 0, 0, 1);
        }
    }
}


// Place a tree at a block location. (Used as a chat command)
// Arguments:
// - block: a block to take the base block position from
// Returns: none
void
tree(
        Model *g,
        Block *block)
{
    int bx = block->x;
    int by = block->y;
    int bz = block->z;
    for (int y = by + 3; y < by + 8; y++) {
        for (int dx = -3; dx <= 3; dx++) {
            for (int dz = -3; dz <= 3; dz++) {
                int dy = y - (by + 4);
                int d = (dx * dx) + (dy * dy) + (dz * dz);
                if (d < 11) {
                    builder_block(g, bx + dx, y, bz + dz, 15);
                }
            }
        }
    }
    for (int y = by; y < by + 7; y++) {
        builder_block(g, bx, y, bz, 5);
    }
}

// Parse a player chat command
// commands list:
// - /identity <username> <token>
// - /logout
// - /login <username>
// - /online <address> <port>
// - /offline [file]
// - /copy
// - /paste
// - /tree
// - /cylinder
// - /fcylinder
// - /cube
// - /fcube
// - /sphere
// - /fsphere
// - /circlex
// - /circley
// - /circlez
// - /fcirclex
// - /fcircley
// - /fcirclez
void
parse_command(
        Model *g,
        const char *buffer,  // command string to parse
        int forward)         // flag about whether to forward the command as a chat message
{
    char username[128] = {0};
    char token[128] = {0};
    char server_addr[MAX_ADDR_LENGTH];
    int server_port = DEFAULT_PORT;
    char filename[MAX_PATH_LENGTH];
    int radius, count, xc, yc, zc;
    if (sscanf(buffer, "/identity %128s %128s", username, token) == 2) {
        db_auth_set(username, token);
        add_message(g, "Successfully imported identity token!");
        login();
    }
    else if (strcmp(buffer, "/logout") == 0) {
        db_auth_select_none();
        login();
    }
    else if (sscanf(buffer, "/login %128s", username) == 1) {
        if (db_auth_select(username)) {
            login();
        }
        else {
            add_message(g, "Unknown username.");
        }
    }
    else if (sscanf(buffer,
                "/online %128s %d", server_addr, &server_port) >= 1)
    {
        g->mode_changed = 1;
        g->mode = MODE_ONLINE;
        strncpy(g->server_addr, server_addr, MAX_ADDR_LENGTH);
        g->server_port = server_port;
        snprintf(g->db_path, MAX_PATH_LENGTH,
                "cache.%s.%d.db", g->server_addr, g->server_port);
    }
    else if (sscanf(buffer, "/offline %128s", filename) == 1) {
        g->mode_changed = 1;
        g->mode = MODE_OFFLINE;
        snprintf(g->db_path, MAX_PATH_LENGTH, "%s.db", filename);
    }
    else if (strcmp(buffer, "/offline") == 0) {
        g->mode_changed = 1;
        g->mode = MODE_OFFLINE;
        snprintf(g->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
    }
    else if (sscanf(buffer, "/view %d", &radius) == 1) {
        // Set view radius
        if (radius >= 1 && radius <= 24) {
            g->create_radius = radius;
            g->render_radius = radius;
            g->delete_radius = radius + 4;
        }
        else {
            add_message(g, "Viewing distance must be between 1 and 24.");
        }
    }
    else if (strcmp(buffer, "/copy") == 0) {
        copy(g);
    }
    else if (strcmp(buffer, "/paste") == 0) {
        paste(g);
    }
    else if (strcmp(buffer, "/tree") == 0) {
        // Place tree
        tree(g, &g->block0);
    }
    else if (sscanf(buffer, "/array %d %d %d", &xc, &yc, &zc) == 3) {
        array(g, &g->block1, &g->block0, xc, yc, zc);
    }
    else if (sscanf(buffer, "/array %d", &count) == 1) {
        array(g, &g->block1, &g->block0, count, count, count);
    }
    else if (strcmp(buffer, "/fcube") == 0) {
        // Place a filled cube
        cube(g, &g->block0, &g->block1, 1);
    }
    else if (strcmp(buffer, "/cube") == 0) {
        // Place an unfilled cube
        cube(g, &g->block0, &g->block1, 0);
    }
    else if (sscanf(buffer, "/fsphere %d", &radius) == 1) {
        // Place a filled sphere with a radius
        sphere(g, &g->block0, radius, 1, 0, 0, 0);
    }
    else if (sscanf(buffer, "/sphere %d", &radius) == 1) {
        // Place an unfilled sphere with a radius
        sphere(g, &g->block0, radius, 0, 0, 0, 0);
    }
    else if (sscanf(buffer, "/fcirclex %d", &radius) == 1) {
        // Place a filled circle on the x-axis with a radius
        sphere(g, &g->block0, radius, 1, 1, 0, 0);
    }
    else if (sscanf(buffer, "/circlex %d", &radius) == 1) {
        // Place an unfilled circle on the x-axis with a radius
        sphere(g, &g->block0, radius, 0, 1, 0, 0);
    }
    else if (sscanf(buffer, "/fcircley %d", &radius) == 1) {
        // Place a filled circle on the y-axis with a radius
        sphere(g, &g->block0, radius, 1, 0, 1, 0);
    }
    else if (sscanf(buffer, "/circley %d", &radius) == 1) {
        // Place an unfilled circle on the y-axis with a radius
        sphere(g, &g->block0, radius, 0, 0, 1, 0);
    }
    else if (sscanf(buffer, "/fcirclez %d", &radius) == 1) {
        // Place a filled circle on the z-axis with a radius
        sphere(g, &g->block0, radius, 1, 0, 0, 1);
    }
    else if (sscanf(buffer, "/circlez %d", &radius) == 1) {
        // Place an unfilled circle on the z-axis with a radius
        sphere(g, &g->block0, radius, 0, 0, 0, 1);
    }
    else if (sscanf(buffer, "/fcylinder %d", &radius) == 1) {
        // Place a filled cylinder with a radius
        cylinder(g, &g->block0, &g->block1, radius, 1);
    }
    else if (sscanf(buffer, "/cylinder %d", &radius) == 1) {
        // Place an unfilled cylinder with a radius
        cylinder(g, &g->block0, &g->block1, radius, 0);
    }
    else if (sscanf(buffer, "/damage %d", &radius) == 1) {
        char out[30];
        snprintf(out, sizeof(out), "set attack_damage=%d", radius);
        add_message(g, out);
        g->players[0].attrs.attack_damage = radius;
    }
    else if (sscanf(buffer, "/reach %d", &radius) == 1) {
        char out[30];
        snprintf(out, sizeof(out), "set reach=%d", radius);
        add_message(g, out);
        g->players[0].attrs.reach = radius;
    }
    else if (forward) {
        // If no command was found, maybe send it as a chat message
        client_talk(buffer);
    }
}


// Arguments: none
// Returns: none
void
on_light(
        Model *g) 
{
    State *s = &g->players->state;
    float y = player_eye_y(s->y);
    int hx, hy, hz;
    int hw = hit_test(g, 0, s->x, y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (hy > 0 && hy < 256 && block_is_destructable(g, hw)) {
        toggle_light(g, hx, hy, hz);
    }
}


// Try to place a block where the player is looking and return success
int
place_block(
        Model *g)
{
    State *s = &g->players->state;
    float y = player_eye_y(s->y);
    int hx, hy, hz;
    int hw = hit_test(g, 1, s->x, y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (!(hy > 0 && hy < 256 && block_is_obstacle(g, hw))) { return 0; }
    if (player_intersects_block(s->x, s->y, s->z, s->vx, s->vy, s->vz, hx, hy, hz)) { return 0; }
    //set_block(g, hx, hy, hz, items[g->item_index]);
    set_block(g, hx, hy, hz, 1);
    //record_block(g, hx, hy, hz, items[g->item_index]);
    record_block(g, hx, hy, hz, 1);
    return 1;
}


// Try to break a block where the player is looking and return success
int
break_block(
        Model *g)
{
    State *s = &(g->players[0].state);
    float y = player_eye_y(s->y);
    int hx, hy, hz;
    int hw = hit_test(g, 0, s->x, y, s->z, s->rx, s->ry, &hx, &hy, &hz);

    // Only break blocks that are in bounds and are destructable
    if (!(hy > 0 && hy < 256 && block_is_destructable(g, hw))) {
        return 0;
    }

    int damage = g->players[0].attrs.attack_damage;
    //printf("doing %d damage\n", damage);
    if (!add_block_damage(g, hx, hy, hz, damage)) { return 0; }

    set_block(g, hx, hy, hz, 0);
    record_block(g, hx, hy, hz, 0);
    if (block_is_plant(g, get_block(g, hx, hy + 1, hz))) {
        set_block(g, hx, hy + 1, hz, 0);
    }
    return 1;
}


// Destroy block on left click, which also has a cool-down time
void
on_left_click(
        Model *g) 
{
    float t = glfwGetTime();
    if (t - g->players[0].attrs.dblockt > g->physics.dblockcool) {
        g->players[0].attrs.dblockt = t;
        break_block(g);
    }
}


// Place block on left click, has a cool-down
void
on_right_click(
        Model *g) 
{
    PlayerAttributes *s = &g->players->attrs;
    float t = glfwGetTime();
    if (t - s->blockt > g->physics.blockcool) {
        if (place_block(g)) {
            s->blockt = t;
        }
    }
}


// Arguments: none
// Returns: none
void
on_middle_click(
        Model *g) 
{
    return; // TODO: implement block picking perhaps?
    //State *s = &g->players->state;
    //float y = player_eye_y(s->y);
    //int hx, hy, hz;
    //int hw = hit_test(g, 0, s->x, y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    //for (int i = 0; i < item_count; i++) {
    //    if (items[i] == hw) {
    //        g->item_index = i;
    //        break;
    //    }
    //}
}


// Move camera with mouse movement
void
handle_mouse_input(
        Model *g) 
{
    int exclusive =
        glfwGetInputMode(g->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    // Previous values
    static double px = 0;
    static double py = 0;
    State *s = &g->players->state;
    if (exclusive && (px || py)) {
        double mx, my;
        glfwGetCursorPos(g->window, &mx, &my);
        float m = 0.0025;
        s->rx += (mx - px) * m;
        if (INVERT_MOUSE) {
            s->ry += (my - py) * m;
        }
        else {
            s->ry -= (my - py) * m;
        }
        // Keep rx in a nice range
        if (s->rx < 0) {
            s->rx += RADIANS(360);
        }
        if (s->rx >= RADIANS(360)){
            s->rx -= RADIANS(360);
        }
        // Update body rotation
        if (fabs(s->rx - s->brx) > 0.8) {
            // move
            s->brx += (mx - px) * m;
        }
        if (s->brx < 0) {
            s->brx += RADIANS(360);
        }
        if (s->brx >= RADIANS(360)){
            s->brx -= RADIANS(360);
        }
        // Clamp ry
        s->ry = MAX(s->ry, -RADIANS(90));
        s->ry = MIN(s->ry, RADIANS(90));
        px = mx;
        py = my;
    }
    else {
        glfwGetCursorPos(g->window, &px, &py);
    }
}


static float
calc_frame_stopping_damage(
        Model *g,
        float dt,
        float dx,
        float dy,
        float dz) 
{
    float mag = v3_mag(dx, dy, dz);
    return calc_damage_from_impulse(g, mag * dt);
}


static void add_player_damage(
        Player *p,
        unsigned damage) 
{
    p->attrs.taken_damage += damage;
}


// Accesses the windown input to get the keys that change the view in special ways
void
input_get_keys_view(
        Model *g)
{
    g->ortho = glfwGetKey(g->window, CRAFT_KEY_ORTHO) ? 64 : 0;
    g->fov = glfwGetKey(g->window, CRAFT_KEY_ZOOM) ? 15 : 65;
}


// Accesses the window input to get keys that move player cursor
void
input_get_keys_look(
        Model *g,
        State *s,           // player state to modify
        float dt)           // change in time (delta t)
{
    const float m = dt * 1.0;
    if (glfwGetKey(g->window, GLFW_KEY_LEFT))  { s->rx -= m; }
    if (glfwGetKey(g->window, GLFW_KEY_RIGHT)) { s->rx += m; }
    if (glfwGetKey(g->window, GLFW_KEY_UP))    { s->ry += m; }
    if (glfwGetKey(g->window, GLFW_KEY_DOWN))  { s->ry -= m; }
}


// Accesses the window input to get keys that walk the player
void
input_get_keys_walk(
        Model *g,
        int *sx,        // flag for strafe X [output pointer]
        int *sz)        // flag for strafe Y [output pointer]
{
    if (glfwGetKey(g->window, CRAFT_KEY_FORWARD))  { *sz -= 1; }
    if (glfwGetKey(g->window, CRAFT_KEY_BACKWARD)) { *sz += 1; }
    if (glfwGetKey(g->window, CRAFT_KEY_LEFT))     { *sx -= 1; }
    if (glfwGetKey(g->window, CRAFT_KEY_RIGHT))    { *sx += 1; }
}


// Get input and handle vertical movement when not flying
float
input_player_jump(
        Model *g,
        Player *p)
{
    if (!glfwGetKey(g->window, CRAFT_KEY_JUMP)) { return 0.0f; }
    float t = glfwGetTime();
    if (!p->attrs.is_grounded || !(t - p->attrs.jumpt > g->physics.jumpcool)) {
        // Can't jump while in the air already
        return 0.0f;
    }
    p->attrs.jumpt = t;
    p->attrs.is_grounded = 0;
    return g->physics.jumpaccel;
}


// Get input and handle vertical movement when flying
float
input_player_fly(
        Model *g)
{
    int jump = glfwGetKey(g->window, CRAFT_KEY_JUMP);
    int down = glfwGetKey(g->window, CRAFT_KEY_CROUCH);
    if (jump && !down) { return g->physics.flysp; }
    else if (down && !jump) { return -g->physics.flysp; }
    return 0.0f;
}


// Handle jump or fly up/down
float
input_player_jump_or_fly(  // Returns vertical acceleration
        Model *g,
        Player *p)
{
    if (p->attrs.flying) { return input_player_fly(g); }
    else { return input_player_jump(g, p); }
}


// Add player acceleration to player velocity based on proper flags
void add_velocity(
        float *vx,
        float *vy,
        float *vz,
        float ax,
        float ay,
        float az,
        float dt,
        int is_flying,
        const PhysicsConfig *phc)
{

    float hspeed = is_flying? phc->flysp : phc->walksp;
    *vx += (ax * hspeed * dt);
    *vz += (az * hspeed * dt);
    *vy += (ay * dt);
}


void constrain_velocity(
        float *vx,
        float *vy,
        float *vz,
        float dt,
        int is_flying,
        int is_grounded,
        const PhysicsConfig *phc)
{
    // Apply gravity (unless flying)
    if (!is_flying) {
        *vy -= phc->grav * dt;
    }

    // Set a minimum velocity (squared) for velocity to be clamped to 0
    float vminsq = 0.01;
    if (powf(*vx,2) + powf(*vy,2) + powf(*vz,2) <= vminsq ) {
        *vx = 0;
        *vy = 0;
        *vz = 0;
    }

    // Decay velocity differently for flying or not flying
    if (is_flying) {
        float r = phc->flyr * dt;
        *vx -= *vx * r;
        *vy -= *vy * r;
        *vz -= *vz * r;
    }
    else {
        // resistance horizontal factor, and resistance vertical factor
        float rh = dt * (is_grounded? phc->groundr : phc->airhr);
        *vx -= *vx * rh;
        *vy -= *vy * phc->airvr * dt;
        *vz -= *vz * rh;
    }

    // Set a maximum y velocity
    const float vy_max = 150;
    if (fabs(*vy) > vy_max) {
        *vy = vy_max * SIGN(*vy);
    }
}


// Handle a player's collision with blocks in `handle_movement`.
int
handle_dynamic_collision(  // Returns whether there was a collision or not
        Model *g,
        Player *p,
        float dt)          // delta time
{
    State *s = &p->state;

    // Get player hitbox
    float bx = s->x, by = s->y, bz = s->z;
    float ex, ey, ez;
    player_hitbox_extent(&ex, &ey, &ez);

    float nx, ny, nz; // normal vector to be set from swept collision

    // Reset this flag because collision will set it if necessary.
    p->attrs.is_grounded = 0;

    // "t" = collision time relative to this frame (between 0.0 and 1.0)
    float t = box_sweep_world(
            g, bx, by, bz, ex, ey, ez, s->vx * dt, s->vy * dt, s->vz * dt,
            &nx, &ny, &nz);
    if (0.0 <= t && t < 1.0) {
        // There was a collision
        // Do multiple collision steps per frame
        const int steps = 4;
        const float ut = dt / steps;
        const float oppose = 1.2 * ut;
        const float pad = 0.001;
        float vx0 = s->vx, vy0 = s->vy, vz0 = s->vz; // save original velocity for damage calculation
        for (int i = 0; i < steps; i++) {
            t = box_sweep_world(
                    g, bx, by, bz, ex, ey, ez, s->vx * ut, s->vy * ut, s->vz * ut,
                    &nx, &ny, &nz);
            // Move up to the collision moment
            bx += s->vx * t * ut;
            by += s->vy * t * ut;
            bz += s->vz * t * ut;
            // Offset the box by pad to prevent the hitbox from being exactly
            // next to a block edge
            // Respond to the collision normal by opposing velocity in that
            // direction.
            if (nx != 0.0) {
                // In x direction
                bx += nx * pad;
                s->vx = nx * oppose;
            }
            else if (ny != 0.0) {
                // In y direction
                by += ny * pad;
                s->vy = ny * oppose;
                // Detect hitting a floor
                if (ny > 0) {
                    p->attrs.is_grounded = 1;
                }
            }
            else if (nz != 0.0) {
                // In z direction
                bz += nz * pad;
                s->vz = nz * oppose;
            }
        }
        // Set player position
        s->x = bx;
        s->y = by;
        s->z = bz;
        // Update player damage
        float stopping_damage = calc_frame_stopping_damage(
                g,
                dt,
                s->vx - vx0,
                s->vy - vy0,
                s->vz - vz0);
        if (stopping_damage) { add_player_damage(p, stopping_damage); }
        return 1;
    }
    else {
        // There was no collision
        // Add full velocity to position
        s->x += s->vx * dt;
        s->y += s->vy * dt;
        s->z += s->vz * dt;
        return 0;
    }
}


// Player movement
void
handle_movement(
        Model *g,
        double dt)
{
    Player *p = &g->players[0];
    State *s = &p->state;
    const PhysicsConfig *phc = &g->physics;

    int sz = 0, sx = 0;
    if (!g->typing) {
        input_get_keys_view(g);
        input_get_keys_look(g, s, dt);
        input_get_keys_walk(g, &sx, &sz);
    }

    // Get acceleration motion from the inputs
    float ax=0, ay=0, az=0;
    get_motion_vector(p->attrs.flying, sz, sx, s->rx, s->ry, &ax, &ay, &az);

    // Handle jump/fly
    if (!g->typing) {
        ay = input_player_jump_or_fly(g, p);
    }

    // Add acceleration from input motion to velocity horizontal and vertical speed
    add_velocity(&s->vx, &s->vy, &s->vz, ax, ay, az, dt, p->attrs.flying, phc);

    // Apply gravity and enforce limits on velocity
    constrain_velocity(
            &s->vx, &s->vy, &s->vz,
            dt, p->attrs.flying, p->attrs.is_grounded, phc);

    handle_dynamic_collision(g, p, dt);

    // Make sure position does not go below the world
    if (s->y < 0) {
        s->vy = 0;
        s->y = highest_block(g, s->x, s->z) + PLAYER_HEIGHT;
    }
}

// Parse response string from server
// Arguments:
// - buffer: the response string to parse
// Returns: none
// Note:
//   multiplayer: A simple, ASCII, line-based protocol is used. Each line is
//   made up of a command code and zero or more comma-separated arguments.
// Server Commands/responses:
// - B,p,q,x,y,z,w         : block update in chunk (p, q) at (x, y, z) of block
//                           type "w"
// - D,pid                 : disconnect player with id "pid"
// - E,e,d                 : "Time". Elapse "e" with day length "d"
// - K,p,q,key             : set "key" for chunk (p, q)
// - L,p,q,x,y,z,w         : light update in chunk (p, q) at (x, y, z) of block
//                           type "w"
// - N,pid,name            : nickname for player with id
// - P,pid,x,y,z,rx,ry     : player movement update
// - R,p,q                 : Redraw chunk at (p, q)
// - S,p,q,x,y,z,face,text : sign placement
// - T,s                   : "Talk". chat message "s"
// - U,pid,x,y,z,rx,ry     : "You". Response to set this client's player
//                           position (maybe upon joining the server?)
void
parse_buffer(
        Model *g,
        char *buffer)
{
    Player *me = g->players;
    State *s = &g->players->state;
    char *key;
    char *line = tokenize(buffer, "\n", &key);
    while (line) {
        // Try and parse this line as a server response/command
        // If the response does not match anything, the line is ignored.
        int pid;
        float ux, uy, uz, urx, ury;
        // Set this client's player position
        if (sscanf(line, "U,%d,%f,%f,%f,%f,%f",
                    &pid, &ux, &uy, &uz, &urx, &ury) == 6) {
            me->id = pid;
            force_chunks(g, me);
            s->x = ux; s->y = uy; s->z = uz; s->rx = urx; s->ry = ury;
            if (uy == 0) {
                s->y = highest_block(g, s->x, s->z) + 2;
            }
        }
        // Block update
        int bp, bq, bx, by, bz, bw;
        if (sscanf(line, "B,%d,%d,%d,%d,%d,%d",
                    &bp, &bq, &bx, &by, &bz, &bw) == 6) {
            _set_block(g, bp, bq, bx, by, bz, bw, 0);
            if (player_intersects_block(s->x, s->y, s->z, s->vx, s->vy, s->vz, bx, by, bz)) {
                s->y = highest_block(g, s->x, s->z) + 2;
            }
        }
        // Light update
        if (sscanf(line, "L,%d,%d,%d,%d,%d,%d",
                    &bp, &bq, &bx, &by, &bz, &bw) == 6)
        {
            set_light(g, bp, bq, bx, by, bz, bw);
        }
        // Player position update
        float px, py, pz, prx, pry;
        if (sscanf(line, "P,%d,%f,%f,%f,%f,%f",
                    &pid, &px, &py, &pz, &prx, &pry) == 6)
        {
            Player *player = find_player(g, pid);
            if (!player && g->player_count < MAX_PLAYERS) {
                player = g->players + g->player_count;
                g->player_count++;
                player->id = pid;
                player->buffer = 0;
                snprintf(player->name, MAX_NAME_LENGTH, "player%d", pid);
                update_player(player, px, py, pz, prx, pry, 1); // twice
            }
            if (player) {
                update_player(player, px, py, pz, prx, pry, 1);
            }
        }
        // Delete player id
        if (sscanf(line, "D,%d", &pid) == 1) {
            delete_player(g, pid);
        }
        // Chunk key
        int kp, kq, kk;
        if (sscanf(line, "K,%d,%d,%d", &kp, &kq, &kk) == 3) {
            db_set_key(kp, kq, kk);
        }
        // Dirty chunk
        if (sscanf(line, "R,%d,%d", &kp, &kq) == 2) {
            Chunk *chunk = find_chunk(g, kp, kq);
            if (chunk) {
                dirty_chunk(g, chunk);
            }
        }
        // Time sync
        double elapsed;
        int day_length;
        if (sscanf(line, "E,%lf,%d", &elapsed, &day_length) == 2) {
            glfwSetTime(fmod(elapsed, day_length));
            g->day_length = day_length;
            g->time_changed = 1;
        }
        // Chat message
        if (line[0] == 'T' && line[1] == ',') {
            char *text = line + 2;
            add_message(g, text);
        }
        char format[64];
        // Player name
        snprintf(
                format, sizeof(format), "N,%%d,%%%ds", MAX_NAME_LENGTH - 1);
        char name[MAX_NAME_LENGTH];
        if (sscanf(line, format, &pid, name) == 2) {
            Player *player = find_player(g, pid);
            if (player) {
                strncpy(player->name, name, MAX_NAME_LENGTH);
            }
        }
        // Sign placement
        snprintf(
                format, sizeof(format),
                "S,%%d,%%d,%%d,%%d,%%d,%%d,%%%d[^\n]", MAX_SIGN_LENGTH - 1);
        int face;
        char text[MAX_SIGN_LENGTH] = {0};
        if (sscanf(line, format, &bp, &bq, &bx, &by, &bz, &face, text) >= 6) {
            _set_sign(g, bp, bq, bx, by, bz, face, text, 0);
        }
        // Get next line
        line = tokenize(NULL, "\n", &key);
    }
}


static void set_default_physics(
        PhysicsConfig *p)
{
    memset(p, 0, sizeof(*p));
    p->flyr = 3.0;
    p->airhr = 8.0;
    p->airvr = 0.1;
    p->groundr = 8.1;
    p->flysp = 90.0;
    p->walksp = 80.0;
    p->grav = 60.0;
    p->jumpaccel = 800.0;
    p->jumpcool = 0.51;
    p->blockcool = 0.1;   // max 10 times per second
    p->dblockcool = 0.05; // max 20 times per second
    p->min_impulse_damage = 0.40;
    p->impulse_damage_min = 10.0;
    p->impulse_damage_scale = 380.0;
}


static 
BlockFaceInfo *
game_block_get_face(
        Model *g,
        BlockKind w,
        int face_index)
{
    BlockProperties *properties = &g->the_block_types[w - 1];
    switch (face_index)
    {
    case 0: return &properties->left_face;
    case 1: return &properties->right_face;
    case 2: return &properties->top_face;
    case 3: return &properties->bottom_face;
    case 4: return &properties->front_face;
    case 5: return &properties->back_face;
    default: return NULL;
    }
}


static
void
game_block_set_face_tile_index(
        Model *g,
        BlockKind w,
        int face_index,
        int tile_number)
{
    BlockFaceInfo *face = game_block_get_face(g, w, face_index);
    assert(face);
    face->texture_tile_index = tile_number;
}


static
void
game_block_set_all_faces_tile_index(
        Model *g,
        BlockKind w,
        int tile_number)
{
    for (int i = 0; i < 6; i++)
    {
        game_block_set_face_tile_index(g, w, i, tile_number);
    }
}


static
void
game_create_standard_blocks(
        Model *g)
{
    g->the_block_types_count = 1;
    g->the_block_types = calloc(g->the_block_types_count, sizeof(*g->the_block_types));
    assert(g->the_block_types);

    game_block_set_all_faces_tile_index(g, 1, 244);
    printf("blocks created\n");
}


// Reset the game model
// Arguments: none
// Returns:
// - no return value
// - modifies game model
void
reset_model(
        Model *g)
{
    memset(g->chunks, 0, sizeof(Chunk) * MAX_CHUNKS);
    g->chunk_count = 0;
    memset(g->players, 0, sizeof(Player) * MAX_PLAYERS);
    g->player_count = 0;
    g->observe1 = 0;
    g->observe2 = 0;
    g->item_index = 0;
    memset(g->typing_buffer, 0, sizeof(char) * MAX_TEXT_LENGTH);
    g->typing = 0;
    memset(g->messages, 0, sizeof(char) * MAX_MESSAGES * MAX_TEXT_LENGTH);
    g->message_index = 0;
    g->day_length = DAY_LENGTH;
    glfwSetTime(g->day_length / 3.0);
    g->time_changed = 1;

    game_create_standard_blocks(g);

    // Default physics
    set_default_physics(&g->physics);
}


// Get whether a certain block face is covered or exposed.
// A face is exposed unless an obstacle block is in front of it.
// Arguments:
// - x, y, z: block to check
// - nx, ny, nz: normal of the block's face to check
// Returns:
// - non-zero if the given block face is covered
int
is_block_face_covered(
        Model *g,
        int x,
        int y,
        int z,
        float nx,
        float ny,
        float nz)
{
    assert(nx != 0.0 || ny != 0.0 || nz != 0.0);
    int w = get_block(g, roundf(x + nx), roundf(y + ny), roundf(z + nz));
    return block_is_obstacle(g, w);
}


// Return whether a bounding box currently intersects a block in the world.
// Arguments:
// - x, y, z: box center position
// - ex, ey, ez: box extents
// Returns: intersected block location through cx, cy, cz
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
        int *cz)
{
    int result = 0;
    int x0, y0, z0, x1, y1, z1;
    // Currently found minimum distance squared
    float dsq = INFINITY;
    box_nearest_blocks(x, y, z, ex, ey, ez, &x0, &y0, &z0, &x1, &y1, &z1);
    for (int bx = x0; bx <= x1; bx++) {
        for (int by = y0; by <= y1; by++) {
            for (int bz = z0; bz <= z1; bz++) {
                int w = get_block(g, bx, by, bz);
                if (!block_is_obstacle(g, w)) {
                    continue;
                }
                if (!box_intersect_block(x, y, z, ex, ey, ez, bx, by, bz)) {
                    continue;
                }
                // Specific distance squared
                float sdsq =
                    powf(x - bx, 2)
                    + powf(y - by, 2) 
                    + powf(z - bz, 2);
                // Check by distance
                if (sdsq < dsq) {
                    dsq = sdsq;
                    *cx = bx;
                    *cy = by;
                    *cz = bz;
                    // Collision did happen
                    result = 1;
                }
            }
        }
    }
    return result;
}

// Sweep moving bounding box with all nearby blocks in the world. Returns the
// info for the earliest intersection time.
// Returns:
// - earliest collision time, between 0.0 and 1.0
// - writes values out to nx, ny, and nz
float
box_sweep_world(
        Model *g,
        float x,        // box center x
        float y,        // box center y
        float z,        // box center z
        float ex,       // box extent x
        float ey,       // box extent y
        float ez,       // box extent z
        float vx,       // box velocity x
        float vy,       // box velocity y
        float vz,       // box velocity z
        float *nx,      // collision normal x
        float *ny,      // collision normal y
        float *nz)      // collision normal z
{
    // Default result
    *nx = *ny = *nz = 0;
    float t = 1.0;

    // No velocity -> no collision
    if (vx == 0 && vy == 0 && vz == 0) { return t; }

    float bbx, bby, bbz, bbex, bbey, bbez;
    box_broadphase(
            x, y, z, ex, ey, ez, vx, vy, vz, &bbx, &bby, &bbz,
            &bbex, &bbey, &bbez);

    // Current block that the bounding box is inside of
    int cx, cy, cz;
    cx = roundf(x);
    cy = roundf(y);
    cz = roundf(z);

    // All possible surrounding blocks
    int x0, y0, z0, x1, y1, z1;
    box_nearest_blocks(
            bbx, bby, bbz, bbex, bbey, bbez, &x0, &y0, &z0, &x1, &y1, &z1);
    for (int bx = x0; bx <= x1; bx++) {
        for (int by = y0; by <= y1; by++) {
            for (int bz = z0; bz <= z1; bz++) {
                // Skip the current block
                if (bx == cx && by == cy && bz == cz) { continue; }
                // Only collide with obstacle blocks
                int w = get_block(g, bx, by, bz);
                if (!block_is_obstacle(g, w)) { continue; }
                // Box must intersect the broad-phase bounding box
                if (!box_intersect_block(bbx, bby, bbz, bbex, bbey, bbez, bx, by, bz)) {
                    continue;
                }
                // Check potential swept block collision
                float snx, sny, snz;
                float st = box_sweep_block(
                        x, y, z, ex, ey, ez, bx, by, bz, vx, vy, vz,
                        &snx, &sny, &snz);
                if (st < 0.0 || st >= 1.0) {
                    // No collision for this frame
                    continue;
                }
                // Can only collide with an exposed block face or a face covered
                // by the current block the player is in
                if (!((bx + snx == cx) && (by + sny == cy) && (bz + sny == cz))
                        && is_block_face_covered(g, bx, by, bz, snx, sny, snz))
                {
                    continue;
                }
                if (st >= 0 && st < t) {
                    //dsq = sdsq;
                    t = st;
                    *nx = snx;
                    *ny = sny;
                    *nz = snz;
                }
            }
        }
    }
    return t;
}

// Linear function: damage(d_vel) = a + b * d_vel IF d_vel > min
float
calc_damage_from_impulse(
        Model *g,
        float d_vel)
{
    const float min_impulse_damage = g->physics.min_impulse_damage;
    const float impulse_damage_min = g->physics.min_impulse_damage;
    const float impulse_damage_scale = g->physics.impulse_damage_scale;
    if (d_vel < min_impulse_damage) { return 0; }
    return roundf(impulse_damage_min + impulse_damage_scale * d_vel);
}


void
set_matrix_3d_player_camera(  // Everything except the player pointer is output
        Model *g,
        float matrix[16],     // [output]
        const Player *p)      // Player to get camera for
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


int
game_get_block_props(
        const Model *g,
        BlockProperties **out_block_props)
{
    *out_block_props = g->the_block_types;
    return g->the_block_types_count;
}

