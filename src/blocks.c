#include "blocks.h"
#include "util.h"
#include <assert.h>


static
const BlockProperties *
get_valid_block_properties(
        const Model *g,
        BlockKind w)
{
    w = ABS(w);

    if (w == 0)
    {
        return NULL;
    }

    BlockProperties *block_props = NULL;
    int count = game_get_block_props(g, &block_props);
    assert(block_props);
    if (w-1 >= count)
    {
        return NULL;
    }

    // Block ids are from 0 to whatever, but the ID 0 is reserved for empty air.
    // This converts the id into an index for the block properties array
    return &block_props[w - 1];
}


bool 
block_is_plant(
        const Model *g,
        BlockKind w)
{
    if (!w) { return false; }
    return get_valid_block_properties(g, w)->is_plant;
}


bool
block_is_obstacle(
        const Model *g,
        BlockKind w)
{
    if (!w) { return false; }
    return get_valid_block_properties(g, w)->is_obstacle;
}


bool
block_is_transparent(
        const Model *g,
        BlockKind w)
{
    if (!w) { return true; }
    return get_valid_block_properties(g, w)->is_transparent;
}


bool
block_is_destructable(
        const Model *g,
        BlockKind w)
{
    if (!w) { return false; }
    return get_valid_block_properties(g, w)->max_damage > 0;
}


int
block_get_max_damage(
        const Model *g,
        BlockKind w)
{
    if (!w) { return 0; }
    return get_valid_block_properties(g, w)->max_damage;
}


int
block_get_min_damage_threshold(
        const Model *g,
        BlockKind w)
{
    if (!w) { return 0; }
    return get_valid_block_properties(g, w)->min_damage_change;
}


static
void
get_tile_coords(          // Convert a tile number in the texture file to actual pixel coordinates for the tile
        int tile_number,
        int *out_x,
        int *out_y)
{
    const int tileSize = 16;
    const int tilesPerRow = 16;
    *out_x = (tile_number % tilesPerRow) * tileSize;
    *out_y = (tile_number / tilesPerRow) * tileSize;
}


static
void
get_texture_coordinates_for_block_if_yes(  // Either sets pixel coordinates for a tile number or sets the texture coordinates to a "none" value
        PointInt2 *out_texture_point,
        int yes,
        int texture_tile_id)
{
    if (yes)
    {
        get_tile_coords(texture_tile_id, &out_texture_point->x, &out_texture_point->y);
        assert(out_texture_point->x >= 0);
        assert(out_texture_point->y >= 0);
    }
    else
    {
        out_texture_point->x = -1;
        out_texture_point->y = -1;
    }
}


void
get_textured_box_for_block(
        const Model *g,
        BlockKind w,
        bool left,
        bool right,
        bool top,
        bool bottom,
        bool front,
        bool back,
        TexturedBox *out_textured_box)
{

    TexturedBox *o = out_textured_box;
    const BlockProperties * const p = get_valid_block_properties(g, w);  // properties

    /* DEBUG TIP: change `pixels_per_block` to a smaller number than 16 to visually debug block faces! */
    const int pixels_per_block = 16;
    o->x_width  = pixels_per_block;
    o->y_height = pixels_per_block;
    o->z_depth  = pixels_per_block;

    get_texture_coordinates_for_block_if_yes(&o->left, left, p->left_face.texture_tile_index);
    o->left_flip = p->left_face.flip_code;

    get_texture_coordinates_for_block_if_yes(&o->right, right, p->right_face.texture_tile_index);
    o->right_flip = p->right_face.flip_code;

    get_texture_coordinates_for_block_if_yes(&o->top, top, p->top_face.texture_tile_index);
    o->top_flip = p->top_face.flip_code;

    get_texture_coordinates_for_block_if_yes(&o->bottom, bottom, p->bottom_face.texture_tile_index);
    o->bottom_flip = p->bottom_face.flip_code;

    get_texture_coordinates_for_block_if_yes(&o->front, front, p->front_face.texture_tile_index);
    o->front_flip = p->front_face.flip_code;

    get_texture_coordinates_for_block_if_yes(&o->back, back, p->back_face.texture_tile_index);
    o->back_flip = p->back_face.flip_code;
}
