/* Typed video callbacks. No register frames or entry descriptors are used. */
void video_background_cga(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result)
{
    fill_background(row, columns, backwards, result, PIXEL_CGA);
}

void video_background_ega(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result)
{
    fill_background(row, columns, backwards, result, PIXEL_EGA);
}

void video_background_tandy(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result)
{
    fill_background(row, columns, backwards, result, PIXEL_TANDY);
}

void video_background_vga(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result)
{
    fill_background(row, columns, backwards, result, PIXEL_VGA);
}

void video_blit_cga(const struct ImageSlice *slice, struct ImageCopyEnd *result)
{
    blit_image(slice, result, PIXEL_CGA);
}

void video_blit_ega(const struct ImageSlice *slice, struct ImageCopyEnd *result)
{
    blit_image(slice, result, PIXEL_EGA);
}

void video_blit_tandy(const struct ImageSlice *slice, struct ImageCopyEnd *result)
{
    blit_image(slice, result, PIXEL_TANDY);
}

void video_blit_vga(const struct ImageSlice *slice, struct ImageCopyEnd *result)
{
    blit_image(slice, result, PIXEL_VGA);
}

struct VideoTransitionReply video_cga_scroll_software(
    SegmentBytes data, int backwards, uint16_t flags)
{
    struct VideoTransitionReply end = {0};
    end.kind = VIDEO_TRANSITION_SOFTWARE;
    end.scroll = c_cga_scroll_software(data, flags);
    return end;
}

void video_colors_cga(uint16_t variant, uint16_t *observation)
{
    c_apply_landscape_colors(variant, PIXEL_CGA, observation);
}

void video_colors_ega(uint16_t variant, uint16_t *observation)
{
    c_apply_landscape_colors(variant, PIXEL_EGA, observation);
}

void video_colors_tandy(uint16_t variant, uint16_t *observation)
{
    c_apply_landscape_colors(variant, PIXEL_TANDY, observation);
}

void video_colors_vga(uint16_t variant, uint16_t *observation)
{
    c_apply_landscape_colors(variant, PIXEL_VGA, observation);
}

struct ResourceDecodeEnd video_decode_cga(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags)
{
    return c_decode_cga(handle, destination, data, flags);
}

struct ResourceDecodeEnd video_decode_ega(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags)
{
    return c_decode_ega(handle, destination, data, flags);
}

struct ResourceDecodeEnd video_decode_packed(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags)
{
    return c_decode_packed(handle, destination, data, flags);
}

void video_draw_cursor_cga(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result)
{
    draw_cursor(data, bank, result, PIXEL_CGA);
}

void video_draw_cursor_ega(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result)
{
    draw_cursor(data, bank, result, PIXEL_EGA);
}

void video_draw_cursor_tandy(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result)
{
    draw_cursor(data, bank, result, PIXEL_TANDY);
}

void video_draw_cursor_vga(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result)
{
    draw_cursor(data, bank, result, PIXEL_VGA);
}

void video_font_cga(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result)
{
    font_glyph(data, request, backwards, result, GLYPH_CGA);
}

void video_font_ega(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result)
{
    font_glyph(data, request, backwards, result, GLYPH_EGA);
}

void video_font_packed(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result)
{
    font_glyph(data, request, backwards, result, GLYPH_PACKED);
}

void video_font_vga(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result)
{
    font_glyph(data, request, backwards, result, GLYPH_VGA);
}

void video_front_cga(void)
{
    front_buffer(PIXEL_CGA);
}

void video_front_ega(void)
{
    front_buffer(PIXEL_EGA);
}

void video_front_tandy(void)
{
    front_buffer(PIXEL_TANDY);
}

void video_front_vga(void)
{
    front_buffer(PIXEL_VGA);
}

struct PaletteEnd video_palette_cga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return c_palette_cga((uint32_t)data >> 16, phase, flags);
}

struct PaletteEnd video_palette_ega(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return c_palette_ega(output, phase, flags);
}

struct PaletteEnd video_palette_tandy(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return c_palette_tandy(output, phase, flags);
}

struct PaletteEnd video_palette_vga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return c_palette_vga(data, output, phase, flags);
}

struct PaletteEnd video_palette_vga_ega(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return c_palette_vga_ega(output, phase, flags);
}

void video_restore_cursor_cga(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result)
{
    restore_cursor(data, bank, result, PIXEL_CGA);
}

void video_restore_cursor_ega(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result)
{
    restore_cursor(data, bank, result, PIXEL_EGA);
}

void video_restore_cursor_tandy(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result)
{
    restore_cursor(data, bank, result, PIXEL_TANDY);
}

void video_restore_cursor_vga(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result)
{
    restore_cursor(data, bank, result, PIXEL_VGA);
}

void video_reveal_cga(int backwards, struct RevealEnd *result)
{
    reveal_selection(backwards, result, PIXEL_CGA);
}

void video_reveal_ega(int backwards, struct RevealEnd *result)
{
    reveal_selection(backwards, result, PIXEL_EGA);
}

void video_reveal_tandy(int backwards, struct RevealEnd *result)
{
    reveal_selection(backwards, result, PIXEL_TANDY);
}

void video_reveal_vga(int backwards, struct RevealEnd *result)
{
    reveal_selection(backwards, result, PIXEL_VGA);
}

void video_scatter_cga(uint16_t count, uint8_t color, struct ScatterEnd *result)
{
    scatter_pixels(count, color, result, PIXEL_CGA);
}

void video_scatter_ega(uint16_t count, uint8_t color, struct ScatterEnd *result)
{
    scatter_pixels(count, color, result, PIXEL_EGA);
}

void video_scatter_tandy(uint16_t count, uint8_t color, struct ScatterEnd *result)
{
    scatter_pixels(count, color, result, PIXEL_TANDY);
}

void video_scatter_vga(uint16_t count, uint8_t color, struct ScatterEnd *result)
{
    scatter_pixels(count, color, result, PIXEL_VGA);
}

struct GameStripCopy video_selection_copy_cga(uint16_t columns, uint16_t origin, int backwards)
{
    return copy_selection_band(columns, origin, backwards, PIXEL_CGA);
}

struct GameStripCopy video_selection_copy_ega(uint16_t columns, uint16_t origin, int backwards)
{
    return copy_selection_band(columns, origin, backwards, PIXEL_EGA);
}

struct GameStripCopy video_selection_copy_tandy(uint16_t columns, uint16_t origin, int backwards)
{
    return copy_selection_band(columns, origin, backwards, PIXEL_TANDY);
}

struct GameStripCopy video_selection_copy_vga(uint16_t columns, uint16_t origin, int backwards)
{
    return copy_selection_band(columns, origin, backwards, PIXEL_VGA);
}

struct TandySpanEnd video_spans_cga(SegmentBytes data, int backwards)
{
    struct TandySpanEnd end = {0};
    end.fill = c_spans_cga(backwards);
    end.value = (uint32_t)data >> 16;
    end.data_output = 1;
    return end;
}

struct TandySpanEnd video_spans_ega(SegmentBytes data, int backwards)
{
    struct TandySpanEnd end = {0};
    end.fill = c_spans_ega(backwards);
    end.value = (uint32_t)data >> 16;
    end.data_output = 1;
    return end;
}

struct TandySpanEnd video_spans_tandy(SegmentBytes data, int backwards)
{
    return c_spans_tandy((uint32_t)data >> 16, backwards);
}

struct TandySpanEnd video_spans_vga(SegmentBytes data, int backwards)
{
    struct TandySpanEnd end = {0};
    end.fill = c_spans_vga(backwards);
    end.value = (uint32_t)data >> 16;
    end.data_output = 1;
    return end;
}

void video_tile_cga(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result)
{
    status_tile(tile, column, data, backwards, result, PIXEL_CGA);
}

void video_tile_ega(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result)
{
    status_tile(tile, column, data, backwards, result, PIXEL_EGA);
}

void video_tile_tandy(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result)
{
    status_tile(tile, column, data, backwards, result, PIXEL_TANDY);
}

void video_tile_vga(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result)
{
    status_tile(tile, column, data, backwards, result, PIXEL_VGA);
}

struct VideoTransitionReply video_transition_cga(SegmentBytes data, int backwards, uint16_t flags)
{
    struct VideoTransitionReply end = {0};
    end.view = c_transition_cga(data, backwards);
    return end;
}

struct VideoTransitionReply video_transition_ega(SegmentBytes data, int backwards, uint16_t flags)
{
    struct VideoTransitionReply end = {0};
    end.view = c_transition_ega(data, backwards);
    return end;
}

struct VideoTransitionReply video_transition_tandy(SegmentBytes data, int backwards, uint16_t flags)
{
    struct VideoTransitionReply end = {0};
    end.kind = VIDEO_TRANSITION_PAGE;
    end.previous = c_transition_tandy();
    return end;
}

struct VideoTransitionReply video_transition_vga(SegmentBytes data, int backwards, uint16_t flags)
{
    struct VideoTransitionReply end = {0};
    end.view = c_transition_vga(data, backwards);
    return end;
}

void video_warning_cga(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result)
{
    warning_bar(data, backwards, idle_port, result, PIXEL_CGA);
}

void video_warning_ega(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result)
{
    warning_bar(data, backwards, idle_port, result, PIXEL_EGA);
}

void video_warning_tandy(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result)
{
    warning_bar(data, backwards, idle_port, result, PIXEL_TANDY);
}

void video_warning_vga(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result)
{
    warning_bar(data, backwards, idle_port, result, PIXEL_VGA);
}

uint16_t video_work_ega(void)
{
    c_work_ega();
    return 0xa000;
}

uint16_t video_work_segment(void)
{
    return c_work_segment();
}

struct PaletteEnd __far call_video_palette(PaletteCallback callback,
    SegmentBytes data,
    SegmentBytes output,
    uint16_t phase,
    uint16_t flags)
{
    return callback(data, output, phase, flags);
}

void __far call_video_colors(ColorsCallback callback, uint16_t variant, uint16_t *observation)
{
    callback(variant, observation);
}

struct ResourceDecodeEnd __far call_video_decode(DecodeCallback callback,
    uint16_t handle,
    SegmentBytes destination,
    SegmentBytes data,
    uint16_t flags)
{
    return callback(handle, destination, data, flags);
}

void __far call_video_blit(
    BlitCallback callback, const struct ImageSlice *slice, struct ImageCopyEnd *result)
{
    /* Only the canonical kernels promise nullable observation storage. */
    if (result || callback == video_blit_cga || callback == video_blit_tandy ||
        callback == video_blit_ega || callback == video_blit_vga) {
        callback(slice, result);
        return;
    }
    struct ImageCopyEnd opaque;
    callback(slice, &opaque);
}

void __far call_video_background(BackgroundCallback callback,
    uint16_t row,
    uint16_t columns,
    int backwards,
    struct BackgroundEnd *result)
{
    /* Only the canonical kernels promise nullable observation storage. */
    if (result || callback == video_background_cga || callback == video_background_tandy ||
        callback == video_background_ega || callback == video_background_vga) {
        callback(row, columns, backwards, result);
        return;
    }
    struct BackgroundEnd opaque;
    callback(row, columns, backwards, &opaque);
}

void __far call_video_tile(TileCallback callback,
    uint16_t tile,
    uint16_t column,
    SegmentBytes data,
    int backwards,
    struct StatusTileEnd *result)
{
    /* Only the canonical kernels promise nullable observation storage. */
    if (result || callback == video_tile_cga || callback == video_tile_tandy ||
        callback == video_tile_ega || callback == video_tile_vga) {
        callback(tile, column, data, backwards, result);
        return;
    }
    struct StatusTileEnd opaque;
    callback(tile, column, data, backwards, &opaque);
}

void __far call_video_scatter(
    ScatterCallback callback, uint16_t count, uint8_t color, struct ScatterEnd *result)
{
    callback(count, color, result);
}

/* Opaque callbacks retain their original, nonnullable by-value result ABI.
 * Canonical callbacks share the same pixel kernels with the raw entry gates. */
uint8_t __far draw_video_spans(SpansCallback callback, SegmentBytes data, int backwards)
{
    if (callback == video_spans_cga)
        fill_polygon_spans(backwards, 0, 0);
    else if (callback == video_spans_ega)
        fill_polygon_spans(backwards, 1, 0);
    else if (callback == video_spans_vga)
        fill_polygon_spans(backwards, 2, 0);
    else if (callback == video_spans_tandy)
        return fill_tandy_polygon_spans(0, backwards, 0);
    else {
        struct TandySpanEnd opaque = callback(data, backwards);
        return opaque.data_output;
    }
    return 1;
}

struct TandySpanEnd __far call_video_spans(SpansCallback callback, SegmentBytes data, int backwards)
{
    return callback(data, backwards);
}

void __far call_video_glyph(GlyphCallback callback,
    SegmentBytes data,
    const struct GlyphRequest *request,
    int backwards,
    struct GlyphEnd *result)
{
    /* Only the canonical kernels promise nullable observation storage. */
    if (result || callback == video_font_cga || callback == video_font_packed ||
        callback == video_font_vga || callback == video_font_ega) {
        callback(data, request, backwards, result);
        return;
    }
    struct GlyphEnd opaque;
    callback(data, request, backwards, &opaque);
}

void __far call_video_front(FrontCallback callback)
{
    callback();
}

uint16_t __far call_video_work(WorkCallback callback)
{
    return callback();
}

void __far call_video_warning(WarningCallback callback,
    SegmentBytes data,
    int backwards,
    uint16_t idle_port,
    struct WarningEnd *result)
{
    callback(data, backwards, idle_port, result);
}

void __far call_video_reveal(RevealCallback callback, int backwards, struct RevealEnd *result)
{
    callback(backwards, result);
}

void __far call_video_cursor_draw(
    CursorDrawCallback callback, SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result)
{
    callback(data, bank, result);
}

void __far call_video_cursor_restore(CursorRestoreCallback callback,
    SegmentBytes data,
    uint16_t bank,
    struct CursorRestoreEnd *result)
{
    callback(data, bank, result);
}

struct GameStripCopy __far call_video_selection_copy(
    SelectionCopyCallback callback, uint16_t columns, uint16_t origin, int backwards)
{
    return callback(columns, origin, backwards);
}

struct VideoTransitionReply __far call_video_transition(
    TransitionCallback callback, SegmentBytes data, int backwards, uint16_t flags)
{
    return callback(data, backwards, flags);
}

/* Canonical callbacks share nullable observation kernels. Opaque callbacks
 * retain valid storage and their original calling convention. */
uint8_t __far dissolve_selection(RevealCallback binding, int backwards)
{
    unsigned mode = binding == video_reveal_cga ? 0
        : binding == video_reveal_tandy         ? 1
        : binding == video_reveal_ega           ? 2
                                                : 3;
    if (binding == video_reveal_cga || binding == video_reveal_tandy ||
        binding == video_reveal_ega || binding == video_reveal_vga)
        return reveal_selection(backwards, 0, mode);
    struct RevealEnd opaque;
    binding(backwards, &opaque);
    return opaque.interrupts_enabled;
}

void __far present_selection_band(
    SelectionCopyCallback binding, uint16_t columns, uint16_t origin, int backwards)
{
    unsigned mode = binding == video_selection_copy_cga ? 0
        : binding == video_selection_copy_tandy         ? 1
        : binding == video_selection_copy_ega           ? 2
                                                        : 3;
    if (binding == video_selection_copy_cga || binding == video_selection_copy_tandy ||
        binding == video_selection_copy_ega || binding == video_selection_copy_vga)
        publish_selection_band(columns, origin, backwards, mode, 0);
    else
        binding(columns, origin, backwards);
}

uint8_t __far present_view_transition(
    TransitionCallback binding, SegmentBytes data, int backwards, uint16_t *flags)
{
    if (binding == video_transition_tandy) {
        uint8_t previous = c_transition_tandy();
        *flags = (subtract8(*flags, previous, 1) & ~CF) | (*flags & CF);
        return 0;
    }
    if (binding == video_cga_scroll_software) {
        scroll_cga_software(data, *flags, 0);
        *flags = status(*flags, 0, 16);
        return 1;
    }
    if (binding == video_transition_cga || binding == video_transition_ega ||
        binding == video_transition_vga) {
        unsigned mode = binding == video_transition_cga ? 0
            : binding == video_transition_ega           ? 1
                                                        : 2;
        uint8_t enabled = transition_view(data, backwards, mode, 0);
        *flags = status(*flags, 0, 16) | (enabled ? 0x200 : 0);
        return 1;
    }
    struct VideoTransitionReply opaque = binding(data, backwards, *flags);
    if (opaque.kind == VIDEO_TRANSITION_PAGE) {
        *flags = (subtract8(*flags, opaque.previous, 1) & ~CF) | (*flags & CF);
        return 0;
    }
    if (opaque.kind == VIDEO_TRANSITION_SOFTWARE)
        *flags = opaque.scroll.flags;
    else
        *flags = status(*flags, 0, 16) | (opaque.view.interrupts_enabled ? 0x200 : 0);
    return 1;
}
