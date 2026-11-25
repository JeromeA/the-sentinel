#ifndef SENTINEL_TERRAIN_CELL_H
#define SENTINEL_TERRAIN_CELL_H
#include <stdint.h>

/* Gameplay encoding only: generation also uses these bytes for noise and
 * pre-swap heights. Decode a local copy, never volatile bitfield storage. */
union __attribute__((packed)) TerrainCell {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t shape : 4;
        uint8_t height : 4;
    } terrain;

    struct __attribute__((packed)) {
        uint8_t slot : 6;
        uint8_t tag : 2;
    } object;
};

_Static_assert(sizeof(union TerrainCell) == 1, "terrain cell byte encoding");

enum {
    TERRAIN_OBJECT_TAG = 3
};

/* Preserve the original raw construction for arbitrary bytes, including
 * high slot bits and height overflow. Publish the completed byte once. */
static inline union TerrainCell terrain_object_cell(uint8_t slot)
{
    return (union TerrainCell){.raw = slot | 0xc0};
}

static inline union TerrainCell terrain_flat_cell(uint8_t height)
{
    return (union TerrainCell){.raw = height << 4};
}
#endif
