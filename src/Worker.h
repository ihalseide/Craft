#ifndef _Worker_h
#define _Worker_h


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tinycthread.h>


enum
{
    WORKER_IDLE = 0,
    WORKER_BUSY = 1,
    WORKER_DONE = 2,
};


struct Model;


// A single item that a Worker can work on
typedef struct
{
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
    const struct Model *game_model;
}
WorkerItem;


typedef struct
{
    int index;
    int state;
    thrd_t thrd;      // thread
    mtx_t mtx;        // mutex
    cnd_t cnd;        // condition
    WorkerItem item;
}
Worker;


#endif
