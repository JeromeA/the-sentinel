/* Data retains its original position in the primary translation unit.
 * Cursor algorithms compile in the secondary code bank with the same far ABI. */
#ifdef SOFTWARE_CURSOR_DATA_ONLY
/* Owned subsystem data. */
/* Saved video addresses with the original 48-entry capacity. */
struct UnalignedWord cursor_video_offset_storage[48] = {0};
/* Saved crosshair pixels and retained capacity for four planar bytes per step. */
uint8_t cursor_pixel_storage[288] = {0};
uint8_t cga_clear_pixel_masks[4] = {
    0x3f,
    0xcf,
    0xf3,
    0xfc,
};
uint8_t cga_set_pixel_masks[4] = {
    0x40,
    0x10,
    0x04,
    0x01,
};
uint8_t tandy_clear_pixel_masks[2] = {
    0x0f,
    0xf0,
};
uint8_t tandy_set_pixel_masks[2] = {
    0xe0,
    0x0e,
};
uint8_t ega_single_pixel_masks[8] = {
    0x80,
    0x40,
    0x20,
    0x10,
    0x08,
    0x04,
    0x02,
    0x01,
};
struct MouseInputStateStorage mouse_input_state_storage = {
    .x = 0xa0, .y = 0x68, .buttons = 0x0, .moved = 0x0, .installed = 0x0};
uint8_t cursor_step_ink_records[42] = {
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0xff,
    0xff,
};
uint8_t cursor_step_x_deltas[22] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xfb,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0xfb,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};
uint8_t cursor_step_scanline_deltas[22] = {
    0x00,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x80,
};
uint8_t cursor_cga_clear_pair_masks[2] = {
    0x0f,
    0xf0,
};
uint8_t cursor_cga_set_pair_masks[2] = {
    0xf0,
    0x0f,
};
uint8_t cursor_ega_pair_masks[4] = {
    0xc0,
    0x30,
    0x0c,
    0x03,
};
uint8_t untraced_signed_step_vectors[17] = {
    0xe0,
    0xff,
    0x20,
    0x00,
    0x00,
    0x0a,
    0x00,
    0xf6,
    0xf0,
    0xff,
    0x10,
    0x00,
    0x00,
    0x0a,
    0x00,
    0xf6,
    0x00,
};

#else
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

static void port_out16(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0,%1" : : "a"(value), "d"(port));
}

/* Save the two pixels covered by each crosshair step and restore them backwards.
 * EGA records the four planes separately; repeated offsets must be restored in
 * reverse order because neighboring cursor steps can cover the same byte. */

void __far restore_cursor(
    SegmentBytes data, uint16_t step_bank, struct CursorRestoreEnd *end, enum VideoPixelLayout mode)
{
    uint16_t index = step_bank | software_cursor_saved_last_step;
    SegmentBytes screen = segment_bytes(mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : 0xa000);
    if (end)
        end->restored = 0;
    if (index > 21)
        return;
    if (end)
        end->restored = !!index;
    if (index) {
        if (mode == 2) {
            ega_polygon_busy = 255;
            port_out16(0x3ce, 1);
            port_out16(0x3ce, 5);
            port_out16(0x3ce, 0xff08);
        }
        int step;
        for (step = index; step >= 0; --step) {
            unsigned pixel_offset = step * (mode == 2 ? 4 : 2);
            unsigned address_index = step * (mode == 2 ? 2 : 1);
            uint16_t destination = cursor_saved_video_offsets[address_index].value;
            if (end)
                end->destination_offset = destination;
            if (mode == 2) {
                unsigned plane;
                for (plane = 0; plane < 4; ++plane) {
                    port_out16(0x3c4, ((uint16_t)(8 >> plane) << 8) | 2);
                    uint8_t pixel = cursor_saved_pixels[pixel_offset + plane];
                    if (end)
                        end->plane_value = pixel;
                    screen[destination] = pixel;
                }
            } else if (mode == 3)
                segment_word_store(screen,
                    destination,
                    segment_word(data, (uint16_t)cursor_saved_pixels + pixel_offset));
            else
                screen[destination] = cursor_saved_pixels[pixel_offset];
        }
        if (mode == 2) {
            port_out16(0x3c4, 0x0f02);
            ega_polygon_busy = 0;
        }
        software_cursor_saved_last_step = 0;
    }
    if (mode == 2)
        port_out16(0x3ce, 0x105);
}

uint8_t __far draw_cursor_effects(SegmentBytes data,
    uint16_t initial_bank,
    struct CursorDrawEnd *end,
    enum VideoPixelLayout mode,
    uint16_t *flags)
{
    /* Accepted steps are below 22; even planar save offsets stay below 88.
     * The terminal observation therefore has no bank and cannot steer traversal. */
    uint8_t terminal_observation = 0;
    if (end)
        end->executed = end->destination_updated = end->mask_updated = 0;
    SegmentBytes screen = segment_bytes(mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : 0xa000);
    uint8_t skip = software_cursor_hide_flags;
    if (end)
        end->skip_status = skip;
    if (flags)
        *flags = status(*flags, skip, 8);
    if ((initial_bank & 0xff00) || software_cursor_saved_last_step > 21) {
        if (end)
            end->skip_status = 128;
        if (flags)
            *flags = status(*flags, 128, 8);
        return 0;
    }
    if (software_cursor_hide_flags & 128)
        return 0;
    if (mode == 2) {
        skip = ega_polygon_busy;
        if (end)
            end->skip_status = skip;
        if (flags)
            *flags = status(*flags, skip, 8);
        if (ega_polygon_busy & 128)
            return 0;
    }
    if (end)
        end->executed = 1;
    struct CursorRestoreEnd restored;
    restore_cursor(data, initial_bank, end ? &restored : 0, mode);
    if (end && restored.restored) {
        if (end) {
            end->destination_offset = restored.destination_offset;
            end->destination_updated = 1;
        }
    }
    if (mode == 2) {
        ega_polygon_busy = 255;
        port_out16(0x3ce, 0x205);
    }
    uint16_t row_offset = (uint16_t)(uint8_t)(199 - software_cursor_inverted_y) * 2;
    uint16_t x = software_cursor_x_pairs;
    for (;;) {
        uint8_t step = software_cursor_saved_last_step;
        if (step >= 22)
            break;
        uint8_t delta = cursor_step_scanline_deltas[step];
        if (end)
            terminal_observation = step;
        if (delta & 128)
            break;
        row_offset += (int16_t)(int8_t)delta;
        x += (int16_t)(int8_t)cursor_step_x_deltas[step];
        uint16_t destination = video_scanline_offsets[row_offset / 2];
        uint16_t index = step * (mode == 2 ? 4 : 2);
        uint8_t ink;
        if (mode == 0) {
            unsigned pair = x & 1;
            uint16_t bank = destination + x / 2;
            destination = ((bank + video_page_offset) & 0x1fff) | (bank & 0x2000);
            cursor_saved_video_offsets[index / 2].value = destination;
            uint8_t value = screen[destination];
            cursor_saved_pixels[index] = value;
            value &= cursor_cga_clear_pair_masks[pair];
            if (cursor_step_ink_records[index])
                value |= cursor_cga_set_pair_masks[pair];
            screen[destination] = value;
            if (end) {
                end->mask_index = pair;
                end->mask_updated = 1;
            }
        } else if (mode == 1) {
            destination += x;
            cursor_saved_video_offsets[index / 2].value = destination;
            cursor_saved_pixels[index] = screen[destination];
            screen[destination] = cursor_step_ink_records[index] ? 0xee : 0;
        } else if (mode == 2) {
            destination += video_page_offset + x / 4;
            cursor_saved_video_offsets[index / 2].value = destination;
            port_out16(0x3ce, ((uint16_t)cursor_ega_pair_masks[x & 3] << 8) | 8);
            unsigned plane;
            for (plane = 0; plane < 4; ++plane) {
                port_out16(0x3ce, ((uint16_t)(3 - plane) << 8) | 4);
                cursor_saved_pixels[index + plane] = screen[destination];
                if (end)
                    terminal_observation = cursor_saved_pixels[index + plane];
            }
            ink = cursor_step_ink_records[index / 2] ? 14 : 0;
            screen[destination] = ink;
            if (end) {
                end->mask_index = index / 2;
                end->mask_updated = 1;
            }
        } else {
            destination += video_page_offset + x * 2;
            cursor_saved_video_offsets[index / 2].value = destination;
            segment_word_store(
                data, (uint16_t)cursor_saved_pixels + index, segment_word(screen, destination));
            segment_word_store(screen, destination, cursor_step_ink_records[index] ? 0x0e0e : 0);
        }
        if (end) {
            end->destination_offset = destination;
            end->destination_updated = 1;
        }
        ++software_cursor_saved_last_step;
    }
    if (mode == 2) {
        port_out16(0x3ce, 0x105);
        ega_polygon_busy = 0;
    }
    uint8_t saved_count = software_cursor_saved_last_step;
    if (end)
        end->saved_count = saved_count;
    if (flags)
        *flags = subtract8(*flags, saved_count, 1) & ~CF;
    --software_cursor_saved_last_step;
    if (end) {
        end->row_offset = row_offset;
        end->x_pairs = x;
        end->terminal_step = terminal_observation;
    }
    return 1;
}

void __far draw_cursor(
    SegmentBytes data, uint16_t bank, struct CursorDrawEnd *end, enum VideoPixelLayout mode)
{
    draw_cursor_effects(data, bank, end, mode, 0);
}

#endif
