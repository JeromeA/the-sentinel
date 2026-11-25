#ifndef SENTINEL_RGB_PALETTE_H
#define SENTINEL_RGB_PALETTE_H
#include <stdint.h>
#include <stddef.h>
#include "video_copy.h"

enum {
    RGB_PALETTE_COLORS = 16
};

struct __attribute__((packed, may_alias)) RgbColor {
    uint8_t red, green, blue;
};

struct __attribute__((packed, may_alias)) RgbPalette {
    struct RgbColor colors[RGB_PALETTE_COLORS];
};
typedef const volatile struct RgbPalette GAME_FAR *RgbPaletteSource;
_Static_assert(sizeof(struct RgbColor) == 3, "RGB color size");
_Static_assert(offsetof(struct RgbColor, red) == 0, "RGB red offset");
_Static_assert(offsetof(struct RgbColor, green) == 1, "RGB green offset");
_Static_assert(offsetof(struct RgbColor, blue) == 2, "RGB blue offset");
_Static_assert(sizeof(struct RgbPalette) == 48, "RGB palette size");
extern struct RgbPalette vga_scene_rgb_palettes[3], vga_work_palette;
extern struct RgbColor vga_landscape_color_1_variants[8], vga_landscape_color_2_variants[8],
    vga_landscape_color_3_variants[8];
#endif
