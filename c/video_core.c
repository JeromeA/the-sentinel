/* Scanline tables and viewport selection, implemented without hardware-specific
 * instruction sequences. The planar-table entry's final GC write is a port I/O
 * boundary; the table construction itself is ordinary C. */

void __far build_video_rows(SegmentBytes target, int backwards, enum VideoRowLayout layout)
{
    unsigned count = layout < ROWS_PLANAR ? 200 : 201;
    uint16_t destination = (uint16_t)video_scanline_offsets;
    unsigned row;
    for (row = 0; row < count; ++row) {
        segment_word_store(target, destination, video_row_offset(row, layout));
        destination += backwards ? -2 : 2;
    }
    if (layout == ROWS_PLANAR)
        port_out16(0x3ce, 7);
}

void __far c_viewport_preset(uint16_t preset)
{
    if (preset >= 3)
        return;
    viewport_preset = (uint8_t)preset;
    viewport_projected_x_limit = viewport_x_limit_table[preset];
    viewport_negative_x_threshold = (viewport_projected_x_limit >> 1) ^ 0x80;
    projection_x_bias = (uint16_t)viewport_origin_high_table[preset] << 8;
    viewport_pixel_width = viewport_width_table[preset].value;
    viewport_x_min = viewport_x_min_table[preset].value;
    viewport_x_end = viewport_x_min + viewport_pixel_width;
    viewport_x_fraction_limit = 0;
}

void __far c_viewport_width(uint8_t columns)
{
    viewport_projected_x_limit = columns >> 1;
    viewport_x_fraction_limit = (columns & 1) ? 0x80 : 0;
    viewport_negative_x_threshold = (viewport_projected_x_limit >> 1) ^ 0x80;
    /* Preserve the byte-width shift used by the original routine. */
    viewport_pixel_width = (uint8_t)(columns << 3);
    viewport_x_end = viewport_pixel_width;
    viewport_x_min = 0;
    projection_x_bias = 0;
    viewport_preset = 2;
}

/* Owned subsystem data. */
uint16_t video_crtc_port = 0x0;
uint8_t viewport_origin_high_table[4] = {
    0x00,
    0x02,
    0x00,
    0x00,
};
struct UnalignedWord viewport_x_min_table[3] = {{0x0000}, {0x0040}, {0x0000}};
struct UnalignedWord viewport_width_table[3] = {{0x0140}, {0x0070}, {0x0080}};
uint8_t viewport_x_limit_table[4] = {
    0x14,
    0x14,
    0x08,
    0x00,
};

/* ViewportWork is reset independently of other subsystems and data placement. */
struct ViewportWork viewport_work = {0};

void __far reset_viewport_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&viewport_work), 0, sizeof(viewport_work), 0);
}

/* 201 scanline-address words. Native builders use CGA=(y&1)*8192+(y>>1)*80, Tandy=(y&3)*8192+(y>>2)*160, EGA=y*40, VGA=y*320. CS:0A8B/0AA0 write only y=0..199; CS:0AB9/0AD1 include y=200. The extra word is not a 201st visible row. Consumers index by 2*y and add their backend-specific X byte offset. */
uint16_t video_scanline_offsets[201] = {0};

/* Draw-target tagged union selected by the installed backend: CGA/Tandy/VGA hold a segment; EGA holds 0000h/8000h byte offsets within A000h. Front-buffer selectors CS:0C00..0C64 also clear video_page_offset and program CRTC start zero. Work selectors CS:0C86/0C8D change only the drawing target, preserving the currently displayed page origin. */
uint16_t video_target_base = 0x0;

/* Video transition exclusion byte. CGA/Tandy/VGA transition paths write FFh while changing display/page state and zero on completion of that guarded section. All four sentry warning-bar backends skip their drawing while the sign bit is set. */
volatile uint8_t video_transition_busy_flags = 0x0;
