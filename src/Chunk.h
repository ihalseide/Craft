#ifndef _Chunk_h
#define _Chunk_h

#include "map.h"
#include "sign.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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


#endif
