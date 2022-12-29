#ifndef _blocks_h_
#define _blocks_h_


#include "GameModel.h"
#include "texturedBox.h"
#include <stdbool.h>


typedef int BlockKind;


typedef struct
{
    int texture_tile_index;
    TextureFlipCode flip_code;
}
BlockFaceInfo;


typedef struct BlockProperties
{
    BlockFaceInfo left_face;
    BlockFaceInfo right_face;
    BlockFaceInfo top_face;
    BlockFaceInfo bottom_face;
    BlockFaceInfo front_face;
    BlockFaceInfo back_face;
    int max_damage;         // if this is <= 0, then it is indestructable
    int min_damage_change;
    bool is_plant;
    bool is_obstacle;
    bool is_transparent;
}
BlockProperties;


bool
block_is_plant(
        const Model *g,
        BlockKind w);


bool
block_is_obstacle(
        const Model *g,
        BlockKind w);


bool
block_is_transparent(
        const Model *g,
        BlockKind w);


bool
block_is_destructable(
        const Model *g,
        BlockKind w);


int
block_get_max_damage(
        const Model *g,
        BlockKind w);


int
block_get_min_damage_threshold(
        const Model *g,
        BlockKind w);


void
get_textured_box_for_block(              // For texturing (cube) blocks
        const Model *g,
        BlockKind w,                     // tile id
        bool left,                       // whether to generate the left   face or not
        bool right,                      // whether to generate the right  face or not
        bool top,                        // whether to generate the top    face or not
        bool bottom,                     // whether to generate the bottom face or not
        bool front,                      // whether to generate the front  face or not
        bool back,                       // whether to generate the back   face or not
        TexturedBox *out_textured_box);  // where to put the result


#endif /* _blocks_h */
