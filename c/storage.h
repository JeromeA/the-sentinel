#ifndef SENTINEL_STORAGE_H
#define SENTINEL_STORAGE_H
#include "register_compat.h"
#include "game_logic.h"
#include "sound_state.h"
#include "adapter_descriptor.h"
#include "video_callbacks.h"

/* Owned subsystem storage. Only the null-assignment guard has a required
 * DS address. Other objects are placed by the linker, with explicit views for
 * the few formats shared between rendering and landscape generation. */
struct __attribute__((packed)) CrtNullGuard {
    uint8_t prefix[8], copyright[56], trailer[2];
};
extern struct CrtNullGuard crt_null_guard;

union SpanWorkspace {
    uint16_t spans[256];
    uint8_t bytes[512];
};
extern union SpanWorkspace left_span_workspace, right_span_workspace;

struct OcclusionRow {
    uint8_t heights[TERRAIN_VERTEX_SIDE], thresholds[TERRAIN_VERTEX_SIDE], reserved[192];
};

_Static_assert(sizeof(struct OcclusionRow) == 256, "occlusion row stride");
_Static_assert(__builtin_offsetof(struct OcclusionRow, thresholds) == TERRAIN_VERTEX_SIDE,
    "occlusion threshold offset");
extern struct OcclusionRow occlusion_workspace[TERRAIN_VERTEX_SIDE];

/* Bindings are typed near C callback pointers in the primary code segment.
 * The menu owns four slots; normal startup offers at most three adapters. */
struct __attribute__((packed, may_alias)) GraphicsSession {
    uint16_t detected_card;
    ScatterCallback scatter;
    DecodeCallback decode;
    TileCallback tile;
    BackgroundCallback background;
    uint16_t title_image;
    TransitionCallback transition;
    WarningCallback warning;
    uint16_t menu_count, menu_column, menu_row, menu_width;
    uint16_t menu_ids[4], menu_names[4], menu_lengths[4];
    PaletteCallback palette;
    ColorsCallback colors;
    CursorRestoreCallback restore_cursor;
    uint16_t text_mode;
    SpansCallback spans;
    GlyphCallback glyph;
    SelectionCopyCallback selection_copy;
    CursorDrawCallback draw_cursor;
    FrontCallback front;
    RevealCallback reveal;
    uint16_t adapter, work_segment;
    BlitCallback blit;
    WorkCallback select_work;
    struct AdapterResults probe_results;
    uint16_t scroll_segment, skybox_segment;
    uint8_t reserved[12];
};

_Static_assert(sizeof(struct GraphicsSession) == 96, "graphics session size");
_Static_assert(offsetof(struct GraphicsSession, probe_results) == 76, "adapter results offset");
_Static_assert(offsetof(struct GraphicsSession, scroll_segment) == 80, "scroll segment offset");
extern struct GraphicsSession graphics_session;
void __far reset_graphics_session(SegmentBytes output);
void __far reset_polygon_arrays(SegmentBytes output);
void __far reset_world_objects(SegmentBytes output);
void __far reset_terrain_arrays(SegmentBytes output);
void __far reset_occlusion_rows(SegmentBytes output);
void __far reset_keyboard_state(SegmentBytes output);
void __far reset_generation_validation(SegmentBytes output);
void __far reset_mouse_state(SegmentBytes output);

extern uint8_t intro_theme_stream[96];
extern struct UnalignedWord resource_cga_color_patterns[32];
extern uint16_t speaker_pitch_divisors[48];
extern volatile uint8_t sound_muted;
extern volatile uint8_t sound_mute_saved;
extern uint16_t raster_x_word_offset;
extern uint16_t raster_color_index_a;
extern uint16_t raster_color_index_b;
extern union WorkWord raster_shade_bank_storage;
#define raster_shade_bank (raster_shade_bank_storage.bytes[0])
extern uint8_t cga_left_pixel_masks[4];
extern uint8_t cga_right_pixel_masks[4];
extern uint8_t cga_dither_patterns_a[32];
extern uint8_t cga_dither_patterns_b[32];
extern uint8_t ega_span_mask_a[8];
extern uint8_t ega_span_mask_b[8];
extern uint8_t ega_span_mask_c[8];
extern uint8_t tandy_edge_pixel_masks[2];
extern uint8_t tandy_right_pixel_masks[2];
extern uint8_t tandy_left_secondary_masks[2];
extern uint8_t tandy_right_secondary_masks[2];
extern uint8_t ega_polygon_busy;
extern uint16_t video_scanline_offsets[201];
extern uint16_t video_target_base;
extern struct UnalignedWord cga_background_pattern_a[8];
extern struct UnalignedWord tandy_background_patterns[4];
extern struct UnalignedWord vga_background_patterns[4];
extern uint8_t ega_background_patterns[5];
extern uint16_t polygon_projected_x_words[96];
extern uint8_t polygon_projected_x_capacity_tail[96];
extern uint8_t object_stack_links[64];
extern uint8_t object_view_pitch[64];
extern uint8_t terrain_cached_cells[TERRAIN_PROJECTED_CACHE_SIZE];
extern uint8_t terrain_cached_cells_capacity_tail[64];
extern uint8_t terrain_grid[TERRAIN_GRID_BYTES];
extern uint8_t terrain_grid_capacity_tail[512];
extern uint8_t object_grid_x[64];
extern uint8_t object_height_integer[64];
extern uint8_t object_grid_y[64];
extern uint8_t object_heading[64];
extern uint8_t projected_y_low[96];
extern uint8_t projected_x_low[96];
extern uint8_t object_height_fraction[64];
extern uint8_t projected_y_high[96];
extern uint8_t projected_x_high[96];
extern uint8_t object_model_id[64];
extern uint8_t terrain_face_indices[7];
extern volatile uint8_t key_down[128];
extern uint8_t generation_validation_ring[256];
extern uint8_t overview_pending;
extern uint8_t overview_camera_slot;
extern volatile uint8_t input_poll_delay_ticks;
extern volatile uint8_t mouse_activation_click_latch;
extern volatile union WorkWord mouse_cursor_mode_flags_storage;
#define mouse_cursor_mode_flags (mouse_cursor_mode_flags_storage.bytes[0])
extern uint8_t mouse_axis_priority;
extern volatile uint8_t cursor_update_guard_flags;
extern uint8_t software_cursor_x_pairs;
extern uint8_t software_cursor_inverted_y;
extern volatile uint8_t video_transition_busy_flags;
extern uint8_t generation_reference_prefix;
#define sound_effect_definitions ((uint8_t *)&sound_storage)
#define melody_effect_control (*((volatile union SoundControl *)&sound_storage.effects[3].control))
#define melody_effect_pitch (sound_storage.effects[3].pitch)
#define sound_voice_slots ((uint8_t *)sound_storage.voices)
#define video_scanline8_offset (video_scanline_offsets[8])
#define cga_background_pattern_b (cga_background_pattern_a + 1)
#define polygon_left_spans (left_span_workspace.spans)
#define terrain_filter_work (left_span_workspace.bytes)
#define placement_candidates (left_span_workspace.bytes + 64)
#define polygon_right_spans (right_span_workspace.spans)
#define placement_region_heights (right_span_workspace.bytes + 16)
#define placement_candidate_x (right_span_workspace.bytes + 96)
#define placement_candidate_y (right_span_workspace.bytes + 160)
#define terrain_model_cell_x (object_grid_x[63])
#define terrain_model_cell_y (object_grid_y[63])
#define terrain_model_shape (object_model_id[63])
#define occlusion_height_rows ((uint8_t *)occlusion_workspace)
#define occlusion_cell_threshold_rows (occlusion_workspace[0].thresholds)
#define generation_reference_window (&generation_reference_prefix)
#define graphics_adapter (graphics_session.adapter)
#define graphics_blit_image (graphics_session.blit)
#define skybox_image_segment (graphics_session.skybox_segment)
#define title_final_image_segment (graphics_session.title_image)
#define graphics_menu_count (graphics_session.menu_count)
#define graphics_menu_ids (graphics_session.menu_ids)
#define graphics_menu_names (graphics_session.menu_names)
#define graphics_menu_lengths (graphics_session.menu_lengths)
#define video_work_buffer_segment (graphics_session.work_segment)
#define graphics_view_transition (graphics_session.transition)
#define graphics_fill_background_band (graphics_session.background)
#define graphics_draw_status_tile (graphics_session.tile)
#define graphics_select_front_buffer (graphics_session.front)
#define graphics_draw_software_cursor (graphics_session.draw_cursor)
#define graphics_restore_software_cursor (graphics_session.restore_cursor)
#define graphics_copy_selection_strip (graphics_session.selection_copy)
#define graphics_reveal_selection_pixels (graphics_session.reveal)
#define model_vertex_range_ends (model_vertex_ranges + 1)
#define model_face_range_ends (model_face_ranges + 1)
#endif
