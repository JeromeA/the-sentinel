/* Mesh, terrain-cell, stack and landscape rendering with typed C interfaces. */
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#include "render_state.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

extern uint8_t terrain_triangle_orientations[];
extern union MaterialColors terrain_face_color_pairs[];

extern uint8_t model_vertex_ranges[], model_vertex_azimuth[], model_vertex_radius[];
extern union MeshHeight model_vertex_height[];

extern uint8_t model_visibility_flags[];
extern uint8_t model_face_ranges[];
extern union MeshFaceHeader model_face_flags_and_counts[];
extern union MaterialColors model_face_color_pairs[];
extern struct UnalignedWord model_face_index_pointers[];

/* Native geometry carries only traversal/publication effects. The optional
 * observation sink belongs to direct original-entry diagnostics. */
static void begin_geometry_observation(struct RenderEnd *observation)
{
    if (observation) {
        uint16_t flags = observation->polygon.flags;
        *observation = (struct RenderEnd){0};
        observation->polygon.flags = flags;
    }
}

static void prepare_geometry_observation(struct RenderEnd *observation, uint16_t flags)
{
    *observation = (struct RenderEnd){0};
    observation->polygon.flags = flags;
}

static struct RenderEffects finish_geometry(
    struct RenderEffects effects, struct RenderEnd *observation)
{
    if (observation) {
        observation->polygon.start_vertex = effects.source;
        observation->polygon.end_slot = effects.target;
        observation->data_output = effects.data_output;
    }
    return effects;
}

static void merge_geometry(struct RenderEffects *before,
    struct RenderEnd *observation,
    const struct RenderEffects *after,
    const struct RenderEnd *raw)
{
    if (after->source_updated) {
        before->source = after->source;
        before->source_updated = 1;
    }
    if (after->target_updated) {
        before->target = after->target;
        before->target_updated = 1;
    }
    before->backwards = after->backwards;
    before->data_output |= after->data_output;
    if (observation)
        render_merge(observation, raw);
}

static struct RenderEffects project_geometry_corner(
    uint8_t column, uint16_t source, uint16_t target, int backwards, struct RenderEnd *observation)
{
    struct TerrainCornerEnd corner = c_project_terrain_corner(column, source);
    struct RenderEffects effects = {0};
    effects.source = (source & 0xff00) | view_object_slot;
    effects.target = (target & 0xff00) | corner.column;
    effects.source_updated = effects.target_updated = 1;
    effects.backwards = backwards;
    if (observation)
        *observation =
            render_corner_observation(&corner, source, target, observation->polygon.flags);
    return effects;
}

static struct RenderEffects find_geometry_interval(
    uint16_t source, uint16_t target, int backwards, struct RenderEnd *observation)
{
    struct TerrainIntervalEnd interval = c_find_terrain_interval(source);
    struct RenderEffects effects = {0};
    effects.source = (source & 0xff00) | view_object_slot;
    effects.target = (target & 0xff00) | interval.column;
    effects.source_updated = effects.target_updated = 1;
    effects.backwards = backwards;
    if (observation) {
        uint16_t flags = observation->polygon.flags;
        *observation = render_corner_observation(&interval.corner, source, target, flags);
        observation->polygon.value = (observation->polygon.value & 0xff00) | interval.scan;
        observation->polygon.end_slot = effects.target;
        if (interval.boundary)
            observation->polygon.flags =
                (interval.boundary == 1 ? status(flags, 0, 8) : subtract8(flags, 32, 32)) | CF;
    }
    return effects;
}

static struct RenderEffects c_draw_face_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t previous_vertex,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_emit_quad_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t color_index,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_checker_quad_geometry(
    SegmentBytes data, SegmentBytes output, int backwards, struct RenderEnd *observation);
static struct RenderEffects c_emit_cell_triangles_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_cell_model_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_cell_oriented_shape_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_cell_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_mesh_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_linked_stack_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_terrain_right_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_terrain_row_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_finish_terrain_near_row_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation);
static struct RenderEffects c_draw_terrain_view_geometry(
    SegmentBytes data, SegmentBytes output, int backwards, struct RenderEnd *observation);

static struct RenderEffects c_draw_face_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t previous_vertex,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    struct PolygonCoverage coverage =
        build_polygon_coverage(data, output, previous_vertex, backwards, work);
    end.source = coverage.source;
    end.target = coverage.target;
    end.source_updated = coverage.source_updated;
    end.target_updated = 1;
    end.backwards = coverage.backwards;
    if (!coverage.visible) {
        end.outcome = RENDER_REJECTED;
        return finish_geometry(end, observation);
    }
    SpansCallback callback = segment_record(data, graphics_session).spans;
    if (!callback)
        return finish_geometry(end, observation);
    if (!observation)
        end.data_output = draw_video_spans(callback, data, coverage.backwards);
    else {
        struct TandySpanEnd spans = call_video_spans(callback, data, coverage.backwards);
        struct SpanFillEnd fill = spans.fill;
        work->value = spans.value;
        if (spans.count_low_only)
            fill.span_count = (work->span_count & 0xff00) | (uint8_t)fill.span_count;
        end.data_output = spans.data_output;
        if (fill.valid & 1) {
            work->span_count = fill.span_count;
            work->changed |= POLYGON_COUNT;
        }
        if (fill.valid & 2) {
            work->span_destination = fill.screen_row;
            work->changed |= POLYGON_DESTINATION;
        }
        if (fill.valid & 4) {
            work->vertex_offset = fill.span_offset;
            work->changed |= POLYGON_VERTEX_OFFSET;
        }
        if (fill.valid & 8) {
            work->error = fill.edge_work;
            work->changed |= POLYGON_ERROR;
        }
        work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        work->flags = status(work->flags, 0, 16);
    }
    end.source = end.target = 0;
    end.source_updated = end.target_updated = 1;
    return finish_geometry(end, observation);
}

static struct RenderEffects c_emit_quad_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t color_index,
    int backwards,
    struct RenderEnd *observation)
{
    polygon_packed_colors = terrain_face_color_pairs[color_index].raw;
    polygon_face_flags = 0;
    return c_draw_face_geometry(data, output, color_index, backwards, observation);
}

static struct RenderEffects c_draw_checker_quad_geometry(
    SegmentBytes data, SegmentBytes output, int backwards, struct RenderEnd *observation)
{
    uint8_t parity = (terrain_cell_x ^ terrain_scan_y) & 1;
    if (parity)
        terrain_checker_prng_snapshot = prng_state_low.middle;
    if (observation)
        observation->polygon.flags = status(observation->polygon.flags, parity, 8);
    return c_emit_quad_geometry(data, output, parity ? 8 : 0, backwards, observation);
}

static struct RenderEffects c_emit_cell_triangles_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    int backwards,
    struct RenderEnd *observation)
{
    uint16_t index = (color_bank & 0xff00) | shape;
    terrain_triangle_shape = shape;
    polygon_face_flags = 128;
    polygon_packed_colors = terrain_face_color_pairs[index].raw;
    struct RenderEffects first = c_draw_face_geometry(data, output, index, backwards, observation);
    if (first.data_output)
        output = data;
    index = (first.source & 0xff00) | (terrain_triangle_shape ^ 16);
    polygon_packed_colors = terrain_face_color_pairs[index].raw;
    polygon_face_flags |= 64;
    struct RenderEnd second_observation;
    if (observation) {
        second_observation = (struct RenderEnd){0};
        second_observation.polygon.flags =
            status(observation->polygon.flags, polygon_face_flags, 8);
    }
    struct RenderEffects second = c_draw_face_geometry(
        data, output, index, first.backwards, observation ? &second_observation : 0);
    merge_geometry(&first, observation, &second, observation ? &second_observation : 0);
    return finish_geometry(first, observation);
}

static struct RenderEffects c_cell_model_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    union TerrainCell cell = {.raw = terrain_cached_cells[cached_cell]};
    terrain_model_shape = cell.terrain.shape;
    if (observation) {
        work->value = (initial_word & 0xff00) | terrain_model_shape;
        work->flags = status(observation->polygon.flags, terrain_model_shape, 8);
    }
    if (!terrain_model_shape)
        return finish_geometry(end, observation);
    terrain_model_cell_x = terrain_cell_x;
    terrain_model_cell_y = terrain_scan_y;
    return c_draw_mesh_geometry(data,
        output,
        cached_cell,
        (object_bank & 0xff00) | 63,
        (object_bank & 0xff00) | 63,
        end.backwards,
        observation);
}

static struct RenderEffects c_cell_oriented_shape_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation)
{
    uint8_t rotated = (shape - terrain_face_rotation_bias) & 15;
    uint16_t index = (color_bank & 0xff00) | shape;
    if ((rotated & 3) == 1) {
        return c_emit_quad_geometry(data, output, index, backwards, observation);
    }
    terrain_triangle_orientation = terrain_triangle_orientations[(object_bank & 0xff00) | rotated];
    uint8_t side = ((shape & 4) >> 2) + terrain_triangle_view_bias;
    if (side < 2)
        shape |= 16;
    return c_emit_cell_triangles_geometry(data, output, shape, index, backwards, observation);
}

static struct RenderEffects c_draw_cell_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;

    uint8_t slot = ((terrain_cell_x | terrain_projected_row_bank) + terrain_corner_bias) &
        TERRAIN_PROJECTED_CACHE_MASK;
    uint16_t cache_index = (cell_bank & 0xff00) | slot;
    uint8_t shade;
    if (!terrain_shading_mode)
        shade = terrain_rows_remaining;
    else if (terrain_shading_mode == 1) {
        shade = terrain_corner_distance[cache_index];
        if (shade <= 16)
            shade = 16;
        else if (shade > 78)
            shade = 78;
        shade = (shade - 16) >> 1;
    } else {
        shade = terrain_corner_distance[cache_index];
        if (shade >= 32)
            shade = 31;
    }
    raster_shade_bank = (shade << 3) & 0xf0;
    uint16_t shaded_slot = ((uint16_t)raster_shade_bank << 8) | slot;
    uint16_t cell_flags = 0;
    if (observation)
        cell_flags = status(work->flags, terrain_model_render_flags & 128, 8);
    end.source = cache_index;
    end.target = object_bank;
    end.source_updated = 1;
    end.target_updated = 0;
    if (observation) {
        work->changed = POLYGON_VERTEX;
    }
    if (terrain_model_render_flags & 128) {
        struct RenderEnd model_observation;
        if (observation)
            prepare_geometry_observation(&model_observation, cell_flags);
        struct RenderEffects model = c_cell_model_geometry(data,
            output,
            cache_index,
            object_bank,
            shaded_slot,
            end.backwards,
            observation ? &model_observation : 0);
        merge_geometry(&end, observation, &model, observation ? &model_observation : 0);
        return finish_geometry(end, observation);
    }
    union TerrainCell cell = {.raw = terrain_cached_cells[cache_index]};
    uint16_t packed_cell = (shaded_slot & 0xff00) | cell.raw;
    if (observation)
        cell_flags = status(cell_flags, cell.raw, 8);
    if (!cell.raw) {
        if (observation) {
            work->value = packed_cell;
            work->flags = cell_flags;
        }
        return finish_geometry(end, observation);
    }
    if (cell.object.tag == TERRAIN_OBJECT_TAG) {
        struct RenderEnd checker_observation;
        if (observation)
            prepare_geometry_observation(&checker_observation, cell_flags);
        struct RenderEffects checker = c_draw_checker_quad_geometry(
            data, output, end.backwards, observation ? &checker_observation : 0);
        merge_geometry(&end, observation, &checker, observation ? &checker_observation : 0);
        if (end.data_output)
            output = data;
        /* Keep the sampled cell across checker drawing, but use its returned banks. */
        struct RenderEnd stack_observation;
        if (observation)
            prepare_geometry_observation(&stack_observation, work->flags);
        struct RenderEffects stack = c_draw_linked_stack_geometry(data,
            output,
            packed_cell,
            end.source,
            end.target,
            end.backwards,
            observation ? &stack_observation : 0);
        merge_geometry(&end, observation, &stack, observation ? &stack_observation : 0);
        return finish_geometry(end, observation);
    }
    uint8_t shape = cell.terrain.shape;
    if (observation)
        cell_flags = status(cell_flags, shape, 8);
    if (!shape) {
        struct RenderEnd checker_observation;
        if (observation)
            prepare_geometry_observation(&checker_observation, cell_flags);
        struct RenderEffects checker = c_draw_checker_quad_geometry(
            data, output, end.backwards, observation ? &checker_observation : 0);
        merge_geometry(&end, observation, &checker, observation ? &checker_observation : 0);
        return finish_geometry(end, observation);
    }
    if (shape == 12 || shape == 4) {
        /* Preserve the partial count output when triangle drawing leaves it unchanged. */
        if (observation) {
            work->span_count = (work->span_count & 0xff00) | shape;
        }
        terrain_triangle_orientation = terrain_triangle_view_bias & 1;
        struct RenderEnd triangles_observation;
        if (observation)
            prepare_geometry_observation(&triangles_observation, cell_flags);
        struct RenderEffects triangles = c_emit_cell_triangles_geometry(data,
            output,
            shape,
            cache_index,
            end.backwards,
            observation ? &triangles_observation : 0);
        merge_geometry(&end, observation, &triangles, observation ? &triangles_observation : 0);
    } else {
        struct RenderEnd oriented_observation;
        if (observation)
            prepare_geometry_observation(&oriented_observation, cell_flags);
        struct RenderEffects oriented = c_cell_oriented_shape_geometry(data,
            output,
            shape,
            cache_index,
            object_bank,
            end.backwards,
            observation ? &oriented_observation : 0);
        merge_geometry(&end, observation, &oriented, observation ? &oriented_observation : 0);
    }
    return finish_geometry(end, observation);
}

static struct RenderEffects c_draw_mesh_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    uint8_t pass = 0;

    end.source = observer_bank;
    end.target = drawn_index;
    if ((observer_bank & 0xff00) || object_record >= 64 || drawn_index >= 64 ||
        view_object_slot >= 64 || object_model_id[object_record] >= 10) {
        if (observation) {
            work->flags |= CF;
        }
        end.outcome = RENDER_REJECTED;
        return finish_geometry(end, observation);
    }
    struct ModelOrigin origin = prepare_model_origin(object_record, drawn_index, 0);
    model_dither_flags = (model_dither_flags >> 1) | ((model_planar_distance >> 8) >= 15 ? 128 : 0);
    {
        struct MeshVerticesEnd raw_projection;
        struct MeshProjection projected =
            project_mesh_vertices(&origin.transform, observation ? &raw_projection : 0);
        if (!projected.valid) {
            if (observation) {
                work->flags |= CF;
            }
            end.outcome = RENDER_REJECTED;
            return finish_geometry(end, observation);
        }
        end.source = (observer_bank & 0xff00) | view_object_slot;
        end.target = projected.next_vertex;
        if (observation) {
            work->span_destination = end.target;
            work->span_count = raw_projection.vertical.pitch;
            work->vertex_offset = bearing_table_index(&raw_projection.vertical.bearing);
        }
        end.source_updated = 1;
        end.target_updated = 1;
        if (observation) {
            work->changed = POLYGON_VERTEX | POLYGON_END_SLOT | POLYGON_DESTINATION |
                POLYGON_COUNT | POLYGON_VERTEX_OFFSET;
        }
    }
    polygon_face_flags = 64;
    model_face_pass = 0;
    model_face_pass_xor = 0;
    if (observation) {
        work->value = 0;
    }
    uint16_t mesh = (end.source & 0xff00) | model_mesh_id;
    end.source = mesh;
    uint8_t visibility = model_visibility_flags[mesh];
    if (visibility) {
        if (!(visibility & 1)) {
            model_relative_heading += 0xc000;
            if (model_relative_heading & 0x8000)
                pass = 2;
        } else if ((int16_t)model_relative_height > 0)
            pass = 2;
    }
    model_face_pass = pass;
    for (;;) {
        mesh = (end.source & 0xff00) | model_mesh_id;
        if (mesh >= 10 || model_face_ranges[mesh] >= model_face_range_ends[mesh] ||
            model_face_range_ends[mesh] > 160) {
            if (observation) {
                work->flags |= CF;
            }
            end.outcome = RENDER_REJECTED;
            return finish_geometry(end, observation);
        }
        model_element_end = model_face_range_ends[mesh];
        uint16_t face_index = (end.target & 0xff00) | model_face_ranges[mesh];
        do {
            union MeshFaceHeader face = {.raw = model_face_flags_and_counts[face_index].raw};
            model_element_index = (uint8_t)face_index;
            uint8_t face_pass = model_face_pass;
            uint16_t observer = (end.source & 0xff00) | face_pass;
            end.source = observer;
            if (!face_pass || !(face.fields.back_pass ^ (model_face_pass_xor >> 7))) {
                polygon_vertex_count = face.fields.vertex_count;
                polygon_packed_colors = model_face_color_pairs[face_index].raw;
                polygon_index_list = model_face_index_pointers[face_index].value;
                /* These are fallback outputs if face drawing leaves them unchanged. */
                end.target = face_index;
                if (observation) {
                    work->span_destination = face_index * 2;
                }
                {
                    struct RenderEnd face_observation;
                    if (observation)
                        prepare_geometry_observation(&face_observation, work->flags);
                    struct RenderEffects face = c_draw_face_geometry(
                        data, output, observer, end.backwards, observation ? &face_observation : 0);
                    merge_geometry(&end, observation, &face, observation ? &face_observation : 0);
                    if (end.data_output)
                        output = data;
                }
                /* Drawing can replace both the live face cursor and its published bank. */
                face_index = (end.target & 0xff00) | model_element_index;
            }
            face_index = (face_index & 0xff00) | (uint8_t)(face_index + 1);
            end.target = face_index;
            if (observation) {
                work->span_destination = end.target;
            }
        } while ((uint8_t)face_index < model_element_end);
        --model_face_pass;
        if ((int8_t)model_face_pass <= 0)
            break;
        model_face_pass_xor = 128;
    }
    if (observation) {
        work->value = (uint16_t)terrain_face_indices;
    }
    polygon_index_list = (uint16_t)terrain_face_indices;
    model_dither_flags &= 127;
    end.target = (end.target & 0xff00) | drawn_object_slot;
    if (observation) {
        work->span_destination = end.target;
        work->flags = status(work->flags, model_dither_flags, 8);
    }

    return finish_geometry(end, observation);
}

static struct RenderEffects c_draw_linked_stack_geometry(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    union TerrainCell cell = {.raw = stack_head};
    uint8_t head = cell.object.slot;
    uint8_t slot = head;
    uint8_t count = 0;
    end.source = observer_bank;
    end.target = object_bank;

    object_stack_head = head;
    while (object_stack_links[slot] >= OBJECT_LINK_SUPPORT) {
        slot = object_stack_links[slot] & OBJECT_SLOT_MASK;
        ++count;
    }
    object_stack_count = count;
    /* Draw supports at or below the viewer from the bottom upward.
     * An equal-height pedestal stays for the top-down pass. */
    if (count) {
        for (;;) {
            uint8_t view = view_object_slot;
            uint16_t viewing =
                ((uint16_t)object_height_integer[view] << 8) | object_height_fraction[view];
            uint16_t height =
                ((uint16_t)object_height_integer[slot] << 8) | object_height_fraction[slot];
            uint16_t difference = viewing - height;
            math_work_lo = (uint8_t)difference;
            end.source = (end.source & 0xff00) | view;
            if ((int16_t)difference < 0 ||
                (!difference && object_model_id[slot] == MODEL_SENTINEL_PEDESTAL))
                break;
            end.target = (end.target & 0xff00) | slot;
            if (observation) {
                work->span_destination = end.target;
            }
            {
                struct RenderEnd mesh_observation;
                if (observation)
                    prepare_geometry_observation(&mesh_observation, work->flags);
                struct RenderEffects mesh = c_draw_mesh_geometry(data,
                    output,
                    end.source,
                    end.target,
                    end.target,
                    end.backwards,
                    observation ? &mesh_observation : 0);
                merge_geometry(&end, observation, &mesh, observation ? &mesh_observation : 0);
                if (end.data_output)
                    output = data;
            }
            --object_stack_count;
            if (object_stack_count & 128)
                goto done;
            slot = head;
            {
                unsigned links = object_stack_count;
                while (links--)
                    slot = object_stack_links[slot] & OBJECT_SLOT_MASK;
            }
        }
    }
    /* Draw the remaining objects from the head down toward the viewer. */
    slot = head;
    do {
        end.target = (end.target & 0xff00) | slot;
        if (observation) {
            work->span_destination = end.target;
        }
        {
            struct RenderEnd mesh_observation;
            if (observation)
                prepare_geometry_observation(&mesh_observation, work->flags);
            struct RenderEffects mesh = c_draw_mesh_geometry(data,
                output,
                end.source,
                end.target,
                end.target,
                end.backwards,
                observation ? &mesh_observation : 0);
            merge_geometry(&end, observation, &mesh, observation ? &mesh_observation : 0);
            if (end.data_output)
                output = data;
        }
        slot = object_stack_links[(uint8_t)end.target] & OBJECT_SLOT_MASK;
        end.target = (end.target & 0xff00) | slot;
        if (observation) {
            work->span_destination = end.target;
        }
        --object_stack_count;
    } while (!(object_stack_count & 128));
done:
    if (observation) {
        work->flags = subtract8(work->flags, 0, 1) & ~CF;
    }

    return finish_geometry(end, observation);
}

static struct RenderEffects c_draw_terrain_right_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    if (observation) {
        work->value = initial_word;
    }
    end.source = observer_bank;
    end.target = position_index;

    uint8_t column = terrain_previous_row_right;
    for (;;) {
        --column;
        if (column & 128) {
            if (observation) {
                work->value = (work->value & 0xff00) | column;
                work->flags = (subtract8(work->flags, 0, 1) & ~CF) | (work->flags & CF);
            }
            return finish_geometry(end, observation);
        }
        terrain_cell_x = column;
        uint8_t left = terrain_previous_row_left;
        if (column < terrain_previous_row_left) {
            if (observation) {
                work->value = (work->value & 0xff00) | column;
                work->flags = subtract8(work->flags, column, left);
            }
            return finish_geometry(end, observation);
        }
        uint8_t viewer_column = view_grid_x;
        if (column < view_grid_x) {
            if (observation) {
                work->value = (work->value & 0xff00) | column;
                work->flags = subtract8(work->flags, column, viewer_column);
            }
            return finish_geometry(end, observation);
        }
        /* Every cell path replaces arithmetic flags and the value result. */
        {
            struct RenderEnd cell_observation;
            if (observation)
                prepare_geometry_observation(&cell_observation, work->flags);
            struct RenderEffects cell = c_draw_cell_geometry(data,
                output,
                end.source,
                end.target,
                end.backwards,
                observation ? &cell_observation : 0);
            merge_geometry(&end, observation, &cell, observation ? &cell_observation : 0);
            if (end.data_output)
                output = data;
        }
        column = terrain_cell_x;
    }

    return finish_geometry(end, observation);
}

static struct RenderEffects c_draw_terrain_row_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    if (observation) {
        work->value = initial_word;
    }
    end.source = observer_bank;
    end.target = position_index;

    terrain_cell_x = terrain_previous_row_left;
    while (terrain_cell_x < terrain_previous_row_right && terrain_cell_x < view_grid_x) {
        {
            struct RenderEnd cell_observation;
            if (observation)
                prepare_geometry_observation(&cell_observation, work->flags);
            struct RenderEffects cell = c_draw_cell_geometry(data,
                output,
                end.source,
                end.target,
                end.backwards,
                observation ? &cell_observation : 0);
            merge_geometry(&end, observation, &cell, observation ? &cell_observation : 0);
            if (end.data_output)
                output = data;
        }
        ++terrain_cell_x;
    }
    uint8_t column = terrain_cell_x;
    uint16_t row_word = observation ? (work->value & 0xff00) | column : 0;
    uint16_t boundary_flags =
        observation ? subtract8(work->flags, terrain_cell_x, terrain_previous_row_right) : 0;
    if (terrain_cell_x >= terrain_previous_row_right) {
        if (observation) {
            work->value = row_word;
            work->flags = boundary_flags;
        }
        return finish_geometry(end, observation);
    }
    {
        struct RenderEnd row_observation;
        if (observation)
            prepare_geometry_observation(&row_observation, boundary_flags);
        struct RenderEffects row = c_draw_terrain_right_geometry(data,
            output,
            row_word,
            end.source,
            end.target,
            end.backwards,
            observation ? &row_observation : 0);
        merge_geometry(&end, observation, &row, observation ? &row_observation : 0);
    }

    return finish_geometry(end, observation);
}

static struct RenderEffects c_finish_terrain_near_row_geometry(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    end.source = observer_bank;
    end.target = position_index;

    terrain_projected_row_bank = 0;
    ++terrain_scan_y;
    {
        struct RenderEnd corner_observation;
        if (observation)
            prepare_geometry_observation(&corner_observation, work->flags);
        struct RenderEffects corner = project_geometry_corner(view_grid_x,
            end.source,
            end.target,
            end.backwards,
            observation ? &corner_observation : 0);
        merge_geometry(&end, observation, &corner, observation ? &corner_observation : 0);
    }
    uint16_t slot = end.target;
    if (observation) {
        work->vertex_offset = slot;
    }
    uint8_t near_height = projected_y_high[slot];
    if (near_height < 2) {
        projected_y_high[slot + 1] = projected_y_high[slot];
        projected_y_low[slot + 1] = projected_y_low[slot];
        terrain_projected_row_bank = TERRAIN_PROJECTED_ROW_SIZE;
        --terrain_scan_y;
        projected_y_high[slot + TERRAIN_PROJECTED_ROW_SIZE] =
            projected_y_high[slot + (TERRAIN_PROJECTED_ROW_SIZE + 1)] = 255;
        projected_x_high[slot + TERRAIN_PROJECTED_ROW_SIZE] = projected_x_high[slot] = 255;
        projected_x_high[slot + (TERRAIN_PROJECTED_ROW_SIZE + 1)] = projected_x_high[slot + 1] = 20;
        terrain_cell_x = view_grid_x;
        {
            struct RenderEnd quad_observation;
            if (observation)
                prepare_geometry_observation(&quad_observation, work->flags);
            struct RenderEffects quad = c_draw_checker_quad_geometry(
                data, output, end.backwards, observation ? &quad_observation : 0);
            merge_geometry(&end, observation, &quad, observation ? &quad_observation : 0);
            if (end.data_output)
                output = data;
        }
    } else {
        if (observation) {
            work->value = (work->value & 0xff00) | near_height;
            work->flags = subtract8(work->flags, near_height, 2);
        }
    }
    if (observation) {
        work->flags &= ~CF;
    }

    return finish_geometry(end, observation);
}

static struct RenderEffects c_draw_terrain_view_geometry(
    SegmentBytes data, SegmentBytes output, int backwards, struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    begin_geometry_observation(observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;

    uint8_t view = view_object_slot;
    uint8_t phase = object_heading[view] + 32;
    uint8_t q = phase >> 6;
    terrain_rows_remaining = TERRAIN_VERTEX_SIDE;
    end.source = view;
    if (observation) {
        work->span_count &= 255;
    }
    end.target &= 255;
    view_grid_x_overflow = 0;
    view_pitch_reference = ((uint16_t)object_view_pitch[view] << 8) | 2;
    terrain_view_rotation_work = object_heading[view] - 10;
    polygon_index_list = (uint16_t)terrain_face_indices;
    terrain_view_rotation = phase;
    terrain_azimuth_bias =
        ((uint16_t)(uint8_t)((phase & 63) - 42) << 8) | (uint8_t)view_azimuth_reference;
    if (observation) {
        work->vertex_offset = q;
    }
    terrain_corner_bias = terrain_quadrant_corner_bias[q];
    terrain_face_rotation_bias = q * 4;
    terrain_triangle_view_bias = q - 2;
    switch (q) {
    case 0:
        view_grid_x = object_grid_x[view];
        view_grid_y = object_grid_y[view];
        break;
    case 1:
        view_grid_x = TERRAIN_LAST_CELL - object_grid_y[view];
        view_grid_y = object_grid_x[view];
        break;
    case 2:
        view_grid_x = TERRAIN_LAST_CELL - object_grid_x[view];
        view_grid_y = TERRAIN_LAST_CELL - object_grid_y[view];
        break;
    default:
        view_grid_x = object_grid_y[view];
        view_grid_y = TERRAIN_LAST_CELL - object_grid_x[view];
        break;
    }
    if (view_grid_x >= TERRAIN_VERTEX_SIDE) {
        view_grid_x_overflow =
            (int8_t)view_grid_x < 0 ? view_grid_x : view_grid_x - TERRAIN_LAST_VERTEX;
        view_grid_x = (int8_t)view_grid_x < 0 ? 0 : TERRAIN_LAST_VERTEX;
    }
    terrain_scan_y = TERRAIN_LAST_VERTEX;
    terrain_row_left = terrain_row_left_hint;
    terrain_projected_row_bank = 0;
    {
        struct RenderEnd interval_observation;
        if (observation)
            prepare_geometry_observation(&interval_observation, work->flags);
        struct RenderEffects interval = find_geometry_interval(
            end.source, end.target, end.backwards, observation ? &interval_observation : 0);
        merge_geometry(&end, observation, &interval, observation ? &interval_observation : 0);
    }
    terrain_row_left_hint = terrain_row_left;
    for (;;) {
        --terrain_rows_remaining;
        terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
        terrain_previous_row_left = terrain_row_left;
        terrain_previous_row_right = terrain_row_right;
        --terrain_scan_y;
        if (terrain_scan_y & 128) {
            if (observation) {
                work->flags &= ~CF;
            }
            return finish_geometry(end, observation);
        }
        if (terrain_scan_y == view_grid_y)
            break;
        {
            struct RenderEnd interval_observation;
            if (observation)
                prepare_geometry_observation(&interval_observation, work->flags);
            struct RenderEffects interval = find_geometry_interval(
                end.source, end.target, end.backwards, observation ? &interval_observation : 0);
            merge_geometry(&end, observation, &interval, observation ? &interval_observation : 0);
        }
        if (terrain_row_left > terrain_previous_row_left) {
            uint8_t column = terrain_row_left;
            do {
                --column;
                {
                    struct RenderEnd corner_observation;
                    if (observation)
                        prepare_geometry_observation(&corner_observation, work->flags);
                    struct RenderEffects corner = project_geometry_corner(column,
                        end.source,
                        end.target,
                        end.backwards,
                        observation ? &corner_observation : 0);
                    merge_geometry(
                        &end, observation, &corner, observation ? &corner_observation : 0);
                }
            } while (column != terrain_previous_row_left);
        } else if (terrain_row_left < terrain_previous_row_left) {
            uint8_t column = terrain_previous_row_left;
            terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
            ++terrain_scan_y;
            do {
                --column;
                {
                    struct RenderEnd corner_observation;
                    if (observation)
                        prepare_geometry_observation(&corner_observation, work->flags);
                    struct RenderEffects corner = project_geometry_corner(column,
                        end.source,
                        end.target,
                        end.backwards,
                        observation ? &corner_observation : 0);
                    merge_geometry(
                        &end, observation, &corner, observation ? &corner_observation : 0);
                }
            } while (column != terrain_row_left);
            terrain_previous_row_left = column;
            --terrain_scan_y;
            terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
        }
        if (terrain_row_right < terrain_previous_row_right) {
            uint8_t column = terrain_row_right;
            do {
                ++column;
                {
                    struct RenderEnd corner_observation;
                    if (observation)
                        prepare_geometry_observation(&corner_observation, work->flags);
                    struct RenderEffects corner = project_geometry_corner(column,
                        end.source,
                        end.target,
                        end.backwards,
                        observation ? &corner_observation : 0);
                    merge_geometry(
                        &end, observation, &corner, observation ? &corner_observation : 0);
                }
            } while (column != terrain_previous_row_right);
        } else if (terrain_row_right > terrain_previous_row_right) {
            uint8_t column = terrain_previous_row_right;
            terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
            ++terrain_scan_y;
            do {
                ++column;
                {
                    struct RenderEnd corner_observation;
                    if (observation)
                        prepare_geometry_observation(&corner_observation, work->flags);
                    struct RenderEffects corner = project_geometry_corner(column,
                        end.source,
                        end.target,
                        end.backwards,
                        observation ? &corner_observation : 0);
                    merge_geometry(
                        &end, observation, &corner, observation ? &corner_observation : 0);
                }
            } while (column != terrain_row_right);
            terrain_previous_row_right = column;
            --terrain_scan_y;
            terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
        }
        {
            struct RenderEnd row_observation;
            if (observation)
                prepare_geometry_observation(&row_observation, work->flags);
            struct RenderEffects row = c_draw_terrain_row_geometry(data,
                output,
                observation ? work->value : 0,
                end.source,
                end.target,
                end.backwards,
                observation ? &row_observation : 0);
            merge_geometry(&end, observation, &row, observation ? &row_observation : 0);
            if (end.data_output)
                output = data;
        }
        if ((terrain_query_flags & 128) && terrain_draw_stop) {
            end.outcome = RENDER_INTERRUPTED;
            if (observation) {
                work->flags |= CF;
            }
            return finish_geometry(end, observation);
        }
    }
    if ((uint8_t)(terrain_previous_row_left + 1) == view_grid_x)
        terrain_previous_row_right = terrain_previous_row_left + 1;
    else if ((uint8_t)(terrain_previous_row_right - 2) == view_grid_x)
        terrain_previous_row_left = terrain_previous_row_right - 1;
    else {
        {
            struct RenderEnd near_observation;
            if (observation)
                prepare_geometry_observation(&near_observation, work->flags);
            struct RenderEffects near = c_finish_terrain_near_row_geometry(data,
                output,
                end.source,
                end.target,
                end.backwards,
                observation ? &near_observation : 0);
            merge_geometry(&end, observation, &near, observation ? &near_observation : 0);
        }
        return finish_geometry(end, observation);
    }
    {
        struct RenderEnd corner_observation;
        if (observation)
            prepare_geometry_observation(&corner_observation, work->flags);
        struct RenderEffects corner = project_geometry_corner(terrain_previous_row_left,
            end.source,
            end.target,
            end.backwards,
            observation ? &corner_observation : 0);
        merge_geometry(&end, observation, &corner, observation ? &corner_observation : 0);
    }
    {
        struct RenderEnd corner_observation;
        if (observation)
            prepare_geometry_observation(&corner_observation, work->flags);
        struct RenderEffects corner = project_geometry_corner(terrain_previous_row_right,
            end.source,
            end.target,
            end.backwards,
            observation ? &corner_observation : 0);
        merge_geometry(&end, observation, &corner, observation ? &corner_observation : 0);
    }
    {
        struct RenderEnd row_observation;
        if (observation)
            prepare_geometry_observation(&row_observation, work->flags);
        struct RenderEffects row = c_draw_terrain_row_geometry(data,
            output,
            observation ? work->value : 0,
            end.source,
            end.target,
            end.backwards,
            observation ? &row_observation : 0);
        merge_geometry(&end, observation, &row, observation ? &row_observation : 0);
        if (end.data_output)
            output = data;
    }
    {
        struct RenderEnd near_observation;
        if (observation)
            prepare_geometry_observation(&near_observation, work->flags);
        struct RenderEffects near = c_finish_terrain_near_row_geometry(data,
            output,
            end.source,
            end.target,
            end.backwards,
            observation ? &near_observation : 0);
        merge_geometry(&end, observation, &near, observation ? &near_observation : 0);
    }

    return finish_geometry(end, observation);
}

struct RenderEffects __far draw_face(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, int backwards)
{
    return c_draw_face_geometry(data, output, previous_vertex, backwards, 0);
}

struct RenderEnd __far c_draw_face(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_face_geometry(data, output, previous_vertex, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_face_observed(
    SegmentBytes data, SegmentBytes output, uint16_t previous_vertex, uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_face_geometry(data, output, previous_vertex, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far emit_quad(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, int backwards)
{
    return c_emit_quad_geometry(data, output, color_index, backwards, 0);
}

struct RenderEnd __far c_emit_quad(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_emit_quad_geometry(data, output, color_index, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_emit_quad_observed(
    SegmentBytes data, SegmentBytes output, uint16_t color_index, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_emit_quad_geometry(data, output, color_index, !!(flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_checker_quad(SegmentBytes data, SegmentBytes output, int backwards)
{
    return c_draw_checker_quad_geometry(data, output, backwards, 0);
}

struct RenderEnd __far c_draw_checker_quad(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_draw_checker_quad_geometry(data, output, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_checker_quad_observed(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_draw_checker_quad_geometry(data, output, !!(flags & 0x400), &end);
    return end;
}

struct RenderEffects __far emit_cell_triangles(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, int backwards)
{
    return c_emit_cell_triangles_geometry(data, output, shape, color_bank, backwards, 0);
}

struct RenderEnd __far c_emit_cell_triangles(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_emit_cell_triangles_geometry(data, output, shape, color_bank, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_emit_cell_triangles_observed(
    SegmentBytes data, SegmentBytes output, uint8_t shape, uint16_t color_bank, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_emit_cell_triangles_geometry(data, output, shape, color_bank, !!(flags & 0x400), &end);
    return end;
}

struct RenderEffects __far cell_model(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    int backwards)
{
    return c_cell_model_geometry(
        data, output, cached_cell, object_bank, initial_word, backwards, 0);
}

struct RenderEnd __far c_cell_model(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_cell_model_geometry(
        data, output, cached_cell, object_bank, initial_word, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_cell_model_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t cached_cell,
    uint16_t object_bank,
    uint16_t initial_word,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_cell_model_geometry(
        data, output, cached_cell, object_bank, initial_word, !!(flags & 0x400), &end);
    return end;
}

struct RenderEffects __far cell_oriented_shape(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    int backwards)
{
    return c_cell_oriented_shape_geometry(
        data, output, shape, color_bank, object_bank, backwards, 0);
}

struct RenderEnd __far c_cell_oriented_shape(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_cell_oriented_shape_geometry(
        data, output, shape, color_bank, object_bank, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_cell_oriented_shape_observed(SegmentBytes data,
    SegmentBytes output,
    uint8_t shape,
    uint16_t color_bank,
    uint16_t object_bank,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    c_cell_oriented_shape_geometry(
        data, output, shape, color_bank, object_bank, !!(flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_cell(
    SegmentBytes data, SegmentBytes output, uint16_t cell_bank, uint16_t object_bank, int backwards)
{
    return c_draw_cell_geometry(data, output, cell_bank, object_bank, backwards, 0);
}

struct RenderEnd __far c_draw_cell(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_cell_geometry(data, output, cell_bank, object_bank, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_cell_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t cell_bank,
    uint16_t object_bank,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_cell_geometry(data, output, cell_bank, object_bank, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards)
{
    return c_draw_mesh_geometry(
        data, output, observer_bank, object_record, drawn_index, backwards, 0);
}

struct RenderEnd __far c_draw_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_mesh_geometry(
        data, output, observer_bank, object_record, drawn_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_mesh_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t object_record,
    uint16_t drawn_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_mesh_geometry(
        data, output, observer_bank, object_record, drawn_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_linked_stack(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    int backwards)
{
    return c_draw_linked_stack_geometry(
        data, output, stack_head, observer_bank, object_bank, backwards, 0);
}

struct RenderEnd __far c_draw_linked_stack(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_linked_stack_geometry(
        data, output, stack_head, observer_bank, object_bank, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_linked_stack_observed(SegmentBytes data,
    SegmentBytes output,
    uint8_t stack_head,
    uint16_t observer_bank,
    uint16_t object_bank,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_linked_stack_geometry(
        data, output, stack_head, observer_bank, object_bank, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_terrain_right(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards)
{
    return c_draw_terrain_right_geometry(
        data, output, initial_word, observer_bank, position_index, backwards, 0);
}

struct RenderEnd __far c_draw_terrain_right(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_right_geometry(
        data, output, initial_word, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_terrain_right_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_right_geometry(
        data, output, initial_word, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_terrain_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards)
{
    return c_draw_terrain_row_geometry(
        data, output, initial_word, observer_bank, position_index, backwards, 0);
}

struct RenderEnd __far c_draw_terrain_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_row_geometry(
        data, output, initial_word, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_terrain_row_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t initial_word,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_row_geometry(
        data, output, initial_word, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far finish_terrain_near_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    int backwards)
{
    return c_finish_terrain_near_row_geometry(
        data, output, observer_bank, position_index, backwards, 0);
}

struct RenderEnd __far c_finish_terrain_near_row(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_finish_terrain_near_row_geometry(
        data, output, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_finish_terrain_near_row_observed(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t position_index,
    uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_finish_terrain_near_row_geometry(
        data, output, observer_bank, position_index, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far draw_terrain_view(SegmentBytes data, SegmentBytes output, int backwards)
{
    return c_draw_terrain_view_geometry(data, output, backwards, 0);
}

struct RenderEnd __far c_draw_terrain_view(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_view_geometry(data, output, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEnd __far c_draw_terrain_view_observed(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    c_draw_terrain_view_geometry(data, output, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far project_mesh(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer,
    uint16_t object_record,
    uint16_t drawn_index,
    int backwards,
    struct RenderEnd *observation)
{
    return c_draw_mesh_geometry(
        data, output, observer, object_record, drawn_index, backwards, observation);
}
