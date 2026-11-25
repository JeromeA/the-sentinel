/* The warning animation and selected-view dissolve have independent 32-bit
 * multiplicative sequences. Neither consumes landscape-generation randomness. */

uint8_t __far warning_bar(SegmentBytes data,
    int backwards,
    uint16_t idle_port,
    struct WarningEnd *end,
    enum VideoPixelLayout mode)
{
    uint16_t index = 0;
    uint8_t drawn = 0;
    uint16_t last_noise = 0;
    SegmentBytes screen = segment_bytes(mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : 0xa000);
    if (end) {
        end->completed = end->drawn = 0;
        end->skip_status = video_transition_busy_flags;
    }
    if (video_transition_busy_flags & 128)
        return 0;
    if (mode == 2) {
        if (end)
            end->skip_status = ega_polygon_busy;
        if (ega_polygon_busy & 128)
            return 0;
    }
    uint8_t pattern = sentry_warning_pattern;
    if ((backwards && (mode == 1 || mode == 3)) || (pattern != 0 && pattern != 4 && pattern != 8)) {
        if (end)
            end->skip_status = 128;
        return 0;
    }
    if (!pattern && pattern == previous_sentry_warning_pattern) {
        if (mode == 2)
            port_out16(idle_port, 0x105);
        goto done;
    }
    previous_sentry_warning_pattern = pattern;
    uint16_t initial_destination = mode == 0 ? ((0x203c + video_page_offset) & 0x1fff) | 0x2000
        : mode == 1                          ? 0x2078
        : mode == 2                          ? 0x46 + video_page_offset
                                             : 0x230 + video_page_offset;
    SegmentBytes destination = screen + initial_destination;
    if (mode == 2) {
        port_out16(0x3ce, 0x0a05);
        port_out16(0x3ce, 7);
    }
    unsigned block;
    unsigned half;
    unsigned row;
    for (block = 0; block < 4; ++block) {
        uint16_t noise = advance_transition_noise(&warning_bar_noise_low, &warning_bar_noise_high);
        for (row = 0; row < 4; ++row) {
            if (row)
                noise >>= 2;
            for (half = 0; half < 2; ++half) {
                if (half)
                    noise >>= 2;
                index = (noise & 3) | pattern;
                if (mode == 0) {
                    uint16_t value =
                        segment_word(data, (uint16_t)cga_sentry_warning_patterns + index * 2);
                    ((volatile struct UnalignedWord GAME_FAR *)destination)->value = value;
                    if (!half)
                        destination = screen +
                            (((video_buffer_offset(destination, screen) + 2) & 0x1fff) |
                                (video_buffer_offset(destination, screen) & 0x2000));
                    else if (video_buffer_offset(destination, screen) & 0x2000)
                        destination =
                            screen + ((video_buffer_offset(destination, screen) + 0x4e) & 0x1fff);
                    else
                        destination = screen +
                            (((video_buffer_offset(destination, screen) - 2) & 0x1fff) | 0x2000);
                } else if (mode == 2) {
                    uint8_t mask = ega_sentry_warning_patterns[index];
                    port_out16(0x3ce, ((uint16_t)mask << 8) | 8);
                    *destination = 0;
                    port_out16(0x3ce, ((uint16_t)(uint8_t)~mask << 8) | 8);
                    *destination &= 9;
                    destination += half ? 39 : 1;
                } else {
                    SegmentBytes source = data +
                        (uint16_t)(mode == 1 ? tandy_sentry_warning_patterns
                                             : vga_sentry_warning_patterns) +
                        index * (mode == 1 ? 4 : 8);
                    copy_video_words(&destination, &source, mode == 1 ? 2 : 4, backwards);
                    if (end)
                        end->source_offset = video_buffer_offset(source, data);
                }
            }
            if (mode == 1) {
                destination += 0x1ff8;
                if (video_buffer_offset(destination, screen) & 0x8000)
                    destination += 0x80a0;
            } else if (mode == 3)
                destination += 0x130;
        }
        if (end)
            last_noise = noise;
        if (mode == 0)
            destination =
                screen + (((video_buffer_offset(destination, screen) - 0x9c) & 0x1fff) | 0x2000);
        else if (mode == 1)
            destination += 0x1f60;
        else if (mode == 2)
            destination -= 0x9e;
        else
            destination -= 0x4f0;
        if (block == 1 && warning_bar_sight == 0x40)
            pattern = 0;
    }
    if (end) {
        end->destination_offset = video_buffer_offset(destination, screen);
        end->last_noise = last_noise;
        end->pattern_index = index;
        end->pattern = pattern;
    }
    drawn = 1;
    if (end)
        end->drawn = 1;
    if (mode == 2)
        port_out16(0x3ce, 0x105);
done:
    if (end)
        end->completed = 1;
    return 1 | (drawn ? 2 : 0);
}

uint8_t __far reveal_selection(int backwards, struct RevealEnd *end, enum VideoPixelLayout mode)
{
    uint8_t interrupts_enabled = 0;
    uint8_t width = selection_view_total_columns;
    uint8_t mask = 0;
    if (end)
        end->interrupts_enabled = 0;
    uint16_t src = 0;
    uint16_t dst = 0;
    uint16_t last_cx = 0;
    unsigned limit = mode == 2 ? 200 : 255;
    SegmentBytes source = segment_bytes(video_work_buffer_segment);
    SegmentBytes target = segment_bytes(mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : 0xa000);
    while (mask < (uint8_t)(width - 1))
        mask = mask * 2 + 1;
    if (mode == 2) {
        ega_selection_column_limit = width;
        ega_selection_column_mask = mask;
    }
    uint16_t high;
    for (;;) {
        selection_copy_inner_countdown = limit;
        do {
            uint16_t shifted_high = (copy_sequence_high << 2) | (copy_sequence_low >> 14);
            high = advance_transition_noise(&copy_sequence_low, &copy_sequence_high);
            uint8_t column = (uint8_t)((uint8_t)high + selection_copy_column_accumulator) & mask;
            if (column >= width)
                column -= width;
            selection_copy_column_accumulator = column;
            uint8_t y = ((high >> 8) >> 1) + ((high >> 8) & 63) + 8;
            uint16_t base = video_scanline_offsets[y];
            uint8_t variant = ((uint8_t)high >> 6) & 3;
            if (mode == 0) {
                uint16_t bitmask = cga_random_copy_nibble_masks[variant].value;
                src = base + column * 2;
                dst = (base + (uint16_t)(uint8_t)(column + selection_view_origin_column) * 2 +
                          video_page_offset) &
                    0x1fff;
                dst |= src & 0x2000;
                uint16_t value = segment_word(source, src) & bitmask;
                last_cx = segment_word(target, dst) & ~bitmask;
                segment_word_store(target, dst, value | last_cx);
                dst += backwards ? -2 : 2;
            } else if (mode == 2) {
                uint8_t bits = ega_random_copy_bit_masks[variant];
                dst = base + video_page_offset + (uint8_t)(column + selection_view_origin_column);
                src = base + video_page_offset + 0x8000 + column;
                __asm__ volatile("cli" ::: "memory");
                port_out16(0x3ce, 0x205);
                unsigned half;
                unsigned plane;
                for (half = 0; half < 2; ++half) {
                    uint8_t color = 0;
                    port_out16(0x3ce, ((uint16_t)bits << 8) | 8);
                    for (plane = 0; plane < 4; ++plane) {
                        port_out16(0x3ce, ((uint16_t)(3 - plane) << 8) | 4);
                        color = (color << 1) | !!(source[src] & bits);
                    }
                    uint8_t latch = target[dst];
                    (void)latch;
                    target[dst] = color;
                    if (!half)
                        bits >>= 1;
                }
                __asm__ volatile("sti" ::: "memory");
                interrupts_enabled = 1;
                if (end) {
                    end->interrupts_enabled = 1;
                    last_cx = ((uint16_t)bits << 8) | (shifted_high & 255);
                }
            } else {
                unsigned size = mode == 1 ? 4 : 8;
                unsigned step = mode == 1 ? 1 : 2;
                src = base + column * size + variant * step;
                last_cx = (uint16_t)(uint8_t)(column + selection_view_origin_column) * size +
                    variant * step;
                dst = base + last_cx + (mode == 3 ? video_page_offset : 0);
                if (step == 1) {
                    target[dst] = source[src];
                    src += backwards ? -1 : 1;
                    dst += backwards ? -1 : 1;
                } else {
                    segment_word_store(target, dst, segment_word(source, src));
                    src += backwards ? -2 : 2;
                    dst += backwards ? -2 : 2;
                }
            }
            if (selection_copy_interrupt_flags & 128)
                goto done;
        } while (--selection_copy_inner_countdown);
        if (!(--selection_copy_iteration_budget))
            break;
    }
done:
    if (end) {
        end->source_offset = src;
        end->destination_offset = dst;
        end->last_merge_operand = last_cx;
    }
    return interrupts_enabled;
}

/* Owned subsystem data. */
struct WarningBarNoiseLowStorage warning_bar_noise_low_storage = {.low = 0x1, .high = 0x0};
uint8_t cga_sentry_warning_patterns[24] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x50,
    0x00,
    0x05,
    0x50,
    0x00,
    0x05,
    0x00,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
};
uint8_t tandy_sentry_warning_patterns[48] = {
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
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x99,
    0x00,
    0x00,
    0x00,
    0x00,
    0x99,
    0x00,
    0x00,
    0x00,
    0x00,
    0x99,
    0x00,
    0x00,
    0x00,
    0x00,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
    0x99,
};
uint8_t ega_sentry_warning_patterns[12] = {
    0xff,
    0xff,
    0xff,
    0xff,
    0x3f,
    0xcf,
    0xf3,
    0xfc,
    0x00,
    0x00,
    0x00,
    0x00,
};
uint8_t vga_sentry_warning_patterns[96] = {
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
    0x00,
    0x00,
    0x09,
    0x09,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x09,
    0x09,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x09,
    0x09,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
};
struct CopySequenceLowStorage copy_sequence_low_storage = {.low = 0x4261, .high = 0x3987};
struct UnalignedWord cga_random_copy_nibble_masks[4] = {{0xf000}, {0x0f00}, {0x00f0}, {0x000f}};
uint8_t ega_random_copy_bit_masks[4] = {
    0x80,
    0x20,
    0x08,
    0x02,
};
