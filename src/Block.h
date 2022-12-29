#ifndef _Block_h
#define _Block_h


#include <stdbool.h>
#include "texturedBox.h"


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


#endif
