#include "text_bytecode.h"

/* Screen text has a small bytecode language for positioning, colors, nested
 * scripts and glyph flags. Terrain lettering remains a separate renderer. */

uint16_t __far c_text_newline(void)
{
    uint16_t row = game_text_row + 1;
    game_text_row = row >= 25 ? 0 : row;
    return row;
}

void __far text_render_glyph(
    SegmentBytes data, uint8_t character, int backwards, struct TextGlyphEnd *capture)
{
    if (capture) {
        struct TextGlyphEnd empty = {0};
        *capture = empty;
    }
    struct GlyphRequest request = {.character = character - TEXT_FIRST_GLYPH,
        .column = game_text_column,
        .row = game_text_row,
        .foreground = game_text_active_foreground,
        .background = game_text_active_background,
        .transparent = game_text_transparent,
        .skip = game_text_glyph_flags};
    GlyphCallback callback = segment_record(data, graphics_session).glyph;
    unsigned mode = callback == video_font_cga ? 0
        : callback == video_font_packed        ? 1
        : callback == video_font_vga           ? 2
                                               : 3;
    if (!glyph_request_valid(&request))
        return;
    prepare_glyph_request(data, &request, mode);
    if (capture) {
        call_video_glyph(callback, data, &request, backwards, &capture->glyph);
        capture->request = request;
        capture->mode = mode;
    } else
        call_video_glyph(callback, data, &request, backwards, 0);
}
