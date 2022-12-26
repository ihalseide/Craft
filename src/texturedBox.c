#include "texturedBox.h"


// Make a textured 3D box (positioned at the origin)
void
make_box(
        float *data,              // output pointer
        const float ao[6][4],
        const float light[6][4],
        const TexturedBox *boxTexture,
        float center_x,
        float center_y,
        float center_z)
{
    // 6 faces each with 4 points, each of which are 3-vectors
    static const float positions[6][4][3] = {
        {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
        {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
        {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
        {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
        {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}
    };
    // 6 faces each with a 3-vector normal direction
    static const float normals[6][3] = {
        {-1, 0, 0},
        {+1, 0, 0},
        {0, +1, 0},
        {0, -1, 0},
        {0, 0, -1},
        {0, 0, +1}
    };
    // 6 faces each with 4 points, each of which are 2-vectors
    static const float uvs[6][4][2] = {
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
        {{0, 1}, {0, 0}, {1, 1}, {1, 0}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{1, 0}, {1, 1}, {0, 0}, {0, 1}}
    };
    static const float indices[6][6] = {
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3},
        {0, 3, 2, 0, 1, 3},
        {0, 3, 1, 0, 2, 3}
    };
    // flipped indices
    static const float flipped[6][6] = {
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1},
        {0, 1, 2, 1, 3, 2},
        {0, 2, 1, 2, 3, 1}
    };
    // left, right, top, bottom, front, back
    const int faces_x[6] = { boxTexture->left.x, boxTexture->right.x, boxTexture->top.x,
                             boxTexture->bottom.x, boxTexture->front.x, boxTexture->back.x };
    const int faces_y[6] = { boxTexture->left.y, boxTexture->right.y, boxTexture->top.y,
                             boxTexture->bottom.y, boxTexture->front.y, boxTexture->back.y };
    const int faces_u_width[6] = { boxTexture->z_depth, boxTexture->z_depth, boxTexture->x_width,
                                 boxTexture->x_width, boxTexture->x_width, boxTexture->x_width };
    const int faces_v_width[6] = { boxTexture->y_height, boxTexture->y_height, boxTexture->z_depth,
                                  boxTexture->z_depth, boxTexture->y_height, boxTexture->y_height};
    const float pixelsPerBlock = 16;
    const float texturePixelWidth = 256;
    const float texturePixelHeight = 256;
    const float extent_x = (boxTexture->x_width  / 2.0) / pixelsPerBlock; // for block position
    const float extent_y = (boxTexture->y_height / 2.0) / pixelsPerBlock; // for block position
    const float extent_z = (boxTexture->z_depth  / 2.0) / pixelsPerBlock; // for block position
    for (int face = 0; face < 6; face++)
    {
        if ((faces_x[face] < 0) || (faces_y[face] < 0))
        {
            // If the texture coordinate for a face is negative, then that is a signal to skip that face
            // Note: the data pointer is purposefully not incremented when we skip this face!
            continue;
        }
        const float u0 = faces_x[face] / texturePixelWidth;
        const float u1 = (faces_x[face] + faces_u_width[face]) / texturePixelWidth;
        const float v0 = 1.0 - (faces_y[face] / texturePixelHeight);
        const float v1 = 1.0 - ((faces_y[face] + faces_v_width[face]) / texturePixelHeight);
        const int is_flipped_vertex = (ao[face][0] + ao[face][3]) > (ao[face][1] + ao[face][2]);
        for (int vertex_it = 0; vertex_it < 6; vertex_it++)
        {
            // 10 floats of data for each vertex...

            // vertex is index into the face's points
            const int vertex = is_flipped_vertex ? flipped[face][vertex_it] : indices[face][vertex_it];

            // Write the position 3-vector
            *(data++) = center_x + extent_x * positions[face][vertex][0];
            *(data++) = center_y + extent_y * positions[face][vertex][1];
            *(data++) = center_z + extent_z * positions[face][vertex][2];
            // Write the normal 3-vector
            *(data++) = normals[face][0];
            *(data++) = normals[face][1];
            *(data++) = normals[face][2];
            // Write the the UV
            *(data++) = uvs[face][vertex][0] ? u0 : u1;
            *(data++) = uvs[face][vertex][1] ? v0 : v1;
            // ataWrite ao and light data
            *(data++) = ao[face][vertex];
            *(data++) = light[face][vertex];
        }
    }
}

