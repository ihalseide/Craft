#ifndef _texturedBox_h_
#define _texturedBox_h_


#include "hitbox.h"


typedef struct PointInt2
{
    int x;
    int y;
}
PointInt2;


typedef struct TexturedBox
{
    PointInt2 left;
    PointInt2 right;
    PointInt2 top;
    PointInt2 bottom;
    PointInt2 front;
    PointInt2 back;
    int x_width;
    int y_height;
    int z_depth;
}
TexturedBox;


// Make a textured 3D box
// Data must have room for <60> floats! (10 floats of data for each of the 6 faces)
void make_box(
        float *data,              // output pointer
        const float ao[6][4],
        const float light[6][4],
        const TexturedBox *boxTexture,
        float center_x,
        float center_y,
        float center_z);


#endif /*_texturedBox_h_*/
