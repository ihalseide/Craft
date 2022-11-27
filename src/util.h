#ifndef _util_h_
#define _util_h_


#include "config.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))


#if DEBUG
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif


// Frames-per-second context
// - fps: frames per second
// - frames: number of frames collected (since last time)
// - since: the last time the frames per second was updated
typedef struct {
    unsigned int fps;
    unsigned int frames;
    double since;
} FPS;


int char_width(
        char input);

void del_buffer(
        GLuint buffer);

GLuint gen_buffer(
        GLsizei size,
        GLfloat *data);

GLuint gen_faces(
        int components,
        int faces,
        GLfloat *data);

void load_png_texture(
        const char *file_name);

GLuint load_program(
        const char *path1,
        const char *path2);

GLuint load_shader(
        GLenum type,
        const char *path);

GLuint make_program(
        GLuint shader1,
        GLuint shader2);

GLuint make_shader(
        GLenum type,
        const char *source);

GLfloat *malloc_faces(
        int components,
        int faces);

double rand_double();

int rand_int(
        int n);

int string_width(
        const char *input);

char *tokenize(
        char *str,
        const char *delim,
        char **key);

void update_fps(
        FPS *fps);

float v3_mag(
        float x,
        float y,
        float z);

int wrap(
        const char *input,
        int max_width,
        char *output,
        int max_length);


#endif
