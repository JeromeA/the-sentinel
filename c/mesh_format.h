#ifndef SENTINEL_MESH_FORMAT_H
#define SENTINEL_MESH_FORMAT_H
#include <stdint.h>

/* Decode local raw-byte snapshots. Reserved face bits and negative zero are
 * valid encodings; asset storage and compatibility boundaries retain raw. */
union __attribute__((packed)) MeshFaceHeader {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t vertex_count : 3;
        uint8_t reserved : 4;
        uint8_t back_pass : 1;
    } fields;
};

union __attribute__((packed)) MeshHeight {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t magnitude : 7;
        uint8_t negative : 1;
    } fields;
};

/* Asset palette_pair[0] is the high nibble, palette_pair[1] the low nibble.
 * The span backend chooses how the pair is used for dithering. */
union __attribute__((packed)) MaterialColors {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t second : 4;
        uint8_t first : 4;
    } fields;
};

_Static_assert(sizeof(union MeshFaceHeader) == 1, "mesh face byte");
_Static_assert(sizeof(union MeshHeight) == 1, "mesh height byte");
_Static_assert(sizeof(union MaterialColors) == 1, "material color byte");
#endif
