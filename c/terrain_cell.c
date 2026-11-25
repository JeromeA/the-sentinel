
/* Original DS:4B96, now owned independently by this module. */
uint8_t terrain_triangle_vertex_delta[4] = {
    0x01,
    0x21,
    0xff,
    0x1f,
};

/* Original DS:4B9A, now owned independently by this module. */
union MaterialColors terrain_face_color_pairs[32] = {
    {.raw = 0x33},
    {.raw = 0x10},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x12},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x11},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x11},
    {.raw = 0x11},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x11},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x11},
    {.raw = 0x11},
    {.raw = 0x10},
    {.raw = 0x12},
    {.raw = 0x10},
    {.raw = 0x11},
    {.raw = 0x10},
    {.raw = 0x12},
};

/* Original DS:4BBA, now owned independently by this module. */
uint8_t terrain_triangle_orientations[16] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
};

/* Distance shading and terrain shape classification feed the common polygon
 * path. Occupied cells draw their checkerboard floor before the object stack. */

extern uint8_t terrain_triangle_orientations[];
extern union MaterialColors terrain_face_color_pairs[];
