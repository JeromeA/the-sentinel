/* The monochrome font atlas is compiled to eight bytes per glyph. */
#include "font_assets.h"

/* Double-width 8-row glyph rendering. Pixel selection, clipping-free row
 * traversal and planar mask construction are C; port_out16 is the EGA boundary.
 * Preserve the packed-four-bit backend's original behavior, including its
 * discarded font byte, rather than silently fixing that legacy rendering bug. */
extern uint8_t game_font_8x8[];

static uint16_t duplicate_glyph_bits(uint8_t glyph)
{
    uint16_t mask = 0;
    unsigned pixel;
    for (pixel = 0; pixel != 8; ++pixel)
        if (glyph & (0x80u >> pixel))
            mask |= 0xc000u >> (pixel * 2);
    return mask;
}

/* Native calls retain the original 16-bit address-wrap spelling of a glyph
 * index. Validate its effective index before row/color lookup or video writes. */
int __far glyph_request_valid(const struct GlyphRequest *request)
{
    return (request->character & 0x1fff) < 64 && request->row < 24 && request->column <= 38;
}

/* C callers name glyph inputs; preparation resolves the row and CGA pixel words. */
void __far prepare_glyph_request(
    SegmentBytes data, struct GlyphRequest *request, enum GlyphLayout mode)
{
    request->row_offset =
        segment_word(data, (uint16_t)video_scanline_offsets + (request->row * 8 + 8) * 2);
    if (mode == 0) {
        request->foreground = cga_text_color_pixels[request->foreground & 15] * 0x101u;
        request->background = cga_text_color_pixels[request->background & 15] * 0x101u;
    }
}

void __far GLYPH_REGISTER_CALL font_glyph(SegmentBytes data,
    const struct GlyphRequest *request,
    int backwards,
    struct GlyphEnd *end,
    enum GlyphLayout mode)
{
    uint16_t character = request->character;
    uint16_t column = request->column;
    uint8_t foreground = (uint8_t)request->foreground;
    uint8_t background = (uint8_t)request->background;
    uint8_t transparent = (uint8_t)request->transparent;
    uint8_t skip = (uint8_t)request->skip;
    uint16_t source = (character & 0x1fff) * 8 - (skip ? 1 : 0);
    uint16_t destination = request->row_offset;
    uint16_t source_step = backwards ? (uint16_t)-1 : 1;
    uint16_t write_step = backwards ? (uint16_t)-1 : 1;
    uint16_t foreground_word = 0;
    uint16_t background_word = 0;
    uint16_t last_background = 0;
    uint16_t bank_step = 0;
    uint16_t other_step = 0;
    uint16_t last_carry = 0;
    uint16_t packed_colors = 0;
    uint16_t wrapped = 0;
    if (!glyph_request_valid(request)) {
        if (end)
            *end = (struct GlyphEnd){0};
        return;
    }
    SegmentBytes target;
    if (mode == 0) {
        target = segment_bytes(video_target_base);
        destination += column * 2;
        foreground_word = request->foreground;
        background_word = request->background;
        bank_step = destination & 0x2000 ? 0xe04c : 0x1ffc;
        other_step = destination & 0x2000 ? 0x1ffc : 0xe04c;
    } else if (mode == 1) {
        target = segment_bytes(0xb000);
        destination += column * 4;
        foreground &= 15;
        background &= 15;
        if (end)
            packed_colors = ((uint16_t)background << 8) | ((uint16_t)background << 4) | foreground;
    } else {
        target = segment_bytes(0xa000);
        destination += mode == 2 ? column * 8 : column;
        if (mode == 3) {
            port_out16(0x3ce, 0x0a05);
            port_out16(0x3ce, 7);
        }
    }
    unsigned y;
    for (y = 0; y != 8; ++y) {
        /* A row shift at the first glyph, or reverse traversal at its start,
         * has a defined blank leading row instead of reading another object. */
        uint8_t glyph = source < sizeof(game_font_8x8) ? data[(uint16_t)game_font_8x8 + source] : 0;
        source += source_step;
        if (mode == 0) {
            unsigned half;
            for (half = 0; half != 2; ++half) {
                unsigned nibble = half ? glyph & 15 : glyph >> 4;
                uint16_t mask = cga_font_nibble_masks[nibble].value;
                uint16_t behind = transparent ? segment_word(target, destination) : background_word;
                last_background = (uint16_t)~mask & behind;
                segment_word_store(target, destination, (mask & foreground_word) | last_background);
                destination += write_step * 2;
            }
            if (end)
                last_carry = (uint16_t)(destination + bank_step) < destination;
            destination += bank_step;
            {
                uint16_t swap = bank_step;
                bank_step = other_step;
                other_step = swap;
            }
        } else if (mode == 1) {
            unsigned pixel;
            /* The assembly shifts AH after LODSB overwrites AL. AH starts at
             * zero and stays zero, so this path always selects background. */
            for (pixel = 0; pixel != 8; ++pixel) {
                if (transparent)
                    ++destination;
                else {
                    target[destination] = background;
                    destination += write_step;
                }
            }
            destination += 0x1ff0;
            if (end)
                wrapped = !!(destination & 0x8000);
            if (destination & 0x8000) {
                if (end)
                    last_carry = (uint16_t)(destination + 0x80a0) < destination;
                destination += 0x80a0;
            }
        } else if (mode == 2) {
            unsigned pixel;
            for (pixel = 0; pixel != 16; ++pixel) {
                if (glyph & (0x80u >> (pixel / 2))) {
                    target[destination] = foreground;
                    destination += write_step;
                } else if (transparent)
                    ++destination;
                else {
                    target[destination] = background;
                    destination += write_step;
                }
            }
            if (end)
                last_carry = (uint16_t)(destination + 0x130) < destination;
            destination += 0x130;
        } else {
            uint16_t bits = duplicate_glyph_bits(glyph);
            unsigned half;
            for (half = 0; half != 2; ++half) {
                uint8_t mask = half ? (uint8_t)bits : (uint8_t)(bits >> 8);
                port_out16(0x3ce, ((uint16_t)mask << 8) | 8);
                target[destination] &= foreground;
                if (!transparent) {
                    port_out16(0x3ce, ((uint16_t)(uint8_t)~mask << 8) | 8);
                    target[destination] &= background;
                }
                if (!half)
                    ++destination;
            }
            if (end)
                last_carry = (uint16_t)(destination + 0x27) < destination;
            destination += 0x27;
        }
    }
    if (end) {
        end->last_background = last_background;
        end->bank_step = bank_step;
        end->packed_colors = packed_colors;
        end->destination_offset = destination;
        end->carry = last_carry;
        end->wrapped = wrapped;
    }
}

/* Owned subsystem data. */
uint8_t cga_text_color_pixels[16] = {
    0x00,
    0x55,
    0xaa,
    0xff,
    0x55,
    0xaa,
    0xff,
    0x55,
    0xaa,
    0xff,
    0x55,
    0xaa,
    0xff,
    0x55,
    0xaa,
    0xff,
};
struct UnalignedWord cga_font_nibble_masks[16] = {{0x0000},
    {0x0f00},
    {0xf000},
    {0xff00},
    {0x000f},
    {0x0f0f},
    {0xf00f},
    {0xff0f},
    {0x00f0},
    {0x0ff0},
    {0xf0f0},
    {0xfff0},
    {0x00ff},
    {0x0fff},
    {0xf0ff},
    {0xffff}};
