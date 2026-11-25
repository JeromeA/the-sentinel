/* Preserve the shipped Tandy filler's color-derived X coordinate and stack
 * consumption as well as its nibble masks. These are original backend quirks,
 * not corrections to the Tandy renderer during the C conversion. */

static __attribute__((noinline)) void tandy_span_edge(
    SegmentBytes target, uint16_t destination, uint8_t mask, uint8_t secondary, uint16_t pattern)
{
    target[destination] =
        (target[destination] & ~mask) | ((pattern >> 8) & mask) | ((uint8_t)pattern & secondary);
}

static void fill_tandy_span_row(SegmentBytes target,
    uint16_t left,
    uint16_t right,
    uint16_t destination,
    uint16_t pattern,
    int backwards,
    struct SpanFillEnd *trace)
{
    /* Clip the shipped color-derived left coordinate without replacing it. */
    if ((int16_t)(left - viewport_x_end) >= 0)
        return;
    int16_t x = left - viewport_x_min;
    int left_edge = x >= 0;
    unsigned left_index = (uint16_t)x & 1;
    if (x < 0)
        x = 0;
    int16_t end = right - viewport_x_min;
    if (trace)
        trace->span_count = end;
    if (end < 0)
        return;
    int right_edge;
    unsigned right_index = 0;
    if ((int16_t)(end - viewport_pixel_width) >= 0) {
        end = viewport_pixel_width;
        right_edge = 0;
    } else {
        right_edge = 1;
        right_index = (uint16_t)end & 1;
    }
    if (trace) {
        trace->edge_work = right_edge ? right_index : 0xffff;
        trace->valid |= 8;
    }
    destination += (uint16_t)x >> 1;
    uint16_t count = ((uint16_t)end >> 1) - ((uint16_t)x >> 1);
    if (trace)
        trace->span_count = count;
    /* Partial bytes use presence, even if a table mask contains zero. */
    if (!count && left_edge && right_edge) {
        uint8_t mask = tandy_edge_pixel_masks[left_index] & tandy_right_pixel_masks[right_index];
        tandy_span_edge(target, destination, mask, 0, pattern);
        return;
    }

    if (left_edge) {
        uint8_t mask = tandy_edge_pixel_masks[left_index];
        tandy_span_edge(target, destination, mask, tandy_left_secondary_masks[left_index], pattern);
        if (!count)
            return;
        ++destination;
        --count;
    }
    /* Complete interior bytes precede the final partial byte. */
    while (count) {
        target[destination] = (uint8_t)pattern;
        destination += backwards ? -1 : 1;
        --count;
    }
    if (trace)
        trace->span_count = 0;
    if (right_edge) {
        uint8_t mask = tandy_right_pixel_masks[right_index];
        tandy_span_edge(
            target, destination, mask, tandy_right_secondary_masks[right_index], pattern);
    }
}

uint8_t __far fill_tandy_polygon_spans(
    uint16_t data_segment, int backwards, struct TandySpanEnd *result)
{
    uint8_t data_output = 0;
    uint8_t high = polygon_y_max;
    uint8_t low = polygon_y_min;
    union MaterialColors colors = {.raw = polygon_packed_colors};
    uint8_t inside = colors.fields.second;
    uint8_t edge = inside;
    if (!(model_dither_flags & 128)) {
        colors.raw = polygon_packed_colors;
        edge = colors.fields.first;
    }
    uint16_t screen = (uint8_t)(247 - high);
    uint16_t rows = 0;
    uint16_t middle = ((unsigned)high + low) / 2;
    if (result) {
        result->fill.span_count = high;
        result->count_low_only = 1;
        result->fill.valid = 1;
    }
    uint16_t width = polygon_right_spans[middle] - polygon_left_spans[middle];
    if (result)
        result->value = width;
    if ((int16_t)width < 0)
        return data_output;
    raster_row_cursor = high;
    raster_color_index_a = inside;
    raster_color_index_b = edge;
    if (result) {
        result->fill.screen_row = screen;
        result->fill.valid |= 2;
        result->value = edge;
    }
    if (high < low)
        return data_output;
    if (result) {
        result->count_low_only = 0;
        result->fill.span_count = high;
    }
    if (result) {
        result->fill.span_offset = 2 * high;
        result->fill.valid |= 4;
    }
    int row;
    for (row = high; row >= low; --row) {
        uint16_t right = polygon_right_spans[row];
        uint16_t left = polygon_left_spans[row];
        uint16_t destination = video_scanline_offsets[screen] + 2 * raster_x_word_offset;
        uint16_t pattern = ((uint16_t)edge << 8) | inside;
        SegmentBytes target = segment_bytes(video_target_base);
        if (result) {
            result->fill.span_offset = 2 * row;
            result->fill.span_count = right;
            result->value = left;
        }
        if ((int16_t)(right - left) < 0)
            break;
        /* The executable overwrites its left endpoint with the packed colors. */
        left = pattern << 4;
        pattern |= left;
        if (result)
            ++rows;
        if (result)
            result->value = data_segment;
        data_output = 1;
        if (result)
            result->data_output = 1;
        fill_tandy_span_row(
            target, left, right, destination, pattern, backwards, result ? &result->fill : 0);
        if (result)
            result->value = (data_segment & 0xff00) | raster_row_cursor;
        if (row == low)
            break;
        ++screen;
        raster_row_cursor = row - 1;
    }
    if (result) {
        result->fill.screen_row = screen;
        result->fill.valid |= 2;
        result->rows = rows;
    }
    return data_output;
}

struct TandySpanEnd __far c_spans_tandy(uint16_t data_segment, int backwards)
{
    struct TandySpanEnd result = {0};
    fill_tandy_polygon_spans(data_segment, backwards, &result);
    return result;
}
