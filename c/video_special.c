/* Expanded input-panel slices, background occupancy checks, and the CGA
 * software-scroll fallback used when hardware CRTC scrolling is unavailable. */
struct InputPanelEnd __far c_expand_input_panel(int backwards)
{
    struct InputPanelEnd end = {0};
    SegmentBytes source = segment_bytes(skybox_image_segment);
    uint16_t initial_destination = video_scanline_offsets[178];
    SegmentBytes output;
    uint16_t last = 0;
    unsigned pass;
    SegmentBytes target;
    uint16_t src;
    unsigned row;
    if (graphics_adapter == 2 || graphics_adapter == 3) {
        unsigned mode = graphics_adapter == 2 ? 0 : 1;
        unsigned width = mode ? 28 : 14;
        uint16_t source_origin = mode ? 0x6f50 : 0x37a8;
        int16_t copy_step = backwards ? -(int16_t)(width * 2) : width * 2;
        int16_t slice_pitch = copy_step - (mode ? 32 : 16);
        int16_t row_pitch = copy_step * 2 + (mode ? 48 : 24);
        target = segment_bytes(mode ? 0xb000 : 0xb800);
        output = target + initial_destination;
        for (row = 0; row < 19; ++row) {
            for (pass = 0; pass < 2; ++pass) {
                SegmentBytes input = source + source_origin + row * row_pitch + pass * slice_pitch;
                copy_video_words(&output, &input, width, backwards);
            }
            last = mode ? (video_buffer_offset(output, target) & 0x8000 ? 0x8030 : 0x1f90)
                        : (video_buffer_offset(output, target) & 0x2000 ? 0xe018 : 0x1fc8);
            end.carry = (uint32_t)video_buffer_offset(output, target) + last > 65535;
            output += last;
        }
        src = source_origin + 19 * row_pitch;
    } else if (graphics_adapter == 5) {
        uint16_t source_origin = 0x6f50;
        int16_t copy_step = backwards ? -56 : 56;
        int16_t slice_pitch = copy_step - 32;
        int16_t row_pitch = copy_step * 2 + 48;
        target = segment_bytes(0xa000);
        output = target + initial_destination;
        end.vga = 1;
        for (row = 0; row < 19; ++row) {
            for (pass = 0; pass < 2; ++pass) {
                SegmentBytes input = source + source_origin + row * row_pitch + pass * slice_pitch;
                unsigned x;
                for (x = 0; x < 56; ++x) {
                    uint8_t packed = *input;
                    input += backwards ? -1 : 1;
                    ((volatile struct UnalignedWord GAME_FAR *)output)->value =
                        (packed >> 4) | ((uint16_t)(packed & 15) << 8);
                    output += backwards ? -2 : 2;
                }
            }
            end.carry = (uint32_t)video_buffer_offset(output, target) + 96 > 65535;
            output += 96;
        }
        src = source_origin + 19 * row_pitch;
    } else {
        uint16_t source_origin = 0x1bd4;
        /* Eight directed 14-byte copies contribute to each source row.
         * Reversing a copy moves its end 28 bytes behind its forward end. */
        int16_t reverse_adjustment = backwards ? -28 : 0;
        int16_t plane_pitch = 8000 + reverse_adjustment;
        int16_t slice_pitch = 6 + 4 * reverse_adjustment;
        int16_t row_pitch = 40 + 8 * reverse_adjustment;
        target = segment_bytes(0xa000);
        output = target + initial_destination;
        port_out16(0x3ce, 1);
        port_out16(0x3ce, 5);
        port_out16(0x3ce, 0xff08);
        for (row = 0; row < 19; ++row) {
            for (pass = 0; pass < 2; ++pass) {
                SegmentBytes saved = output;
                unsigned plane;
                for (plane = 0; plane < 4; ++plane) {
                    port_out16(0x3c4, ((uint16_t)(1 << plane) << 8) | 2);
                    output = saved;
                    SegmentBytes input = source + source_origin + row * row_pitch +
                        pass * slice_pitch + plane * plane_pitch;
                    copy_video_bytes(&output, &input, 14, backwards);
                }
            }
            end.carry = (uint32_t)video_buffer_offset(output, target) + 12 > 65535;
            output += 12;
        }
        port_out16(0x3c4, 0x0f02);
        src = source_origin + 19 * row_pitch;
        end.planar = 1;
    }
    end.source_end = src;
    end.destination_end = video_buffer_offset(output, target);
    return end;
}

struct BackgroundProbeEnd __far c_background_is_occupied(SegmentBytes data)
{
    struct BackgroundProbeEnd end = {0};
    SegmentBytes target = segment_bytes(graphics_adapter == 2 ? 0xb800
            : graphics_adapter == 3                           ? 0xb000
                                                              : 0xa000);
    uint16_t alternate;
    uint16_t pattern;
    if (graphics_adapter == 2) {
        pattern = cga_background_pattern_a[0].value;
        alternate = segment_word(data, (uint16_t)&cga_background_pattern_a + 2);
    } else if (graphics_adapter == 3)
        pattern = alternate = tandy_background_patterns[0].value;
    else if (graphics_adapter == 5)
        pattern = alternate = vga_background_patterns[0].value;
    else {
        end.planar = 1;
        ega_polygon_busy = 255;
        port_out16(0x3ce, 0x0f07);
        port_out16(0x3ce, ((uint16_t)ega_background_patterns[0] << 8) | 2);
        port_out16(0x3ce, 0x0805);
        pattern = alternate = 255;
    }
    end.row = 48;
    unsigned row;
    for (row = 48; row < 148; ++row) {
        uint16_t destination = video_scanline_offsets[row] + video_page_offset;
        end.row = row;
        if (graphics_adapter == 2)
            destination = (destination & 0x1fff) | (row & 1 ? 0x2000 : 0);
        end.destination = destination;
        uint16_t value = end.planar ? target[destination] : segment_word(target, destination);
        end.comparison_left = value;
        end.comparison_right = pattern;
        if (value != pattern) {
            end.occupied = 1;
            break;
        }
        {
            uint16_t swap = pattern;
            pattern = alternate;
            alternate = swap;
        }
        end.row = row + 1;
        end.comparison_left = end.row;
        end.comparison_right = 148;
    }
    if (end.planar) {
        port_out16(0x3ce, 7);
        ega_polygon_busy = 0;
    }
    end.alternate = alternate;
    return end;
}

void __far scroll_cga_software(SegmentBytes data, uint16_t flags, struct SoftwareScrollEnd *end)
{
    SegmentBytes screen = segment_bytes(0xb800);
    SegmentBytes work = segment_bytes(video_work_buffer_segment);
    SegmentBytes spare = segment_bytes(segment_record(data, graphics_session).scroll_segment);
    SegmentBytes source = spare;
    SegmentBytes destination = screen;
    for (;;) {
        uint8_t step = --video_transition_steps_left;
        if (step & 128)
            break;
        unsigned row;
        if (camera_transition_mode < INPUT_DIRECTION_FIRST_VERTICAL) {
            uint16_t before = camera_transition_mode == INPUT_DIRECTION_RIGHT ? 0x144 : 0x140;
            uint16_t after = camera_transition_mode == INPUT_DIRECTION_RIGHT ? 0x140 : 0x144;
            --video_transition_steps_left;
            uint16_t bank;
            for (bank = 0; bank < 2; ++bank) {
                SegmentBytes input = screen + before + bank * 0x2000;
                SegmentBytes output = spare + after + bank * 0x2000;
                copy_video_words(&output, &input, 0xefe, !!(flags & 0x400));
            }
            uint16_t source_offset = segment_word(data,
                (camera_transition_mode == INPUT_DIRECTION_RIGHT
                        ? ((uint16_t)&cga_transition_source_mode_0)
                        : ((uint16_t)&cga_transition_source_mode_1)) +
                    step * 2);
            uint16_t destination_offset =
                camera_transition_mode == INPUT_DIRECTION_RIGHT ? 0x18e : 0x140;
            SegmentBytes input = work + source_offset;
            SegmentBytes output = spare + destination_offset;
            for (row = 0; row < 192; ++row) {
                copy_video_words(&output, &input, 2, !!(flags & 0x400));
                input += row & 1 ? 0xe04c : 0x1ffc;
                output += row & 1 ? 0xe04c : 0x1ffc;
            }
        }
        wait_for_video_retrace(end ? &end->retrace : 0);
        if (end)
            flags = status(flags, 0, 8);
        source = spare + 0x140;
        destination = screen + 0x140;
        for (row = 0; row < 192; ++row) {
            copy_video_words(&destination, &source, 40, !!(flags & 0x400));
            source += row & 1 ? 0xe000 : 0x1fb0;
            destination += row & 1 ? 0xe000 : 0x1fb0;
        }
    }
    if (end) {
        end->source = video_buffer_offset(source, spare);
        end->destination = video_buffer_offset(destination, screen);
        end->flags = status(flags, 0, 16);
    }
}

struct SoftwareScrollEnd __far c_cga_scroll_software(SegmentBytes data, uint16_t flags)
{
    struct SoftwareScrollEnd end = {0};
    scroll_cga_software(data, flags, &end);
    return end;
}

/* Scene background patterns selected by scene_background_style: four styles of two alternating scanline words; pattern B is the +2 alias in each four-byte record */
struct UnalignedWord cga_background_pattern_a[8] = {
    {0xeeee},
    {0xbbbb},
    {0xeeee},
    {0xbbbb},
    {0x0000},
    {0x0000},
    {0x0000},
    {0x0000},
};

/* Scene background patterns selected by scene_background_style: four styles of packed 4-bit pixel words */
struct UnalignedWord tandy_background_patterns[4] = {
    {0x7777},
    {0x7777},
    {0x0000},
    {0x0000},
};

/* Scene background patterns selected by scene_background_style: four styles of two linear 8-bit pixel indices */
struct UnalignedWord vga_background_patterns[4] = {
    {0x0707},
    {0x0707},
    {0x0000},
    {0x0000},
};

/* Scene background patterns selected by scene_background_style: four styles of planar fill-color selectors */
uint8_t ega_background_patterns[5] = {
    0x07,
    0x07,
    0x00,
    0x00,
    0x00,
};
