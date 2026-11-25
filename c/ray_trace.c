#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* March cursor/sentry rays in24-bit positions. Native results describe contact
 * and full indices; direct-entry observations are captured only when requested. */
#include "ray_position.h"

static void advance_ray_position(struct RayPosition *position, uint16_t step)
{
    uint32_t value = load_ray_position(position);
    value += (int16_t)step;
    store_ray_position(position, value);
}

static struct SurfaceSample ray_sample_surface(
    uint8_t x, uint8_t y, uint16_t *position, struct WorldRayEnd *capture)
{
    struct WorldHeightEnd raw;
    struct SurfaceSample surface =
        capture ? sample_world_surface(*position, &raw) : sample_surface_at(x, y, *position, 0);
    *position = (*position & 0xff00) | surface.selected_index;
    if (capture) {
        capture->cell_bank = raw.bank;
        capture->cell_offset = *position;
        capture->cell_sampled = 1;
        if (surface.stack) {
            capture->height_work &= 0xff00;
            if (raw.quarter_set)
                capture->height_work = (uint16_t)raw.quarter << 8;
        }
        capture->value = raw.value;
    }
    return surface;
}

static struct RayContact trace_world_ray(
    SegmentBytes data, uint16_t observer_bank, uint16_t position_index, struct WorldRayEnd *capture)
{
    struct RayContact result = {observer_bank, position_index, 0};
    result.observer_index = (result.observer_index & 0xff00) | view_object_slot;
    ray_target_hit_flags >>= 1;
    ray_intervening_hit_flags >>= 1;
    c_ray_object_origin(result.observer_index);
    for (;;) {
        advance_ray_position(&ray_position_x, segment_word(data, (uint16_t)&occlusion_ray_step_x));
        advance_ray_position(
            &ray_position_height, segment_word(data, (uint16_t)&occlusion_ray_step_height));
        advance_ray_position(&ray_position_y, segment_word(data, (uint16_t)&occlusion_ray_step_y));
        /* Preserve the distinct supplied-segment word read, even without AX capture. */
        uint16_t last_step_y = segment_word(data, (uint16_t)&occlusion_ray_step_y);
        if (capture) {
            capture->value = last_step_y;
            capture->height_work = (int16_t)last_step_y < 0 ? 0xffff : 0;
        }
        uint8_t sample_x = ray_position_x.integer;
        terrain_scan_x = sample_x;
        if (capture) {
            capture->value = (capture->value & 0xff00) | terrain_scan_x;
            capture->flags = subtract8(capture->flags, terrain_scan_x, TERRAIN_CELL_SIDE);
        }
        if (terrain_scan_x >= TERRAIN_CELL_SIDE)
            goto reject;
        uint8_t sample_y = ray_position_y.integer;
        terrain_scan_y = sample_y;
        if (capture) {
            capture->value = (capture->value & 0xff00) | terrain_scan_y;
            capture->flags = subtract8(capture->flags, terrain_scan_y, TERRAIN_CELL_SIDE);
        }
        if (terrain_scan_y >= TERRAIN_CELL_SIDE)
            goto reject;
        ray_sample_mode_flags = 128;
        edge_delta_y = (edge_delta_y & 0xff00) | 128;
        ray_sample_fraction = ray_sample_special_flags = 0;
        struct SurfaceSample sample =
            ray_sample_surface(sample_x, sample_y, &result.position_index, capture);
        uint8_t height = sample.height;
        /* Keep the complete selected index comparison: do not narrow its bank. */
        if (sample.stack || sample.selected_index == 0) {
            uint16_t ray_height =
                ((uint16_t)ray_position_height.integer << 8) | ray_position_height.fraction_high;
            uint16_t surface_height = ((uint16_t)height << 8) | ray_sample_fraction;
            uint16_t height_difference = surface_height - ray_height;
            if (capture)
                capture->height_work = ray_height;
            ray_sample_fraction = (uint8_t)height_difference;
            uint8_t integer_difference = height_difference >> 8;
            if (integer_difference & 128)
                continue;
            if (capture) {
                capture->value = (capture->value & 0xff00) | integer_difference;
                capture->flags = status(capture->flags, integer_difference, 8);
            }
            if (integer_difference)
                goto reject;
            /* Publication and each live threshold operand remain separate reads. */
            uint8_t contact_fraction = ray_sample_fraction;
            if (capture) {
                capture->value = (capture->value & 0xff00) | contact_fraction;
                capture->flags =
                    subtract8(capture->flags, ray_sample_fraction, (uint8_t)edge_delta_y);
            } else {
                (void)ray_sample_fraction;
                (void)(uint8_t)edge_delta_y;
            }
            if (ray_sample_fraction >= (uint8_t)edge_delta_y)
                goto reject;
            if (ray_sample_mode_flags & 64)
                goto reject;
            if (!((ray_target_test_flags | ray_sample_special_flags) & 128) &&
                !(data[((uint16_t)&ray_work + 3)] & 128))
                goto reject;
            result.observer_index = (result.observer_index & 0xff00) | view_object_slot;
            if (terrain_scan_x == object_grid_x[result.observer_index] &&
                terrain_scan_y == object_grid_y[result.observer_index])
                continue;
            result.contact = 1;
            if (capture)
                capture->flags &= ~CF;
            break;
        }
        ray_corner_heights[0] = ray_corner_height_wrap = height;
        ray_sample_mode_flags &= 127;
        ++sample_x;
        ++terrain_scan_x;
        sample = ray_sample_surface(sample_x, sample_y, &result.position_index, capture);
        ray_corner_heights[3] = sample.height;
        ++sample_y;
        ++terrain_scan_y;
        sample = ray_sample_surface(sample_x, sample_y, &result.position_index, capture);
        ray_corner_heights[2] = sample.height;
        --sample_x;
        --terrain_scan_x;
        sample = ray_sample_surface(sample_x, sample_y, &result.position_index, capture);
        ray_corner_heights[1] = sample.height;
        --sample_y;
        --terrain_scan_y;
        struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        if (capture) {
            capture->cell_bank = cell.bank;
            capture->cell_offset = cell.offset;
            capture->cell_sampled = 1;
            capture->flags = subtract8(capture->flags, cell.value.raw, 0xc0) ^ CF;
        }
        uint8_t cell_shape = cell.value.terrain.shape;
        if (cell_shape == 4 || cell_shape == 12) {
            if (capture)
                capture->value = (cell.bank & 0xff00) | cell.value.raw;
            int above = 0;
            for (unsigned i = 0; i < 4; ++i)
                if (ray_position_height.integer >= ray_corner_heights[i])
                    above = 1;
            if (above)
                continue;
            goto reject;
        } else {
            uint8_t edge_selector;
            if ((cell_shape & 3) == 1)
                edge_selector = (cell_shape >> 2) & 1;
            else {
                uint8_t edge_group =
                    (cell_shape & 3) == 3 ? ((cell_shape >> 2) + 2) & 3 : cell_shape >> 2;
                ray_contact_work = edge_group;
                uint8_t diagonal_x = ray_position_x.fraction_high;
                if (edge_group & 1)
                    diagonal_x = ~diagonal_x;
                uint16_t selector_index = (result.position_index & 0xff00) |
                    (uint8_t)(edge_group * 2 + (diagonal_x >= ray_position_y.fraction_high));
                if (capture)
                    capture->cell_offset = selector_index;
                edge_selector = ray_contact_edge_selectors[selector_index];
            }
            uint16_t corner_index = (result.observer_index & 0xff00) | edge_selector;
            result.observer_index = corner_index;
            uint8_t coordinate = ray_position_x.fraction_high;
            if (!(edge_selector & 1))
                coordinate = ray_position_y.fraction_high;
            result.position_index = (result.position_index & 0xff00) | coordinate;
            uint8_t interpolation_coordinate =
                edge_selector & 2 ? (uint8_t)~coordinate : coordinate;
            ray_contact_fraction = interpolation_coordinate;
            uint8_t base = ray_corner_heights[corner_index];
            ray_contact_work = base;
            uint8_t delta = ray_corner_heights[corner_index + 1] - base;
            ray_corner_heights[2] = (int8_t)delta < 0 ? (uint8_t)-delta : delta;
            uint16_t product = (uint16_t)interpolation_coordinate * math_work_hi;
            math_work_lo = (uint8_t)product;
            uint8_t height_offset = product >> 8;
            if ((int8_t)delta < 0) {
                uint16_t magnitude = ((uint16_t)height_offset << 8) | math_work_lo;
                uint16_t negative_product = -magnitude;
                math_work_lo = (uint8_t)negative_product;
                math_work_hi = negative_product >> 8;
                height_offset = negative_product >> 8;
            }
            uint8_t surface_height = height_offset + base;
            ray_corner_heights[2] = surface_height;
            uint16_t ray_height = segment_word(data, (uint16_t)&ray_position_height.fraction_high);
            uint16_t sampled_height = segment_word(data, (uint16_t)ray_corner_heights + 1);
            uint16_t height_difference = ray_height - sampled_height;
            if (capture) {
                capture->value = height_difference;
                capture->flags = subtract16(capture->flags, ray_height, sampled_height);
            }
            if ((int16_t)height_difference < 0)
                goto reject;
        }
    }
    goto finish;
reject:
    if (capture)
        capture->flags |= CF;
finish:
    if (capture) {
        capture->observer_index = result.observer_index;
        capture->position_index = result.position_index;
    }
    return result;
}

struct RayContact __far trace_world_contact(
    SegmentBytes data, uint16_t observer_bank, uint16_t position_index)
{
    return trace_world_ray(data, observer_bank, position_index, 0);
}

struct WorldRayEnd __far c_trace_world_ray(
    SegmentBytes data, uint16_t observer_bank, uint16_t position_index, uint16_t initial_flags)
{
    struct WorldRayEnd end = {0};
    end.flags = initial_flags;
    trace_world_ray(data, observer_bank, position_index, &end);
    return end;
}
