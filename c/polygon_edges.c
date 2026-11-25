#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"
#define polygon_edge_end_x (((struct UnalignedWord *)&edge_end_x)->value)
#define polygon_edge_end_y (((struct UnalignedWord *)&edge_end_y)->value)

/* Integer edge walking, from the lower endpoint toward the upper endpoint.
 * Byte/word error accumulators preserve the original tie-breaking at corners.
 * The former INC/DEC and branch patches are ordinary C direction choices. */

uint8_t __far rasterize_polygon_edge(const struct RasterEdge *edge,
    SegmentBytes y_spans,
    SegmentBytes x_spans,
    struct EdgeRasterEnd *end)
{
    uint16_t end_y = edge->end_y;
    uint16_t start_y = edge->start_y;
    uint16_t dy = edge->delta_y;
    uint8_t high = start_y >> 8;
    uint8_t low = start_y;
    if (end) {
        end->value = high;
        end->compare_left = high;
    }
    if (end) {
        end->compare_right = 0;
        end->compare_subtract = 0;
    }
    if (high & 128) {
        polygon_y_min = viewport_y_min;
        if (end)
            end->value = viewport_y_min;
    } else {
        if (high)
            return 0;
        if (end) {
            end->value = low;
            end->compare_left = low;
        }
        if (end) {
            end->compare_right = viewport_y_end;
            end->compare_subtract = 1;
        }
        if (low >= viewport_y_end)
            return 0;
        if (low < polygon_y_min) {
            if (low < viewport_y_min)
                low = viewport_y_min;
            polygon_y_min = low;
        }
    }
    high = end_y >> 8;
    low = end_y;
    if (end) {
        end->value = high;
        end->compare_left = high;
    }
    if (end) {
        end->compare_right = 0;
        end->compare_subtract = 0;
    }
    if (high & 128)
        return 0;
    if (high)
        polygon_y_max = viewport_y_end - 1;
    else {
        if (end) {
            end->value = low;
            end->compare_left = low;
        }
        if (end) {
            end->compare_right = viewport_y_min;
            end->compare_subtract = 1;
        }
        if (low < viewport_y_min)
            return 0;
        if (low >= polygon_y_max)
            polygon_y_max = low >= viewport_y_end ? viewport_y_end - 1 : low;
    }
    uint16_t magnitude = edge->end_x - edge->start_x;
    int step = (int16_t)magnitude < 0 ? 1 : -1;
    if (step > 0)
        magnitude = -magnitude;
    edge_abs_delta_x = magnitude;
    uint16_t destination = edge_span_buffer + 2 * (uint8_t)end_y;
    uint16_t x = edge->end_x;
    uint16_t count;
    if (magnitude < dy) {
        uint8_t remaining_y = end_y;
        uint8_t error = (uint8_t)~((uint8_t)dy >> 1);
        count = (uint8_t)dy + 1;
        if (end_y >> 8) {
            ++remaining_y;
            while (--remaining_y) {
                if (!--count)
                    break;
                unsigned sum = (unsigned)error + (uint8_t)magnitude;
                error = sum;
                if (sum > 255) {
                    error -= (uint8_t)dy;
                    x += step;
                }
            }
            if (count) {
                destination = edge_span_buffer + 510;
                --remaining_y;
                --count;
            }
        } else {
            /* The lower endpoint is stored before the first raster step. */
            segment_word_store(y_spans, destination, x);
            destination -= 2;
            if (!--remaining_y)
                count = 0;
            else
                --count;
        }
        while (count) {
            unsigned sum = (unsigned)error + (uint8_t)magnitude;
            error = sum;
            if (sum > 255) {
                error -= (uint8_t)dy;
                x += step;
            }
            segment_word_store(y_spans, destination, x);
            destination -= 2;
            if (!--remaining_y) {
                count = 0;
                break;
            }
            --count;
        }
        if (end)
            end->count = count;
    } else {
        uint16_t remaining_y = (uint8_t)end_y;
        uint16_t error = ~(magnitude >> 1);
        int write_without_y_step = (edge_span_buffer == (uint16_t)polygon_left_spans) == (step < 0);
        count = magnitude + 1;
        if (end_y >> 8) {
            ++remaining_y;
            --count;
            while (count) {
                x += step;
                uint16_t sum = error + dy;
                int carry = sum < error;
                error = sum;
                if (carry) {
                    error -= magnitude;
                    if (!--remaining_y)
                        break;
                }
                --count;
            }
            if (count) {
                destination = edge_span_buffer + 510;
                --remaining_y;
                --count;
            }
        } else {
            /* The lower endpoint is stored even when this edge has no steps. */
            segment_word_store(x_spans, destination, x);
            --count;
        }
        while (count) {
            x += step;
            uint16_t sum = error + dy;
            int carry = sum < error;
            error = sum;
            if (carry) {
                error -= magnitude;
                destination -= 2;
                if (!--remaining_y) {
                    count = 0;
                    break;
                }
            }
            if (carry || write_without_y_step)
                segment_word_store(x_spans, destination, x);
            --count;
        }
        if (end) {
            end->error = error;
            end->remaining_y = remaining_y;
        }
        if (end) {
            end->x_major = 1;
            end->count = count;
        }
    }
    projection_clip_status = (uint8_t)count;
    if (end) {
        end->value = x;
        end->destination = destination;
        end->completed = 1;
    }
    return 1;
}

struct EdgeRasterEnd __far c_polygon_rasterize_edge(SegmentBytes y_spans, SegmentBytes x_spans)
{
    struct EdgeRasterEnd end = {0};
    /* The independently callable raw entry starts from its published inputs. */
    struct RasterEdge edge;
    edge.end_y = polygon_edge_end_y;
    edge.start_y = edge_start_y;
    edge.delta_y = edge_delta_y;
    edge.end_x = polygon_edge_end_x;
    edge.start_x = edge_start_x;
    rasterize_polygon_edge(&edge, y_spans, x_spans, &end);
    return end;
}

/* RasterWork is reset independently of other subsystems and data placement. */
struct RasterWork raster_work = {0};

void __far reset_raster_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&raster_work), 0, sizeof(raster_work), 0);
}

/* Generation uses byte views; rasterization uses word boundaries. */
union SpanWorkspace left_span_workspace = {0}, right_span_workspace = {0};

/* 96 word-sized polygon X values indexed by projected vertex ID; 20BAh assembles split X bytes, adds projection_x_bias, arithmetic-shifts right four times, and stores here for edge rasterization */
uint16_t polygon_projected_x_words[96] = {0};

/* 48 additional zero words after the declared 96-word projected-X cache. Terrain uses IDs 00h..3Fh; all ten model ranges fit the 40h..5Fh cache, so validated face loops never address this trailing area. The raw extent ends at object_stack_links. */
uint8_t polygon_projected_x_capacity_tail[96] = {0};

/* 96 split-coordinate cache entries: low byte of projected Y; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices */
uint8_t projected_y_low[96] = {0};

/* 96 split-coordinate cache entries: low byte of projected X; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices */
uint8_t projected_x_low[96] = {0};

/* 96 split-coordinate cache entries: high byte of projected Y; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices */
uint8_t projected_y_high[96] = {0};

/* 96 split-coordinate cache entries: high byte of projected X; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices */
uint8_t projected_x_high[96] = {0};

/* five-byte terrain polygon loop; quads use four indices plus a repeat of index 0, triangles use three indices plus a repeat in slot 3. Mesh drawing restores polygon_index_list to this scratch loop afterward. */
uint8_t terrain_face_indices[7] = {0};

void __far reset_polygon_arrays(SegmentBytes output)
{
    c_memory_fill(
        output + ((uint16_t)&polygon_projected_x_words), 0, sizeof(polygon_projected_x_words), 0);
    c_memory_fill(output + ((uint16_t)&polygon_projected_x_capacity_tail),
        0,
        sizeof(polygon_projected_x_capacity_tail),
        0);
    c_memory_fill(output + ((uint16_t)&left_span_workspace), 0, sizeof(left_span_workspace), 0);
    c_memory_fill(output + ((uint16_t)&right_span_workspace), 0, sizeof(right_span_workspace), 0);
    c_memory_fill(output + ((uint16_t)&projected_y_low), 0, sizeof(projected_y_low), 0);
    c_memory_fill(output + ((uint16_t)&projected_x_low), 0, sizeof(projected_x_low), 0);
    c_memory_fill(output + ((uint16_t)&projected_y_high), 0, sizeof(projected_y_high), 0);
    c_memory_fill(output + ((uint16_t)&projected_x_high), 0, sizeof(projected_x_high), 0);
    c_memory_fill(output + ((uint16_t)&terrain_face_indices), 0, sizeof(terrain_face_indices), 0);
}
