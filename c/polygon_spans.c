/* Clipped scanline fills. CGA edges mix two packed dither patterns; EGA edges
 * use graphics-controller masks; VGA stores the two endpoint colors directly. */

static void cga_span_edge(SegmentBytes target,
    uint16_t destination,
    uint8_t mask,
    uint8_t second_mask,
    uint8_t primary,
    uint8_t secondary)
{
    uint8_t color = (primary & mask & ~second_mask) | (secondary & second_mask);
    target[destination] = (target[destination] & ~mask) | color;
}

static void ega_span_edge(SegmentBytes target, uint16_t destination, uint8_t mask, uint8_t color)
{
    port_out16(0x3ce, ((uint16_t)mask << 8) | 8);
    target[destination] &= color;
}

static void fill_cga_span_row(SegmentBytes target,
    uint16_t left,
    uint16_t right,
    uint16_t dst,
    int backwards,
    struct SpanFillEnd *trace)
{
    int16_t x;
    int16_t end;
    uint16_t count;

    dst += raster_x_word_offset;
    unsigned parity = (dst & 0x2000) ? 1 : 0;
    if (trace) {
        trace->valid |= 8;
        trace->edge_work = raster_color_index_a + parity;
    }
    uint8_t primary = cga_dither_patterns_a[raster_color_index_b + parity];
    uint8_t secondary = cga_dither_patterns_b[raster_color_index_a + parity];
    /* Clip against live viewport bounds before accessing the span. */
    if ((int16_t)(left - viewport_x_end) >= 0)
        return;
    x = (int16_t)(left - viewport_x_min);
    end = (int16_t)(right - viewport_x_min);
    if (x < 0)
        x = 0;
    else {
        if (trace) {
            trace->valid |= 8;
            trace->edge_work = x & 3;
        }
    }
    if (trace) {
        trace->valid |= 1;
        trace->span_count = (uint16_t)end;
    }
    if (end < 0)
        return;
    {
        uint8_t lm = (int16_t)(left - viewport_x_min) < 0 ? 0 : cga_left_pixel_masks[x & 3];
        uint8_t rm;
        if ((int16_t)(end - viewport_pixel_width) >= 0) {
            end = viewport_pixel_width;
            rm = 0;
        } else {
            if (trace) {
                trace->valid |= 8;
                trace->edge_work = end & 3;
            }
            rm = cga_right_pixel_masks[end & 3];
        }
        dst += (uint16_t)x >> 2;
        count = ((uint16_t)end >> 2) - ((uint16_t)x >> 2);
        if (trace) {
            trace->valid |= 1;
            trace->span_count = count;
        }
        /* Partial edges sharing one byte are combined first. */
        if (!count && lm && rm) {
            cga_span_edge(target, dst, lm & rm, (lm >> 2) & (uint8_t)(rm << 2), primary, secondary);
            return;
        }

        if (lm) {
            cga_span_edge(target, dst, lm, lm >> 2, primary, secondary);
            if (!count)
                return;
            ++dst;
            --count;
        }
        /* Fill the complete interior bytes, then the right edge. */
        while (count) {
            target[dst] = secondary;
            dst += backwards ? -1 : 1;
            --count;
        }
        if (trace) {
            trace->valid |= 1;
            trace->span_count = 0;
        }
        if (rm)
            cga_span_edge(target, dst, rm, (uint8_t)(rm << 2), primary, secondary);
    }
}

static void fill_ega_span_row(SegmentBytes target,
    uint16_t left,
    uint16_t right,
    uint16_t dst,
    int backwards,
    uint8_t inside,
    uint8_t edge,
    struct SpanFillEnd *trace)
{
    int16_t x;
    int16_t end;
    uint16_t count;

    dst += video_page_offset + video_target_base + (raster_x_word_offset >> 1);
    /* Clip against live viewport bounds before accessing the span. */
    if ((int16_t)(left - viewport_x_end) >= 0)
        return;
    x = (int16_t)(left - viewport_x_min);
    end = (int16_t)(right - viewport_x_min);
    int left_edge = x >= 0;
    unsigned left_index = (uint16_t)x & 7;
    if (x < 0)
        x = 0;
    if (trace) {
        trace->valid |= 8;
        trace->edge_work = (uint16_t)end;
    }
    if (end < 0)
        return;
    int right_edge;
    unsigned right_index = 0;
    if ((int16_t)(end - viewport_pixel_width) >= 0) {
        end = viewport_pixel_width;
        right_edge = 0;
    } else {
        right_edge = 1;
        right_index = (uint16_t)end & 7;
    }
    if (trace) {
        trace->valid |= 8;
        trace->edge_work = right_edge ? right_index : 0xffff;
    }
    dst += (uint16_t)x >> 3;
    count = ((uint16_t)end >> 3) - ((uint16_t)x >> 3);
    if (trace) {
        trace->valid |= 1;
        trace->span_count = count;
    }
    /* Partial edges sharing one byte are combined first. */
    if (!count && left_edge && right_edge) {
        ega_span_edge(
            target, dst, ega_span_mask_a[left_index] | ega_span_mask_a[right_index], edge);
        ega_span_edge(
            target, dst, ega_span_mask_b[left_index] & ega_span_mask_c[right_index], inside);
        return;
    }

    if (left_edge) {
        ega_span_edge(target, dst, ega_span_mask_a[left_index], edge);
        ega_span_edge(target, dst, ega_span_mask_b[left_index], inside);
        if (!count)
            return;
        ++dst;
        --count;
    }
    if (count) {
        port_out16(0x3ce, 0xff08);
        /* Fill the complete interior bytes, then the right edge. */
        while (count) {
            target[dst] = inside;
            dst += backwards ? -1 : 1;
            --count;
        }
    }
    if (trace) {
        trace->valid |= 1;
        trace->span_count = 0;
    }
    if (right_edge) {
        ega_span_edge(target, dst, ega_span_mask_a[right_index], edge);
        ega_span_edge(target, dst, ega_span_mask_c[right_index], inside);
    }
}

static void fill_vga_span_row(SegmentBytes target,
    uint16_t left,
    uint16_t right,
    uint16_t dst,
    int backwards,
    uint8_t inside,
    uint8_t edge,
    struct SpanFillEnd *trace)
{
    int16_t x;
    int16_t end;
    uint16_t count;

    uint8_t first = edge;
    dst += raster_x_word_offset * 4;
    /* Clip against live viewport bounds before accessing the span. */
    if ((int16_t)(left - viewport_x_end) >= 0)
        return;
    x = (int16_t)(left - viewport_x_min);
    end = (int16_t)(right - viewport_x_min);
    if (x < 0) {
        x = 0;
        first = inside;
    }
    if (trace) {
        trace->valid |= 8;
        trace->edge_work = (uint16_t)end;
    }
    if (end < 0)
        return;
    /* Endpoints are written before the interior, even at zero width. */
    target[dst + x] = first;
    ++x;
    if ((int16_t)(end - viewport_pixel_width) >= 0)
        end = viewport_pixel_width;
    else
        target[dst + end] = edge;
    if (trace) {
        trace->valid |= 8;
        trace->edge_work = end;
    }
    if (trace) {
        trace->valid |= 1;
        trace->span_count = (uint16_t)(end - x);
    }
    if (end > x) {
        dst += x;
        count = end - x;
        while (count--) {
            target[dst] = inside;
            dst += backwards ? -1 : 1;
        }
        if (trace) {
            trace->valid |= 1;
            trace->span_count = 0;
        }
    }
}

void __far fill_polygon_spans(int backwards, unsigned mode, struct SpanFillEnd *trace)
{
    uint8_t high = polygon_y_max;
    uint8_t low = polygon_y_min;
    union MaterialColors colors = {.raw = polygon_packed_colors};
    uint8_t inside = colors.fields.second;
    uint8_t edge = (model_dither_flags & 128) ? inside : colors.fields.first;
    unsigned middle = ((unsigned)high + low) / 2;
    uint16_t screen = (uint8_t)(247 - high);
    SegmentBytes target = segment_bytes(mode == 1 ? 0xa000 : video_target_base);
    if (trace) {
        trace->valid |= 1;
        trace->span_count = ((uint16_t)low << 8) | high;
    }
    if ((int16_t)(polygon_right_spans[middle] - polygon_left_spans[middle]) < 0) {
        if (mode == 1)
            ega_polygon_busy = 0;
        return;
    }
    raster_row_cursor = high;
    if (mode == 0) {
        raster_color_index_a = inside * 2;
        raster_color_index_b = edge * 2;
    } else if (mode == 1) {
        ega_polygon_busy = 255;
        port_out16(0x3ce, 0x0a05);
    } else {
        inside |= raster_shade_bank;
        edge |= raster_shade_bank;
    }
    if (trace) {
        trace->valid |= 2;
        trace->screen_row = screen;
    }
    if (high < low) {
        if (mode == 1)
            ega_polygon_busy = 0;
        return;
    }
    if (trace) {
        trace->valid |= 1;
        trace->span_count = high;
    }
    if (trace) {
        trace->valid |= 4;
        trace->span_offset = high * 2;
    }
    int row;
    for (row = high; row >= low; --row) {
        uint16_t left = polygon_left_spans[row];
        uint16_t right = polygon_right_spans[row];
        uint16_t dst = video_scanline_offsets[screen];
        if (trace) {
            trace->valid |= 4;
            trace->span_offset = row * 2;
        }
        if (mode == 0) {
            if (trace) {
                trace->valid |= 1;
                trace->span_count = right;
            }
        } else if (mode == 1) {
            if (trace) {
                trace->valid |= 8;
                trace->edge_work = right;
            }
            if (trace) {
                trace->valid |= 1;
                trace->span_count = left;
            }
        } else {
            if (trace) {
                trace->valid |= 8;
                trace->edge_work = right;
            }
        }
        if ((int16_t)(right - left) < 0)
            break;
        if (mode == 0)
            fill_cga_span_row(target, left, right, dst, backwards, trace);
        else if (mode == 1)
            fill_ega_span_row(target, left, right, dst, backwards, inside, edge, trace);
        else
            fill_vga_span_row(target, left, right, dst, backwards, inside, edge, trace);

        if (row == low)
            break;
        ++screen;
        raster_row_cursor = (uint8_t)(row - 1);
    }
    if (trace) {
        trace->valid |= 2;
        trace->screen_row = screen;
    }
    if (mode == 1)
        ega_polygon_busy = 0;
    return;
}

struct SpanFillEnd __far c_spans_cga(int backwards)
{
    struct SpanFillEnd end = {0};
    fill_polygon_spans(backwards, 0, &end);
    return end;
}

struct SpanFillEnd __far c_spans_ega(int backwards)
{
    struct SpanFillEnd end = {0};
    fill_polygon_spans(backwards, 1, &end);
    return end;
}

struct SpanFillEnd __far c_spans_vga(int backwards)
{
    struct SpanFillEnd end = {0};
    fill_polygon_spans(backwards, 2, &end);
    return end;
}

/* Horizontal raster offset in four-pixel units (CGA bytes): CGA adds it directly, Tandy doubles it, VGA quadruples it, and EGA halves it. The paired-view render loop selects zero or 40. */
uint16_t raster_x_word_offset = 0x0;

/* Raster workspace: first packed-color selector. CGA stores twice the low logical-color nibble as a word-table offset; Tandy stores the unscaled palette index. */
uint16_t raster_color_index_a = 0x0;

/* Raster workspace: second packed-color selector, normally the high color nibble; CGA scales it by two and Tandy leaves it unscaled. Model dithering can select the first color for both. */
uint16_t raster_color_index_b = 0x0;

/* VGA fog bank = (shade_input<<3)&F0h, so each pair of adjacent inputs shares a bank and input 32 wraps to bank zero. Mode 1 maps distances 16..78 monotonically onto banks 0..15. VGA span filling ORs the bank into the pixel high nibble; low-nibble object colors remain intact. */
union WorkWord raster_shade_bank_storage = {0};

/* CGA span filler: retain the selected 2-bit pixel and all pixels to its right in the first partial byte */
uint8_t cga_left_pixel_masks[4] = {
    0xff,
    0x3f,
    0x0f,
    0x03,
};

/* CGA span filler: retain pixels up to and including the selected pixel in the last partial byte */
uint8_t cga_right_pixel_masks[4] = {
    0xc0,
    0xf0,
    0xfc,
    0xff,
};

/* 16 logical colors, two scanline-parity patterns each; indexed by 2*color plus bank parity at 037Dh; every row is four 2-bit CGA pixels */
uint8_t cga_dither_patterns_a[32] = {
    0x00,
    0x00,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xff,
    0xff,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xaa,
    0xaa,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xaa,
    0xaa,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xaa,
    0xaa,
    0x00,
    0x00,
    0xaa,
    0xaa,
    0xaa,
    0xaa,
};

/* 16 logical colors, two scanline-parity patterns each; indexed by 2*color plus bank parity at 037Dh; every row is four 2-bit CGA pixels */
uint8_t cga_dither_patterns_b[32] = {
    0x00,
    0x00,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xff,
    0xff,
    0x11,
    0x44,
    0x55,
    0x55,
    0xaa,
    0xaa,
    0xbb,
    0xee,
    0x77,
    0xdd,
    0xff,
    0xff,
    0x66,
    0x66,
    0xff,
    0xff,
    0x55,
    0x55,
    0xff,
    0xff,
    0x66,
    0x99,
    0x22,
    0x88,
};

/* EGA span-edge masks: single boundary pixel, bit 7 leftmost; written to EGA GC bit-mask register 8 */
uint8_t ega_span_mask_a[8] = {
    0x80,
    0x40,
    0x20,
    0x10,
    0x08,
    0x04,
    0x02,
    0x01,
};

/* EGA span-edge masks: pixels strictly to the right of a boundary pixel */
uint8_t ega_span_mask_b[8] = {
    0x7f,
    0x3f,
    0x1f,
    0x0f,
    0x07,
    0x03,
    0x01,
    0x00,
};

/* EGA span-edge masks: pixels strictly to the left of a boundary pixel */
uint8_t ega_span_mask_c[8] = {
    0x00,
    0x80,
    0xc0,
    0xe0,
    0xf0,
    0xf8,
    0xfc,
    0xfe,
};

/* Tandy span filler, indexed by pixel parity 0/1: first-byte primary-color masks: selected nibble and pixels to its right */
uint8_t tandy_edge_pixel_masks[2] = {
    0xff,
    0x0f,
};

/* Tandy span filler, indexed by pixel parity 0/1: last-byte primary-color masks: selected nibble and pixels to its left */
uint8_t tandy_right_pixel_masks[2] = {
    0xf0,
    0xff,
};

/* Tandy span filler, indexed by pixel parity 0/1: first-byte secondary dither-color contributions */
uint8_t tandy_left_secondary_masks[2] = {
    0x0f,
    0x00,
};

/* Tandy span filler, indexed by pixel parity 0/1: last-byte secondary dither-color contributions */
uint8_t tandy_right_secondary_masks[2] = {
    0x00,
    0xf0,
};

/* EGA graphics-controller exclusion latch: span/background writers set FFh while changing GC state and clear it on completion; scattered-pixel and sentry-warning callbacks skip drawing while it is set. */
uint8_t ega_polygon_busy = 0x0;
