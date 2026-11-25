/* Polygon geometry is kept separate from the hardware span fillers. */
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* Convert indexed projected vertices into pixel X coordinates, classify edge
 * direction and subdivide long edges before the C edge walker fills the boundary
 * arrays. Direction and tie-breaking no longer require instruction patching. */

#define polygon_edge_end_x (((struct UnalignedWord *)&edge_end_x)->value)
#define polygon_edge_end_y (((struct UnalignedWord *)&edge_end_y)->value)

extern uint8_t terrain_triangle_vertex_delta[];

static uint16_t projected_y(unsigned vertex)
{
    return ((uint16_t)projected_y_high[vertex] << 8) | projected_y_low[vertex];
}

/* Only coverage, traversal indices and direction continue into native drawing.
 * The original entry observations are collected separately when requested. */
static void polygon_walk_edge(struct PolygonCoverage *coverage,
    struct PolygonScanEnd *observation,
    const struct RasterEdge *input,
    SegmentBytes y_spans,
    SegmentBytes x_spans)
{
    struct EdgeRasterEnd edge;
    if (observation)
        edge = (struct EdgeRasterEnd){0};
    int completed = rasterize_polygon_edge(input, y_spans, x_spans, observation ? &edge : 0);
    if (completed) {
        coverage->source = coverage->target = 0;
        coverage->source_updated = 1;
        coverage->backwards = 0;
    }
    if (!observation)
        return;
    if (!completed) {
        observation->value = (observation->value & 0xff00) | edge.value;
        observation->flags = edge.compare_subtract
            ? subtract8(observation->flags, edge.compare_left, edge.compare_right)
            : status(observation->flags, edge.compare_left, 8);
    } else {
        observation->span_count = edge.count;
        observation->value = edge.value;
        observation->span_destination = edge.destination;
        observation->start_vertex = observation->end_slot = 0;
        observation->changed |=
            POLYGON_COUNT | POLYGON_DESTINATION | POLYGON_VERTEX | POLYGON_END_SLOT;
        if (edge.x_major) {
            observation->error = edge.error;
            observation->vertex_offset = edge.remaining_y;
            observation->changed |= POLYGON_ERROR | POLYGON_VERTEX_OFFSET;
        }
        observation->flags = status(observation->flags, 0, 16) & ~0x400;
    }
}

static void subdivide_polygon_edge(SegmentBytes y_spans,
    SegmentBytes x_spans,
    uint8_t start_vertex,
    uint8_t end_vertex,
    struct PolygonCoverage *coverage,
    struct PolygonScanEnd *observation)
{
    uint8_t start = start_vertex;
    uint8_t end = end_vertex;
    unsigned pieces = 1;
    uint16_t difference = polygon_projected_x_words[end] - polygon_projected_x_words[start];
    uint16_t magnitude = (int16_t)difference < 0 ? -difference : difference;
    edge_saved_vertex = start;
    edge_delta_x_sign = difference >> 8;
    while (magnitude > 254 || edge_delta_y > 254) {
        edge_delta_y >>= 1;
        magnitude >>= 1;
        pieces *= 2;
    }
    edge_scaled_delta_x = (int16_t)difference < 0 ? -magnitude : magnitude;
    edge_start_x = polygon_projected_x_words[end];
    edge_start_y = projected_y(end);
    edge_subdivision_count = (uint8_t)(pieces - 1);
    coverage->source = 0;
    coverage->source_updated = 1;
    if (observation) {
        observation->start_vertex = 0;
        observation->changed |= POLYGON_VERTEX;
    }
    while (edge_subdivision_count) {
        struct RasterEdge piece;
        piece.end_y = edge_start_y;
        polygon_edge_end_y = piece.end_y;
        piece.start_y = edge_start_y - edge_delta_y;
        edge_start_y = piece.start_y;
        piece.end_x = edge_start_x;
        polygon_edge_end_x = piece.end_x;
        piece.start_x = edge_start_x - edge_scaled_delta_x;
        edge_start_x = piece.start_x;
        piece.delta_y = edge_delta_y;
        polygon_walk_edge(coverage, observation, &piece, y_spans, x_spans);
        --edge_subdivision_count;
    }
    struct RasterEdge remainder;
    remainder.end_y = edge_start_y;
    polygon_edge_end_y = remainder.end_y;
    remainder.end_x = edge_start_x;
    polygon_edge_end_x = remainder.end_x;
    coverage->source = (coverage->source & 0xff00) | edge_saved_vertex;
    if (observation) {
        observation->start_vertex = coverage->source;
        observation->vertex_offset = coverage->source * 2;
        observation->changed |= POLYGON_VERTEX_OFFSET;
    }
    remainder.start_y = projected_y(coverage->source);
    edge_start_y = remainder.start_y;
    remainder.start_x = polygon_projected_x_words[coverage->source];
    edge_start_x = remainder.start_x;
    remainder.delta_y = polygon_edge_end_y - edge_start_y;
    edge_delta_y = remainder.delta_y;
    if (observation)
        observation->value = edge_delta_y;
    polygon_walk_edge(coverage, observation, &remainder, y_spans, x_spans);
}

struct PolygonScanEnd __far c_polygon_subdivide(SegmentBytes y_spans,
    SegmentBytes x_spans,
    uint8_t start_vertex,
    uint8_t end_vertex,
    uint16_t initial_work,
    uint16_t initial_flags)
{
    struct PolygonScanEnd walk = {0};
    struct PolygonCoverage coverage = {0};
    walk.value = initial_work;
    walk.flags = initial_flags;
    coverage.backwards = !!(initial_flags & 0x400);
    subdivide_polygon_edge(y_spans, x_spans, start_vertex, end_vertex, &coverage, &walk);
    return walk;
}

static struct PolygonCoverage project_and_scan_polygon(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t previous_vertex,
    int backwards,
    struct PolygonScanEnd *observation)
{
    struct PolygonCoverage coverage = {0};
    coverage.source = previous_vertex;
    coverage.backwards = backwards;
    if (observation) {
        observation->start_vertex = previous_vertex;
        observation->changed = POLYGON_END_SLOT | POLYGON_DESTINATION;
    }
    polygon_clip_work = 0;
    int vertex;
    for (vertex = polygon_vertex_count; vertex >= 0; --vertex) {
        uint8_t slot = data[polygon_index_list + vertex];
        uint16_t value = ((uint16_t)projected_x_high[slot] << 8) | projected_x_low[slot];
        polygon_projected_x_words[slot] = (int16_t)(uint16_t)(value + projection_x_bias) >> 4;
    }
    polygon_y_max = polygon_horizontal_edge_count = 0;
    polygon_horizontal_x_max = 0;
    polygon_y_min = projection_clip_status = 255;
    polygon_horizontal_x_min = 0xffff;
    if (observation)
        observation->end_slot = observation->span_destination = 0;
    unsigned edge;
    for (edge = 0; edge < polygon_vertex_count; ++edge) {
        uint8_t start = data[polygon_index_list + edge];
        uint8_t end = data[polygon_index_list + edge + 1];
        uint16_t delta = projected_y(end) - projected_y(start);
        uint16_t start_y = projected_y(start);
        if (observation)
            observation->span_count = start_y;
        polygon_edge_index = edge;
        edge_span_buffer = (uint16_t)polygon_left_spans;
        if ((int16_t)delta < 0) {
            uint8_t swap = start;
            start = end;
            end = swap;
            delta = -delta;
            edge_span_buffer = (uint16_t)polygon_right_spans;
        }
        coverage.source_updated = 1;
        if (observation)
            observation->changed |=
                POLYGON_VERTEX | POLYGON_COUNT | POLYGON_VERTEX_OFFSET | POLYGON_ERROR;
        edge_delta_y = delta;
        coverage.source = start;
        coverage.target = end;
        if (observation) {
            observation->start_vertex = start;
            observation->end_slot = observation->span_destination = end;
            observation->vertex_offset = start * 2;
            observation->error = end * 2;
        }
        if (delta >> 8) {
            subdivide_polygon_edge(y_spans, data, start, end, &coverage, observation);
        } else if (!delta) {
            uint16_t x = polygon_projected_x_words[start];
            if (x >= polygon_horizontal_x_max)
                polygon_horizontal_x_max = x;
            if (x < polygon_horizontal_x_min)
                polygon_horizontal_x_min = x;
            ++polygon_horizontal_edge_count;
            if (observation)
                observation->value = x;
        } else {
            struct RasterEdge input;
            input.end_y = projected_y(end);
            polygon_edge_end_y = input.end_y;
            input.start_y = projected_y(start);
            edge_start_y = input.start_y;
            input.end_x = polygon_projected_x_words[end];
            polygon_edge_end_x = input.end_x;
            input.start_x = polygon_projected_x_words[start];
            edge_start_x = input.start_x;
            input.delta_y = delta;
            if (observation)
                observation->value = edge_start_x;
            polygon_walk_edge(&coverage, observation, &input, y_spans, data);
        }
    }
    coverage.target = (coverage.target & 0xff00) | polygon_vertex_count;
    if (observation) {
        observation->end_slot = coverage.target;
        observation->span_destination = coverage.target;
    }
    if (polygon_horizontal_edge_count == polygon_vertex_count &&
        !projected_y_high[coverage.source]) {
        uint8_t y = projected_y_low[coverage.source];
        coverage.target = (coverage.target & 0xff00) | y;
        if (observation) {
            observation->end_slot = coverage.target;
            observation->span_destination = coverage.target;
        }
        if (y >= viewport_y_min && y < viewport_y_end) {
            polygon_y_min = polygon_y_max = y;
            uint16_t offset = coverage.target * 2;
            if (observation) {
                observation->error = offset;
                observation->changed |= POLYGON_ERROR;
            }
            segment_word_store(
                data, (uint16_t)polygon_left_spans + offset, polygon_horizontal_x_min);
            segment_word_store(
                data, (uint16_t)polygon_right_spans + offset, polygon_horizontal_x_max);
            projection_clip_status = 0;
            if (observation)
                observation->value = polygon_horizontal_x_max;
        }
    }
    if (observation) {
        observation->flags = status(observation->flags, projection_clip_status, 8);
        if (!projection_clip_status) {
            observation->value = (observation->value & 0xff00) | polygon_y_max;
            observation->flags = subtract8(observation->flags, polygon_y_max, polygon_y_min);
        }
    }
    coverage.visible = !(projection_clip_status || polygon_y_max < polygon_y_min);
    if (observation) {
        if (coverage.visible)
            observation->flags &= ~CF;
        else
            observation->flags |= CF;
    }
    return coverage;
}

struct PolygonScanEnd __far polygon_project_and_scan(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t initial_work,
    uint16_t previous_vertex,
    uint16_t initial_flags)
{
    struct PolygonScanEnd walk = {0};
    walk.value = initial_work;
    walk.flags = initial_flags;
    project_and_scan_polygon(data, y_spans, previous_vertex, !!(initial_flags & 0x400), &walk);
    return walk;
}

static struct PolygonCoverage triangle_coverage(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t corner_flags,
    int backwards,
    struct PolygonScanEnd *observation)
{
    uint8_t corner = corner_flags;
    uint8_t orientation = terrain_triangle_orientation;
    if (corner_flags & 0x4000) {
        corner = (corner + (TERRAIN_PROJECTED_ROW_SIZE + 1)) & TERRAIN_PROJECTED_CACHE_MASK;
        orientation += 2;
    }
    terrain_face_indices[0] = terrain_face_indices[3] = corner;
    corner ^= TERRAIN_PROJECTED_ROW_SIZE;
    terrain_face_indices[1] = corner;
    corner = (corner + terrain_triangle_vertex_delta[orientation]) & TERRAIN_PROJECTED_CACHE_MASK;
    terrain_face_indices[2] = corner;
    polygon_vertex_count = 3;
    {
        if (observation)
            observation->value = (corner_flags & 0xff00) | corner;
        struct PolygonCoverage coverage =
            project_and_scan_polygon(data, y_spans, 3, backwards, observation);
        if (!coverage.source_updated) {
            coverage.source = 3;
            coverage.source_updated = 1;
            if (observation) {
                observation->start_vertex = 3;
                observation->changed |= POLYGON_VERTEX;
            }
        }
        return coverage;
    }
}

struct PolygonCoverage __far build_polygon_coverage(SegmentBytes data,
    SegmentBytes y_spans,
    uint16_t previous_vertex,
    int backwards,
    struct PolygonScanEnd *observation)
{
    uint8_t corner = terrain_cell_x | terrain_projected_row_bank;
    uint16_t corner_flags = ((uint16_t)polygon_face_flags << 8) | corner;
    int quad = !(polygon_face_flags & 64);
    if (polygon_face_flags & 128)
        return triangle_coverage(data, y_spans, corner_flags, backwards, observation);
    if (quad) {
        terrain_face_indices[0] = terrain_face_indices[4] = corner;
        corner ^= TERRAIN_PROJECTED_ROW_SIZE;
        terrain_face_indices[1] = corner;
        ++corner;
        terrain_face_indices[2] = corner;
        corner ^= TERRAIN_PROJECTED_ROW_SIZE;
        terrain_face_indices[3] = corner;
        polygon_vertex_count = 4;
        previous_vertex = 4;
        corner_flags = (corner_flags & 0xff00) | corner;
    }
    if (observation)
        observation->value = corner_flags;
    struct PolygonCoverage coverage =
        project_and_scan_polygon(data, y_spans, previous_vertex, backwards, observation);
    if (quad && !coverage.source_updated) {
        coverage.source = 4;
        coverage.source_updated = 1;
        if (observation) {
            observation->start_vertex = 4;
            observation->changed |= POLYGON_VERTEX;
        }
    }
    return coverage;
}

struct PolygonScanEnd __far c_polygon_triangle(
    SegmentBytes data, SegmentBytes y_spans, uint16_t corner_flags, uint16_t initial_flags)
{
    struct PolygonScanEnd end = {0};
    end.flags = initial_flags;
    triangle_coverage(data, y_spans, corner_flags, !!(initial_flags & 0x400), &end);
    return end;
}

struct PolygonScanEnd __far c_polygon_build_spans(
    SegmentBytes data, SegmentBytes y_spans, uint16_t previous_vertex, uint16_t initial_flags)
{
    struct PolygonScanEnd end = {0};
    end.flags = initial_flags;
    build_polygon_coverage(data, y_spans, previous_vertex, !!(initial_flags & 0x400), &end);
    return end;
}
