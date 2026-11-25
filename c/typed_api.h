#ifndef SENTINEL_TYPED_API_H
#define SENTINEL_TYPED_API_H
#include <stdint.h>
#include <stddef.h>
#include "video_copy.h"
#include "game_logic.h"

#include "video_rows.h"
#include "rgb_palette.h"
#include "adapter_descriptor.h"
#include "terrain_cell.h"
#include "mesh_format.h"
#include "text_scripts.h"

void __far c_viewport_preset(uint16_t);
void __far c_viewport_width(uint8_t);

enum VideoPixelLayout {
    PIXEL_CGA,
    PIXEL_TANDY,
    PIXEL_EGA,
    PIXEL_VGA
};

/* Glyph legacy mode ordering differs from the other pixel operations. */
enum GlyphLayout {
    GLYPH_CGA,
    GLYPH_PACKED,
    GLYPH_VGA,
    GLYPH_EGA
};

enum RenderPanel {
    RENDER_TITLE,
    RENDER_SKYBOX,
    RENDER_LANDSCAPE_PANEL,
    RENDER_PROMPT_HEADER,
    RENDER_PROMPT_BACKGROUND
};

struct ImageSlice {
    uint16_t source_segment, first_row, rows, columns;
    int backwards;
};

struct ImageCopyEnd {
    uint16_t source_offset, destination_offset, row_tail, bank_step, next_bank_step, word_width,
        wrapped;
};

struct BackgroundEnd {
    uint16_t destination_offset, word_width, other_step, second_pattern;
};

struct StatusTileEnd {
    uint16_t source_offset, destination_offset, pattern, bank_step, next_bank_step, last_pixel,
        wrapped;
    uint8_t valid;
};

uint16_t __far c_work_segment(void);
void __far c_work_ega(void);

struct GlyphRequest {
    uint16_t character, column, row, foreground, background, transparent, skip;
    uint16_t row_offset;
};

struct GlyphEnd {
    uint16_t last_background, bank_step, packed_colors, destination_offset, carry, wrapped;
};

int __far glyph_request_valid(const struct GlyphRequest *request);
void __far prepare_glyph_request(SegmentBytes, struct GlyphRequest *, enum GlyphLayout);

struct CountdownTick {
    uint8_t previous_phase, last_counter;
};

struct ActionScanEnd {
    uint16_t binding_end, last_slot;
    uint8_t combined_action;
};

uint8_t __far c_test_scan(uint16_t);
SegmentBytes __far c_clear_pending(SegmentBytes, int);
uint16_t __far c_mouse_edge_actions(void);
struct CountdownTick __far c_sentry_countdowns(void);
void __far c_collect_actions(SegmentBytes, int, uint16_t, uint16_t, struct ActionScanEnd *);

struct EnergyChange {
    uint8_t model, cost, before, after;
};

uint8_t __far c_random_decimal_pair(void);
uint8_t __far c_random_small_sum(uint8_t *);
uint8_t __far c_random_grid_coordinate(void);
uint8_t __far c_select_landscape(union LandscapeNumber);
uint8_t __far c_random_sentry_count(uint8_t *);
uint8_t __far c_energy_bcd(void);
void __far c_start_melody(uint8_t);
int __far c_allocate_saved(void);
int __far c_allocate_model(uint8_t);
int __far c_adjust_energy(uint16_t, int, struct EnergyChange *);
struct GameStripCopy __far c_copy_cga_strip(SegmentBytes, SegmentBytes, uint16_t, uint16_t, int);
struct GameStripCopy __far c_copy_four_bank_strip(
    SegmentBytes, SegmentBytes, uint16_t, uint16_t, int);

struct PaletteFade {
    uint8_t decrement, remainder, last_value, padding;
};
struct PaletteFade __far c_darken_palette(void);
void __far c_apply_landscape_colors(
    uint16_t variant, enum VideoPixelLayout layout, uint16_t *observation);
uint16_t __far c_colors_cga(uint16_t);
uint16_t __far c_colors_tandy(uint16_t);
uint16_t __far c_colors_ega(uint16_t);
uint16_t __far c_colors_vga(uint16_t);
void __far draw_panel(enum RenderPanel panel, int backwards);
unsigned __far c_blit_panel(enum RenderPanel panel, int backwards, struct ImageCopyEnd *end);

struct ScatterEnd {
    uint16_t noise_product_high, destination_offset, mask_index;
    uint8_t mask_updated, interrupts_enabled;
};

struct CursorRestoreEnd {
    uint16_t destination_offset;
    uint8_t plane_value, restored;
};

struct CursorDrawEnd {
    uint16_t destination_offset, row_offset, x_pairs, terminal_step, mask_index;
    uint8_t executed, destination_updated, mask_updated, skip_status, saved_count;
};

typedef void (*CursorDrawCallback)(SegmentBytes, uint16_t, struct CursorDrawEnd *);

struct WarningEnd;
struct RevealEnd;
void __far blit_image(const struct ImageSlice *, struct ImageCopyEnd *, enum VideoPixelLayout);
void __far fill_background(uint16_t, uint16_t, int, struct BackgroundEnd *, enum VideoPixelLayout);
void __far front_buffer(enum VideoPixelLayout);
void __far status_tile(
    uint16_t, uint16_t, SegmentBytes, int, struct StatusTileEnd *, enum VideoPixelLayout);
/* This kernel retains its IA16 register-argument convention across C callers. */
#ifdef __ia16__
#define GLYPH_REGISTER_CALL __attribute__((regparmcall))
#else
#define GLYPH_REGISTER_CALL
#endif
void __far GLYPH_REGISTER_CALL font_glyph(
    SegmentBytes, const struct GlyphRequest *, int, struct GlyphEnd *, enum GlyphLayout);
uint8_t __far scatter_pixels(uint16_t, uint8_t, struct ScatterEnd *, enum VideoPixelLayout);
uint8_t __far warning_bar(SegmentBytes, int, uint16_t, struct WarningEnd *, enum VideoPixelLayout);
uint8_t __far reveal_selection(int, struct RevealEnd *, enum VideoPixelLayout);
struct GameStripCopy __far copy_selection_band(uint16_t, uint16_t, int, enum VideoPixelLayout);
void __far restore_cursor(SegmentBytes, uint16_t, struct CursorRestoreEnd *, enum VideoPixelLayout);
void __far draw_cursor(SegmentBytes, uint16_t, struct CursorDrawEnd *, enum VideoPixelLayout);

struct CursorSync {
    uint8_t inverted_y, transition_index;
};

void __far c_mouse_view_edges(uint16_t);
struct CursorSync __far c_sync_software_cursor(void);

uint8_t __far draw_cursor_effects(SegmentBytes data,
    uint16_t bank,
    struct CursorDrawEnd *observation,
    enum VideoPixelLayout mode,
    uint16_t *flags);
void __far publish_selection_band(uint16_t columns,
    uint16_t column,
    int backwards,
    enum VideoPixelLayout mode,
    struct GameStripCopy *observation);

struct WarningEnd {
    uint16_t source_offset, destination_offset, last_noise, pattern_index;
    uint8_t pattern, completed, drawn, skip_status;
};

struct RevealEnd {
    uint16_t source_offset, destination_offset, last_merge_operand;
    uint8_t interrupts_enabled;
};

struct PlanarDeltaEnd {
    uint8_t delta_y, absolute_y, object_y, observer_y;
};

struct HeightDeltaEnd {
    uint16_t object_height, observer_height;
};

struct OctantEnd {
    uint16_t table_index, angle;
    uint8_t equal;
};

struct TrigLookupEnd {
    uint16_t reverse_index;
    uint8_t heading, phase, sine, cosine;
};

struct EncodedProduct {
    uint16_t magnitude, value;
    uint8_t negative;
};

struct BearingEnd {
    struct OctantEnd octant;
    uint16_t minor, major;
    uint8_t y_major, adjustment, angle_high;
};

/* Quantized planar geometry, shared by angular and distance calculations. */
struct BearingGeometry {
    uint16_t angle, minor, major, ratio;
    uint8_t x_sign, y_sign, y_major;
};

static inline uint16_t bearing_table_index(const struct BearingGeometry *geometry)
{
    return geometry->minor == geometry->major ? geometry->minor : (geometry->ratio >> 4) & 0xfffe;
}

struct VerticalAngleEnd {
    struct BearingGeometry bearing;
    uint16_t angle, pitch, screen_y;
};

struct PlanarDistanceEnd {
    uint16_t table_index, product_low, correction, major, value;
};
struct PlanarDeltaEnd __far c_model_planar_delta(uint16_t observer, uint16_t object);
struct HeightDeltaEnd __far c_model_height_delta(uint16_t observer, uint16_t object);
void __far c_atan_equal(void);
struct OctantEnd __far c_atan_octant(uint16_t minor, uint16_t major);
struct TrigLookupEnd __far c_lookup_trig(uint8_t angle);
struct EncodedProduct __far c_multiply_encoded(void);
struct BearingEnd __far c_bearing_y_major(uint16_t x, uint16_t y);
struct BearingGeometry __far c_bearing(uint16_t x, uint16_t y, uint8_t x_sign, uint8_t y_sign);
struct VerticalAngleEnd __far c_vertical_angle(
    uint16_t height, uint16_t distance, uint16_t observer);
struct PlanarDistanceEnd __far c_planar_distance(const struct BearingGeometry *geometry);
struct BearingEnd __far bearing_observation(const struct BearingGeometry *geometry);
void __far publish_projection_axes(const struct BearingGeometry *geometry);
void __far publish_projection_bearing(const struct BearingGeometry *geometry);
void __far publish_vertical_projection(const struct VerticalAngleEnd *vertical);

struct TextLengthEnd {
    uint16_t length, remaining, legacy_extent;
};

struct TextCompareEnd {
    uint8_t left, right;
};

uint16_t __far c_text_attribute(uint16_t foreground, uint16_t background);
struct TextLengthEnd __far c_string_length(SegmentBytes text, int backwards);
struct TextCompareEnd __far c_string_compare(SegmentBytes left, SegmentBytes right, int backwards);
void __far c_memory_fill(SegmentBytes destination, uint8_t value, uint16_t count, int backwards);
void __far c_memory_copy(
    SegmentBytes destination, SegmentBytes source, uint16_t count, int backwards);

struct PitchDivision {
    uint16_t divisor;
    int8_t octave;
    uint8_t carry;
};
struct PitchDivision __far c_speaker_pitch(uint8_t pitch, uint8_t octave);
uint8_t __far c_sound_effect(int8_t effect);
uint8_t __far c_silence_voice(uint8_t voice);
uint8_t __far c_silence_all(void);

struct HeightCandidatesEnd {
    uint16_t count, bit;
    uint8_t mask;
};

struct PlacementSampleEnd {
    uint16_t bank, offset;
    uint8_t value;
};

struct RayCenterDistance {
    uint8_t x, y, distance;
};

struct VideoRetraceEnd {
    uint16_t port;
    uint8_t status;
};

void __far c_keyboard_cursor_horizontal(void);
void __far c_keyboard_cursor_vertical(uint16_t robot_bank);
uint8_t __far c_input_snapshot(void);
uint8_t __far c_input_ready(void);
uint8_t __far c_center_cursor(void);
void __far c_reset_sentry_scan(uint16_t slot);
struct HeightCandidatesEnd __far c_collect_height_candidates(
    uint8_t required_height, const uint8_t *region_heights, uint8_t *indices);
void __far c_publish_height_candidates(struct HeightCandidatesEnd *end);
struct PlacementSampleEnd __far c_sample_placement(void);
uint8_t __far c_configure_preview_object(uint8_t model, uint16_t record);
struct RayCenterDistance __far c_ray_center_distance(uint8_t fraction_x, uint8_t fraction_y);
uint16_t __far c_text_newline(void);
void __far wait_for_video_retrace(struct VideoRetraceEnd *observation);
struct VideoRetraceEnd __far c_wait_video_retrace(void);
uint8_t __far c_transition_tandy(void);

struct RayTrigEnd {
    uint16_t phase;
    uint8_t phase_used, sign_test, angle;
};

struct RayDirectionEnd {
    struct RayTrigEnd pitch, heading;
    struct EncodedProduct x;
};

void __far c_synthesize_ray_direction(uint16_t pitch, SegmentBytes data);
void __far c_synthesize_cursor_ray(uint16_t observer, SegmentBytes data);
/* Raw observations for original-entry compatibility, not native continuations. */
struct RayTrigEnd __far c_ray_trig(uint16_t direction_angle, SegmentBytes data);
uint16_t __far c_ray_decode_component(SegmentBytes data, uint16_t component_offset);
struct EncodedProduct __far c_ray_scale_component(
    SegmentBytes data, uint16_t component_offset, uint16_t vector_offset);
struct RayDirectionEnd __far c_ray_pitch_direction(uint16_t pitch, SegmentBytes data);
struct RayDirectionEnd __far c_build_cursor_ray(uint16_t observer, SegmentBytes data);

enum ResourceFormat {
    RESOURCE_CGA,
    RESOURCE_EGA,
    RESOURCE_PACKED
};

struct ResourceReader {
    uint16_t handle, remaining;
    SegmentBytes cursor;
    uint16_t flags;
    SegmentBytes data;
};

struct ResourceDecodeEnd {
    uint16_t last_word, last_color, remaining, io_flags, carry;
};

uint16_t __far c_resource_word(struct ResourceReader *reader);
struct ResourceDecodeEnd __far c_decode_cga(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags);
struct ResourceDecodeEnd __far c_decode_ega(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags);
struct ResourceDecodeEnd __far c_decode_packed(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags);

struct TerrainCellAddress {
    uint16_t bank, offset;
    union TerrainCell value;
};

struct ObjectDetachEnd {
    struct TerrainCellAddress cell;
    uint8_t support, height, value;
};

struct ObjectAttachEnd {
    struct TerrainCellAddress cell;
    uint16_t support_slot;
    uint8_t support_model, accepted, random, random_high, heading;
};

struct ObjectPlacementEnd {
    struct ObjectAttachEnd attachment;
    struct TerrainCellAddress failure_cell;
    uint8_t found, height_limit;
};
struct TerrainCellAddress __far c_terrain_address(uint8_t x, uint8_t y);
uint8_t __far c_ray_object_origin(uint16_t slot);
struct ObjectDetachEnd __far c_detach_object(uint16_t slot, SegmentBytes data);
struct ObjectAttachEnd __far c_attach_object(uint16_t slot, SegmentBytes data);
struct ObjectPlacementEnd __far c_place_object_flat(
    uint16_t slot, uint8_t height_limit, SegmentBytes samples);

struct AdapterStoreEnd {
    uint16_t destination;
    uint8_t previous;
};

struct CrtcProbeEnd {
    uint8_t saved, observed;
};

struct WorkspaceEnd {
    uint8_t previous_noise, y_min;
};

struct DecimalPackEnd {
    uint16_t packed, last_low, source_end, destination_end;
};

struct KeyboardScanEnd {
    uint16_t index;
    uint8_t prefix, buttons;
};

struct PitCalibrationEnd {
    uint16_t maximum, last;
};
struct AdapterStoreEnd __far c_probe_store_adapter(
    SegmentBytes data, uint16_t result, struct AdapterDescriptor descriptor);
struct CrtcProbeEnd __far c_probe_crtc_register(uint16_t port);
uint8_t __far c_clip_view_index(uint16_t preset);
uint8_t __far c_clip_view_full(void);
uint8_t __far c_clip_view_one(void);
uint8_t __far c_clip_view_dynamic(void);
int __far initialize_scene(SegmentBytes, SegmentBytes, int, struct WorkspaceEnd *);
int __far finish_scene_initialization(SegmentBytes, int, struct WorkspaceEnd *);
struct WorkspaceEnd __far c_scene_workspace_final(SegmentBytes output, int backwards);
struct DecimalPackEnd __far c_pack_decimal_digits(
    SegmentBytes data, SegmentBytes output, int backwards);
struct KeyboardScanEnd __far c_keyboard_irq_scan(void);
struct KeyboardScanEnd __far c_keyboard_bios_scan(uint8_t scan);
struct PitCalibrationEnd __far c_calibrate_pit(void);

struct SpanFillEnd {
    uint16_t span_count, screen_row, span_offset, edge_work;
    uint8_t valid;
};
struct SpanFillEnd __far c_spans_cga(int backwards);
struct SpanFillEnd __far c_spans_ega(int backwards);
struct SpanFillEnd __far c_spans_vga(int backwards);

struct EdgeRasterEnd {
    uint16_t value, destination, count, error, remaining_y;
    uint8_t compare_left, compare_right, compare_subtract, completed, x_major;
};

struct RasterEdge {
    uint16_t start_x, start_y, end_x, end_y, delta_y;
};

uint8_t __far rasterize_polygon_edge(const struct RasterEdge *,
    SegmentBytes y_spans,
    SegmentBytes x_spans,
    struct EdgeRasterEnd *observation);
void __far fill_polygon_spans(int backwards, unsigned mode, struct SpanFillEnd *observation);
struct EdgeRasterEnd __far c_polygon_rasterize_edge(SegmentBytes y_spans, SegmentBytes x_spans);

struct TerrainFilterEnd {
    struct TerrainCellAddress cell;
    uint16_t work_pointer;
    uint8_t work_pointer_set, value;
};

struct TerrainTransformEnd {
    struct TerrainCellAddress cell;
    uint8_t value, high, nibble, carry;
};

struct TerrainClassifyEnd {
    struct TerrainCellAddress cell;
    uint8_t value, shape, comparison, flat;
};
struct TerrainFilterEnd __far c_terrain_filter(
    uint8_t mode, uint8_t fixed_coordinate, SegmentBytes data);
struct TerrainFilterEnd __far c_terrain_smooth(uint8_t flags, SegmentBytes data);
struct TerrainTransformEnd __far c_terrain_transform(uint8_t mode);
struct TerrainClassifyEnd __far c_terrain_classify(uint8_t x, uint8_t y);

struct ViewTransitionEnd {
    uint16_t bank_base, source_end, destination_end, copy_count;
    uint8_t copied, interrupts_enabled;
};

struct TwoBandCopyEnd {
    struct GameStripCopy copy;
    uint16_t second_source, second_destination;
};

uint8_t __far transition_view(
    SegmentBytes data, int backwards, unsigned mode, struct ViewTransitionEnd *observation);
struct ViewTransitionEnd __far c_transition_cga(SegmentBytes data, int backwards);
struct ViewTransitionEnd __far c_transition_ega(SegmentBytes data, int backwards);
struct ViewTransitionEnd __far c_transition_vga(SegmentBytes data, int backwards);
struct TwoBandCopyEnd __far c_copy_two_cga_bands(
    SegmentBytes source, SegmentBytes destination, uint16_t src, uint16_t dst, int backwards);
struct TwoBandCopyEnd __far c_copy_two_four_bank_bands(
    SegmentBytes source, SegmentBytes destination, uint16_t src, uint16_t dst, int backwards);

struct InputPanelEnd {
    uint16_t source_end, destination_end;
    uint8_t carry, planar, vga;
};

struct BackgroundProbeEnd {
    uint16_t row, destination, alternate, comparison_left, comparison_right;
    uint8_t occupied, planar;
};

struct HudPublishEnd {
    uint16_t source_end, destination_end;
    uint8_t copied, cga;
};
struct InputPanelEnd __far c_expand_input_panel(int backwards);
struct BackgroundProbeEnd __far c_background_is_occupied(SegmentBytes data);
struct HudPublishEnd __far c_present_hud(int backwards);

struct AdapterNameEnd {
    uint16_t table_offset, name;
};
struct AdapterNameEnd __far c_adapter_name(SegmentBytes data, uint8_t adapter);

struct HudTileEnd {
    struct StatusTileEnd tile;
    uint16_t number;
    uint8_t mode, column;
};

struct TextGlyphEnd {
    struct GlyphRequest request;
    struct GlyphEnd glyph;
    uint8_t mode;
};

struct TerrainGlyphEnd {
    uint16_t bank, offset, pair;
    uint8_t coordinate, control, column;
};
struct HudTileEnd __far c_hud_append(uint8_t tile, SegmentBytes data, int backwards);
void __far text_render_glyph(
    SegmentBytes data, uint8_t character, int backwards, struct TextGlyphEnd *capture);
void __far draw_terrain_character(uint8_t character,
    SegmentBytes data,
    SegmentBytes output,
    int backwards,
    struct TerrainGlyphEnd *capture);
struct TerrainGlyphEnd __far c_terrain_character(
    uint8_t character, SegmentBytes data, SegmentBytes output, int backwards);

CursorDrawCallback __far c_keyboard_cursor_move(SegmentBytes data, uint16_t robot_bank);
void __far c_input_check_draw_interrupt(
    SegmentBytes output, int backwards, uint16_t first_binding, uint16_t scan_bank);

enum MelodyEvent {
    MELODY_IDLE,
    MELODY_INVALID_STREAM,
    MELODY_ENDED,
    MELODY_NOTE
};

/* AL/CL writes accumulate across duration commands, even if a later cursor
 * check ends the step without fetching another note. */
struct MelodyObservation {
    uint8_t accumulator_low, duration_units, accumulator_written, duration_written;
};

struct MelodyStep {
    enum MelodyEvent event;
    uint16_t cursor;
    struct MelodyObservation observations;

    union {
        struct {
            uint8_t scan_position;
        } idle;

        struct {
            uint8_t sound_status;
        } note;
    } payload;
};

enum SoundSequenceMode {
    SOUND_MODE_MELODY = 3,
    SOUND_MODE_FIXED = 4,
    SOUND_MODE_RAMP = 6
};

enum SoundSequenceEvent {
    SOUND_PENDING,
    SOUND_IDLE,
    SOUND_FIXED_EFFECT,
    SOUND_MELODY,
    SOUND_RAMP_LIMIT,
    SOUND_RAMP_ADVANCED
};

struct SoundTriggerEnd {
    enum SoundSequenceEvent event;

    union {
        struct {
            uint8_t countdown;
        } pending;

        struct {
            uint8_t mode;
        } idle;

        struct {
            uint8_t effect, sound_status;
        } fixed_effect;
        struct MelodyStep melody;

        struct {
            uint8_t index;
        } ramp_limit;

        struct {
            uint8_t previous_index, pitch, random_high;
        } ramp_advance;
    } payload;
};

/* Only these effects survive the timer's sound sequence stage. */
struct SoundTickEffects {
    uint16_t lookup_bank;
    uint8_t warning_port, interrupts_enabled;
};
struct SoundTickEffects __far c_tick_sound_sequence(SegmentBytes data);
void __far c_tick_voices(void);

struct VoiceTickEnd {
    uint16_t sample, pitch, sample_mask, pitch_mask;
};
struct MelodyStep __far c_advance_melody(uint16_t stream_bank);
struct SoundTriggerEnd __far c_sound_triggers(SegmentBytes data, uint16_t stream_bank);
struct VoiceTickEnd __far c_update_voices(void);

struct CrtcAdapterEnd {
    struct CrtcProbeEnd probe;
    struct AdapterStoreEnd store;
    struct AdapterDescriptor descriptor;
    uint16_t remaining;
    uint8_t accepted;
};

enum PostActionRoute {
    POST_ACTION_TITLE,
    POST_ACTION_LOSS,
    POST_ACTION_PLAY
};

struct PostActionEnd {
    uint8_t speaker_control, observed_state;
    enum PostActionRoute route;
};
struct CrtcAdapterEnd __far c_probe_color_crtc(SegmentBytes data, uint16_t result);
struct CrtcAdapterEnd __far c_probe_mono_crtc(SegmentBytes data, uint16_t result);
struct PostActionEnd __far c_post_action_gate(void);

struct ModelOriginEnd {
    struct BearingGeometry bearing;
    struct PlanarDistanceEnd distance;
    uint16_t observer_height, height, height_before_bias;
    uint8_t preset, scaled;
};
struct ModelOriginEnd __far c_prepare_model_origin(uint16_t object_index, uint16_t drawn_index);

struct MeshVerticesEnd {
    struct VerticalAngleEnd vertical;
    uint16_t next_vertex;
    uint8_t valid;
};
struct MeshVerticesEnd __far c_project_mesh_vertices(void);

struct SurfaceSample {
    uint16_t selected_index;
    uint8_t height, stack;
};

_Static_assert(sizeof(struct SurfaceSample) == 4, "surface sample scalar result");
struct WorldHeightEnd;
/* The raw wrapper clears capture before reading the published coordinates.
 * Native callers supply their computed point and no observation packet. */
struct SurfaceSample __far sample_surface_at(
    uint8_t x, uint8_t y, uint16_t object_bank, struct WorldHeightEnd *capture);
struct SurfaceSample __far sample_world_surface(
    uint16_t object_bank, struct WorldHeightEnd *capture);
uint8_t __far ray_center_distance(
    uint8_t fraction_x, uint8_t fraction_y, struct RayCenterDistance *capture);
void __far rebuild_visibility(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t cache_bank,
    int backwards);

struct WorldHeightEnd {
    uint16_t bank, offset, index, value;
    uint8_t stack, quarter, quarter_set, flag_left, flag_right, operation;
};
struct WorldHeightEnd __far c_sample_world_height(uint16_t object_bank);

struct OcclusionHeightsEnd {
    struct WorldHeightEnd sample;
    uint8_t threshold;
};
struct OcclusionHeightsEnd __far c_build_occlusion_heights(SegmentBytes data, uint16_t object_bank);
uint8_t __far action_error(SegmentBytes data);
void __far action_refresh(uint8_t melody);

struct OcclusionRayEnd {
    uint16_t threshold_row, cache_index;
    uint8_t sampled;
};
struct OcclusionRayEnd __far c_trace_occlusion_row(
    SegmentBytes data, uint16_t observer_bank, uint16_t cache_bank);

struct VisibilityEnd {
    uint8_t outcome, rebuilt, last_old, last_value, last_mask;
};
struct VisibilityEnd __far c_rebuild_visibility_bits(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t cache_bank,
    int backwards);

struct TerrainCornerEnd {
    struct VerticalAngleEnd vertical;
    uint16_t terrain_bank;
    uint8_t slot, column, clip;
};
struct TerrainCornerEnd __far c_project_terrain_corner(uint8_t column, uint16_t observer_bank);

struct TerrainStepEnd {
    struct TerrainCornerEnd corner;
    uint8_t column, boundary;
};
struct TerrainStepEnd __far c_previous_terrain_corner(uint16_t observer_bank);
struct TerrainStepEnd __far c_next_terrain_corner(uint16_t observer_bank);
struct TerrainCornerEnd __far terrain_corner_at(uint8_t column, uint16_t observer_bank);

struct TerrainIntervalEnd {
    struct TerrainCornerEnd corner;
    uint8_t column, boundary, scan;
};
struct TerrainIntervalEnd __far c_find_terrain_interval(uint16_t observer_bank);

/* Native text operations expose only output selection and real stream progress. */
struct TextStreamEffect {
    uint16_t next_source;
    uint8_t output_selected;
};

int __far write_text_script(SegmentBytes data, TextScriptId script, int backwards);
struct TextStreamEffect __far write_inline_text(
    SegmentBytes data, uint8_t character, int backwards, uint16_t source);
int __far write_decimal_pair(
    SegmentBytes data, SegmentBytes output, union BcdPair pair, int backwards);
int __far write_landscape_number(SegmentBytes data, SegmentBytes output, int backwards);
int __far repaint_digit_field(SegmentBytes data, SegmentBytes output, int backwards);
void __far write_completion_terrain(SegmentBytes data, SegmentBytes output, int backwards);

struct TextCharacterEnd {
    uint16_t value, flags, port, port_mask;
    uint8_t rendered;
};
struct TextCharacterEnd __far c_text_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags);

struct FormattedCharacterEnd {
    struct TextCharacterEnd character;
    uint8_t formatted;
};

struct TextRunEnd {
    struct TextCharacterEnd character;
    uint16_t source;
    TextScriptId nested_script;
    uint8_t formatted, source_written, script_written, script_run;
};
struct FormattedCharacterEnd __far c_formatted_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags);
struct TextRunEnd __far c_script_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags);
struct TextRunEnd __far c_draw_text_script(SegmentBytes data,
    TextScriptId script,
    uint16_t initial_character_word,
    uint16_t initial_flags);
struct TextRunEnd __far c_script_inline_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags, uint16_t source);

struct NumberTextEnd {
    struct TextCharacterEnd character;
    uint16_t pair, terrain_bank, terrain_offset;
    uint8_t pair_written, terrain_written;
};
struct NumberTextEnd __far c_decimal_character(
    SegmentBytes data, SegmentBytes output, uint16_t character_word, uint16_t initial_flags);
struct NumberTextEnd __far c_decimal_digit(
    SegmentBytes data, SegmentBytes output, uint16_t digit_word, uint16_t flags);
struct NumberTextEnd __far c_draw_decimal_pair(
    SegmentBytes data, SegmentBytes output, uint16_t packed_pair, uint16_t flags);
struct NumberTextEnd __far c_draw_landscape_bcd(
    SegmentBytes data, SegmentBytes output, uint16_t initial_character_word, uint16_t flags);
struct NumberTextEnd __far c_completion_code_terrain(
    SegmentBytes data, SegmentBytes output, uint16_t counter_bank, uint16_t initial_flags);
struct NumberTextEnd __far c_repaint_digit_field(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);

enum PolygonOutput {
    POLYGON_VERTEX = 1,
    POLYGON_COUNT = 2,
    POLYGON_END_SLOT = 4,
    POLYGON_DESTINATION = 8,
    POLYGON_VERTEX_OFFSET = 16,
    POLYGON_ERROR = 32
};

struct PolygonScanEnd {
    uint16_t value, flags, start_vertex, span_count, end_slot, span_destination, vertex_offset,
        error;
    uint8_t changed;
};

struct PolygonCoverage {
    uint16_t source, target;
    uint8_t source_updated, visible, backwards;
};
struct PolygonCoverage __far build_polygon_coverage(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t previous_vertex,
    int backwards,
    struct PolygonScanEnd *observation);
struct PolygonScanEnd __far c_polygon_subdivide(SegmentBytes y_spans,
    SegmentBytes x_spans,
    uint8_t start_vertex,
    uint8_t end_vertex,
    uint16_t initial_work,
    uint16_t initial_flags);
struct PolygonScanEnd __far polygon_project_and_scan(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t initial_work,
    uint16_t previous_vertex,
    uint16_t initial_flags);
struct PolygonScanEnd __far c_polygon_triangle(
    SegmentBytes data, SegmentBytes y_spans, uint16_t corner_flags, uint16_t initial_flags);
struct PolygonScanEnd __far c_polygon_build_spans(
    SegmentBytes data, SegmentBytes y_spans, uint16_t previous_vertex, uint16_t initial_flags);

struct PlatformReply {
    uint16_t value, handle, count, offset, extra_segment, flags;
};
struct PlatformReply __far platform_video(uint16_t function,
    uint16_t handle,
    uint16_t count,
    uint16_t offset,
    uint16_t data_segment,
    uint16_t extra_segment,
    uint16_t initial_flags);
struct PlatformReply __far platform_dos(uint16_t function,
    uint16_t handle,
    uint16_t count,
    uint16_t offset,
    uint16_t data_segment,
    uint16_t extra_segment,
    uint16_t initial_flags);

struct DosMemoryEnd {
    uint16_t segment, paragraphs, flags;
};

struct DosOpenEnd {
    uint16_t handle, flags;
};

struct DosSizeEnd {
    uint16_t low, high, flags;
};

struct DosPrintEnd {
    uint16_t value, remaining, flags;
};

struct VideoModeSetEnd {
    uint16_t crtc_port, flags;
};

struct VideoModeEnd {
    uint16_t mode, page, flags;
};

struct ConsoleTextEnd {
    uint16_t value, count, flags;
    uint8_t written;
};
struct DosMemoryEnd __far dos_allocate(
    uint16_t paragraphs, uint8_t previous_low, uint16_t initial_flags);
struct VideoModeSetEnd __far bios_mode(uint16_t mode, uint16_t initial_flags);
struct VideoModeSetEnd __far c_bios_mode(uint16_t mode, uint16_t initial_flags);
struct VideoModeEnd __far c_bios_get_mode(uint16_t initial_flags);
struct DosOpenEnd __far c_dos_open(SegmentBytes filename, uint8_t mode, uint16_t initial_flags);
struct DosOpenEnd __far c_dos_create(
    SegmentBytes filename, uint8_t request_low, uint16_t initial_flags);

struct DosTransferEnd {
    uint16_t bytes, flags;
};
struct DosTransferEnd __far c_dos_read(
    uint16_t handle, SegmentBytes buffer, uint16_t length, uint16_t initial_flags);
struct DosTransferEnd __far c_dos_write(uint16_t handle,
    SegmentBytes buffer,
    uint16_t length,
    uint8_t previous_low,
    uint16_t initial_flags);
struct DosOpenEnd __far c_dos_close(uint16_t handle, uint8_t previous_low, uint16_t initial_flags);
struct DosSizeEnd __far c_dos_size(uint16_t handle, uint16_t initial_flags);
struct DosMemoryEnd __far c_dos_alloc(
    uint16_t paragraphs, uint8_t previous_low, uint16_t initial_flags);
struct DosOpenEnd __far c_dos_free(uint16_t segment, uint16_t initial_flags);
struct DosOpenEnd __far c_bios_hide_cursor(uint8_t previous_low, uint16_t initial_flags);
/* Native console/menu stages carry only non-arithmetic firmware controls.
 * Full reply flags remain at real BIOS boundaries and optional raw gates. */
void __far set_console_attribute(uint16_t foreground, uint16_t background);
uint16_t __far write_console_text(SegmentBytes, uint16_t, uint16_t, struct ConsoleTextEnd *);
uint16_t __far write_console_line(
    SegmentBytes, uint16_t, uint16_t, uint16_t, struct ConsoleTextEnd *);
uint16_t __far draw_menu_box(
    SegmentBytes, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, struct ConsoleTextEnd *);
uint16_t __far write_menu_option(SegmentBytes, uint16_t, uint16_t, struct ConsoleTextEnd *);
uint16_t __far draw_menu_options(SegmentBytes, uint16_t, struct ConsoleTextEnd *);
struct DosOpenEnd __far console_position(uint16_t row, uint16_t column, uint16_t initial_flags);
struct ConsoleTextEnd __far console_text(
    SegmentBytes text, uint16_t initial_word, uint16_t initial_flags);
struct DosOpenEnd __far c_bios_cursor(uint16_t row, uint16_t column, uint16_t initial_flags);
struct ConsoleTextEnd __far c_bios_text(
    SegmentBytes text, uint16_t initial_word, uint16_t initial_flags);
struct DosPrintEnd __far c_dos_print(SegmentBytes scan_start,
    SegmentBytes terminator_start,
    SegmentBytes display_start,
    int backwards,
    uint16_t initial_flags);

struct PaletteUploadEnd {
    uint16_t value, flags;
};

enum PaletteOutput {
    PALETTE_VALUE = 1,
    PALETTE_COUNT = 2,
    PALETTE_SOURCE = 4,
    PALETTE_DESTINATION = 8,
    PALETTE_DATA_OUTPUT = 16,
    PALETTE_ATTRIBUTE_COUNT = 32,
    PALETTE_NO_OPERATION = 64
};

struct PaletteEnd {
    uint16_t value, flags, count, source, destination;
    uint8_t changed;
};
struct PaletteUploadEnd __far upload_palette(
    RgbPaletteSource colors, uint16_t start, uint16_t initial_flags);
uint16_t __far finish_palette(uint16_t flags);
struct PaletteEnd __far attribute_palette(
    SegmentBytes colors, uint16_t phase, uint16_t initial_flags);
struct PaletteEnd __far c_palette_tandy(
    SegmentBytes output, uint16_t phase, uint16_t initial_flags);
struct PaletteEnd __far c_palette_ega(SegmentBytes output, uint16_t phase, uint16_t initial_flags);
struct PaletteEnd __far c_palette_vga_ega(
    SegmentBytes output, uint16_t phase, uint16_t initial_flags);
struct PaletteEnd __far intensity_banks(
    SegmentBytes output, unsigned start, uint16_t initial_flags);
struct PaletteEnd __far c_palette_intensities(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct PaletteEnd __far c_palette_intensity_continue(
    SegmentBytes output, unsigned start, uint16_t initial_flags);
struct PaletteEnd __far fixed_banks(
    RgbPaletteSource colors, unsigned start, uint16_t initial_flags);
struct PaletteEnd __far c_palette_fixed_continue(
    RgbPaletteSource colors, unsigned start, uint16_t initial_flags);
struct PaletteEnd __far c_palette_vga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t initial_flags);
struct PaletteEnd __far c_palette_cga(
    uint16_t data_segment, uint16_t phase, uint16_t initial_flags);

struct GraphicsInitEnd {
    struct VideoModeSetEnd mode;
    uint8_t adapter_mode;
};

void __far graphics_memory_error(SegmentBytes data, uint16_t message);
struct DosMemoryEnd __far graphics_allocate(
    SegmentBytes data, uint16_t paragraphs, uint16_t error, uint16_t initial_flags);
struct GraphicsInitEnd __far initialize_graphics(
    SegmentBytes data, SegmentBytes output, unsigned mode, uint16_t initial_flags);
struct GraphicsInitEnd __far c_init_cga(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct GraphicsInitEnd __far c_init_tandy(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct GraphicsInitEnd __far c_init_ega(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct GraphicsInitEnd __far c_init_vga(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);

struct KeyboardReadEnd {
    uint16_t key, flags;
};

struct MenuAddEnd {
    uint16_t index, remaining, count;
};

struct ResourceLoadEnd {
    struct ResourceDecodeEnd decode;
    SegmentBytes buffer;
    uint16_t handle, flags;
    enum ResourceFormat format;
};
struct PlatformReply __far platform_keyboard(uint16_t function,
    uint16_t handle,
    uint16_t count,
    uint16_t offset,
    uint16_t data_segment,
    uint16_t extra_segment,
    uint16_t initial_flags);
struct KeyboardReadEnd __far c_bios_read_key(uint16_t initial_flags);
struct MenuAddEnd __far c_menu_add(SegmentBytes strings, uint16_t id, uint16_t name, int backwards);
struct ConsoleTextEnd __far menu_box(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint16_t height,
    uint16_t width,
    uint16_t flags);
struct ConsoleTextEnd __far c_menu_frame(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint16_t height,
    uint16_t width,
    uint16_t flags);
struct ConsoleTextEnd __far menu_option(SegmentBytes data, uint16_t option, uint16_t flags);
struct ConsoleTextEnd __far c_menu_option(SegmentBytes data, uint16_t option, uint16_t flags);
struct ConsoleTextEnd __far c_menu_options(SegmentBytes data, uint16_t flags);
void __far menu_fatal(SegmentBytes data, SegmentBytes strings, uint16_t message, uint16_t flags);
typedef struct ResourceDecodeEnd (*DecodeCallback)(uint16_t, SegmentBytes, SegmentBytes, uint16_t);
struct ResourceLoadEnd __far c_load_scene_resource(
    SegmentBytes data, SegmentBytes strings, uint16_t name, DecodeCallback decoder, uint16_t flags);

struct NearReserveEnd {
    uint16_t offset, paragraphs, flags;
    uint8_t resized;
};

struct EnvironmentEnd {
    struct NearReserveEnd reserve;
    uint16_t amount, source_end, string_end;
};

struct CrtMessageEnd {
    uint16_t message, value, length, flags;
    uint8_t written;
};

struct StackReturnEnd {
    uint16_t stack_pointer, instruction;
};
struct NearReserveEnd __far c_crt_reserve_near(
    uint16_t data_segment, uint16_t amount, uint16_t error_code, uint16_t flags);
struct EnvironmentEnd __far c_crt_prepare_environment(SegmentBytes data, uint16_t flags);
void __far c_crt_prepare_argv(SegmentBytes stack, uint16_t return_slot);
struct CrtMessageEnd __far crt_write_message(SegmentBytes data, uint16_t code, uint16_t flags);
struct StackReturnEnd __far crt_discard_argument(SegmentBytes stack, uint16_t return_slot);
uint16_t __far c_crt_lookup_message(uint16_t code);
struct CrtMessageEnd __far c_crt_write_message(SegmentBytes data, uint16_t code, uint16_t flags);
void __far c_crt_divide_error(void);
void __far c_crt_allocation_error(void);
uint16_t __far typed_resume_legacy(uint16_t stack_pointer, uint16_t instruction);

struct MouseSetupEnd {
    struct PlatformReply reply;
    uint8_t present;
};
struct PlatformReply __far platform_mouse(uint16_t function,
    uint16_t handle,
    uint16_t count,
    uint16_t offset,
    uint16_t data_segment,
    uint16_t extra_segment,
    uint16_t initial_flags);
struct DosOpenEnd __far set_interrupt_vector(
    unsigned vector, uint16_t offset, uint16_t segment, uint16_t flags);
struct MouseSetupEnd __far c_mouse_initialize(uint16_t button_state, uint16_t initial_flags);
struct MouseSetupEnd __far c_mouse_remove(uint16_t button_state,
    uint16_t callback_offset,
    uint16_t callback_segment,
    uint16_t initial_flags);
uint16_t __far c_mouse_callback_body(uint16_t events, uint16_t buttons, uint16_t x, uint16_t y);

struct HudDrawEnd {
    uint16_t source, destination;
    uint8_t cga, ega;
};

int __far draw_hud(SegmentBytes data, int clear, int backwards);
struct HudDrawEnd __far c_hud_energy(SegmentBytes data, int backwards);
struct HudDrawEnd __far c_hud_clear(SegmentBytes data, int backwards);

struct WarningAggregateEnd {
    uint16_t last_slot;
    uint8_t warning, sight, previous_mode, silenced, control;
};

struct SentryClockEnd {
    uint16_t value;
    uint8_t previous, carry;
};
struct WarningAggregateEnd __far c_aggregate_sentry_warning(uint16_t sentry_bank);
struct SentryClockEnd __far c_sentry_schedule_finish(void);

struct ScatterDispatchEnd {
    struct ScatterEnd scatter;
    uint8_t executed;
};
struct ScatterDispatchEnd __far c_scatter_dispatch(
    SegmentBytes data, uint16_t pixels, uint8_t color);
struct ScatterDispatchEnd __far c_scatter_highlight(SegmentBytes data);
struct ScatterDispatchEnd __far c_scatter_clear(SegmentBytes data);
struct ScatterDispatchEnd __far c_scatter_repeated(SegmentBytes data, uint8_t passes);
struct WorkspaceEnd __far c_scene_workspace(SegmentBytes data, SegmentBytes output, int backwards);
struct PaletteEnd __far select_scene_palette(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags);

struct AdapterProbeEnd {
    uint16_t value, index, flags, switches, index_mask;
    uint8_t switches_written;
};

struct AdapterNormalizeEnd {
    uint16_t value, flags;
    uint8_t page, queried;
};

struct AdapterDetectEnd {
    uint16_t value, index, port, flags;
    uint8_t port_written;
};
struct AdapterProbeEnd __far c_probe_display_combination(
    SegmentBytes data, uint16_t result, uint16_t flags);
struct AdapterProbeEnd __far c_probe_ega_configuration(
    SegmentBytes data, uint16_t result, uint8_t previous_low, uint16_t flags);
struct AdapterProbeEnd __far c_probe_tandy_columns(
    SegmentBytes data, uint16_t result, uint16_t previous_index, uint16_t flags);
struct AdapterNormalizeEnd __far c_probe_normalize(
    SegmentBytes data, uint16_t result, uint16_t flags);
struct AdapterDetectEnd __far c_detect_adapters(
    SegmentBytes data, uint16_t result, uint16_t previous_index, uint16_t flags);

struct TandySpanEnd {
    struct SpanFillEnd fill;
    uint16_t value, rows;
    uint8_t count_low_only, data_output;
};

uint8_t __far fill_tandy_polygon_spans(
    uint16_t data_segment, int backwards, struct TandySpanEnd *observation);
struct TandySpanEnd __far c_spans_tandy(uint16_t data_segment, int backwards);

struct SelectionIntervalEnd {
    struct VerticalAngleEnd vertical;
    uint16_t selected_index, model_index;
    uint8_t selected, prepared, accepted, columns;
};

#ifdef __ia16__
/* scripts/c_layout.py provides the controlled partial-redraw result at this ABI. */
_Static_assert(sizeof(struct SelectionIntervalEnd) == 26, "partial-redraw result size");
_Static_assert(
    offsetof(struct SelectionIntervalEnd, prepared) == 23, "partial-redraw prepared offset");
_Static_assert(
    offsetof(struct SelectionIntervalEnd, accepted) == 24, "partial-redraw accepted offset");
#endif

struct SentryPartialEnd {
    struct SelectionIntervalEnd interval;
    uint8_t mode;
};

struct SentryRetryEnd {
    struct StackReturnEnd resume;
    uint8_t previous;
};
struct SelectionIntervalEnd __far c_prepare_selection_interval(
    uint8_t selected, uint16_t observer_bank, uint16_t target_bank);
struct SentryPartialEnd __far c_sentry_partial_change(
    SegmentBytes data, uint8_t selected, uint16_t observer_bank, uint16_t target_bank);
struct StackReturnEnd __far c_sentry_abort(void);
struct SentryRetryEnd __far c_sentry_retry_after_redraw(uint16_t source_slot);
struct StackReturnEnd __far c_sentry_player_absorbed(void);

struct RayContact {
    uint16_t observer_index, position_index;
    uint8_t contact;
};

_Static_assert(sizeof(struct RayContact) == 6 && offsetof(struct RayContact, contact) == 4,
    "controlled native contact result layout");
struct RayContact __far trace_world_contact(
    SegmentBytes data, uint16_t observer_bank, uint16_t position_index);

struct WorldRayEnd {
    uint16_t value, observer_index, height_work, position_index, cell_bank, cell_offset, flags;
    uint8_t cell_sampled;
};
struct WorldRayEnd __far c_trace_world_ray(
    SegmentBytes data, uint16_t observer_bank, uint16_t position_index, uint16_t initial_flags);

struct KeyboardInstallEnd {
    struct DosOpenEnd vector;
    uint16_t offset, configuration_offset, configuration_segment;
    uint8_t configuration_available;
};

struct EgaCaptureEnd {
    uint16_t value, handle, flags, data_segment;
    uint8_t stage;
};
struct PlatformReply __far platform_system_bios(uint16_t function,
    uint16_t handle,
    uint16_t count,
    uint16_t offset,
    uint16_t data_segment,
    uint16_t extra_segment,
    uint16_t initial_flags);
struct KeyboardInstallEnd __far c_install_keyboard(uint8_t request_low, uint16_t flags);
struct EgaCaptureEnd __far c_capture_ega_frame(
    SegmentBytes filename, uint8_t request_low, uint16_t flags);

struct PrincipalPlacementEnd {
    uint16_t value, slot, lookup, bank, offset;
    uint8_t placed;
};

void __far c_place_initial_objects(SegmentBytes data);
struct PrincipalPlacementEnd __far c_place_principal_objects(SegmentBytes data);

struct WarningDispatchEnd {
    struct WarningEnd warning;
    uint8_t mode, executed;
};

struct PauseEnd {
    struct WarningDispatchEnd before, after;
    uint8_t action, entered, control, restored_mute, pause_flags;
};
struct WarningDispatchEnd __far draw_warning_bar(
    SegmentBytes data, int backwards, uint16_t idle_port);
struct PauseEnd __far c_pause_check(SegmentBytes data, int backwards, uint16_t idle_port);

struct LandscapeGenerateEnd {
    struct TerrainTransformEnd terrain;
    uint8_t final_shape, noise_sample, sampled;
};
struct LandscapeGenerateEnd __far c_generate_landscape(SegmentBytes data);

struct PreviewPrepareEnd {
    struct PrincipalPlacementEnd placement;
    uint16_t variant, color;
    uint8_t randomized, random_offset, mode, colored;
};

void __far c_build_preview(SegmentBytes data);
struct PreviewPrepareEnd __far c_prepare_preview(SegmentBytes data);

struct PreviewPopulateEnd {
    uint16_t value, validation_index, cell_bank, support_slot;
    uint8_t support_written, previous_generation_flags, accepted;
};
struct PreviewPopulateEnd __far c_populate_preview(SegmentBytes data, uint16_t validation_bank);

struct CursorEffects {
    uint16_t lookup_bank;
    uint8_t invoked, restoring, mode, executed;
};

struct CursorOperationEnd {
    union {
        struct CursorDrawEnd draw;
        struct CursorRestoreEnd restore;
    } result;

    uint8_t mode, invoked, restoring, initial_mode;
};

struct CursorEffects __far operate_cursor(SegmentBytes data,
    uint16_t object_bank,
    int restoring,
    uint16_t *flags,
    struct CursorOperationEnd *observation);
struct CursorEffects __far restore_action_cursor(SegmentBytes data,
    uint16_t object_bank,
    uint16_t *flags,
    struct CursorOperationEnd *observation);

struct HyperspaceResult {
    uint16_t robot;
    uint8_t model, found;
};

struct HyperspaceEnd {
    struct ObjectPlacementEnd placement;
    struct EnergyChange energy;
    struct ObjectDetachEnd detached;
    uint16_t robot;
    uint8_t stage;
};
struct CursorOperationEnd __far operate_software_cursor(
    SegmentBytes data, uint16_t object_bank, int restore);
struct CursorOperationEnd __far c_restore_action_cursor(SegmentBytes data, uint16_t object_bank);
struct HyperspaceEnd __far c_hyperspace(SegmentBytes data);

struct SightIndices {
    uint16_t observer, target;
};

_Static_assert(sizeof(struct SightIndices) == 4, "native sight register pair");
struct SightIndices __far test_sentry_visibility(
    SegmentBytes data, uint8_t filter, uint16_t source, uint16_t target, uint16_t object_record);

struct SentrySightEnd {
    struct WorldRayEnd ray;
    uint16_t lookup;
    uint8_t source, target, prepared;
};
struct SentrySightEnd __far c_sentry_test_sight(SegmentBytes data,
    uint16_t model_word,
    uint16_t source_index,
    uint16_t target_index,
    uint16_t object_record,
    uint16_t initial_flags);
void __far copy_video_bytes(
    SegmentBytes *destination, SegmentBytes *source, uint16_t count, int backwards);
void __far copy_video_words(
    SegmentBytes *destination, SegmentBytes *source, uint16_t count, int backwards);

enum RenderOutcome {
    RENDER_COMPLETE,
    RENDER_REJECTED,
    RENDER_INTERRUPTED
};

struct RenderEffects {
    uint16_t source, target;
    uint8_t source_updated, target_updated, backwards, data_output, interrupts_enabled;
    enum RenderOutcome outcome;
};

#ifdef __ia16__
/* Controlled gameplay hooks allocate the actual native return record. */
_Static_assert(sizeof(struct RenderEffects) == 10, "selection effect hook result size");
_Static_assert(offsetof(struct RenderEffects, backwards) == 6, "selection effect hook direction");
_Static_assert(offsetof(struct RenderEffects, outcome) == 9, "selection effect hook outcome");
#endif

struct RenderEnd {
    struct PolygonScanEnd polygon;
    uint8_t data_output;
};
struct RenderEnd __far c_draw_face(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, uint16_t initial_flags);
struct RenderEnd __far c_emit_quad(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, uint16_t flags);
struct RenderEnd __far c_draw_checker_quad(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct RenderEnd __far c_emit_cell_triangles(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, uint16_t flags);
struct RenderEnd __far c_cell_model(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    uint16_t flags);
struct RenderEnd __far c_cell_oriented_shape(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    uint16_t flags);
struct RenderEnd __far c_draw_cell(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_linked_stack(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_right(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_finish_terrain_near_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_view(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct RenderEffects __far scene_background(SegmentBytes, SegmentBytes, int, struct RenderEnd *);
struct RenderEffects __far scene_render(
    SegmentBytes, SegmentBytes, uint8_t, uint16_t, uint16_t, int, struct RenderEnd *);
struct RenderEnd __far title_screen_flow(SegmentBytes, SegmentBytes, uint8_t, uint16_t, int);
struct RenderEnd __far completion_landscape_flow(SegmentBytes, SegmentBytes, uint16_t, int);
struct GameFlowEnd __far preview_caption_flow(SegmentBytes, SegmentBytes, uint16_t, int);
struct RenderEnd __far overview_scene_flow(SegmentBytes, SegmentBytes, uint16_t, int);
struct RenderEnd __far c_scene_background(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
struct RenderEnd __far c_draw_view_model(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t record,
    uint16_t observer_bank,
    uint16_t flags);
struct RenderEnd __far c_draw_clipped_view_model(
    SegmentBytes data, SegmentBytes output, uint8_t model, uint16_t observer_bank, uint16_t flags);
struct RenderEnd __far c_draw_pedestal_view(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_bank,
    uint16_t flags);
struct RenderEnd __far c_scene_render(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t background,
    uint16_t camera,
    uint16_t flags);
struct RenderEnd __far c_title_screen(
    SegmentBytes data, SegmentBytes output, uint8_t completion, uint16_t flags);
struct RenderEnd __far c_completion_landscape(
    SegmentBytes data, SegmentBytes output, uint16_t flags);

struct KeyWaitEnd {
    uint16_t scan, remaining, flags;
    uint8_t character, leave, clear_high;
};
struct KeyWaitEnd __far c_wait_key(SegmentBytes data, SegmentBytes scans, uint16_t initial_flags);
struct KeyWaitEnd __far c_wait_fresh_key(SegmentBytes data, SegmentBytes scans, uint16_t flags);

struct MenuSelectionEnd {
    struct ConsoleTextEnd text;
    uint16_t adapter, selection, flags;
};

struct MenuChoice {
    uint16_t adapter, selection, flags; /* Full selected-key reply for graphics initialization. */
};
struct MenuChoice __far choose_adapter_menu(
    SegmentBytes, SegmentBytes, uint16_t, uint16_t, struct MenuSelectionEnd *);
struct MenuSelectionEnd __far c_menu_select(
    SegmentBytes data, SegmentBytes strings, uint16_t selection, uint16_t flags);
struct MenuAddEnd __far bootstrap_option(
    SegmentBytes strings, uint16_t id, uint16_t name, int backwards);
struct MenuSelectionEnd __far bootstrap_menu(
    SegmentBytes data, SegmentBytes strings, uint16_t initial, uint16_t flags);

struct DigitInputEnd {
    struct RenderEnd render;
    uint16_t count;
    uint8_t leave;
};
struct DigitInputEnd __far c_enter_decimal_digits(
    SegmentBytes data, SegmentBytes output, uint8_t maximum, uint16_t flags);

struct SoftwareScrollEnd {
    struct VideoRetraceEnd retrace;
    uint16_t source, destination, flags;
};

void __far scroll_cga_software(
    SegmentBytes data, uint16_t flags, struct SoftwareScrollEnd *observation);
struct SoftwareScrollEnd __far c_cga_scroll_software(SegmentBytes data, uint16_t flags);
struct RenderEnd __far c_render_selection(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t target_bank,
    uint16_t flags);

enum PlayerActionOutcome {
    PLAYER_ACTION_REJECTED,
    PLAYER_ACTION_HANDLED,
    PLAYER_ACTION_CHANGED,
    PLAYER_ACTION_REFRESH_STARTED
};

/* Native gameplay carries actor identities and actual execution effects,
 * independently of the original entries' arithmetic observations. */
struct ActorState {
    uint16_t source, target;
    uint8_t backwards, interrupts_enabled, data_output;
};

struct ActorAction {
    struct ActorState state;
    enum PlayerActionOutcome outcome;
};

struct PlayerActionEnd {
    struct RenderEnd render;
    enum PlayerActionOutcome outcome;
};
struct PlayerActionEnd __far c_execute_player_action(
    SegmentBytes data, uint16_t observer_bank, uint16_t target_bank, uint16_t initial_flags);

enum FlowRoute {
    FLOW_NONE,
    FLOW_PREVIEW_POPULATION,
    FLOW_BUILD_TITLE,
    FLOW_LANDSCAPE_PROMPT,
    FLOW_SECRET_PROMPT,
    FLOW_GENERATION_PALETTE,
    FLOW_TITLE,
    FLOW_POST_ACTION,
    FLOW_OVERVIEW,
    FLOW_PLAY_RENDER,
    FLOW_PLAY_ACTIONS,
    FLOW_PLAY_LOSS,
    FLOW_PLAY_WIN
};

struct GameFlowEnd {
    struct RenderEnd render;
    enum FlowRoute route;
    uint8_t leave;
};
struct GameFlowEnd __far c_preview_caption(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_startup_title(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_startup_build_title(
    SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_prompt_landscape(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_prompt_secret(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_wrong_secret(SegmentBytes data, SegmentBytes output, uint16_t flags);

enum SentryRoute {
    SENTRY_RETURN,
    SENTRY_FINISH,
    SENTRY_RETRY,
    SENTRY_PLAYER_LOST,
    SENTRY_ABORT,
    SENTRY_CREATION,
    SENTRY_ACTIVE,
    SENTRY_MEANIE,
    SENTRY_SCAN,
    SENTRY_EXPIRE,
    SENTRY_TURN,
    SENTRY_CHANGE
};

/* Gameplay outcomes are independent of redraw/stack-unwind routes. */
enum SentryOutcome {
    SENTRY_NO_CHANGE,
    SENTRY_TREE_CREATED,
    SENTRY_TARGET_FOUND,
    SENTRY_PLAYER_DRAINED,
    SENTRY_ENERGY_ABSORBED
};

/* Only state used by the next gameplay stage crosses the scheduler boundary. */
struct SentryTask {
    struct ActorState state;
    enum SentryRoute route;
    enum SentryOutcome outcome;
};

struct SentryState {
    uint16_t source, target, flags;
    uint8_t data_output;
};

struct SentryEnd {
    struct SentryState state;
    enum SentryRoute route;
    enum SentryOutcome outcome;
};
struct SentryEnd __far c_sentry_find_meanie_tree(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_absorb_unit(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_replant(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_find_stack_energy(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_schedule_start(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_active_dispatch(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_active_meanie(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_publish_change(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_publish_creation(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_meanie_turn(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_meanie_expire(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far sentry_track_visible_target(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);
struct SentryEnd __far c_sentry_scan_world(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    uint16_t initial_flags);

struct RenderEnd __far c_render_transition_strip(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t transition_bank,
    uint16_t flags);
struct RenderEnd __far c_end_effect(
    SegmentBytes data, SegmentBytes output, uint8_t passes, uint16_t flags);
struct RenderEnd __far c_overview_scene(SegmentBytes data, SegmentBytes output, uint16_t flags);
void __far c_update_cursor_input(SegmentBytes data,
    SegmentBytes scans,
    uint16_t observer_bank,
    uint16_t binding_bank,
    int backwards);
uint16_t __far c_timer_tick(SegmentBytes data, SegmentBytes scans, uint16_t flags);
struct SentryState __far c_run_sentry_schedule(
    SegmentBytes data, SegmentBytes output, uint16_t flags);

enum PlayerWaitOutcome {
    PLAYER_WAIT_CAMERA_TRANSITION,
    PLAYER_WAIT_SCENE_REFRESH
};

struct ActionWait {
    struct ActorState state;
    uint8_t source_updated, target_updated;
    enum PlayerWaitOutcome outcome;
};

struct PlayerWaitEnd {
    struct SentryState state;
    uint8_t source_updated, target_updated;
    enum PlayerWaitOutcome outcome;
};
struct PlayerWaitEnd __far c_player_wait_action(
    SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_preview_key_gate(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_begin_play_view(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_overview_gate(SegmentBytes data, SegmentBytes output, uint16_t flags);

struct BootstrapEnd {
    uint16_t status, flags;
};
struct BootstrapEnd __far c_game_bootstrap(SegmentBytes data, SegmentBytes strings, uint16_t flags);
struct GameFlowEnd __far c_play_render(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_play_loss(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_play_win(SegmentBytes data, SegmentBytes output, uint16_t flags);
struct GameFlowEnd __far c_play_actions(SegmentBytes data, SegmentBytes output, uint16_t flags);

/* Exact terminal observations for synthetic original-entry gates. */
struct RenderEnd __far c_draw_face_observed(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, uint16_t initial_flags);
struct RenderEnd __far c_emit_quad_observed(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, uint16_t flags);
struct RenderEnd __far c_draw_checker_quad_observed(
    SegmentBytes data, SegmentBytes output, uint16_t flags);
struct RenderEnd __far c_emit_cell_triangles_observed(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, uint16_t flags);
struct RenderEnd __far c_cell_model_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    uint16_t flags);
struct RenderEnd __far c_cell_oriented_shape_observed(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    uint16_t flags);
struct RenderEnd __far c_draw_cell_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_mesh_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_linked_stack_observed(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_right_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_row_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_finish_terrain_near_row_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags);
struct RenderEnd __far c_draw_terrain_view_observed(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags);
uint8_t __far show_warning(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningDispatchEnd *observation);
void __far check_game_pause(SegmentBytes data, int backwards, uint16_t idle_port);
uint8_t __far scatter_effect(
    SegmentBytes data, uint16_t count, uint8_t color, struct ScatterDispatchEnd *observation);
uint8_t __far scatter_repeated(
    SegmentBytes data, uint8_t passes, struct ScatterDispatchEnd *observation);
struct RenderEffects __far draw_face(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, int backwards);
struct RenderEffects __far emit_quad(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, int backwards);
struct RenderEffects __far draw_checker_quad(SegmentBytes data, SegmentBytes output, int backwards);
struct RenderEffects __far emit_cell_triangles(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, int backwards);
struct RenderEffects __far cell_model(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    int backwards);
struct RenderEffects __far cell_oriented_shape(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    int backwards);
struct RenderEffects __far draw_cell(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    int backwards);
struct RenderEffects __far draw_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards);
struct RenderEffects __far draw_linked_stack(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    int backwards);
struct RenderEffects __far draw_terrain_right(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards);
struct RenderEffects __far draw_terrain_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards);
struct RenderEffects __far finish_terrain_near_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards);
struct RenderEffects __far draw_terrain_view(SegmentBytes data, SegmentBytes output, int backwards);
struct RenderEffects __far project_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards,
    struct RenderEnd *observation);
struct RenderEffects __far draw_clipped_view_model(
    SegmentBytes data, SegmentBytes output, uint8_t model, uint16_t observer, int backwards);
struct RenderEffects __far render_selected_effects(
    SegmentBytes data, SegmentBytes output, uint16_t observer, uint16_t target, int backwards);
struct RenderEffects __far present_selected_effects(
    SegmentBytes, SegmentBytes, uint16_t, uint16_t, int, struct RenderEnd *);
struct RenderEffects __far render_end_effects(SegmentBytes, SegmentBytes, uint8_t, int);
struct RenderEffects __far present_end_effects(
    SegmentBytes, SegmentBytes, uint8_t, int, struct RenderEnd *);
struct ActorAction __far execute_actor_action(
    SegmentBytes, uint16_t, uint16_t, int, struct PlayerActionEnd *);

struct ActorState __far run_sentry_schedule(SegmentBytes, SegmentBytes, int, struct SentryState *);

struct ActionWait __far wait_player_action(SegmentBytes, SegmentBytes, int, struct PlayerWaitEnd *);

struct GameFlowEnd __far run_player_actions(SegmentBytes, SegmentBytes, uint16_t);
void __far detach_object(uint16_t, SegmentBytes, struct ObjectDetachEnd *);
int __far attach_object(uint16_t, SegmentBytes, struct ObjectAttachEnd *);
int __far place_object_flat(uint16_t, uint8_t, SegmentBytes, struct ObjectPlacementEnd *);
uint8_t __far collect_actions(SegmentBytes, int, uint16_t, uint16_t, struct ActionScanEnd *);
struct HyperspaceResult __far hyperspace_object(SegmentBytes, struct HyperspaceEnd *);
void __far arm_generation_preview(SegmentBytes, struct TerrainFilterEnd *);
void __far filter_terrain(uint8_t, uint8_t, SegmentBytes, struct TerrainFilterEnd *);
void __far smooth_terrain(uint8_t, SegmentBytes, struct TerrainFilterEnd *);
uint8_t __far transform_terrain(uint8_t, struct TerrainTransformEnd *);
uint8_t __far classify_terrain(uint8_t, uint8_t, struct TerrainClassifyEnd *);

/* Final camera-relative coefficients computed together by the model origin. */
struct ModelTransform {
    uint16_t heading, distance, height, azimuth_bias;
};

struct ModelOrigin {
    uint16_t angle, distance;
    struct ModelTransform transform;
};

struct MeshProjection {
    uint16_t next_vertex;
    uint8_t valid;
};

uint16_t __far planar_distance(const struct BearingGeometry *, struct PlanarDistanceEnd *);
struct ModelOrigin __far prepare_model_origin(uint16_t, uint16_t, struct ModelOriginEnd *);
struct MeshProjection __far project_mesh_vertices(
    const struct ModelTransform *, struct MeshVerticesEnd *);

struct LoadedResource {
    SegmentBytes buffer;
    uint16_t flags;
};

uint16_t __far resource_completion_flags(unsigned, uint16_t, uint16_t, uint16_t);
uint16_t __far decode_resource(
    uint16_t, SegmentBytes, SegmentBytes, uint16_t, unsigned, struct ResourceDecodeEnd *);
struct LoadedResource __far load_scene_resource(
    SegmentBytes, SegmentBytes, uint16_t, DecodeCallback, uint16_t, struct ResourceLoadEnd *);

struct PopulationCheck {
    uint8_t previous_flags, validation, accepted;
};

uint8_t __far generate_landscape(SegmentBytes, struct LandscapeGenerateEnd *);
struct PopulationCheck __far populate_preview(SegmentBytes, uint16_t, struct PreviewPopulateEnd *);

struct InputKey {
    uint16_t flags, remaining;
    uint8_t character, leave;
};

struct DecimalInput {
    uint16_t flags;
    uint8_t count, leave, data_output;
};
struct InputKey __far wait_key(SegmentBytes, SegmentBytes, uint16_t, struct KeyWaitEnd *);
struct InputKey __far wait_fresh_key(SegmentBytes, SegmentBytes, uint16_t, struct KeyWaitEnd *);
struct DecimalInput __far enter_decimal_digits(
    SegmentBytes, SegmentBytes, uint8_t, uint16_t, struct DigitInputEnd *);
uint16_t __far pack_decimal_digits(SegmentBytes, SegmentBytes, int, struct DecimalPackEnd *);

struct PaletteValue {
    uint16_t value, flags;
};

struct PaletteEffect {
    uint16_t value, flags;
    uint8_t operated, data_output, value_valid;
};

uint16_t __far darken_palette(struct PaletteFade *);
struct PaletteEffect __far apply_scene_palette(
    SegmentBytes, SegmentBytes, uint16_t, uint16_t, struct PaletteEnd *);
struct GameFlowEnd __far startup_title_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far startup_build_title_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far prompt_landscape_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far prompt_secret_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far wrong_secret_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far play_render_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far play_loss_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far play_win_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far preview_key_gate_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far begin_play_view_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
struct GameFlowEnd __far overview_gate_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed);
#ifdef __ia16__
_Static_assert(sizeof(struct InputKey) == 6, "native key hook size");
_Static_assert(
    offsetof(struct InputKey, remaining) == 2 && offsetof(struct InputKey, character) == 4,
    "native key hook fields");
_Static_assert(sizeof(struct DecimalInput) == 6, "native decimal hook size");
_Static_assert(
    offsetof(struct DecimalInput, count) == 2 && offsetof(struct DecimalInput, data_output) == 4,
    "native decimal hook fields");
_Static_assert(sizeof(struct PaletteEffect) == 8, "native palette hook size");
_Static_assert(offsetof(struct PaletteEffect, flags) == 2 &&
        offsetof(struct PaletteEffect, operated) == 4 &&
        offsetof(struct PaletteEffect, value_valid) == 6,
    "native palette hook fields");
#endif
#endif
