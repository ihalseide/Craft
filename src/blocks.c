#include "blocks.h"
#include <assert.h>


static
const BlockProperties *
get_valid_block_properties(
        const Model *g,
        BlockKind w)
{
    if (w <= 0)
    {
        return NULL;
    }

    BlockProperties *block_props = NULL;
    int count = game_get_block_props(g, &block_props);
    assert(block_props);
    if (w >= count)
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
    return get_valid_block_properties(g, w)->is_plant;
}


bool
block_is_obstacle(
        const Model *g,
        BlockKind w)
{
    return get_valid_block_properties(g, w)->is_obstacle;
}


bool
block_is_transparent(
        const Model *g,
        BlockKind w)
{
    return get_valid_block_properties(g, w)->is_transparent;
}


bool
block_is_destructable(
        const Model *g,
        BlockKind w)
{
    return get_valid_block_properties(g, w)->max_damage > 0;
}


int
block_get_max_damage(
        const Model *g,
        BlockKind w)
{
    return get_valid_block_properties(g, w)->max_damage;
}


int
block_get_min_damage_threshold(
        const Model *g,
        BlockKind w)
{
    return get_valid_block_properties(g, w)->min_damage_change;
}


static
void
get_tile_coords(
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
get_texture_coordinates_for_block_if_yes(
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


static
void
get_texture_flips_for_block(
        BlockKind w,
        TexturedBox *out_textured_box)
{
    TexturedBox *o = out_textured_box;
    o->left_flip = TextureFlipCode_FLIP_NONE;
    o->right_flip = TextureFlipCode_FLIP_NONE;
    o->top_flip = TextureFlipCode_FLIP_NONE;
    o->bottom_flip = TextureFlipCode_FLIP_NONE;
    o->front_flip = TextureFlipCode_FLIP_NONE;
    o->back_flip = TextureFlipCode_FLIP_NONE;
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

    /* Tip: change `pixels_per_block` to a smaller number than 16 to visually debug block faces! */
    const int pixels_per_block = 16;
    o->x_width  = pixels_per_block;
    o->y_height = pixels_per_block;
    o->z_depth  = pixels_per_block;

    const BlockProperties *properties = get_valid_block_properties(g, w);
    get_texture_coordinates_for_block_if_yes(&o->left,   left,   properties->left_face.texture_tile_index);
    get_texture_coordinates_for_block_if_yes(&o->right,  right,  properties->right_face.texture_tile_index);
    get_texture_coordinates_for_block_if_yes(&o->top,    top,    properties->top_face.texture_tile_index);
    get_texture_coordinates_for_block_if_yes(&o->bottom, bottom, properties->bottom_face.texture_tile_index);
    get_texture_coordinates_for_block_if_yes(&o->front,  front,  properties->front_face.texture_tile_index);
    get_texture_coordinates_for_block_if_yes(&o->back,   back,   properties->back_face.texture_tile_index);

    get_texture_flips_for_block(w, o);
}
