#ifndef _cube_h_
#define _cube_h_


void make_cube(
        float *data,
        const float ao[6][4],
        const float light[6][4],
        int left,
        int right,
        int top,
        int bottom,
        int front,
        int back,
        float x,
        float y,
        float z,
        float n,
        int w);

void make_plant(
        float *data,
        const float ao,
        const float light,
        float px,
        float py,
        float pz,
        float n,
        int w,
        float rotation);

void make_cube_wireframe(
        float *data,
        float x,
        float y,
        float z,
        float n);

void make_box_wireframe(
        float *data,
        float x,
        float y,
        float z,
        float ex,
        float ey,
        float ez);

void make_character(
        float *data,
        float x,
        float y,
        float n,
        float m,
        char c);

void make_character_3d(
        float *data,
        float x,
        float y,
        float z,
        float n,
        int face,
        char c);

void make_sphere(
        float *data,
        float r,
        int detail);


#endif
