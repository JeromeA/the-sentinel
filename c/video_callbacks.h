#ifndef SENTINEL_VIDEO_CALLBACKS_H
#define SENTINEL_VIDEO_CALLBACKS_H
#include "typed_api.h"
/* Near, typed entry points keep the small-model callback ABI uniform across
 * the two code segments. Their implementations call the far C algorithms. */
void video_background_cga(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result);

enum VideoTransitionKind {
    VIDEO_TRANSITION_COPY,
    VIDEO_TRANSITION_PAGE,
    VIDEO_TRANSITION_SOFTWARE
};

struct VideoTransitionReply {
    struct ViewTransitionEnd view;
    struct SoftwareScrollEnd scroll;
    enum VideoTransitionKind kind;
    uint8_t previous;
};

void video_background_ega(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result);
void video_background_tandy(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result);
void video_background_vga(
    uint16_t row, uint16_t columns, int backwards, struct BackgroundEnd *result);
void video_blit_cga(const struct ImageSlice *slice, struct ImageCopyEnd *result);
void video_blit_ega(const struct ImageSlice *slice, struct ImageCopyEnd *result);
void video_blit_tandy(const struct ImageSlice *slice, struct ImageCopyEnd *result);
void video_blit_vga(const struct ImageSlice *slice, struct ImageCopyEnd *result);
struct VideoTransitionReply video_cga_scroll_software(
    SegmentBytes data, int backwards, uint16_t flags);
void video_colors_cga(uint16_t variant, uint16_t *observation);
void video_colors_ega(uint16_t variant, uint16_t *observation);
void video_colors_tandy(uint16_t variant, uint16_t *observation);
void video_colors_vga(uint16_t variant, uint16_t *observation);
struct ResourceDecodeEnd video_decode_cga(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags);
struct ResourceDecodeEnd video_decode_ega(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags);
struct ResourceDecodeEnd video_decode_packed(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t flags);
void video_draw_cursor_cga(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result);
void video_draw_cursor_ega(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result);
void video_draw_cursor_tandy(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result);
void video_draw_cursor_vga(SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result);
void video_font_cga(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result);
void video_font_ega(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result);
void video_font_packed(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result);
void video_font_vga(
    SegmentBytes data, const struct GlyphRequest *request, int backwards, struct GlyphEnd *result);
void video_front_cga(void);
void video_front_ega(void);
void video_front_tandy(void);
void video_front_vga(void);
struct PaletteEnd video_palette_cga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);
struct PaletteEnd video_palette_ega(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);
struct PaletteEnd video_palette_tandy(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);
struct PaletteEnd video_palette_vga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);
struct PaletteEnd video_palette_vga_ega(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);
void video_restore_cursor_cga(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result);
void video_restore_cursor_ega(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result);
void video_restore_cursor_tandy(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result);
void video_restore_cursor_vga(SegmentBytes data, uint16_t bank, struct CursorRestoreEnd *result);
void video_reveal_cga(int backwards, struct RevealEnd *result);
void video_reveal_ega(int backwards, struct RevealEnd *result);
void video_reveal_tandy(int backwards, struct RevealEnd *result);
void video_reveal_vga(int backwards, struct RevealEnd *result);
void video_scatter_cga(uint16_t count, uint8_t color, struct ScatterEnd *result);
void video_scatter_ega(uint16_t count, uint8_t color, struct ScatterEnd *result);
void video_scatter_tandy(uint16_t count, uint8_t color, struct ScatterEnd *result);
void video_scatter_vga(uint16_t count, uint8_t color, struct ScatterEnd *result);
struct GameStripCopy video_selection_copy_cga(uint16_t columns, uint16_t origin, int backwards);
struct GameStripCopy video_selection_copy_ega(uint16_t columns, uint16_t origin, int backwards);
struct GameStripCopy video_selection_copy_tandy(uint16_t columns, uint16_t origin, int backwards);
struct GameStripCopy video_selection_copy_vga(uint16_t columns, uint16_t origin, int backwards);
struct TandySpanEnd video_spans_cga(SegmentBytes data, int backwards);
struct TandySpanEnd video_spans_ega(SegmentBytes data, int backwards);
struct TandySpanEnd video_spans_tandy(SegmentBytes data, int backwards);
struct TandySpanEnd video_spans_vga(SegmentBytes data, int backwards);
void video_tile_cga(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result);
void video_tile_ega(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result);
void video_tile_tandy(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result);
void video_tile_vga(
    uint16_t tile, uint16_t column, SegmentBytes data, int backwards, struct StatusTileEnd *result);
struct VideoTransitionReply video_transition_cga(SegmentBytes data, int backwards, uint16_t flags);
struct VideoTransitionReply video_transition_ega(SegmentBytes data, int backwards, uint16_t flags);
struct VideoTransitionReply video_transition_tandy(
    SegmentBytes data, int backwards, uint16_t flags);
struct VideoTransitionReply video_transition_vga(SegmentBytes data, int backwards, uint16_t flags);
void video_warning_cga(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result);
void video_warning_ega(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result);
void video_warning_tandy(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result);
void video_warning_vga(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningEnd *result);
uint16_t video_work_ega(void);
uint16_t video_work_segment(void);
typedef struct PaletteEnd (*PaletteCallback)(SegmentBytes, SegmentBytes, uint16_t, uint16_t);
typedef void (*ColorsCallback)(uint16_t, uint16_t *);
typedef void (*BlitCallback)(const struct ImageSlice *, struct ImageCopyEnd *);
typedef void (*BackgroundCallback)(uint16_t, uint16_t, int, struct BackgroundEnd *);
typedef void (*TileCallback)(uint16_t, uint16_t, SegmentBytes, int, struct StatusTileEnd *);
typedef void (*ScatterCallback)(uint16_t, uint8_t, struct ScatterEnd *);
typedef struct TandySpanEnd (*SpansCallback)(SegmentBytes, int);
typedef void (*GlyphCallback)(SegmentBytes, const struct GlyphRequest *, int, struct GlyphEnd *);
typedef void (*FrontCallback)(void);
typedef uint16_t (*WorkCallback)(void);
typedef void (*WarningCallback)(SegmentBytes, int, uint16_t, struct WarningEnd *);
typedef void (*RevealCallback)(int, struct RevealEnd *);
typedef void (*CursorRestoreCallback)(SegmentBytes, uint16_t, struct CursorRestoreEnd *);
typedef struct GameStripCopy (*SelectionCopyCallback)(uint16_t, uint16_t, int);
typedef struct VideoTransitionReply (*TransitionCallback)(SegmentBytes, int, uint16_t);

struct VideoCallbacks {
    PaletteCallback palette;
    ColorsCallback colors;
    DecodeCallback decode;
    BlitCallback blit;
    GlyphCallback glyph;
    TileCallback tile;
    BackgroundCallback background;
    ScatterCallback scatter;
    SpansCallback spans;
    CursorDrawCallback draw_cursor;
    CursorRestoreCallback restore_cursor;
    FrontCallback front;
    WorkCallback select_work;
    WarningCallback warning;
    SelectionCopyCallback selection_copy;
    RevealCallback reveal;
};
struct PaletteEnd __far call_video_palette(PaletteCallback callback,
    SegmentBytes data,
    SegmentBytes output,
    uint16_t phase,
    uint16_t flags);
void __far call_video_colors(ColorsCallback callback, uint16_t variant, uint16_t *observation);
struct ResourceDecodeEnd __far call_video_decode(DecodeCallback callback,
    uint16_t handle,
    SegmentBytes destination,
    SegmentBytes data,
    uint16_t flags);
void __far call_video_blit(
    BlitCallback callback, const struct ImageSlice *slice, struct ImageCopyEnd *result);
void __far call_video_background(BackgroundCallback callback,
    uint16_t row,
    uint16_t columns,
    int backwards,
    struct BackgroundEnd *result);
void __far call_video_tile(TileCallback callback,
    uint16_t tile,
    uint16_t column,
    SegmentBytes data,
    int backwards,
    struct StatusTileEnd *result);
void __far call_video_scatter(
    ScatterCallback callback, uint16_t count, uint8_t color, struct ScatterEnd *result);
uint8_t __far draw_video_spans(SpansCallback callback, SegmentBytes data, int backwards);
struct TandySpanEnd __far call_video_spans(
    SpansCallback callback, SegmentBytes data, int backwards);
void __far call_video_glyph(GlyphCallback callback,
    SegmentBytes data,
    const struct GlyphRequest *request,
    int backwards,
    struct GlyphEnd *result);
void __far call_video_front(FrontCallback callback);
uint16_t __far call_video_work(WorkCallback callback);
void __far call_video_warning(WarningCallback callback,
    SegmentBytes data,
    int backwards,
    uint16_t idle_port,
    struct WarningEnd *result);
void __far call_video_reveal(RevealCallback callback, int backwards, struct RevealEnd *result);
void __far call_video_cursor_draw(
    CursorDrawCallback callback, SegmentBytes data, uint16_t bank, struct CursorDrawEnd *result);
void __far call_video_cursor_restore(CursorRestoreCallback callback,
    SegmentBytes data,
    uint16_t bank,
    struct CursorRestoreEnd *result);
struct GameStripCopy __far call_video_selection_copy(
    SelectionCopyCallback callback, uint16_t columns, uint16_t origin, int backwards);
struct VideoTransitionReply __far call_video_transition(
    TransitionCallback callback, SegmentBytes data, int backwards, uint16_t flags);
uint8_t __far dissolve_selection(RevealCallback binding, int backwards);
void __far present_selection_band(
    SelectionCopyCallback binding, uint16_t columns, uint16_t origin, int backwards);
uint8_t __far present_view_transition(
    TransitionCallback binding, SegmentBytes data, int backwards, uint16_t *flags);
#endif
