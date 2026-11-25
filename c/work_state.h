#ifndef SENTINEL_WORK_STATE_H
#define SENTINEL_WORK_STATE_H
#include <stddef.h>
#include "register_compat.h"
#include "game_logic.h"
#include "ray_position.h"
#include "mesh_format.h"

/* Subsystem work records. Phase aliases share explicit union/byte views;
 * reset functions operate on each owner, independent of linker ordering. */
union __attribute__((packed)) WorkWord {
    uint16_t word;
    uint8_t bytes[2];
};

union __attribute__((packed)) SampleCorners {
    struct __attribute__((packed)) {
        uint8_t a;
        volatile uint8_t c, d;
        uint8_t b, wrap;
    } value;

    uint8_t bytes[5];
};

struct __attribute__((packed)) RasterWork {
    /* Edge-rasterizer workspace: near write pointer into the selected span-boundary buffer */
    union WorkWord span_buffer;
    /* Edge-rasterizer workspace: signed edge Y difference; the high-byte alias is used during clipping */
    union WorkWord delta_y;
    /* Edge-rasterizer workspace: absolute X difference used for interpolation and major-axis choice */
    union WorkWord abs_delta_x;
    /* Edge-rasterizer workspace: edge start Y word assembled from the split projection cache */
    uint16_t start_y;
    /* Edge-rasterizer workspace: edge start X word in polygon raster coordinates */
    uint16_t start_x;
    /* Edge-rasterizer workspace: edge end Y word; its low byte is reused as raster_row_cursor during span filling */
    union WorkWord end_y;
    /* Edge-rasterizer workspace: edge end X word in polygon raster coordinates */
    union WorkWord end_x;
    /* Horizontal-edge extent minimum, initialized to FFFFh and updated at 21D5h. When every edge is horizontal and the row is in the viewport, the polygon path emits one span with this left boundary. */
    uint16_t horizontal_x_min;
    /* Horizontal-edge extent maximum, initialized to zero and updated at 21D5h; paired with the minimum to emit the single-row polygon span when all edges are horizontal. */
    uint16_t horizontal_x_max;
    /* Edge-rasterizer workspace: scaled X increment used when long edges are subdivided */
    uint16_t scaled_delta_x;
    /* Near pointer to the current closed vertex-index loop. Mesh drawing switches it to a face loop and restores terrain_face_indices; polygon_project_index_loop and edge traversal dereference it through BP. */
    uint16_t index_list;
    /* Polygon rasterizer minimum row, initialized to FFh and updated by rasterized edges; the final validity test compares it with polygon_y_max. */
    uint8_t y_min;
    /* Polygon rasterizer maximum row, initialized to zero and used with polygon_y_min to bound adapter span filling. */
    uint8_t y_max;
    /* Edge workspace: sign of the original X difference, retained while interpolation uses its absolute magnitude. */
    uint8_t delta_x_sign;
    /* Long-edge subdivision scratch: polygon_subdivide_long_edge saves endpoint vertex BL before interpolation overwrites registers, then restores BL and reloads the projected endpoint after the subdivisions. This is an index into the projection caches, not a world coordinate. */
    uint8_t saved_vertex;
    /* Polygon workspace: count of non-repeated vertex indices, excluding the closing repeat; edge traversal stops at this count. */
    uint8_t vertex_count;
    /* Polygon workspace: two palette indices packed into low/high nibbles; adapter span writers extract them for solid/dithered filling. */
    union MaterialColors packed_colors;
    /* Face-construction mode: 00h builds a terrain quad, 40h uses the mesh index loop, 80h builds the first terrain triangle, C0h builds its opposite half. Vertex count is stored separately. */
    uint8_t face_flags;
    /* Long-edge subdivision countdown; the rasterizer consumes this while splitting an edge into shorter interpolation steps. */
    union WorkWord subdivision_count;
    /* Current vertex-loop edge index, saved before edge helpers and incremented until polygon_vertex_count. */
    uint8_t edge_index;
    /* Polygon workspace cleared at CS:20AB before each projected index loop. The full-level trace records 94,081 zero writes and no reads of this byte; its role beyond that reset remains unconfirmed. The neighboring byte at DS:1107 remains separate unexplained storage. */
    union WorkWord clip_work;
};
extern struct RasterWork raster_work;
_Static_assert(sizeof(struct RasterWork) == 34, "raster_work layout");
void __far reset_raster_work(SegmentBytes output);

struct __attribute__((packed)) ViewportWork {
    /* Polygon rasterizer X adjustment added before arithmetic halving into polygon_projected_x_words */
    volatile uint16_t x_bias;
    /* Viewport state: left pixel clip coordinate */
    volatile uint16_t x_min;
    /* Viewport state: right clip coordinate, computed as origin plus width */
    volatile uint16_t x_end;

    /* Unsigned raster width. Presets use full word values 320/112/128; CS:2901 instead shifts only AL three times and clears AH, yielding (8*column_count) modulo 256. Selected-view callers cap a render pass to 20 columns before this helper, keeping that width at or below 160 pixels. */
    struct __attribute__((packed)) {
        volatile uint16_t value;
        uint8_t retained[4];
    } pixel_width;

    /* Viewport state: projected-X clipping threshold */
    volatile uint8_t projected_x_limit;
    /* Viewport state: selected predefined viewport index */
    volatile uint8_t preset;
    /* Coarse wrapped-negative X rejection boundary = (viewport_projected_x_limit>>1) XOR 80h (CS:28C7/2901). CS:282E first flags X at/above the positive cutoff, then increments clipping status when its high byte lies below this boundary; this is a classification threshold, not the left pixel coordinate. */
    volatile union WorkWord negative_x_threshold;
    /* Low byte of the unsigned projected-X cutoff. Presets clear it; CS:2901 sets 80h for an odd width in eight-pixel columns and 00h for an even width. Combined with viewport_projected_x_limit, the threshold is columns*128 angular units = columns*8 pixels. */
    volatile union WorkWord x_fraction_limit;
    /* Viewport state: exclusive upper Y clip coordinate */
    uint8_t y_end;
    /* Viewport state: inclusive lower Y clip coordinate */
    uint8_t y_min;
};
extern struct ViewportWork viewport_work;
_Static_assert(sizeof(struct ViewportWork) == 20, "viewport_work layout");
void __far reset_viewport_work(SegmentBytes output);

struct __attribute__((packed)) TerrainWork {
    /* Terrain projection bias: setup derives a quadrant-relative angular word from view rotation; terrain_project_corner subtracts it from projection_angle before caching X. */
    struct __attribute__((packed)) {
        uint16_t value;
        uint8_t retained[4];
    } azimuth_bias;

    /* Terrain traversal camera X in the rotated viewing grid, clamped to 0..31 with excess retained separately. */
    uint8_t work_view_grid_x;
    /* Alternating terrain row-bank bit: zero or 20h, ORed into corner/cache indices and toggled as traversal advances rows. */
    uint8_t projected_row_bank;
    /* Corner-projection column saved by terrain_project_corner; sentry target search also reuses this byte for a candidate slot. */
    uint8_t corner_column;
    /* First-corner offset selected from terrain_quadrant_corner_bias; rotates cached quad/triangle assembly with the viewing quadrant. */
    uint8_t corner_bias;
    /* Phase alias for view-quadrant rotation and terrain_generation_mode. CS:2ACC treats zero as clear, any negative mode as PRNG fill, positive odd modes as noise-to-height conversion, and positive even modes as nibble swap. In CS:2B8E instead, bit 7 selects column filtering and bit 6 selects nonlinear filtering. These are two separate mode protocols sharing one byte. */
    uint8_t view_rotation;
    /* Terrain traversal camera Y in the rotated viewing grid; corner projection subtracts it from the row coordinate. */
    uint8_t work_view_grid_y;
    /* Destination index into split X/Y projection caches: terrain uses column OR row-bank bit, meshes start at 40h. */
    uint8_t work_projected_vertex_slot;
    /* Shared terrain-coordinate workspace: X cell/corner currently sampled by terrain drawing, generation, placement, or ray tests. */
    uint8_t scan_x;
    /* Terrain draw-loop cell X, retained while corner and shape helpers overwrite scan-coordinate scratch. */
    uint8_t cell_x;
    /* Shared terrain-coordinate workspace: Y row currently sampled by terrain drawing, generation, placement, or ray tests. */
    uint8_t scan_y;
    /* Terrain traversal: current visible row interval left endpoint; terrain_find_visible_row_interval updates it and terrain_draw_view saves it before advancing rows. */
    uint8_t row_left;
    /* Terrain traversal: current visible row interval right endpoint, paired with terrain_row_left. */
    uint8_t row_right;
    /* Shape/color-table index for a terrain triangle pair: low four bits retain the cell shape and bit 4 selects the first half color. The second half uses XOR 10h to select the opposite color. */
    uint8_t triangle_shape;
    /* Terrain traversal: previous row left endpoint, saved before row-bank exchange and compared against the next projected interval. */
    uint8_t previous_row_left;
    /* Terrain traversal: previous row right endpoint, saved alongside the previous left endpoint. */
    uint8_t previous_row_right;

    /* Per-cell split selector consumed by CS:204D: zero joins the top-left/bottom-right diagonal, one joins top-right/bottom-left. Ordinary slopes use terrain_triangle_orientations after view rotation; saddle codes 4/12 use terrain_triangle_view_bias&1. Both halves share the same selector. */
    struct __attribute__((packed)) {
        uint8_t value;
        uint8_t retained[4];
    } triangle_orientation;

    /* Signed quadrant bias = view_quadrant-2, hence FEh/FFh/00h/01h (CS:2444). For shape 4/12 its parity selects the diagonal. Other slopes add shape bit 2 to this byte and use the unsigned result <2 to select which color-half bank is emitted first (CS:2A0A). */
    uint8_t triangle_view_bias;
    /* Phase alias: terrain rendering stores quadrant*4 here for shape rotation. Generation calls it terrain_smoothing_flags: 00h selects four-sample averaging and 40h selects the descending in-place median-of-three filter. CS:2B8E ORs in the caller direction bit 80h to choose a column rather than a row; each smoothing stage runs two row-and-column passes. */
    uint8_t face_rotation_bias;
    /* Terrain setup stores object heading minus 0Ah here. The full-level trace records 811 byte writes and no reads; a functional consumer or other phase use remains unconfirmed. */
    uint8_t view_rotation_work;

    /* Held-direction cancellation flag. The timer-side helper writes zero while the stored direction matches an active directional input and 80h otherwise. When terrain_query_flags bit 7 allows interruption, terrain_draw_view tests this byte after a row and returns with carry on cancellation. The render controller clears it before a new transition. */
    struct __attribute__((packed)) {
        volatile uint8_t value;
        uint8_t retained[2];
    } draw_stop;

    /* Terrain-query control byte, with bit 7 enabling early cancellation of a view render. The transition controller sets this bit for eligible cursor/view modes around rendering, then clears it. Terrain row traversal consults terrain_draw_stop only while this bit is set; other bits remain unnamed. */
    volatile uint8_t query_flags;
    /* Persistent left-bound hint for the next terrain view. Setup copies it into terrain_row_left before finding the first visible interval, then stores the resulting left bound back here. Subsequent rows use separate current/previous interval fields. */
    uint8_t row_left_hint;
    /* Rendering policy byte: bit 7 switches terrain cells to reserved-slot model representations; title construction sets it temporarily, and scene/terrain paths also use indexed style aliases. */
    uint8_t model_render_flags;
};
extern struct TerrainWork terrain_work;
_Static_assert(sizeof(struct TerrainWork) == 35, "terrain_work layout");
void __far reset_terrain_work(SegmentBytes output);

struct __attribute__((packed)) RayWork {
    /* Visibility-ray workspace: target-X minus ray-origin X, normalized by left shifts at 2E66h; its signed high byte advances the split X accumulator at 2EAFh. */
    uint16_t step_x;
    /* Visibility-ray workspace: target-height minus origin height, normalized with the X/Y steps; both bytes advance the split height accumulator with carry. */
    uint16_t step_height;
    /* Visibility-ray workspace: target-Y minus ray-origin Y, normalized with the X/height steps; its signed high byte advances the split Y accumulator. */
    uint16_t step_y;
    /* Signed horizontal magnitude derived from the pitch direction table by the ray setup at 3118h. The heading helper multiplies it by the heading components to produce X/Y ray steps; height stepping uses the separately stored pitch component. */
    uint16_t horizontal_scale;
    /* Shared direction-angle word: cursor aiming combines horizontal cursor displacement with object heading here; direction-vector helpers and camera offset setup consume the low fractional/high angular bytes. */
    uint16_t heading_angle;

    /* Shared direction-angle word: cursor aiming combines vertical cursor displacement with object pitch; the common direction helper consumes its low fractional/high angular bytes. */
    struct __attribute__((packed)) {
        uint16_t value;
        uint8_t retained[2];
    } pitch_angle;

    /* Saved angular byte in 1B3Fh. Bit 6 chooses the complementary-component placement; bit 7 supplies component A sign; XOR of bits 6 and 7 supplies component B sign. Sign bits are inserted into bit 0 of the two encoded words. */
    struct __attribute__((packed)) {
        uint8_t value;
        uint8_t retained[2];
    } source_angle;

    /* Trigonometric approximation workspace. 1B3Fh scales the angular input using C900h and stores the high product here. Complementary component evaluation replaces it with C900h minus its old value; subsequent multiply/subtract paths produce the two encoded direction components. */
    uint16_t phase_work;
    /* Ray sample mode/status: height-cache building starts at zero; detailed ray stepping sets 80h. The sampler may set C0h, whose bit 6 the tracer uses to reject a contact. */
    uint8_t sample_mode_flags;
    /* Contact interpolation scratch: first holds the shape/contact case, then the selected edge starting height while the signed height difference is multiplied by the selected fractional coordinate. */
    uint8_t contact_work;
    /* Fractional height returned by the detailed terrain/object sampler: ray stepping clears it before sampling, object-specific paths fill it, and height comparisons combine it with the integer result in AL. */
    uint8_t sample_fraction;
    /* 24-bit ray X accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization uses the object cell center (fraction 8000h). */
    struct RayPosition position_x;
    /* 24-bit ray height accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization appends a zero low fractional byte to the object 8.8 height. */
    struct RayPosition position_height;
    /* 24-bit ray Y accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization uses the object cell center (fraction 8000h). */
    struct RayPosition position_y;
    /* Retained bytes belong to the original seven-byte Y ABI span. */
    uint8_t position_y_retained[4];
    uint16_t encoded_components[2];
    /* Ray sample status byte cleared before each sample; the model-3 path sets bit 7, which the tracer combines with action flags before testing the vertical step direction. */
    uint8_t sample_special_flags;
    /* Shared workspace byte: during ray targeting bit 7 enables the special target-test path. Player actions clear it; the sentry sight test sets it for the first ray and clears it for the second. Terrain text also addresses this area as indexed style/validation workspace, so ray naming is restricted to ray code. */
    uint8_t target_test_flags;
};
extern struct RayWork ray_work;
_Static_assert(sizeof(struct RayWork) == 41, "ray_work layout");
_Static_assert(offsetof(struct RayWork, position_x) == 22, "ray X offset");
_Static_assert(offsetof(struct RayWork, position_height) == 25, "ray height offset");
_Static_assert(offsetof(struct RayWork, position_y) == 28, "ray Y offset");
_Static_assert(offsetof(struct RayWork, position_y_retained) == 31, "ray Y retained offset");
_Static_assert(offsetof(struct RayWork, encoded_components) == 35, "ray encoded components offset");
void __far reset_ray_work(SegmentBytes output);

struct __attribute__((packed)) ProjectionWork {
    /* Model heading minus bearing angle, in 65536 units per turn; vertex azimuths are added to its high byte */
    uint16_t relative_heading;

    /* Bearing/distance workspace: smaller absolute axis selected for first-octant ratio lookup and distance correction. */
    struct __attribute__((packed)) {
        uint16_t value;
        uint8_t retained[2];
    } minor_axis;

    /* Bearing/distance workspace: larger absolute axis; planar distance adds the ratio-based correction to this word. */
    uint16_t major_axis;
    /* Shared projection workspace: planar distance result used by vertex and corner projection */
    uint16_t distance;
    /* Shared projection workspace: absolute X or vertical-angle input magnitude, depending on projection phase */
    union WorkWord abs_delta_x;
    /* Shared projection workspace: signed split-byte 8.8 height difference between drawn and viewing objects */
    uint16_t height_delta;
    /* Shared projection workspace: absolute Y magnitude used for bearing and distance lookup */
    uint16_t abs_delta_y;
    /* Full-turn bearing word: 0000h points along positive Y, 4000h along positive X, 8000h along negative Y, C000h along negative X. CS:1CB4/1CE8 restore these quadrants after the first-octant lookup; differences wrap modulo 65536. */
    uint16_t angle;

    /* Signed vertical raster coordinate = arithmetic_shift_right_4(vertical_angle - ((camera_pitch<<8)|20h)), with word subtraction wrapping first (CS:1D12). One pixel is 16 angular units, or 360/4096 degrees; the low-byte 20h pitch bias contributes two pixels. */
    struct __attribute__((packed)) {
        uint16_t value;
        uint8_t retained[5];
    } screen_y;

    /* Terrain-view setup stores (object_view_pitch << 8) | 2 here. The full-level trace records 811 word writes and no reads; a functional consumer remains unconfirmed. */
    uint16_t work_view_pitch_reference;
    /* Horizontal centering angle = origin_bearing - camera_heading + 0A00h (CS:174D). The 0A00h bias becomes 160 pixels after polygon conversion divides by 16, centering a 320-pixel view. Vertex projection adds this to each vertex bearing before storing split X bytes. */
    uint16_t azimuth_bias;
    /* Viewing heading reference; the object heading occupies its high byte */
    union WorkWord work_view_azimuth_reference;
    /* Saved camera-relative signed split-byte 8.8 model height, added to transformed vertex heights */
    uint16_t relative_height;
    /* Saved planar model-origin distance, used while projecting each mesh vertex */
    uint16_t planar_distance;
    /* Mesh selector copied from the drawn object; indexes vertex/face range and visibility-policy tables. */
    union WorkWord mesh_id;
    /* Shared mesh-loop index: current vertex while projecting, current face while drawing. */
    uint8_t element_index;
    /* Exclusive vertex/face range end loaded from the next mesh-range entry. */
    uint8_t element_end;

    /* Mesh face-order countdown: zero selects one stored-order pass; two then one select the ordered bit-7-clear/bit-7-set passes, using model_face_pass_xor to choose the group. */
    struct __attribute__((packed)) {
        uint8_t value;
        uint8_t retained[5];
    } face_pass;

    /* Shared direction workspace: sine/cosine lookup saves the angular byte here; bits 7/6 encode signs/quadrants, and signed multiplication toggles bit 7 for factor signs. */
    uint8_t direction_flags;
    /* Shared projection context: object slot used as the viewing origin for camera-relative model/terrain transforms; other phases may reuse this byte. */
    uint8_t work_view_object_slot;
    /* Shared projection context: drawn-object slot saved by model_prepare_camera_relative_origin before helper calls overwrite DI; recovered before returning from model drawing. */
    uint8_t work_drawn_object_slot;
    /* Upper byte of q=floor(65536*minor/major), so the distance-correction index is ceil(this_byte/2), 0..128. Equal axes, including zero/zero, set FFh without DIV at CS:1B33. Thus no divide-by-zero occurs when both input deltas are zero. */
    uint8_t axis_ratio;
    /* Shared projection/clipping status byte: corner projection records rejection bits/counts here; polygon edge processing clears it on success and tests it before accepting spans. */
    uint8_t clip_status;
    /* Shared projection workspace: sign of the original X/height delta before taking its absolute magnitude; quadrant restoration tests bit 7. */
    union WorkWord delta_x_sign;
    /* Shared projection workspace: sign of the original Y/planar delta; combined with the other sign to restore the full-turn bearing quadrant. */
    union WorkWord delta_y_sign;
    /* Unsigned sine-like component from the quarter-wave lookup, with component exchange determined by quadrant bits in projection_direction_flags. */
    uint8_t sin;
    /* Unsigned cosine-like component paired with projection_sin; signs are handled separately by projection_direction_flags. */
    uint8_t cos;
    /* Mesh face-group XOR mask: initialized to zero, then changed to 80h for the second ordered face pass. */
    uint8_t face_pass_xor;
    /* Mesh monochrome-distance latch: CS:18AA sets bit 7 when the planar-distance integer byte is at least 15, otherwise clears it. Span setup then chooses palette color A for both dither colors. This removes the second dither color at distance; it does not discard the mesh. CS:1953 clears the sign bit after drawing. */
    uint8_t dither_flags;
};
extern struct ProjectionWork projection_work;
_Static_assert(sizeof(struct ProjectionWork) == 58, "projection_work layout");
void __far reset_projection_work(SegmentBytes output);

struct __attribute__((packed)) GameWork {
    /* Shared multiplication workspace: magnitude word whose bit 0 encodes a sign toggle; math_multiply_encoded_signed combines it with projection_direction_flags and keeps the high product word. */
    uint16_t work_math_encoded_factor;
    /* Shared multiplication workspace: signed factor; math_multiply_encoded_signed takes its absolute value in place and folds its sign into projection_direction_flags. */
    uint16_t work_math_signed_factor;

    /* Visibility workspace: near pointer to a 256-byte-stride height row; initialized to row 31 for building and recomputed from the tested Y coordinate for ray visibility. */
    struct __attribute__((packed)) {
        uint16_t value;
        uint8_t retained[2];
    } work_occlusion_height_row_pointer;

    /* Rolling equality-bit history for generated reference bytes: placement rotates each comparison result through carry into this word. Preview entry requires low-byte bits 1..4 all set (mask 1Eh). */
    uint16_t validation_bits;
    /* Last successful free-slot selection, not an allocation counter or permanently reserved slot. CS:3681 scans slots 63 down to 0 and changes this byte only on success. Exhaustion returns carry with this byte stale; callers must honor carry. Failed attachment also leaves it pointing at the still-free candidate; later allocations may select that same slot again. */
    uint8_t work_allocated_object_slot;
    /* Current player-controlled robot slot, used as the viewing/action origin. Transfer and successful hyperspace change this slot. */
    uint8_t robot_slot;
    /* Placement-sampling scratch: a low-bit mask chosen from placement_count_masks for the current nonzero candidate count. Principal-object placement masks each random byte and rejects values outside the actual count before indexing placement_candidates. Other phase aliases remain scoped. */
    uint8_t work_placement_candidate_mask;
    /* Current energy-HUD tile column. hud_draw_player_energy clears it, hud_append_tile passes it to graphics_draw_status_tile and increments it, and the padding/frame loops stop at columns 29 and 38. */
    uint8_t work_hud_tile_column;
    /* Secondary-placement height limit derived from the principal placement threshold>>4. CS:3E95 accepts heights strictly below this byte, not <= it; an 8-bit retry counter relaxes the limit by one on wrap, before attempts 256,512,..., until it reaches 12. Player placement caps its initial limit at 6, while trees start from this landscape limit. Coordinates are sampled only in 0..30. */
    uint8_t object_height_limit;
    /* Principal-count cap from valid packed BCD: landscapes 0..99 use min(8,1+floor(number/10)); every landscape >=100 uses 8. CS:160C saves the original BCD and low PRNG word before changing BL to this cap. It is a cap, not the actual sentry count: CS:408E clamps the separately sampled count and placement may reduce it further. */
    volatile uint8_t difficulty;
    /* Noise amplitude multiplier used by CS:2B03: height=clamp(7+floor((noise-128)*scale/256),1,11), with noise-128 interpreted as a signed byte. Negative products round downward through the high byte of the negated full product. Landscape zero selects 24; other landscapes select prng_small_sum+14. Smoothing occurs both before and after this conversion. */
    uint8_t height_scale;
    /* Last warning pattern accepted by the active graphics backend. Each backend skips drawing when both current and previous patterns are zero; active patterns continue animating, and a transition to zero clears the old bar. */
    volatile uint8_t work_previous_sentry_warning_pattern;
    /* Player energy, initially 10. Create/hyperspace spend object_energy_values; absorption credits energy. The HUD displays this six-bit counter. */
    uint8_t energy;
    /* Scene composition model selector: nonnegative values request the view-object avatar; a negative selector omits that avatar before terrain drawing. */
    uint8_t work_scene_view_model_selector;
    /* Background/dither style index used by adapter background-pattern tables and scene text colors; style 3 also selects scattered highlight preparation. */
    uint8_t work_scene_background_style;
    /* Shared end-state flag: zero permits normal play; exhausted-player absorption writes 80h, while the render controller also copies the signed hyperspace outcome here. Timer/input paths and selection presentation test its sign to stop normal play or select the end transition. */
    volatile uint8_t end_flags;
    /* Aggregated sentry sight of the player robot. 45EFh prefers a result with bit 7 set over target-only 40h; the adapter renderers clear the second half of the bar only for 40h. Full sight permits absorption. */
    volatile union WorkWord work_warning_bar_sight;
    /* Landscape-zero test byte: CS:160C stores the BCD high pair when nonzero, otherwise the low pair. Thus nonzero is not a normalized boolean: landscape 0013 stores 13h and 1200 stores 12h. Only zero selects the built-in entry, fixed scale 24, one principal and starting robot square (8,17). Other setup uses its zero/nonzero status rather than its numeric magnitude. */
    volatile uint8_t nonzero;
    /* Shared seed for the generated validation ring. Odd-parity terrain quads copy the middle PRNG byte here at CS:29F8 (BX=8). CS:4120 reads it with BX=AAh to choose the first output index; CS:2E83 reads it with BX=FFh to locate the startup reference check. The full-level trace records 8,190 snapshots and seeds 38h/A5h for the startup/completion runs. */
    uint8_t work_generation_validation_seed;
    /* Allocation request latch: CS:367E stores AL before searching; CS:3681 retries the saved model without a new AL request. The latch changes even when the pool is exhausted. Only successful search copies it into object_model_id; no object slot is reserved until the later attachment changes its free link marker. */
    volatile uint8_t work_pending_object_model;
    /* Linked-stack draw workspace: number of additional linked objects; drawing decrements it through zero to negative while traversing the stack. */
    uint8_t work_object_stack_count;
    /* Linked-stack draw workspace: low-six-bit head slot saved so multi-pass stack drawing can restart traversal. */
    uint8_t work_object_stack_head;
    /* Actual principal-slot count, including Sentinel slot zero. Requested count is 1 on landscape zero, otherwise a 1..8 sample capped by landscape_difficulty. If no candidate remains as the height threshold falls to zero, CS:4172 replaces this count with the number already placed. Palette variant is (count-1)&7; initial trees are capped at 48-3*count, leaving room in the 64-slot pool. */
    uint8_t sentry_count;
    /* Bit 7 selects completion/code-generation mode. Winning hyperspace sets 80h; terrain filtering suppresses the startup return patch in that mode, and object placement starts the BCD/reference loop at A5h rather than AAh. Placement shifts the flag out and returns instead of entering the preview. */
    volatile uint8_t generation_flags;
    /* Signed horizontal screen offset for scene rendering. landscape_render_scene loads it from scene_lateral_offset and clears it on exit; mesh and terrain corner projection subtract this byte from the projected X coordinate. */
    union WorkWord work_view_lateral_offset;
    /* Lazy reference-pattern check latch in occlusion_trace_grid_row. A clear sign performs the generated-buffer test: four bytes must be 7Fh and the preceding fifth byte must differ; failure exits the row test. Success sets bit 7 so subsequent rays bypass the check. Workspace reset clears it. */
    uint8_t work_occlusion_reference_check_flags;
    /* Outcome byte read by the gameplay render controller: failed hyperspace due to insufficient energy sets 80h, while departing from the original Sentinel square sets C0h. Bit 7 enters an end sequence and bit 6 selects landscape completion and secret-code generation. */
    volatile uint8_t work_hyperspace_outcome_flags;
    /* Phase-dependent secret-code display workspace. Transfer records a supporting object model here; completion halves it into a BCD-pair loop index, advances the PRNG until the index is below four, then prints down to index zero. MODEL_SENTINEL_PEDESTAL (6) therefore prints four pairs (eight digits). Reset initializes 80h; the loop finishes with FFh. */
    uint8_t work_secret_code_pair_work;
    /* Scene composition flags: bit 7 selects the TFINAL image and forces background style 2; bit 5 adds the SKYBOX image overlay; bit 6 bypasses view-object and terrain drawing. Title/code setup uses C0h/A0h and clears this byte after rendering. */
    uint8_t work_scene_flags;
};
extern struct GameWork game_work;
_Static_assert(sizeof(struct GameWork) == 37, "game_work layout");
void __far reset_game_work(SegmentBytes output);

struct __attribute__((packed)) SentryWork {
    uint8_t ray_hit_state[3];
    /* Round-robin sentry scheduler index. The update at 42FAh processes this slot, decrements it and wraps a negative value to 7. The Sentinel is slot zero; absorption and tree replanting use the same index. */
    uint8_t work_current_sentry_slot;
    /* Working sight result initialized to zero at 453Ch. The two-ray test rotates target-hit carry bits into bits 6 and 7: target/body visibility and supporting-square visibility. Target selection caches it in sentry_target_sight; bit 7 permits absorption. */
    uint8_t target_sight_result;
    /* Warning-bar pattern-group offset: 0 clears, 4 selects the animated warning group, and the pause path selects solid group 8. Sentry warning aggregation writes this byte; all four adapter backends consume it. */
    volatile uint8_t warning_pattern;
    uint8_t work_meanie_turn_step[1];
    /* Original Sentinel square X, saved when principal slot zero is placed; independent of the current player robot. */
    uint8_t work_sentinel_start_square_x;
    /* Original Sentinel square Y; paired with sentinel_start_square_x for the return-square comparison after hyperspace. */
    uint8_t work_sentinel_start_square_y;
    /* Eight sentry target-acquisition warmup counters. CS:44BD reloads 120 only when zero; 119 clock updates bring it to ready value 1, which the clock holds. Absorbing a unit does not reload 120: subsequent actions are paced by the scan cooldown until sight loss clears this counter. Zero means no acquired target, not an immediately ready absorption. */
    volatile uint8_t absorb_delays[8];
    /* Eight independent rotation cooldowns. A turn at CS:449D loads 200; 199 clock updates bring it to 1. The no-target path accepts either 0 or 1, but a retained energy target bypasses rotation even if the cooldown is ready. The clock continues decreasing it while eligible, so rotation can resume immediately after losing a long-held target. */
    uint8_t rotation_delays[8];
    /* Eight scheduler cooldowns; values 0/1 permit a scan and 2..255 wait. CS:4336 reloads 4 (three decrements); Meanie turning loads 10 (nine), absorption loads 30 (29), and successful Meanie creation loads 50 (49). These cooldowns pace repeated energy removal after acquisition warmup has reached its held ready value. */
    uint8_t scan_delays[8];
    /* Object slot currently tested by sentry rays. 4531h copies the candidate DL here; the ray sampler marks hits against this slot, and sentry_absorb_energy_unit uses it to choose the player energy balance or an unattended object. */
    uint8_t test_target_slot;
    /* Angular comparison width for the sentry sight test: normal scheduling sets 14h and the secondary-target scan sets 28h. 4531h halves the selected width for its centered heading-window comparison; values are internal angular units. */
    uint8_t scan_angle_width;
    /* Shared comparison byte: terrain text calls the alternate PRNG entry only when this byte is below terrain_generation_saved_noise. Sentry sight setup stores saved_noise+1, or saved_noise+7 when its low nibble is 9, modulo 256. The full-level trace confirms 3,301 writes of 1; the eight completion digits compare 1 against 0, and both title banners compare 0 against 0. The purpose of this coupling remains unconfirmed. */
    uint8_t work_text_prng_compare_value;
    /* Eight sentry entries, indexed by the sentry scheduler slot 0..7: scan cursor; sentry_reset_scan_state resets the selected entry to 40h. The initialized image contains zeros before scene setup. */
    uint8_t scan_cursor[8];
    /* Eight sentry energy balances: absorption credits one unit; successful tree replanting spends one unit. Slot zero belongs to the Sentinel. */
    uint8_t replant_energy[8];
    /* Eight sentry entries, indexed by the sentry scheduler slot 0..7: selected target object slot; reset sentinel 80h. The initialized image contains zeros before scene setup. */
    uint8_t scan_target[8];
    /* Eight sentry entries, indexed by the sentry scheduler slot 0..7: scan pass state; reset value zero. The initialized image contains zeros before scene setup. */
    uint8_t scan_pass[8];
    /* Eight sentry entries, indexed by the sentry scheduler slot 0..7: active Meanie object slot; reset marker 80h. A suitable tree is converted into this model during the secondary-target scan. The initialized image contains zeros before scene setup. */
    uint8_t meanie_slots[8];
    /* Eight sentry selected object slots, stored from DL after target tests and read while an action timer is active; distinct from the scan cursor and Meanie-slot arrays. */
    uint8_t target_slots[8];
    /* Eight cached sentry sight results: bit 6 is target/body visibility (half warning for the player); bit 7 is supporting-square visibility (full warning and absorption allowed). */
    uint8_t target_sight[8];
    /* Eight per-sentry request flags. Failed secondary-target handling with scan-pass state below two sets bit 7. A later scheduler visit invokes the supported-object search at 47BDh, resets the scan cursor on success, or clears the request bit when no eligible target is found. */
    uint8_t stack_search_flags[8];
    /* Bit 7 gates sentry countdowns and the warning-bar timer callback. Scene reset initializes 80h, which suppresses those updates; accepting a player action clears bit 7. The game timer advances sentry clocks and draws the warning only when the sign is clear. */
    volatile uint8_t clock_gate_flags;
    /* Eight sentry signed heading increments. Placement chooses +20 or -20 at 4214h, and the scheduler adds the selected byte to the sentry heading at 449Dh; angles wrap modulo 256. */
    uint8_t rotation_steps[16];
};
extern struct SentryWork sentry_work;
_Static_assert(sizeof(struct SentryWork) == 117, "sentry_work layout");
void __far reset_sentry_work(SegmentBytes output);

struct __attribute__((packed)) InputWork {
    /* Camera/display transition mode 0..3, indexing camera_transition_steps and adapter source/delta tables; horizontal modes use heading, vertical modes use pitch. */
    volatile uint8_t transition_mode;
    /* Requested camera transition: cursor-limit handlers store modes 0..3 here; negative values indicate no pending mode in the input/controller paths. */
    volatile uint8_t transition_request;
    /* Last nonnegative camera_transition_request accepted by the input callback. During rendering, the held-direction helper compares this action against horizontal and vertical input slots to decide whether terrain drawing may continue or should stop. */
    volatile uint8_t work_held_direction_action;
    /* Bit 7 permits object changes to update the selected view while the player input loop waits. That loop clears it each visit, then sets it when the saved input-ready flag is negative and terrain_draw_stop is zero. The nested object update helper tests it before preparing the partial redraw. */
    volatile uint8_t work_partial_view_update_flags;
    /* Shared turn/input flag: the player wait starts at zero, idle mouse-cursor handling stores 40h, and the turn-around command shifts it left and requires the resulting sign bit before reversing heading. The render controller skips occlusion rebuilding while its sign is set. */
    volatile uint8_t work_turn_and_occlusion_flags;
    /* Bit 7 selects cursor mode. Space toggles it at 3D94h; mouse selection can set it at 3D6Fh/3EF6h. Nonnegative mode allows looking around; object actions below ACTION_HYPERSPACE require bit 7 at 3F5Ch. Preview entry clears it. */
    volatile uint8_t work_cursor_mode_flags;
    /* Pending view-refresh bit. Successful transfer/turn-around dispatch and hyperspace cleanup set 80h. The action loop tests and clears this bit to return for a redraw, or shifts it into carry while waiting to force the same exit. */
    volatile uint8_t view_refresh_flags;
    /* Bit 7 requests return to the title/landscape flow. Escape sets it in the key wait or gameplay input; player_poll_actions leaves its wait loop when set, and the preview gate checks it before starting play. Workspace reset clears it. */
    volatile union WorkWord work_restart_requested_flags;
    /* Pause latch: the F4 action path sets bit 7 before displaying the solid warning pattern and waiting for the continue action; it clears bit 7 on resume. game_timer_tick tests this flag after its sound updates and skips normal game processing while paused. */
    volatile uint8_t pause_flags;

    /* Signed remaining transition-step counter. Adapter transition loops decrement it before lookup, returning when it becomes negative; twice its nonnegative value indexes source-offset words. */
    struct __attribute__((packed)) {
        volatile uint8_t value;
        uint8_t retained[4];
    } work_video_transition_steps_left;

    /* Persistent horizontal aiming coordinate in two-pixel units, copied into software_cursor_x_pairs before display and updated by cursor-limit/transition logic. */
    uint8_t cursor_x_pairs;
    /* Persistent inverted vertical aiming coordinate, copied into software_cursor_inverted_y and adjusted by cursor/pitch-transition logic. */
    uint8_t cursor_inverted_y;
    /* Keyboard selection-cursor repeat phase. The input callback seeds 6Bh when leaving mouse handling, then shifts it left during cursor-mode directional input. Carry skips cursor movement on that update; the phase is separate from the Space debounce latch. */
    volatile uint8_t work_cursor_repeat_phase;

    /* Cursor restore-state sentinel and last saved step index. Zero means no saved cursor and makes restore return without touching video memory. Drawing all 21 steps increments to 21 at the Y terminator then decrements to 20; restore walks 20..0 and resets to zero. Thus this byte is not a count of saved pixels, and the initial zero deliberately does not restore record zero. */
    struct __attribute__((packed)) {
        uint8_t value;
        uint8_t retained[3];
    } saved_last_step;

    /* Number of steps requested by camera-transition setup and copied into video_transition_steps_left; pitch paths use eight steps. */
    volatile uint8_t work_video_transition_step_count;
    /* Cursor drawing is skipped when this byte is negative; the paired-view render path sets bit 7 while preparing a view and clears it before restoring cursor drawing. */
    volatile uint8_t hide_flags;
    /* Snapshot of input_request_ready_flags copied at 3E5Eh. The player wait loop tests its sign alongside terrain_draw_stop to decide whether to set the partial-view update flag. This is separate from the current readiness byte written by the timer input path. */
    volatile uint8_t ready_snapshot;
    /* Input handshake byte: player_poll_actions clears it while waiting; the input callback writes 80h when an action or view transition is ready. The foreground loop then handles that request, while the timer uses the sign to select new-input polling versus held-direction checks. */
    volatile uint8_t request_ready_flags;
    /* Four action bytes reset to 80h on every binding collection, even when only a directional prefix is scanned. Entries 0/2 are horizontal/vertical values, 1 is an object/game command, and 3 is auxiliary control. CS:3FC7 returns AL=slot0 AND slot2, making its sign a both-directions-inactive test: one active direction clears the sign. Held-direction code saves/restores the object command because its directional refresh otherwise clears slot 1. */
    volatile uint8_t action_slots[4];
};
extern struct InputWork input_work;
_Static_assert(sizeof(struct InputWork) == 30, "input_work layout");
void __far reset_input_work(SegmentBytes output);

struct __attribute__((packed)) SharedWork {
    uint8_t pass_counter;
    uint8_t loop_work;
};
extern struct SharedWork shared_work;
_Static_assert(sizeof(struct SharedWork) == 2, "shared_work layout");
void __far reset_shared_work(SegmentBytes output);

struct __attribute__((packed)) SampleWork {
    /* Shared terrain-generation scratch: height of the first sampled corner used by smoothing/shape comparisons; other phases reuse this byte. */
    /* Shared arithmetic scratch low byte: fractional phase input to sine/cosine lookup and low half of the signed work pair; other gameplay phases reuse this storage. */
    /* Shared arithmetic scratch high byte: byte multiplier operand and high half of the signed work pair; math_negate_work_pair negates the two-byte value in place. */
    /* Shared terrain-generation scratch: second sampled corner height used with terrain_corner_height_a to choose height/shape transitions. */
    /* Closing copy of height A: ray contact sampling stores the first corner at both DS:110A and 110E, allowing adjacent endpoint reads for all four edges of the A,C,D,B,A height ring. */
    union SampleCorners corners;
};
extern struct SampleWork sample_work;
_Static_assert(sizeof(struct SampleWork) == 5, "sample_work layout");
void __far reset_sample_work(SegmentBytes output);

struct __attribute__((packed)) LetterWork {
    /* Bit 7 selects direct text input rendering. The decimal-entry repaint helper sets it while drawing digits/backspaces and clears it afterward. The formatted-character dispatcher bypasses its normal printable-character wrapper when the bit is set. */
    uint8_t work_text_input_render_flags;
    /* Character/control byte saved on entry to game_draw_character. Printable characters select an eight-row font record. The terrain glyph loop later reuses this byte as a blank row by clearing it after its seven visible font rows. */
    uint8_t text_character_work;
    /* Eight bytes copied from the selected game_font_8x8 glyph after subtracting character 20h and multiplying by eight. game_draw_character consumes paired font bits to build terrain text cells; its preceding character/work byte supplies a cleared blank baseline. */
    uint8_t glyph_rows[9];
    /* Two terrain-text coordinates: column at +0 and row at +1. game_draw_character control bytes select which coordinate to set; glyph drawing copies them into terrain_scan_x/y and advances the column by four terrain cells after a character. */
    union WorkWord text_column;
    /* Bit 7 routes decimal digits to game_draw_character for terrain-built text instead of the regular screen-text formatter. The completion code emitter sets it before printing BCD pairs and clears it at the end; numeric zero is displayed as the letter O. */
    uint8_t work_number_terrain_text_flags;
};
extern struct LetterWork letter_work;
_Static_assert(sizeof(struct LetterWork) == 14, "letter_work layout");
void __far reset_letter_work(SegmentBytes output);

struct __attribute__((packed)) SelectionWork {
    /* Bit 7 interrupts randomized selected-view copying. Input readiness and model/view setup set it; new action/transition paths clear it. All four selection-reveal backends test it after copying a mask group and return early when set. */
    volatile uint8_t copy_interrupt_flags;
    /* Partial-view drawing policy: bit 7 selects drawing only the allocated model rather than the terrain view; reset after the presentation path. */
    volatile uint8_t model_draw_flags;
    /* Current partial-view start in eight-pixel columns; converted to an angular reference by shifting the word (column << 8) right once. */
    uint8_t view_column;
    /* Current render-pass width in eight-pixel columns. Selection setup caps its first pass at 20 and draws any remainder at a 160-pixel offset; full-view camera transitions can load 40. */
    uint8_t work_render_pass_columns;
    /* Selected interval width remaining: initialized with total width, reduced by the current pass, then reused as the second-pass width when nonzero. */
    uint8_t view_columns_left;
    /* Selected-update presentation policy: bit 7 requests cursor restore before rendering; bit 6 requests randomized reveal afterward. With bit 6 clear the direct strip copy runs; with it set, direct copy follows only when game_end_flags is nonnegative. Before direct copy the controller always restores and hides the cursor, then optionally redraws it if cursor mode is active. Cleanup clears this byte after either path; lower bits are not tested here. */
    volatile uint8_t presentation_flags;
    /* Randomized selection-copy column accumulator shared by CGA/Tandy/EGA/VGA reveal routines. Each iteration adds a PRNG-derived byte, masks to a power-of-two candidate range, and wraps candidates at the actual view width. It selects corresponding source and destination columns. */
    uint8_t copy_column_accumulator;
    /* EGA selection-reveal candidate mask. The backend derives it from placement_count_masks using the requested selection width, then applies it to the accumulated random column before comparing against ega_selection_column_limit. */
    union WorkWord work_ega_selection_column_mask;
    /* Inner randomized-copy iteration budget. CGA/Tandy/VGA initialize FFh, EGA initializes C8h. Each copied mask/pixel group decrements it; expiry consumes one selection_copy_iteration_budget unit and starts another batch unless a redraw interruption ends the operation. */
    volatile uint8_t copy_inner_countdown;
    /* Actual EGA selection width copied from selection_view_total_columns at backend entry. A masked random candidate at or above this limit has the width subtracted to wrap it into the selected source/destination columns. */
    uint8_t work_ega_selection_column_limit;

    /* Transient model extent override: 3603h takes the maximum of this byte and model_selection_extents, clears it, then projects that extent to an angular half-width. */
    struct __attribute__((packed)) {
        uint8_t value;
        uint8_t retained[2];
    } extent_override;
};
extern struct SelectionWork selection_work;
_Static_assert(sizeof(struct SelectionWork) == 14, "selection_work layout");
void __far reset_selection_work(SegmentBytes output);

struct __attribute__((packed)) SoundWork {
    /* Current melody duration in game-timer ticks: a C8h..FEh command supplies four times its offset from C8h; each note copies this byte into sound_trigger_countdown. */
    volatile uint8_t melody_duration;
    /* Sound dispatcher mode: 3 advances melody bytecode, 4 suppresses automatic triggers, 6 runs the effect-6 pitch ramp; other values schedule effect 4 with countdown 36h. */
    volatile uint8_t sequence_mode;
    /* Effect-6 ramp counter: triggers stop at AAh, delays are random 1..4 ticks, and each trigger advances the counter and patches effect-6 pitch from min(31,(counter>>2)+1) OR 32h. */
    volatile uint8_t ramp_index;

    /* Interrupt-driven sound countdown. game_timer_tick decrements it and clamps underflow to zero; zero allows the sequencer/effect-trigger dispatcher to advance. */
    struct __attribute__((packed)) {
        volatile uint8_t value;
        uint8_t retained[7];
    } trigger_countdown;

    /* Melody-stream byte offset: sound_advance_melody increments it before decoding a command or pitch. FFh is the stream-end/inactive value tested by startup and scene waits. */
    volatile uint8_t melody_cursor;
};
extern struct SoundWork sound_work;
_Static_assert(sizeof(struct SoundWork) == 12, "sound_work layout");
void __far reset_sound_work(SegmentBytes output);

struct __attribute__((packed)) RandomWork {
    /* 40-bit shift register stored low-word/middle-byte/high-word. Each output advances eight times: state=((state<<1)|((state>>19 XOR state>>32)&1)) modulo 2^40, then returns bits 32..39. Derived from the carry chain at 240Ch and checked against 128 native DOSBox bytes. */
    volatile struct GameRandom work_prng_state_low;
};
extern struct RandomWork random_work;
_Static_assert(sizeof(struct RandomWork) == 5, "random_work layout");
void __far reset_random_work(SegmentBytes output);
#define edge_span_buffer (raster_work.span_buffer.word)
#define ray_contact_fraction (raster_work.span_buffer.bytes[1])
#define edge_delta_y (raster_work.delta_y.word)
#define placement_rows_remaining (raster_work.delta_y.bytes[0])
#define edge_abs_delta_x (raster_work.abs_delta_x.word)
#define placement_columns_remaining (raster_work.abs_delta_x.bytes[0])
#define projection_x_bias (viewport_work.x_bias)
#define edge_start_y (raster_work.start_y)
#define edge_start_x (raster_work.start_x)
#define edge_end_y (raster_work.end_y.bytes[0])
#define raster_row_cursor (raster_work.end_y.bytes[0])
#define placement_region_y_origin (raster_work.end_y.bytes[0])
#define edge_end_x (raster_work.end_x.bytes[0])
#define placement_region_x_origin (raster_work.end_x.bytes[0])
#define terrain_azimuth_bias (terrain_work.azimuth_bias.value)
#define occlusion_ray_step_x (ray_work.step_x)
#define occlusion_ray_step_height (ray_work.step_height)
#define occlusion_ray_step_y (ray_work.step_y)
#define polygon_horizontal_x_min (raster_work.horizontal_x_min)
#define polygon_horizontal_x_max (raster_work.horizontal_x_max)
#define ray_horizontal_scale (ray_work.horizontal_scale)
#define viewport_x_min (viewport_work.x_min)
#define viewport_x_end (viewport_work.x_end)
#define edge_scaled_delta_x (raster_work.scaled_delta_x)
#define polygon_index_list (raster_work.index_list)
#define ray_heading_angle (ray_work.heading_angle)
#define ray_pitch_angle (ray_work.pitch_angle.value)
#define model_relative_heading (projection_work.relative_heading)
#define projection_minor_axis (projection_work.minor_axis.value)
#define viewport_pixel_width (viewport_work.pixel_width.value)
#define math_encoded_factor (game_work.work_math_encoded_factor)
#define math_signed_factor (game_work.work_math_signed_factor)
#define occlusion_height_row_pointer (game_work.work_occlusion_height_row_pointer.value)
#define projection_major_axis (projection_work.major_axis)
#define projection_distance (projection_work.distance)
#define projection_abs_delta_x (projection_work.abs_delta_x.word)
#define projection_delta_x_high (projection_work.abs_delta_x.bytes[1])
#define model_height_delta (projection_work.height_delta)
#define projection_abs_delta_y (projection_work.abs_delta_y)
#define projection_angle (projection_work.angle)
#define projection_screen_y (projection_work.screen_y.value)
#define trig_source_angle (ray_work.source_angle.value)
#define view_pitch_reference (projection_work.work_view_pitch_reference)
#define trig_phase_work (ray_work.phase_work)
#define model_azimuth_bias (projection_work.azimuth_bias)
#define view_azimuth_reference (projection_work.work_view_azimuth_reference.word)
#define view_heading_byte (projection_work.work_view_azimuth_reference.bytes[1])
#define model_relative_height (projection_work.relative_height)
#define model_planar_distance (projection_work.planar_distance)
#define landscape_validation_bits (game_work.validation_bits)
#define ray_target_hit_flags (sentry_work.ray_hit_state[0])
#define ray_intervening_hit_flags (sentry_work.ray_hit_state[1])
#define sentry_intervening_sight_bits (sentry_work.ray_hit_state[2])
#define current_sentry_slot (sentry_work.work_current_sentry_slot)
#define allocated_object_slot (game_work.work_allocated_object_slot)
#define view_grid_x (terrain_work.work_view_grid_x)
#define polygon_y_min (raster_work.y_min)
#define terrain_projected_row_bank (terrain_work.projected_row_bank)
#define polygon_y_max (raster_work.y_max)
#define placement_height_threshold (raster_work.y_max)
#define viewport_projected_x_limit (viewport_work.projected_x_limit)
#define camera_transition_mode (input_work.transition_mode)
#define camera_transition_request (input_work.transition_request)
#define edge_delta_x_sign (raster_work.delta_x_sign)
#define player_robot_slot (game_work.robot_slot)
#define edge_saved_vertex (raster_work.saved_vertex)
#define terrain_corner_column (terrain_work.corner_column)
#define viewport_preset (viewport_work.preset)
#define viewport_negative_x_threshold (viewport_work.negative_x_threshold.bytes[0])
#define sentry_target_sight_result (sentry_work.target_sight_result)
#define terrain_smoothing_passes (shared_work.pass_counter)
#define placement_retry_counter (shared_work.pass_counter)
#define scene_scatter_passes (shared_work.pass_counter)
#define hud_energy_remaining (shared_work.pass_counter)
#define polygon_vertex_count (raster_work.vertex_count)
#define polygon_packed_colors (raster_work.packed_colors.raw)
#define terrain_corner_bias (terrain_work.corner_bias)
#define terrain_view_rotation (terrain_work.view_rotation)
#define terrain_generation_mode (terrain_work.view_rotation)
#define view_grid_y (terrain_work.work_view_grid_y)
#define polygon_horizontal_edge_count (shared_work.loop_work)
#define scene_scatter_outer_count (shared_work.loop_work)
#define placement_remaining_trees (shared_work.loop_work)
#define sentry_sight_rays_left (shared_work.loop_work)
#define projected_vertex_slot (terrain_work.work_projected_vertex_slot)
#define terrain_scan_x (terrain_work.scan_x)
#define terrain_cell_x (terrain_work.cell_x)
#define terrain_scan_y (terrain_work.scan_y)
#define placement_candidate_mask (game_work.work_placement_candidate_mask)
#define viewport_x_fraction_limit (viewport_work.x_fraction_limit.bytes[0])
#define terrain_row_left (terrain_work.row_left)
#define terrain_row_right (terrain_work.row_right)
#define terrain_triangle_shape (terrain_work.triangle_shape)
#define terrain_previous_row_left (terrain_work.previous_row_left)
#define terrain_previous_row_right (terrain_work.previous_row_right)
#define polygon_face_flags (raster_work.face_flags)
#define edge_subdivision_count (raster_work.subdivision_count.bytes[0])
#define terrain_triangle_orientation (terrain_work.triangle_orientation.value)
#define polygon_edge_index (raster_work.edge_index)
#define terrain_triangle_view_bias (terrain_work.triangle_view_bias)
#define model_mesh_id (projection_work.mesh_id.bytes[0])
#define model_element_index (projection_work.element_index)
#define model_element_end (projection_work.element_end)
#define viewport_y_end (viewport_work.y_end)
#define viewport_y_min (viewport_work.y_min)
#define model_face_pass (projection_work.face_pass.value)
#define ray_sample_mode_flags (ray_work.sample_mode_flags)
#define terrain_face_rotation_bias (terrain_work.face_rotation_bias)
#define terrain_smoothing_flags (terrain_work.face_rotation_bias)
#define projection_direction_flags (projection_work.direction_flags)
#define polygon_clip_work (raster_work.clip_work.bytes[0])
#define view_object_slot (projection_work.work_view_object_slot)
#define drawn_object_slot (projection_work.work_drawn_object_slot)
#define terrain_corner_height_a (sample_work.corners.value.a)
#define ray_corner_heights (sample_work.corners.bytes)
#define math_work_lo (sample_work.corners.value.c)
#define terrain_corner_height_c (sample_work.corners.value.c)
#define math_work_hi (sample_work.corners.value.d)
#define terrain_corner_height_d (sample_work.corners.value.d)
#define terrain_corner_height_b (sample_work.corners.value.b)
#define ray_corner_height_wrap (sample_work.corners.value.wrap)
#define ray_contact_work (ray_work.contact_work)
#define ray_sample_fraction (ray_work.sample_fraction)
#define projection_axis_ratio (projection_work.axis_ratio)
#define projection_clip_status (projection_work.clip_status)
#define projection_delta_x_sign (projection_work.delta_x_sign.bytes[0])
#define projection_delta_y_sign (projection_work.delta_y_sign.bytes[0])
#define projection_sin (projection_work.sin)
#define projection_cos (projection_work.cos)
#define ray_position_x (ray_work.position_x)
#define ray_position_height (ray_work.position_height)
#define ray_position_y (ray_work.position_y)
#define terrain_view_rotation_work (terrain_work.view_rotation_work)
#define terrain_draw_stop (terrain_work.draw_stop.value)
#define trig_component_a (ray_work.encoded_components[0])
#define trig_component_b (ray_work.encoded_components[1])
#define sentry_warning_pattern (sentry_work.warning_pattern)
#define hud_tile_column (game_work.work_hud_tile_column)
#define landscape_object_height_limit (game_work.object_height_limit)
#define landscape_difficulty (game_work.difficulty)
#define landscape_height_scale (game_work.height_scale)
#define previous_sentry_warning_pattern (game_work.work_previous_sentry_warning_pattern)
#define player_energy (game_work.energy)
#define text_input_render_flags (letter_work.work_text_input_render_flags)
#define terrain_text_character_work (letter_work.text_character_work)
#define game_glyph_rows (letter_work.glyph_rows)
#define sentinel_start_square_x (sentry_work.work_sentinel_start_square_x)
#define sentinel_start_square_y (sentry_work.work_sentinel_start_square_y)
#define terrain_query_flags (terrain_work.query_flags)
#define scene_view_model_selector (game_work.work_scene_view_model_selector)
#define held_direction_action (input_work.work_held_direction_action)
#define selection_copy_interrupt_flags (selection_work.copy_interrupt_flags)
#define partial_view_update_flags (input_work.work_partial_view_update_flags)
#define sentry_absorb_delays (sentry_work.absorb_delays)
#define sentry_countdown_bytes ((volatile uint8_t *)&sentry_work + 9)
#define sentry_rotation_delays (sentry_work.rotation_delays)
#define sentry_scan_delays (sentry_work.scan_delays)
#define model_face_pass_xor (projection_work.face_pass_xor)
#define terrain_row_left_hint (terrain_work.row_left_hint)
#define terrain_text_column (letter_work.text_column.bytes[0])
#define terrain_text_row (letter_work.text_column.bytes[1])
#define terrain_model_render_flags (terrain_work.model_render_flags)
#define scene_background_style (game_work.work_scene_background_style)
#define selection_model_draw_flags (selection_work.model_draw_flags)
#define game_end_flags (game_work.end_flags)
#define warning_bar_sight (game_work.work_warning_bar_sight.bytes[0])
#define turn_and_occlusion_flags (input_work.work_turn_and_occlusion_flags)
#define landscape_nonzero (game_work.nonzero)
#define generation_validation_seed (game_work.work_generation_validation_seed)
#define terrain_checker_prng_snapshot (game_work.work_generation_validation_seed)
#define sentry_test_target_slot (sentry_work.test_target_slot)
#define cursor_mode_flags (input_work.work_cursor_mode_flags)
#define number_terrain_text_flags (letter_work.work_number_terrain_text_flags)
#define pending_object_model (game_work.work_pending_object_model)
#define player_action_code (game_work.work_pending_object_model)
#define selection_view_column (selection_work.view_column)
#define player_view_refresh_flags (input_work.view_refresh_flags)
#define restart_requested_flags (input_work.work_restart_requested_flags.bytes[0])
#define ray_sample_special_flags (ray_work.sample_special_flags)
#define sentry_scan_angle_width (sentry_work.scan_angle_width)
#define render_pass_columns (selection_work.work_render_pass_columns)
#define selection_view_columns_left (selection_work.view_columns_left)
#define object_stack_count (game_work.work_object_stack_count)
#define object_stack_head (game_work.work_object_stack_head)
#define selection_presentation_flags (selection_work.presentation_flags)
#define ray_target_test_flags (ray_work.target_test_flags)
#define landscape_sentry_count (game_work.sentry_count)
#define sound_melody_duration (sound_work.melody_duration)
#define landscape_generation_flags (game_work.generation_flags)
#define game_pause_flags (input_work.pause_flags)
#define sound_sequence_mode (sound_work.sequence_mode)
#define sound_ramp_index (sound_work.ramp_index)
#define text_prng_compare_value (sentry_work.work_text_prng_compare_value)
#define sentry_sight_noise_limit (sentry_work.work_text_prng_compare_value)
#define view_lateral_offset (game_work.work_view_lateral_offset.bytes[0])
#define model_dither_flags (projection_work.dither_flags)
#define prng_state_low (random_work.work_prng_state_low)
#define sentry_scan_cursor (sentry_work.scan_cursor)
#define sentry_replant_energy (sentry_work.replant_energy)
#define sentry_scan_target (sentry_work.scan_target)
#define sentry_scan_pass (sentry_work.scan_pass)
#define sentry_meanie_slots (sentry_work.meanie_slots)
#define sentry_target_slots (sentry_work.target_slots)
#define sentry_target_sight (sentry_work.target_sight)
#define sentry_stack_search_flags (sentry_work.stack_search_flags)
#define video_transition_steps_left (input_work.work_video_transition_steps_left.value)
#define camera_cursor_x_pairs (input_work.cursor_x_pairs)
#define camera_cursor_inverted_y (input_work.cursor_inverted_y)
#define cursor_repeat_phase (input_work.work_cursor_repeat_phase)
#define software_cursor_saved_last_step (input_work.saved_last_step.value)
#define selection_copy_column_accumulator (selection_work.copy_column_accumulator)
#define occlusion_reference_check_flags (game_work.work_occlusion_reference_check_flags)
#define ega_selection_column_mask (selection_work.work_ega_selection_column_mask.bytes[0])
#define video_transition_step_count (input_work.work_video_transition_step_count)
#define selection_copy_inner_countdown (selection_work.copy_inner_countdown)
#define ega_selection_column_limit (selection_work.work_ega_selection_column_limit)
#define selection_extent_override (selection_work.extent_override.value)
#define software_cursor_hide_flags (input_work.hide_flags)
#define input_ready_snapshot (input_work.ready_snapshot)
#define hyperspace_outcome_flags (game_work.work_hyperspace_outcome_flags)
#define sound_trigger_countdown (sound_work.trigger_countdown.value)
#define input_request_ready_flags (input_work.request_ready_flags)
#define sentry_clock_gate_flags (sentry_work.clock_gate_flags)
#define secret_code_pair_work (game_work.work_secret_code_pair_work)
#define sound_melody_cursor (sound_work.melody_cursor)
#define input_action_slots (input_work.action_slots)
#define scene_flags (game_work.work_scene_flags)
#define sentry_rotation_steps (sentry_work.rotation_steps)

#endif
