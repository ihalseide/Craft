#ifndef _texturedBox_h_
#define _texturedBox_h_


#include "hitbox.h"


typedef struct
{
    int x;
    int y;
}
PointInt2;


typedef enum
{
    TextureFlipCode_FLIP_NONE = 0,
    TextureFlipCode_FLIP_U = 1,
    TextureFlipCode_FLIP_V = 2,
    TextureFlipCode_FLIP_UV = 3,
}
TextureFlipCode;


typedef struct
{
    PointInt2 left;
    PointInt2 right;
    PointInt2 top;
    PointInt2 bottom;
    PointInt2 front;
    PointInt2 back;
    TextureFlipCode left_flip;
    TextureFlipCode right_flip;
    TextureFlipCode top_flip;
    TextureFlipCode bottom_flip;
    TextureFlipCode front_flip;
    TextureFlipCode back_flip;
    int x_width;
    int y_height;
    int z_depth;
}
TexturedBox;


// Make a textured 3D box
// Data must have room for <60> floats! (10 floats of data for each of the 6 faces)
void
make_box(
        float *data,              // output pointer
        const float ao[6][4],
        const float light[6][4],
        const TexturedBox *boxTexture,
        float center_x,
        float center_y,
        float center_z);


#endif /*_texturedBox_h_*/
