/* Publish a 192-line selected-view band from the working page. The CGA and
 * Tandy loops retain their respective 8 KiB bank-wrap rules. */
void __far publish_selection_band(uint16_t columns,
    uint16_t column,
    int backwards,
    enum VideoPixelLayout mode,
    struct GameStripCopy *observation)
{
    uint16_t source = video_scanline_offsets[8];
    uint16_t destination = source;
    uint16_t width = mode == 0 ? columns
        : mode == 1            ? columns * 2
        : mode == 2            ? columns
                               : columns * 4;
    SegmentBytes from = segment_bytes(video_work_buffer_segment);
    destination += column * (mode == 0 ? 2 : mode == 1 ? 4 : mode == 2 ? 1 : 8);
    if (mode != 1)
        destination += video_page_offset;
    SegmentBytes to = segment_bytes(mode == 0 ? 0xb800 : mode == 1 ? 0xb000 : 0xa000);
    if (mode == 2) {
        from = to;
        source = video_scanline_offsets[8] + video_page_offset + 0x8000;
        port_out16(0x3ce, 0x105);
    }
    unsigned row;
    if (mode == 0) {
        for (row = 0; row < 96; ++row) {
            uint16_t saved_source = source;
            uint16_t saved_destination = destination;
            unsigned x;
            for (x = 0; x < width; ++x) {
                destination &= 0x1fff;
                segment_word_store(to, destination, segment_word(from, source));
                source += backwards ? -2 : 2;
                destination += backwards ? -2 : 2;
            }
            source = saved_source | 0x2000;
            destination = saved_destination;
            for (x = 0; x < width; ++x) {
                destination = (destination & 0x1fff) | 0x2000;
                segment_word_store(to, destination, segment_word(from, source));
                source += backwards ? -2 : 2;
                destination += backwards ? -2 : 2;
            }
            destination += 80 - width * 2;
            source += 80 - width * 2;
            source ^= 0x2000;
        }
    } else {
        SegmentBytes input = from + source;
        SegmentBytes output = to + destination;
        for (row = 0; row < 192; ++row) {
            uint16_t tail = mode == 1 ? 0x2000 - width * 2
                : mode == 2           ? 40 - width
                                      : 320 - width * 2;
            if (mode == 2)
                copy_video_bytes(&output, &input, width, backwards);
            else
                copy_video_words(&output, &input, width, backwards);
            output += tail;
            input += tail;
            if (mode == 1 && (video_buffer_offset(output, to) & 0x8000)) {
                output += 0x80a0 - width * 2;
                input += 0x80a0 - width * 2;
            }
        }
        if (observation) {
            source = video_buffer_offset(input, from);
            destination = video_buffer_offset(output, to);
        }
    }
    if (observation) {
        observation->source_end = source;
        observation->destination_end = destination;
    }
}

struct GameStripCopy __far copy_selection_band(
    uint16_t columns, uint16_t column, int backwards, enum VideoPixelLayout mode)
{
    struct GameStripCopy end;
    publish_selection_band(columns, column, backwards, mode, &end);
    return end;
}
