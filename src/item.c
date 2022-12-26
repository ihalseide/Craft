#include "item.h"
#include "util.h"
#include <assert.h>

// The list of block ids the player can build.
// Note: the player cannot build every block type (such as clouds).
const int items[] = {
    GRASS,
    SAND,
    STONE,
    BRICK,
    WOOD,
    CEMENT,
    DIRT,
    PLANK,
    SNOW,
    GLASS,
    COBBLE,
    LIGHT_STONE,
    DARK_STONE,
    CHEST,
    LEAVES,
    TALL_GRASS,
    YELLOW_FLOWER,
    RED_FLOWER,
    PURPLE_FLOWER,
    SUN_FLOWER,
    WHITE_FLOWER,
    BLUE_FLOWER,
};

const int item_count = sizeof(items) / sizeof(int);

const int blocks[256][6] = {
    // maps w (block id) => (left, right, top, bottom, front, back) tiles
    {0, 0, 0, 0, 0, 0}, // 0 - empty
    {16, 16, 32, 0, 16, 16}, // 1 - grass
    {1, 1, 1, 1, 1, 1}, // 2 - sand
    {2, 2, 2, 2, 2, 2}, // 3 - stone
    {3, 3, 3, 3, 3, 3}, // 4 - brick
    {20, 20, 36, 4, 20, 20}, // 5 - wood
    {5, 5, 5, 5, 5, 5}, // 6 - cement
    {6, 6, 6, 6, 6, 6}, // 7 - dirt
    {7, 7, 7, 7, 7, 7}, // 8 - plank
    {24, 24, 40, 8, 24, 24}, // 9 - snow
    {9, 9, 9, 9, 9, 9}, // 10 - glass
    {10, 10, 10, 10, 10, 10}, // 11 - cobble
    {11, 11, 11, 11, 11, 11}, // 12 - light stone
    {12, 12, 12, 12, 12, 12}, // 13 - dark stone
    {13, 13, 13, 13, 13, 13}, // 14 - chest
    {14, 14, 14, 14, 14, 14}, // 15 - leaves
    {15, 15, 15, 15, 15, 15}, // 16 - cloud
    {0, 0, 0, 0, 0, 0}, // 17
    {0, 0, 0, 0, 0, 0}, // 18
    {0, 0, 0, 0, 0, 0}, // 19
    {0, 0, 0, 0, 0, 0}, // 20
    {0, 0, 0, 0, 0, 0}, // 21
    {0, 0, 0, 0, 0, 0}, // 22
    {0, 0, 0, 0, 0, 0}, // 23
    {0, 0, 0, 0, 0, 0}, // 24
    {0, 0, 0, 0, 0, 0}, // 25
    {0, 0, 0, 0, 0, 0}, // 26
    {0, 0, 0, 0, 0, 0}, // 27
    {0, 0, 0, 0, 0, 0}, // 28
    {0, 0, 0, 0, 0, 0}, // 29
    {0, 0, 0, 0, 0, 0}, // 30
    {0, 0, 0, 0, 0, 0}, // 31
};

const int plants[256] = {
    // w (block id) => tile
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 16
    48, // 17 - tall grass
    49, // 18 - yellow flower
    50, // 19 - red flower
    51, // 20 - purple flower
    52, // 21 - sun flower
    53, // 22 - white flower
    54, // 23 - blue flower
};

// Predicate function for whether a block id is a plant type
// Arguments:
// - w: block id (block type)
// Returns:
// - boolean whether block type is a plant
int is_plant(int w) {
    switch (w) {
    case TALL_GRASS:
    case YELLOW_FLOWER:
    case RED_FLOWER:
    case PURPLE_FLOWER:
    case SUN_FLOWER:
    case WHITE_FLOWER:
    case BLUE_FLOWER:
        return 1;
    default:
        return 0;
    }
}

// Predicate function for whether a block id is an obstacle (blocking movement)
// Arguments:
// - w: block id (block type)
// Returns:
// - boolean whether block type is obstacle
int is_obstacle(int w) {
    w = ABS(w);
    if (is_plant(w)) { return 0; }
    switch (w) {
    case EMPTY:
    case CLOUD:
        return 0;
    default:
        return 1;
    }
}

// Predicate function for whether a block id is transparent
// Arguments:
// - w: block id (block type)
// Returns:
// - boolean whether block type is transparent
int is_transparent(int w) {
    if (w == EMPTY) { return 1; }
    w = ABS(w);
    if (is_plant(w)) { return 1; }
    switch (w) {
    case EMPTY:
    case GLASS:
    case LEAVES:
        return 1;
    default:
        return 0;
    }
}

// Predicate function for whether a block id is destructable
// Arguments:
// - w: block id (block type)
// Returns:
// - boolean whether block type is destructable
int is_destructable(int w) {
    switch (w) {
    case EMPTY:
    case CLOUD:
        return 0;
    default:
        return 1;
    }
}


// Return the minimum amount of damage that is required in order to
// change the block's damage value.
int block_get_min_damage_threshold(int w) {
    switch (w) {
    case STONE:
    case BRICK:
    case CEMENT:
    case DARK_STONE:
    case LIGHT_STONE:
        return 3;
    case COBBLE:
        return 2;
    default:
        return 1;
    }
}


// Return the damage value that destroys a block.
int block_get_max_damage(int w) {
    switch (w) {
    case STONE:
    case BRICK:
    case CEMENT:
    case DARK_STONE:
    case LIGHT_STONE:
    case COBBLE:
        return 100;
    case GRASS:
        return 60;
    case DIRT:
    case SAND:
        return 50;
    default:
        if (is_plant(w)) { return 1; }
        return 2;
    }
}


int
block_type_get_texture_x(
        int tile_id)
{
    int tileSize = 16;
    int tilesPerRow = 16;
    int value = (tile_id % tilesPerRow) * tileSize;
    return value;
}


int
block_type_get_texture_y(
        int tile_id)
{
    int textureHeight = 256;
    int tileSize = 16;
    int tilesPerRow = 16;
    int value = textureHeight - ((1 + tile_id / tilesPerRow) * tileSize);
    //int value = (tile_id / tilesPerRow) * tileSize;
    return value;
}


static
void
get_texture_coordinates_for_block_if_yes(
        PointInt2 *out_texture_point,
        int yes,
        int texture_tile_id)
{
    int x, y;
    if (yes)
    {
        x = block_type_get_texture_x(texture_tile_id);
        y = block_type_get_texture_y(texture_tile_id);
//        if (texture_tile_id == GRASS)
//        {
//            x = 0;
//            y = 224;
//        }
        assert(x >= 0);
        assert(y >= 0);
    }
    else
    {
        x = -1;
        y = -1;
    }
    out_texture_point->x = x;
    out_texture_point->y = y;
}


void
get_textured_box_for_block(
        int w,                          // tile id
        int left,                       // whether to generate the left   face or not
        int right,                      // whether to generate the right  face or not
        int top,                        // whether to generate the top    face or not
        int bottom,                     // whether to generate the bottom face or not
        int front,                      // whether to generate the front  face or not
        int back,                       // whether to generate the back   face or not
        TexturedBox *out_textured_box)  // where to put the result
{
    /* Tip: change pixelsPerBlock to a smaller number than 16 to visually debug block faces! */
    const int pixelsPerBlock = 16;
    TexturedBox *o = out_textured_box;
    o->x_width  = pixelsPerBlock;
    o->y_height = pixelsPerBlock;
    o->z_depth  = pixelsPerBlock;
    get_texture_coordinates_for_block_if_yes(&o->left,   left,   blocks[w][0]);
    get_texture_coordinates_for_block_if_yes(&o->right,  right,  blocks[w][1]);
    get_texture_coordinates_for_block_if_yes(&o->top,    top,    blocks[w][2]);
    get_texture_coordinates_for_block_if_yes(&o->bottom, bottom, blocks[w][3]);
    get_texture_coordinates_for_block_if_yes(&o->front,  front,  blocks[w][4]);
    get_texture_coordinates_for_block_if_yes(&o->back,   back,   blocks[w][5]);
}

