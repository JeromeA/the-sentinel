/* Editable icon sheets are compiled into the adapter-specific tile banks. */
#include "icon_assets.h"

/* Image copies, background patterns and energy tiles share the same row layouts.
 * The C loops keep far image pointers explicit; EGA plane selection remains I/O. */

extern uint8_t status_tiles_cga[], status_tiles_packed[], status_tiles_ega[];

void __far copy_video_bytes(
    SegmentBytes *destination, SegmentBytes *source, uint16_t count, int backwards)
{
    int step = backwards ? -1 : 1;
    while (count--) {
        uint8_t value = **source;
        **destination = value;
        *destination += step;
        *source += step;
    }
}

void __far copy_video_words(
    SegmentBytes *destination, SegmentBytes *source, uint16_t count, int backwards)
{
    int step = backwards ? -2 : 2;
    while (count--) {
        uint16_t value = ((volatile struct UnalignedWord GAME_FAR *)*source)->value;
        ((volatile struct UnalignedWord GAME_FAR *)*destination)->value = value;
        *destination += step;
        *source += step;
    }
}

/* A visible-page HUD/scroll transfer has a raster deadline. The C word loop
 * can outlast display blanking at 6,000 cycles. Keep this hardware transfer at
 * the original REP MOVSW throughput; layout and copy scheduling remain C. */
static __attribute__((noinline)) void video_copy_words_retrace(SegmentBytes to,
    uint16_t *destination,
    SegmentBytes from,
    uint16_t *source,
    uint16_t words,
    uint16_t flags)
{
    uint16_t src = *source;
    uint16_t dst = *destination;
    uint16_t count = words;
    uint16_t source_segment = (uint32_t)from >> 16;
    uint16_t target_segment = (uint32_t)to >> 16;
    __asm__ volatile("pushw %%ds\n\tpushw %%es\n\tmovw %%ax,%%ds\n\tmovw %%dx,%%es\n\t"
                     "testw $0x400,%%bx\n\tjz 1f\n\tstd\n\tjmp 2f\n1:\tcld\n2:\trep movsw\n\t"
                     "cld\n\tpopw %%es\n\tpopw %%ds"
        : "+S"(src), "+D"(dst), "+c"(count)
        : "a"(source_segment), "d"(target_segment), "b"(flags)
        : "memory", "cc");
    *source = src;
    *destination = dst;
}

void __far blit_image(
    const struct ImageSlice *slice, struct ImageCopyEnd *end, enum VideoPixelLayout mode)
{
    SegmentBytes source = segment_bytes(slice->source_segment);
    uint16_t row = slice->first_row;
    uint16_t remaining = slice->rows;
    uint16_t width = slice->columns;
    uint16_t dst = video_scanline_offsets[row];
    uint16_t bank = 0;
    uint16_t other = 0;
    uint16_t tail = 0;
    unsigned wrapped = 0;
    SegmentBytes destination;
    uint16_t src;
    if (mode == 0) {
        destination = segment_bytes(video_target_base);
        src = row * 80;
        bank = dst & 0x2000 ? 0xe000 : 0x1fb0;
        other = dst & 0x2000 ? 0x1fb0 : 0xe000;
    } else if (mode == 1) {
        uint16_t swapped = (row >> 8) | (row << 8);
        destination = segment_bytes(0xb000);
        src = swapped - (swapped >> 2) - (swapped >> 3);
        width *= 2;
    } else if (mode == 2) {
        destination = segment_bytes(0xa000);
        src = dst;
        port_out16(0x3ce, 1);
        port_out16(0x3ce, 5);
        port_out16(0x3ce, 0xff08);
    } else {
        destination = segment_bytes(0xa000);
        src = dst >> 1;
        width *= 4;
    }
    SegmentBytes input = source + src;
    SegmentBytes output = destination + dst;
    do {
        if (mode == 2) {
            SegmentBytes saved = output;
            unsigned plane;
            for (plane = 0; plane < 4; ++plane) {
                port_out16(0x3c4, ((uint16_t)(1 << plane) << 8) | 2);
                input = source + video_buffer_offset(saved, destination) + plane * 8000;
                output = saved;
                copy_video_bytes(&output, &input, width, slice->backwards);
            }
            tail = 40 - width;
            output += tail;
        } else if (mode == 3) {
            uint16_t x = width;
            do {
                uint8_t packed = *input;
                input += slice->backwards ? -1 : 1;
                ((volatile struct UnalignedWord GAME_FAR *)output)->value =
                    (packed >> 4) | ((uint16_t)(packed & 15) << 8);
                output += slice->backwards ? -2 : 2;
            } while (--x);
            tail = 160 - width;
            input += tail;
            output += tail;
            output += tail;
        } else {
            copy_video_words(&output, &input, width, slice->backwards);
            tail = ((mode == 0 ? 40 : 80) - width) * 2;
            output += tail;
            input += tail;
            if (mode == 0) {
                uint16_t swap = bank;
                output += bank;
                bank = other;
                other = swap;
            } else {
                output += 0x1f60;
                wrapped = !!(video_buffer_offset(output, destination) & 0x8000);
                if (wrapped)
                    output += 0x80a0;
            }
        }
    } while (--remaining);
    if (end) {
        end->source_offset = video_buffer_offset(input, source);
        end->destination_offset = video_buffer_offset(output, destination);
        end->row_tail = tail;
        end->bank_step = bank;
        end->next_bank_step = other;
        end->word_width = width;
        end->wrapped = wrapped;
    }
    if (mode == 2)
        port_out16(0x3c4, 0x0f02);
}

void __far fill_background(uint16_t row_units,
    uint16_t columns,
    int backwards,
    struct BackgroundEnd *end,
    enum VideoPixelLayout mode)
{
    uint16_t dst = video_scanline8_offset;
    uint16_t rows = row_units * 8;
    uint16_t width = columns;
    uint16_t second = 0;
    uint16_t step = 0;
    uint16_t other = 0;
    SegmentBytes target = segment_bytes(video_target_base);
    unsigned style = scene_background_style;
    uint16_t pattern;
    if (mode == 0) {
        pattern = cga_background_pattern_a[style * 2].value;
        second = cga_background_pattern_b[style * 2].value;
        step = 0x2000 - width * 2;
        other = 0xe050 - width * 2;
        if (dst & 0x2000) {
            uint16_t swap = step;
            step = other;
            other = swap;
        }
    } else if (mode == 1) {
        pattern = tandy_background_patterns[style].value;
        width *= 2;
        step = 0x2000 - width * 2;
    } else if (mode == 2) {
        ega_polygon_busy = 255;
        target = segment_bytes(0xa000);
        dst += video_page_offset + video_target_base;
        port_out16(0x3ce, 0x0a05);
        port_out16(0x3ce, 0xff08);
        pattern = ega_background_patterns[style];
        step = 40 - width;
    } else {
        pattern = vga_background_patterns[style].value;
        width *= 4;
        step = 320 - width * 2;
    }
    do {
        uint16_t x = width;
        while (x--) {
            if (mode == 2)
                target[dst] = (uint8_t)pattern;
            else
                segment_word_store(target, dst, pattern);
            dst += (backwards ? -1 : 1) * (mode == 2 ? 1 : 2);
        }
        dst += step;
        if (mode == 0) {
            uint16_t swap = pattern;
            pattern = second;
            second = swap;
            swap = step;
            step = other;
            other = swap;
        } else if (mode == 1 && dst & 0x8000)
            dst += 0x80a0;
    } while (--rows);
    if (mode == 2)
        port_out16(0x3ce, 0x105);
    if (end) {
        end->destination_offset = dst;
        end->word_width = width;
        end->other_step = other;
        end->second_pattern = second;
    }
    if (mode == 2)
        ega_polygon_busy = 0;
}

void __far front_buffer(enum VideoPixelLayout mode)
{
    video_target_base = mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : mode == 2 ? 0 : 0xa000;
    video_page_offset = 0;
    port_out8(video_crtc_port, 12);
    port_out8(video_crtc_port + 1, 0);
    port_out8(video_crtc_port, 13);
    port_out8(video_crtc_port + 1, 0);
}

uint16_t __far c_work_segment(void)
{
    video_target_base = video_work_buffer_segment;
    return video_target_base;
}

void __far c_work_ega(void)
{
    video_target_base = 0x8000;
}

void __far status_tile(uint16_t tile,
    uint16_t column,
    SegmentBytes data,
    int backwards,
    struct StatusTileEnd *end,
    enum VideoPixelLayout mode)
{
    uint16_t pattern = 0;
    uint16_t step = 0;
    uint16_t other = 0;
    uint16_t last_pixel = 0;
    unsigned wrapped = 0;
    if (end)
        *end = (struct StatusTileEnd){0};
    /* There are ten tiles. Reverse traversal needs rows preceding its anchor. */
    if (tile >= 10 || (backwards && !tile && mode != 2))
        return;
    if (end)
        end->valid = 1;
    SegmentBytes target;
    uint16_t dst;
    uint16_t src;
    if (mode == 0) {
        dst = column * 2;
        src = (uint16_t)status_tiles_cga + tile * 16;
        target = segment_bytes(video_work_buffer_segment);
        step = 0x1ffe;
        other = 0xe04e;
    } else {
        uint16_t offset = ((tile >> 8) | (tile << 8)) >> 3;
        src = (uint16_t)(mode == 2 ? status_tiles_ega : status_tiles_packed) + offset;
        dst = mode == 2 ? column + video_page_offset + 0x8000 : column * (mode == 1 ? 4 : 8);
        target = segment_bytes(mode == 1 ? 0xb000 : mode == 2 ? 0xa000 : video_work_buffer_segment);
        if (mode == 2) {
            __asm__ volatile("cli" ::: "memory");
            port_out16(0x3ce, 1);
            port_out16(0x3ce, 5);
            port_out16(0x3ce, 0xff08);
        }
    }
    SegmentBytes input = data + src;
    SegmentBytes output = target + dst;
    unsigned y;
    for (y = 0; y < 8; ++y) {
        if (mode == 0 || mode == 1) {
            copy_video_words(&output, &input, mode == 0 ? 1 : 2, backwards);
            if (mode == 0) {
                uint16_t swap = step;
                output += step;
                step = other;
                other = swap;
            } else {
                output += 0x1ffc;
                wrapped = !!(video_buffer_offset(output, target) & 0x8000);
                if (wrapped)
                    output += 0x80a0;
            }
        } else if (mode == 2) {
            unsigned plane;
            for (plane = 0; plane < 4; ++plane) {
                port_out16(0x3c4, ((uint16_t)(1 << plane) << 8) | 2);
                last_pixel = *input++;
                *output = (uint8_t)last_pixel;
            }
            output += 40;
        } else {
            unsigned x;
            for (x = 0; x < 4; ++x) {
                uint8_t packed = *input;
                input += backwards ? -1 : 1;
                pattern = (packed >> 4) | ((uint16_t)(packed & 15) << 8);
                ((volatile struct UnalignedWord GAME_FAR *)output)->value = pattern;
                output += backwards ? -2 : 2;
            }
            output += 312;
        }
    }
    if (end) {
        end->source_offset = video_buffer_offset(input, data);
        end->destination_offset = video_buffer_offset(output, target);
        end->pattern = pattern;
        end->bank_step = step;
        end->next_bank_step = other;
        end->last_pixel = last_pixel;
        end->wrapped = wrapped;
    }
    if (mode == 2) {
        port_out16(0x3c4, 0x0f02);
        __asm__ volatile("sti" ::: "memory");
    }
}
